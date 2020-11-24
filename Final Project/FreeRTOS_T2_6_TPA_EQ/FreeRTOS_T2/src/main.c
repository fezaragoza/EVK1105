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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Environment header files. */
#include "power_clocks_lib.h"
#include "print_funcs.h"
	/** Audio **/
#include "tpa6130.h"
#include "abdac.h"

#include "conf_tpa6130.h"
#include "board.h"
#include "audio.h"

/* Scheduler header files. */
#include "FreeRTOS.h"
#include "task.h"

/* Local includes */
#include "lib/conf_interrupt.h"
#include "lib/conf_tc.h"

#include "lib/sound.h"

/*! \name Priority definitions for most of the tasks in the demo application.
 * Some tasks just use the idle priority.
 */
//! @{
#define mainLED_TASK_PRIORITY     ( tskIDLE_PRIORITY + 1 )
#define mainCOM_TEST_PRIORITY     ( tskIDLE_PRIORITY + 2 )
#define mainQUEUE_POLL_PRIORITY   ( tskIDLE_PRIORITY + 2 )
#define mainSEM_TEST_PRIORITY     ( tskIDLE_PRIORITY + 1 )
#define mainBLOCK_Q_PRIORITY      ( tskIDLE_PRIORITY + 3 )
#define mainCHECK_TASK_PRIORITY   ( tskIDLE_PRIORITY + 4 )
#define mainCREATOR_TASK_PRIORITY ( tskIDLE_PRIORITY + 3 )
//! @}

//! Baud rate used by the serial port tasks.
#define mainCOM_TEST_BAUD_RATE    ( ( unsigned portLONG ) 57600 )

//! LED used by the serial port tasks.  This is toggled on each character Tx,
//! and mainCOM_TEST_LED + 1 is toggled on each character Rx.
#define mainCOM_TEST_LED          ( 3 )

//! LED that is toggled by the check task.  The check task periodically checks
//! that all the other tasks are operating without error.  If no errors are found
//! the LED is toggled.  If an error is found at any time the LED toggles faster.
#define mainCHECK_TASK_LED        ( 6 )

//! LED that is set upon error.
#define mainERROR_LED             ( 7 )

//! The period between executions of the check task.
#define mainCHECK_PERIOD          ( ( TickType_t ) 1000 / portTICK_RATE_MS  )

//! If an error is detected in a task, the vErrorChecks task will enter in an
//! infinite loop flashing the LED at this rate.
#define mainERROR_FLASH_RATE      ( ( TickType_t ) 500 / portTICK_RATE_MS )

/*! \name Constants used by the vMemCheckTask() task.
 */
//! @{
#define mainCOUNT_INITIAL_VALUE   ( ( unsigned portLONG ) 0 )
#define mainNO_TASK               ( 0 )
//! @}

/*! \name The size of the memory blocks allocated by the vMemCheckTask() task.
 */
//! @{
#define mainMEM_CHECK_SIZE_1      ( ( size_t ) 51 )
#define mainMEM_CHECK_SIZE_2      ( ( size_t ) 52 )
#define mainMEM_CHECK_SIZE_3      ( ( size_t ) 15 )
//! @}

/* TPA EXAMPLE */
//! Sample Count Value
#define SOUND_SAMPLES                256
#define FPBA_HZ                 62092800 /**/
#define TPA6130_TWI_MASTER_SPEED  100000

void dac_reload_callback(void);
void dac_overrun_callback(void);
void adc_underrun_callback(void);
void adc_reload_callback(void);

int16_t samples[SOUND_SAMPLES];
uint32_t samples_count;

#define SAMPLE_OFFSET   0x80
#define SAMPLE_RATE     46875
#define SAMPLE_COUNT    (sizeof(sound_data))

