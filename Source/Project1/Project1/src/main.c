/**
 * \file
 *
 * \brief Empty user application template
 *
 */

/**
 * \mainpage User Application template doxygen documentation
 *
 * \par Empty user application template
 *
 * Bare minimum empty user application template
 *
 * \par Content
 *
 * -# Include the ASF header files (through asf.h)
 * -# "Insert system clock initialization code here" comment
 * -# Minimal main function that starts with a call to board_init()
 * -# "Insert application code here" comment
 *
 */

/*
 * Include header files for all drivers that have been imported from
 * Atmel Software Framework (ASF).
 */
/*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
 */
#include <asf.h>
#include <string.h>
#include "resrc/conf_gpio.h"
#include "resrc/conf_qt.h"

#define NUMBER 2

typedef enum{
	ENTER_NUM = 0,
	OR_AND
} state_machine_t;

int main (void)
{

	/* Insert system clock initialization code here (sysclk_init()). */

	// board_init();
	
	/* Insert application code here, after the board has been initialized. */
	
	// Exercise A)
	uint8_t count = 0, position = 0;
	
	// Configure LEDS and QT
	config_led_gpio();
	qt_sensor_t *qt = config_qt_gpio();
	
	evk_led_t evkl[NUMBER];
	memset(&evkl, 0, sizeof(evkl));
	
	state_machine_t state = ENTER_NUM;
	
	// Poll
	while (1)
	{
		static uint8_t num = 0; // Cannot have var declaration in the middle of cases. To workaround, put stuff inside case in a new scope {}
		switch(state)
		{
			case ENTER_NUM:
				poll_qt_button(&qt->button_s._left, QT_PRESSED);
				poll_qt_button(&qt->button_s._right, QT_PRESSED);
				poll_qt_button(&qt->button_s._enter, QT_PRESSED);
				if (qt->button_s._left.active)
				{
					if (position < 4)
					{
						position++;
						num = evkl[count].ledx;
						num |= 1 << (position - 1);
						// Update LEDs
						set_ledx_num(num);
					}
					
				}
				else if (qt->button_s._right.active)
				{
					num = evkl[count].ledx;

					if (position > 0)
					{
						--position;
						if ((position - 1) >= 0)
							num |= 1 << (position - 1);	
					}
						
				
					//if ((--position) != 0)
					//{
					//num |= 1 << position;
					//}
					//else
					//{
					//num = evkl[count].ledx;
					//}
				
					// Update LEDs
					set_ledx_num(num);
				}
			
				if (qt->button_s._enter.active)
				{
					if (position > 0)
					{
						// Entered a number, store it.
						evkl[count].ledx = num;
						// Reset position to 0
						position = 0;
						// Update LEDs
						set_ledx_num(evkl[count].ledx);
					}
					else
					{
						// Ready for the current number
						num = 0;
						position = 0;
						//Update LEDS
						set_ledx_num(num);
						if (++count == NUMBER)
						{
							// Ready
							// Change state
							state = OR_AND;
						}
					}
				}
				break;
			case OR_AND:
				poll_qt_button(&qt->button_s._up, QT_PRESSED);
				poll_qt_button(&qt->button_s._down, QT_PRESSED);
				poll_qt_button(&qt->button_s._enter, QT_PRESSED);
				if (qt->button_s._down.active)
				{
					num = evkl[0].ledx | evkl[1].ledx;
					set_ledx_num(num);
				}
				else if (qt->button_s._up.active)
				{
					num = evkl[0].ledx & evkl[1].ledx;
					set_ledx_num(num);
				}
				
				if (qt->button_s._enter.active)
				{
					// LED routine
					state = ENTER_NUM;
					num = 0;
				}
				break;
			default:
				// Nothing to do in default case.
				break;
		}
	}
		
	//for (size_t i = 0; i < 1000; i += 4) {
	//if (gpio_get_pin_value(QT1081_TOUCH_SENSOR_LEFT) == false) {
	//++position;
	//}
	//else if (gpio_get_pin_value(QT1081_TOUCH_SENSOR_) == false) {
	//gpio_set_gpio_pin(GPIO_PIN_EXAMPLE_2);
	//}
	//}
}
