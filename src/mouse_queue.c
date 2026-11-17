#include "touch_mouse.h"
static int32_t ds4_mouse_bounded(int32_t x){return x>32767?32767:x< -32767?-32767:x;}
void bb_mouse_queue_push(bb_mouse_queue_t* q,bb_pointer_sample_t s) {
    if(q->count) {
        bb_mouse_event_t* e=&q->events[(q->head+q->count-1u)%8];
        if(e->buttons==s.buttons){e->x=ds4_mouse_bounded(e->x+s.x);e->y=ds4_mouse_bounded(e->y+s.y);e->wheel=ds4_mouse_bounded(e->wheel+s.wheel);return;}
    } else if(!s.x&&!s.y&&!s.wheel&&s.buttons==q->last_buttons)return;
    if(q->count==8){q->head=(q->head+1u)%8;--q->count;++q->overflows;}
    unsigned tail=(q->head+q->count)%8;
    q->events[tail]=(bb_mouse_event_t){.x=s.x,.y=s.y,.wheel=s.wheel,.buttons=s.buttons,.sequence=++q->sequence};
    ++q->count;
}
bool bb_mouse_queue_peek(const bb_mouse_queue_t* q,bb_pointer_sample_t* out,uint32_t* token) {
    if(!q->count)return false;
    const bb_mouse_event_t* e=&q->events[q->head];
    *out=(bb_pointer_sample_t){.x=(int16_t)(e->x>127?127:e->x< -127?-127:e->x),
        .y=(int16_t)(e->y>127?127:e->y< -127?-127:e->y),.wheel=(int16_t)(e->wheel>127?127:e->wheel< -127?-127:e->wheel),.buttons=e->buttons};
    *token=e->sequence;return true;
}
void bb_mouse_queue_ack(bb_mouse_queue_t* q,uint32_t token,bb_pointer_sample_t sent) {
    q->last_buttons=sent.buttons;
    if(!q->count||q->events[q->head].sequence!=token)return;
    bb_mouse_event_t* e=&q->events[q->head];e->x-=sent.x;e->y-=sent.y;e->wheel-=sent.wheel;
    if(!e->x&&!e->y&&!e->wheel){q->head=(q->head+1u)%8;--q->count;}
}
void bb_mouse_queue_release(bb_mouse_queue_t* q) {
    if(!q->count&&!q->last_buttons)return;
    if(q->count==1){const bb_mouse_event_t* e=&q->events[q->head];if(!e->buttons&&!e->x&&!e->y&&!e->wheel)return;}
    /* A press may be in flight before its ACK updates last_buttons. Preserve
     * an explicit neutral after that send, even if last_buttons is still 0. */
    q->head=0;q->count=1;q->events[0]=(bb_mouse_event_t){.sequence=++q->sequence};
}

void bb_mouse_queue_expire_motion(bb_mouse_queue_t* q) {
    for(unsigned i=0;i<q->count;i++) {
        bb_mouse_event_t* e=&q->events[(q->head+i)%8];e->x=e->y=e->wheel=0;e->sequence=++q->sequence;
    }
}

