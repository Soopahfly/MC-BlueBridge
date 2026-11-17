#include "touch_mouse.h"
#include <string.h>
static uint32_t crc_byte(uint32_t crc,uint8_t byte){crc^=byte;for(unsigned i=0;i<8;i++)crc=(crc>>1)^((crc&1u)?0xedb88320u:0);return crc;}
typedef struct {int16_t x,y;uint8_t id;} point_t;
static bool point(const uint8_t* p,point_t* out){
    int x=p[1]|((p[2]&15u)<<8),y=(p[2]>>4)|(p[3]<<4);
    if((p[0]&0x80u)||x>=1920||y>=942)return false;
    *out=(point_t){(int16_t)x,(int16_t)y,(uint8_t)(p[0]&127u)};return true;
}
bool bb_ds4_pointer_decode(bb_ds4_pointer_t* s,const uint8_t* p,uint16_t len,uint32_t now,bb_pointer_sample_t* out){
    return bb_ds4_pointer_decode_config(s,p,len,now,bb_ds4_mouse_defaults(),out);
}
bool bb_ds4_pointer_decode_config(bb_ds4_pointer_t* s,const uint8_t* p,uint16_t len,
    uint32_t now,bb_ds4_mouse_t config,bb_pointer_sample_t* out){
    if(!p||len!=78||p[0]!=0x11||p[35]>4||!bb_ds4_mouse_valid(config))return false;
    uint32_t crc=crc_byte(0xffffffffu,0xa1);for(unsigned i=0;i<74;i++)crc=crc_byte(crc,p[i]);
    uint32_t received=(uint32_t)p[74]|((uint32_t)p[75]<<8)|((uint32_t)p[76]<<16)|((uint32_t)p[77]<<24);
    if((~crc)!=received)return false;
    *out=(bb_pointer_sample_t){0};
    /* Stale contact geometry must rebase, but never switch a held click's type. */
    if((uint32_t)(now-s->at)>500u){s->touching=s->scrolling=s->have_timestamp=false;s->points=0;s->remainder_x=s->remainder_y=s->scroll_remainder=0;}
    s->at=now;
    for(unsigned i=0;i<p[35];i++){
        const uint8_t* record=p+36+9*i;uint8_t timestamp=record[0];
        if(s->have_timestamp&&(int8_t)(timestamp-s->timestamp)<=0)continue;
        s->have_timestamp=true;s->timestamp=timestamp;
        point_t points[2];unsigned count=0;
        for(unsigned j=0;j<2;j++){point_t v;if(point(record+1+4*j,&v)&&(count==0||points[0].id!=v.id))points[count++]=v;}
        if(config.scroll&&count==2&&points[0].id>points[1].id){point_t swap=points[0];points[0]=points[1];points[1]=swap;}
        s->points=(uint8_t)count;
        if(!count){s->touching=s->scrolling=false;s->remainder_x=s->remainder_y=s->scroll_remainder=0;continue;}
        bool was_scrolling=s->scrolling;
        if(config.scroll&&count==2){
            int16_t y=(int16_t)(((int)points[0].y+points[1].y)/2);
            int dy=(int)s->scroll_y-y;
            if(was_scrolling&&s->contact==points[0].id&&s->contact2==points[1].id&&dy>=-512&&dy<=512){
                if(config.invert_scroll)dy=-dy;
                int total=dy+s->scroll_remainder;out->wheel+=(int16_t)(total/config.scroll_step);s->scroll_remainder=(int16_t)(total%config.scroll_step);
            } else s->scroll_remainder=0;
            s->scrolling=s->touching=true;s->scroll_y=y;s->contact=points[0].id;s->contact2=points[1].id;
            s->x=points[0].x;s->y=points[0].y;s->remainder_x=s->remainder_y=0;
        } else {
            s->scrolling=false;s->scroll_remainder=0;
            int dx=(int)points[0].x-s->x,dy=(int)points[0].y-s->y;
            if(!was_scrolling&&s->touching&&s->contact==points[0].id&&dx>=-512&&dx<=512&&dy>=-512&&dy<=512){
                int x=dx+s->remainder_x,y=dy+s->remainder_y;out->x+=(int16_t)(x/4);out->y+=(int16_t)(y/4);s->remainder_x=(int16_t)(x%4);s->remainder_y=(int16_t)(y%4);
            } else s->remainder_x=s->remainder_y=0;
            s->touching=true;s->contact=points[0].id;s->x=points[0].x;s->y=points[0].y;
        }
    }
    bool press=(p[9]&2u)!=0;
    if(!press){s->click_latched=false;s->click_button=0;}
    else if(!s->click_latched){
        s->click_latched=true;s->click_button=(config.right_click==3&&config.right_button==13)?2:1;
        if((config.right_click==1&&s->points==2)||(config.right_click==2&&s->touching&&s->x>=960))s->click_button=2;
    }
    if(s->scrolling)out->x=out->y=0;
    out->buttons=s->click_button;return true;
}
void bb_ds4_mouse_right_button(bb_ds4_mouse_t config,uint32_t raw_buttons,bb_input_state_t* input,bb_pointer_sample_t* pointer){
    if(config.right_click!=3||!bb_ds4_mouse_valid(config))return;
    uint32_t mask=1u<<config.right_button;
    if(raw_buttons&mask)pointer->buttons|=2;
    input->buttons&=~mask;
    if(config.right_button==6)input->lt=0;
    if(config.right_button==7)input->rt=0;
}
