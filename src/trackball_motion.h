#pragma once
#include "trackball.h"
typedef struct {
    int x,y;
    uint32_t last_motion,last_step;
    bool have_step,have_sample,down;
} bb_trackball_motion_t;
void bb_trackball_decode(bb_trackball_motion_t* motion,const uint8_t registers[5],
    uint32_t now,unsigned threshold,bool invert_x,bool invert_y,bb_trackball_event_t* event);
