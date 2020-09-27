/*
 * conf_gpio.h
 *
 * Created: 9/22/2020 15:23:33
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
#define LED3_MASK 0b0001
#define LED2_MASK 0b0010
#define LED1_MASK 0b0100
#define LED0_MASK 0b1000

/* TYPEDEFS */
typedef struct
{
	union
	{
		struct  
		{
			uint8_t led0 : 1;
			uint8_t led1 : 1;
			uint8_t led2 : 1;
			uint8_t led3 : 1;
		} led_s;
		uint8_t ledx;
	};
} evk_led_t;

void config_led_gpio(void);
void set_ledx_num(uint8_t number);
void apply_led_mask(uint8_t number, uint8_t pin, uint8_t mask);

#endif /* CONF_GPIO_H_ */