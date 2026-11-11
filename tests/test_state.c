#include "bluebridge_state.h"
#include "config_store.h"
#include "hardware/flash.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
uint8_t test_flash[PICO_FLASH_SIZE_BYTES];
int test_flash_error;
unsigned test_flash_erases;
unsigned test_core;
uint32_t test_time;
static bb_config_t cfg, loaded;
static uint32_t hash(const void* data,size_t size) {
    const uint8_t* p=data; uint32_t h=2166136261u;
    while(size--) { h^=*p++; h*=16777619u; } return h;
}
static void input(uint32_t buttons) { bb_state_update_live_input(0,0,0,0,0,0,0,buttons); }
static uint32_t events(void) { bb_runtime_state_t r; bb_state_get(&r); return r.pew_events; }
int main(void) {
    memset(test_flash,255,sizeof(test_flash));
    bb_state_init();
    assert(bb_panel_input_get()==BB_PANEL_NONE&&bb_panel_input_current()==BB_PANEL_NONE);
    assert(!bb_panel_input_set((bb_panel_input_t)3));
    const uint8_t a[6]={1,2,3,4,5,6}, b[6]={2,3,4,5,6,7};
    uint8_t ci,pi;
    assert(bb_controller_activate_or_create(a,0x054c,0x0ce6,"DualSense",&ci));
    assert(ci==0 && bb_controller_count()==1);
    bb_state_set_controller(true,ci,"DualSense");
    assert(bb_profile_oled_get(0)==0);
    input(1); assert(events()==0); /* Default off. */
    input(0);
    assert(bb_profile_set_oled(0,true,0));
    input(1); assert(events()==1);
    input(1); assert(events()==1); /* Held reports do not retrigger. */
    input(0); input(1); assert(events()==2);
    assert(bb_profile_set_oled(0,true,3));
    input(0); input(1); assert(events()==2);
    input(8); assert(events()==3);
    assert(!bb_profile_set_oled(0,true,26));
    assert(!bb_profile_set_oled(8,true,0));
    assert(bb_capture_start(BB_CAPTURE_ONCE));
    input(0); input(8); assert(events()==3); /* Remap capture suppresses effect. */
    bb_capture_stop();
    assert(bb_profile_duplicate(0,"Copy",&pi) && pi==1);
    assert(bb_profile_oled_get(pi)==0x83);
    assert(bb_profile_create("Other",&pi) && pi==2);
    assert(bb_profile_oled_get(pi)==0);
    assert(bb_profile_set_oled(pi,true,5));
    assert(bb_profile_delete(1));
    assert(bb_profile_oled_get(1)==0x85); /* Follows shifted profile. */
    assert(bb_profile_create("Fresh",&pi) && pi==2);
    assert(bb_profile_oled_get(pi)==0);
    assert(bb_controller_activate_or_create(b,1,2,"Second",&ci) && ci==1);
    assert(bb_controller_select(1));
    assert(bb_profile_set_oled(0,true,4));
    assert(bb_controller_forget(0,NULL));
    assert(bb_profile_oled_get(0)==0x84); /* Follows shifted controller. */
    bb_config_get(&cfg);
    assert(bb_config_save(&cfg));
    assert(bb_config_load(&loaded));
    assert(!memcmp(&cfg,&loaded,sizeof(cfg)));
    assert(sizeof(cfg)+12 <= 4*FLASH_SECTOR_SIZE);

    /* Simulate a real schema-13 flash record: original size and checksum. */
    cfg.schema=13;
    uint32_t header[]={0x42424733u,BB_CONFIG_V13_SIZE,hash(&cfg,BB_CONFIG_V13_SIZE)};
    unsigned base=PICO_FLASH_SIZE_BYTES-16*FLASH_SECTOR_SIZE;
    memset(test_flash+base,255,4*FLASH_SECTOR_SIZE);
    memcpy(test_flash+base,header,sizeof(header));
    memcpy(test_flash+base+sizeof(header),&cfg,BB_CONFIG_V13_SIZE);
    memset(&loaded,0xa5,sizeof(loaded));
    assert(bb_config_load(&loaded));
    assert(loaded.schema==13 && loaded.controller_count==1);
    for(unsigned c=0;c<BB_MAX_CONTROLLERS;++c)
        for(unsigned p=0;p<BB_MAX_PROFILES;++p) assert(loaded.oled_pew[c][p]==0);
    bb_state_init(); bb_config_get(&loaded);
    assert(loaded.schema==17 && loaded.controllers[0].native_product_id==2);
    assert(bb_panel_input_get()==BB_PANEL_ENCODER); /* Existing installations keep encoder behavior. */
    assert(bb_profile_oled_get(0)==0);
    /* Old exports are padded by the CDC importer before validation. */
    loaded.schema=13; loaded.oled_pew[0][0]=0x80;
    assert(bb_config_import(&loaded,bb_config_checksum(&loaded)));
    assert(bb_profile_oled_get(0)==0);
    bb_config_get(&loaded); loaded.oled_pew[0][0]=0x9f;
    assert(!bb_config_import(&loaded,bb_config_checksum(&loaded)));
    test_flash[base+sizeof(header)+10]^=1;
    assert(!bb_config_load(&loaded)); /* Corruption rejected. */
    bb_config_get(&loaded);loaded.schema=14;loaded.oled_pew[0][0]=0x84;
    uint32_t v14_header[]={0x42424733u,BB_CONFIG_V14_SIZE,hash(&loaded,BB_CONFIG_V14_SIZE)};
    memcpy(test_flash+base,v14_header,sizeof(v14_header));
    memcpy(test_flash+base+sizeof(v14_header),&loaded,BB_CONFIG_V14_SIZE);
    bb_state_init();assert(bb_panel_input_current()==BB_PANEL_ENCODER);
    assert(bb_profile_oled_get(0)==0x84); /* Schema-14 PEW settings survive. */
    assert(bb_panel_input_set(BB_PANEL_TRACKBALL));
    assert(bb_panel_input_get()==BB_PANEL_TRACKBALL&&bb_panel_input_current()==BB_PANEL_ENCODER);
    assert(bb_mode_restart_pending());test_time+=250;bb_config_flush_pending();bb_state_init();
    assert(bb_panel_input_current()==BB_PANEL_TRACKBALL);
    assert(bb_panel_input_set(BB_PANEL_NONE));test_time+=250;bb_config_flush_pending();bb_state_init();
    assert(bb_panel_input_current()==BB_PANEL_NONE);
    bb_config_get(&loaded);loaded.panel_input=3;
    assert(!bb_config_import(&loaded,bb_config_checksum(&loaded)));
    assert(bb_oled_enabled_get()&&bb_oled_enabled_current());
    assert(bb_panel_settings_set(BB_PANEL_NONE,false));
    assert(!bb_oled_enabled_get()&&bb_oled_enabled_current());
    test_time+=250;bb_config_flush_pending();bb_state_init();assert(!bb_oled_enabled_current());
    bb_config_get(&loaded);loaded.oled_enabled=2;
    assert(!bb_config_import(&loaded,bb_config_checksum(&loaded)));
    loaded.oled_enabled=0;loaded.schema=15;loaded.panel_input=BB_PANEL_TRACKBALL;
    uint32_t v15_header[]={0x42424733u,BB_CONFIG_V15_SIZE,hash(&loaded,BB_CONFIG_V15_SIZE)};
    memcpy(test_flash+base,v15_header,sizeof(v15_header));
    memcpy(test_flash+base+sizeof(v15_header),&loaded,BB_CONFIG_V15_SIZE);
    bb_state_init();assert(bb_oled_enabled_current()&&bb_panel_input_current()==BB_PANEL_TRACKBALL);
    test_time+=250;bb_config_flush_pending();assert(!bb_config_is_pending());
    unsigned erased=test_flash_erases;
    bb_config_get(&loaded);bb_config_set(&loaded,true);
    bb_config_flush_pending();assert(bb_config_is_pending()); /* Coalescing delay. */
    test_time+=250;bb_config_flush_pending();assert(!bb_config_is_pending());
    assert(test_flash_erases==erased); /* Identical image never erases flash. */
    assert(bb_panel_input_set(BB_PANEL_NONE));test_flash_error=-1;
    test_time+=250;bb_config_flush_pending();assert(bb_config_is_pending());
    assert(bb_config_save_failures()==1);
    bb_config_flush_pending();assert(bb_config_save_failures()==1); /* No hot retry loop. */
    test_flash_error=0;test_time+=1000;bb_config_flush_pending();
    assert(!bb_config_is_pending());bb_state_init();assert(bb_panel_input_current()==BB_PANEL_NONE);
    assert(bb_output_mode_set(BB_OUTPUT_DUALSENSE));assert(bb_output_mode_value()==BB_OUTPUT_DUALSENSE);
    test_time+=250;bb_config_flush_pending();bb_state_init();assert(bb_output_mode_value()==BB_OUTPUT_DUALSENSE);
    assert(!bb_output_mode_set((bb_output_mode_t)(BB_OUTPUT_LAST+1)));
    /* Schema 16 preserves panel/OLED/profile fields and defaults mouse off. */
    bb_config_get(&loaded);loaded.schema=16;loaded.oled_enabled=0;loaded.panel_input=BB_PANEL_TRACKBALL;
    uint32_t v16_header[]={0x42424733u,BB_CONFIG_V16_SIZE,hash(&loaded,BB_CONFIG_V16_SIZE)};
    memcpy(test_flash+base,v16_header,sizeof(v16_header));
    memcpy(test_flash+base+sizeof(v16_header),&loaded,BB_CONFIG_V16_SIZE);
    bb_state_init();assert(!bb_oled_enabled_get()&&bb_panel_input_current()==BB_PANEL_TRACKBALL&&!bb_touch_mouse_get());
    bb_touch_mouse_set(true);assert(bb_touch_mouse_get()&&!bb_touch_mouse_current());
    test_time+=250;bb_config_flush_pending();bb_state_init();assert(bb_touch_mouse_current());
    bb_config_get(&loaded);loaded.touch_mouse_enabled=2;assert(!bb_config_import(&loaded,bb_config_checksum(&loaded)));
    loaded.schema=16;assert(bb_config_import(&loaded,bb_config_checksum(&loaded)));
    assert(!bb_touch_mouse_get()&&!bb_oled_enabled_get());
    printf("state: edge events, profile lifecycle, flash round-trip and schema migration passed (%zu bytes)\n",sizeof(cfg));
}
