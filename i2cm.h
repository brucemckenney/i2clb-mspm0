///
//      i2cm.h
//      Rev 0.1.0
//      Copyright 2025 Bruce McKenney
//      BSD 2-clause license
//
#ifndef I2CM_H_
#define I2CM_H_ 1
#include <stdint.h>

#define I2CM_DMA    1

extern void i2cm_init(void);
extern uint32_t i2cm_wregs(uint8_t addr, uint8_t reg, uint32_t cnt, uint8_t *valp);
extern uint32_t i2cm_rregs(uint8_t addr, uint8_t reg, uint32_t cnt, uint8_t *valp);

extern uint32_t i2cm_probe(uint8_t addr);
extern void i2cm_clearSDA(void);

#endif // I2CM_H_
