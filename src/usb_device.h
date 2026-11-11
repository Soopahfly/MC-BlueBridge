#pragma once
#include <stdbool.h>
#include "bluebridge_state.h"
#include "touch_mouse.h"

void bb_usb_core1(void);
void bb_usb_submit(const bb_gamepad_report_t* report);
void bb_usb_set_press_latch(bool enabled);
bool bb_usb_ready(void);
void bb_usb_request_reenumeration(void);
void bb_usb_service_pending(void);

typedef struct {
    uint32_t overflows, max_age_ms, oldest_age_ms;
    uint8_t depth, high_water;
} bb_usb_queue_stats_t;
void bb_usb_queue_stats(uint8_t slot, bb_usb_queue_stats_t* out);

bool bb_usb_touch_mouse_enabled(void);
void bb_usb_touch_mouse_submit(bb_pointer_sample_t sample,bool event_driven);
void bb_usb_touch_mouse_release(void);