static const int8_t sound_data[] =
{
	0x00, 0x04, 0x08, 0x0C, 0x10, 0x14, 0x17, 0x1B, 0x1F, 0x23,
	0x27, 0x2B, 0x2F, 0x32, 0x36, 0x3A, 0x3D, 0x41, 0x44, 0x47,
	0x4B, 0x4E, 0x51, 0x54, 0x57, 0x5A, 0x5D, 0x60, 0x62, 0x65,
	0x67, 0x69, 0x6C, 0x6E, 0x70, 0x72, 0x73, 0x75, 0x77, 0x78,
	0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7E, 0x7F, 0x7F, 0x7F,
	0x7F, 0x7F, 0x7F, 0x7F, 0x7E, 0x7E, 0x7D, 0x7C, 0x7B, 0x7A,
	0x79, 0x78, 0x77, 0x75, 0x73, 0x72, 0x70, 0x6E, 0x6C, 0x69,
	0x67, 0x65, 0x62, 0x60, 0x5D, 0x5A, 0x57, 0x54, 0x51, 0x4E,
	0x4B, 0x47, 0x44, 0x41, 0x3D, 0x3A, 0x36, 0x32, 0x2F, 0x2B,
	0x27, 0x23, 0x1F, 0x1B, 0x17, 0x14, 0x10, 0x0C, 0x08, 0x04,
	0x00, 0xFC, 0xF8, 0xF4, 0xF0, 0xEC, 0xE9, 0xE5, 0xE1, 0xDD,
	0xD9, 0xD5, 0xD1, 0xCE, 0xCA, 0xC6, 0xC3, 0xBF, 0xBC, 0xB9,
	0xB5, 0xB2, 0xAF, 0xAC, 0xA9, 0xA6, 0xA3, 0xA0, 0x9E, 0x9B,
	0x99, 0x97, 0x94, 0x92, 0x90, 0x8E, 0x8D, 0x8B, 0x89, 0x88,
	0x87, 0x86, 0x85, 0x84, 0x83, 0x82, 0x82, 0x81, 0x81, 0x81,
	0x80, 0x81, 0x81, 0x81, 0x82, 0x82, 0x83, 0x84, 0x85, 0x86,
	0x87, 0x88, 0x89, 0x8B, 0x8D, 0x8E, 0x90, 0x92, 0x94, 0x97,
	0x99, 0x9B, 0x9E, 0xA0, 0xA3, 0xA6, 0xA9, 0xAC, 0xAF, 0xB2,
	0xB5, 0xB9, 0xBC, 0xBF, 0xC3, 0xC6, 0xCA, 0xCE, 0xD1, 0xD5,
	0xD9, 0xDD, 0xE1, 0xE5, 0xE9, 0xEC, 0xF0, 0xF4, 0xF8, 0xFC
};

//! Welcome message to display.
#define MSG_WELCOME "\x1B[2J\x1B[H---------- Welcome to TPA6130 example ---------- \r\n"

static void master_callback(uint32_t arg)
{
	if( arg == AUDIO_DAC_OUT_OF_SAMPLE_CB )
	{
		dac_overrun_callback();
	}

	else if( arg == AUDIO_DAC_RELOAD_CB )
	{
		dac_reload_callback();
	}

	else if( arg == AUDIO_ADC_OUT_OF_SAMPLE_CB )
	{
		adc_underrun_callback();;
	}

	else if( arg == AUDIO_ADC_RELOAD_CB )
	{
		adc_reload_callback();;
	}
}

void dac_reload_callback(void)
{
	// Nothing todo
}

void dac_overrun_callback(void)
{
	// Nothing todo
}

void adc_underrun_callback(void)
{
	// Nothing todo
}

void adc_reload_callback(void)
{
	// Nothing todo
}

/* Local Definitions */
#define RC0_VALUE		46875 // 37500 // 100 ms

typedef float float16_t;
typedef double float32_t;

typedef struct {
	float32_t a0;
	float32_t a1;
	float32_t a2;
	
} bandpass_IIR_t;

/* Local Declarations */
// Module's memory address
volatile avr32_tc_t *tc = &AVR32_TC;
volatile avr32_pm_t *pm = &AVR32_PM;

intc_qt_flags_t intc_qt;
intc_tc_flags_t intc_tc;
tc_ch_count_t tick_count;

/* TaskHandles */
TaskHandle_t qtHandle = NULL;
TaskHandle_t myIntTaskHandleTC = NULL;
TaskHandle_t audioHandle = NULL;

