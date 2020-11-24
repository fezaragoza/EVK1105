/*
 * conf_utils.h
 *
 * Created: 9/28/2020 15:16:46
 *  Author: Fernando Zaragoza
 */ 


#ifndef CONF_UTILS_H_
#define CONF_UTILS_H_

#include "evk1105.h"

#define OSCX 0
#define PLLX 0
#define FOSC0_X 12000000
#define FOSC1_X 12000000
#define	STARTUP_OSC0 OSC0_STARTUP
#define STARTUP_OSC1 AVR32_PM_OSCCTRL1_STARTUP_4096_RCOSC // 36 ms. Aprox equiv. time using RCOSC = 115 kHz

#define PBA_HZ FOSC0

#define CPUDIV 1
#define CPUSEL 0
#define HSBDIV CPUDIV
#define HSBSEL CPUSEL
#define PBADIV 1
#define PBASEL 3
#define PBBDIV 1
#define PBBSEL 3

#define A_FUNC 0
#define B_FUNC 1
#define C_FUNC 2




#endif /* CONF_UTILS_H_ */