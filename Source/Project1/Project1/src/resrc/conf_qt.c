/*
 * config_qt.c
 *
 * Created: 9/22/2020 21:37:03
 *  Author: Fernando Zaragoza
 */ 

#include "conf_qt.h"

/* QT_TOUCH_SENSOR UTILS */

qt_sensor_t* config_qt_gpio(void)
{
	gpio_configure_pin(QT1081_TOUCH_SENSOR_ENTER, GPIO_DIR_INPUT);
	gpio_configure_pin(QT1081_TOUCH_SENSOR_LEFT,  GPIO_DIR_INPUT);
	gpio_configure_pin(QT1081_TOUCH_SENSOR_RIGHT, GPIO_DIR_INPUT);
	gpio_configure_pin(QT1081_TOUCH_SENSOR_UP,	  GPIO_DIR_INPUT);
	gpio_configure_pin(QT1081_TOUCH_SENSOR_DOWN,  GPIO_DIR_INPUT);
	/* gpio_configure_pin 
	*	PUER - Pull up resistor enable reg. Flag: GPIO_PULL_UP to set puers. (When input)
	*	IMR0/1 - Interrupt mode reg. Flag: GPIO_INTERRUPT, depends on GPIO_BOTHEDGES, GPIO_RISING, GPIO_FALLING to set/clear imr.
	*	ODER - Output driver enable reg. Flag: GPIO_OUTPUT or GPIO_INPUT, sets pin to be output or input. Sets oder
	*	OVR - Output value reg. Flag: GPIO_INIT_HIGH or GPIO_INIT_LOW, sets/clears pin in ovr.
	*	GPER - Enable GPIO. set/clear gper.
	*	
	*	Flags can be found in gpio.h
	*/
	qt_sensor_t *qt = malloc(sizeof(*qt)); // Or sizeof(qt_sensor)
	*qt = (qt_sensor_t){ 
		{ // anonymous union
		.button_s = {._enter = INIT_QT_ENTER, ._left = INIT_QT_LEFT, ._right = INIT_QT_RIGHT,\
						._up = INIT_QT_UP, ._down = INIT_QT_DOWN }
		}
	};
	
	return qt;
}

void poll_qt_button(button_t* button, bool edge)
{
	button->current_state = gpio_get_pin_value(button->pin);
	//if (button->current_state == 1)
	//{
		////delay
	if (button->current_state == gpio_get_pin_value(button->pin))
	{
		button->active = qt_get_activation(button->current_state,\
							button->past_state, edge);
	}
	//}
	button->past_state = button->current_state;
}

bool qt_get_activation(bool current, bool past, bool edge)
{
	bool activation = false;
	switch(edge)
	{
		default:
		case QT_PRESSED:
			if ((current == true) && (past == false))
			activation = true;
			break;
		
		case QT_RELEASED:
			if ((current == false) && (past == true))
			activation = true;
			break;
	}
	return activation;
}

/* END QT_TOUCH_SENSOR UTILS */