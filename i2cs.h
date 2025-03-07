///
//      i2cs.h
//      Rev 0.1.0
//      Copyright 2025 Bruce McKenney
//      BSD 2-clause license
//
#ifndef I2CS_H_
#define I2CS_H_  1

#define I2CS_ADDR   0x48        // Slave address (7-bit)
#define I2CS_REGCNT 32          // Number of registers defined

extern void i2cs_init(void);

#endif // I2CS_H_
