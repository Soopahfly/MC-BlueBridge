#include "bluebridge_state.h"
#include "config_store.h"
#include "controller_input.h"
#include "hardware/flash.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
uint8_t test_flash[PICO_FLASH_SIZE_BYTES];
int test_flash_error;
unsigned test_flash_erases,test_core;
uint32_t test_time;
static bb_config_t cfg,loaded;
void bb_usb_submit(const bb_gamepad_report_t* report){(void)report;}
static uint32_t hash(const void* data,size_t size){const uint8_t* p=data;uint32_t h=2166136261u;while(size--){h^=*p++;h*=16777619u;}return h;}
int main(void){
    assert(bb_controller_capabilities(BB_CONTROLLER_STEAM,"Steam Controller").buttons==0x33fffu);
    assert(!strcmp(bb_controller_button_label(BB_CONTROLLER_STEAM,"Steam Controller",16),"Left grip"));
    assert(!strcmp(bb_controller_kind_name(BB_CONTROLLER_STEAM),"Steam Controller"));
    memset(test_flash,255,sizeof(test_flash));bb_state_init();
    assert(!bb_touch_mouse_get()&&!bb_touch_mouse_current());
    bb_steam_aim_t aim=bb_steam_aim_get();assert(!aim.enabled&&aim.sensitivity==100&&aim.pause_button==16);
    aim.enabled=1;aim.activation=2;aim.flags=7;aim.sensitivity=150;
    assert(bb_pointer_settings_set(aim,true)&&bb_mode_restart_pending());assert(!bb_touch_mouse_current());
    bb_config_flush_pending();bb_state_init();bb_steam_aim_t active=bb_steam_aim_current();
    assert(bb_touch_mouse_current()&&!memcmp(&aim,&active,sizeof(aim)));
    aim.sensitivity=0;assert(!bb_pointer_settings_set(aim,false)&&bb_touch_mouse_get());
    aim.sensitivity=201;assert(!bb_pointer_settings_set(aim,false));aim.sensitivity=100;
    aim.flags=8;assert(!bb_pointer_settings_set(aim,false));aim.flags=0;
    aim.activation=3;assert(!bb_pointer_settings_set(aim,false));aim.activation=0;
    aim.pause_button=18;assert(!bb_pointer_settings_set(aim,false));aim.pause_button=255;assert(bb_pointer_settings_set(aim,true));
    bb_config_get(&cfg);cfg.schema=13;strcpy(cfg.adapter_name,"Controller test");cfg.touch_mouse_enabled=1;
    unsigned base=PICO_FLASH_SIZE_BYTES-16*FLASH_SECTOR_SIZE;
    uint32_t header[]={0x42424733u,BB_CONFIG_V13_SIZE,hash(&cfg,BB_CONFIG_V13_SIZE)};
    memcpy(test_flash+base,header,sizeof(header));memcpy(test_flash+base+sizeof(header),&cfg,BB_CONFIG_V13_SIZE);
    bb_state_init();bb_config_get(&loaded);assert(loaded.schema==15&&!strcmp(loaded.adapter_name,"Controller test"));
    assert(!bb_touch_mouse_get()&&!bb_steam_aim_get().enabled&&bb_steam_aim_get().sensitivity==100);
    cfg.schema=13;assert(bb_config_import(&cfg,bb_config_checksum(&cfg)));assert(!bb_touch_mouse_get());
    bb_config_get(&cfg);cfg.steam_aim.flags=8;assert(!bb_config_import(&cfg,bb_config_checksum(&cfg)));
    cfg.steam_aim.flags=0;cfg.touch_mouse_enabled=2;assert(!bb_config_import(&cfg,bb_config_checksum(&cfg)));
    bb_config_get(&cfg);cfg.schema=14;cfg.touch_mouse_enabled=1;cfg.steam_aim.enabled=1;
    uint32_t old_header[]={0x42424733u,BB_CONFIG_V14_SIZE,hash(&cfg,BB_CONFIG_V14_SIZE)};
    memcpy(test_flash+base,old_header,sizeof(old_header));memcpy(test_flash+base+sizeof(old_header),&cfg,BB_CONFIG_V14_SIZE);
    bb_state_init();assert(bb_touch_mouse_current()&&bb_steam_aim_current().enabled&&!bb_ds4_aim_current().enabled);
    assert(bb_ds4_aim_get().sensitivity==100&&bb_ds4_aim_get().pause_button==11);
    bb_ds4_aim_t ds4=bb_ds4_aim_get();ds4.enabled=1;ds4.sensitivity=125;ds4.flags=4;
    assert(bb_controller_pointer_set(true,bb_steam_aim_get(),ds4));bb_config_flush_pending();bb_state_init();
    assert(bb_ds4_aim_current().enabled&&bb_ds4_aim_current().sensitivity==125&&bb_steam_aim_current().enabled);
    ds4.pause_button=16;assert(!bb_controller_pointer_set(false,bb_steam_aim_get(),ds4)&&bb_touch_mouse_get());
    bb_config_get(&cfg);cfg.ds4_aim.activation=3;assert(!bb_config_import(&cfg,bb_config_checksum(&cfg)));
    cfg.schema=14;assert(bb_config_import(&cfg,bb_config_checksum(&cfg))&&!bb_ds4_aim_get().enabled);
    assert(bb_config_reset());assert(!bb_touch_mouse_get()&&!bb_steam_aim_get().enabled);
    printf("Controller settings: defaults, atomic validation, restart, flash round-trip, schema-13 migration/import and reset passed (%zu bytes)\n",sizeof(cfg));
}
