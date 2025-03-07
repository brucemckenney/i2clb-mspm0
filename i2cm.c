///
//      i2cm.c
//
#include <ti/devices/msp/msp.h>
#include "i2cm.h"
#include "i2cm_conf.h"
#include "clk.h"

#define SINGLE_OP   1       // Use the Read On Tx Empty option for reads
#define I2CM_DMA    1
#if I2CM_DMA
#define I2CM_DMACHAN    0   // Channel 0 (I just picked one)
#define I2CM_DMATRIG    6   // I2C1 Publisher 1 (DMA_TRIG1) per SLASEX0D Table 8-2
#endif

//  Some shorthand
#define I2CM_TXDONE() (I2CM->CPU_INT.RIS & I2C_CPU_INT_RIS_MTXDONE_SET)
#define I2CM_RXDONE() (I2CM->CPU_INT.RIS & I2C_CPU_INT_RIS_MRXDONE_SET)
#define I2CM_RXFIFOEMPTY() ((I2CM->MASTER.MFIFOSR & I2C_MFIFOSR_RXFIFOCNT_MASK) == 0)  // != 0 as a truth value
#define I2CM_TXFIFOFULL() ((I2CM->MASTER.MFIFOSR & I2C_MFIFOSR_TXFIFOCNT_MASK) == I2C_MFIFOSR_TXFIFOCNT_MAXIMUM)   // != 0 as a truth value

///
//  i2cm_txflush()
//  Shorthand to flush the Tx FIFO
//
void
i2cm_txflush(void)
{
    I2CM->MASTER.MFIFOCTL |= I2C_MFIFOCTL_TXFLUSH_FLUSH;
    while ( (I2CM->MASTER.MFIFOSR & I2C_MFIFOSR_TXFIFOCNT_MASK) != I2C_MFIFOSR_TXFIFOCNT_MAXIMUM)
        { /*EMPTY*/;}
    I2CM->MASTER.MFIFOCTL &= ~I2C_MFIFOCTL_TXFLUSH_FLUSH;
    return;
}

///
//  i2cm_waitbusy()
//
void
i2cm_waitbusy(void)
{
    //while ((I2CM->CPU_INT.RIS & I2C_CPU_INT_RIS_MSTOP_SET) == 0) {/*EMPTY*/;}
    //while (I2CM->MASTER.MSR & I2C_MSR_BUSY_SET) {/*EMPTY*/;}
    while ((I2CM->MASTER.MSR & I2C_MSR_IDLE_SET) == 0) {/*EMPTY*/;}
    return;
}

///
//  i2cm_clearSDA()
//
extern void _delay_cycles(uint32_t count); // in main() somewhere
#define SCL_CLK_CYCLES  80u
void
i2cm_clearSDA(void)
{
#define IOMUX_PF_GPIO   1u          // You'd think someone would define this

    uint32_t saved_scl_iomux;
    uint32_t i;

    //  Connect SCL to GPIO
    saved_scl_iomux = IOMUX->SECCFG.PINCM[I2CMSCL_MUX];
    IOMUX->SECCFG.PINCM[I2CMSCL_MUX] = (IOMUX_PINCM_PC_CONNECTED | IOMUX_PINCM_INENA_ENABLE |
                                        IOMUX_PINCM_HIZ1_ENABLE | IOMUX_PF_GPIO); // PA15:IOMUX 16 as GPIO
    I2CM_PORT->DOESET31_0 = I2CMSCL_PIN;    // I don't know if we need this

    //  9 clock pulses
    for (i = 0 ; i < (1+8) ; ++i)
    {
        I2CM_PORT->DOUTCLR31_0 = I2CMSCL_PIN;
        while ((I2CM_PORT->DIN31_0 & I2CMSCL_PIN) != 0) {/*EMPTY*/;}    // Wait for it to go low
        _delay_cycles(SCL_CLK_CYCLES);
        I2CM_PORT->DOUTSET31_0 = I2CMSCL_PIN;
        while ((I2CM_PORT->DIN31_0 & I2CMSCL_PIN) == 0) {/*EMPTY*/;}    // Wait for it to go high
        _delay_cycles(SCL_CLK_CYCLES);
    }

    IOMUX->SECCFG.PINCM[I2CMSCL_MUX] = saved_scl_iomux;
    return;
}

