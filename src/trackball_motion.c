#include "trackball_motion.h"
#include <string.h>

void bb_trackball_decode(bb_trackball_motion_t* m,const uint8_t r[5],uint32_t now,
    unsigned threshold,bool invert_x,bool invert_y,bb_trackball_event_t* e) {
    memset(e,0,sizeof(*e));
    bool down=(r[4]&0x80u)!=0;
    e->down=down;
    /* The switch-change counter catches taps completed between input reads.
     * Do not turn a long-hold release or stale boot counter into a new click. */
    if(m->have_sample&&!m->down&&!down&&(r[4]&0x7fu))e->clicks=1;
    m->have_sample=true;m->down=down;
    int x=(int)r[1]-r[0],y=(int)r[3]-r[2];
    if(invert_x)x=-x;
    if(invert_y)y=-y;
    if((uint32_t)(now-m->last_motion)>=200u){m->x=0;m->y=0;}
    if(down){m->x=0;m->y=0;return;}
    if(x||y){m->last_motion=now;m->x+=x;m->y+=y;}
    if(m->have_step&&(uint32_t)(now-m->last_step)<80u)return;
    int ax=m->x<0?-m->x:m->x,ay=m->y<0?-m->y:m->y;
    int* dominant=ax>=ay?&m->x:&m->y;
    int mag=ax>=ay?ax:ay;
    if(!threshold||mag<(int)threshold)return;
    int delta=*dominant/(int)threshold;
    if(delta>3)delta=3;
    if(delta< -3)delta=-3;
    if(ax>=ay)e->x=delta;else e->y=delta;
    /* Discard cross-axis jitter rather than changing two menu values at once. */
    m->x=0;m->y=0;m->last_step=now;m->have_step=true;
}
