#include "steam_battery.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    bb_steam_battery_t s={0},other={0};
    bb_steam_battery_config_t c=bb_steam_battery_defaults();
    bb_steam_battery_output_t o=bb_steam_battery_tick(&s,0,c,70,false);
    assert(!o.valid&&!o.alert_active&&o.brightness==70);
    assert(!bb_steam_battery_sample(&s,0,0,c)&&!bb_steam_battery_sample(&s,65535,0,c));
    assert(bb_steam_battery_sample(&s,2100,100,c));
    assert(bb_steam_battery_sample(&s,2100,1100,c));assert(!s.low);
    assert(bb_steam_battery_sample(&s,2300,2100,c));assert(!s.low&&!s.candidate_samples);
    for(unsigned i=0;i<3;i++)bb_steam_battery_sample(&s,2100,3000+i*1000,c);
    assert(s.low);
    o=bb_steam_battery_tick(&s,5000,c,70,false);assert(o.valid&&!o.alert_active&&o.brightness==70);
    c.haptics=1;c.led=1;
    o=bb_steam_battery_tick(&s,5000,c,70,false);assert(o.alert_active&&o.refresh&&o.left==89&&!o.right&&!o.brightness);
    o=bb_steam_battery_tick(&s,5080,c,70,false);assert(o.alert_active&&o.refresh&&!o.left&&!o.right);
    o=bb_steam_battery_tick(&s,5160,c,70,false);assert(o.alert_active&&!o.left&&o.right==89&&o.brightness==70);
    o=bb_steam_battery_tick(&s,5240,c,70,false);assert(o.alert_active&&!o.left&&!o.right);
    o=bb_steam_battery_tick(&s,5400,c,70,false);assert(o.alert_active&&o.left==59&&o.right==59);
    o=bb_steam_battery_tick(&s,5480,c,70,false);assert(o.alert_active&&o.refresh&&o.left==59&&o.right==59);
    o=bb_steam_battery_tick(&s,5560,c,70,false);assert(!o.alert_active&&o.refresh);
    /* Fresh low readings do not restart it; reminders wait five minutes. */
    bb_steam_battery_sample(&s,2100,6000,c);
    o=bb_steam_battery_tick(&s,6000,c,70,false);assert(!o.alert_active);
    bb_steam_battery_sample(&s,2100,304999,c);
    /* A gap invalidates classification and requires confirmation again. */
    assert(!s.low);bb_steam_battery_sample(&s,2100,305999,c);bb_steam_battery_sample(&s,2100,306999,c);
    o=bb_steam_battery_tick(&s,306999,c,70,false);assert(o.alert_active);
    /* Stay fresh for an entire reminder interval. */
    for(unsigned i=1;i<=299;i++) {
        bb_steam_battery_sample(&s,2100,306999+i*1000,c);
        o=bb_steam_battery_tick(&s,306999+i*1000,c,70,false);assert(!o.alert_active);
    }
    bb_steam_battery_sample(&s,2100,606999,c);
    o=bb_steam_battery_tick(&s,606999,c,70,false);assert(o.alert_active);
    c.haptics=0;o=bb_steam_battery_tick(&s,607010,c,70,false);assert(!o.alert_active);
    /* Recovery requires >=150mV hysteresis, three samples, two seconds. */
    bb_steam_battery_sample(&s,2300,608000,c);assert(s.low);
    bb_steam_battery_sample(&s,2400,609000,c);bb_steam_battery_sample(&s,2400,610000,c);assert(s.low);
    bb_steam_battery_sample(&s,2400,611000,c);assert(!s.low);
    o=bb_steam_battery_tick(&s,611000,c,70,false);assert(!o.alert_active&&o.brightness==70);
    /* Test uses the real overlay but never fabricates a battery reading. */
    c=bb_steam_battery_defaults();o=bb_steam_battery_tick(&other,0,c,0,true);
    assert(!o.valid&&o.alert_active&&o.left==89&&o.brightness==25&&!other.have_voltage&&!other.low);
    o=bb_steam_battery_tick(&other,560,c,0,false);assert(!o.alert_active);
    o=bb_steam_battery_tick(&other,2000,c,0,false);assert(!o.alert_active&&!o.brightness&&!other.test_active);
    /* High-rate samples still need two seconds; stale data cannot warn. */
    other=(bb_steam_battery_t){0};c.haptics=1;c.led=1;
    for(unsigned i=0;i<100;i++)bb_steam_battery_sample(&other,2000,100+i,c);
    assert(!other.low);bb_steam_battery_sample(&other,2000,2100,c);assert(other.low);
    o=bb_steam_battery_tick(&other,32100,c,70,false);assert(!o.valid&&!o.alert_active&&!other.low&&o.brightness==70);
    /* Changing threshold reclassifies from fresh reports; timers wrap safely. */
    other=(bb_steam_battery_t){0};
    bb_steam_battery_sample(&other,2000,0xfffff800u,c);bb_steam_battery_sample(&other,2000,0xfffffc00u,c);
    bb_steam_battery_sample(&other,2000,0,c);assert(other.low);
    o=bb_steam_battery_tick(&other,0,c,70,false);assert(o.alert_active);
    c.threshold_mv=1900;o=bb_steam_battery_tick(&other,10,c,70,false);assert(!other.low&&!o.alert_active);
    assert(s.reports&&!other.test_active);
    puts("Steam battery: debounce, hysteresis, freshness, opt-in, pad pattern, reminders, test isolation, threshold changes and timer wrap passed");
}
