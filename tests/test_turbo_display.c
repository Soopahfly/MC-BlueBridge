#include "bluebridge_state.h"
#include "controller_input.h"
#include "hardware/flash.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
unsigned test_core;
uint32_t test_time;
uint8_t test_flash[PICO_FLASH_SIZE_BYTES];
void bb_usb_submit(const bb_gamepad_report_t* report){(void)report;}
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
    puts("turbo display: real input pipeline publishes modifier assignments, profile identity and disabled state");
}
