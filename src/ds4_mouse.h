#pragma once
#include <stdbool.h>
#include <stdint.h>
/* 0 legacy left-click only, 1 two-finger press, 2 right-half press,
 * 3 selected canonical controller button reserved for right-click. */
typedef struct __attribute__((packed)) {
    uint8_t right_click,right_button,scroll,scroll_step,invert_scroll;
} bb_ds4_mouse_t;
static inline bb_ds4_mouse_t bb_ds4_mouse_defaults(void){return (bb_ds4_mouse_t){0,5,0,64,0};}
static inline bool bb_ds4_mouse_valid(bb_ds4_mouse_t c){return c.right_click<=3&&c.right_button<=13&&c.scroll<=1&&c.scroll_step>=16&&c.invert_scroll<=1;}
