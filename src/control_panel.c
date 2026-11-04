#include "control_panel.h"
#if BB_ENABLE_OLED
#include "oled_display.h"
#include "panel_controls.h"
#if BB_USE_TRACKBALL
#include "trackball.h"
#endif
#include "bluebridge_state.h"
#include "bluebridge_platform.h"
#include "controller_input.h"
#include "oled_gamepad.h"
#include "bt_host.h"
#include "usb_device.h"
#include "led_status.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

enum { USB_MODE, CONTROLLER, PROFILE, TURBO, TURBO_RATE, TURBO_CONTROL,
       TURBO_BUTTON, TURBO_OUTPUT, PEW, FIRE_BUTTON, PAIR, MULTI, DISPLAY, BACK, ITEM_COUNT };
static const char* const titles[] = {"USB MODE", "CONTROLLER", "PROFILE", "TURBO",
    "TURBO RATE", "TURBO CONTROL", "TURBO BUTTON", "TURBO OUTPUT", "PEW PEW",
    "FIRE BUTTON", "PAIRING", "CONTROLLER MODE", "DISPLAY SCREEN", "BACK"};
static const char* const turbo_modes[] = {"MODIFIER", "TOGGLE", "HOLD"};
#if BB_USE_ENCODER
static bb_encoder_t encoder;
static volatile int movement;
static bb_panel_button_t button;
#endif
#if BB_USE_TRACKBALL
static bb_panel_button_t trackball_button;
#endif
static bb_controller_t controller;
static bb_runtime_state_t runtime;
static char rows[8][22];
static bool menu, editing;
static unsigned item;
static int value;
static uint32_t edit_controller, edit_profile;
static uint32_t seen_pew[BB_MAX_SLOTS], last_pew, pew_at;
static bool pew_active, have_pew;
static uint32_t last_snapshot;
static uint8_t effect;
static bool initialized;
static bool live_screen = true;
static uint32_t last_live_frame;
static bb_pad_picture_t last_picture;
static char last_footer[22];
static bool picture_valid;

