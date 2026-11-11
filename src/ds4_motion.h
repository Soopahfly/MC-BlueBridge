#pragma once
#include "steam_aim.h"
#include "controller_input.h"
#include "touch_mouse.h"
/* Same option layout, independently saved for DS4. Activation 1 = touchpad. */
typedef bb_steam_aim_t bb_ds4_aim_t;
static inline bb_ds4_aim_t bb_ds4_aim_defaults(void){return (bb_ds4_aim_t){0,100,0,0,11};}
static inline bool bb_ds4_aim_valid(bb_ds4_aim_t a){return bb_steam_aim_valid(a)&&(a.pause_button==255||a.pause_button<=13);}
typedef struct {
    int16_t gyro_bias[3],accel_bias[3],raw_gyro[3],raw_accel[3];
    int32_t gyro_denominator[3],accel_range[3],gyro_speed;
    int32_t gyro_q10[3],accel_mg[3];
    int64_t remainder_x,remainder_y;
    uint32_t at,samples,invalid;
    uint16_t timestamp;
    uint8_t calibration; /* 0 missing, 1 factory calibration valid, 2 rejected */
    bool have_timestamp,active;
} bb_ds4_motion_t;
/* Full Bluetooth 0x05, 41 bytes, feature CRC seed 0xa3. Atomic validation. */
bool bb_ds4_motion_calibrate(bb_ds4_motion_t* s,const uint8_t* p,uint16_t length);
/* Always decode sensors for diagnostics. Modify input/mouse only when eligible,
 * configured and calibrated. No accelerometer-to-stick/tilt mapping here. */
bool bb_ds4_motion_decode(bb_ds4_motion_t* s,const uint8_t* p,uint16_t length,
    uint32_t now,bb_ds4_aim_t aim,bool eligible,bool touching,
    bb_input_state_t* input,bb_pointer_sample_t* pointer);
