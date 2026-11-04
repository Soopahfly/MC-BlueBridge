#pragma once
#include <stdint.h>
typedef struct { uint32_t enable,tar,clr_stop_det,clr_tx_abrt,raw_intr_stat,status,data_cmd; } i2c_hw_t;
typedef struct { i2c_hw_t hw; } i2c_inst_t;
extern i2c_inst_t test_i2c[2];
#define i2c0 (&test_i2c[0])
#define i2c1 (&test_i2c[1])
static inline i2c_hw_t* i2c_get_hw(i2c_inst_t* p) { return &p->hw; }
static inline void i2c_init(i2c_inst_t* p,unsigned rate) { (void)p; (void)rate; }
