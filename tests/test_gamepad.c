#include "oled_gamepad.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t bitmap[1024],neutral[1024],other[1024];
static void save_picture(const char* name){
    FILE* f=fopen(name,"wb");assert(f);fputs("P4\n128 64\n",f);
    for(unsigned y=0;y<64;y++)for(unsigned block=0;block<16;block++){
        unsigned value=0;
        for(unsigned bit=0;bit<8;bit++)if(bitmap[y/8*128+block*8+bit]&(1u<<(y%8)))value|=128u>>bit;
        fputc((int)value,f);
    }
    fclose(f);
}
int main(void){
    assert(bb_pad_layout(BB_CONTROLLER_DUALSENSE)==BB_PAD_PLAYSTATION);
    assert(bb_pad_layout(BB_CONTROLLER_XBOX_SERIES)==BB_PAD_XBOX);
    assert(bb_pad_layout(BB_CONTROLLER_SWITCH_PRO)==BB_PAD_NINTENDO);
    assert(bb_pad_layout(BB_CONTROLLER_UNKNOWN)==BB_PAD_GENERIC);
    bb_pad_picture_t v={0};v.layout=BB_PAD_PLAYSTATION;
    bb_pad_render(neutral,&v);v.turbo_buttons=1;
    bb_pad_render(bitmap,&v);
    assert(bitmap[40/8*128+110]&(1u<<(40%8))); /* Outer South ring, released. */
    assert(memcmp(bitmap,neutral,1024));
    save_picture("build-host/pad-playstation.pbm");
    memcpy(other,bitmap,1024);v.buttons=1;bb_pad_render(bitmap,&v);
    assert(memcmp(bitmap,other,1024)); /* Press fills without losing ring. */
    assert(bitmap[40/8*128+110]&(1u<<(40%8)));
    v.buttons=0;v.turbo_buttons=0;v.dpad=1;bb_pad_render(bitmap,&v);assert(memcmp(bitmap,neutral,1024));
    v.dpad=0;v.x=4;v.ry=-4;bb_pad_render(bitmap,&v);assert(memcmp(bitmap,neutral,1024));
    v.x=v.ry=0;v.lt=8;bb_pad_render(bitmap,&v);assert(memcmp(bitmap,neutral,1024));
    v.layout=BB_PAD_XBOX;v.turbo_buttons=1;bb_pad_render(bitmap,&v);save_picture("build-host/pad-xbox.pbm");
    memcpy(other,bitmap,1024);v.layout=BB_PAD_NINTENDO;bb_pad_render(bitmap,&v);assert(memcmp(bitmap,other,1024));
    save_picture("build-host/pad-nintendo.pbm");
    puts("gamepad: family layouts, press fill, persistent turbo rings, D-pad, sticks and triggers passed");
}
