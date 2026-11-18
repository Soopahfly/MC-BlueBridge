#include "touch_mouse.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void checksum(uint8_t p[78]) {
    uint32_t crc=0xffffffffu;
    for(int i=-1;i<74;i++) {
        crc^=i<0?0xa1:p[i];
        for(unsigned bit=0;bit<8;bit++)crc=crc&1?(crc>>1)^0xedb88320u:crc>>1;
    }
    crc=~crc;for(unsigned i=0;i<4;i++)p[74+i]=(uint8_t)(crc>>(8*i));
}
static void point(uint8_t p[78],uint8_t timestamp,uint8_t contact,unsigned x,unsigned y) {
    p[35]=1;p[36]=timestamp;p[37]=contact;p[38]=(uint8_t)x;
    p[39]=(uint8_t)((x>>8)|((y&15)<<4));p[40]=(uint8_t)(y>>4);p[41]=0x80;checksum(p);
}
int main(void) {
    uint8_t p[78]={0x11};bb_ds4_pointer_t state={0};bb_pointer_sample_t sample;
    point(p,1,7,500,300);assert(bb_ds4_pointer_decode(&state,p,78,1,true,&sample));assert(!sample.x&&!sample.y);
    point(p,2,7,540,320);assert(bb_ds4_pointer_decode(&state,p,78,2,true,&sample));assert(sample.x==10&&sample.y==5);
    assert(bb_ds4_pointer_decode(&state,p,78,3,true,&sample)&&!sample.x); /* Duplicate history does not replay. */
    point(p,3,0x87,0,0);assert(bb_ds4_pointer_decode(&state,p,78,4,true,&sample));
    point(p,4,8,1500,600);assert(bb_ds4_pointer_decode(&state,p,78,5,true,&sample)&&!sample.x); /* Lift/re-touch. */
    p[9]=2;checksum(p);assert(bb_ds4_pointer_decode(&state,p,78,6,true,&sample)&&sample.buttons==2);
    p[9]=0;checksum(p);assert(bb_ds4_pointer_decode(&state,p,78,7,true,&sample)&&!sample.buttons);
    p[38]^=1;assert(!bb_ds4_pointer_decode(&state,p,78,8,true,&sample)); /* CRC. */
    assert(!bb_ds4_pointer_decode(&state,p,12,8,true,&sample));
    p[35]=5;checksum(p);assert(!bb_ds4_pointer_decode(&state,p,78,8,true,&sample));
    memset(&state,0,sizeof(state));point(p,255,3,500,300);bb_ds4_pointer_decode(&state,p,78,9,true,&sample);
    point(p,0,3,504,304);assert(bb_ds4_pointer_decode(&state,p,78,10,true,&sample)&&sample.x==1&&sample.y==1);
    point(p,1,3,1500,500);assert(bb_ds4_pointer_decode(&state,p,78,11,true,&sample)&&!sample.x); /* Jump rejected. */
    point(p,2,3,1516,516);assert(bb_ds4_pointer_decode(&state,p,78,1000,true,&sample)&&!sample.x); /* Stale baseline. */
    /* Right-half click stays right through movement and contact lift. */
    memset(&state,0,sizeof(state));p[9]=2;point(p,1,1,1500,300);
    assert(bb_ds4_pointer_decode(&state,p,78,1001,true,&sample)&&sample.buttons==2);
    point(p,2,1,900,300);assert(bb_ds4_pointer_decode(&state,p,78,1002,true,&sample)&&sample.buttons==2);
    point(p,3,0x81,0,0);assert(bb_ds4_pointer_decode(&state,p,78,1003,true,&sample)&&sample.buttons==2);
    p[9]=0;checksum(p);assert(bb_ds4_pointer_decode(&state,p,78,1004,true,&sample)&&!sample.buttons);
    p[9]=2;checksum(p);assert(bb_ds4_pointer_decode(&state,p,78,1005,true,&sample)&&sample.buttons==1); /* No finger. */
    p[9]=0;point(p,4,2,959,300);bb_ds4_pointer_decode(&state,p,78,1006,true,&sample);
    p[9]=2;checksum(p);assert(bb_ds4_pointer_decode(&state,p,78,1007,true,&sample)&&sample.buttons==1);
    point(p,5,2,1000,300);assert(bb_ds4_pointer_decode(&state,p,78,1008,true,&sample)&&sample.buttons==1);
    p[9]=0;point(p,6,2,960,300);bb_ds4_pointer_decode(&state,p,78,1009,true,&sample);
    p[9]=2;checksum(p);assert(bb_ds4_pointer_decode(&state,p,78,1010,true,&sample)&&sample.buttons==2);
    bb_ds4_pointer_t unchanged=state;p[74]^=1;
    assert(!bb_ds4_pointer_decode(&state,p,78,1011,true,&sample)&&!memcmp(&state,&unchanged,sizeof(state)));
    /* Independent controller states must not inherit each other's held click. */
    bb_ds4_pointer_t other={0};point(p,1,1,200,200);
    assert(bb_ds4_pointer_decode(&other,p,78,1012,true,&sample)&&sample.buttons==1&&state.click_button==2);
    /* Disabled is left-click everywhere; changing the option cannot switch a held button. */
    bb_ds4_pointer_t option={0};p[9]=2;point(p,1,1,1500,200);
    assert(bb_ds4_pointer_decode(&option,p,78,1013,false,&sample)&&sample.buttons==1);
    assert(bb_ds4_pointer_decode(&option,p,78,1014,true,&sample)&&sample.buttons==1);
    p[9]=0;checksum(p);bb_ds4_pointer_decode(&option,p,78,1015,true,&sample);
    p[9]=2;checksum(p);assert(bb_ds4_pointer_decode(&option,p,78,1016,true,&sample)&&sample.buttons==2);
    assert(bb_ds4_pointer_decode(&option,p,78,1017,false,&sample)&&sample.buttons==2);
    p[9]=0;checksum(p);bb_ds4_pointer_decode(&option,p,78,1018,false,&sample);
    p[9]=2;checksum(p);assert(bb_ds4_pointer_decode(&option,p,78,1019,false,&sample)&&sample.buttons==1);
    bb_mouse_queue_t queue={0};uint32_t token;
    bb_mouse_queue_push(&queue,(bb_pointer_sample_t){.x=300,.buttons=1});
    assert(bb_mouse_queue_peek(&queue,&sample,&token)&&sample.x==127&&sample.buttons==1);
    /* Failed endpoint attempt: do not ack; concurrent motion must remain queued. */
    bb_mouse_queue_push(&queue,(bb_pointer_sample_t){.x=40,.buttons=1});
    bb_mouse_queue_ack(&queue,token,sample);int total=127;
    while(bb_mouse_queue_peek(&queue,&sample,&token)){total+=sample.x;bb_mouse_queue_ack(&queue,token,sample);}
    assert(total==340&&queue.last_buttons==1);
    bb_mouse_queue_push(&queue,(bb_pointer_sample_t){0});bb_mouse_queue_push(&queue,(bb_pointer_sample_t){.buttons=1});
    assert(bb_mouse_queue_peek(&queue,&sample,&token)&&!sample.buttons);bb_mouse_queue_ack(&queue,token,sample);
    assert(bb_mouse_queue_peek(&queue,&sample,&token)&&sample.buttons==1);bb_mouse_queue_ack(&queue,token,sample);
    bb_mouse_queue_release(&queue);assert(bb_mouse_queue_peek(&queue,&sample,&token)&&!sample.buttons);
    bb_mouse_queue_ack(&queue,token,sample);assert(!queue.count&&!queue.last_buttons);
    bb_mouse_queue_push(&queue,(bb_pointer_sample_t){.x=100});bb_mouse_queue_peek(&queue,&sample,&token);
    bb_mouse_queue_expire_motion(&queue);bb_mouse_queue_ack(&queue,token,sample);
    assert(bb_mouse_queue_peek(&queue,&sample,&token)&&!sample.x); /* Expiry invalidates an in-flight token. */
    bb_mouse_queue_ack(&queue,token,sample);
    for(unsigned i=0;i<20;i++)bb_mouse_queue_push(&queue,(bb_pointer_sample_t){.buttons=i&1});
    assert(queue.count==8&&queue.overflows);
    bb_mouse_queue_t right={0};
    bb_mouse_queue_push(&right,(bb_pointer_sample_t){.buttons=2});
    assert(bb_mouse_queue_peek(&right,&sample,&token)&&sample.buttons==2);
    bb_mouse_queue_ack(&right,token,sample);bb_mouse_queue_release(&right);
    assert(bb_mouse_queue_peek(&right,&sample,&token)&&!sample.buttons);
    puts("DS4 pointer: CRC/length/history checks, motion, lift/contact changes, click, wrap, failed sends and queue edges passed");
}
