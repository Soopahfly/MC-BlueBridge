/* Render documentation frames with the actual firmware graphics code.
 * Hardware APIs are stubbed; no adapter or I2C connection is used. */
#include "oled_display.h"
#include "oled_gamepad.h"
#include "hardware/i2c.h"
#include <assert.h>
#include <stdio.h>
static uint8_t* bitmap;
i2c_inst_t test_i2c[2];
static void save(const char* name){
    char path[180];snprintf(path,sizeof(path),"build-host/demo-frames/%s.pbm",name);
    FILE* f=fopen(path,"wb");assert(f);fputs("P4\n128 64\n",f);
    for(unsigned y=0;y<64;y++)for(unsigned block=0;block<16;block++){
        unsigned value=0;
        for(unsigned bit=0;bit<8;bit++)if(bitmap[y/8*128+block*8+bit]&(1u<<(y%8)))value|=128u>>bit;
        fputc((int)value,f);
    }
    fclose(f);
}
static void pad(bb_pad_picture_t* v){bb_pad_render(bb_oled_graphics_begin(),v);bb_oled_graphics_footer("DEFAULT TURBO");}
int main(void){
    bitmap=bb_oled_graphics_begin();
    bb_pad_picture_t v={0};v.turbo_buttons=1;
    const char* names[]={"generic","playstation","xbox","nintendo"};
    for(unsigned layout=0;layout<4;layout++){
        v.layout=(bb_pad_layout_t)layout;pad(&v);save(names[layout]);
    }
    v.layout=BB_PAD_PLAYSTATION;
    for(unsigned n=0;n<12;n++){
        v.buttons=n==1?1:n==2?2:n==3?4:n==4?8:n==9?(1u<<4):0;
        v.dpad=n==5?1:n==6?8:0;
        v.x=n==7?4:0;v.y=n==8?-4:0;v.lt=n==10?8:0;
        pad(&v);char name[32];snprintf(name,sizeof(name),"input-%02u",n);save(name);
    }
    v.buttons=0;v.dpad=0;v.x=v.y=0;v.lt=0;pad(&v);save("pew-status");
    for(unsigned n=0;n<4;n++){bb_oled_pew((uint8_t)n);char name[32];snprintf(name,sizeof(name),"pew-%u",n);save(name);}
    char rows[8][22]={"DUALSENSE","","DEFAULT","","TURBO: ON 12 HZ","","BATTERY: 85%","PRESS FOR MENU"};
    bb_oled_text(rows);save("status");
    char menu[8][22]={"SETTINGS",""," USB MODE"," CONTROLLER",">PROFILE"," TURBO"," TURBO RATE","HOLD FOR STATUS"};
    bb_oled_text(menu);save("menu");
    return 0;
}