#if BB_USE_ENCODER
static uint8_t encoder_pins(void) {
    return (uint8_t)((gpio_get(BB_ENCODER_A) ? 2 : 0) | (gpio_get(BB_ENCODER_B) ? 1 : 0));
}
static void encoder_irq(uint gpio, uint32_t events) {
    (void)gpio; (void)events;
    int step = bb_encoder_update(&encoder, encoder_pins(), BB_ENCODER_STEPS);
    int next = movement + step;
    if (next >= -32 && next <= 32) movement = next;
}
#endif
static void snapshot(void) {
    if(menu)bb_active_controller_get(&controller, NULL);
    else bb_active_controller_summary_get(&controller);
    bb_state_get(&runtime);
    if (runtime.controller_index != bb_selected_controller_index()) {
        runtime.controller_connected = false;
        runtime.battery_supported = false;
        runtime.controller_index = -1;
    }
    effect = bb_profile_oled_get(controller.active_profile);
    /* Cancel an edit if management changed its target in the meantime. */
    if (editing && (controller.id != edit_controller ||
        controller.profiles[controller.active_profile].id != edit_profile)) editing = false;
}
static const bb_profile_t* profile(void) { return &controller.profiles[controller.active_profile]; }
static int wrap(int v, int count) { return count > 0 ? (v % count + count) % count : 0; }
static const char* button_label(unsigned index) {
    bb_controller_kind_t kind = bb_controller_identify(controller.native_vendor_id, controller.native_product_id, controller.name);
    return bb_controller_button_label(kind, controller.name, (uint8_t)index);
}
static bool needs_profile(unsigned choice) { return choice >= PROFILE && choice <= FIRE_BUTTON; }
static bool turbo_item(unsigned choice) { return choice >= TURBO && choice <= TURBO_OUTPUT; }
static bool blocked(unsigned choice) {
    return (needs_profile(choice) && !controller.profile_count) ||
           (bb_multi_mode() && (choice == USB_MODE || turbo_item(choice)));
}
static void begin_edit(void) {
    if (blocked(item)) return;
    const bb_profile_t* p = profile();
    switch (item) {
        case USB_MODE: value = bb_output_mode() - 1; break;
        case CONTROLLER: value = bb_selected_controller_index(); break;
        case PROFILE: value = controller.active_profile; break;
        case TURBO: value = p->turbo_enabled; break;
        case TURBO_RATE: value = p->turbo_rate_hz; break;
        case TURBO_CONTROL: value = p->turbo_control_mode; break;
        case TURBO_BUTTON: value = p->turbo_control_mode ? p->turbo_control_button : p->turbo_modifier; break;
        case TURBO_OUTPUT: value = 0; break;
        case PEW: value = (effect & 0x80u) != 0; break;
        case FIRE_BUTTON: value = effect & 31u; break;
        case PAIR: value = !runtime.pairing; break;
        case MULTI: value = !bb_multi_mode(); break;
        case DISPLAY: value = live_screen; break;
        case BACK: menu = false; return;
    }
    edit_controller = controller.id;
    edit_profile = profile()->id;
    editing = true;
}
static void rotate(int delta) {
    if (!menu) { menu = true; item = 0; return; }
    if (!editing) { item = (unsigned)wrap((int)item + delta, ITEM_COUNT); return; }
    int count = 2;
    switch (item) {
        case USB_MODE: count = 5; break;
        case CONTROLLER: count = bb_controller_count(); break;
        case PROFILE: count = controller.profile_count; break;
        case TURBO_CONTROL: count = 3; break;
        case FIRE_BUTTON: case TURBO_BUTTON: count = BB_BUTTON_COUNT; break;
        case TURBO_OUTPUT: count = BB_OUTPUT_BUTTON_COUNT; break;
        case TURBO_RATE:
            value += delta;
            if (value < 1) value = 1;
            if (value > 30) value = 30;
            return;
    }
    value = wrap(value + delta, count);
}
static void apply(void) {
    const bb_profile_t* p = profile();
    uint8_t pi = controller.active_profile;
    bool ok = false;
    uint16_t old_identity = bb_current_mister_identity();
    switch (item) {
        case USB_MODE:
            if (value + 1 == (int)bb_output_mode()) ok = true;
            else if ((ok = bb_output_mode_set((bb_output_mode_t)(value + 1)))) bb_usb_request_reenumeration();
            break;
        case CONTROLLER: ok = bb_controller_select((uint8_t)value); break;
        case PROFILE:
            ok = bb_profile_select((uint8_t)value);
            if (ok && bb_output_mode() == BB_OUTPUT_MISTER && old_identity != bb_current_mister_identity()) bb_usb_request_reenumeration();
            if (ok) bb_led_signal_profile_changed();
            break;
        case TURBO: case TURBO_CONTROL:
            ok = bb_profile_set_turbo_control(pi, item == TURBO ? value : p->turbo_enabled,
                item == TURBO_CONTROL ? value : p->turbo_control_mode, p->turbo_control_button);
            break;
        case TURBO_BUTTON:
            ok = p->turbo_control_mode ? bb_profile_set_turbo_control(pi, p->turbo_enabled, p->turbo_control_mode, (uint8_t)value)
                : bb_profile_set_turbo_modifier(pi, (uint8_t)value);
            break;
        case TURBO_RATE: case TURBO_OUTPUT:
            ok = bb_profile_set_tuning(pi, p->invert_x, p->invert_y, p->invert_rx, p->invert_ry,
                p->deadzone_left, p->deadzone_right, p->trigger_deadzone,
                item == TURBO_RATE ? value : p->turbo_rate_hz,
                item == TURBO_OUTPUT ? p->turbo_mask ^ (1u << value) : p->turbo_mask);
            break;
        case PEW: case FIRE_BUTTON:
            ok = bb_profile_set_oled(pi, item == PEW ? value : (effect & 0x80u) != 0,
                item == FIRE_BUTTON ? (uint8_t)value : (uint8_t)(effect & 31u));
            break;
        case PAIR:
            if (!value) { bb_platform_stop_pairing(); ok = true; }
            else if (bb_bt_host_pairing_available()) { bb_platform_start_pairing(); ok = true; }
            break;
        case MULTI: ok = bb_multi_mode_set(value != 0); break;
        case DISPLAY: live_screen = value != 0; ok = true; break;
    }
    if (ok) editing = false;
}
static void value_text(unsigned choice, int v, char* out, size_t size) {
    switch (choice) {
        case USB_MODE: snprintf(out,size,"%s",bb_output_mode_name((bb_output_mode_t)(v+1))); break;
        case CONTROLLER: bb_controller_name_get((uint8_t)v,out,size); break;
        case PROFILE: snprintf(out,size,"%s",controller.profiles[v].name); break;
        case TURBO_RATE: snprintf(out,size,"%d HZ",v); break;
        case TURBO_CONTROL: snprintf(out,size,"%s",turbo_modes[v]); break;
        case FIRE_BUTTON: case TURBO_BUTTON: snprintf(out,size,"%s",button_label((unsigned)v)); break;
        case TURBO_OUTPUT: snprintf(out,size,"%s %s",button_label((unsigned)v),profile()->turbo_mask & (1u<<v) ? "[X]" : "[ ]"); break;
        case PAIR: snprintf(out,size,"%s",v ? "START" : "STOP"); break;
        case MULTI: snprintf(out,size,"%s",v ? "TWO CONTROLLERS" : "SINGLE CONTROLLER"); break;
        case DISPLAY: snprintf(out,size,"%s",v ? "LIVE CONTROLLER" : "STATUS"); break;
        default: snprintf(out,size,"%s",v ? "[X] ENABLED" : "[ ] DISABLED"); break;
    }
}
static void draw(uint32_t now) {
    if(!bb_oled_enabled_current())return;
    if (!menu && pew_active) {
        picture_valid=false;
        bb_oled_pew((uint8_t)((uint32_t)(now-pew_at)/100u));
        return;
    }
    if (!menu && live_screen && runtime.controller_connected) {
        /* A live screen is intentionally active during play. Cap picture changes
         * to 20 fps; idle pixels still generate no I2C traffic. */
        if ((uint32_t)(now-last_live_frame)<50u) return;
        last_live_frame=now;
        bb_controller_kind_t kind=bb_controller_identify(controller.native_vendor_id,controller.native_product_id,controller.name);
        bb_pad_picture_t v={0};
        v.layout=bb_pad_layout(kind);
        v.buttons=runtime.raw_buttons;v.dpad=runtime.raw_dpad;
        v.x=(int8_t)((int32_t)runtime.raw_x*4/32767);v.y=(int8_t)((int32_t)runtime.raw_y*4/32767);
        v.rx=(int8_t)((int32_t)runtime.raw_rx*4/32767);v.ry=(int8_t)((int32_t)runtime.raw_ry*4/32767);
        v.lt=(uint8_t)(runtime.raw_lt*8u/255u);v.rt=(uint8_t)(runtime.raw_rt*8u/255u);
        uint16_t turbo=0;
        if(!bb_multi_mode() && profile()->turbo_enabled) {
            if(profile()->turbo_control_mode)turbo=profile()->turbo_mask;
            else if(runtime.turbo_profile_id==profile()->id)turbo=runtime.turbo_assignments;
        }
        /* Rings belong to physical inputs, even when remapped to another output.
         * Macro trigger buttons inherit a ring if any macro output has turbo. */
        for(unsigned i=0;i<BB_BUTTON_COUNT;i++) {
            unsigned target=profile()->button_map[i];
            uint16_t mask=target<BB_OUTPUT_BUTTON_COUNT?(uint16_t)(1u<<target):0;
            if(target>=BB_MAP_MACRO1&&target<=BB_MAP_MACRO4) {
                const bb_macro_t* macro=&profile()->macros[target-BB_MAP_MACRO1];
                if(macro->enabled)mask=macro->output_mask;
            }
            if(mask&turbo)v.turbo_buttons|=1u<<i;
        }
        char footer[22];
        snprintf(footer,sizeof(footer),"%.15s%s",profile()->name,turbo?" TURBO":"");
        if(!picture_valid || memcmp(&last_picture,&v,sizeof(v)) || strcmp(last_footer,footer)) {
            bb_pad_render(bb_oled_graphics_begin(),&v);
            bb_oled_graphics_footer(footer);
            last_picture=v;memcpy(last_footer,footer,sizeof(last_footer));picture_valid=true;
        }
        return;
    }
    picture_valid=false;
    memset(rows, 0, sizeof(rows));
    if (!menu) {
        char name[BB_MAX_NAME];
        bb_controller_name_get(bb_selected_controller_index(),name,sizeof(name));
        snprintf(rows[0],22,"%.21s",controller.profile_count ? name : "NO REMEMBERED PAD");
        snprintf(rows[2],22,"%.21s",runtime.pairing ? "PAIRING..." : runtime.controller_connected ? profile()->name : "WAITING FOR PAD");
        if (bb_multi_mode()) snprintf(rows[4],22,"DUAL MODE - NO TURBO");
        else snprintf(rows[4],22,"TURBO: %s %u HZ",profile()->turbo_enabled ? "ON" : "OFF",profile()->turbo_rate_hz);
        if (runtime.battery_supported) snprintf(rows[6],22,"BATTERY: %u%%",runtime.battery_percent);
        snprintf(rows[7],22,"PRESS FOR MENU");
    } else if (editing) {
        snprintf(rows[0],22,"SETTINGS");
        snprintf(rows[2],22,"%s",titles[item]);
        value_text(item,value,rows[4],22);
        snprintf(rows[6],22,"PRESS TO APPLY");
        snprintf(rows[7],22,"HOLD TO CANCEL");
    } else {
        snprintf(rows[0],22,"SETTINGS");
        unsigned first = item / 5 * 5;
        for (unsigned n = 0; n < 5 && first+n < ITEM_COUNT; ++n)
            snprintf(rows[n+2],22,"%c%s%s",first+n == item ? '>' : ' ',titles[first+n],blocked(first+n) ? " -" : "");
        snprintf(rows[7],22,"HOLD FOR STATUS");
    }
    bb_oled_text(rows);
}

