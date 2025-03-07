/*
 * leds.c
 *
 *  Created on: Sep 20, 2024
 *      Author: selki
 */
#include <ti/devices/msp/msp.h>
#include "leds.h"
#define IOMUX_PF_GPIO   1u          // You'd think someone would define this

//  IOMUX per SLASEX0D Table 6-1
#define LEDB_MUX    (IOMUX_PINCM14) // PA13=IOMUX14
#define LEDR_MUX    (IOMUX_PINCM27) // PA26=IOMUX27
#define LEDG_MUX    (IOMUX_PINCM28) // PA27=IOMUX28
void
led_init(void)
{
    //  IOMUX for the RGB LEDs. PF=1 is GPIO
    IOMUX->SECCFG.PINCM[LEDB_MUX] = (IOMUX_PINCM_PC_CONNECTED | IOMUX_PF_GPIO); // PB22: IOMUX 50 as GPIO
    IOMUX->SECCFG.PINCM[LEDR_MUX] = (IOMUX_PINCM_PC_CONNECTED | IOMUX_PF_GPIO); // PB26: IOMUX 57 as GPIO
    IOMUX->SECCFG.PINCM[LEDG_MUX] = (IOMUX_PINCM_PC_CONNECTED | IOMUX_PF_GPIO); // PB27: IOMUX 58 as GPIO

    //  All off initially
    LEDS_PORT->DOUTCLR31_0 = (LEDB_PIN|LEDR_PIN|LEDG_PIN);
    LEDS_PORT->DOESET31_0  = (LEDB_PIN|LEDR_PIN|LEDG_PIN);
    return;
}