void myIntTaskTC0 (void *p);
void myIntTaskTC0 (void *p)
{
	// TC
	// CH0
	init_tc_output_ch0(tc, TC_CHANNEL_0);
	static const tc_interrupt_t tc0_interrupt = {
		.etrgs = 0,
		.ldrbs = 0,
		.ldras = 0,
		.cpcs  = 1, // Enable interrupt on RC compare alone
		.cpbs  = 0,
		.cpas  = 0,
		.lovrs = 0,
		.covfs = 0
	};
	tc_write_rc(tc, TC_CHANNEL_0, RC0_VALUE);
	tc_configure_interrupts(tc, TC_CHANNEL_0, &tc0_interrupt);
	tc_start(tc, TC_CHANNEL_0);
	
	gpio_set_gpio_pin(LED0_GPIO);
	gpio_set_gpio_pin(LED1_GPIO);
	gpio_set_gpio_pin(LED2_GPIO);
	gpio_set_gpio_pin(LED3_GPIO);
	
	while (1)
	{
		vTaskSuspend(NULL);
		gpio_tgl_gpio_pin(LED0_GPIO);
		vTaskSuspend(NULL);
		gpio_tgl_gpio_pin(LED1_GPIO);
		vTaskSuspend(NULL);
		gpio_tgl_gpio_pin(LED2_GPIO);
		vTaskSuspend(NULL);
		gpio_tgl_gpio_pin(LED3_GPIO);
	}
	
}

// audioHandle
portTASK_FUNCTION(playAudioTask, p );
portTASK_FUNCTION(playAudioTask, p )
{
	//print_dbg("Running audio...\r\n");
	static uint32_t count = 0;
	static uint32_t i = 0;
	static portBASE_TYPE notificationValue = 0;
	static bool playAudio = false;
	static bool notify	  = false;

	while(true)
	{
		notificationValue = ulTaskNotifyTake( pdTRUE, (TickType_t) 1 );
		
		if (notificationValue > 0)
		{
			//playAudio = !playAudio;
			notify = !notify;
			print_dbg("Notification received.");

		}
		
		if ((pdca_get_transfer_status(TPA6130_ABDAC_PDCA_CHANNEL) & PDCA_TRANSFER_COMPLETE) && (notify == true))
		{
			playAudio = true;
			//print_dbg("Running audio...\r\n");
		}
		
		if (playAudio)
		{
			playAudio = false;
			count = 0;
			// Store sample from the sound_table array
			while(count < (SOUND_SAMPLES)){
				samples[count++] = ((uint8_t)sound_table[i]+0x80) << 8;
				samples[count++] = ((uint8_t)sound_table[i]+0x80) << 8;
				i++;
				if (i >= sizeof(sound_table)) i = 0;
			}

			gpio_set_gpio_pin(LED0_GPIO);
			gpio_clr_gpio_pin(LED1_GPIO);

			// Play buffer
			tpa6130_dac_output((void *) samples,SOUND_SAMPLES/2);

			gpio_clr_gpio_pin(LED0_GPIO);
			gpio_set_gpio_pin(LED1_GPIO);

			/* Wait until the reload register is empty.
			* This means that one transmission is still ongoing
			* but we are already able to set up the next transmission
			*/
			//while(!tpa6130_dac_output(NULL, 0));
		}
		
	}
}

// qtHandle
portTASK_FUNCTION( qtButtonTask, p );
portTASK_FUNCTION( qtButtonTask, p )
{
	gpio_set_gpio_pin(LED0_GPIO);
	gpio_set_gpio_pin(LED1_GPIO);
	gpio_set_gpio_pin(LED2_GPIO);
	gpio_set_gpio_pin(LED3_GPIO);
	
	while (1)
	{
		vTaskSuspend(NULL); // Suspend itself at start, remain there and wait for an external event to resume it.
		if (INTC_QT_FLAG._left) {
			INTC_QT_FLAG._left = false;
			//gpio_tgl_gpio_pin(LED0_GPIO);
		}
		else if (INTC_QT_FLAG._right) {
			INTC_QT_FLAG._right = false;
			//gpio_tgl_gpio_pin(LED1_GPIO);
		}
		else if (INTC_QT_FLAG._up) {
			INTC_QT_FLAG._up = false;
			//gpio_tgl_gpio_pin(LED2_GPIO);
		}
		else if (INTC_QT_FLAG._down) {
			INTC_QT_FLAG._down = false;
			//gpio_tgl_gpio_pin(LED3_GPIO);
		}
		else if (INTC_QT_FLAG._enter) {
			INTC_QT_FLAG._enter = false;
			xTaskNotifyGive(audioHandle);
		}

	}
}