///
//  i2cm_init()
//
void
i2cm_init(void)
{
    I2CM->GPRCM.PWREN = (I2C_PWREN_KEY_UNLOCK_W | I2C_PWREN_ENABLE_ENABLE); // Switch it on

    IOMUX->SECCFG.PINCM[I2CMSDA_MUX] = (IOMUX_PINCM_PC_CONNECTED | IOMUX_PINCM_INENA_ENABLE | IOMUX_PINCM_PIPU_ENABLE |
                                        IOMUX_PINCM_HIZ1_ENABLE | I2CMSDA_PF); // PA16:IOMUX 17 as I2C1_SDA
    IOMUX->SECCFG.PINCM[I2CMSCL_MUX] = (IOMUX_PINCM_PC_CONNECTED | IOMUX_PINCM_INENA_ENABLE | IOMUX_PINCM_PIPU_ENABLE |
                                        IOMUX_PINCM_HIZ1_ENABLE | I2CMSCL_PF); // PA15:IOMUX 16 as I2C1_SCL

    //  SCL=I2CM_HZ
#define MFCLK_HZ    4000000UL           // SLAU847D Sec 2.3.2.4
    I2CM->CLKSEL = I2C_CLKSEL_MFCLK_SEL_ENABLE; // We suppose someone already set USEMFTICK
    I2CM->CLKDIV = 0;       // MFCLK /1
    I2CM->MASTER.MTPR = ((MFCLK_HZ/1)/(10*I2CM_HZ))-1;    // Formula from SLAU847D Sec 21.2.1.1

    //  Enable clock stretch, active
    I2CM->MASTER.MCR = I2C_MCR_CLKSTRETCH_ENABLE | I2C_MCR_ACTIVE_ENABLE;
#if I2CM_DMA
    //  Reserve our DMA channel
    DMA->DMATRIG[I2CM_DMACHAN].DMATCTL = DMA_DMATCTL_DMATINT_EXTERNAL | (I2CM_DMATRIG << DMA_DMATCTL_DMATSEL_OFS);
#endif // I2CM_DMA

    return;
}

///
//  i2cm_wregs()
//
uint32_t
i2cm_wregs(uint8_t addr, uint8_t reg, uint32_t cnt, uint8_t *valp)
{
    uint32_t i;
    uint32_t wcnt;

    I2CM->CPU_INT.ICLR = I2C_CPU_INT_ICLR_MTXDONE_CLR | I2C_CPU_INT_ICLR_MNACK_CLR | // Clear stale
                            I2C_CPU_INT_ICLR_MSTOP_CLR;         // For debug

    I2CM->MASTER.MTXDATA = reg;         // First byte
    I2CM->MASTER.MSA = (addr << I2C_MSA_SADDR_OFS) | I2C_MSA_DIR_TRANSMIT;
    I2CM->MASTER.MCTR = I2C_MCTR_START_ENABLE | I2C_MCTR_STOP_ENABLE |
                        (0*I2C_MCTR_ACK_ENABLE) |
                        I2C_MCTR_BURSTRUN_ENABLE | ((cnt + 1) << I2C_MCTR_MBLEN_OFS);

#if I2CM_DMA
    I2CM->DMA_TRIG1.ICLR  = I2C_DMA_TRIG1_ICLR_MTXFIFOTRG_CLR;  // Clear stale
    I2CM->DMA_TRIG1.IMASK = I2C_DMA_TRIG1_IMASK_MTXFIFOTRG_SET; // TXFIFO trigger
    DMA->DMACHAN[I2CM_DMACHAN].DMASA = (uint32_t)valp;
    DMA->DMACHAN[I2CM_DMACHAN].DMADA = (uint32_t)&I2CM->MASTER.MTXDATA;
    DMA->DMACHAN[I2CM_DMACHAN].DMASZ = cnt;
    DMA->DMACHAN[I2CM_DMACHAN].DMACTL = DMA_DMACTL_DMASRCWDTH_BYTE | DMA_DMACTL_DMADSTWDTH_WORD |
                                        DMA_DMACTL_DMASRCINCR_INCREMENT | DMA_DMACTL_DMADSTINCR_UNCHANGED |
                                        DMA_DMACTL_DMATM_SINGLE | DMA_DMACTL_DMAEN_ENABLE;
#else // I2CM_DMA
    //  Start filling the FIFO while the register number is being sent
    i = 0;
    while (cnt > 0)
    {
        if (I2CM->MASTER.MSR & I2C_MSR_ERR_SET)
            break;                      // We won't progress now
        while (cnt > 0 && !I2CM_TXFIFOFULL())
        {
            I2CM->MASTER.MTXDATA = valp[i];
            ++i;
            --cnt;
        }
    }
#endif // I2CM_DMA
    while (!I2CM_TXDONE()) {/*EMPTY*/;}
    i2cm_waitbusy();
#if I2CM_DMA
    I2CM->DMA_TRIG1.IMASK = 0;          // Avoid stray triggers
#endif // I2CM_DMA

    wcnt = cnt - ((I2CM->MASTER.MSR & I2C_MSR_MBCNT_MASK) >> I2C_MSR_MBCNT_OFS);
    return(wcnt);
}

