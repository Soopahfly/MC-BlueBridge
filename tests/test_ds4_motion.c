#include "ds4_motion.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void put16(uint8_t* p,int n){p[0]=(uint8_t)n;p[1]=(uint8_t)((uint16_t)n>>8);}
static void crc(uint8_t* p,unsigned size,uint8_t seed){
    uint32_t c=0xffffffffu;
    for(int i=-1;i<(int)size-4;i++){c^=i<0?seed:p[i];for(unsigned bit=0;bit<8;bit++)c=(c>>1)^((c&1)?0xedb88320u:0);}
    c=~c;for(unsigned i=0;i<4;i++)p[size-4+i]=(uint8_t)(c>>(8*i));
}
static void calibration(uint8_t p[41]){
    memset(p,0,41);p[0]=5;
    for(unsigned i=0;i<3;i++){put16(p+1+2*i,10+10*i);put16(p+7+2*i,1010+10*i);put16(p+13+2*i,-990+10*i);put16(p+23+4*i,8292);put16(p+25+4*i,-8092);}
    put16(p+19,100);put16(p+21,100);crc(p,41,0xa3);
}
static void report(uint8_t p[78],uint16_t timestamp){
    memset(p,0,78);p[0]=0x11;put16(p+12,timestamp);
    put16(p+15,110);put16(p+17,120);put16(p+19,30);
    put16(p+21,8292);put16(p+23,100);put16(p+25,100);crc(p,78,0xa1);
}
static bb_input_state_t input;static bb_pointer_sample_t pointer;
static bool sample(bb_ds4_motion_t* s,bb_ds4_aim_t aim,uint16_t timestamp,uint32_t now,bool eligible,bool touch){
    uint8_t p[78];report(p,timestamp);pointer=(bb_pointer_sample_t){0};
    return bb_ds4_motion_decode(s,p,78,now,aim,eligible,touch,&input,&pointer);
}
int main(void){
    uint8_t c[41],p[78];bb_ds4_motion_t s={0},other={0};bb_ds4_aim_t aim=bb_ds4_aim_defaults();aim.enabled=1;
    assert(sample(&s,aim,100,1,true,false)&&s.raw_gyro[0]==110&&!pointer.x&&!s.active); /* Missing calibration fails closed. */
    calibration(c);assert(bb_ds4_motion_calibrate(&s,c,41)&&s.calibration==1);
    assert(sample(&s,aim,100,10,true,false)&&!pointer.x);
    assert(sample(&s,aim,1975,20,true,false)&&pointer.x==1&&pointer.y==-1);
    assert(s.gyro_q10[0]==10240&&s.gyro_q10[1]==10240&&s.accel_mg[0]==1000&&s.accel_mg[1]==0);
    assert(sample(&s,aim,1975,21,true,false)&&!pointer.x); /* Duplicate timestamp never replays gyro. */
    aim.sensitivity=200;aim.flags=3;
    assert(sample(&s,aim,3850,30,true,false)&&pointer.x==-2&&pointer.y==2);
    aim.sensitivity=100;aim.flags=0;input.buttons=1u<<11;
    assert(sample(&s,aim,5725,40,true,false)&&!pointer.x&&!input.buttons);
    assert(sample(&s,aim,7600,50,true,false)&&!pointer.x);
    assert(sample(&s,aim,9475,60,true,false)&&pointer.x==1);
    aim.activation=1;assert(sample(&s,aim,11350,70,true,false)&&!pointer.x);
    assert(sample(&s,aim,13225,80,true,true)&&!pointer.x);
    assert(sample(&s,aim,15100,90,true,true)&&pointer.x==1);
    aim.activation=2;assert(sample(&s,aim,16975,100,true,true)&&!pointer.x);
    input.lt=200;assert(sample(&s,aim,18850,110,true,false)&&!pointer.x);
    assert(sample(&s,aim,20725,120,true,false)&&pointer.x==1);
    aim.flags=4;input.rt=200;input.buttons=1u<<7;
    assert(sample(&s,aim,22600,130,true,false)&&pointer.buttons==1&&!input.rt&&!input.buttons);
    assert(sample(&s,aim,24475,140,true,false)&&!pointer.buttons);
    input.buttons=1u<<7;input.rt=200;
    assert(sample(&s,aim,26350,150,false,false)&&input.rt==200&&(input.buttons&(1u<<7))&&!pointer.buttons&&!pointer.x);
    input=(bb_input_state_t){0};aim.activation=0;aim.flags=0;
    assert(sample(&s,aim,28225,160,true,false)&&!pointer.x);
    assert(sample(&s,aim,30100,250,true,false)&&!pointer.x); /* Wall-clock gap rebases. */
    assert(sample(&s,aim,50000,260,true,false)&&!pointer.x); /* Sensor gap rebases. */
    assert(sample(&s,aim,49000,270,true,false)&&!pointer.x); /* Old/out-of-order timestamp. */
    s.timestamp=65000;s.at=0xfffffffau;s.have_timestamp=s.active=true;
    assert(sample(&s,aim,1339,4,true,false)&&pointer.x==1); /* Sensor and wall-clock wrap. */
    report(p,3214);p[15]^=1;assert(!bb_ds4_motion_decode(&s,p,78,14,aim,true,false,&input,&pointer)&&!s.active);
    assert(!bb_ds4_motion_decode(&s,p,77,14,aim,true,false,&input,&pointer));
    crc(p,78,0xa1);p[35]=5;crc(p,78,0xa1);assert(!bb_ds4_motion_decode(&s,p,78,14,aim,true,false,&input,&pointer));
    assert(!other.samples&&!other.calibration); /* Per-controller state isolation. */
    calibration(c);c[1]^=1;assert(!bb_ds4_motion_calibrate(&s,c,41)&&s.calibration==2);
    calibration(c);put16(c+19,0);put16(c+21,0);crc(c,41,0xa3);assert(!bb_ds4_motion_calibrate(&s,c,41));
    calibration(c);put16(c+7,10);put16(c+13,10);crc(c,41,0xa3);assert(!bb_ds4_motion_calibrate(&s,c,41));
    calibration(c);put16(c+23,100);put16(c+25,100);crc(c,41,0xa3);assert(!bb_ds4_motion_calibrate(&s,c,41));
    calibration(c);assert(!bb_ds4_motion_calibrate(&s,c,40));assert(bb_ds4_motion_calibrate(&s,c,41));
    /* Extreme signed input and low-speed fractional integration remain bounded. */
    aim.sensitivity=1;aim.pause_button=255;report(p,100);put16(p+15,-32768);put16(p+17,32767);crc(p,78,0xa1);
    assert(bb_ds4_motion_decode(&s,p,78,10,aim,true,false,&input,&pointer));
    for(unsigned i=1;i<=20;i++){put16(p+12,(int)(100+1875*i));crc(p,78,0xa1);pointer=(bb_pointer_sample_t){0};assert(bb_ds4_motion_decode(&s,p,78,10+10*i,aim,true,false,&input,&pointer));assert(pointer.x>=0&&pointer.x<=2000);}
    assert(bb_ds4_motion_calibrate(&s,c,41));input=(bb_input_state_t){0};aim=bb_ds4_aim_defaults();aim.enabled=1;aim.sensitivity=1;
    assert(sample(&s,aim,100,0,true,false));int total=0;
    for(unsigned i=1;i<=100;i++){assert(sample(&s,aim,(uint16_t)(100+1875*i),10*i,true,false));total+=pointer.x;}
    assert(total==1); /* Preserve sub-count gyro movement across timestamp wraps. */
    aim.pause_button=13;report(p,(uint16_t)(100+1875*101));p[9]=2;crc(p,78,0xa1);pointer=(bb_pointer_sample_t){0};
    assert(bb_ds4_motion_decode(&s,p,78,1010,aim,true,true,&input,&pointer)&&!s.active&&!pointer.x); /* Touchpad press can pause even after pointer consumption. */
    puts("DS4 motion: factory CRC/calibration, signed sensors/units, gyro settings, pause/activation, trigger edges, capture gate, timestamps/wrap, gaps and invalid data passed");
}
