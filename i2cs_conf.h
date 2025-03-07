///
//  i2cs_conf.h
//
#ifndef I2CS_CONF_H_
#define I2CS_CONF_H_    1

///
//  Config for L1306: I2C0 on PA0/1
//
#define I2CS_HZ       400000UL        // Is this needed?
#define I2CS          I2C0
#define I2CS_ISR      I2C0_IRQHandler
#define I2CS_IRQn     I2C0_INT_IRQn
#define I2CS_PORT     GPIOA
#define I2CSSDA_PIN   (1u << 0)        // PA0 as I2C0_SDA
#define I2CSPOCI_PIN  (1u << 1)        // PA1 as I2C0_SCL
#define I2CSSDA_MUX   IOMUX_PINCM1
#define I2CSSCL_MUX   IOMUX_PINCM2
#define I2CSSDA_PF    3                // Per SLASEX0D Table 6-1
#define I2CSSCL_PF    3

#endif // I2CS_CONF_H_ */
