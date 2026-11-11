#include "steam_controller.h"
#include <string.h>

static int16_t signed16(const uint8_t* p) {
    uint16_t v=(uint16_t)(p[0]|((uint16_t)p[1]<<8));
    return (int16_t)(v<=32767u?(int32_t)v:(int32_t)v-65536);
}
static int16_t invert(int16_t v){return v==(-32767-1)?32767:(int16_t)-v;}
static int invalid(bb_steam_controller_t* s){s->next_segment=0;++s->malformed;return -1;}

static int decode_payload(bb_steam_controller_t* s,unsigned size,uint32_t now,bool mouse,
    bb_input_state_t* input,bb_pointer_sample_t* pointer) {
    const uint8_t* p=s->payload;
    if(size<2)return invalid(s);
    if((p[0]&15u)!=4u)return 0; /* Status and other non-input messages. */
    uint16_t flags=(uint16_t)((p[0]&0xf0u)|((uint16_t)p[1]<<8));
    if(flags&0xe000u)return invalid(s);
    static const uint8_t sizes[]={3,2,3,4,4,4,6,6,8};
    unsigned required=2;
    for(unsigned bit=0;bit<9;bit++)if(flags&(0x10u<<bit))required+=sizes[bit];
    if(required>size)return invalid(s);
    /* Delta reports omit unchanged fields. Apply atomically after bounds checks. */
    bb_steam_controls_t c=s->controls;
    unsigned at=2;
    bool was_touch=(c.buttons&0x100000u)!=0;
    if(flags&0x10u){c.buttons=p[at]|((uint32_t)p[at+1]<<8)|((uint32_t)p[at+2]<<16);at+=3;c.have_buttons=true;}
    if(flags&0x20u){c.lt=p[at++];c.rt=p[at++];}
    if(flags&0x40u)at+=3; /* Additional button bytes: not used by the original pad. */
    if(flags&0x80u){c.stick_x=signed16(p+at);c.stick_y=signed16(p+at+2);at+=4;}
    if(flags&0x100u){c.left_x=signed16(p+at);c.left_y=signed16(p+at+2);at+=4;}
    if(flags&0x200u){c.right_x=signed16(p+at);c.right_y=signed16(p+at+2);}
    s->controls=c;
    memset(input,0,sizeof(*input));*pointer=(bb_pointer_sample_t){0};
    input->x=c.stick_x;input->y=invert(c.stick_y);input->lt=c.lt;input->rt=c.rt;
    static const uint8_t source[]={7,5,6,4,3,2,1,0,12,14,22,18,13,17,15,16};
    static const uint8_t target[]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,16,17};
    for(unsigned i=0;i<sizeof(source);i++)if(c.buttons&(1u<<source[i]))input->buttons|=1u<<target[i];
    if(c.buttons&0x100u)input->dpad|=1;
    if(c.buttons&0x800u)input->dpad|=2;
    if(c.buttons&0x400u)input->dpad|=4;
    if(c.buttons&0x200u)input->dpad|=8;
    if(c.buttons&0x80000u) {
        if(c.left_y>8192)input->dpad|=1;
        if(c.left_y< -8192)input->dpad|=2;
        if(c.left_x< -8192)input->dpad|=4;
        if(c.left_x>8192)input->dpad|=8;
    }
    bool touching=c.have_buttons&&(c.buttons&0x100000u)!=0;
    if(!mouse) {
        if(touching){input->rx=c.right_x;input->ry=invert(c.right_y);}
        s->pointer_down=false;s->remainder_x=s->remainder_y=0;
    } else {
        pointer->buttons=(c.buttons&0x40000u)?1:0;
        input->buttons&=~(1u<<11); /* Right pad press is left mouse, not R3. */
        if(!touching||!was_touch||(uint32_t)(now-s->pointer_at)>500u) {
            s->pointer_down=false;s->remainder_x=s->remainder_y=0;
        }
        if((flags&0x200u)&&touching) {
            int32_t dx=(int32_t)c.right_x-s->pointer_x,dy=(int32_t)s->pointer_y-c.right_y;
            if(s->pointer_down && dx>=-16384&&dx<=16384&&dy>=-16384&&dy<=16384) {
                int32_t x=dx+s->remainder_x,y=dy+s->remainder_y;
                pointer->x=(int16_t)(x/128);pointer->y=(int16_t)(y/128);
                s->remainder_x=(int16_t)(x%128);s->remainder_y=(int16_t)(y%128);
            } else s->remainder_x=s->remainder_y=0;
            s->pointer_x=c.right_x;s->pointer_y=c.right_y;s->pointer_down=true;
            s->pointer_at=now;
        }
    }
    ++s->reports;return 1;
}

int bb_steam_controller_decode(bb_steam_controller_t* s,const uint8_t* data,uint16_t length,
    uint32_t now,bool mouse,bb_input_state_t* input,bb_pointer_sample_t* pointer) {
    if(!data)return invalid(s);
    if(length==20){if(data[0]!=3)return 0;++data;--length;}
    if(length!=19)return invalid(s);
    uint8_t header=data[0],segment=header&7u;
    if(!(header&0x80u))return 0;
    if(header&0x38u)return invalid(s);
    if(s->next_segment && (uint32_t)(now-s->fragment_at)>100u)s->next_segment=0;
    if(!segment)s->next_segment=0;
    if(segment!=s->next_segment)return invalid(s);
    memcpy(s->payload+18u*segment,data+1,18);
    s->fragment_at=now;++s->fragments;
    if(header&0x40u){s->next_segment=0;return decode_payload(s,18u*(segment+1u),now,mouse,input,pointer);}
    if(segment==7)return invalid(s);
    s->next_segment=(uint8_t)(segment+1u);return 0;
}
