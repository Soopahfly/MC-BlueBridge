#include "bluebridge_state.h"
#include "config_store.h"
#include "hardware/flash.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
uint8_t test_flash[PICO_FLASH_SIZE_BYTES];
int test_flash_error;
unsigned test_flash_erases,test_core;
uint32_t test_time;
static bb_config_t before,after;
static uint32_t hash(const void* data,size_t size){
    const uint8_t* p=data;uint32_t h=2166136261u;while(size--){h^=*p++;h*=16777619u;}return h;
}
int main(void){
    memset(test_flash,255,sizeof(test_flash));bb_state_init();
    assert(!bb_ds4_right_click_get()&&!bb_ds4_right_click_current());
    bb_config_get(&before);before.schema=20;before.touch_mouse_enabled=1;
    before.ds4_aim=(bb_steam_aim_t){1,123,2,3,10};before.lightbar.theme=0;
    before.lightbar.players[0]=(bb_rgb_t){3,4,5};
    const uint8_t address[6]={1,2,3,4,5,6};uint8_t index;
    assert(bb_controller_activate_or_create(address,0x054c,0x09cc,"DS4",&index));
    bb_config_t record;bb_config_get(&record);before.controller_count=record.controller_count;
    before.controllers[0]=record.controllers[0];
    before.controllers[0].profiles[0].button_map[0]=1;
    uint32_t header[]={0x42424733u,BB_CONFIG_V20_SIZE,hash(&before,BB_CONFIG_V20_SIZE)};
    unsigned base=PICO_FLASH_SIZE_BYTES-16*FLASH_SECTOR_SIZE;
    memcpy(test_flash+base,header,sizeof(header));
    memcpy(test_flash+base+sizeof(header),&before,BB_CONFIG_V20_SIZE);
    bb_state_init();bb_config_get(&after);
    assert(after.schema==21&&BB_CONFIG_V20_SIZE==11110&&sizeof(after)==11111);
    assert(!memcmp((uint8_t*)&before+4,(uint8_t*)&after+4,BB_CONFIG_V20_SIZE-4));
    assert(!bb_ds4_right_click_get()&&!bb_ds4_right_click_current());
    bb_ds4_right_click_set(true);assert(bb_ds4_right_click_get()&&bb_ds4_right_click_current());
    assert(!bb_mode_restart_pending());test_time+=250;bb_config_flush_pending();bb_state_init();
    assert(bb_ds4_right_click_get()&&bb_ds4_right_click_current());
    bb_config_get(&after);after.ds4_right_click=2;
    assert(!bb_config_import(&after,bb_config_checksum(&after))&&bb_ds4_right_click_current());
    after.ds4_right_click=0;assert(bb_config_import(&after,bb_config_checksum(&after)));
    assert(!bb_ds4_right_click_current());
    bb_ds4_right_click_set(true);assert(bb_config_reset()&&!bb_ds4_right_click_current());
    puts("right-click setting: default off, schema-20 prefix preserved, live save, persistence, import validation and reset passed");
}
