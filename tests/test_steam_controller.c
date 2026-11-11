#include "steam_controller.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void put16(uint8_t* p,int v){p[0]=(uint8_t)v;p[1]=(uint8_t)((uint16_t)v>>8);}
static unsigned frame(uint8_t* p,uint16_t flags,uint32_t buttons) {
    memset(p,0,144);p[0]=(uint8_t)((flags&0xf0)|4);p[1]=(uint8_t)(flags>>8);
    unsigned at=2;
    if(flags&0x10){p[at++]=(uint8_t)buttons;p[at++]=(uint8_t)(buttons>>8);p[at++]=(uint8_t)(buttons>>16);}
    if(flags&0x20){p[at++]=12;p[at++]=200;}
    if(flags&0x40){p[at++]=0xaa;p[at++]=0xbb;p[at++]=0xcc;}
    if(flags&0x80){put16(p+at,1234);put16(p+at+2,-2345);at+=4;}
    if(flags&0x100){put16(p+at,10000);put16(p+at+2,12000);at+=4;}
    if(flags&0x200){put16(p+at,1000);put16(p+at+2,2000);at+=4;}
    if(flags&0x400){put16(p+at,1);put16(p+at+2,2);put16(p+at+4,3);at+=6;}
    if(flags&0x800){put16(p+at,16384);put16(p+at+2,0);put16(p+at+4,16384);at+=6;}
    return at;
}
static int send(bb_steam_controller_t* s,const uint8_t* p,unsigned length,uint32_t now,bool mouse,bb_input_state_t* input,bb_pointer_sample_t* pointer) {
    unsigned count=(length+17)/18;int result=0;
    for(unsigned i=0;i<count;i++) {
        uint8_t segment[20]={3,(uint8_t)(0x80|i|(i==count-1?0x40:0))};
        unsigned n=length-i*18;if(n>18)n=18;memcpy(segment+2,p+i*18,n);
        result=bb_steam_controller_decode(s,i&1?segment+1:segment,i&1?19:20,now+i,mouse,input,pointer);
        if(i<count-1)assert(result==0);
    }
    return result;
}
int main(void) {
    bb_steam_controller_t s={0},other={0};bb_input_state_t input;bb_pointer_sample_t pointer;uint8_t p[144];
    uint32_t buttons=0x80|0x8000|0x10000|0x40000|0x80000|0x100000;
    unsigned n=frame(p,0x3b0,buttons);assert(n==19);
    assert(send(&s,p,n,10,true,&input,&pointer)==1);
    assert(input.x==1234&&input.y==2345&&input.lt==12&&input.rt==200);
    assert((input.buttons&((1u<<0)|(1u<<16)|(1u<<17)))==((1u<<0)|(1u<<16)|(1u<<17)));
    assert(!(input.buttons&(1u<<11))&&input.dpad==9&&pointer.buttons==1&&!pointer.x&&!pointer.y);
    n=frame(p,0x200,0);put16(p+2,1128);put16(p+4,1872);
    assert(send(&s,p,n,20,true,&input,&pointer)==1&&pointer.x==1&&pointer.y==1);
    assert((input.buttons&1)&&input.lt==12&&input.x==1234); /* Absent chunks retain state. */
    n=frame(p,0x10,0);assert(send(&s,p,n,30,true,&input,&pointer)==1&&!pointer.buttons&&!input.buttons&&!input.dpad);
    n=frame(p,0x210,0x100000);assert(send(&s,p,n,40,true,&input,&pointer)==1&&!pointer.x); /* Lift/re-touch. */
    assert(send(&other,p,n,40,false,&input,&pointer)==1&&input.rx==1000&&input.ry==-2000); /* Per-device state. */
    n=frame(p,0x2d0,0x100000|0x40000);
    assert(send(&s,p,n,50,false,&input,&pointer)==1&&input.x==1234&&input.rx==1000&&(input.buttons&(1u<<11)));
    /* Every optional section advances the cursor, including ignored button bytes. */
    bb_steam_controls_t before=s.controls;
    n=frame(p,0x3b0,0);uint8_t bad[20]={3,0xc0};memcpy(bad+2,p,18);
    assert(bb_steam_controller_decode(&s,bad,20,60,true,&input,&pointer)==-1);
    assert(!memcmp(&s.controls,&before,sizeof(before))); /* Truncated multi-section frame is atomic. */
    bad[1]=0xc1;assert(bb_steam_controller_decode(&s,bad,20,61,true,&input,&pointer)==-1);
    bad[1]=0x80;assert(bb_steam_controller_decode(&s,bad,20,62,true,&input,&pointer)==0);
    bad[1]=0xc1;assert(bb_steam_controller_decode(&s,bad,20,200,true,&input,&pointer)==-1); /* Fragment timeout. */
    n=frame(p,0x10,1);assert(send(&s,p,n,201,false,&input,&pointer)==1); /* Resynchronizes on segment zero. */
    p[0]=5;assert(send(&s,p,n,202,false,&input,&pointer)==0); /* Non-input status. */
    assert(other.controls.stick_x==0&&other.controls.right_x==1000&&other.controls.buttons==0x100000u); /* Other controller was never overwritten. */
    /* Gyro-only updates integrate once, with bounded timing and no replay. */
    bb_steam_controller_t gyro={0};gyro.aim=bb_steam_aim_defaults();gyro.aim.enabled=1;
    n=frame(p,0xc10,0);assert(send(&gyro,p,n,100,true,&input,&pointer)==1&&!pointer.x);
    assert(send(&gyro,p,n,110,true,&input,&pointer)==1&&pointer.x==100&&pointer.y==-100);
    n=frame(p,0x10,0);assert(send(&gyro,p,n,115,true,&input,&pointer)==1&&!pointer.x&&!pointer.y);
    n=frame(p,0x800,0);assert(send(&gyro,p,n,120,true,&input,&pointer)==1&&pointer.x==100); /* No cached gyro on button-only frame. */
    assert(send(&gyro,p,n,200,true,&input,&pointer)==1&&!pointer.x); /* No catch-up after radio gap. */
    gyro.aim.sensitivity=50;gyro.aim.flags=3;
    assert(send(&gyro,p,n,210,true,&input,&pointer)==1&&pointer.x==-50&&pointer.y==50);
    gyro.aim.flags=0;gyro.aim.sensitivity=100;
    n=frame(p,0x810,0x8000);assert(send(&gyro,p,n,220,true,&input,&pointer)==1&&!pointer.x&&!(input.buttons&(1u<<16)));
    n=frame(p,0x810,0);assert(send(&gyro,p,n,230,true,&input,&pointer)==1&&!pointer.x);
    assert(send(&gyro,p,n,240,true,&input,&pointer)==1&&pointer.x==100);
    gyro.aim.activation=1;assert(send(&gyro,p,n,250,true,&input,&pointer)==1&&!pointer.x);
    n=frame(p,0x810,0x100000);assert(send(&gyro,p,n,260,true,&input,&pointer)==1&&!pointer.x);
    assert(send(&gyro,p,n,270,true,&input,&pointer)==1&&pointer.x==100);
    gyro.aim.activation=2;n=frame(p,0x810,0);assert(send(&gyro,p,n,280,true,&input,&pointer)==1&&!pointer.x);
    n=frame(p,0x810,2);assert(send(&gyro,p,n,290,true,&input,&pointer)==1&&!pointer.x);
    assert(send(&gyro,p,n,300,true,&input,&pointer)==1&&pointer.x==100);
    gyro.aim.flags=BB_AIM_TRIGGER_FIRE;n=frame(p,0x830,3);
    assert(send(&gyro,p,n,310,true,&input,&pointer)==1&&pointer.buttons==1&&!input.rt&&!(input.buttons&(1u<<7)));
    n=frame(p,0x830,2);p[6]=0;assert(send(&gyro,p,n,320,true,&input,&pointer)==1&&!pointer.buttons);
    assert(send(&gyro,p,n,330,false,&input,&pointer)==1&&!pointer.x&&!pointer.buttons); /* Capture/mode gate. */
    n=frame(p,0x830,2);assert(send(&gyro,p,n,340,true,&input,&pointer)==1&&!pointer.x);
    /* Wrap-safe timer and fractional low-speed motion. */
    gyro.gyro_at=0xfffffffau;gyro.gyro_active=true;gyro.aim.activation=0;gyro.aim.flags=0;
    n=frame(p,0x800,0);assert(send(&gyro,p,n,4,true,&input,&pointer)==1&&pointer.x==100);
    put16(p+2,100);put16(p+6,100);gyro.gyro_remainder_x=gyro.gyro_remainder_y=0;
    int total=0;for(unsigned i=1;i<=20;i++){assert(send(&gyro,p,n,4+i*10,true,&input,&pointer)==1);total+=pointer.x;}
    assert(total==12); /* Fractional movement accumulates without truncation loss. */
    puts("Steam gyro: sensitivity, inversion, activation, pause/rebase, trigger fire, stale gaps, mode gate, fractions and wrap passed");
    /* Bounded malformed-input sweep for lengths, segment numbering and flags. */
    uint32_t random=7;
    for(unsigned i=0;i<10000;i++) {
        uint8_t bytes[21];for(unsigned j=0;j<sizeof(bytes);j++){random=random*1664525u+1013904223u;bytes[j]=(uint8_t)(random>>24);}
        if(i%22==20)bytes[0]=3;
        (void)bb_steam_controller_decode(&s,bytes,i%22,i,true,&input,&pointer);
        assert(s.next_segment<=7);
    }
    puts("Steam decoder: mixed chunks, signed axes, delta retention, contact/click mapping, fragments, timeout, isolation and malformed-input sweep passed");
}
