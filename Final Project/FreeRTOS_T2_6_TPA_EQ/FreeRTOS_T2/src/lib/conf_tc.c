/*
 * conf_tc.c
 *
 * Created: 24/10/2020 20:20:25
 *  Author: Fernando Zaragoza
 */ 

#include "conf_tc.h"

void init_tc_output_ch0(volatile avr32_tc_t *tc, unsigned int channel)
{
	// Options for waveform generation.
	tc_waveform_opt_t waveform_opt =
	{
		.channel  = channel,    	              // Channel selection.
		
		.bswtrg = TC_EVT_EFFECT_NOOP,       	// Software trigger effect on TIOB.
		.beevt	= TC_EVT_EFFECT_NOOP,       	// External event effect on TIOB.
		.bcpc 	= TC_EVT_EFFECT_NOOP,       	// RC compare effect on TIOB.
		.bcpb 	= TC_EVT_EFFECT_NOOP,       	// RB compare effect on TIOB.
		
		.aswtrg = TC_EVT_EFFECT_NOOP,       	// Software trigger effect on TIOA.
		.aeevt	= TC_EVT_EFFECT_NOOP,      	// External event effect on TIOA.
		.acpc 	= TC_EVT_EFFECT_NOOP,      	// RC compare effect on TIOA.
		.acpa 	= TC_EVT_EFFECT_NOOP,      	// RA compare effect on TIOA.
		
		.wavsel   = TC_WAVEFORM_SEL_UP_MODE_RC_TRIGGER,// Waveform selection: Up mode with automatic trigger on RC compare.
		.enetrg   = false,                                                                               	// External event trigger enable.
		.eevt 	  = TC_EXT_EVENT_SEL_TIOB_INPUT,                       	// External event selection.
		.eevtedg  = TC_SEL_NO_EDGE,                  	// External event edge selection.
		.cpcdis   = false,               	                            	// Counter disable when RC compare.
		.cpcstop  = false,                                              	// Counter clock stopped with RC compare.
		
		.burst	= TC_BURST_NOT_GATED,       	// Burst signal selection.
		.clki 	= TC_CLOCK_RISING_EDGE,     	// Clock inversion. - 0
		.tcclks = TC_CLOCK_SOURCE_TC4       	// Internal source clock 4, connected to fPBA / 32
	};
	// Initialize the timer/counter waveform.
	tc_init_waveform(tc, &waveform_opt);
}

void init_tc_output_ch1(volatile avr32_tc_t *tc, unsigned int channel)
{
	// Options for waveform generation.
	tc_waveform_opt_t waveform_opt =
	{
		.channel  = channel,    	              // Channel selection.
		
		.bswtrg = TC_EVT_EFFECT_NOOP,       	// Software trigger effect on TIOB.
		.beevt	= TC_EVT_EFFECT_NOOP,       	// External event effect on TIOB.
		.bcpc 	= TC_EVT_EFFECT_NOOP,       	// RC compare effect on TIOB.
		.bcpb 	= TC_EVT_EFFECT_NOOP,       	// RB compare effect on TIOB.
		
		.aswtrg = TC_EVT_EFFECT_NOOP,       	// Software trigger effect on TIOA.
		.aeevt	= TC_EVT_EFFECT_NOOP,      	// External event effect on TIOA.
		.acpc 	= TC_EVT_EFFECT_NOOP,      	// RC compare effect on TIOA.
		.acpa 	= TC_EVT_EFFECT_NOOP,      	// RA compare effect on TIOA.
		
		.wavsel   = TC_WAVEFORM_SEL_UP_MODE_RC_TRIGGER,// Waveform selection: Up mode with automatic trigger on RC compare.
		.enetrg   = false,                                                                               	// External event trigger enable.
		.eevt 	  = TC_EXT_EVENT_SEL_TIOB_INPUT,                       	// External event selection.
		.eevtedg  = TC_SEL_NO_EDGE,                  	// External event edge selection.
		.cpcdis   = false,               	                            	// Counter disable when RC compare.
		.cpcstop  = false,                                              	// Counter clock stopped with RC compare.
		
		.burst	= TC_BURST_NOT_GATED,       	// Burst signal selection.
		.clki 	= TC_CLOCK_RISING_EDGE,     	// Clock inversion. - 0
		.tcclks = TC_CLOCK_SOURCE_TC4       	// Internal source clock 3, connected to fPBA / 32
	};
	// Initialize the timer/counter waveform.
	tc_init_waveform(tc, &waveform_opt);
}

void init_tc_output_ch2(volatile avr32_tc_t *tc, unsigned int channel)
{
	// Options for waveform generation.
	tc_waveform_opt_t waveform_opt =
	{
		.channel  = channel,    	              // Channel selection.
		
		.bswtrg = TC_EVT_EFFECT_NOOP,       	// Software trigger effect on TIOB.
		.beevt	= TC_EVT_EFFECT_NOOP,       	// External event effect on TIOB.
		.bcpc 	= TC_EVT_EFFECT_NOOP,       	// RC compare effect on TIOB.
		.bcpb 	= TC_EVT_EFFECT_NOOP,       	// RB compare effect on TIOB.
		
		.aswtrg = TC_EVT_EFFECT_NOOP,       	// Software trigger effect on TIOA.
		.aeevt	= TC_EVT_EFFECT_NOOP,      	// External event effect on TIOA.
		.acpc 	= TC_EVT_EFFECT_NOOP,      	// RC compare effect on TIOA.
		.acpa 	= TC_EVT_EFFECT_NOOP,      	// RA compare effect on TIOA.
		
		.wavsel   = TC_WAVEFORM_SEL_UP_MODE_RC_TRIGGER,// Waveform selection: Up mode with automatic trigger on RC compare.
		.enetrg   = false,                                                                               	// External event trigger enable.
		.eevt 	  = TC_EXT_EVENT_SEL_TIOB_INPUT,                       	// External event selection.
		.eevtedg  = TC_SEL_NO_EDGE,                  	// External event edge selection.
		.cpcdis   = false,               	                            	// Counter disable when RC compare.
		.cpcstop  = false,                                              	// Counter clock stopped with RC compare.
		
		.burst	= TC_BURST_NOT_GATED,       	// Burst signal selection.
		.clki 	= TC_CLOCK_RISING_EDGE,     	// Clock inversion. - 0
		.tcclks = TC_CLOCK_SOURCE_TC4       	// Internal source clock 3, connected to fPBA / 32
	};
	// Initialize the timer/counter waveform.
	tc_init_waveform(tc, &waveform_opt);
}

//__attribute__((__interrupt__))
void tc1_irq(void)
{
	TC_TICK.ch1++;
	// Clear the interrupt flag. This is a side effect of reading the TC SR.
	tc_read_sr(&AVR32_TC, TC_CHANNEL_1);

}

//__attribute__((__interrupt__))
void tc2_irq(void)
{
	TC_TICK.ch2++;
	// Clear the interrupt flag. This is a side effect of reading the TC SR.
	tc_read_sr(&AVR32_TC, TC_CHANNEL_2);

}