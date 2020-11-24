/*
 * conf_tc.h
 *
 * Created: 24/10/2020 20:20:13
 *  Author: Fernando Zaragoza
 */ 


#ifndef CONF_TC_H_
#define CONF_TC_H_

#include <tc.h>
#include "compiler.h"

#define TC_CHANNEL_0	0
#define TC_CHANNEL_1 	1
#define TC_CHANNEL_2 	2
#define TC_TICK			tick_count
#define INTC_TC_FLAG	intc_tc

/* TYPEDEFS */
typedef struct
{
	uint16_t ch0;
	uint16_t ch1;
	uint16_t ch2;
} tc_ch_count_t;

typedef struct
{
	uint8_t ch0 : 1;
	uint8_t ch1 : 1;
	uint8_t ch2 : 1;
} intc_tc_flags_t;


extern tc_ch_count_t tick_count;
extern intc_tc_flags_t intc_tc;

void init_tc_output_ch0(volatile avr32_tc_t *tc, unsigned int channel);
void init_tc_output_ch1(volatile avr32_tc_t *tc, unsigned int channel);
void init_tc_output_ch2(volatile avr32_tc_t *tc, unsigned int channel);
__attribute__((__interrupt__)) void tc1_irq(void);
__attribute__((__interrupt__)) void tc2_irq(void);


#endif /* CONF_TC_H_ */