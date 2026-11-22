#include "steam_battery.h"

static void configure(bb_steam_battery_t* s,bb_steam_battery_config_t c) {
    if(s->threshold_mv==c.threshold_mv)return;
    s->threshold_mv=c.threshold_mv;s->candidate_samples=0;s->low=false;s->warned=false;
    if(!s->test_active)s->pattern_active=false;
}
bool bb_steam_battery_sample(bb_steam_battery_t* s,uint16_t mv,uint32_t now,bb_steam_battery_config_t c) {
    if(mv<1000 || mv>4000 || !bb_steam_battery_config_valid(c))return false;
    configure(s,c);
    if(s->have_voltage && (uint32_t)(now-s->sample_at)>=30000u) {
        s->candidate_samples=0;s->low=false;s->warned=false;
    }
    s->millivolts=mv;s->sample_at=now;s->have_voltage=true;++s->reports;
    bool candidate=s->low?mv>=c.threshold_mv+150u:mv<=c.threshold_mv;
    if(!candidate){s->candidate_samples=0;return true;}
    if(!s->candidate_samples)s->candidate_at=now;
    if(s->candidate_samples<65535u)++s->candidate_samples;
    /* Three fresh reports spanning >=2 seconds reject transient load sag. */
    if(s->candidate_samples>=3 && (uint32_t)(now-s->candidate_at)>=2000u) {
        s->low=!s->low;s->candidate_samples=0;s->warned=false;
        if(!s->low && !s->test_active)s->pattern_active=false;
    }
    return true;
}
bb_steam_battery_output_t bb_steam_battery_tick(bb_steam_battery_t* s,uint32_t now,
    bb_steam_battery_config_t c,uint8_t brightness,bool test) {
    bb_steam_battery_output_t out={.brightness=brightness};
    if(!bb_steam_battery_config_valid(c))return out;
    configure(s,c);
    out.valid=s->have_voltage && (uint32_t)(now-s->sample_at)<30000u;
    if(!out.valid){s->low=false;s->candidate_samples=0;s->warned=false;}
    if(s->test_active && (uint32_t)(now-s->test_at)>=2000u)s->test_active=false;
    if(!s->test_active && (!c.haptics || !s->low))s->pattern_active=false;
    if(!c.haptics)s->warned=false;
    if(test || (!s->test_active && !s->pattern_active && out.valid && s->low && c.haptics && (!s->warned || (uint32_t)(now-s->warning_at)>=300000u))) {
        s->pattern_active=true;s->pattern_at=s->refresh_at=now;
        if(test){s->test_active=true;s->test_at=now;if(s->low){s->warned=true;s->warning_at=now;}}
        else {s->warned=true;s->warning_at=now;}
        out.refresh=true;
    }
    if(s->pattern_active) {
        uint32_t elapsed=now-s->pattern_at;
        if(elapsed>=560u){s->pattern_active=false;out.refresh=true;}
        else {
            out.alert_active=true;
            uint8_t tap=(uint8_t)(255u*c.strength/100u);
            if(elapsed<80u)out.left=tap;
            else if(elapsed>=160u && elapsed<240u)out.right=tap;
            else if(elapsed>=400u)out.left=out.right=(uint8_t)(tap*2u/3u);
            if((uint32_t)(now-s->refresh_at)>=80u){out.refresh=true;s->refresh_at=now;}
        }
    }
    if(s->test_active || (out.valid && s->low && c.led)) {
        uint32_t phase=s->test_active?(now-s->test_at)%5000u:now%5000u;
        bool blink=phase<120u || (phase>=260u && phase<380u);
        /* A selected brightness of zero still permits explicitly enabled alerts. */
        out.brightness=brightness?(blink?0:brightness):(blink?25:0);
    }
    return out;
}
