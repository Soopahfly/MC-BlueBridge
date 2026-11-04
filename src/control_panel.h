#pragma once
#include <stdbool.h>
#ifndef BB_USE_ENCODER
#define BB_USE_ENCODER 1
#endif
#ifndef BB_USE_TRACKBALL
#define BB_USE_TRACKBALL 0
#endif
/* Call only from USB core 1. Disabled builds link to no-op implementations. */
void bb_panel_init(void);
void bb_panel_poll(void);
bool bb_panel_trackball_ready(void);
