#include "trackball_motion.h"
#include <assert.h>
#include <stdio.h>
int main(void){
    bb_trackball_motion_t m={0};bb_trackball_event_t e;
    uint8_t r[5]={0};
    r[4]=2;bb_trackball_decode(&m,r,0,3,false,false,&e);assert(!e.clicks); /* Ignore boot counter. */
    r[4]=0;r[1]=2;bb_trackball_decode(&m,r,10,3,false,false,&e);assert(!e.x);
    r[1]=1;bb_trackball_decode(&m,r,20,3,false,false,&e);assert(e.x==1&&!e.y);
    r[1]=3;bb_trackball_decode(&m,r,30,3,false,false,&e);assert(!e.x); /* Rate limited. */
    r[1]=0;bb_trackball_decode(&m,r,101,3,false,false,&e);assert(e.x==1);
    r[0]=8;r[3]=2;bb_trackball_decode(&m,r,201,3,false,false,&e);assert(e.x==-2&&!e.y);
    r[0]=0;r[3]=6;bb_trackball_decode(&m,r,301,3,false,true,&e);assert(e.y==-2);
    r[3]=0;r[1]=255;bb_trackball_decode(&m,r,401,3,false,false,&e);assert(e.x==3); /* Burst cap. */
    r[1]=0;r[4]=0x80;bb_trackball_decode(&m,r,501,3,false,false,&e);assert(e.down&&!e.clicks);
    r[1]=40;bb_trackball_decode(&m,r,601,3,false,false,&e);assert(!e.x&&e.down);
    r[1]=0;r[4]=1;bb_trackball_decode(&m,r,701,3,false,false,&e);assert(!e.down&&!e.clicks);
    r[4]=2;bb_trackball_decode(&m,r,801,3,false,false,&e);assert(e.clicks==1); /* Tap between reads. */
    r[4]=0;r[1]=2;bb_trackball_decode(&m,r,901,3,false,false,&e);assert(!e.x);
    r[1]=1;bb_trackball_decode(&m,r,1201,3,false,false,&e);assert(!e.x); /* Stale partial roll discarded. */
    m=(bb_trackball_motion_t){0};r[1]=3;
    bb_trackball_decode(&m,r,UINT32_MAX-20,3,true,false,&e);assert(e.x==-1);
    r[1]=0;r[3]=3;bb_trackball_decode(&m,r,70,3,false,false,&e);assert(e.y==1); /* Clock wrap. */
    puts("trackball motion: threshold, dominant axis, inversion, rate cap, click state, tap counter and timer wrap passed");
}
