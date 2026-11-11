#include "ds4_motion.h"
#include <string.h>
static int16_t signed16(const uint8_t* p){uint16_t v=p[0]|((uint16_t)p[1]<<8);return (int16_t)(v<=32767u?(int32_t)v:(int32_t)v-65536);}
static int32_t absolute(int32_t n){return n<0?-n:n;}
static int32_t bounded(int64_t n,int32_t limit){return n>limit?limit:n< -limit?-limit:(int32_t)n;}
static void rebase(bb_ds4_motion_t* s){s->active=false;s->remainder_x=s->remainder_y=0;}
bool bb_ds4_motion_calibrate(bb_ds4_motion_t* s,const uint8_t* p,uint16_t length){
    rebase(s);s->have_timestamp=false;s->calibration=2;
    if(!p||length!=41||p[0]!=5||!bb_hid_crc_valid(p,length,0xa3))return false;
    int32_t speed=(int32_t)signed16(p+19)+signed16(p+21);
    if(speed<=0||speed>4000)return false;
    int16_t bias[3],accel_bias[3];int32_t denominator[3],range[3];
    for(unsigned i=0;i<3;i++){
        bias[i]=signed16(p+1+2*i);
        int32_t plus=signed16(p+7+2*i),minus=signed16(p+13+2*i);
        denominator[i]=absolute(plus-bias[i])+absolute(minus-bias[i]);
        if(absolute(bias[i])>4096||plus<=bias[i]||minus>=bias[i]||denominator[i]<speed)return false;
        plus=signed16(p+23+4*i);minus=signed16(p+25+4*i);range[i]=plus-minus;
        if(range[i]<1024)return false;
        accel_bias[i]=(int16_t)(plus-range[i]/2);
    }
    memcpy(s->gyro_bias,bias,sizeof(bias));memcpy(s->gyro_denominator,denominator,sizeof(denominator));
    memcpy(s->accel_bias,accel_bias,sizeof(accel_bias));memcpy(s->accel_range,range,sizeof(range));
    s->gyro_speed=speed;s->calibration=1;return true;
}
bool bb_ds4_motion_decode(bb_ds4_motion_t* s,const uint8_t* p,uint16_t length,
    uint32_t now,bb_ds4_aim_t aim,bool eligible,bool touching,
    bb_input_state_t* input,bb_pointer_sample_t* pointer){
    if(!p||length!=78||p[0]!=0x11||p[35]>4||!bb_hid_crc_valid(p,length,0xa1)){
        ++s->invalid;rebase(s);s->have_timestamp=false;return false;
    }
    uint16_t timestamp=(uint16_t)(p[12]|((uint16_t)p[13]<<8));
    uint16_t ticks=(uint16_t)(timestamp-s->timestamp);
    bool fresh=s->have_timestamp&&(uint32_t)(now-s->at)<=50u&&ticks<=9375u;
    for(unsigned i=0;i<3;i++){
        s->raw_gyro[i]=signed16(p+15+2*i);s->raw_accel[i]=signed16(p+21+2*i);
        if(s->calibration==1){
            s->gyro_q10[i]=bounded((int64_t)(s->raw_gyro[i]-s->gyro_bias[i])*s->gyro_speed*1024/s->gyro_denominator[i],2048000);
            s->accel_mg[i]=bounded((int64_t)(s->raw_accel[i]-s->accel_bias[i])*2000/s->accel_range[i],4000);
        } else s->gyro_q10[i]=s->accel_mg[i]=0;
    }
    ++s->samples;s->timestamp=timestamp;s->at=now;s->have_timestamp=true;
    if(!eligible||!aim.enabled||!bb_ds4_aim_valid(aim)||s->calibration!=1){rebase(s);return true;}
    uint32_t buttons=input->buttons;if(p[9]&2u)buttons|=1u<<13;
    bool paused=aim.pause_button<32&&(buttons&(1u<<aim.pause_button));
    if(aim.pause_button<32)input->buttons&=~(1u<<aim.pause_button);
    bool active=!paused&&(aim.activation==0||(aim.activation==1&&touching)||
        (aim.activation==2&&((input->buttons&(1u<<6))||input->lt>=128)));
    if(aim.flags&BB_AIM_TRIGGER_FIRE){
        if((input->buttons&(1u<<7))||input->rt>=128)pointer->buttons|=1;
        input->buttons&=~(1u<<7);input->rt=0;
    }
    if(active&&s->active&&fresh&&ticks){
        int32_t yaw=s->gyro_q10[1],pitch=s->gyro_q10[0];
        if(absolute(yaw)<=512)yaw=0;
        if(absolute(pitch)<=512)pitch=0;
        if(aim.flags&BB_AIM_INVERT_X)yaw=-yaw;
        if(aim.flags&BB_AIM_INVERT_Y)pitch=-pitch;
        /* Sensor ticks = 16/3 us; q10 degrees/s, 10 counts/degree at 100%.
         * int64 retains sub-count motion and bounds extreme sensor values. */
        int64_t x=(int64_t)yaw*ticks*aim.sensitivity+s->remainder_x;
        int64_t y=-(int64_t)pitch*ticks*aim.sensitivity+s->remainder_y;
        pointer->x+=(int16_t)(x/1920000000LL);pointer->y+=(int16_t)(y/1920000000LL);
        s->remainder_x=x%1920000000LL;s->remainder_y=y%1920000000LL;
    } else if(!active||!fresh){s->remainder_x=s->remainder_y=0;}
    s->active=active;return true;
}
