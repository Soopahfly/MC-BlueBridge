#pragma once
#include <stdint.h>
#include "controller_input.h"

typedef enum { BB_PAD_GENERIC, BB_PAD_PLAYSTATION, BB_PAD_XBOX, BB_PAD_NINTENDO } bb_pad_layout_t;
typedef struct {
    bb_pad_layout_t layout;
    uint32_t buttons, turbo_buttons;
    uint8_t dpad, lt, rt;
    int8_t x,y,rx,ry; /* Quantized screen displacement -4..4. */
} bb_pad_picture_t;
bb_pad_layout_t bb_pad_layout(bb_controller_kind_t kind);
/* Draws into SSD1306 page-order memory. Does not perform I2C. */
void bb_pad_render(uint8_t bitmap[1024], const bb_pad_picture_t* picture);
