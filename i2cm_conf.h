///
//  i2cm_conf.h
//
#ifndef I2CM_CONF_H_
#define I2CM_CONF_H_    1
//
//  For L1306: I2C1 on PA16/15

#define I2CM_HZ       100000UL         // 100kHz max with MFCLK
#define I2CM          I2C1
#define I2CM_ISR      I2C1_IRQHandler
#define I2CM_IRQn     I2C1_INT_IRQn
#define I2CM_PORT     GPIOA
#define I2CMSDA_PIN   (1u << 16)       // PA16 as I2C1_SDA
#define I2CMSCL_PIN   (1u << 15)       // PA15 as I2C1_SCL
#define I2CMSDA_MUX   IOMUX_PINCM17
#define I2CMSCL_MUX   IOMUX_PINCM16
#define I2CMSDA_PF    3                // Per SLASEX0B Table 6-1
#define I2CMSCL_PF    3

#endif // I2CM_CONF_H_ */
