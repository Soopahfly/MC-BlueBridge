#include "bluebridge_state.h"
#include "config_store.h"
#include "hardware/flash.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
uint8_t test_flash[PICO_FLASH_SIZE_BYTES];int test_flash_error;unsigned test_flash_erases,test_core;uint32_t test_time;
static bb_config_t cfg;static uint32_t hash(const void* data,size_t n){const uint8_t* p=data;uint32_t h=2166136261u;while(n--){h^=*p++;h*=16777619u;}return h;}
int main(void){
    memset(test_flash,255,sizeof(test_flash));bb_state_init();bb_ds4_mouse_t c=bb_ds4_mouse_get();assert(!c.right_click&&!c.scroll&&c.scroll_step==64);
    bb_config_get(&cfg);cfg.schema=14;cfg.touch_mouse_enabled=1;cfg.ds4_aim.enabled=1;cfg.ds4_aim.sensitivity=150;cfg.lightbar.controllers[0].custom_enabled=1;cfg.lightbar.controllers[0].rgb=(bb_rgb_t){10,20,30};
    unsigned base=PICO_FLASH_SIZE_BYTES-16*FLASH_SECTOR_SIZE;uint32_t header[]={0x42424733u,BB_CONFIG_V14_SIZE,hash(&cfg,BB_CONFIG_V14_SIZE)};
    memcpy(test_flash+base,header,sizeof(header));memcpy(test_flash+base+sizeof(header),&cfg,BB_CONFIG_V14_SIZE);
    bb_state_init();assert(bb_touch_mouse_current()&&bb_ds4_aim_current().enabled&&bb_ds4_aim_current().sensitivity==150&&!bb_ds4_mouse_current().scroll);
    bb_lightbar_config_t lightbar;bb_lightbar_get(&lightbar);assert(lightbar.controllers[0].custom_enabled&&lightbar.controllers[0].rgb.b==30);
    c.right_click=1;c.scroll=1;assert(bb_ds4_mouse_settings_set(c,true)&&bb_mode_restart_pending());assert(!bb_ds4_mouse_current().scroll);
    bb_config_flush_pending();bb_state_init();assert(bb_ds4_mouse_current().scroll&&bb_ds4_mouse_current().right_click==1&&bb_ds4_aim_current().sensitivity==150);
    c.scroll_step=0;assert(!bb_ds4_mouse_settings_set(c,false)&&bb_touch_mouse_get());c.scroll_step=15;assert(!bb_ds4_mouse_settings_set(c,false));
    c.scroll_step=64;c.right_click=4;assert(!bb_ds4_mouse_settings_set(c,false));c.right_click=3;c.right_button=14;assert(!bb_ds4_mouse_settings_set(c,false));
    c.right_button=5;c.invert_scroll=2;assert(!bb_ds4_mouse_settings_set(c,false));c.invert_scroll=0;assert(bb_ds4_mouse_settings_set(c,true));
    bb_config_get(&cfg);cfg.ds4_mouse.scroll_step=0;assert(!bb_config_import(&cfg,bb_config_checksum(&cfg)));cfg.schema=14;
    assert(bb_config_import(&cfg,bb_config_checksum(&cfg))&&!bb_ds4_mouse_get().scroll&&bb_touch_mouse_get());
    assert(bb_config_reset()&&!bb_ds4_mouse_get().scroll&&!bb_touch_mouse_get());
    printf("Mouse settings: schema-14 migration, existing gyro/lightbar preservation, atomic save/validation, restart and reset passed (%zu bytes)\n",sizeof(cfg));
}
