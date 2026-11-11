#include "dualsense_device.h"
#include <assert.h>
#include <stdio.h>
uint32_t test_time;
static unsigned calls;static uint8_t left,right;
void bb_bt_host_set_output_rumble(uint8_t l,uint8_t r){++calls;left=l;right=r;}
int main(void){
 uint8_t out[BB_DS5_INPUT_SIZE];bb_gamepad_report_t r={0};bb_ds5_reset();
 bb_ds5_build_input(&r,out);assert(out[0]==128&&out[1]==128&&out[7]==8&&out[32]==128&&out[36]==128);
 r.buttons=1|(1u<<6)|(1u<<13);r.hat=3;bb_ds5_build_input(&r,out);
 assert((out[7]&0x2f)==0x22&&out[4]==255&&(out[9]&2));
 uint8_t rumble[]={2,0,0,30,80};bb_ds5_handle_output(0,rumble,sizeof(rumble));assert(left==80&&right==30&&calls==1);
 bb_ds5_handle_output(0,rumble,sizeof(rumble));assert(calls==1);
 test_time=1000;bb_ds5_task();assert(calls==2);bb_ds5_reset();assert(!left&&!right);
 assert(!bb_ds5_get_feature(0,out,sizeof(out)));assert(bb_ds5_get_feature(0x20,out,2)==2);
 puts("DualSense: input encoding, touch inactivity, bounded feature reads and rumble passed");
}