///
//  i2cm_rregs()
//
uint32_t
i2cm_rregs(uint8_t addr, uint8_t reg, uint32_t cnt, uint8_t *valp)
{
    uint32_t i;
    uint32_t rcnt;

    I2CM->CPU_INT.ICLR = I2C_CPU_INT_ICLR_MTXDONE_CLR | I2C_CPU_INT_ICLR_MRXDONE_CLR | // Clear stale
                         I2C_CPU_INT_ICLR_MNACK_CLR   | I2C_CPU_INT_ICLR_MSTOP_CLR; // These for debug
    I2CM->MASTER.MTXDATA = reg;             // Register number is first (only) Tx byte
#if !SINGLE_OP      // Sends setup-write with a Stop
    I2CM->MASTER.MSA = (addr << I2C_MSA_SADDR_OFS) | I2C_MSA_DIR_TRANSMIT;
    I2CM->MASTER.MCTR = I2C_MCTR_START_ENABLE | (1*I2C_MCTR_STOP_ENABLE) | I2C_MCTR_ACK_ENABLE |
                        I2C_MCTR_BURSTRUN_ENABLE | (( 1u) << I2C_MCTR_MBLEN_OFS);
    while ((I2CM->CPU_INT.RIS & I2C_CPU_INT_RIS_MTXDONE_SET) == 0) {/*EMPTY*/;}
    i2cm_waitbusy();
#endif

    //  Read back data. If SINGLE_OP=1 then send the Tx byte(s) first, followed by Repeated Start.
    I2CM->MASTER.MSA = (addr << I2C_MSA_SADDR_OFS) | I2C_MSA_DIR_RECEIVE;
    I2CM->MASTER.MCTR = I2C_MCTR_START_ENABLE | I2C_MCTR_STOP_ENABLE | (0*I2C_MCTR_ACK_ENABLE) |
                        (SINGLE_OP * I2C_MCTR_RD_ON_TXEMPTY_ENABLE) |
                        I2C_MCTR_BURSTRUN_ENABLE | ((cnt + 0) << I2C_MCTR_MBLEN_OFS);
#if I2CM_DMA
    I2CM->DMA_TRIG1.ICLR  = I2C_DMA_TRIG1_ICLR_MRXFIFOTRG_CLR;  // Clear stale
    I2CM->DMA_TRIG1.IMASK = I2C_DMA_TRIG1_IMASK_MRXFIFOTRG_SET; // RXFIFO trigger
    DMA->DMACHAN[I2CM_DMACHAN].DMASA = (uint32_t)&I2CM->MASTER.MRXDATA;
    DMA->DMACHAN[I2CM_DMACHAN].DMADA = (uint32_t)valp;
    DMA->DMACHAN[I2CM_DMACHAN].DMASZ = cnt;
    DMA->DMACHAN[I2CM_DMACHAN].DMACTL = DMA_DMACTL_DMADSTWDTH_BYTE | DMA_DMACTL_DMASRCWDTH_WORD |
                                        DMA_DMACTL_DMADSTINCR_INCREMENT | DMA_DMACTL_DMASRCINCR_UNCHANGED |
                                        DMA_DMACTL_DMATM_SINGLE | DMA_DMACTL_DMAEN_ENABLE; // Set up and start
    while (!I2CM_RXDONE())
    {
        if (I2CM->MASTER.MSR & I2C_MSR_ERR_SET)
            break;                      // We won't progress now
    }
    I2CM->DMA_TRIG1.IMASK = 0;          // Avoid stray triggers
#else // I2CM_DMA
    while (!I2CM_RXDONE())              // Siphon the Rx data from the FIFO
    {
        if (I2CM->MASTER.MSR & I2C_MSR_ERR_SET)
            break;                      // We won't progress now
        while (!I2CM_RXFIFOEMPTY())
        {
            uint8_t c;
            c = I2CM->MASTER.MRXDATA;
            if (cnt > 0)
            {
                valp[i] = c;
                ++i;
                --cnt;
            }
        }
    }
#endif // I2CM_DMA
    i2cm_waitbusy();

    //  Empty any dregs
    while (!I2CM_RXFIFOEMPTY())
    {
        uint8_t c;
        c = I2CM->MASTER.MRXDATA;
        if (cnt > 0)
        {
            valp[i] = c;
            ++i;
            --cnt;
        }
    }
    rcnt = cnt - ((I2CM->MASTER.MSR & I2C_MSR_MBCNT_MASK) >> I2C_MSR_MBCNT_OFS);
    return(rcnt);
}

uint32_t
i2cm_probe(uint8_t addr)
{
    uint32_t ok = 1;
    I2CM->MASTER.MSA = (addr << I2C_MSA_SADDR_OFS) | I2C_MSA_DIR_TRANSMIT;
    I2CM->MASTER.MCTR = I2C_MCTR_START_ENABLE | I2C_MCTR_STOP_ENABLE | (0*I2C_MCTR_ACK_ENABLE) |
                        (0 * I2C_MCTR_RD_ON_TXEMPTY_ENABLE) |
                        I2C_MCTR_BURSTRUN_ENABLE | (0u << I2C_MCTR_MBLEN_OFS);
    while (!I2CM_TXDONE())
    {
        if (I2CM->MASTER.MSR & I2C_MSR_ERR_SET)
            break;                      // We won't progress now
    }
    if (I2CM->MASTER.MSR & I2C_MSR_ERR_SET)
        ok = 0;
    i2cm_waitbusy();

    return(ok);
}
