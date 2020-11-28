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
#include <fastmath.h>

/* Environment header files. */
#include "power_clocks_lib.h"
#include "et024006dhu.h"

	/** Audio **/
#include "tpa6130.h"
#include "abdac.h"
#include "conf_tpa6130.h"
#include "board.h"
#include "audio.h"
#include "sdramc.h"

/* Scheduler header files. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Local includes */
#include "lib/conf_utils.h"
#include "lib/conf_interrupt.h"
#include "lib/conf_tc.h"
#include "lib/conf_spi_sd.h"
//#include "lib/sound.h"
#include "img/image.h"
#include "lib/letdown.h"

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

/*************** TPA ***************/
//! Sample Count Value
#define SOUND_SAMPLES                256
#define TPA6130_TWI_MASTER_SPEED  100000

void dac_reload_callback(void);
void dac_overrun_callback(void);
void adc_underrun_callback(void);
void adc_reload_callback(void);

int16_t samples[SOUND_SAMPLES];
uint32_t samples_count;

//! Welcome message to display.
#define MSG_WELCOME "\x1B[2J\x1B[H---------- Welcome to Final Project ---------- \r\n"

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

/***************    FAT   *****************/
unsigned int Filessize[10];
static char str_buff[MAX_FILE_PATH_LENGTH];
static char filenames[4][MAX_FILE_PATH_LENGTH];

static bool first_ls;
typedef struct
{
	uint8_t   lun;
	char      drive_name;
	uint8_t   devices_available; // Same value as lun.
	uint8_t   drive_number;
	uint8_t   number_of_files;
	FS_STRING name_of_files[10];
}sd_fat_data_t;
static sd_fat_data_t sd;

/**************   SDRAM   ***************/
#define LED_SDRAM_WRITE     LED0
#define LED_SDRAM_READ      LED1
#define LED_SDRAM_ERRORS    (LED0 | LED1 | LED2 | LED3)
#define LED_SDRAM_OK        (LED0 | LED1 | LED2 | LED3)

#define MASK_B0(x) (uint8_t)( ((0xFF << 24) & x) >> 24 )
#define MASK_B1(x) (uint8_t)( ((0xFF << 16) & x) >> 16 )
#define MASK_B2(x) (uint8_t)( ((0xFF << 8)  & x) >> 8 )
#define MASK_B3(x) (uint8_t)( ((0xFF << 0)  & x) >> 0 )

typedef struct
{
	union
	{
		struct
		{
			uint32_t b0 : 8; // msb
			uint32_t b1 : 8;
			uint32_t b2 : 8;
			uint32_t b3 : 8; // lsb
		};
		uint8_t byte[4]; // 0 - msb
		unsigned long word;
	};
} sdram_udata_t;

//volatile unsigned long *sdram = SDRAM;
unsigned long sdram_last = 0;
/*************** PERSONAL ***************/
/* Local Definitions */
#define RC0_VALUE		46875 // 37500 // 100 ms

/* Typedefs */
typedef float float16_t;
typedef double float32_t;

typedef enum
{
	MAIN,
	REPRODUCIR,
	LYRICS,
	EQ
}state_t;

typedef struct
{
	uint8_t lr;
	uint8_t ud;
}menu_keys_t;

/* Local Declarations */
// Module's memory address
volatile avr32_tc_t *tc = &AVR32_TC;
volatile avr32_pm_t *pm = &AVR32_PM;

intc_qt_flags_t intc_qt;
intc_tc_flags_t intc_tc;
state_t state = MAIN;
/* Prototype */
static void init_sdram(void);
static void init_fs(void);
static void rep_menu(bool);
static void menu_gui(bool);
static unsigned long a2ul(const char*);
static uint8_t x2u8(const char*);
/*************** FREERTOS ***************/

/* TaskHandles */
TaskHandle_t myIntTaskHandleTC = NULL;
TaskHandle_t qtHandle = NULL;
TaskHandle_t audioHandle = NULL;
TaskHandle_t fsHandle = NULL;
TaskHandle_t etHandle = NULL;
TaskHandle_t sdramHandle = NULL;

