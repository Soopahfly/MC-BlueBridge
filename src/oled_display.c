#include "oled_display.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/regs/i2c.h"
#include <ctype.h>
#include <string.h>

/* This module owns the selected I2C peripheral. All calls are on USB core 1.
 * FIFO service is bounded; it never waits for a byte, ACK or bus completion. */
static uint8_t desired[1024], sent[1024], packet[135];
static unsigned length, offset, next_page;
static int inflight_page = -1;
static uint32_t started;
static bool available, busy, initializing;
static bool valid[8];
static char previous[8][22];

/* Original compact 5x7 glyphs, columns with bit 0 at the top. */
static const uint8_t letters[26][5] = {
    {126,9,9,9,126},{127,73,73,73,54},{62,65,65,65,34},
    {127,65,65,34,28},{127,73,73,73,65},{127,9,9,9,1},
    {62,65,73,73,122},{127,8,8,8,127},{65,65,127,65,65},
    {32,64,65,63,1},{127,8,20,34,65},{127,64,64,64,64},
    {127,2,12,2,127},{127,4,8,16,127},{62,65,65,65,62},
    {127,9,9,9,6},{62,65,81,33,94},{127,9,25,41,70},
    {38,73,73,73,50},{1,1,127,1,1},{63,64,64,64,63},
    {31,32,64,32,31},{63,64,56,64,63},{99,20,8,20,99},
    {3,4,120,4,3},{97,81,73,69,67}
};
static const uint8_t digits[10][5] = {
    {62,81,73,69,62},{0,66,127,64,0},{98,81,73,73,70},
    {34,65,73,73,54},{24,20,18,127,16},{39,69,69,69,57},
    {62,73,73,73,48},{1,113,9,5,3},{54,73,73,73,54},{6,73,73,73,62}
};
static void glyph(char value, uint8_t out[5]) {
    unsigned char c = (unsigned char)toupper((unsigned char)value);
    memset(out, 0, 5);
    if (c >= 'A' && c <= 'Z') memcpy(out, letters[c - 'A'], 5);
    else if (c >= '0' && c <= '9') memcpy(out, digits[c - '0'], 5);
    else switch (c) {
        case ':': out[2] = 36; break;
        case '.': out[2] = 64; break;
        case '-': memset(out, 8, 5); break;
        case '!': out[2] = 95; break;
        case '?': out[0]=2; out[1]=1; out[2]=81; out[3]=9; out[4]=6; break;
        case '>': out[1]=65; out[2]=34; out[3]=20; out[4]=8; break;
        case '<': out[0]=8; out[1]=20; out[2]=34; out[3]=65; break;
        case '[': out[1]=127; out[2]=65; break;
        case ']': out[2]=65; out[3]=127; break;
        case '/': out[0]=32; out[1]=16; out[2]=8; out[3]=4; out[4]=2; break;
        case '%': out[0]=99; out[1]=19; out[2]=8; out[3]=100; out[4]=99; break;
        case '+': out[0]=8; out[1]=8; out[2]=62; out[3]=8; out[4]=8; break;
        case '=': memset(out, 20, 5); break;
        default: break;
    }
}

void bb_oled_text(const char rows[8][22]) {
    for (unsigned page = 0; page < 8; ++page) {
        if (!memcmp(previous[page], rows[page], 22)) continue;
        memcpy(previous[page], rows[page], 22);
        memset(desired + page * 128, 0, 128);
        for (unsigned c = 0; c < 21 && rows[page][c]; ++c)
            glyph(rows[page][c], desired + page * 128 + c * 6);
    }
}

static i2c_inst_t* bus(void) { return BB_OLED_I2C ? i2c1 : i2c0; }
static void start(unsigned size, uint32_t now) {
    i2c_hw_t* hw = i2c_get_hw(bus());
    (void)hw->clr_stop_det;
    (void)hw->clr_tx_abrt;
    length = size;
    offset = 0;
    started = now;
    busy = true;
}

void bb_oled_init(void) {
    i2c_init(bus(), 400000);
    gpio_set_function(BB_OLED_SDA, GPIO_FUNC_I2C);
    gpio_set_function(BB_OLED_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(BB_OLED_SDA);
    gpio_pull_up(BB_OLED_SCL);
    i2c_hw_t* hw = i2c_get_hw(bus());
    hw->enable = 0;
    hw->tar = BB_OLED_ADDRESS;
    hw->enable = 1;
    static const uint8_t init[] = {
        0x00,0xae,0xd5,0x80,0xa8,0x3f,0xd3,0x00,0x40,
        0x8d,0x14,0x20,0x02,0xa1,0xc8,0xda,0x12,
        0x81,0x7f,0xd9,0xf1,0xdb,0x40,0xa4,0xa6,0xaf
    };
    memcpy(packet, init, sizeof(init));
    available = true;
    initializing = true;
    start(sizeof(init), 0);
}

bool bb_oled_available(void) { return available; }

void bb_oled_poll(uint32_t now) {
    if (!available) return;
    i2c_hw_t* hw = i2c_get_hw(bus());
    if (busy) {
        /* Seed the startup timeout on first service, not absolute boot time. */
        if (initializing && !started) started = now;
        if (hw->raw_intr_stat & I2C_IC_RAW_INTR_STAT_TX_ABRT_BITS) {
            hw->enable = 0;
            available = false; /* No retry storm when the display is absent. */
            busy = false;
            return;
        }
        /* At most one FIFO's worth of writes per call, regardless of bus speed. */
        unsigned before = offset;
        for (unsigned budget = 0; budget < 16 && offset < length; ++budget) {
            if (!(hw->status & I2C_IC_STATUS_TFNF_BITS)) break;
            uint32_t value = packet[offset++];
            if (offset == length) value |= I2C_IC_DATA_CMD_STOP_BITS;
            hw->data_cmd = value;
        }
        if (offset != before) started = now;
        if (offset != length || !(hw->raw_intr_stat & I2C_IC_RAW_INTR_STAT_STOP_DET_BITS)) {
            /* Timeout on lack of progress. A delayed USB-loop service must not
             * discard a transfer that already completed during flash/CDC work. */
            if ((uint32_t)(now - started) >= 100u) {
                hw->enable = 0; available = false; busy = false;
            }
            return;
        }
        (void)hw->clr_stop_det;
        busy = false;
        if (inflight_page >= 0) {
            memcpy(sent + inflight_page * 128, packet + 7, 128);
            valid[inflight_page] = true;
            inflight_page = -1;
        }
        initializing = false;
    }
    for (unsigned n = 0; n < 8; ++n) {
        unsigned page = next_page++ % 8;
        if (valid[page] && !memcmp(sent + page * 128, desired + page * 128, 128)) continue;
        /* Co=1 commands followed by Co=0 data, in SSD1306 page addressing mode. */
        packet[0]=0x80; packet[1]=(uint8_t)(0xb0 | page);
        packet[2]=0x80; packet[3]=0x00;
        packet[4]=0x80; packet[5]=0x10; packet[6]=0x40;
        memcpy(packet + 7, desired + page * 128, 128);
        inflight_page = (int)page;
        start(135, now);
        return;
    }
}
