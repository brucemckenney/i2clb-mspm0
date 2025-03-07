/*
 * leds.h
 *
 *  Created on: Sep 20, 2024
 *      Author: selki
 */

#ifndef LEDS_H_
#define LEDS_H_
#include <ti/devices/msp/msp.h>
#define LEDS_PORT   GPIOA
#define LEDB_PIN    (1u << 13)      // PA13
#define LEDR_PIN    (1u << 26)      // PA26
#define LEDG_PIN    (1u << 27)      // PA27

extern void led_init(void);

#endif /* LEDS_H_ */
