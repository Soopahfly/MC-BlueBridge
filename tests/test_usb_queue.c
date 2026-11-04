#include "usb_report_queue.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    bb_usb_queue_t q={0};bb_gamepad_report_t r={0};bb_usb_queue_entry_t e;
    bb_queue_push(&q,&r,0);r.x=45;bb_queue_push(&q,&r,1);
    assert(q.count==1);assert(bb_queue_peek(&q,&e)&&e.report.x==45);
    /* Fake endpoint rejects this send: do not acknowledge. */
    r.buttons=1;bb_queue_push(&q,&r,2);r.buttons=0;bb_queue_push(&q,&r,3);
    assert(q.count==3);assert(bb_queue_peek(&q,&e)&&e.report.buttons==0);
    bb_queue_ack(&q,e.sequence,4);assert(bb_queue_peek(&q,&e)&&e.report.buttons==1);
    bb_queue_ack(&q,e.sequence,5);assert(bb_queue_peek(&q,&e)&&e.report.buttons==0);
    bb_queue_ack(&q,e.sequence,6);assert(!q.count);
    /* Trigger transitions are edges even without corresponding digital bits. */
    r.lt=50;bb_queue_push(&q,&r,7);r.lt=100;bb_queue_push(&q,&r,8);
    assert(q.count==1);r.lt=0;bb_queue_push(&q,&r,9);assert(q.count==2);
    /* Producer replaces analog while endpoint accepts the previous snapshot. */
    bb_queue_reset(&q,&r,10);bb_queue_peek(&q,&e);
    r.x=99;bb_queue_push(&q,&r,11);bb_queue_ack(&q,e.sequence,12);
    assert(q.count==1);bb_queue_peek(&q,&e);assert(e.report.x==99);
    /* Overflow drops the oldest report, retains bounded newest history, counts loss. */
    for(unsigned i=0;i<20;i++){r.buttons=i&1;bb_queue_push(&q,&r,20+i);}
    assert(q.count==BB_USB_QUEUE_DEPTH&&q.overflows&&q.high_water==BB_USB_QUEUE_DEPTH);
    uint32_t stale=e.sequence;bb_queue_ack(&q,stale,50);assert(q.count==BB_USB_QUEUE_DEPTH);
    while(bb_queue_peek(&q,&e))bb_queue_ack(&q,e.sequence,60);
    assert(q.max_age_ms>=21);
    bb_queue_reset(&q,&r,0xfffffff0u);bb_queue_peek(&q,&e);bb_queue_ack(&q,e.sequence,16);
    assert(!q.count);
    puts("USB queue: coalescing, digital/trigger edges, rejected sends, producer race, overflow and timer wrap passed");
}
