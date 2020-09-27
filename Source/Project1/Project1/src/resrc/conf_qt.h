/*
 * conf_qt.h
 *
 * Created: 9/22/2020 21:37:31
 * Author: Fernando Zaragoza
 * Description: QT TOUCH SENSOR Library with Utils and Functions.
 */ 



#ifndef CONF_QT_H_
#define CONF_QT_H_

/* INCLUDES */
#include "gpio.h"
#include "evk1105.h"

/* DEFINES */
#define INIT_QT(_pin)	{ .pin = _pin, .past_state = 1, .current_state = 1,  .active = 0 }
#define INIT_QT_ENTER	{ .pin = QT1081_TOUCH_SENSOR_ENTER, .past_state = 0, .current_state = 0,  .active = 0 }
#define INIT_QT_LEFT	{ QT1081_TOUCH_SENSOR_LEFT, 0, 0, 0 }
#define INIT_QT_RIGHT	{ QT1081_TOUCH_SENSOR_RIGHT, 0, 0, 0 }
#define INIT_QT_UP		{ .pin = QT1081_TOUCH_SENSOR_UP, .past_state = 0, .current_state = 0,  .active = 0 }
#define INIT_QT_DOWN	{ QT1081_TOUCH_SENSOR_DOWN, 0, 0, 0 }

#define QT_PRESSED  0
#define QT_RELEASED 1

/* TYPEDEFS */
typedef struct
{
	uint8_t pin;
	bool past_state;
	bool current_state;
	_Bool active;
} button_t;

typedef struct
{
	union {
		struct
		{
			button_t _enter; // 0
			button_t _left;  // 1
			button_t _right; // 2
			button_t _up;	 // 3
			button_t _down;	 // 4
		} button_s;
		button_t button_a[5];
	};
} qt_sensor_t;

/* FUNCTION PROTOTYPES */
qt_sensor_t* config_qt_gpio(void);
void poll_qt_button(button_t* button, bool edge);
bool qt_get_activation(bool current, bool past, bool edge);


#endif /* CONF_QT_H_ */