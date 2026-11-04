#pragma once
#include <stdbool.h>
#include <stdint.h>

void bb_oled_init(void);
void bb_oled_init_off(void);
void bb_oled_poll(uint32_t now);
bool bb_oled_available(void);
/* Eight text rows, at most 21 ASCII characters each. No transfer unless changed. */
void bb_oled_text(const char rows[8][22]);
/* Full-screen three-word comic burst; frame 0-3 reveals top, left, right. */
void bb_oled_pew(uint8_t frame);
/* Graphics occupy rows 0-6; the last row is a short profile/status footer. */
uint8_t* bb_oled_graphics_begin(void);
void bb_oled_graphics_footer(const char* text);
