#include <ti/devices/msp/msp.h>
#include <stdio.h>      // snprintf()
#include "clk.h"
#include "timer.h"
#include "leds.h"
#include "i2cm.h"
#include "i2cs.h"

#define IOMUX_PF_GPIO   1u          // You'd think someone would define this

void
gpio_powerup(void)
{
    //  Power up all the GPIOs
    GPIOA->GPRCM.PWREN = (GPIO_PWREN_KEY_UNLOCK_W | GPIO_PWREN_ENABLE_ENABLE);
#if defined(GPIOB)
    GPIOB->GPRCM.PWREN = (GPIO_PWREN_KEY_UNLOCK_W | GPIO_PWREN_ENABLE_ENABLE);
#endif // GPIOB
    return;
}

void
_delay_cycles(uint32_t count)
{
    volatile uint32_t n = count / 8;    // 8 turns out to be a not-bad guess
    while (n-- > 0) /*EMPTY*/;
    return;
}


uint8_t out[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
uint8_t in[sizeof(out)];
uint32_t target_ok;

int
main(void)
{
    SYSCTL->SOCLOCK.MCLKCFG |= SYSCTL_MCLKCFG_USEMFTICK_ENABLE; // Put this in clk_init someday
    gpio_powerup();
    led_init();
    timer_init();
    timer_wait(3); // for LFOSC
    i2cm_init();
    i2cs_init();
    target_ok = i2cm_probe(I2CS_ADDR);
    while (1)
    {
        LEDS_PORT->DOUTTGL31_0 = (LEDB_PIN|LEDR_PIN|LEDG_PIN);
        //i2cm_clearSDA();
        i2cm_wregs(I2CS_ADDR, 2, sizeof(out), out);
        //timer_wait(3);          // Shot in the dark
        i2cm_rregs(I2CS_ADDR, 2, sizeof(in),  in);
        out[0] += 1;
        timer_wait(500);        // Slow down so we can see
    }
    return 0;
}
