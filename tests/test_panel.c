#define BB_ENABLE_OLED 1
#define BB_ENCODER_A 6
#define BB_ENCODER_B 7
#define BB_ENCODER_SW 8
#define BB_ENCODER_STEPS 4
#include "../src/control_panel.c"
#include "config_store.h"
#include "hardware/flash.h"
#include <assert.h>

unsigned test_core=1;
uint32_t test_time;
bool test_gpio[30];
uint8_t test_flash[PICO_FLASH_SIZE_BYTES];
static unsigned reenumerations;
void bb_oled_init(void) {}
void bb_oled_poll(uint32_t now) { (void)now; }
bool bb_oled_available(void) { return true; }
void bb_oled_text(const char lines[8][22]) { (void)lines; }
void bb_usb_request_reenumeration(void) { ++reenumerations; }
void bb_led_signal_profile_changed(void) {}
bool bb_bt_host_pairing_available(void) { return true; }
void bb_platform_start_pairing(void) { bb_state_set_pairing(true); }
void bb_platform_stop_pairing(void) { bb_state_set_pairing(false); }
bb_controller_kind_t bb_controller_identify(uint16_t vid,uint16_t pid,const char* name) {
    (void)vid;(void)pid;(void)name;return BB_CONTROLLER_GENERIC;
}
const char* bb_controller_button_label(bb_controller_kind_t kind,const char* name,uint8_t index) {
    (void)kind;(void)name;(void)index;return "SOUTH";
}
static void tick(void) { test_time+=20; bb_panel_poll(); }
static void raw(uint32_t buttons) {
    test_core=0;
    bb_state_update_live_input(0,0,0,0,0,0,0,buttons);
    test_core=1;
}
int main(void) {
    memset(test_flash,255,sizeof(test_flash));
    memset(test_gpio,1,sizeof(test_gpio));
    bb_state_init();
    uint8_t ci,pi;
    uint8_t address[6]={1,2,3,4,5,6};
    assert(bb_controller_activate_or_create(address,1,2,"Test pad",&ci));
    bb_state_set_controller(true,0,"Test pad");
    bb_panel_init();
    assert(strstr(rows[1],"MiSTer"));
    item=USB_MODE; menu=true; begin_edit(); rotate(1);
    assert(value==1 && bb_output_mode()==BB_OUTPUT_MISTER && reenumerations==0);
    apply(); assert(bb_output_mode()==BB_OUTPUT_XINPUT && reenumerations==1);
    snapshot(); item=TURBO; begin_edit(); rotate(1); apply(); snapshot();
    assert(profile()->turbo_enabled);
    item=TURBO_RATE; begin_edit(); rotate(100); assert(value==30);
    rotate(-100); assert(value==1); apply(); snapshot(); assert(profile()->turbo_rate_hz==1);
    item=PEW; begin_edit(); rotate(1); apply(); snapshot(); assert(effect==0x80);
    assert(bb_profile_duplicate(0,"Arcade",&pi)); snapshot();
    item=PROFILE; begin_edit(); rotate(1);
    assert(controller.active_profile==0); /* Preview has no persistent effect. */
    apply(); snapshot(); assert(controller.active_profile==1);
    item=TURBO_RATE; begin_edit();
    assert(bb_profile_select(0)); snapshot(); assert(!editing); /* External edit cancels stale preview. */
    menu=false; raw(0); raw(1); tick();
    assert(pew_active && strstr(rows[6],"PEW PEW!"));
    raw(1); test_time+=400; tick(); assert(!pew_active);
    raw(0); raw(1); tick(); assert(!pew_active); /* Rate limited. */
    test_time+=400; raw(0); raw(1); tick(); assert(pew_active);
    menu=true; tick(); assert(!pew_active); /* Menu takes priority. */
    item=PAIR; begin_edit(); apply(); snapshot(); assert(runtime.pairing);

    /* Reboot into multi-controller mode and check mode/turbo restrictions. */
    assert(bb_multi_mode_set(true)); bb_config_flush_pending(); bb_state_init();
    snapshot(); assert(bb_multi_mode() && bb_output_mode()==BB_OUTPUT_MISTER);
    editing=false; item=USB_MODE; begin_edit(); assert(!editing);
    item=TURBO; begin_edit(); assert(!editing);
    item=PEW; begin_edit(); assert(editing); /* Cosmetic setting remains available. */
    puts("panel: mode confirmation, turbo settings, profile edits, PEW timing and multi-mode restrictions passed");
}
