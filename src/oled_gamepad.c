#include "oled_gamepad.h"
#include <stdbool.h>
#include <string.h>

/* GP2040-CE's input display is the behavioral reference: pressed controls fill,
 * and turbo assignments have a separate ring. Controller geometry is original.
 * https://gp2040-ce.info/web-configurator/menu-pages/display-configuration/ */
bb_pad_layout_t bb_pad_layout(bb_controller_kind_t kind) {
    switch(kind) {
        case BB_CONTROLLER_DUALSENSE: case BB_CONTROLLER_DUALSENSE_EDGE:
        case BB_CONTROLLER_DUALSHOCK4: case BB_CONTROLLER_DUALSHOCK3: return BB_PAD_PLAYSTATION;
        case BB_CONTROLLER_XBOX_ONE: case BB_CONTROLLER_XBOX_SERIES: case BB_CONTROLLER_XBOX_ADAPTIVE:
        case BB_CONTROLLER_XBOX_ELITE: case BB_CONTROLLER_XBOX_WIRELESS: return BB_PAD_XBOX;
        case BB_CONTROLLER_SWITCH_PRO: case BB_CONTROLLER_SWITCH2_PRO:
        case BB_CONTROLLER_JOYCON_LEFT: case BB_CONTROLLER_JOYCON_RIGHT:
        case BB_CONTROLLER_NSO_NES: case BB_CONTROLLER_NSO_SNES: case BB_CONTROLLER_NSO_N64:
        case BB_CONTROLLER_NSO_GENESIS: case BB_CONTROLLER_NSO_GAMECUBE:
        case BB_CONTROLLER_WII_U_PRO: case BB_CONTROLLER_WII_REMOTE: case BB_CONTROLLER_WII_CLASSIC:
        case BB_CONTROLLER_8BITDO: return BB_PAD_NINTENDO;
        default: return BB_PAD_GENERIC;
    }
}
static void pixel(uint8_t* b,int x,int y,bool on) {
    if(x<0||x>=128||y<0||y>=64)return;
    unsigned pos=(unsigned)y/8*128+(unsigned)x;
    uint8_t bit=(uint8_t)(1u<<(y%8));
    if(on)b[pos]|=bit;else b[pos]&=(uint8_t)~bit;
}
static void line(uint8_t* b,int x,int y,int x2,int y2,bool on) {
    int dx=x2>x?x2-x:x-x2,sx=x<x2?1:-1;
    int dy=y2>y?y-y2:y2-y,sy=y<y2?1:-1,err=dx+dy;
    for(;;){pixel(b,x,y,on);if(x==x2&&y==y2)break;int e=2*err;if(e>=dy){err+=dy;x+=sx;}if(e<=dx){err+=dx;y+=sy;}}
}
static void rect(uint8_t* b,int x,int y,int w,int h,bool filled,bool on) {
    for(int yy=0;yy<h;yy++)for(int xx=0;xx<w;xx++)if(filled||!xx||!yy||xx==w-1||yy==h-1)pixel(b,x+xx,y+yy,on);
}
static void circle(uint8_t* b,int x,int y,int radius,bool filled,bool on) {
    int outer=radius*radius,inner=(radius-1)*(radius-1);
    for(int dy=-radius;dy<=radius;dy++)for(int dx=-radius;dx<=radius;dx++){
        int distance=dx*dx+dy*dy;
        if(distance<=outer&&(filled||distance>inner))pixel(b,x+dx,y+dy,on);
    }
}
static bool pressed(const bb_pad_picture_t* v,unsigned button){return (v->buttons&(1u<<button))!=0;}
static void face(uint8_t* b,const bb_pad_picture_t* v,int x,int y,unsigned button) {
    bool down=pressed(v,button);
    circle(b,x,y,5,down,true);
    if(v->turbo_buttons&(1u<<button))circle(b,x,y,7,false,true);
    bool ink=!down;
    if(v->layout==BB_PAD_PLAYSTATION){
        switch(button){
            case 0:line(b,x-2,y-2,x+2,y+2,ink);line(b,x-2,y+2,x+2,y-2,ink);break;
            case 1:circle(b,x,y,2,false,ink);break;
            case 2:rect(b,x-2,y-2,5,5,false,ink);break;
            case 3:line(b,x,y-2,x-2,y+2,ink);line(b,x-2,y+2,x+2,y+2,ink);line(b,x+2,y+2,x,y-2,ink);break;
        }
    } else {
        /* Tiny 3x5 ABXY glyphs keep the four labels legible inside circles. */
        static const uint8_t glyphs[4][3]={{30,5,30},{31,21,10},{17,10,17},{3,28,3}};
        unsigned label=button;
        if(v->layout==BB_PAD_NINTENDO)label=button==0?1:button==1?0:button==2?3:2;
        for(int gx=0;gx<3;gx++)for(int gy=0;gy<5;gy++)if(glyphs[label][gx]&(1u<<gy))pixel(b,x+gx-1,y+gy-2,ink);
    }
}
static void stick(uint8_t* b,const bb_pad_picture_t* v,int x,int y,bool right) {
    unsigned button=right?11:10;
    circle(b,x,y,7,false,true);
    if(v->turbo_buttons&(1u<<button))circle(b,x,y,9,false,true);
    if(pressed(v,button))circle(b,x,y,5,true,true);
    circle(b,x+(right?v->rx:v->x),y+(right?v->ry:v->y),2,true,!pressed(v,button));
}
void bb_pad_render(uint8_t bitmap[1024],const bb_pad_picture_t* v) {
    memset(bitmap,0,1024);
    /* Distinct silhouette: PS symmetric grips, Xbox/Switch asymmetric sticks. */
    static const int8_t ps[][2]={{12,14},{36,10},{49,15},{79,15},{92,10},{116,14},{124,40},{117,53},{101,50},{27,50},{11,53},{4,40},{12,14}};
    static const int8_t xbox[][2]={{12,14},{32,10},{47,14},{81,14},{96,10},{116,14},{124,42},{119,54},{108,53},{96,50},{32,50},{20,53},{9,54},{4,42},{12,14}};
    static const int8_t nintendo[][2]={{12,13},{37,11},{48,14},{80,14},{91,11},{116,13},{123,43},{116,53},{105,52},{94,50},{34,50},{23,52},{12,53},{5,43},{12,13}};
    const int8_t (*outline)[2]=ps;unsigned count=sizeof(ps)/sizeof(ps[0]);
    if(v->layout==BB_PAD_XBOX){outline=xbox;count=sizeof(xbox)/sizeof(xbox[0]);}
    if(v->layout==BB_PAD_NINTENDO){outline=nintendo;count=sizeof(nintendo)/sizeof(nintendo[0]);}
    for(unsigned n=1;n<count;n++)line(bitmap,outline[n-1][0],outline[n-1][1],outline[n][0],outline[n][1],true);
    for(unsigned side=0;side<2;side++){
        int x=side?91:15;unsigned shoulder=side?5:4,trigger=side?7:6;
        rect(bitmap,x,2,22,4,pressed(v,trigger),true);
        unsigned level=side?v->rt:v->lt;if(level>8)level=8;
        if(level)rect(bitmap,x+1,3,(int)(level*20/8),2,true,true);
        rect(bitmap,x,8,22,4,pressed(v,shoulder),true);
        if(v->turbo_buttons&(1u<<trigger))circle(bitmap,x+11,4,4,false,true);
        if(v->turbo_buttons&(1u<<shoulder))circle(bitmap,x+11,10,4,false,true);
    }
    bool symmetric=v->layout==BB_PAD_PLAYSTATION||v->layout==BB_PAD_GENERIC;
    int dpx=symmetric?26:44,dpy=symmetric?28:35;
    rect(bitmap,dpx-2,dpy-7,5,5,(v->dpad&1)!=0,true);
    rect(bitmap,dpx-2,dpy+3,5,5,(v->dpad&2)!=0,true);
    rect(bitmap,dpx-7,dpy-2,5,5,(v->dpad&4)!=0,true);
    rect(bitmap,dpx+3,dpy-2,5,5,(v->dpad&8)!=0,true);
    stick(bitmap,v,symmetric?47:26,symmetric?40:26,false);
    stick(bitmap,v,80,40,true);
    face(bitmap,v,103,40,0);face(bitmap,v,115,28,1);
    face(bitmap,v,91,28,2);face(bitmap,v,103,16,3);
    /* Select / Home / Start and touchpad/capture. */
    const int centres[]={51,64,77};const unsigned ids[]={8,12,9};
    for(unsigned n=0;n<3;n++){
        circle(bitmap,centres[n],22,2,pressed(v,ids[n]),true);
        if(v->turbo_buttons&(1u<<ids[n]))circle(bitmap,centres[n],22,4,false,true);
    }
    if(v->layout==BB_PAD_PLAYSTATION)rect(bitmap,53,12,22,6,pressed(v,13),true);
}
