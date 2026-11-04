#include "usb_report_queue.h"

static bool same_edges(const bb_gamepad_report_t* a, const bb_gamepad_report_t* b) {
    return a->buttons == b->buttons && a->hat == b->hat &&
           !!a->lt == !!b->lt && !!a->rt == !!b->rt;
}
bool bb_queue_push(bb_usb_queue_t* q, const bb_gamepad_report_t* r, uint32_t now) {
    if (q->count) {
        unsigned last = (q->head + q->count - 1u) % BB_USB_QUEUE_DEPTH;
        if (same_edges(&q->entries[last].report, r)) {
            q->entries[last].report = *r;
            q->entries[last].sequence = ++q->sequence;
            /* Keep the original edge time for honest backpressure age. */
            return false;
        }
    }
    bool overflow = q->count == BB_USB_QUEUE_DEPTH;
    if (overflow) {
        q->head = (q->head + 1u) % BB_USB_QUEUE_DEPTH;
        --q->count;
        ++q->overflows;
    }
    unsigned tail = (q->head + q->count) % BB_USB_QUEUE_DEPTH;
    q->entries[tail] = (bb_usb_queue_entry_t){ .report=*r, .sequence=++q->sequence, .at=now };
    if (++q->count > q->high_water) q->high_water = q->count;
    return overflow;
}
bool bb_queue_peek(const bb_usb_queue_t* q, bb_usb_queue_entry_t* entry) {
    if (!q->count) return false;
    *entry = q->entries[q->head];
    return true;
}
void bb_queue_ack(bb_usb_queue_t* q, uint32_t sequence, uint32_t now) {
    if (!q->count || q->entries[q->head].sequence != sequence) return;
    uint32_t age = now - q->entries[q->head].at;
    if (age > q->max_age_ms) q->max_age_ms = age;
    q->head = (q->head + 1u) % BB_USB_QUEUE_DEPTH;
    --q->count;
}
void bb_queue_reset(bb_usb_queue_t* q, const bb_gamepad_report_t* r, uint32_t now) {
    q->head = q->count = 0;
    bb_queue_push(q,r,now);
}
