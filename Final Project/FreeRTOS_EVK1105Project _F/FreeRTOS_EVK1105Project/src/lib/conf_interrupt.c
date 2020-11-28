/*
 * conf_interrpt.c
 *
 * Created: 9/29/2020 13:25:40
 *  Author: Fernando Zaragoza
 */ 

#include "conf_interrupt.h"

//__attribute__ ((__interrupt__))
void ISR_gpio_qt_70(void)
{
	// UP, DOWN
	if (gpio_get_pin_interrupt_flag(QT1081_TOUCH_SENSOR_UP))
	{
		INTC_QT_FLAG._up = true;
		gpio_clear_pin_interrupt_flag(QT1081_TOUCH_SENSOR_UP);
		
	} else if (gpio_get_pin_interrupt_flag(QT1081_TOUCH_SENSOR_DOWN))
	{
		INTC_QT_FLAG._down = true;
		gpio_clear_pin_interrupt_flag(QT1081_TOUCH_SENSOR_DOWN);
	}
	
}

//__attribute__ ((__interrupt__))
void ISR_gpio_qt_71(void)
{
	// ENTER, LEFT, RIGHT
	if (gpio_get_pin_interrupt_flag(QT1081_TOUCH_SENSOR_ENTER))
	{
		INTC_QT_FLAG._enter = true;
		gpio_clear_pin_interrupt_flag(QT1081_TOUCH_SENSOR_ENTER);
	} else if (gpio_get_pin_interrupt_flag(QT1081_TOUCH_SENSOR_LEFT))
	{
		INTC_QT_FLAG._left = true;
		gpio_clear_pin_interrupt_flag(QT1081_TOUCH_SENSOR_LEFT);
	} else if (gpio_get_pin_interrupt_flag(QT1081_TOUCH_SENSOR_RIGHT))
	{
		INTC_QT_FLAG._right = true;
		gpio_clear_pin_interrupt_flag(QT1081_TOUCH_SENSOR_RIGHT);
	}
}

//__attribute__ ((__interrupt__))
void ISR_eic_qt_33(void)
{
	if (gpio_get_pin_value(QT1081_TOUCH_SENSOR_UP))
	{
		EIC_QT_FLAG._up = true;	
	} else if (gpio_get_pin_value(QT1081_TOUCH_SENSOR_DOWN))
	{
		EIC_QT_FLAG._down = true;
	} else if (gpio_get_pin_value(QT1081_TOUCH_SENSOR_RIGHT))
	{
		EIC_QT_FLAG._right = true;
	} else if (gpio_get_pin_value(QT1081_TOUCH_SENSOR_LEFT))
	{
		EIC_QT_FLAG._left = true;
	} else if (gpio_get_pin_value(QT1081_TOUCH_SENSOR_ENTER))
	{
		EIC_QT_FLAG._enter = true;
	}
	
	//eic_clear_interrupt_line(&AVR32_EIC, QT_EIC_LINE);
}

//__attribute__ ((__interrupt__))
void ISR_tc0_irq_448(void)
{

	// Clear the interrupt flag. This is a side effect of reading the TC SR.
	tc_read_sr(&AVR32_TC, 0);

	// Toggle a GPIO pin
	gpio_tgl_gpio_pin(AVR32_PIN_PB25);
}

//__attribute__ ((__interrupt__))
void ISR_tc1_irq_449(void)
{

	// Clear the interrupt flag. This is a side effect of reading the TC SR.
	tc_read_sr(&AVR32_TC, 1);

	// Toggle a GPIO pin
	gpio_tgl_gpio_pin(AVR32_PIN_PB25);
}

//__attribute__ ((__interrupt__))
void ISR_tc2_irq_450(void)
{

	// Clear the interrupt flag. This is a side effect of reading the TC SR.
	tc_read_sr(&AVR32_TC, 2);

	// Toggle a GPIO pin
	gpio_tgl_gpio_pin(AVR32_PIN_PB25);
}

//ISR(pdca_int_handler, AVR32_PDCA_IRQ_GROUP, 0)
//__attribute__((__interrupt__))
void ISR_pdca0_irq_96(void)
{
	// Disable all interrupts.
	Disable_global_interrupt();

	// Disable interrupt channel.
	pdca_disable_interrupt_transfer_complete(0);
	// Disable unnecessary channel
	pdca_disable(0);
	
	// Enable all interrupts.
	Enable_global_interrupt();
	
	INTC_PDCA_FLAG.ch0 = true;
}

void clear_all_qt_flags(void)
{
	INTC_QT_FLAG._enter = false;
	INTC_QT_FLAG._left	= false;
	INTC_QT_FLAG._right = false;
	INTC_QT_FLAG._up	= false;
	INTC_QT_FLAG._down	= false;
}


