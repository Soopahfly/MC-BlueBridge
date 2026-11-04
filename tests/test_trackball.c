#define BB_TRACKBALL_I2C 1
#define BB_TRACKBALL_SDA 2
#define BB_TRACKBALL_SCL 3
#define BB_TRACKBALL_ADDRESS 0x0a
#define BB_TRACKBALL_COUNTS_PER_STEP 3
#define BB_TRACKBALL_INVERT_X 0
#define BB_TRACKBALL_INVERT_Y 0
#include "../src/trackball.c"
#include <assert.h>
#include <stdio.h>
i2c_inst_t test_i2c[2];
static uint32_t now;
static void service(void){
    test_i2c[1].hw.status=I2C_IC_STATUS_TFNF_BITS;
    test_i2c[1].hw.raw_intr_stat=0;
    bb_trackball_poll(now++);
}
static void finish(const uint8_t* data){
    assert(busy);service();
    if(rx_length){
        assert(test_i2c[1].hw.data_cmd&I2C_IC_DATA_CMD_CMD_BITS);
        memcpy(rx,data,rx_length);received=rx_length; /* Emulated hardware RX FIFO contents. */
    }
    assert(test_i2c[1].hw.data_cmd&I2C_IC_DATA_CMD_STOP_BITS);
    test_i2c[1].hw.raw_intr_stat=I2C_IC_RAW_INTR_STAT_STOP_DET_BITS;
    bb_trackball_poll(now++);
    assert(!busy);
}
static void begin_input(void){
    for(unsigned budget=0;budget<30&&!busy;budget++)service();
    assert(job==INPUT_POINTER&&busy&&tx[0]==4);
    finish(NULL);assert(job==INPUT_WAIT);
    now+=20;service();assert(job==INPUT_READ&&busy);
}
int main(void){
    bb_trackball_init();service();assert(!busy&&!ready);
    now=100;service();assert(busy&&job==PROBE_POINTER&&tx[0]==0xfa);
    assert(test_i2c[1].hw.tar==0x0a);
    finish(NULL);assert(job==PROBE_WAIT);
    now+=18;service();assert(!busy); /* Pointer/read pause. */
    service();assert(busy&&job==PROBE_READ);
    const uint8_t id[]={0x11,0xba};finish(id);assert(bb_trackball_available());
    bb_trackball_colour(0,24,0,0);
    const uint8_t input[]={0,3,0,0,0};begin_input();finish(input);
    bb_trackball_event_t e;bb_trackball_take(&e);assert(e.x==1&&!e.down);
    bb_trackball_take(&e);assert(!e.x&&!e.clicks);
    service();assert(job==LED_WRITE&&busy&&tx[0]==0&&tx[2]==24);
    bb_trackball_colour(32,0,0,0);finish(NULL);
    assert(applied[1]==24&&applied[0]==0); /* Cache actual in-flight colour. */
    const uint8_t held[]={0,0,0,0,0x80};begin_input();finish(held);
    bb_trackball_take(&e);assert(e.down);
    service();assert(busy&&tx[1]==32);finish(NULL);
    begin_input();finish(held);service();assert(!busy&&job==INPUT_POINTER); /* Unchanged LED suppressed. */
    begin_input();
    test_i2c[1].hw.raw_intr_stat=I2C_IC_RAW_INTR_STAT_TX_ABRT_BITS;
    bb_trackball_poll(now++);assert(!ready&&failed&&!busy);
    bb_trackball_take(&e);assert(!e.down&&!e.clicks); /* No stuck click after loss. */
    unsigned saved_job=job;for(unsigned n=0;n<100;n++)service();assert(job==saved_job); /* No retry storm. */
    bb_trackball_init();service();now+=100;service();finish(NULL);now+=20;service();
    const uint8_t wrong_id[]={0x00,0x00};finish(wrong_id);assert(failed&&!ready);
    bb_trackball_init();service();now+=100;service();
    test_i2c[1].hw.status=0;bb_trackball_poll(now++);now+=101;bb_trackball_poll(now++);
    assert(failed&&!busy); /* Stuck-bus timeout has no blocking wait. */
    puts("trackball driver: chip ID, timed reads, input events, LED cache, NACK, device loss and timeout passed");
}
