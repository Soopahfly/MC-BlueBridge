#include "panel_controls.h"

int bb_encoder_update(bb_encoder_t* encoder, uint8_t pins, unsigned steps) {
    static const int8_t transitions[16] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};
    pins &= 3;
    uint8_t previous = encoder->previous;
    encoder->previous = pins;
    if ((previous ^ pins) == 3) { encoder->quarters = 0; return 0; }
    encoder->quarters += transitions[(previous << 2) | pins];
    if (encoder->quarters >= (int)steps) { encoder->quarters = 0; return 1; }
    if (encoder->quarters <= -(int)steps) { encoder->quarters = 0; return -1; }
    return 0;
}

int bb_panel_button_update(bb_panel_button_t* button, bool down, uint32_t now) {
    if (down != button->observed) { button->observed = down; button->changed_at = now; }
    if (down != button->stable && (uint32_t)(now - button->changed_at) >= 30u) {
        button->stable = down;
        if (down) { button->pressed_at = now; button->held = false; }
        else if (!button->held) return 1;
    }
    if (button->stable && !button->held && (uint32_t)(now - button->pressed_at) >= 800u) {
        button->held = true;
        return 2;
    }
    return 0;
}
