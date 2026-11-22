"""Run actual BT output/service functions with a controlled transport and clock."""
import os
from pathlib import Path
import re
import subprocess

root = Path(__file__).resolve().parents[1]
source = (root/'src/bt_host.c').read_text(encoding='utf-8')

def function(name):
    match = re.search(r'static (?:bool|void) '+name+r'\([^;{}]*\)\s*\{', source)
    assert match, name
    start = match.start()
    end = source.index('{', start) + 1
    depth = 1
    while depth:
        if source[end] == '{': depth += 1
        if source[end] == '}': depth -= 1
        end += 1
    return source[start:end]

prefix = r'''
#include "controller_driver.h"
#include "bluebridge_state.h"
#include "lightbar.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define ERROR_CODE_SUCCESS 0
#define ERROR_CODE_COMMAND_DISALLOWED 12
typedef struct {
    unsigned steam_client;
    bool ds4_lightbar_valid,ds4_rumble_retry,ds4_low_battery,ds4_blink_sent;
    bb_ds4_motors_t ds4_motors_sent;
    bb_rgb_t ds4_lightbar_sent;
    uint32_t ds4_lightbar_retry_at;
} bb_host_context_t;
static bb_host_context_t host_slots[2];
static uint8_t selected_slot,classic_hid_cid=1,ds4_activation_step=3,ds4_output_report[78];
static int current_controller_index;
static bool ds4_activation_pending,manual_disconnect_pending;
static bb_controller_identity_t current_identity;
static bb_runtime_state_t runtime;
static bb_rgb_t colour={64,0,0};
static uint32_t now,rumble_stop_at,rumble_refresh_at;
static uint8_t rumble_request_strength,rumble_request_big,rumble_request_small;
static uint8_t rumble_motor_big,rumble_motor_small,rumble_active_strength;
static uint16_t rumble_request_duration_ms;
static bool rumble_request_pending,rumble_active,ds3_leds_pending;
static bool xbox_rumble_retry,eightbitdo64_rumble_retry,ds3_rumble_retry,classic_switch_protocol;
static uint8_t next_status,last[78];
static unsigned successful;
uint8_t bb_slot_current(void){return selected_slot;}
void bb_state_get(bb_runtime_state_t* out){*out=runtime;}
bb_rgb_t bb_lightbar_colour(unsigned index,unsigned slot){(void)index;(void)slot;return colour;}
static uint32_t btstack_run_loop_get_time_ms(void){return now;}
static uint8_t send_classic_report(uint16_t cid,uint16_t id,const uint8_t* data,uint16_t size){
    assert(cid==1&&id==0x11&&size==77);
    uint8_t status=next_status;next_status=0;
    if(!status){last[0]=0x11;memcpy(last+1,data,77);++successful;}
    return status;
}
static bool xbox_ble_connected(void){return false;}
static bool eightbitdo64_ble_connected(void){return false;}
#define OTHER(name) static bool name(uint8_t strength){(void)strength;assert(0);return false;}
static bb_steam_features_t features={0,1,50,100};
static unsigned steam_writes;
static uint8_t steam_left,steam_right;
bb_steam_features_t bb_steam_features_get(void){return features;}
static bool bb_steam_ble_haptics(unsigned* c,uint8_t left,uint8_t right){
    (void)c;steam_left=left;steam_right=right;++steam_writes;return true;
}
OTHER(send_xbox_ble_rumble)
OTHER(send_dualsense_rumble)
OTHER(send_ds3_rumble)
OTHER(send_switch_rumble)
OTHER(send_switch2_rumble)
OTHER(send_8bitdo_dinput_rumble)
OTHER(send_wiiu_rumble)
'''
main = r'''
static void request(uint8_t strength,uint8_t big,uint8_t small,uint16_t duration){
    rumble_request_strength=strength;rumble_request_big=big;rumble_request_small=small;
    rumble_request_duration_ms=duration;rumble_request_pending=true;
}
int main(void){
    current_identity.kind=BB_CONTROLLER_DUALSHOCK4;
    runtime.battery_supported=true;runtime.battery_percent=55;runtime.battery_state=1;
    assert(send_ds4_rumble(0)&&!last[6]&&!last[7]);
    now=10;request(60,180,0,1000);service_rumble();
    assert(rumble_active&&!rumble_request_pending&&last[6]==0&&last[7]==180);
    colour=(bb_rgb_t){0,0,64};now=120;service_ds4_lightbar(now);
    assert(last[6]==0&&last[7]==180&&last[8]==0&&last[10]==64);
    now=121;request(100,0,255,100);next_status=ERROR_CODE_COMMAND_DISALLOWED;service_rumble();
    assert(rumble_request_pending&&host_slots[0].ds4_motors_sent.big==180);
    colour=(bb_rgb_t){0,64,0};now=220;service_ds4_lightbar(now);
    assert(last[7]==180&&last[6]==0&&rumble_request_pending); /* Keep last accepted motors. */
    now=221;service_rumble();assert(!rumble_request_pending&&last[6]==255&&!last[7]);
    now=321;next_status=ERROR_CODE_COMMAND_DISALLOWED;service_rumble();
    assert(rumble_active&&host_slots[0].ds4_rumble_retry&&last[6]==255);
    now=322;service_rumble();assert(!rumble_active&&!last[6]&&!last[7]);
    request(0,1,0,100);service_rumble();assert(rumble_active&&last[7]==1&&!last[6]);
    now=422;service_rumble();assert(!rumble_active&&!last[7]&&!last[6]);
    request(90,0,0,1000);service_rumble();assert(last[6]==90&&last[7]==90);
    runtime.battery_percent=15;now=520;service_ds4_lightbar(now);
    assert(last[6]==90&&last[7]==90&&last[11]==100&&(last[3]&4));
    request(40,60,0,1000);service_rumble();assert(!last[6]&&last[7]==60&&last[3]==3);
    request(0,0,0,100);service_rumble();assert(!rumble_active&&!last[6]&&!last[7]);
    ds4_activation_step=2;request(70,120,0,100);unsigned before=successful;service_rumble();
    assert(rumble_request_pending&&successful==before);
    ds4_activation_step=3;service_rumble();assert(!rumble_request_pending&&last[7]==120);
    /* Colour-only output in another slot cannot inherit slot 0 motors. */
    selected_slot=1;now=700;service_ds4_lightbar(now);
    assert(!last[6]&&!last[7]&&host_slots[0].ds4_motors_sent.big==120);
    current_identity.kind=BB_CONTROLLER_STEAM;now=1000;
    request(70,200,60,200);service_rumble();assert(steam_left==100&&steam_right==30&&rumble_active);
    unsigned steam_before=steam_writes;now=1080;service_rumble();assert(steam_writes==steam_before+1);
    now=1200;service_rumble();assert(!rumble_active&&!steam_left&&!steam_right);
    request(0,1,0,200);features.haptic_gain=100;service_rumble();assert(rumble_active&&steam_left==1&&!steam_right);
    features.haptics=0;now=1280;service_rumble();assert(!steam_left&&!steam_right);
    features.haptics=1;request(80,0,0,100);service_rumble();assert(steam_left==80&&steam_right==80);
    selected_slot=0;request(0,0,0,50);service_rumble();assert(!steam_left&&!steam_right);
    puts("Steam host output: separate channels, gain, finite-pulse refresh, tiny effects, disabled stop and fallback passed");
    puts("DS4 output service: routing, separate motors, fallback, busy start/stop, tiny effects, colours, blink and slot caches passed");
}
'''
out = root/'build-host'
out.mkdir(exist_ok=True)
unit = out/'ds4-output-service.c'
unit.write_text(prefix + '\n'.join(function(n) for n in [
    'send_ds4_motors', 'send_ds4_rumble', 'service_ds4_lightbar',
    'send_steam_rumble', 'send_current_rumble', 'service_rumble']) + main, encoding='utf-8')
exe = out/('ds4-output-service.exe' if os.name == 'nt' else 'ds4-output-service')
subprocess.run([os.environ.get('CC', 'gcc'), '-std=c11', '-Wall', '-Wextra', '-Werror',
                '-I'+str(root/'src'), str(unit), str(root/'src/lightbar.c'), '-o', str(exe)], check=True)
subprocess.run([str(exe)], check=True)
