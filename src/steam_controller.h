#pragma once
#include "controller_input.h"
#include "touch_mouse.h"
#include "steam_aim.h"

typedef struct {
    uint32_t buttons;
    int16_t stick_x,stick_y,left_x,left_y,right_x,right_y;
    uint8_t lt,rt;
    bool have_buttons;
} bb_steam_controls_t;
typedef struct {
    uint8_t payload[144], next_segment;
    uint32_t fragment_at, pointer_at, reports, malformed, fragments;
    bb_steam_controls_t controls;
    bb_steam_aim_t aim;
    uint32_t gyro_at;
    int32_t gyro_remainder_x,gyro_remainder_y;
    bool gyro_active;
    bool pointer_down;
    int16_t pointer_x,pointer_y,remainder_x,remainder_y;
} bb_steam_controller_t;
/* Original 2015 controller BLE only. Returns 1 for a complete input update,
 * 0 for an incomplete/irrelevant report, -1 for a malformed sequence/report.
 * Accepts 20-byte report-ID-prefixed or 19-byte raw GATT notifications. */
int bb_steam_controller_decode(bb_steam_controller_t* s,const uint8_t* data,uint16_t length,
    uint32_t now,bool mouse,bb_input_state_t* input,bb_pointer_sample_t* pointer);
