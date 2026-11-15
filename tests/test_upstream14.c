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
static bb_config_t cfg,out;
static uint8_t raw[BB_CONFIG_UPSTREAM14_SIZE];
static uint32_t hash(const void* data,size_t size){
    const uint8_t* p=data;uint32_t h=2166136261u;while(size--){h^=*p++;h*=16777619u;}return h;
}
int main(int argc,char** argv){
    memset(test_flash,255,sizeof(test_flash));bb_state_init();
    const uint8_t address[6]={1,2,3,4,5,6};uint8_t index;
    assert(bb_controller_activate_or_create(address,0x054c,0x09cc,"DS4 saved pad",&index));
    bb_config_get(&cfg);cfg.controllers[0].profiles[0].button_map[0]=1;
    cfg.controllers[0].profiles[0].button_map[1]=0;
    memcpy(cfg.adapter_name,"Saved adapter",14);
    memcpy(raw,&cfg,BB_CONFIG_V13_SIZE);uint32_t schema=14;memcpy(raw,&schema,4);
    size_t at=BB_CONFIG_V13_SIZE;raw[at++]=1;
    bb_steam_aim_t aim={1,134,2,3,10};memcpy(raw+at,&aim,sizeof(aim));at+=sizeof(aim);
    bb_lightbar_config_t light=bb_lightbar_defaults();light.theme=0;light.players[0]=(bb_rgb_t){4,5,6};
    light.controllers[0]=(bb_lightbar_choice_t){3,1,{7,8,9}};
    memcpy(raw+at,&light,sizeof(light));
    assert(BB_CONFIG_UPSTREAM14_SIZE==11039&&BB_CONFIG_V14_SIZE!=sizeof(raw));
    assert(bb_config_from_upstream14(raw,sizeof(raw),&out));
    assert(out.schema==20&&!memcmp((uint8_t*)&out+4,raw+4,BB_CONFIG_V13_SIZE-4));
    assert(out.touch_mouse_enabled==1&&!memcmp(&out.ds4_aim,&aim,sizeof(aim)));
    assert(!memcmp(&out.lightbar,&light,sizeof(light))&&!out.steam_aim.enabled&&out.oled_enabled);
    assert(!out.panel_input);for(unsigned i=0;i<sizeof(out.oled_pew);i++)assert(!((uint8_t*)out.oled_pew)[i]);
    assert(bb_config_import(&out,bb_config_checksum(&out)));
    uint32_t header[]={0x42424733u,sizeof(raw),hash(raw,sizeof(raw))};
    unsigned base=PICO_FLASH_SIZE_BYTES-16*FLASH_SECTOR_SIZE;
    memcpy(test_flash+base,header,sizeof(header));memcpy(test_flash+base+sizeof(header),raw,sizeof(raw));
    assert(bb_config_load(&cfg)&&cfg.schema==20&&!memcmp(&cfg,&out,sizeof(cfg)));
    bb_state_init();assert(bb_touch_mouse_current()&&bb_ds4_aim_current().sensitivity==134);
    test_flash[base+sizeof(header)+5]^=1;assert(!bb_config_load(&cfg));
    assert(!bb_config_from_upstream14(raw,sizeof(raw)-1,&cfg));
    raw[BB_CONFIG_V13_SIZE]=2;assert(!bb_config_from_upstream14(raw,sizeof(raw),&cfg));
    raw[BB_CONFIG_V13_SIZE]=1;raw[BB_CONFIG_V13_SIZE+2]=0;
    assert(!bb_config_from_upstream14(raw,sizeof(raw),&cfg));
    /* Also validate/convert a real exported stock payload when supplied locally. */
    if(argc==3){
        FILE* in=fopen(argv[1],"rb");assert(in&&fread(raw,1,sizeof(raw),in)==sizeof(raw));
        assert(fgetc(in)==EOF);fclose(in);
        assert(bb_config_from_upstream14(raw,sizeof(raw),&out));
        assert(!memcmp((uint8_t*)&out+4,raw+4,BB_CONFIG_V13_SIZE-4));
        FILE* dest=fopen(argv[2],"wb");assert(dest&&fwrite(&out,1,sizeof(out),dest)==sizeof(out));fclose(dest);
    }
    puts("upstream schema 14: exact profile/pairing/settings prefix, DS4/colour migration, flash CRC and malformed input passed");
}
