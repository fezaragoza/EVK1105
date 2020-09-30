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
// Ex. A
#define LED3_MASK 0b0001
#define LED2_MASK 0b0010
#define LED1_MASK 0b0100
#define LED0_MASK 0b1000
// Ex. B
#define LED2_MASK_B 0b001
#define LED1_MASK_B 0b010
#define LED0_MASK_B 0b100
// Ex. C
#define LED2_MASK_C 0b001
#define LED1_MASK_C 0b010
#define LED0_MASK_C 0b100

// EX D
#define LED4_GPIO AVR32_PIN_PB30
#define LED5_GPIO AVR32_PIN_PB31
#define LED5_MASK_D 0b000001
#define LED4_MASK_D 0b000010
#define LED3_MASK_D 0b000100
#define LED2_MASK_D 0b001000
#define LED1_MASK_D 0b010000
#define LED0_MASK_D 0b100000

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
void set_ledx_num_b(uint8_t number); // Ex. B
void set_ledx_num_c(uint8_t number); // Ex. C
void set_ledx_num_d(uint8_t number); // Ex. D
void apply_led_mask(uint8_t number, uint8_t pin, uint8_t mask);

#endif /* CONF_GPIO_H_ */