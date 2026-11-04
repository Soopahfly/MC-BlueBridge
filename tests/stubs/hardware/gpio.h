#pragma once
#include <stdbool.h>
#include <stdint.h>
#define GPIO_FUNC_I2C 3
#define GPIO_IN 0
#define GPIO_IRQ_EDGE_RISE 8
#define GPIO_IRQ_EDGE_FALL 4
extern bool test_gpio[30];
static inline bool gpio_get(unsigned pin) { return test_gpio[pin]; }
static inline void gpio_init(unsigned pin) { (void)pin; }
static inline void gpio_set_dir(unsigned pin, unsigned dir) { (void)pin; (void)dir; }
static inline void gpio_set_irq_enabled_with_callback(unsigned pin,unsigned events,bool enabled,void (*fn)(unsigned,uint32_t)) {
    (void)pin;(void)events;(void)enabled;(void)fn;
}
static inline void gpio_set_irq_enabled(unsigned pin,unsigned events,bool enabled) { (void)pin;(void)events;(void)enabled; }
static inline void gpio_set_function(unsigned pin,unsigned function) { (void)pin; (void)function; }
static inline void gpio_pull_up(unsigned pin) { (void)pin; }
