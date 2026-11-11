#include "bluebridge_state.h"
#include "controller_input.h"
#include "hardware/flash.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
unsigned test_core, test_flash_erases;
int test_flash_error;
uint32_t test_time;
uint8_t test_flash[PICO_FLASH_SIZE_BYTES];
static bb_gamepad_report_t sent[BB_MAX_SLOTS];
void bb_usb_submit(const bb_gamepad_report_t* r) { sent[bb_slot_current()]=*r; }
int main(void) {
    memset(test_flash,255,sizeof(test_flash));bb_state_init();
    uint8_t a[6]={1},b[6]={2},first,second;
    assert(bb_controller_activate_or_create(a,0x057e,0x2019,"N64",&first));
    bb_state_set_controller(true,first,"N64");
    assert(bb_profile_set_mapping(0,10,7));
    assert(bb_profile_set_mapping(0,7,BB_MAP_DISABLED));
    bb_input_state_t input={.rt=180,.x=32767};
    assert(bb_profile_set_tuning(0,1,0,0,0,8,8,4,12,0));
    bb_input_submit(&input);assert(sent[0].rt>100&&sent[0].x<0);
    input.rt=0;input.x=100;bb_input_submit(&input);assert(!sent[0].rt&&!sent[0].x);
    assert(bb_profile_set_macro(0,0,1,"Chord",(1u<<2)|(1u<<6)));
    assert(bb_profile_set_mapping(0,1,BB_MAP_MACRO1));
    input.buttons=1u<<1;bb_input_submit(&input);
    assert(sent[0].buttons==((1u<<2)|(1u<<6))&&sent[0].lt==255);
    assert(bb_capture_start(BB_CAPTURE_TESTER));bb_input_submit(&input);assert(!sent[0].buttons&&!sent[0].lt);
    bb_capture_stop();
    assert(bb_controller_activate_or_create(b,0x054c,0x0ce6,"DualSense",&second));
    assert(bb_controller_select(second));assert(bb_profile_set_mapping(0,0,3));
    assert(bb_multi_mode_set(true));test_time+=250;bb_config_flush_pending();bb_state_init();
    assert(bb_multi_mode());
    bb_slot_enter(0);bb_state_set_controller(true,first,"N64");
    bb_slot_enter(1);bb_state_set_controller(true,second,"DualSense");
    bb_input_config_t config;
    bb_slot_enter(0);bb_input_config_get(&config);assert(config.product_id==0x2019);
    input=(bb_input_state_t){.buttons=1u<<1};bb_input_submit(&input);
    assert(!sent[0].buttons); /* Macros disabled in multi mode. */
    bb_slot_enter(1);bb_input_config_get(&config);assert(config.product_id==0x0ce6);
    input=(bb_input_state_t){.buttons=1};bb_input_submit(&input);assert(sent[1].buttons==(1u<<3));
    bb_slot_enter(0);input=(bb_input_state_t){.rt=180};bb_input_submit(&input);
    assert(sent[0].rt>100&&sent[1].buttons==(1u<<3));
    bb_input_tick();assert(sent[1].buttons==(1u<<3));
    assert(!strcmp(bb_controller_button_label(BB_CONTROLLER_STEAM,"Steam Controller",16),"Left grip"));
    assert(bb_controller_capabilities(BB_CONTROLLER_STEAM,"Steam Controller").buttons&(1u<<17));
    puts("input: compact snapshots, identity/trigger mapping, deadzones, inversion, macros, capture and two-slot isolation passed");
}