void bb_panel_init(void) {
    if(bb_oled_enabled_current())bb_oled_init();else bb_oled_init_off();
#if BB_USE_ENCODER
    if(bb_panel_input_current()==BB_PANEL_ENCODER){
    const uint pins[] = {BB_ENCODER_A,BB_ENCODER_B,BB_ENCODER_SW};
    for (unsigned i=0;i<3;++i) { gpio_init(pins[i]); gpio_set_dir(pins[i],GPIO_IN); gpio_pull_up(pins[i]); }
    encoder.previous = encoder_pins();
    gpio_set_irq_enabled_with_callback(BB_ENCODER_A,GPIO_IRQ_EDGE_RISE|GPIO_IRQ_EDGE_FALL,true,encoder_irq);
    gpio_set_irq_enabled(BB_ENCODER_B,GPIO_IRQ_EDGE_RISE|GPIO_IRQ_EDGE_FALL,true);
    }
#endif
#if BB_USE_TRACKBALL
    if(bb_panel_input_current()==BB_PANEL_TRACKBALL)bb_trackball_init();
#endif
    initialized = true;
    snapshot();
    for (unsigned s=0;s<BB_MAX_SLOTS;++s) { bb_runtime_state_t r; bb_state_get_slot(s,&r); seen_pew[s]=r.pew_events; }
    draw(to_ms_since_boot(get_absolute_time()));
}
void bb_panel_poll(void) {
    if (!initialized) return;
    uint32_t now = to_ms_since_boot(get_absolute_time());
    bb_oled_poll(now);
    if(!bb_oled_enabled_current() && bb_panel_input_current()==BB_PANEL_NONE)return;
    int delta=0,press=0;
#if BB_USE_ENCODER
    if(bb_panel_input_current()==BB_PANEL_ENCODER){
    uint32_t saved = save_and_disable_interrupts();
    delta = movement;
    movement = 0;
    restore_interrupts(saved);
    press = bb_panel_button_update(&button,!gpio_get(BB_ENCODER_SW),now);
    }
#endif
#if BB_USE_TRACKBALL
    if(bb_panel_input_current()==BB_PANEL_TRACKBALL){
    /* Colour requests are cached by the driver; unchanged LEDs are not written. */
    if(pew_active)bb_trackball_colour(32,0,0,0);
    else if(editing)bb_trackball_colour(0,0,0,20);
    else if(menu)bb_trackball_colour(16,0,16,0);
    else if(runtime.pairing)bb_trackball_colour(0,0,32,0);
    else if(runtime.controller_connected)bb_trackball_colour(0,24,0,0);
    else bb_trackball_colour(24,6,0,0);
    bb_trackball_poll(now);
    if(bb_trackball_available()) {
        bb_trackball_event_t event;
        bb_trackball_take(&event);
        int click=bb_panel_button_update(&trackball_button,event.down,now);
        if(event.clicks&&!trackball_button.stable&&!trackball_button.observed)click=1;
        if(click>press)press=click;
        /* A click never also changes the currently selected value. */
        if(!event.down&&!click)delta+=editing?event.x:event.y;
        if(!menu&&!event.down&&!click&&event.x)delta+=event.x;
    } else {
        /* Loss of the device while held must not synthesize an Apply click. */
        trackball_button=(bb_panel_button_t){0};
    }
    }
#endif
    bool refresh = delta || press || (uint32_t)(now-last_snapshot) >= 20u;
    if (!refresh) return;
    last_snapshot = now;
    snapshot();
    if (delta) rotate(delta);
    if (press == 2) { if (editing) editing=false; else menu=false; }
    if (press == 1) {
        if (!menu) { menu=true; item=0; }
        else if (editing) apply();
        else begin_edit();
        snapshot();
    }
    for (unsigned s=0;s<(bb_multi_mode()?BB_MAX_SLOTS:1);++s) {
        bb_runtime_state_t r;
        bb_state_get_slot(s,&r);
        if (r.pew_events != seen_pew[s]) {
            seen_pew[s]=r.pew_events;
            if (bb_oled_enabled_current() && r.controller_index == runtime.controller_index && !menu && !pew_active &&
                (!have_pew || (uint32_t)(now-last_pew)>=750u)) {
                pew_active=true; have_pew=true; pew_at=last_pew=now;
            }
        }
    }
    if (pew_active && (menu || (uint32_t)(now-pew_at)>=400u || !(effect&0x80u) || !runtime.controller_connected)) pew_active=false;
    draw(now); /* Text/glyph and page comparisons suppress all unchanged traffic. */
}
bool bb_panel_trackball_ready(void){
#if BB_USE_TRACKBALL
    return bb_panel_input_current()==BB_PANEL_TRACKBALL&&bb_trackball_available();
#else
    return false;
#endif
}
#else
void bb_panel_init(void) {}
void bb_panel_poll(void) {}
bool bb_panel_trackball_ready(void){return false;}
#endif
