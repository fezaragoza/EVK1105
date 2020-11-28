/*
 * conf_interrpt.h
 *
 * Created: 9/29/2020 13:25:28
 *  Author: Fernando Zaragoza
 */ 


#ifndef CONF_INTERRUPT_H_
#define CONF_INTERRUPT_H_

#include "gpio.h"
//#include "eic.h"
#include "evk1105.h"
#include "tc.h"
#include "pdca.h"

#include "conf_qt.h"
#include "conf_tc.h"

/* FreeRTOS */
/* Scheduler header files. */
//#include "FreeRTOS.h"
//#include "task.h"

#define INTC_QT_FLAG intc_qt
#define EIC_QT_FLAG eic_qt
///
#define INTC_PDCA_FLAG intc_pdca

/* TYPEDEFS */
typedef struct
{
	uint8_t _enter	: 1;
	uint8_t _left	: 1;
	uint8_t _right	: 1;
	uint8_t _up		: 1;
	uint8_t _down	: 1;
} intc_qt_flags_t;

typedef intc_qt_flags_t eic_qt_flags_t;
typedef intc_tc_flags_t intc_pdca_flags_t;

extern intc_qt_flags_t intc_qt;
extern eic_qt_flags_t eic_qt;
///
extern intc_pdca_flags_t intc_pdca;
extern volatile bool end_of_rx_transfer;

__attribute__ ((__interrupt__)) void ISR_eic_qt_33(void);
__attribute__ ((__interrupt__)) void ISR_gpio_qt_70(void);
__attribute__ ((__interrupt__)) void ISR_gpio_qt_71(void);
__attribute__ ((__interrupt__)) void ISR_tc0_irq_448(void);
__attribute__ ((__interrupt__)) void ISR_tc1_irq_449(void);
__attribute__ ((__interrupt__)) void ISR_tc2_irq_450(void);
__attribute__ ((__interrupt__)) void ISR_pdca0_irq_96(void);
extern __attribute__ ((__interrupt__)) void ISR_pdca1_irq_97(void);
extern __attribute__ ((__interrupt__)) void ISR_pdca2_irq_98(void);
void clear_all_qt_flags(void);
void wait(void);

/* FreeRTOS */
//extern TaskHandle_t myIntTaskHandle;

#endif /* CONF_INTERRUPT_H_ */