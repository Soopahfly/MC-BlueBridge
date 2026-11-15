#include "hid_feedback.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint8_t sent_slot, sent_strength, sent_big, sent_small;
static unsigned sends;
void bb_bt_host_set_slot_rumble(uint8_t slot,uint8_t big,uint8_t small) {
    sent_slot=slot;sent_strength=big>small?big:small;sent_big=big;sent_small=small;++sends;
}
void bb_bt_host_set_slot_rumble_motors(uint8_t slot,uint8_t strength,uint8_t big,uint8_t small) {
    sent_slot=slot;sent_strength=strength;sent_big=big;sent_small=small;++sends;
}
static void effect(uint16_t magnitude,uint16_t period) {
    const uint8_t allocate[]={1},start[]={1,1,1};
    const uint8_t setup[]={1,1,255,255,255,0,0,0,0,0,0,0,0,0};
    uint8_t periodic[]={1,(uint8_t)magnitude,(uint8_t)(magnitude>>8),0,0,0,0,(uint8_t)period,(uint8_t)(period>>8)};
    bb_feedback_set(9,true,allocate,sizeof(allocate));
    bb_feedback_set(2,false,setup,sizeof(setup));
    bb_feedback_set(3,false,periodic,sizeof(periodic));
    bb_feedback_set(5,false,start,sizeof(start));
}
int main(void) {
    /* Exercise the real USB PID report path, not just the arithmetic helper.
     * Linux FF_RUMBLE encodes strong/3 + weak/6 into its periodic magnitude. */
    for(unsigned small=0;small<=255;small+=255)for(unsigned big=0;big<=255;big++) {
        bb_feedback_select(0);bb_feedback_reset();
        sends=0;sent_slot=sent_strength=sent_big=sent_small=0;
        uint16_t magnitude=(uint16_t)((big*257u)/3u+(small*257u)/6u);
        effect(magnitude,50);bb_feedback_poll(1);
        assert(sent_slot==0&&sent_big==big&&sent_small==small);
        assert(sent_strength==(uint8_t)((uint32_t)magnitude*255u/32767u));
        if(big||small)assert(sends==1);
    }
    /* Non-rumble periodic effects keep merged strength, with unknown motors. */
    bb_feedback_reset();effect(18000,60);bb_feedback_poll(10);
    assert(sent_strength>0&&sent_big==0&&sent_small==0);
    bb_feedback_stop();assert(sent_strength==0&&sent_big==0&&sent_small==0);
    /* Motor-only changes must be sent even if quantized strength is unchanged. */
    bb_feedback_reset();effect(85,50);bb_feedback_poll(20);
    assert(sent_big==1&&sent_small==0&&sent_strength==0);
    unsigned before=sends;
    const uint8_t update[]={1,0,0,0,0,0,0,50,0};
    bb_feedback_set(3,false,update,sizeof(update));bb_feedback_poll(21);
    assert(sends==before+1&&sent_big==0&&sent_strength==0);
    /* Slot 1 reset/stop must leave slot 0's effect running. */
    bb_feedback_select(0);bb_feedback_reset();effect(21845,50);bb_feedback_poll(100);
    bb_feedback_select(1);bb_feedback_reset();effect(10922,50);bb_feedback_poll(100);
    assert(sent_slot==1&&sent_big==0&&sent_small==255);
    bb_feedback_stop();assert(sent_slot==1&&sent_strength==0);
    bb_feedback_select(0);bb_feedback_poll(400);
    assert(sent_slot==0&&sent_big==255&&sent_small==0);
    puts("feedback: all 512 big/small combinations, merged fallback, stop, motor-only changes and slot isolation passed");
}
