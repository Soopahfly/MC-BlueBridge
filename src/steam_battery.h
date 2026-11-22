#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct __attribute__((packed)) {
    uint8_t led, haptics, strength;
    uint16_t threshold_mv;
} bb_steam_battery_config_t;
static inline bb_steam_battery_config_t bb_steam_battery_defaults(void) {
    /* Experimental voltage threshold: opt in after checking the installed AAs. */
    return (bb_steam_battery_config_t){0,0,35,2200};
}
static inline bool bb_steam_battery_config_valid(bb_steam_battery_config_t c) {
    return c.led<=1 && c.haptics<=1 && c.strength>=1 && c.strength<=100 &&
        c.threshold_mv>=1800 && c.threshold_mv<=3000;
}
typedef struct {
    uint32_t sample_at,reports,candidate_at,warning_at,pattern_at,refresh_at,test_at;
    uint16_t millivolts,threshold_mv,candidate_samples;
    bool have_voltage,low,warned,pattern_active,test_active;
} bb_steam_battery_t;
typedef struct {
    bool valid,alert_active,refresh;
    uint8_t left,right,brightness;
} bb_steam_battery_output_t;

bool bb_steam_battery_sample(bb_steam_battery_t* s,uint16_t mv,uint32_t now,bb_steam_battery_config_t c);
bb_steam_battery_output_t bb_steam_battery_tick(bb_steam_battery_t* s,uint32_t now,
    bb_steam_battery_config_t c,uint8_t brightness,bool test);
