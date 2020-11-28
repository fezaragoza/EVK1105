/*
 * conf_gpio.h
 *
 * Created: 9/29/2020 12:24:17
 *  Author: Fernando Zaragoza
 */ 


#ifndef CONF_GPIO_H_
#define CONF_GPIO_H_

/* INCLUDES */
#include "gpio.h"
#include "evk1105.h"

/* DEFINES */
#define LED(x)	LED##x##_GPIO
#define LOOP_LED(x)	for (size_t i = 0; i < x; i++)

#define LED0_MASK (1 << (7))// 0b10000000
#define LED1_MASK (1 << (6)) // 0b01000000
#define LED2_MASK 0b00100000 // (1 << 5)
#define LED3_MASK 0b00010000 // (1 << 4)
// Must correspond to evk_led_t

#define LED4_GPIO AVR32_PIN_PB30
#define LED5_GPIO AVR32_PIN_PB31

/* TYPEDEFS */
typedef struct
{
	union
	{
		struct  
		{ // Big endian
			//uint8_t led7 : 1;
			//uint8_t led6 : 1;
			//uint8_t led5 : 1;
			//uint8_t led4 : 1;
			uint8_t led0 : 1;
			uint8_t led1 : 1;
			uint8_t led2 : 1;
			uint8_t led3 : 1; 
		} led_s;
		uint8_t led;
	};
} evk_led_t;

void config_led_gpio(void);
void set_ledx_num(uint8_t number);
void apply_led_mask(uint8_t number, uint8_t pin, uint8_t mask);

#endif /* CONF_GPIO_H_ */