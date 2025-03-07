///
//      i2cs.c
//
#include <ti/devices/msp/msp.h>
#include "i2cs.h"
#include "i2cs_conf.h"

//  Register array
uint8_t i2cs_regs[I2CS_REGCNT];
uint32_t i2cs_regi;

// Byte count for current transaction. Its values are {0, >0 }.
uint32_t i2cs_tcnt;

#if I2CS_TRACE
//  ISR trace table
#define I2CS_LTRACESZ   8
uint32_t i2cs_ltrace[I2CS_LTRACESZ];
uint32_t i2cs_ltracei;
#endif // I2CS_TRACE


///
//  i2cs_txflush()
//  Shorthand to flush the Tx FIFO
//
void
i2cs_txflush(void)
{
    I2CS->SLAVE.SFIFOCTL |= I2C_SFIFOCTL_TXFLUSH_FLUSH;
    while ( (I2CS->SLAVE.SFIFOSR & I2C_SFIFOSR_TXFIFOCNT_MASK) != I2C_SFIFOSR_TXFIFOCNT_MAXIMUM)
        { /*EMPTY*/;}
    I2CS->SLAVE.SFIFOCTL &= ~I2C_SFIFOCTL_TXFLUSH_FLUSH;
    return;
}

///
//  i2cs_init()
//
void
i2cs_init(void)
{
    I2CS->GPRCM.PWREN = (I2C_PWREN_KEY_UNLOCK_W | I2C_PWREN_ENABLE_ENABLE);

    //  IOMUX: HIZ1 doesn't matter for PA0/1 but does for others.
    IOMUX->SECCFG.PINCM[I2CSSDA_MUX] = (IOMUX_PINCM_PC_CONNECTED | IOMUX_PINCM_INENA_ENABLE | IOMUX_PINCM_PIPU_ENABLE |
                                        IOMUX_PINCM_HIZ1_ENABLE | I2CSSDA_PF); // PA0:IOMUX 1 as I2C0_SDA
    IOMUX->SECCFG.PINCM[I2CSSCL_MUX] = (IOMUX_PINCM_PC_CONNECTED | IOMUX_PINCM_INENA_ENABLE | IOMUX_PINCM_PIPU_ENABLE |
                                        IOMUX_PINCM_HIZ1_ENABLE | I2CSSCL_PF); // PA1:IOMUX 2 as I2C0_SCL

    //  Evidently a Slave does need a clock of some sort.
    I2CS->CLKSEL = I2C_CLKSEL_MFCLK_SEL_ENABLE; // MFCLK/1 with CLKDIV=0
#if 0
    I2CS->CLKDIV = 8-1; // 128us with RXTIMEOUT=64-1. SCR doesn't seem to enter into this
#endif

    // Trigger when Tx goes empty or Rx goes up to 1.
    I2CS->SLAVE.SFIFOCTL = (0 << I2C_SFIFOCTL_TXTRIG_OFS) | (0u << I2C_SFIFOCTL_RXTRIG_OFS);

    // Enable clock stretching, but not SWU (I2C_ERR_04). Trigger Tx FIFO Empty only when it's needed (TREQ).
    I2CS->SLAVE.SCTR = I2C_SCTR_TXWAIT_STALE_TXFIFO_ENABLE|I2C_SCTR_TXEMPTY_ON_TREQ_ENABLE|
                        I2C_SCTR_SCLKSTRETCH_ENABLE|I2C_SCTR_SWUEN_DISABLE|
                        I2C_SCTR_ACTIVE_ENABLE;
    I2CS->SLAVE.SOAR = (I2CS_ADDR << I2C_SOAR_OAR_OFS) | I2C_SOAR_OAREN_ENABLE; // Our address

#if POISON_TXFIFO
    while ( (I2CS->SLAVE.SFIFOSR & I2C_SFIFOSR_TXFIFOCNT_MASK) != 0)
        I2CS->SLAVE.STXDATA = 0xFF;
    i2cs_txflush();
#endif // POISON_TXFIFO

    I2CS->CPU_INT.ICLR  = I2C_CPU_INT_ICLR_STXEMPTY_CLR  | I2C_CPU_INT_ICLR_SRXFIFOTRG_CLR  | I2C_CPU_INT_ICLR_SSTART_CLR|I2C_CPU_INT_ICLR_STXFIFOTRG_CLR;
    I2CS->CPU_INT.IMASK = I2C_CPU_INT_IMASK_STXEMPTY_SET | I2C_CPU_INT_IMASK_SRXFIFOTRG_SET | I2C_CPU_INT_IMASK_SSTART_SET;
    NVIC_ClearPendingIRQ(I2CS_IRQn);
    NVIC_EnableIRQ(I2CS_IRQn);

    return;
}   // end i2cs_init()

///
//  I2CS_ISR()
//
void
I2CS_ISR(void)
{
    uint32_t mis;
    uint32_t i;
    mis = I2CS->CPU_INT.MIS;
    I2CS->CPU_INT.ICLR = mis;           // Clear 'em all

#if I2CS_TRACE
    i = i2cs_ltracei;
    i2cs_ltrace[i] = mis;
    if (++i >= I2CS_LTRACESZ)
        i = 0;
    i2cs_ltracei = i;
#endif // I2CS_TRACE

    //  Check for Start first, to keep ordering straight
    if (mis & I2C_CPU_INT_MIS_SSTART_SET)
    {
        i2cs_tcnt = 0;                  // No bytes (yet) in transaction
    }

    //  if STXEMPTY, the master has done a read
    if (mis & I2C_CPU_INT_MIS_STXEMPTY_SET)
    {
        //  Get rid of any stale stuff
        if (I2CS->SLAVE.SSR & I2C_SSR_STALE_TXFIFO_SET)
        {
            i2cs_txflush();
        }

        //  Feed out register contents
        i = i2cs_regi;                  // register pointer register
        I2CS->SLAVE.STXDATA = i2cs_regs[i]; // Return register
        if (++i >= I2CS_REGCNT)         // Bump pointer
            i = 0;                      //  cyclically
        i2cs_regi = i;
        ++i2cs_tcnt;                    // I suppose
    }

    //  if SRXFIFOTRG, the master has done a write
    if (mis & I2C_CPU_INT_MIS_SRXFIFOTRG_SET)   // Rx threshold
    {
        //  We need to always empty the Rx FIFO (cross the threshold),
        //   or we'll never see another RX int
        i = i2cs_regi;
        while ((I2CS->SLAVE.SFIFOSR & I2C_SFIFOSR_RXFIFOCNT_MASK) != 0) // until empty
        {
            uint8_t c;
            c = I2CS->SLAVE.SRXDATA;
            if (i2cs_tcnt == 0)                 // First byte (of transaction)?
            {
                if (c < I2CS_REGCNT)            // Set as register pointer
                {
                    i = c;                      // Eventually reaches i2cs_regi
                }   // else maybe we should NACK?
            }
            else                                // Write to registers
            {
                i2cs_regs[i] = c;               // Set register
                if (++i >= I2CS_REGCNT)         // Move pointer
                {
                    i = 0;                      //  cyclically
                }
            }
            ++i2cs_tcnt;                        // One more byte in this transaction
        }
        i2cs_regi = i;                          // Pointer register
    }

    return;
}   // end I2CS_ISR()
