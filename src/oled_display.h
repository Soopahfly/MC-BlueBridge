#pragma once
#include <stdbool.h>
#include <stdint.h>

void bb_oled_init(void);
void bb_oled_poll(uint32_t now);
bool bb_oled_available(void);
/* Eight text rows, at most 21 ASCII characters each. No transfer unless changed. */
void bb_oled_text(const char rows[8][22]);
