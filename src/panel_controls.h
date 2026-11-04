#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct { uint8_t previous; int8_t quarters; } bb_encoder_t;
/* Full quadrature detents only; illegal two-bit jumps reset the accumulator. */
int bb_encoder_update(bb_encoder_t* encoder, uint8_t pins, unsigned steps);
typedef struct {
    bool observed, stable, held;
    uint32_t changed_at, pressed_at;
} bb_panel_button_t;
/* 0: no event, 1: short release, 2: long hold (once; suppresses release). */
int bb_panel_button_update(bb_panel_button_t* button, bool down, uint32_t now);
