#pragma once
#include <stdbool.h>
#include <stdint.h>
/* Boot-applied aiming option layout; DS4 stores an independent copy. */
typedef struct __attribute__((packed)) {
    uint8_t enabled, sensitivity, activation, flags, pause_button;
} bb_steam_aim_t;
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