ISR_FREERTOS(RT_ISR_gpio_qt_70, 70, 0)
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
	
	BaseType_t checkIfYieldRequired = xTaskResumeFromISR(qtHandle);
	return (checkIfYieldRequired ? 1 : 0);
	
	//vTaskNotifyGiveFromISR(playAudioHandle, 0);
	//return 1;
}

ISR_FREERTOS(RT_ISR_gpio_qt_71, 70, 0)
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
	
	BaseType_t checkIfYieldRequired = xTaskResumeFromISR(qtHandle);
	return (checkIfYieldRequired ? 1 : 0);
	
	//vTaskNotifyGiveFromISR(playAudioHandle, 0);
	//return 1;
}

//ISR_FREERTOS(RT_ISR_tc0_irq_448, 448, 1)
//{
//
	//// Clear the interrupt flag. This is a side effect of reading the TC SR.
	//TC_TICK.ch0++;
	//// Clear the interrupt flag. This is a side effect of reading the TC SR.
	//tc_read_sr(&AVR32_TC, TC_CHANNEL_0);
//
	//BaseType_t checkIfYieldRequired = xTaskResumeFromISR(myIntTaskHandleTC);
	//return (checkIfYieldRequired ? 1 : 0);
//}

/*! \brief Initializes the MCU system clocks and TWI-TPA for audio
 */
static void init_sys_clocks(void)
{
  // Switch to OSC0 to speed up the booting
  pm_switch_to_osc0(&AVR32_PM, FOSC0, OSC0_STARTUP);

  // Start oscillator1
  pm_enable_osc1_crystal(&AVR32_PM, FOSC1);
  pm_enable_clk1(&AVR32_PM, OSC1_STARTUP);

  // Set PLL0 (fed from OSC1 = 11.2896 MHz) to 124.1856 MHz
  // We use OSC1 since we need a correct master clock for the SSC module to generate
  // 
  pm_pll_setup(&AVR32_PM, 0,  // pll.
    10,  // mul.
    1,   // div.
    1,   // osc.
    16); // lockcount.

  // Set PLL operating range and divider (fpll = fvco/2)
  // -> PLL0 output = 62.0928 MHz
  pm_pll_set_option(&AVR32_PM, 0, // pll.
    1,  // pll_freq.
    1,  // pll_div2.
    0); // pll_wbwdisable.

  // start PLL0 and wait for the lock
  pm_pll_enable(&AVR32_PM, 0);
  pm_wait_for_pll0_locked(&AVR32_PM);
  // Set all peripheral clocks torun at master clock rate
  pm_cksel(&AVR32_PM,
    0,   // pbadiv.
    0,   // pbasel.
    0,   // pbbdiv.
    0,   // pbbsel.
    0,   // hsbdiv.
    0);  // hsbsel.

  // Set one waitstate for the flash
  flashc_set_wait_state(1);

  // Switch to PLL0 as the master clock
  pm_switch_to_clock(&AVR32_PM, AVR32_PM_MCCTRL_MCSEL_PLL0);

  // Use 12MHz from OSC0 and generate 96 MHz
  pm_pll_setup(&AVR32_PM, 1,  // pll.
    7,   // mul.
    1,   // div.
    0,   // osc.
    16); // lockcount.

  pm_pll_set_option(&AVR32_PM, 1, // pll.
    1,  // pll_freq: choose the range 80-180MHz.
    1,  // pll_div2.
    0); // pll_wbwdisable.

  // start PLL1 and wait forl lock
  pm_pll_enable(&AVR32_PM, 1);

  // Wait for PLL1 locked.
  pm_wait_for_pll1_locked(&AVR32_PM);

}

