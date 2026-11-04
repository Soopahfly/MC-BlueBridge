#include "bluebridge_state.h"
#include "config_store.h"
#include "hardware/flash.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
uint8_t test_flash[PICO_FLASH_SIZE_BYTES];
unsigned test_core;
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
    assert(loaded.schema==14 && loaded.controllers[0].native_product_id==2);
    assert(bb_profile_oled_get(0)==0);
    /* Old exports are padded by the CDC importer before validation. */
    loaded.schema=13; loaded.oled_pew[0][0]=0x80;
    assert(bb_config_import(&loaded,bb_config_checksum(&loaded)));
    assert(bb_profile_oled_get(0)==0);
    bb_config_get(&loaded); loaded.oled_pew[0][0]=0x9f;
    assert(!bb_config_import(&loaded,bb_config_checksum(&loaded)));
    test_flash[base+sizeof(header)+10]^=1;
    assert(!bb_config_load(&loaded)); /* Corruption rejected. */
    printf("state: edge events, profile lifecycle, flash round-trip and schema migration passed (%zu bytes)\n",sizeof(cfg));
}
