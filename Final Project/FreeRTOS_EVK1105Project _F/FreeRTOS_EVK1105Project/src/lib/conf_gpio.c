/*
 * conf_gpio.c
 *
 * Created: 9/29/2020 12:24:28
 *  Author: Fernando Zaragoza
 */ 


/* INCLUDES */
#include "conf_gpio.h"

void config_led_gpio(void)
{
	gpio_configure_pin(LED0_GPIO, GPIO_DIR_OUTPUT | GPIO_INIT_HIGH);
	gpio_configure_pin(LED1_GPIO, GPIO_DIR_OUTPUT | GPIO_INIT_HIGH);
	gpio_configure_pin(LED2_GPIO, GPIO_DIR_OUTPUT | GPIO_INIT_HIGH);
	gpio_configure_pin(LED3_GPIO, GPIO_DIR_OUTPUT | GPIO_INIT_HIGH);
}

void set_ledx_num(uint8_t number)
{
	apply_led_mask(number, LED0_GPIO, LED0_MASK);
	apply_led_mask(number, LED1_GPIO, LED1_MASK);
	apply_led_mask(number, LED2_GPIO, LED2_MASK);
	apply_led_mask(number, LED3_GPIO, LED3_MASK);
}

void apply_led_mask(uint8_t number, uint8_t pin, uint8_t mask)
{
	if (number & mask)
		gpio_set_pin_low(pin); // Turn LED ON
	else
		gpio_set_pin_high(pin); // Turn LED OFF
}