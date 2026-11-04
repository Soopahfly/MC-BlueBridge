#include "panel_controls.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    bb_encoder_t e = {3,0};
    /* CW detent, then CCW; contact bounce must cancel rather than add steps. */
    assert(bb_encoder_update(&e,1,4)==0);
    assert(bb_encoder_update(&e,0,4)==0);
    assert(bb_encoder_update(&e,2,4)==0);
    assert(bb_encoder_update(&e,3,4)==1);
    assert(bb_encoder_update(&e,2,4)==0);
    assert(bb_encoder_update(&e,0,4)==0);
    assert(bb_encoder_update(&e,1,4)==0);
    assert(bb_encoder_update(&e,3,4)==-1);
    assert(bb_encoder_update(&e,1,4)==0);
    assert(bb_encoder_update(&e,3,4)==0);
    assert(e.quarters==0);
    assert(bb_encoder_update(&e,0,4)==0); /* Illegal two-bit jump. */
    assert(e.quarters==0);
    e=(bb_encoder_t){3,0};
    assert(bb_encoder_update(&e,1,2)==0);
    assert(bb_encoder_update(&e,0,2)==1);

    bb_panel_button_t b={0};
    assert(bb_panel_button_update(&b,true,100)==0);
    assert(bb_panel_button_update(&b,false,110)==0); /* Bounce. */
    assert(bb_panel_button_update(&b,true,120)==0);
    assert(bb_panel_button_update(&b,true,150)==0);
    assert(bb_panel_button_update(&b,false,200)==0);
    assert(bb_panel_button_update(&b,false,230)==1);
    assert(bb_panel_button_update(&b,false,240)==0);
    assert(bb_panel_button_update(&b,true,300)==0);
    assert(bb_panel_button_update(&b,true,330)==0);
    assert(bb_panel_button_update(&b,true,1130)==2);
    assert(bb_panel_button_update(&b,true,1200)==0);
    assert(bb_panel_button_update(&b,false,1250)==0);
    assert(bb_panel_button_update(&b,false,1280)==0); /* No extra short press. */
    b=(bb_panel_button_t){0};
    assert(bb_panel_button_update(&b,true,UINT32_MAX-20)==0);
    assert(bb_panel_button_update(&b,true,10)==0);
    assert(b.stable); /* Debouncing across timer wrap. */
    puts("controls: quadrature, bounce, hold and timer wrap passed");
}
