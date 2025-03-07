///
//      timer.c
//
#include <ti/devices/msp/msp.h>
#include "clk.h"
#include "timer.h"
#define TIMT        TIMG1
#define TIMT_ISR    TIMG1_IRQHandler
#define TIMT_IRQn   TIMG1_INT_IRQn
volatile uint32_t timer_ms;

void
TIMT_ISR(void)
{
    uint32_t mis;
    mis = TIMT->CPU_INT.MIS;
    TIMT->CPU_INT.ICLR = mis;               // Clear 'em all
    if (mis & GPTIMER_CPU_INT_MIS_Z_SET)
    {
        ++timer_ms;
    }
    return;
}

void
timer_init(void)
{
    //  1kHz SW timer
    TIMT->GPRCM.PWREN = (GPTIMER_PWREN_KEY_UNLOCK_W | GPTIMER_PWREN_ENABLE_ENABLE);
    //  CTR: Start at 0, repeat, count up, Advance per CCCTL_01[0].ACOND
    TIMT->COUNTERREGS.CTRCTL = GPTIMER_CTRCTL_CAC_CCCTL0_ACOND|GPTIMER_CTRCTL_CVAE_ZEROVAL|
                                GPTIMER_CTRCTL_REPEAT_REPEAT_1|GPTIMER_CTRCTL_CM_UP;
    TIMT->COUNTERREGS.LOAD = (HZ/1000)-1;     // 1kHz period

    //  CCR0: advance on clock, compare mode
    TIMT->COUNTERREGS.CCCTL_01[0] = GPTIMER_CCCTL_01_ACOND_TIMCLK|GPTIMER_CCCTL_01_COC_COMPARE;
    TIMT->COUNTERREGS.CC_01[0] = (HZ/1000)/4; // 25% duty

    //  Enable clock, then timer
    TIMT->CLKSEL = GPTIMER_CLKSEL_BUSCLK_SEL_ENABLE;
    TIMT->COMMONREGS.CCLKCTL = GPTIMER_CCLKCTL_CLKEN_ENABLED;
    TIMT->COUNTERREGS.CTRCTL |= GPTIMER_CTRCTL_EN_ENABLED;

    TIMT->CPU_INT.IMASK |= GPTIMER_CPU_INT_IMASK_Z_SET;
    NVIC_ClearPendingIRQ(TIMT_IRQn);
    NVIC_EnableIRQ(TIMT_IRQn);
    return;
}

void
timer_wait(uint32_t ms)
{
    uint32_t start = timer_ms;

    while (timer_ms - start < ms)
    {
        __WFI();
    }
    return;
}
