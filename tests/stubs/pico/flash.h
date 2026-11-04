#pragma once
#include "pico/stdlib.h"
static inline int flash_safe_execute(void (*fn)(void*), void* context, uint32_t timeout) {
    (void)timeout; fn(context); return PICO_OK;
}
