#pragma once
#include <stdbool.h>
#include <stdint.h>
/* Global, boot-applied settings for the original Steam Controller only. */
typedef struct __attribute__((packed)) {
    uint8_t enabled, sensitivity, activation, flags, pause_button;
} bb_steam_aim_t;
/* Live settings, independent of the boot-applied gyro configuration. */
typedef struct __attribute__((packed)) {
    uint8_t right_click, haptics, haptic_gain, led_brightness;
} bb_steam_features_t;
static inline bb_steam_features_t bb_steam_features_defaults(void) {
    return (bb_steam_features_t){0,0,50,100};
}
static inline bool bb_steam_features_valid(bb_steam_features_t f) {
    return f.right_click<=1 && f.haptics<=1 && f.haptic_gain<=100 && f.led_brightness<=100;
}
#define BB_AIM_INVERT_X 1u
#define BB_AIM_INVERT_Y 2u
#define BB_AIM_TRIGGER_FIRE 4u
static inline bb_steam_aim_t bb_steam_aim_defaults(void) {
    return (bb_steam_aim_t){0,100,0,0,16};
}
static inline bool bb_steam_aim_valid(bb_steam_aim_t a) {
    return a.enabled<=1 && a.sensitivity>=1 && a.sensitivity<=200 && a.activation<=2 &&
        !(a.flags&~7u) && (a.pause_button==255 || a.pause_button<=17);
}
/* Independent DS4 settings share the encoding, not the saved value. */
static inline bb_steam_aim_t bb_ds4_aim_defaults(void) {
    return (bb_steam_aim_t){0,100,1,BB_AIM_INVERT_X,10};
}
static inline bool bb_ds4_aim_valid(bb_steam_aim_t a) {
    return bb_steam_aim_valid(a) && (a.pause_button==255 || a.pause_button<=13);
}