static void init_twi_tpa(void)
{
	/* TWI */
	const gpio_map_t TPA6130_TWI_GPIO_MAP =
	{
		{TPA6130_TWI_SCL_PIN, TPA6130_TWI_SCL_FUNCTION},
		{TPA6130_TWI_SDA_PIN, TPA6130_TWI_SDA_FUNCTION}
	};

	const twi_options_t TPA6130_TWI_OPTIONS =
	{
		.pba_hz = FPBA_HZ,
		.speed  = TPA6130_TWI_MASTER_SPEED,
		.chip   = TPA6130_TWI_ADDRESS
	};

	// Assign I/Os to SPI.
	gpio_enable_module(TPA6130_TWI_GPIO_MAP,
	sizeof(TPA6130_TWI_GPIO_MAP) / sizeof(TPA6130_TWI_GPIO_MAP[0]));

	// Initialize as master.
	twi_master_init(TPA6130_TWI, &TPA6130_TWI_OPTIONS);
	
	/* TWA */
	tpa6130_init();
	
	tpa6130_dac_start(DEFAULT_DAC_SAMPLE_RATE_HZ,
	DEFAULT_DAC_NUM_CHANNELS,
	DEFAULT_DAC_BITS_PER_SAMPLE,
	DEFAULT_DAC_SWAP_CHANNELS,
	master_callback,
	AUDIO_DAC_OUT_OF_SAMPLE_CB
	| AUDIO_DAC_RELOAD_CB,
	FPBA_HZ); /**/

	tpa6130_set_volume(0x20); // 2F
	tpa6130_get_volume();
	
}

static void init_qt_interrupt(void)
{
	/* INTC */
	Disable_global_interrupt();
	//INTC_init_interrupts();
	// QT
	INTC_register_interrupt(&RT_ISR_gpio_qt_70, 70, AVR32_INTC_INT0);
	INTC_register_interrupt(&RT_ISR_gpio_qt_71, 71, AVR32_INTC_INT0);
	Enable_global_interrupt();
		
	/* GPIO */
	gpio_enable_pin_interrupt(QT1081_TOUCH_SENSOR_UP,    GPIO_RISING_EDGE);
	gpio_enable_pin_interrupt(QT1081_TOUCH_SENSOR_DOWN,  GPIO_RISING_EDGE);
	gpio_enable_pin_interrupt(QT1081_TOUCH_SENSOR_LEFT,  GPIO_RISING_EDGE);
	gpio_enable_pin_interrupt(QT1081_TOUCH_SENSOR_RIGHT, GPIO_RISING_EDGE);
	gpio_enable_pin_interrupt(QT1081_TOUCH_SENSOR_ENTER, GPIO_RISING_EDGE);
		
	memset(&INTC_QT_FLAG, 0, sizeof(INTC_QT_FLAG));
}

int main (void)
{
	/* Insert system clock initialization code here (sysclk_init()). */
	init_sys_clocks();
	
	/* Initialize RS232 debug text output. */
	init_dbg_rs232(FPBA_HZ); /**/
	
	init_twi_tpa();
	
	init_qt_interrupt();
	
	// Enable LED0 and LED1
	gpio_enable_gpio_pin(LED0_GPIO);
	gpio_enable_gpio_pin(LED1_GPIO);
	
	print_dbg(MSG_WELCOME);
	
	/* Insert application code here, after the board has been initialized. */
	
	//uint16_t pass = 25;
	//xTaskCreate(myTask1, "taks1", 256, (void *)pass, mainLED_TASK_PRIORITY, &myTask1Handle);
	xTaskCreate(qtButtonTask,  "tQT",        256, (void *) 0, mainCOM_TEST_PRIORITY, &qtHandle);
	xTaskCreate(playAudioTask, "tPlayAudio", 256, (void *) 0, mainLED_TASK_PRIORITY, &audioHandle);
	
	vTaskStartScheduler();
	
	while (1)
	{

	}
	
	return 0;
	
}

/*-----------------------------------------------------------*/
