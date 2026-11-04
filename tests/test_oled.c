#define BB_OLED_I2C 0
#define BB_OLED_SDA 4
#define BB_OLED_SCL 5
#define BB_OLED_ADDRESS 0x3c
#include "../src/oled_display.c"
#include <assert.h>
#include <stdio.h>
i2c_inst_t test_i2c[2];
static unsigned transactions;
static uint32_t clock_ms=1;
static void finish(void) {
    unsigned before=transactions;
    while(busy && before==transactions) {
        test_i2c[0].hw.status=I2C_IC_STATUS_TFNF_BITS;
        test_i2c[0].hw.raw_intr_stat=0;
        if(offset==length) {
            test_i2c[0].hw.raw_intr_stat=I2C_IC_RAW_INTR_STAT_STOP_DET_BITS;
            ++transactions;
        }
        unsigned prior=offset;
        bb_oled_poll(clock_ms++);
        if(before==transactions) assert(offset-prior<=16);
    }
}
static void settle(void) {
    for(unsigned budget=0;budget<30 && busy;++budget) finish();
    assert(!busy);
}
int main(void) {
    char screen[8][22]={{0}};
    bb_oled_init(); settle();
    assert(bb_oled_available() && transactions==9); /* Init plus eight pages. */
    unsigned old=transactions;
    for(unsigned n=0;n<1000;++n) bb_oled_poll(clock_ms++);
    assert(transactions==old && !busy); /* Idle screen emits nothing. */
    snprintf(screen[2],22,"USB: MISTER"); bb_oled_text(screen);
    bb_oled_poll(clock_ms++);
    assert(busy && inflight_page==2 && length==135);
    assert(packet[0]==0x80 && packet[1]==0xb2 && packet[6]==0x40);
    assert(!memcmp(packet+7,desired+256,128));
    settle(); assert(transactions==old+1); /* Only one changed page. */
    uint8_t normal[1024]; memcpy(normal,desired,sizeof(normal));
    bb_oled_pew(0); assert(memcmp(desired,normal,sizeof(normal)));
    uint8_t top[1024]; memcpy(top,desired,sizeof(top));
    bb_oled_pew(2); assert(memcmp(desired,top,sizeof(top)));
    uint8_t comic[1024]; memcpy(comic,desired,sizeof(comic));
    FILE* preview=fopen("build-host/pew.pbm","wb");
    assert(preview);
    fputs("P4\n128 64\n",preview);
    for(unsigned y=0;y<64;++y) for(unsigned byte=0;byte<16;++byte) {
        uint8_t pixels=0;
        for(unsigned bit=0;bit<8;++bit)
            if(comic[y/8*128+byte*8+bit]&(1u<<(y%8))) pixels|=(uint8_t)(0x80u>>bit);
        fputc(pixels,preview);
    }
    fclose(preview);
    bb_oled_pew(2); assert(!memcmp(desired,comic,sizeof(comic)));
    bb_oled_text(screen); assert(!memcmp(desired,normal,sizeof(normal)));
    bb_oled_text(screen); bb_oled_poll(clock_ms++);
    assert(!busy);
    snprintf(screen[2],22,"USB: SWITCH"); bb_oled_text(screen);
    bb_oled_poll(clock_ms++); assert(busy);
    uint8_t original[128]; memcpy(original,packet+7,128);
    snprintf(screen[2],22,"USB: X-INPUT"); bb_oled_text(screen);
    finish();
    assert(!memcmp(sent+256,original,128)); /* Commit actual sent data, not newer target. */
    settle(); assert(!memcmp(sent+256,desired+256,128));
    snprintf(screen[4],22,"DELAYED SERVICE"); bb_oled_text(screen);
    bb_oled_poll(clock_ms++);
    while(offset<length) {
        test_i2c[0].hw.raw_intr_stat=0;
        bb_oled_poll(clock_ms++);
    }
    clock_ms+=200;
    test_i2c[0].hw.raw_intr_stat=I2C_IC_RAW_INTR_STAT_STOP_DET_BITS;
    bb_oled_poll(clock_ms++);
    assert(available && !busy); /* Completed transfer survives delayed service. */
    snprintf(screen[3],22,"PEW PEW!"); bb_oled_text(screen);
    bb_oled_poll(clock_ms++); assert(busy);
    test_i2c[0].hw.raw_intr_stat=I2C_IC_RAW_INTR_STAT_TX_ABRT_BITS;
    bb_oled_poll(clock_ms++);
    assert(!available && !busy && test_i2c[0].hw.enable==0);
    unsigned prior=offset;
    for(unsigned n=0;n<1000;++n) bb_oled_poll(clock_ms++);
    assert(offset==prior); /* Missing display does not cause retry traffic. */
    test_i2c[0].hw.raw_intr_stat=0;
    test_i2c[0].hw.status=0;
    bb_oled_init(); bb_oled_poll(clock_ms++);
    clock_ms+=101; bb_oled_poll(clock_ms++);
    assert(!available && !busy); /* Stuck bus times out without blocking. */
    test_i2c[0].hw.raw_intr_stat=0;test_i2c[0].hw.status=I2C_IC_STATUS_TFNF_BITS;
    bb_oled_init_off();assert(length==2&&packet[0]==0&&packet[1]==0xae);
    unsigned old_transactions=transactions;settle();
    assert(transactions==old_transactions+1&&!available&&!busy);
    for(unsigned n=0;n<1000;n++)bb_oled_poll(clock_ms++);
    assert(transactions==old_transactions+1); /* Exactly one off command, then zero traffic. */
    puts("oled: idle suppression, changed pages, in-flight updates, NACK and timeout passed");
}
