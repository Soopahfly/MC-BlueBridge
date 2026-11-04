#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct { int x,y; bool down; unsigned clicks; } bb_trackball_event_t;
void bb_trackball_init(void);
/* Core 1 only; services a bounded amount of FIFO work, never waits on the bus. */
void bb_trackball_poll(uint32_t now);
bool bb_trackball_available(void);
void bb_trackball_take(bb_trackball_event_t* event);
void bb_trackball_colour(uint8_t red,uint8_t green,uint8_t blue,uint8_t white);