/* QueueHandles */
QueueHandle_t forwardQueue;
QueueHandle_t reverseQueue;
QueueHandle_t repLrQueue;
QueueHandle_t repUdQueue;
QueueHandle_t mainLrQueue;
QueueHandle_t mainUdQueue;
QueueHandle_t sdramQueue;

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
portTASK_FUNCTION_PROTO(playAudioTask, p );
portTASK_FUNCTION(playAudioTask, p )
{
	//print_dbg("Running audio...\r\n");
	static uint32_t count = 0;
	static uint32_t i = 0;
	static portBASE_TYPE notificationValue = 0;
	static bool playAudio = false;
	static bool notify	  = false;
	static uint16_t samplesRx;

	while(true)
	{
		notificationValue = ulTaskNotifyTake( pdTRUE, (TickType_t) 1 );

		if (notificationValue > 0)
		{
			//playAudio = !playAudio;
			notify = !notify;
			//print_dbg("Notification received.");

		}

		if ((pdca_get_transfer_status(TPA6130_ABDAC_PDCA_CHANNEL) & PDCA_TRANSFER_COMPLETE) && (notify == true))
		{
			playAudio = true;
			//print_dbg("Running audio...\r\n");

			if (forwardQueue != 0)
			{
				if (xQueueReceive( forwardQueue, &samplesRx, (TickType_t) 2 ))
				{
					i = ( (i + samplesRx) <= sizeof(letdownsong) ) ? (i + samplesRx) : i;
				}
			}

			//if (reverseQueue != 0)
			//{
				//if (xQueueReceive( reverseQueue, &samplesRx, (TickType_t) 5 ))
				//{
					//i = (i > samplesRx) ? (i - samplesRx) : i;
				//}
			//}
		}

		if (playAudio)
		{
			playAudio = false;
			count = 0;
			// Store sample from the sound_table array
			while(count < (SOUND_SAMPLES)){
				samples[count++] = ((uint8_t)letdownsong[i]+0x80) << 8; 
				samples[count++] = ((uint8_t)letdownsong[i]+0x80) << 8;
				i++;
				if (i >= sizeof(letdownsong)) i = 0;
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
portTASK_FUNCTION_PROTO( qtButtonTask, p );
portTASK_FUNCTION( qtButtonTask, p )
{
	gpio_set_gpio_pin(LED0_GPIO);
	gpio_set_gpio_pin(LED1_GPIO);
	gpio_set_gpio_pin(LED2_GPIO);
	gpio_set_gpio_pin(LED3_GPIO);

	static uint16_t samplesToMove;
	static uint8_t lrValue = 0;
	static uint8_t udValue = 0;
	forwardQueue = xQueueCreate( 1 , sizeof(uint16_t));
	reverseQueue = xQueueCreate( 1 , sizeof(uint16_t));
	repUdQueue = xQueueCreate( 1 , sizeof(uint8_t));
	repLrQueue = xQueueCreate( 1 , sizeof(uint8_t));
	mainUdQueue = xQueueCreate( 1 , sizeof(uint8_t));
	mainLrQueue = xQueueCreate( 1 , sizeof(uint8_t));

	while (1)
	{
		vTaskSuspend(NULL); // Suspend itself at start, remain there and wait for an external event to resume it.
		if (INTC_QT_FLAG._left) {
			INTC_QT_FLAG._left = false;
			//gpio_tgl_gpio_pin(LED0_GPIO);
			if (state == REPRODUCIR)
			{		
				lrValue = (lrValue > 0) ? lrValue - 1 : lrValue;
				xQueueSend( repLrQueue, &lrValue, (TickType_t) 0);
				vTaskResume(etHandle);
			}
			else if(state == MAIN)
			{
				lrValue = (lrValue > 0) ? lrValue - 1 : lrValue;
				xQueueSend( mainLrQueue, &lrValue, (TickType_t) 0);
				vTaskResume(etHandle);
			}
			else
			{
				samplesToMove = 4096;
				xQueueSend( reverseQueue, &samplesToMove, (TickType_t) 0 );
			}
			
		}
		else if (INTC_QT_FLAG._right) {
			INTC_QT_FLAG._right = false;
			//gpio_tgl_gpio_pin(LED1_GPIO);
			if (state == REPRODUCIR)
			{
				lrValue = (lrValue + 1 <= 2) ? lrValue + 1 : lrValue;
				xQueueSend( repLrQueue, &lrValue, (TickType_t) 0);
				vTaskResume(etHandle);
			}
			else if(state == MAIN)
			{
				lrValue = (lrValue + 1 <= 2) ? lrValue + 1 : lrValue;
				xQueueSend( mainLrQueue, &lrValue, (TickType_t) 0);
				vTaskResume(etHandle);
			}
			else
			{
				samplesToMove = 4096;
				xQueueSend( forwardQueue, &samplesToMove, (TickType_t) 0 );
			}
		}
		else if (INTC_QT_FLAG._up) {
			INTC_QT_FLAG._up = false;
			//gpio_tgl_gpio_pin(LED2_GPIO);
			if (state == REPRODUCIR)
			{
				udValue = (udValue > 0) ? udValue - 1 : udValue;
				xQueueSend( repUdQueue, &udValue, (TickType_t) 0);
				vTaskResume(etHandle);
			}
			else if(state == MAIN)
			{
				udValue = (udValue > 0) ? udValue - 1 : udValue;
				xQueueSend( mainUdQueue, &udValue, (TickType_t) 0);
				vTaskResume(etHandle);
			}
		}
		else if (INTC_QT_FLAG._down) {
			INTC_QT_FLAG._down = false;
			//gpio_tgl_gpio_pin(LED3_GPIO);
			if (state == REPRODUCIR)
			{
				udValue = (udValue + 1 <= 2) ? udValue + 1 : udValue;
				xQueueSend( repUdQueue, &udValue, (TickType_t) 0);
				vTaskResume(etHandle);
			}
			else if(state == MAIN)
			{
				udValue = (udValue + 1 <= 2) ? udValue + 1 : udValue;
				xQueueSend( mainUdQueue, &udValue, (TickType_t) 0);
				vTaskResume(etHandle);
			}
		}
		else if (INTC_QT_FLAG._enter) {
			INTC_QT_FLAG._enter = false;
			xTaskNotifyGive(audioHandle);

		}

	}
}

// fsHandle
portTASK_FUNCTION_PROTO( fsTask, p );
portTASK_FUNCTION( fsTask, p )
{
	sdramQueue = xQueueCreate( 1 , sizeof(unsigned long));
	
	nav_filelist_reset();
	nav_filelist_goto( 0 );
	uint8_t files = 0;
	//while (nav_filelist_set(sd.drive_number, FS_FIND_NEXT))
	for(size_t i = 0; i < sd.number_of_files; i++)
	{
		nav_filelist_set(sd.drive_number, FS_FIND_NEXT);
		nav_file_getname(sd.name_of_files[i], 30);
		print_dbg(sd.name_of_files[i]);
		print_dbg("\r\n");
		files++;
		//vTaskDelay(pdMS_TO_TICKS(100));
	}
	if (files == sd.number_of_files)
	{
		print_dbg("Number of files coincide.\r\n");
	}


	uint8_t audio_files_collected = 0;
	uint32_t size_in_bytes = 0;
	uint8_t word_complete = 0;

	sdram_udata_t data_sd;
	portBASE_TYPE notificationValue = 0;

	nav_filelist_reset();
	nav_filterlist_setfilter("h");
	nav_filterlist_root();
	nav_filterlist_goto( 0 );
	while (nav_filelist_set( sd.drive_number, FS_FIND_NEXT ))					//nav_filterlist_next()
	{
		print_dbg("\r\n Archivo Encontrado\r");
		file_open(FOPEN_MODE_R);
		while (!file_eof())							//Hasta encontrar el fin del archivo
		{
			char current_char = file_getc();
			print_dbg_char(current_char);
			// Search for size fist, by looking for '[' and ']'
			if (current_char == '[')
			{
				char size_of_song[9] = "";
				current_char = file_getc();
				while( current_char != ']' ){
					strncat(size_of_song, &current_char, 1);
					current_char = file_getc();
				}
				size_in_bytes = a2ul(size_of_song);
				print_dbg("\r\nSize of song in bytes:");
				print_dbg_ulong(size_in_bytes);
				print_dbg("\r\n");
			}
			else if (current_char == '0')
			{
				char hex_byte[] = "";
				strncat(hex_byte, &current_char, 1);
				for (uint8_t i = 0; i < 3; i++)
				{
					current_char = file_getc();
					strncat(hex_byte, &current_char, 1);
				}
				//uint8_t data_byte = strtol(hex_byte, NULL, 0);
				//uint8_t data_byte;
				//sscanf(hex_byte, "%x", &data_byte);
				uint8_t data_byte = x2u8(hex_byte);
				// SDRAM
				data_sd.byte[word_complete] = data_byte;
				word_complete++;
			
				if (word_complete == 4)
				{
					while(!(notificationValue > 0))
					{
						notificationValue = ulTaskNotifyTake( pdTRUE, (TickType_t) 1 );
					}
					print_dbg("Here\r\n");
					word_complete = 0;
					
					xQueueSend( sdramQueue , &data_sd.word, (TickType_t) 1);
					vTaskResume(sdramHandle);
					//vTaskDelay(pdMS_TO_TICKS(100));
					
					memset(&data_sd, 0, sizeof(data_sd));
					print_dbg("Saved\r\n");
				}
			}
		
			//print_dbg_char(file_getc());				// Display next char from file.
			vTaskDelay(pdMS_TO_TICKS(100));
		}
		//end_pos = samples_collected;
		// Close the file.
		file_close();
		print_dbg("DONE WITH FIRST FILE, SAMPLES: ");
		//print_dbg_ulong(samples_collected);
		print_dbg("\r\n");
		//cpu_delay_ms(5000, PBA_HZ);
	
	}


	print_dbg("DONE");
	nav_exit();										// Cerramos sistemas de archivos

	while (1)
	{

	}
}

// ethHandle
portTASK_FUNCTION_PROTO( etTask, p );
portTASK_FUNCTION( etTask, p )
{
	while (1)
	{
		switch(state)
		{
			default:
			case MAIN:
				menu_gui(false);
				break;
			case REPRODUCIR:
				rep_menu(false);
				break;
			case LYRICS:
				break;
			case EQ:
				break;
		}
		vTaskSuspend(NULL);
	}
}

// sdramHandle
portTASK_FUNCTION_PROTO( sdramTask, p );
portTASK_FUNCTION( sdramTask, p )
{
	volatile unsigned long *sdram = SDRAM;
	
	//sdram_udata_t exram;
	//unsigned long sdram_size, progress_inc, i, j, noErrors = 0;
	unsigned long sdram_size = 0;
	uint32_t samples_collected = 0; // For SDRAM, sample times 4 will give the real number of song samples
	unsigned long sample = 0;
	
	// Calculate SDRAM size in words (32 bits).
	sdram_size = SDRAM_SIZE >> 2;
	print_dbg("\x0CSDRAM size: ");
	print_dbg_ulong(SDRAM_SIZE >> 20);
	print_dbg(" MB\r\n");

	//// Determine the increment of SDRAM word address requiring an update of the
	//// printed progression status.
	//progress_inc = (sdram_size + 50) / 100;
	//
	//// Fill the SDRAM with the test pattern.
	//for (i = 0, j = 0; i < sdram_size; i++)
	//{
		//if (i == j * progress_inc)
		//{
			//LED_Toggle(LED_SDRAM_WRITE);
			////print_dbg("\rFilling SDRAM with test pattern: ");
			////print_dbg_ulong(j++);
			////print_dbg_char('%');
			////print_dbg("\rByte number: ");
			////print_dbg_ulong(i);
		//}
		//sdram[i] = i;	
	//}
	//LED_Off(LED_SDRAM_WRITE);
	//print_dbg("\rSDRAM filled with test pattern       \r\n");
	//
	//// Recover the test pattern from the SDRAM and verify it.
	//for (i = 0, j = 0; i < sdram_size; i++)
	//{
		//if (i == j * progress_inc)
		//{
			//LED_Toggle(LED_SDRAM_READ);
			////print_dbg("\rRecovering test pattern from SDRAM: ");
			////print_dbg_ulong(j++);
			////print_dbg_char('%');
		//}
		//exram.word = sdram[i];
		////if ( (exram.byte[0] != MASK_B0(i)) || (exram.byte[1] != MASK_B1(i)) || (exram.byte[2] != MASK_B2(i)) || (exram.byte[3] != MASK_B3(i)) )
		//if ( (exram.b0 != MASK_B0(i)) || (exram.b1 != MASK_B1(i)) || (exram.b2 != MASK_B2(i)) || (exram.b3 != MASK_B3(i)) )
		//{
			//noErrors++;
		//}
	//}
	//LED_Off(LED_SDRAM_READ);
	////print_dbg("\rSDRAM tested: ");
	////print_dbg_ulong(noErrors);
	////print_dbg(" corrupted word(s)       \r\n");
	//if (noErrors)
	//{
		//LED_Off(LED_SDRAM_ERRORS);
		//print_dbg("More than one error check.");
		//while (1)
		//{
			//LED_Toggle(LED_SDRAM_ERRORS);
			////cpu_delay_ms(200, PBA_HZ);   // Fast blink means errors.
			//vTaskDelay(pdMS_TO_TICKS(200));
		//}
	//}
	//else
	//{
		//LED_Off(LED_SDRAM_OK);
		//print_dbg("No error.");
		//while (1)
		//{
		//LED_Toggle(LED_SDRAM_OK);
		////cpu_delay_ms(1000, PBA_HZ);  // Slow blink means OK.
		//vTaskDelay(pdMS_TO_TICKS(1000));
	//
		//}
	//}


	print_dbg("Suspending task");
	xTaskNotifyGive(fsHandle);
	vTaskSuspend(NULL);
	
	while(1)
	{
		if (sdramQueue != 0)
		{
			if (xQueueReceive( sdramQueue, &sample, (TickType_t) 2 ))
			{
				print_dbg("SDRAM QUEUE Received\r\n");
				print_dbg_ulong(sample);
				print_dbg("\r\n");
				sdram[samples_collected++] = sample;
				print_dbg_ulong(samples_collected);
				if (samples_collected > 15)
				{
					vTaskSuspend(fsHandle);
					for (uint8_t i = 0; i < samples_collected; i++)
					{
						print_dbg_ulong(sdram[i]);
						print_dbg("\r\n");
						vTaskDelay(pdMS_TO_TICKS(1000));
					}
					vTaskSuspend(NULL);
				}
				else
				{
					xTaskNotifyGive(fsHandle);
					vTaskSuspend(NULL);
				}
				
			}
		}
		
	}

	
}

// ISR
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

ISR_FREERTOS(RT_ISR_tc0_irq_448, 448, 1)
{

	// Clear the interrupt flag. This is a side effect of reading the TC SR.
	TC_TICK.ch0++;
	// Clear the interrupt flag. This is a side effect of reading the TC SR.
	tc_read_sr(&AVR32_TC, TC_CHANNEL_0);

	BaseType_t checkIfYieldRequired = xTaskResumeFromISR(myIntTaskHandleTC);
	return (checkIfYieldRequired ? 1 : 0);
}

/*! \brief Static function definitions
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
		.pba_hz = PBA_HZ,
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
						PBA_HZ); /**/

	tpa6130_set_volume(0x35); // 2F
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

static void init_tft_bl(void)
{
	avr32_pwm_channel_t pwm_channel6;
	pwm_channel6.cdty = 0;
	pwm_channel6.cprd = 100;

	et024006_Init(PBA_HZ, PBA_HZ);

	pwm_opt_t opt;
	opt.diva = 0;
	opt.divb = 0;
	opt.prea = 0;
	opt.preb = 0;

	pwm_init(&opt);
	pwm_channel6.CMR.calg = PWM_MODE_LEFT_ALIGNED;
	pwm_channel6.CMR.cpol = PWM_POLARITY_HIGH; //PWM_POLARITY_LOW;//PWM_POLARITY_HIGH;
	pwm_channel6.CMR.cpd = PWM_UPDATE_DUTY;
	pwm_channel6.CMR.cpre = AVR32_PWM_CMR_CPRE_MCK_DIV_2;

	pwm_channel_init(6, &pwm_channel6);
	pwm_start_channels(AVR32_PWM_ENA_CHID6_MASK);

	et024006_DrawFilledRect(0, 0, ET024006_WIDTH, ET024006_HEIGHT, WHITE);

	while(pwm_channel6.cdty < pwm_channel6.cprd)
	{
		pwm_channel6.cdty++;
		pwm_channel6.cupd = pwm_channel6.cdty;
		//pwm_channel6.cdty--;
		pwm_async_update_channel(AVR32_PWM_ENA_CHID6, &pwm_channel6);
		delay_ms(10);
	}
}

static void init_fs(void)
{
	/*Inicialiazamos los archivos de la SD manejada por SPI, para lograr que funcionara tuvo que habilitarla mediante el
	conf_access.h y conf_explorer.h, debido a que por default viene habilitada la memoria incluida en la EVK*/
	
#ifdef FREERTOS_USED
	if (ctrl_access_init())
	{
		print_dbg("Access to SD granted.\r\n");
	}
#endif

	first_ls = true;
	if (nav_drive_get() >= nav_drive_nb() || first_ls)
	{
		first_ls = false;
		// Reset navigators .
		nav_reset();
		// Use the last drive available as default.
		nav_drive_set(nav_drive_nb() - 1); // Or sd.drive_number
		//nav_drive_set(nav_drive_nb() - 1);
		// Mount it.
		nav_partition_mount();
	}
	nav_dir_name((FS_STRING)str_buff, MAX_FILE_PATH_LENGTH);
	// Try to sort items by folders
	if (!nav_filelist_first(FS_DIR))
	{
		// Sort items by files
		nav_filelist_first(FS_FILE);
	}
	nav_filelist_reset();
	// Get data
	sd.lun				 = get_nb_lun();				// Read actual LUN
	sd.drive_name		 = nav_drive_getname();			// Read drive assigned letter
	sd.devices_available = nav_drive_nb();				// Read available devices. Equal to LUN
	sd.drive_number		 = nav_drive_get();				// Returns nav_drive_nb()-1
	sd.number_of_files	 = nav_filelist_nb(FS_FILE);	// Get the number of available files
	print_dbg_ulong(sd.number_of_files);
}

static void rep_menu(bool init)
{
	static bool first_time = true;
	static menu_keys_t keys;

	if (repLrQueue != 0)
	{
		if (xQueueReceive( repLrQueue, &keys.lr, (TickType_t) 2 ))
		{
			print_dbg("Received for REP LR");
			print_dbg_ulong(keys.lr);
		}
	}
	
	if (repUdQueue != 0)
	{
		if (xQueueReceive( repUdQueue, &keys.ud, (TickType_t) 2 ))
		{
			print_dbg("Received for REP UD");
			print_dbg_ulong(keys.ud);
		}
	}

	if (first_time || init)
	{
		first_time = false;
		memset(&keys, 0, sizeof(keys));

		et024006_DrawFilledRect(0, 0, 320, 240, BLACK);
		et024006_PrintString("Lyrics",		 (const unsigned char *) &FONT8x16, 10, 10, WHITE, -1);
		et024006_PrintString("Main",		 (const unsigned char *) &FONT8x16, 120, 10, WHITE, -1);
		et024006_PrintString("Ecualizador", (const unsigned char *) &FONT8x16, 220, 10, WHITE, -1);

		et024006_PutPixmap(letdown, 70, 0, 0, 120, 70, 70, 70);
		et024006_PrintString("Let Down", (const unsigned char *) &FONT8x16, 125, 160, WHITE, -1);

		et024006_PutPixmap(atrasarblanco, 50, 0, 0, 10, 190, 50, 50);
		et024006_PutPixmap(play, 50, 0, 0, 135, 190, 50, 50);
		et024006_PutPixmap(adelantar, 50, 0, 0, 260, 190, 50, 50);
	}

	if (keys.ud == 0 && keys.lr == 0){
		et024006_PrintString("Lyrics", (const unsigned char*) &FONT8x16, 10, 10, GREEN, -1);
		et024006_PrintString("Main", (const unsigned char*) &FONT8x16, 120, 10, WHITE, -1);
		et024006_PrintString("Ecualizador", (const unsigned char*) &FONT8x16, 220, 10, WHITE, -1);
	}
	else if (keys.ud == 0 && keys.lr == 1){
		et024006_PrintString("Lyrics", (const unsigned char*) &FONT8x16, 10, 10, WHITE, -1);
		et024006_PrintString("Main", (const unsigned char*) &FONT8x16, 120, 10, GREEN, -1);
		et024006_PrintString("Ecualizador", (const unsigned char*) &FONT8x16, 220, 10, WHITE, -1);
	}
	else if (keys.ud == 0 && keys.lr == 2){
		et024006_PrintString("Lyrics", (const unsigned char*) &FONT8x16, 10, 10, WHITE, -1);
		et024006_PrintString("Main", (const unsigned char*) &FONT8x16, 120, 10, WHITE, -1);
		et024006_PrintString("Ecualizador", (const unsigned char*) &FONT8x16, 220, 10, GREEN, -1);
	}
	else if (keys.ud == 2 && keys.lr == 0){
		et024006_PutPixmap(letdown, 70, 0, 0, 120, 70, 70, 70);
		et024006_PrintString("cancion2", (const unsigned char*) &FONT8x16, 120, 170, WHITE, -1);

	}
	else if (keys.ud == 2 && keys.lr == 1){
		et024006_PutPixmap(atrasarblanco, 50, 0, 0, 10, 190, 50, 50);
		et024006_PutPixmap(playverde, 50, 0, 0, 140, 190, 50, 50);
		et024006_PutPixmap(adelantar, 50, 0, 0, 260, 190, 50, 50);
	}
	else if(keys.ud == 2 && keys.lr == 0){
		et024006_PutPixmap(atrasarverde, 50, 0, 0, 10, 190, 50, 50);
		et024006_PutPixmap(play, 50, 0, 0, 140, 190, 50, 50);
		et024006_PutPixmap(adelantar, 50, 0, 0, 260, 190, 50, 50);
	}
	else if (keys.ud == 2 && keys.lr == 2){
		et024006_PutPixmap(atrasarblanco, 50, 0, 0, 10, 190, 50, 50);
		et024006_PutPixmap(play, 50, 0, 0, 140, 190, 50, 50);
		et024006_PutPixmap(adelantarverde, 50, 0, 0, 260, 190, 50, 50);
	}

}

static void menu_gui(bool init)
{
	static bool first_time = true;
	static menu_keys_t keys;
	
	if (mainLrQueue != 0)
	{
		if (xQueueReceive( mainLrQueue, &keys.lr, (TickType_t) 2 ))
		{
			print_dbg("Received for MAIN LR");
			print_dbg_ulong(keys.lr);
		}
	}
	
	if (mainUdQueue != 0)
	{
		if (xQueueReceive( mainUdQueue, &keys.ud, (TickType_t) 2 ))
		{
			print_dbg("Received for MAIN UD");
			print_dbg_ulong(keys.ud);
		}
	}
	
	if (first_time || init)
	{
		first_time = false;
		et024006_DrawFilledRect(0, 0, 320, 240, BLACK);
		et024006_DrawVertLine(160,0,120,WHITE);
		et024006_DrawVertLine(160,120,120,WHITE);
	
		et024006_DrawHorizLine(0,120,160,WHITE);
		et024006_DrawHorizLine(160,120,160,WHITE);
	
	
		et024006_PutPixmap(letdown, 70, 0, 0, 30, 140, 70, 70);
		et024006_PrintString("dur: 4:59", (const unsigned char *) &FONT8x8, 30, 220, WHITE, -1);
	
		et024006_PutPixmap(wearethechampions, 70, 0, 0, 30, 20, 70, 70);
		et024006_PrintString("dur: 3:04", (const unsigned char*) &FONT8x8, 30, 100, WHITE, -1);
	
		et024006_PutPixmap(fercaspian, 70, 0, 0, 190, 20, 70, 70);
		et024006_PrintString("dur: 3:04", (const unsigned char*) &FONT8x8, 190, 100, WHITE, -1);
	
		et024006_PutPixmap(takeonme, 70, 0, 0, 190, 140, 70, 70);
		et024006_PrintString("dur: 4:36", (const unsigned char*) &FONT8x8, 190, 220, WHITE, -1);
		
	}
	
	if (keys.ud == 0 && keys.lr == 0){
		et024006_DrawVertLine(160,0,120,WHITE);
		et024006_DrawVertLine(160,120,120,WHITE);
		et024006_DrawHorizLine(0,120,160,WHITE);
		et024006_DrawHorizLine(160,120,160,WHITE);
		et024006_DrawVertLine(160,0,120,GREEN);
		et024006_DrawHorizLine(0,120,160,GREEN);
		//if(center==1){
			//et024006_DrawFilledRect(30, 20, 100, 90, BLACK);
			//et024006_PrintString("We are the champions", (const unsigned char*) &FONT8x8, 30, 20, WHITE, -1);
			//et024006_PrintString("Queen", (const unsigned char*) &FONT8x8, 30, 40, WHITE, -1);
			//et024006_PrintString("News of the World", (const unsigned char*) &FONT8x8, 30, 60, WHITE, -1);
			//et024006_PrintString("1977", (const unsigned char*) &FONT8x8, 30, 80, WHITE, -1);
			//et024006_PrintString("dur: 3:04", (const unsigned char*) &FONT8x8, 30, 100, BLACK, -1);
		//}
	}
	else if (keys.ud == 1 && keys.lr == 0){
		et024006_DrawVertLine(160,0,120,WHITE);
		et024006_DrawVertLine(160,120,120,WHITE);
		et024006_DrawHorizLine(0,120,160,WHITE);
		et024006_DrawHorizLine(160,120,160,WHITE);
		et024006_DrawVertLine(160,120,120,GREEN);
		et024006_DrawHorizLine(0,120,160,GREEN);
		//if(center==1){
			//et024006_DrawFilledRect(30, 140, 100, 210, BLACK);
			//et024006_PrintString("Let Down", (const unsigned char*) &FONT8x8, 30, 140, WHITE, -1);
			//et024006_PrintString("Radiohead", (const unsigned char*) &FONT8x8, 30, 160, WHITE, -1);
			//et024006_PrintString("Ok Computer", (const unsigned char*) &FONT8x8, 30, 180, WHITE, -1);
			//et024006_PrintString("1997", (const unsigned char*) &FONT8x8, 30, 200, WHITE, -1);
			//et024006_PrintString("dur: 4:59", (const unsigned char*) &FONT8x8, 30, 220, BLACK, -1);
		//}
	}
	else if (keys.ud == 0 && keys.lr == 1){
		et024006_DrawVertLine(160,0,120,WHITE);
		et024006_DrawVertLine(160,120,120,WHITE);
		et024006_DrawHorizLine(0,120,160,WHITE);
		et024006_DrawHorizLine(160,120,160,WHITE);
		et024006_DrawVertLine(160,0,120,GREEN);
		et024006_DrawHorizLine(160,120,160,GREEN);
		//if(center==1){
			//et024006_DrawFilledRect(190, 20, 260, 90, BLACK);
			//et024006_PrintString("Let�s go outside", (const unsigned char*) &FONT8x8,190, 20, WHITE, -1);
			//et024006_PrintString("Far caspian", (const unsigned char*) &FONT8x8, 190, 40, WHITE, -1);
			//et024006_PrintString("between days", (const unsigned char*) &FONT8x8, 190, 60, WHITE, -1);
			//et024006_PrintString("2018", (const unsigned char*) &FONT8x8, 190, 80, WHITE, -1);
			//et024006_PrintString("dur: 4:36", (const unsigned char*) &FONT8x8, 190, 100, BLACK, -1);
		//}
	}
	else if (keys.ud == 1 && keys.lr == 1){
		et024006_DrawVertLine(160,0,120,WHITE);
		et024006_DrawVertLine(160,120,120,WHITE);
		et024006_DrawHorizLine(0,120,160,WHITE);
		et024006_DrawHorizLine(160,120,160,WHITE);
		et024006_DrawVertLine(160,120,120,GREEN);
		et024006_DrawHorizLine(160,120,160,GREEN);
		//if(center==1){
			//et024006_DrawFilledRect(190, 140, 260, 210, BLACK);
			//et024006_PrintString("Take on Me", (const unsigned char*) &FONT8x8,190, 140, WHITE, -1);
			//et024006_PrintString("a-Ha", (const unsigned char*) &FONT8x8, 190, 160, WHITE, -1);
			//et024006_PrintString("Hunting High and Low", (const unsigned char*) &FONT8x8, 190, 180, WHITE, -1);
			//et024006_PrintString("1985", (const unsigned char*) &FONT8x8, 190, 200, WHITE, -1);
			//et024006_PrintString("dur: 4:36", (const unsigned char*) &FONT8x8, 190, 220, BLACK, -1);
		//}
	}
	
	
}

static void init_sdram(void)
{
	// Initialize the external SDRAM chip.
	sdramc_init(PBA_HZ);
	print_dbg("\r\nSDRAM initialized\r\n");
}

static void get_files(void)
{
	
	nav_filelist_reset();
	nav_filelist_goto( 0 );
	uint8_t files = 0;
	//while (nav_filelist_set(sd.drive_number, FS_FIND_NEXT))
	for(size_t i = 0; i < sd.number_of_files; i++)
	{
		nav_filelist_set(sd.drive_number, FS_FIND_NEXT);
		nav_file_getname(sd.name_of_files[i], 30);
		print_dbg(sd.name_of_files[i]);
		print_dbg("\r\n");
		files++;
		//vTaskDelay(pdMS_TO_TICKS(100));
	}
	if (files == sd.number_of_files)
	{
		print_dbg("Number of files coincide.\r\n");
	}
	
	
	uint8_t audio_files_collected = 0;
	uint32_t size_in_bytes = 0;
	uint8_t word_complete = 0;
	uint32_t samples_collected = 0; // For SDRAM, sample times 4 will give the real number of song samples
	uint32_t init_pos = 0;
	uint32_t end_pos = 0;
	sdram_udata_t data_sd;
	
	nav_filelist_reset();
	nav_filterlist_setfilter("h");
	nav_filterlist_root();
	nav_filterlist_goto( 0 );
	while (nav_filelist_set( sd.drive_number, FS_FIND_NEXT ))					//nav_filterlist_next()
	{
		print_dbg("\r\n Archivo Encontrado\r");
		file_open(FOPEN_MODE_R);
		while (!file_eof())							//Hasta encontrar el fin del archivo
		{
			char current_char = file_getc();
			print_dbg_char(current_char);
			// Search for size fist, by looking for '[' and ']'
			if (current_char == '[')
			{
				char size_of_song[9] = "";
				current_char = file_getc();
				while( current_char != ']' ){
					strncat(size_of_song, &current_char, 1);
					current_char = file_getc();
				}
				size_in_bytes = a2ul(size_of_song);
				print_dbg("\r\nSize of song in bytes:");
				print_dbg_ulong(size_in_bytes);
				print_dbg("\r\n");
			}
			else if (current_char == '0')
			{
				char hex_byte[] = "";
				strncat(hex_byte, &current_char, 1);
				for (uint8_t i = 0; i < 3; i++)
				{
					current_char = file_getc();
					strncat(hex_byte, &current_char, 1);
				}
				//uint8_t data_byte = strtol(hex_byte, NULL, 0);
				//uint8_t data_byte;
				//sscanf(hex_byte, "%x", &data_byte);
				uint8_t data_byte = x2u8(hex_byte);
				// SDRAM
				data_sd.byte[word_complete] = data_byte;
				word_complete++;
				
				if (word_complete == 4)
				{
					print_dbg("Here\r\n");
					word_complete = 0;
					//sdram[0] = data_sd.word;
					samples_collected++;
					memset(&data_sd, 0, sizeof(data_sd));
					print_dbg("Saved\r\n");
				}
			}
			
			//print_dbg_char(file_getc());				// Display next char from file.
			//vTaskDelay(pdMS_TO_TICKS(200));
		}
		end_pos = samples_collected;
		// Close the file.
		file_close();
		print_dbg("DONE WITH FIRST FILE, SAMPLES: ");
		print_dbg_ulong(samples_collected);
		print_dbg("\r\n");
		cpu_delay_ms(5000, PBA_HZ);
		
	}

	
	print_dbg("DONE");
	nav_exit();										// Cerramos sistemas de archivos
}

static unsigned long a2ul(const char *s)
{
	//unsigned long x = 0;
	//while(isdigit(*s))
	//{
	//x *= 10;
	//x += *s++ - '0';
	//}
	//return x;
	
	unsigned long result = 0;
	const char *c = s;

	while ('0' <= *c && *c <= '9') {
		result = result * 10 + (*(c++) - '0');
	}
	return result;
}

static uint8_t x2u8(const char *str)
{
	uint8_t res = 0; // uint64_t
	char c;

	while ((c = *str++)) {
		char v = ((c & 0xF) + (c >> 6)) | ((c >> 3) & 0x8);
		res = ((res << 4) | (uint8_t) v);
	}

	return res;
}

int main (void)
{
	/* Insert system clock initialization code here (sysclk_init()). */
	init_sys_clocks();

	/* Initialize RS232 debug text output. */
	init_dbg_rs232(PBA_HZ); /**/

	init_tft_bl();

	init_twi_tpa();

	sd_mmc_resources_init();
	check_sd_card();
	et024006_PrintString("****** INIT SD/MMC ******",(const unsigned char*)&FONT8x8,10,10,BLUE,-1);

	init_qt_interrupt();

	init_fs();
	
	init_sdram();

	// Enable LED0 and LED1
	gpio_enable_gpio_pin(LED0_GPIO);
	gpio_enable_gpio_pin(LED1_GPIO);

	print_dbg(MSG_WELCOME);
	
	//get_files();

	/* Insert application code here, after the board has been initialized. */

	//uint16_t pass = 25;
	//xTaskCreate(myTask1, "taks1", 256, (void *)pass, mainLED_TASK_PRIORITY, &myTask1Handle);
	//xTaskCreate(qtButtonTask,  "tQT",        256,  (void *) 0, mainCOM_TEST_PRIORITY, &qtHandle);
	//xTaskCreate(playAudioTask, "tPlayAudio", 2048, (void *) 0, mainLED_TASK_PRIORITY, &audioHandle);
	xTaskCreate(fsTask,		   "tFS",		 1024,  (void *) 0, mainLED_TASK_PRIORITY, &fsHandle);
	//xTaskCreate(etTask,		   "tET",		 512,  (void *) 0, mainLED_TASK_PRIORITY, &etHandle);
	xTaskCreate(sdramTask,     "tSDRAM",	 256,  (void *) 0, mainLED_TASK_PRIORITY + 1, &sdramHandle);
	
	vTaskStartScheduler();

	while (1)
	{

	}

	return 0;

}

/*-----------------------------------------------------------*/
