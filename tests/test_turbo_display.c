#include "bluebridge_state.h"
#include "controller_input.h"
#include "hardware/flash.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int test_flash_error;
unsigned test_flash_erases;
unsigned test_core;
uint32_t test_time;
uint8_t test_flash[PICO_FLASH_SIZE_BYTES];
static unsigned submitted;
static bb_gamepad_report_t sent;
void bb_usb_submit(const bb_gamepad_report_t* report){++submitted;sent=*report;}
int main(void){
    memset(test_flash,255,sizeof(test_flash));bb_state_init();
    uint8_t a[6]={1,2,3,4,5,6},ci;
    assert(bb_controller_activate_or_create(a,0x054c,0x0ce6,"DualSense",&ci));
    bb_state_set_controller(true,ci,"DualSense");
    assert(bb_profile_set_turbo_control(0,true,0,13));
    bb_input_state_t input={0};bb_runtime_state_t state;
    input.buttons=(1u<<8)|1u;bb_input_submit(&input);bb_state_get(&state);
    assert(state.turbo_assignments==1); /* Modifier + South assigns turbo. */
    bb_profile_t p;bb_active_profile_get(&p);assert(state.turbo_profile_id==p.id);
    input.buttons=1;bb_input_submit(&input);bb_state_get(&state);assert(state.turbo_assignments==1);
    input.buttons=0;bb_input_submit(&input);
    input.buttons=(1u<<8)|1u;bb_input_submit(&input);bb_state_get(&state);assert(state.turbo_assignments==0);
    assert(bb_profile_set_tuning(0,0,0,0,0,8,8,4,12,1u<<2));
    assert(bb_profile_set_turbo_control(0,true,1,13));
    input.buttons=0;bb_input_submit(&input);bb_state_get(&state);assert(state.turbo_assignments==(1u<<2));
    assert(bb_profile_set_turbo_control(0,false,1,13));
    bb_input_submit(&input);bb_state_get(&state);assert(state.turbo_assignments==0);
    /* Timed output continues with no further Bluetooth input reports. */
    assert(bb_profile_set_turbo_control(0,true,2,13));
    assert(bb_profile_set_tuning(0,0,0,0,0,8,8,4,10,1));
    test_time=100;input.buttons=(1u<<13)|1;bb_input_submit(&input);
    assert(sent.buttons&1);bb_state_get(&state);uint32_t bt_count=state.bt_reports;
    unsigned before=submitted;test_time=120;bb_input_tick();assert(submitted==before);
    test_time=150;bb_input_tick();assert(!(sent.buttons&1));
    test_time=200;bb_input_tick();assert(sent.buttons&1);
    bb_state_get(&state);assert(state.bt_reports==bt_count); /* No synthetic BT reports. */
    input.buttons=0;test_time=201;bb_input_submit(&input);assert(!sent.buttons);
    before=submitted;test_time=250;bb_input_tick();assert(submitted==before); /* Immediate release cancels timer. */
    /* A mapping edit during a held turbo input is picked up on the timer. */
    input.buttons=(1u<<13)|1;test_time=300;bb_input_submit(&input);
    assert(bb_profile_set_mapping(0,0,2));test_time=301;bb_input_tick();assert(sent.buttons&(1u<<2));
    assert(bb_profile_set_mapping(0,0,0));bb_input_submit(&input);
    assert(bb_capture_start(BB_CAPTURE_ONCE));test_time=302;bb_input_tick();assert(!sent.buttons);
    bb_capture_stop();
    input.buttons=0;bb_input_submit(&input);input.buttons=(1u<<13)|1;bb_input_submit(&input);
    bb_state_set_controller(false,-1,"");test_time=350;bb_input_tick();assert(!sent.buttons);
    bb_state_set_controller(true,ci,"DualSense");
    /* Modifier assignments survive timer ticks without generating press edges. */
    assert(bb_profile_set_turbo_control(0,true,0,13));
    test_time=400;input.buttons=(1u<<8)|1;bb_input_submit(&input);
    input.buttons=1;bb_input_submit(&input);assert(sent.buttons&1);
    test_time=450;bb_input_tick();assert(!(sent.buttons&1));
    bb_state_get(&state);assert(state.turbo_assignments==1);
    /* Dedicated toggle remains active after releasing its control button. */
    assert(bb_profile_set_turbo_control(0,true,1,13));
    test_time=500;input.buttons=(1u<<13)|1;bb_input_submit(&input);
    input.buttons=1;bb_input_submit(&input);assert(sent.buttons&1);
    test_time=550;bb_input_tick();assert(!(sent.buttons&1));
    test_time=600;bb_input_tick();assert(sent.buttons&1);
    /* Analog trigger turbo and immediate analog release. */
    assert(bb_profile_set_turbo_control(0,true,2,13));
    assert(bb_profile_set_tuning(0,0,0,0,0,8,8,4,10,1u<<6));
    test_time=700;input.buttons=1u<<13;input.lt=200;bb_input_submit(&input);assert(sent.lt);
    test_time=750;bb_input_tick();assert(!sent.lt);
    test_time=800;bb_input_tick();assert(sent.lt);
    input.lt=0;bb_input_submit(&input);assert(!sent.lt);
    /* Deadline comparisons remain valid when the millisecond clock wraps. */
    test_time=0xfffffff0u;input.lt=200;bb_input_submit(&input);
    test_time=20;bb_input_tick();assert(sent.lt);
    puts("turbo display: real input pipeline publishes modifier assignments, profile identity and disabled state");
}

