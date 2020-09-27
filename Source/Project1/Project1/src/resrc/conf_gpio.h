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

/* DEFINES */
#define LED(x)	LED##x##_GPIO
#define LOOP_LED(x)	for (size_t i = 0; i < x; i++)

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
void set_ledx_num(uint8_t led);

void config_led_gpio(void)
{
	gpio_configure_pin(LED0_GPIO, GPIO_DIR_OUTPUT | GPIO_INIT_HIGH);
	gpio_configure_pin(LED1_GPIO, GPIO_DIR_OUTPUT | GPIO_INIT_HIGH);
	gpio_configure_pin(LED2_GPIO, GPIO_DIR_OUTPUT | GPIO_INIT_HIGH);
	gpio_configure_pin(LED3_GPIO, GPIO_DIR_OUTPUT | GPIO_INIT_HIGH);
}

void set_ledx_num(uint8_t led)
{
	switch (led)
	{
	case 0: // 0b0000
		gpio_set_pin_high(LED0_GPIO);
		gpio_set_pin_high(LED1_GPIO);
		gpio_set_pin_high(LED2_GPIO);
		gpio_set_pin_high(LED3_GPIO);
		break;
	case 1: // 0b0001
		gpio_set_pin_high(LED0_GPIO);
		gpio_set_pin_high(LED1_GPIO);
		gpio_set_pin_high(LED2_GPIO);
		gpio_set_pin_low(LED3_GPIO);
		break;
	case 2: // 0b0010
		gpio_set_pin_high(LED0_GPIO);
		gpio_set_pin_high(LED1_GPIO);
		gpio_set_pin_low(LED2_GPIO);
		gpio_set_pin_high(LED3_GPIO);
		break;
	case 3: // 0b0011
		gpio_set_pin_high(LED0_GPIO);
		gpio_set_pin_high(LED1_GPIO);
		gpio_set_pin_low(LED2_GPIO);
		gpio_set_pin_low(LED3_GPIO);
		break;
	case 4: // 0b0100
		gpio_set_pin_high(LED0_GPIO);
		gpio_set_pin_low(LED1_GPIO);
		gpio_set_pin_high(LED2_GPIO);
		gpio_set_pin_high(LED3_GPIO);
		break;
	case 5: // 0b0101
		gpio_set_pin_high(LED0_GPIO);
		gpio_set_pin_low(LED1_GPIO);
		gpio_set_pin_high(LED2_GPIO);
		gpio_set_pin_low(LED3_GPIO);
		break;
	case 6: // 0b0110
		gpio_set_pin_high(LED0_GPIO);
		gpio_set_pin_low(LED1_GPIO);
		gpio_set_pin_low(LED2_GPIO);
		gpio_set_pin_high(LED3_GPIO);
		break;
	case 7: // 0b0111
		gpio_set_pin_high(LED0_GPIO);
		gpio_set_pin_low(LED1_GPIO);
		gpio_set_pin_low(LED2_GPIO);
		gpio_set_pin_low(LED3_GPIO);
		break;
	default:
		gpio_set_pin_low(LED0_GPIO);
		gpio_set_pin_low(LED1_GPIO);
		gpio_set_pin_low(LED2_GPIO);
		gpio_set_pin_low(LED3_GPIO);
		break;
	}
	
}


#endif /* CONF_GPIO_H_ */