#include "touch_mouse.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void checksum(uint8_t* p){uint32_t c=0xffffffffu;for(int i=-1;i<74;i++){c^=i<0?0xa1:p[i];for(unsigned j=0;j<8;j++)c=(c>>1)^((c&1)?0xedb88320u:0);}c=~c;for(unsigned j=0;j<4;j++)p[74+j]=(uint8_t)(c>>(8*j));}
static void pt(uint8_t* p,unsigned id,int x,int y){p[0]=(uint8_t)id;p[1]=(uint8_t)x;p[2]=(uint8_t)((x>>8)|((y&15)<<4));p[3]=(uint8_t)(y>>4);}
static void frame(uint8_t* p,uint8_t timestamp,unsigned count,int y,bool press){memset(p,0,78);p[0]=0x11;p[9]=press?2:0;p[35]=1;p[36]=timestamp;pt(p+37,count?1:0x81,500,y);pt(p+41,count==2?2:0x82,1200,y);checksum(p);}
int main(void){
    bb_ds4_mouse_t c=bb_ds4_mouse_defaults();c.right_click=1;c.scroll=1;
    bb_ds4_pointer_t s={0};bb_pointer_sample_t out;uint8_t p[78];
    frame(p,1,2,500,false);assert(bb_ds4_pointer_decode_config(&s,p,78,1,c,&out)&&!out.wheel&&!out.x&&s.scrolling);
    frame(p,2,2,436,false);assert(bb_ds4_pointer_decode_config(&s,p,78,2,c,&out)&&out.wheel==1&&!out.x&&!out.y);
    assert(bb_ds4_pointer_decode_config(&s,p,78,3,c,&out)&&!out.wheel); /* Duplicate history. */
    frame(p,3,2,372,true);assert(bb_ds4_pointer_decode_config(&s,p,78,4,c,&out)&&out.buttons==2&&out.wheel==1);
    frame(p,4,1,372,true);assert(bb_ds4_pointer_decode_config(&s,p,78,5,c,&out)&&out.buttons==2&&!out.x&&!out.y); /* Latched through lift. */
    frame(p,5,1,400,true);assert(bb_ds4_pointer_decode_config(&s,p,78,6,c,&out)&&out.buttons==2&&out.y==7);
    frame(p,6,1,400,false);assert(bb_ds4_pointer_decode_config(&s,p,78,7,c,&out)&&!out.buttons);
    frame(p,7,1,400,true);assert(bb_ds4_pointer_decode_config(&s,p,78,8,c,&out)&&out.buttons==1);
    frame(p,8,2,400,true);assert(bb_ds4_pointer_decode_config(&s,p,78,9,c,&out)&&out.buttons==1&&!out.wheel); /* Adding finger does not switch held click. */
    frame(p,9,2,368,true);assert(bb_ds4_pointer_decode_config(&s,p,78,10,c,&out)&&!out.wheel);
    frame(p,10,2,336,true);assert(bb_ds4_pointer_decode_config(&s,p,78,11,c,&out)&&out.wheel==1); /* Fractional scroll. */
    frame(p,11,2,272,true);pt(p+37,2,1200,272);pt(p+41,1,500,272);checksum(p);
    assert(bb_ds4_pointer_decode_config(&s,p,78,12,c,&out)&&out.wheel==1); /* Contact array order can swap. */
    frame(p,12,2,208,false);pt(p+41,3,1200,208);checksum(p);assert(bb_ds4_pointer_decode_config(&s,p,78,13,c,&out)&&!out.wheel); /* New contact rebases. */
    frame(p,13,2,144,false);assert(bb_ds4_pointer_decode_config(&s,p,78,14,c,&out)&&!out.wheel);
    c.invert_scroll=1;frame(p,14,2,80,false);assert(bb_ds4_pointer_decode_config(&s,p,78,15,c,&out)&&out.wheel==-1);
    frame(p,15,2,16,false);assert(bb_ds4_pointer_decode_config(&s,p,78,700,c,&out)&&!out.wheel); /* Stale rebase. */
    bb_ds4_pointer_t before=s;p[74]^=1;assert(!bb_ds4_pointer_decode_config(&s,p,78,701,c,&out)&&!memcmp(&s,&before,sizeof(s)));
    checksum(p);p[35]=5;checksum(p);assert(!bb_ds4_pointer_decode_config(&s,p,78,702,c,&out));
    c=bb_ds4_mouse_defaults();c.right_click=2;s=(bb_ds4_pointer_t){0};frame(p,1,1,300,true);pt(p+37,1,1500,300);checksum(p);
    assert(bb_ds4_pointer_decode_config(&s,p,78,1,c,&out)&&out.buttons==2);
    frame(p,2,1,300,false);assert(bb_ds4_pointer_decode_config(&s,p,78,2,c,&out)&&!out.buttons);
    frame(p,3,1,300,true);assert(bb_ds4_pointer_decode_config(&s,p,78,3,c,&out)&&out.buttons==1);
    c=bb_ds4_mouse_defaults();s=(bb_ds4_pointer_t){0};frame(p,1,2,300,true);
    assert(bb_ds4_pointer_decode_config(&s,p,78,1,c,&out)&&out.buttons==1&&!s.scrolling); /* Legacy default. */
    c.right_click=3;c.right_button=5;bb_input_state_t input={.buttons=(1u<<5)|1};out=(bb_pointer_sample_t){.buttons=1};
    bb_ds4_mouse_right_button(c,input.buttons,&input,&out);assert(out.buttons==3&&input.buttons==1);
    out=(bb_pointer_sample_t){0};bb_ds4_mouse_right_button(c,0,&input,&out);assert(!out.buttons);
    c.right_button=7;input=(bb_input_state_t){.buttons=1u<<7,.rt=200};bb_ds4_mouse_right_button(c,input.buttons,&input,&out);assert(out.buttons==2&&!input.buttons&&!input.rt);
    c.right_button=13;s=(bb_ds4_pointer_t){0};frame(p,1,1,300,true);assert(bb_ds4_pointer_decode_config(&s,p,78,1,c,&out)&&out.buttons==2);
    c.scroll_step=0;assert(!bb_ds4_mouse_valid(c));c=bb_ds4_mouse_defaults();c.right_button=14;assert(!bb_ds4_mouse_valid(c));
    /* Wheel-only reports survive backpressure and consume only on successful ACK. */
    bb_mouse_queue_t q={0};uint32_t token;bb_mouse_queue_push(&q,(bb_pointer_sample_t){.wheel=300,.buttons=2});
    assert(bb_mouse_queue_peek(&q,&out,&token)&&out.wheel==127&&out.buttons==2);
    bb_mouse_queue_push(&q,(bb_pointer_sample_t){.wheel=20,.buttons=2});bb_mouse_queue_ack(&q,token,out);int total=127;
    while(bb_mouse_queue_peek(&q,&out,&token)){total+=out.wheel;bb_mouse_queue_ack(&q,token,out);}assert(total==320);
    bb_mouse_queue_push(&q,(bb_pointer_sample_t){.wheel=-200,.buttons=2});assert(bb_mouse_queue_peek(&q,&out,&token));
    bb_mouse_queue_expire_motion(&q);bb_mouse_queue_ack(&q,token,out);assert(bb_mouse_queue_peek(&q,&out,&token)&&!out.wheel);bb_mouse_queue_ack(&q,token,out);
    bb_mouse_queue_release(&q);assert(bb_mouse_queue_peek(&q,&out,&token)&&!out.buttons&&!out.wheel);bb_mouse_queue_ack(&q,token,out);assert(!q.count);
    bb_mouse_queue_t race={0};bb_mouse_queue_push(&race,(bb_pointer_sample_t){.buttons=2});assert(bb_mouse_queue_peek(&race,&out,&token));
    bb_mouse_queue_release(&race);bb_mouse_queue_ack(&race,token,out);assert(bb_mouse_queue_peek(&race,&out,&token)&&!out.buttons);
    bb_mouse_queue_ack(&race,token,out);assert(!race.count&&!race.last_buttons);bb_mouse_queue_release(&race);assert(!race.count);
    puts("DS4 gestures: click modes/latching, scroll transitions/contacts/history/stale guards, button reservation and wheel backpressure/expiry passed");
}
