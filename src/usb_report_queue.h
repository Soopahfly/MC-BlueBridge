#pragma once
#include "bluebridge_state.h"
#define BB_USB_QUEUE_DEPTH 8
typedef struct {
    bb_gamepad_report_t report;
    uint32_t sequence, at;
} bb_usb_queue_entry_t;
typedef struct {
    bb_usb_queue_entry_t entries[BB_USB_QUEUE_DEPTH];
    uint32_t sequence, overflows, max_age_ms;
    uint8_t head, count, high_water;
} bb_usb_queue_t;
/* Caller serializes all access. A failed transfer leaves the head untouched. */
bool bb_queue_push(bb_usb_queue_t* q, const bb_gamepad_report_t* r, uint32_t now);
bool bb_queue_peek(const bb_usb_queue_t* q, bb_usb_queue_entry_t* entry);
void bb_queue_ack(bb_usb_queue_t* q, uint32_t sequence, uint32_t now);
void bb_queue_reset(bb_usb_queue_t* q, const bb_gamepad_report_t* r, uint32_t now);
