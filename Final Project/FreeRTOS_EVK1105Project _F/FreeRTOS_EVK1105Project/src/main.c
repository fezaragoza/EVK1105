/*****************************************************************************
 *
 * \file
 *
 * \brief FreeRTOS Real Time Kernel example.
 *
 * Creates all the demo application tasks, then starts the scheduler.  The WEB
 * documentation provides more details of the demo application tasks.
 *
 * Main. c also creates a task called "Check".  This only executes every three
 * seconds but has the highest priority so is guaranteed to get processor time.
 * Its main function is to check that all the other tasks are still operational.
 * Each task that does not flash an LED maintains a unique count that is
 * incremented each time the task successfully completes its function.  Should
 * any error occur within such a task the count is permanently halted.  The
 * check task inspects the count of each task to ensure it has changed since
 * the last time the check task executed.  If all the count variables have
 * changed all the tasks are still executing error free, and the check task
 * toggles an LED.  Should any task contain an error at any time the LED toggle
 * will stop.
 *
 * The LED flash and communications test tasks do not maintain a count.
 *
 *****************************************************************************/

/*
    FreeRTOS V7.0.0 - Copyright (C) 2011 Real Time Engineers Ltd.


    ***************************************************************************
     *                                                                       *
     *    FreeRTOS tutorial books are available in pdf and paperback.        *
     *    Complete, revised, and edited pdf reference manuals are also       *
     *    available.                                                         *
     *                                                                       *
     *    Purchasing FreeRTOS documentation will not only help you, by       *
     *    ensuring you get running as quickly as possible and with an        *
     *    in-depth knowledge of how to use FreeRTOS, it will also help       *
     *    the FreeRTOS project to continue with its mission of providing     *
     *    professional grade, cross platform, de facto standard solutions    *
     *    for microcontrollers - completely free of charge!                  *
     *                                                                       *
     *    >>> See http://www.FreeRTOS.org/Documentation for details. <<<     *
     *                                                                       *
     *    Thank you for using FreeRTOS, and thank you for your support!      *
     *                                                                       *
    ***************************************************************************


    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation AND MODIFIED BY the FreeRTOS exception.
    >>>NOTE<<< The modification to the GPL is included to allow you to
    distribute a combined work that includes FreeRTOS without being obliged to
    provide the source code for proprietary components outside of the FreeRTOS
    kernel.  FreeRTOS is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
    more details. You should have received a copy of the GNU General Public
    License and the FreeRTOS license exception along with FreeRTOS; if not it
    can be viewed here: http://www.freertos.org/a00114.html and also obtained
    by writing to Richard Barry, contact details for whom are available on the
    FreeRTOS WEB site.

    1 tab == 4 spaces!

    http://www.FreeRTOS.org - Documentation, latest information, license and
    contact details.

    http://www.SafeRTOS.com - A version that is certified for use in safety
    critical systems.

    http://www.OpenRTOS.com - Commercial support, development, porting,
    licensing and training services.
*/


#include <asf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

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
#include "lib/sound.h"
#include "img/image.h"
//#include "lib/letdown.h"

/*-----------------------------------------------------------*/
/*--------------------------FREERTOS-------------------------*/
/*-----------------------------------------------------------*/

/* Local Definitions */

#define mainLED_TASK_PRIORITY     ( tskIDLE_PRIORITY + 1 )
#define mainCOM_TEST_PRIORITY     ( tskIDLE_PRIORITY + 2 )
#define mainQUEUE_POLL_PRIORITY   ( tskIDLE_PRIORITY + 2 )
#define mainSEM_TEST_PRIORITY     ( tskIDLE_PRIORITY + 1 )
#define mainBLOCK_Q_PRIORITY      ( tskIDLE_PRIORITY + 3 )
#define mainCHECK_TASK_PRIORITY   ( tskIDLE_PRIORITY + 4 )
#define mainCREATOR_TASK_PRIORITY ( tskIDLE_PRIORITY + 3 )

#define mainCOM_TEST_BAUD_RATE    ( ( unsigned portLONG ) 57600 )

#define mainCOM_TEST_LED          ( 3 )

#define mainCHECK_TASK_LED        ( 6 )

#define mainERROR_LED             ( 7 )

#define mainCHECK_PERIOD          ( ( portTickType ) 1000 / portTICK_RATE_MS  )

#define mainERROR_FLASH_RATE      ( (portTickType) 500 / portTICK_RATE_MS )

#define mainCOUNT_INITIAL_VALUE   ( ( unsigned portLONG ) 0 )
#define mainNO_TASK               ( 0 )

#define mainMEM_CHECK_SIZE_1      ( ( size_t ) 51 )
#define mainMEM_CHECK_SIZE_2      ( ( size_t ) 52 )
#define mainMEM_CHECK_SIZE_3      ( ( size_t ) 15 )

/* Prototypes */
portTASK_FUNCTION_PROTO( etTask,		p );
portTASK_FUNCTION_PROTO( sdramTask,		p );
portTASK_FUNCTION_PROTO( fsTask,		p );
portTASK_FUNCTION_PROTO( playAudioTask, p );
portTASK_FUNCTION_PROTO( qtButtonTask,	p );
portTASK_FUNCTION_PROTO( tftTask,		p );

/* TaskHandles */
TaskHandle_t myIntTaskHandleTC = NULL;
TaskHandle_t qtHandle		   = NULL;
TaskHandle_t audioHandle	   = NULL;
TaskHandle_t fsHandle		   = NULL;
TaskHandle_t etHandle		   = NULL;
TaskHandle_t sdramHandle	   = NULL;
TaskHandle_t tftHandle		   = NULL;

/* QueueHandles */
QueueHandle_t forwardQueue;
QueueHandle_t reverseQueue;
QueueHandle_t lrQueue;
QueueHandle_t udQueue;
QueueHandle_t initBoolQueue;
QueueHandle_t volumeUdQueue;
QueueHandle_t sdramQueue;
QueueHandle_t tftQueue;
QueueHandle_t etToggleQueue;

/*-----------------------------------------------------------*/
/*							DEFINES							 */
/*-----------------------------------------------------------*/

/*************** TPA ***************/
#define MSG_WELCOME "\x1B[2J\x1B[H---------- Welcome to Final Project ---------- \r\n"	//! Welcome message to display.
#define SOUND_SAMPLES             512													//! Sample Count Value
#define TPA6130_TWI_MASTER_SPEED  100000
#define MAX_NUMBER_OF_SONGS		  10
#define SIZE_OF_STRING			  20
#define AUDIO					  0

/*************  SDRAM  **************/
#define LED_SDRAM_WRITE     LED0
#define LED_SDRAM_READ      LED1
#define LED_SDRAM_ERRORS    (LED0 | LED1 | LED2 | LED3)
#define LED_SDRAM_OK        (LED0 | LED1 | LED2 | LED3)

#define MASK_B0(x) (uint8_t)( ((0xFF << 24) & x) >> 24 )
#define MASK_B1(x) (uint8_t)( ((0xFF << 16) & x) >> 16 )
#define MASK_B2(x) (uint8_t)( ((0xFF << 8)  & x) >> 8 )
#define MASK_B3(x) (uint8_t)( ((0xFF << 0)  & x) >> 0 )

/**********  TFT - TOUCH  ***********/
#define TR 63
#define TT 62
#define TB 23
#define TL 21

/*************** MAIN ***************/
/* Local Definitions */
#define RC0_VALUE		46875 // 37500 // 100 ms

/*-----------------------------------------------------------*/
/*							TYPEDEFS						 */
/*-----------------------------------------------------------*/

/*************** TPA ***************/

/************  SD-FAT  *************/
typedef struct
{
	char name[20];
	char artist[20];
	char album[20];
	char year[20];
	char duration[20];
}song_info_t;

typedef struct
{
	UBaseType_t size_in_bytes;
	UBaseType_t init_ptr;
	UBaseType_t end_ptr;
} audio_data_t;

typedef struct
{
	uint8_t		 lun;
	char		 drive_name;
	uint8_t		 devices_available;							// Same value as lun.
	uint8_t		 drive_number;
	uint8_t		 number_of_files;
	uint8_t		 number_of_audio_files;
	char		 name_of_files[25][30];							// Strings of each name inside SD card. 30 Size of buffer
	char		 name_of_audio_files[MAX_NUMBER_OF_SONGS][30];	// Strings of each name of the audio songs. 30 Size of buffer
	audio_data_t audio_data[MAX_NUMBER_OF_SONGS];			// Audio data with pointer reference per song.
}sd_fat_data_t;

typedef struct  
{
	char line1[40];
	char line2[40];
	char line3[40];
	char line4[40];
	char line5[40];
	char line6[40];
}lyrics_t;

/************  SDRAM  *************/
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

/*************** ET-LCD ***************/
typedef struct 
{
	uint8_t pages_available;
	uint8_t actual_page;
} et_data_t;

/**********  TFT - TOUCH  ***********/
typedef struct
{
	int16_t v1;
	int16_t v2;
	int16_t v3;
	int16_t v4;
	int16_t v5;
	int16_t v21;
	int16_t v32;
	int16_t v43;
	int16_t v54;
	int16_t vprom;
	int16_t vtotal;
} filter_t;

/*************** MAIN ***************/
typedef enum
{
	MAIN,
	REPRODUCIR,
	LYRICS,
	VOLUME
} state_t;

typedef struct
{
	uint8_t lr;
	uint8_t ud;
} menu_keys_t;

typedef enum
{
	LYRICS_GUI,
	MAIN_GUI,
	VOLUME_GUI,
	PLAY,
	FORWARD,
	REVERSE
} reproduce_menu_t;

typedef float float16_t;
typedef double float32_t;

/*-----------------------------------------------------------*/
/*							PROTOTYPES						 */
/*-----------------------------------------------------------*/

/*************** TPA ***************/
void dac_reload_callback(void);
void dac_overrun_callback(void);
void adc_underrun_callback(void);
void adc_reload_callback(void);

/*************** MAIN ***************/
static void init_sdram(void);
static void init_fs(void);
static void rep_menu(bool);
static void menu_gui(bool, bool, bool);
static void volumen_gui(bool);
static void lyrics_gui(bool init);
static unsigned long a2ul(const char*);
static uint8_t x2u8(const char*);

/**********  TFT - TOUCH  ***********/
static void get_XY(void);
static int32_t moving_filter(filter_t* x, filter_t* y);

/*-----------------------------------------------------------*/
/*						  DECLARATIONS						 */
/*-----------------------------------------------------------*/

/***************   TPA    ***************/
static int16_t     samples[SOUND_SAMPLES];
static song_info_t song_info[MAX_NUMBER_OF_SONGS];
static uint8_t     selected_song = 0;
static int8_t	   volume = 0x00;
static uint32_t	   sdram_song_ptr = 0;
static bool		   playAudio = false;
//static char song_data[10][5][20]; // Songs, parameters, data

/***************    FAT   *****************/
static char			 str_buff[MAX_FILE_PATH_LENGTH];
static char			 filenames[4][MAX_FILE_PATH_LENGTH];
static bool			 first_ls;
static sd_fat_data_t sd;
static lyrics_t	     song_lyrics[MAX_FILE_PATH_LENGTH];
/***************  SDRAM  *****************/
//volatile unsigned long *sdram = SDRAM;
unsigned long sdram_ptr  = 0;	// Next location - word
unsigned long sdram_size = 0;	// Number of words

/*************** ET-LCD ***************/
static et_data_t et_data;
static reproduce_menu_t reproduce_option;

/**********  TFT - TOUCH  ***********/
int32_t x_touch;
int32_t y_touch;

/***************   MAIN   ***************/
// Module's memory address
volatile avr32_tc_t *tc   = &AVR32_TC;
volatile avr32_pm_t *pm	  = &AVR32_PM;
volatile avr32_adc_t *adc = &AVR32_ADC;

intc_qt_flags_t intc_qt;
intc_tc_flags_t intc_tc;
static state_t state = MAIN;

/*-----------------------------------------------------------*/
/*						  FUNCTION DEFS						 */
/*-----------------------------------------------------------*/
/***************   TPA    ***************/
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

/*************** FREERTOS ***************/
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

// fsHandle
portTASK_FUNCTION( fsTask, p )
{
	sdramQueue = xQueueCreate( 1 , sizeof(unsigned long));
	
	/****		Get all files data	 ****/
	nav_filelist_reset();
	nav_filelist_goto( 0 ); // System volume information
	uint8_t files = 0;
	print_dbg("Files: \r\n");
	//while (nav_filelist_set(sd.drive_number, FS_FIND_NEXT))
	for(size_t i = 0; i < sd.number_of_files; i++)
	{
		nav_filelist_set(sd.drive_number, FS_FIND_NEXT);
		nav_file_getname(sd.name_of_files[i], 30);
		print_dbg(sd.name_of_files[i]);
		print_dbg("\r\n");
		files++;
		vTaskDelay(pdMS_TO_TICKS(100));
	}
	if (files == sd.number_of_files)
	{
		print_dbg("Number of files coincide.\r\n");
	}
	print_dbg_ulong(sd.number_of_files / 2);
	
	/***	Retrieve Info data	****/
	char info[20];
	char lyrics[40];
	FS_STRING name;
	nav_filterlist_reset();
	nav_filterlist_setfilter("txt");
	nav_filterlist_root();
	nav_filterlist_goto( 0 ); // System volume information
	for (size_t i = 0; i < 2*(sd.number_of_files / 3); i++)
	{
		if (i < (sd.number_of_files / 3))
		{
			nav_filterlist_next();
			nav_file_getname(name, 30);
			print_dbg(name);
			print_dbg("\r\n");
			
			//file_open(FOPEN_MODE_R);
			reader_txt_open( true );
			
			reader_txt_get_line(false, info, 20);
			strcpy(song_info[i].name, info);
			reader_txt_get_line(false, info, 20);
			strcpy(song_info[i].artist, info);
			reader_txt_get_line(false, info, 20);
			strcpy(song_info[i].album, info);
			reader_txt_get_line(false, info, 20);
			strcpy(song_info[i].year, info);
			reader_txt_get_line(false, info, 20);
			strcpy(song_info[i].duration, info);

			// Close the file.
			reader_txt_close();
			
			print_dbg(song_info[i].name);
			print_dbg(song_info[i].artist);
			print_dbg(song_info[i].album);
			print_dbg(song_info[i].year);
			print_dbg(song_info[i].duration);
			
			print_dbg("\r\n");
		}
		else
		{
			uint8_t j = i - 8;
			
			nav_filterlist_next();
			nav_file_getname(name, 30);
			print_dbg(name);
			print_dbg("\r\n");
			
			//file_open(FOPEN_MODE_R);
			reader_txt_open( true );
			
			reader_txt_get_line(false, lyrics, 40);
			strcpy(song_lyrics[j].line1, lyrics);
			memset(lyrics, 0, 40*sizeof(char));
			reader_txt_get_line(false, lyrics, 40);
			strcpy(song_lyrics[j].line2, lyrics);
			memset(lyrics, 0, 40*sizeof(char));
			reader_txt_get_line(false, lyrics, 40);
			strcpy(song_lyrics[j].line3, lyrics);
			memset(lyrics, 0, 40*sizeof(char));
			reader_txt_get_line(false, lyrics, 40);
			strcpy(song_lyrics[j].line4, lyrics);
			memset(lyrics, 0, 40*sizeof(char));
			reader_txt_get_line(false, lyrics, 40);
			strcpy(song_lyrics[j].line5, lyrics);
			memset(lyrics, 0, 40*sizeof(char));
			reader_txt_get_line(false, lyrics, 40);
			strcpy(song_lyrics[j].line6, lyrics);
			memset(lyrics, 0, 40*sizeof(char));
			
			// Close the file.
			reader_txt_close();
			
			print_dbg(song_lyrics[j].line1);
			print_dbg(song_lyrics[j].line2);
			print_dbg(song_lyrics[j].line3);
			print_dbg(song_lyrics[j].line4);
			print_dbg(song_lyrics[j].line5);
			print_dbg(song_lyrics[j].line6);
			
			print_dbg("\r\n");
			
		}
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
	
	/***	Retrieve Audio Info		****/

	/* Filter List ".h" */
	nav_filelist_reset();
	nav_filterlist_setfilter("h");
	nav_filterlist_root();
	nav_filterlist_goto( 0 ); // System volume information
	//sd.number_of_audio_files = nav_filterlist_nb(FS_FILE, "h");
	sd.number_of_audio_files = (sd.number_of_files / 3);
	print_dbg_ulong(sd.number_of_audio_files);
	et_data.pages_available = sd.number_of_audio_files / 4;
	if (sd.number_of_files % 4 != 0)
	{
		et_data.pages_available++;
	}
	//print_dbg_ulong(et_data.pages_available);
	
	/* Filter List ".h" */
	files = 0;
	nav_filelist_reset();
	nav_filterlist_setfilter("h");
	nav_filterlist_root();
	nav_filterlist_goto( 0 ); // System volume information
	print_dbg("Audio Files: \r\n");
	for(size_t i = 0; i < sd.number_of_audio_files; i++)
	{
		nav_filterlist_next();
		nav_file_getname(sd.name_of_audio_files[i], 30);
		print_dbg(sd.name_of_audio_files[i]);
		print_dbg("\r\n");
		files++;
		vTaskDelay(pdMS_TO_TICKS(100));
	}
	if (files == sd.number_of_audio_files)
	{
		print_dbg("Number of files coincide.\r\n");
	}
	else
	{
		print_dbg("Number of files does not coincide.\r\n");
	}

#if AUDIO
	/* Filter List ".h" */
	nav_filelist_reset();
	nav_filterlist_setfilter("h");
	nav_filterlist_root();
	nav_filterlist_goto( 0 ); // System volume information
	for(size_t i = 0; i < sd.number_of_audio_files; i++) // Loop through all
	{
		//nav_filelist_set(sd.drive_number, FS_FIND_NEXT);
		nav_filterlist_next();
		nav_file_getname(sd.name_of_audio_files[i], 30);
		print_dbg(sd.name_of_audio_files[i]);
		print_dbg("\r\n");
		//vTaskDelay(pdMS_TO_TICKS(2000));
		
		sd.audio_data[i].init_ptr = sdram_ptr; /* Audio data */
		
		/* Local declarations */
		sdram_udata_t data_sd;
		portBASE_TYPE notificationValue = 0;
		uint8_t word_complete = 0;
		
		nav_checkdisk_disable();   // To optimize speed
		
		file_open(FOPEN_MODE_R);
		
		while (!file_eof())
		{
			char current_char = file_getc();
			// Search for size fist, by looking for '[' and ']'
			if (current_char == '[')
			{
				char size_of_song[9] = "";
				current_char = file_getc();
				while( current_char != ']' ){
					strncat(size_of_song, &current_char, 1);
					current_char = file_getc();
				}
				sd.audio_data[i].size_in_bytes = a2ul(size_of_song); /* Audio data */
				print_dbg("\r\nSize of song in bytes:");
				print_dbg_ulong(sd.audio_data[i].size_in_bytes);
				print_dbg("\r\n");
			}
			// Search for the start of the next hex number
			else if (current_char == '0')
			{
				char hex_byte[] = "";
				strncat(hex_byte, &current_char, 1);
				
				for (uint8_t i = 0; i < 3; i++)				// Append next for 3 characters to get the byte in the form of 0x00
				{
					current_char = file_getc();
					strncat(hex_byte, &current_char, 1);
				}
				
				uint8_t data_byte = x2u8(hex_byte);			// Cast hex string to uint8_t
				data_sd.byte[word_complete] = data_byte;	// SDRAM data type
				word_complete++;
				
				if (word_complete == 4)
				{
					while(!(notificationValue > 0))
					{
						notificationValue = ulTaskNotifyTake( pdTRUE, (TickType_t) 1 );
					}
					word_complete = 0;
					xQueueSend( sdramQueue , &data_sd.word, (TickType_t) 1);
					vTaskResume( sdramHandle );
					memset(&data_sd, 0, sizeof(data_sd));
					//vTaskDelay(pdMS_TO_TICKS(100));
				}
			}
			//vTaskDelay(pdMS_TO_TICKS(100));
		}
		
		// Send the remaining data that didn't fill a word
		if (word_complete != 0)
		{
			while(!(notificationValue > 0))
			{
				notificationValue = ulTaskNotifyTake( pdTRUE, (TickType_t) 1 );
			}
			word_complete = 0;
			xQueueSend( sdramQueue , &data_sd.word, (TickType_t) 1);
			vTaskResume( sdramHandle );
			memset(&data_sd, 0, sizeof(data_sd));
		}
		
		vTaskDelay(pdMS_TO_TICKS(10));
		sd.audio_data[i].end_ptr = sdram_ptr; /* Audio data */
		
		// Close the file.
		file_close();
		print_dbg("DONE WITH FILE, SAMPLES IN BYTES: ");
		print_dbg_ulong((sd.audio_data[i].end_ptr - sd.audio_data[i].init_ptr) * 4);
		print_dbg("\r\n");
		
		nav_checkdisk_enable();
	}
#endif

	print_dbg("DONE");
	nav_exit();	// FS Closed
	

	xTaskCreate(qtButtonTask,  "tQT",        256,  (void *) 0, mainCOM_TEST_PRIORITY, &qtHandle);
	xTaskCreate(playAudioTask, "tPlayAudio", 2048, (void *) 0, mainLED_TASK_PRIORITY, &audioHandle);
	xTaskCreate(etTask,		   "tET",		 512,  (void *) 0, mainLED_TASK_PRIORITY, &etHandle);
	xTaskCreate(tftTask,	   "tTFT",		 256,  (void *) 0, mainCOM_TEST_PRIORITY, &tftHandle);
	
	vTaskDelete(sdramHandle);
	vTaskDelete(NULL);
	
}

// sdramHandle
portTASK_FUNCTION( sdramTask, p )
{
	volatile unsigned long *sdram = SDRAM;
	UBaseType_t sample = 0;
	
	// Calculate SDRAM size in words (32 bits).
	sdram_size = SDRAM_SIZE >> 2;
	print_dbg("\x0CSDRAM size in bytes: ");
	print_dbg_ulong(sdram_size*4);
	print_dbg("\r\n");

	//print_dbg("Suspending task");
	xTaskNotifyGive(fsHandle);
	vTaskSuspend(NULL);
	
	while(1)
	{
		if (sdramQueue != 0)
		{
			if (xQueueReceive( sdramQueue, &sample, (TickType_t) 2 ))
			{
				sdram[sdram_ptr++] = sample;
				xTaskNotifyGive(fsHandle);
				vTaskSuspend(NULL);
			}
		}
	}
}

// qtHandle
portTASK_FUNCTION( qtButtonTask, p )
{
	gpio_set_gpio_pin(LED0_GPIO);
	gpio_set_gpio_pin(LED1_GPIO);
	gpio_set_gpio_pin(LED2_GPIO);
	gpio_set_gpio_pin(LED3_GPIO);

	static uint16_t samplesToMove;
	static uint8_t lrValue    = 0;
	static uint8_t udValue    = 0;
	static int8_t  volume_ud  = 0;
	static bool    init_gui   = false;
	static uint8_t tft_square = 0;
	static bool	   tft_touch  = false;
	
	forwardQueue  = xQueueCreate( 1 , sizeof(uint16_t));
	reverseQueue  = xQueueCreate( 1 , sizeof(uint16_t));
	udQueue		  = xQueueCreate( 1 , sizeof(uint8_t));
	lrQueue       = xQueueCreate( 1 , sizeof(uint8_t));
	initBoolQueue = xQueueCreate( 1 , sizeof(bool));
	volumeUdQueue = xQueueCreate( 1 , sizeof(int8_t));
	etToggleQueue = xQueueCreate( 1 , sizeof(bool));

	while (1)
	{
		vTaskSuspend(NULL); // Suspend itself at start, remain there and wait for an external event to resume it.
		
		if (tftQueue != 0)
		{
			if (xQueueReceive( tftQueue, &tft_square, (TickType_t) 2 ))
			{
				tft_touch = true;
				lrValue = (tft_square % 2 == 0) ? 0 : 1;
				udValue = (tft_square > 1) ? 1 : 0;
			}
		}
		
		switch(state)
		{
			default:
			case MAIN:
				if (INTC_QT_FLAG._left) {
					INTC_QT_FLAG._left = false;
					lrValue = (lrValue > 0) ? lrValue - 1 : lrValue;
					xQueueSend( lrQueue, &lrValue, (TickType_t) 0);
					vTaskResume(etHandle);
				}
				else if (INTC_QT_FLAG._right) {
					INTC_QT_FLAG._right = false;
					lrValue = (lrValue + 1 < 2) ? lrValue + 1 : lrValue;
					xQueueSend( lrQueue, &lrValue, (TickType_t) 0);
					vTaskResume(etHandle);
				}
				else if (INTC_QT_FLAG._up) {
					INTC_QT_FLAG._up = false;
					//udValue = (udValue > 0) ? udValue - 1 : udValue;
					if (udValue == 0)
					{
						if (et_data.actual_page > 0)
						{
							et_data.actual_page--;
							xTaskNotifyGive( etHandle );
							udValue = 1;
						}
					}
					else
					{
						udValue--;
					}
					xQueueSend( udQueue, &udValue, (TickType_t) 0);
					vTaskResume(etHandle);
				}
				else if (INTC_QT_FLAG._down) {
					INTC_QT_FLAG._down = false;
					//udValue = (udValue + 1 <= 2) ? udValue + 1 : udValue;
					if (udValue == 1)
					{
						if (et_data.actual_page < et_data.pages_available - 1)
						{
							et_data.actual_page++;
							xTaskNotifyGive( etHandle );
							udValue = 0;
						}
					}
					else
					{
						udValue++;
					}
					xQueueSend( udQueue, &udValue, (TickType_t) 0);
					vTaskResume(etHandle);
				}
				else if (INTC_QT_FLAG._enter) {
					INTC_QT_FLAG._enter = false;
					// Change state
					state = REPRODUCIR;
					// Reproduce song
					// Check first selected song and update pointer
					// selected_song = et_data.actual_page * 4 + (2*udValue) + lrValue;
					if (selected_song != (et_data.actual_page * 4 + (2*udValue) + lrValue))
					{
						selected_song = et_data.actual_page * 4 + (2*udValue) + lrValue;
						vTaskSuspend( audioHandle );
						sdram_song_ptr = sd.audio_data[selected_song].init_ptr;
						vTaskResume( audioHandle );
					}
					// Reset Values (Or set) Put into play
					udValue = 2;
					lrValue = 1;
					init_gui = true;
					// Send to queue those values
					xQueueSend( lrQueue, &lrValue, (TickType_t) 0);
					xQueueSend( udQueue, &udValue, (TickType_t) 0);
					xQueueSend( initBoolQueue, &init_gui, (TickType_t) 0);
					init_gui = false;
					// Resume etTask
					vTaskSuspend( tftHandle );
					vTaskResume( etHandle );
					//xTaskNotifyGive(audioHandle);
				}
				else if (tft_touch)
				{
					xQueueSend( lrQueue, &lrValue, (TickType_t) 0);
					xQueueSend( udQueue, &udValue, (TickType_t) 0);
					xQueueSend( etToggleQueue, &tft_touch, (TickType_t) 0);
					tft_touch = false;
					vTaskResume(etHandle);
				}
				
			case REPRODUCIR:
				if (INTC_QT_FLAG._left) {
					INTC_QT_FLAG._left = false;
					lrValue = (lrValue > 0) ? lrValue - 1 : lrValue;
					xQueueSend( lrQueue, &lrValue, (TickType_t) 0);
					vTaskResume(etHandle);
				}
				else if (INTC_QT_FLAG._right) {
					INTC_QT_FLAG._right = false;
					lrValue = (lrValue + 1 < 3) ? lrValue + 1 : lrValue;
					xQueueSend( lrQueue, &lrValue, (TickType_t) 0);
					vTaskResume(etHandle);
				}
				else if (INTC_QT_FLAG._up) {
					INTC_QT_FLAG._up = false;
					udValue = (udValue > 0) ? udValue - 1 : udValue;
					xQueueSend( udQueue, &udValue, (TickType_t) 0);
					vTaskResume(etHandle);
				}
				else if (INTC_QT_FLAG._down) {
					INTC_QT_FLAG._down = false;
					udValue = (udValue + 1 < 3) ? udValue + 1 : udValue;
					xQueueSend( udQueue, &udValue, (TickType_t) 0);
					vTaskResume(etHandle);
				}
				else if (INTC_QT_FLAG._enter) {
					INTC_QT_FLAG._enter = false;
					switch(reproduce_option)
					{
						case LYRICS_GUI:
							state = LYRICS;
							init_gui = true;
							xQueueSend( initBoolQueue, &init_gui, (TickType_t) 0);
							init_gui = false;
							vTaskResume(etHandle);
							break;
							
						default:
						case MAIN_GUI:
							state = MAIN;
							udValue = 0;
							lrValue = 0;
							//et_data.actual_page = 0;
							init_gui = true;
							xQueueSend( udQueue, &udValue, (TickType_t) 0);
							xQueueSend( lrQueue, &lrValue, (TickType_t) 0);
							xQueueSend( initBoolQueue, &init_gui, (TickType_t) 0);
							init_gui = false;
							vTaskResume(etHandle);
							vTaskResume(tftHandle);
							// Adjust volume to 0 or leave it as previous
							break;
							
						case VOLUME_GUI:
							state = VOLUME;
							udValue = 0;
							lrValue = 0;
							init_gui = true;
							xQueueSend( udQueue, &udValue, (TickType_t) 0);
							xQueueSend( lrQueue, &lrValue, (TickType_t) 0);
							xQueueSend( initBoolQueue, &init_gui, (TickType_t) 0);
							init_gui = false;
							vTaskResume(etHandle);
							break;
							
						case PLAY:
							xTaskNotifyGive(audioHandle);
							//print_dbg("Playing song: ");
							//print_dbg(sd.name_of_audio_files[selected_song]);
							//print_dbg("Num: ");
							//print_dbg_ulong(selected_song);
							//print_dbg("\r\n");
							break;
							
						case FORWARD:
							//samplesToMove = 4096;
							//xQueueSend( reverseQueue, &samplesToMove, (TickType_t) 0 );
							
							selected_song = (selected_song + 1 < 8) ? selected_song + 1 : selected_song;
							vTaskSuspend( audioHandle );
							sdram_song_ptr = sd.audio_data[selected_song].init_ptr;
							vTaskResume( audioHandle );
							init_gui = true;
							xQueueSend( initBoolQueue, &init_gui, (TickType_t) 0);
							init_gui = false;
							vTaskResume(etHandle);
							// Adjust time with this
							break;
							
						case REVERSE:
							//samplesToMove = 4096;
							//xQueueSend( reverseQueue, &samplesToMove, (TickType_t) 0 );
							
							selected_song = (selected_song - 1 >= 0) ? selected_song - 1 : selected_song;
							vTaskSuspend( audioHandle );
							sdram_song_ptr = sd.audio_data[selected_song].init_ptr;
							vTaskResume( audioHandle );
							init_gui = true;
							xQueueSend( initBoolQueue, &init_gui, (TickType_t) 0);
							init_gui = false;
							vTaskResume( etHandle );
							
							// Adjust time with this
							break;
					}
				}
				break;
				
			case LYRICS:
				if (INTC_QT_FLAG._enter) {
					INTC_QT_FLAG._enter = false;
					state = REPRODUCIR;
					udValue = 0;
					lrValue = 0;
					init_gui = true;
					xQueueSend( lrQueue, &lrValue, (TickType_t) 0);
					xQueueSend( udQueue, &udValue, (TickType_t) 0);
					xQueueSend( initBoolQueue, &init_gui, (TickType_t) 0);
					init_gui = false;
					vTaskResume(etHandle);
				}
				break;
				
			case VOLUME:
				if (INTC_QT_FLAG._left) {
					INTC_QT_FLAG._left = false;
					lrValue = (lrValue == 1) ? 0 : 1;
					xQueueSend( lrQueue, &lrValue, (TickType_t) 0);
					vTaskResume(etHandle);
				}
				else if (INTC_QT_FLAG._right) {
					INTC_QT_FLAG._right = false;
					lrValue = (lrValue == 0) ? 1 : 0;
					xQueueSend( lrQueue, &lrValue, (TickType_t) 0);
					vTaskResume(etHandle);
				}
				else if (INTC_QT_FLAG._up) {
					INTC_QT_FLAG._up = false;
					if (lrValue == 1)
					{
						//udValue = (udValue + 1 <= 6) ? udValue + 1 : udValue;
						//xQueueSend( udQueue, &udValue, (TickType_t) 0);
						volume_ud = 1;
						xQueueSend( volumeUdQueue, &volume_ud, (TickType_t) 0);
						volume_ud = 0;
						vTaskResume(etHandle);
					}
				}
				else if (INTC_QT_FLAG._down) {
					INTC_QT_FLAG._down = false;
					if (lrValue == 1)
					{
						//udValue = (udValue > 0) ? udValue - 1 : udValue;
						//xQueueSend( udQueue, &udValue, (TickType_t) 0);
						volume_ud = -1;
						xQueueSend( volumeUdQueue, &volume_ud, (TickType_t) 0);
						volume_ud = 0;
						vTaskResume(etHandle);
					}
				}
				else if (INTC_QT_FLAG._enter) {
					INTC_QT_FLAG._enter = false;
					if (lrValue == 0)
					{
						state = REPRODUCIR;
						udValue = 2;
						lrValue = 1;
						init_gui = true;
						xQueueSend( lrQueue, &lrValue, (TickType_t) 0);
						xQueueSend( udQueue, &udValue, (TickType_t) 0);
						xQueueSend( initBoolQueue, &init_gui, (TickType_t) 0);
						init_gui = false;
						vTaskResume(etHandle);
					}
				}
				break;
		} // switch
	} // while
} // function

// audioHandle
portTASK_FUNCTION( playAudioTask, p )
{
	volatile unsigned long *sdram = SDRAM;
	print_dbg("Running audio...\r\n");
	static uint32_t count = 0;
	//uint32_t i = sd.audio_data[selected_song].init_ptr;
	static portBASE_TYPE notificationValue = 0;
	//static bool playAudio = false;
	static bool notify	  = false;
	static uint16_t samplesRx;
	static sdram_udata_t data;
	
	tpa6130_set_volume(volume); // 2F

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
					sdram_song_ptr = ( (sdram_song_ptr + samplesRx) <= sizeof(sd.audio_data[selected_song].end_ptr) ) ? (sdram_song_ptr + samplesRx) : sdram_song_ptr;
				}
			}

			//if (reverseQueue != 0)
			//{
				//if (xQueueReceive( reverseQueue, &samplesRx, (TickType_t) 5 ))
				//{
					//sdram_song_ptr = ((sd.audio_data[selected_song].end_ptr - sdram_song_ptr) > samplesRx) ? (sdram_song_ptr - samplesRx) : sdram_song_ptr;
				//}
			//}
		}

		if (playAudio)
		{
			playAudio = false;
			count = 0;
			// Store sample from the sound_table array
			while(count < (SOUND_SAMPLES)){
				memset(&data, 0, sizeof(data));
				data.word = sdram[sdram_song_ptr];
				for (uint8_t j = 0; j < 4; j++)
				{	
					samples[count++] = ((uint8_t)data.byte[j]+0x80) << 8;
					samples[count++] = ((uint8_t)data.byte[j]+0x80) << 8;
				}
				sdram_song_ptr++;
				if (sdram_song_ptr >= (sd.audio_data[selected_song].end_ptr))
				{
					playAudio = false;
					notify = !notify;
					sdram_song_ptr = sd.audio_data[selected_song].init_ptr;
				}
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

// ethHandle
portTASK_FUNCTION( etTask, p )
{
	static portBASE_TYPE notif_chaneg_page = 0;
	static bool change_page = false;
	static bool init_gui = false;
	static bool toggle = false;
	
	while (1)
	{
		if ( initBoolQueue != 0)
		{
			if (xQueueReceive( initBoolQueue, &init_gui, (TickType_t) 1 ))
			{
			}
		}
		
		switch(state)
		{
			default:
			case MAIN:
				notif_chaneg_page = ulTaskNotifyTake( pdTRUE, (TickType_t) 1 );
				if (notif_chaneg_page > 0)
				{
					change_page = true;
				}
				if ( etToggleQueue != 0)
				{
					if (xQueueReceive( etToggleQueue, &toggle, (TickType_t) 1 ))
					{
					}
				}
				menu_gui(init_gui, change_page, toggle);
				init_gui    = false;
				change_page = false;
				if (toggle)
				{
					toggle = false;
					vTaskResume( tftHandle );
				}
				break;
				
			case REPRODUCIR:
				rep_menu(init_gui);
				init_gui	= false;
				break;
				
			case LYRICS:
				lyrics_gui(init_gui);
				init_gui = false;
				break;
				
			case VOLUME:
				volumen_gui(init_gui);
				init_gui = false;
				break;
		}
		
		vTaskSuspend(NULL);
	}
}

// tftHandle
portTASK_FUNCTION( tftTask,	p )
{
	static filter_t x;
	static filter_t y;
	static int32_t  xytotal;
	static uint8_t square;
	static bool	   pressed;
	
	memset(&x, 0, sizeof(x));
	memset(&y, 0, sizeof(y));
	
	//print_dbg("\r\n TFT TASK\r\n");
	
	tftQueue = xQueueCreate( 1 , sizeof(uint8_t));
	
	while(1)
	{
		switch(state)
		{
			case MAIN:
				get_XY();
				xytotal = moving_filter(&x, &y);
				//print_dbg("DETECTADO, COORDS:");
				//print_dbg("\r\n");
				//print_dbg_ulong(x.vtotal);
				//print_dbg("\r\n");
				//print_dbg_ulong(y.vtotal);
				if(x.vtotal <= 10 && y.vtotal <= 10 && pressed != true && playAudio == false) // Check playAudio to enable this and avoid weird sounds.
				{
					pressed = true;
					if (x_touch == 52 && y_touch == 40)
					{
						// Upper Left
						square = 0;
						xQueueSend( tftQueue, &square, (TickType_t) 0);
						vTaskResume( qtHandle );
						vTaskSuspend( NULL );
					}
					else if (x_touch == 257 && y_touch == 40)
					{
						// Upper right
						square = 1;
						xQueueSend( tftQueue, &square, (TickType_t) 0);
						vTaskResume( qtHandle );
						vTaskSuspend( NULL );
					}
					else if (x_touch == 52 && y_touch == 194)
					{
						// Lower Left
						square = 2;
						xQueueSend( tftQueue, &square, (TickType_t) 0);
						vTaskResume( qtHandle );
						vTaskSuspend( NULL );
					}
					else if (x_touch == 257 && y_touch == 194)
					{
						// Lower right
						square = 3;
						xQueueSend( tftQueue, &square, (TickType_t) 0);
						vTaskResume( qtHandle );
						vTaskSuspend( NULL );
					}
				}
				else if (x.vtotal > 10 && y.vtotal > 10 && pressed == true)
				{
					pressed = false;
				}
				break;
			default:
				//vTaskSuspend( NULL );
				break;	
		}
		vTaskDelay(pdMS_TO_TICKS(5));
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

/**********  TFT - TOUCH  ***********/
static void get_XY(void)
{
	/* X Coordinate */
	gpio_enable_gpio_pin(TL);	// Reinitialize pins
	gpio_enable_gpio_pin(TR);
	gpio_set_gpio_pin(TT);		// Screen polarization
	gpio_clr_gpio_pin(TB);
	cpu_delay_ms( 1, PBA_HZ);
	

	adc_enable( adc , 0 );
	cpu_delay_ms( 1, PBA_HZ );
	adc_start( adc );
	x_touch = adc_get_value( adc , 0 )*.4;	// Reading and linearization
	adc_disable( adc, 0 );
	cpu_delay_ms( 1, PBA_HZ );
	
	/* Y Coordinate */
	gpio_enable_gpio_pin(TT);	// Reinitialize pins
	gpio_enable_gpio_pin(TB);
	gpio_set_gpio_pin(TL);		// Screen polarization
	gpio_clr_gpio_pin(TR);
	
	adc_enable( adc, 2 );
	cpu_delay_ms( 1, PBA_HZ );
	adc_start(&AVR32_ADC);
	y_touch = adc_get_value( adc, 2 )*.3;	// Reading and linearization
	adc_disable( adc, 2 );
	
	// Offset elimination
	y_touch = 270 - y_touch;
	x_touch = x_touch - 50;
}

static int32_t moving_filter(filter_t* x, filter_t* y)
{
	y->v4  = y->v3;
	y->v3  = y->v2;
	y->v2  = y->v1;
	y->v1 = y_touch;
	
	x->v4 = x->v3;
	x->v3 = x->v2;
	x->v2 = x->v1;
	x->v1 = x_touch;
	
	/***** X ****/
	if(x->v4 > x->v3){
		x->v43 = x->v4 - x->v3;
	}
	else{
		x->v43 = x->v3 - x->v4;
	}
	
	if(x->v3 > x->v2){
		x->v32 = x->v3 - x->v2;
	}
	else{
		x->v32 = x->v2 - x->v3;
	}
	
	if(x->v2 > x->v1){
		x->v21 = x->v2 - x->v1;
	}
	else{
		x->v21 = x->v1 - x->v2;
	}

	/***** X ****/
	if(y->v4 - y->v3){
		y->v43 = y->v4 - y->v3;
	}
	else{
		y->v43 = y->v3 - y->v4;
	}
	if(y->v3 > y->v2){
		y->v32 = y->v3 - y->v2;
	}
	else{
		y->v32 = y->v2 - y->v3;
	}
	
	if(y->v2 > y->v1){
		y->v21 = y->v2 - y->v1;
	}
	else{
		y->v21 = y->v1 - y->v2;
	}
	
	y->vtotal = y->v43 + y->v32 + y->v21;
	x->vtotal = x->v43 + x->v32 + x->v21;
	
	return (x->vtotal + y->vtotal);

}

/***************   MAIN   ***************/
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

	tpa6130_set_volume(volume); // 2F
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

	et024006_DrawFilledRect(0, 0, ET024006_WIDTH, ET024006_HEIGHT, BLACK);

	while(pwm_channel6.cdty < pwm_channel6.cprd)
	{
		pwm_channel6.cdty++;
		pwm_channel6.cupd = pwm_channel6.cdty;
		//pwm_channel6.cdty--;
		pwm_async_update_channel(AVR32_PWM_ENA_CHID6, &pwm_channel6);
		delay_ms(10);
	}
	
	et024006_PrintString("BIENVENIDO AL REPRODUCTOR", (const unsigned char *) &FONT8x16, 70, 110, WHITE, -1);
	et024006_PrintString("CARGANDO...", (const unsigned char *) &FONT8x16, 90, 140, WHITE, -1);
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

	if (lrQueue != 0)
	{
		if (xQueueReceive( lrQueue, &keys.lr, (TickType_t) 2 ))
		{
			//print_dbg("Received for REP LR");
			//print_dbg_ulong(keys.lr);
		}
	}
	
	if (udQueue != 0)
	{
		if (xQueueReceive( udQueue, &keys.ud, (TickType_t) 2 ))
		{
			//print_dbg("Received for REP UD");
			//print_dbg_ulong(keys.ud);
		}
	}

	if (first_time || init)
	{
		first_time = false;
		//memset(&keys, 0, sizeof(keys));

		et024006_DrawFilledRect(0, 0, 320, 240, BLACK);
		et024006_PrintString("Lyrics", (const unsigned char *) &FONT8x16, 10, 10, WHITE, -1);
		et024006_PrintString("Main",   (const unsigned char *) &FONT8x16, 120, 10, WHITE, -1);
		et024006_PrintString("Volume", (const unsigned char *) &FONT8x16, 220, 10, WHITE, -1);

		et024006_PutPixmap(song_image[selected_song], 70, 0, 0, 120, 70, 70, 70);
		et024006_PrintString(song_info[selected_song].name, (const unsigned char *) &FONT8x16, 110, 160, WHITE, -1); // Ajustar el nombre de la cancion dependiendo la cancion guardada.

		et024006_PutPixmap(atrasarblanco, 50, 0, 0, 10, 190, 50, 50);
		et024006_PutPixmap(play, 50, 0, 0, 135, 190, 50, 50);
		et024006_PutPixmap(adelantar, 50, 0, 0, 260, 190, 50, 50);
	}

	if (keys.ud == 0 && keys.lr == 0){
		et024006_PrintString("Lyrics", (const unsigned char*) &FONT8x16, 10, 10, GREEN, -1);
		et024006_PrintString("Main", (const unsigned char*) &FONT8x16, 120, 10, WHITE, -1);
		et024006_PrintString("Volume", (const unsigned char*) &FONT8x16, 220, 10, WHITE, -1);
		et024006_PutPixmap(atrasarblanco, 50, 0, 0, 10, 190, 50, 50);
		et024006_PutPixmap(play, 50, 0, 0, 135, 190, 50, 50);
		et024006_PutPixmap(adelantar, 50, 0, 0, 260, 190, 50, 50);
		reproduce_option = LYRICS_GUI;
	}
	else if (keys.ud == 0 && keys.lr == 1){
		et024006_PrintString("Lyrics", (const unsigned char*) &FONT8x16, 10, 10, WHITE, -1);
		et024006_PrintString("Main", (const unsigned char*) &FONT8x16, 120, 10, GREEN, -1);
		et024006_PrintString("Volume", (const unsigned char*) &FONT8x16, 220, 10, WHITE, -1);
		et024006_PutPixmap(atrasarblanco, 50, 0, 0, 10, 190, 50, 50);
		et024006_PutPixmap(play, 50, 0, 0, 135, 190, 50, 50);
		et024006_PutPixmap(adelantar, 50, 0, 0, 260, 190, 50, 50);
		reproduce_option = MAIN_GUI;
	}
	else if (keys.ud == 0 && keys.lr == 2){
		et024006_PrintString("Lyrics", (const unsigned char*) &FONT8x16, 10, 10, WHITE, -1);
		et024006_PrintString("Main", (const unsigned char*) &FONT8x16, 120, 10, WHITE, -1);
		et024006_PrintString("Volume", (const unsigned char*) &FONT8x16, 220, 10, GREEN, -1);
		et024006_PutPixmap(atrasarblanco, 50, 0, 0, 10, 190, 50, 50);
		et024006_PutPixmap(play, 50, 0, 0, 135, 190, 50, 50);
		et024006_PutPixmap(adelantar, 50, 0, 0, 260, 190, 50, 50);
		reproduce_option = VOLUME_GUI;
	}
	//else if (keys.ud == 2 && keys.lr == 0){
		//et024006_PutPixmap(song_image[3], 70, 0, 0, 120, 70, 70, 70);
		//et024006_PrintString("cancion2", (const unsigned char*) &FONT8x16, 120, 170, WHITE, -1);
//
	//}
	else if(keys.ud == 2 && keys.lr == 0){
		et024006_PrintString("Lyrics", (const unsigned char*) &FONT8x16, 10, 10, WHITE, -1);
		et024006_PrintString("Main", (const unsigned char*) &FONT8x16, 120, 10, WHITE, -1);
		et024006_PrintString("Volume", (const unsigned char*) &FONT8x16, 220, 10, WHITE, -1);
		et024006_PutPixmap(atrasarverde, 50, 0, 0, 10, 190, 50, 50);
		et024006_PutPixmap(play, 50, 0, 0, 135, 190, 50, 50);
		et024006_PutPixmap(adelantar, 50, 0, 0, 260, 190, 50, 50);
		reproduce_option = REVERSE;
	}
	else if (keys.ud == 2 && keys.lr == 1){
		et024006_PrintString("Lyrics", (const unsigned char*) &FONT8x16, 10, 10, WHITE, -1);
		et024006_PrintString("Main", (const unsigned char*) &FONT8x16, 120, 10, WHITE, -1);
		et024006_PrintString("Volume", (const unsigned char*) &FONT8x16, 220, 10, WHITE, -1);
		et024006_PutPixmap(atrasarblanco, 50, 0, 0, 10, 190, 50, 50);
		et024006_PutPixmap(playverde, 50, 0, 0, 135, 190, 50, 50);
		et024006_PutPixmap(adelantar, 50, 0, 0, 260, 190, 50, 50);
		reproduce_option = PLAY;
	}
	else if (keys.ud == 2 && keys.lr == 2){
		et024006_PrintString("Lyrics", (const unsigned char*) &FONT8x16, 10, 10, WHITE, -1);
		et024006_PrintString("Main", (const unsigned char*) &FONT8x16, 120, 10, WHITE, -1);
		et024006_PrintString("Volume", (const unsigned char*) &FONT8x16, 220, 10, WHITE, -1);
		et024006_PutPixmap(atrasarblanco, 50, 0, 0, 10, 190, 50, 50);
		et024006_PutPixmap(play, 50, 0, 0, 135, 190, 50, 50);
		et024006_PutPixmap(adelantarverde, 50, 0, 0, 260, 190, 50, 50);
		reproduce_option = FORWARD;
	}

}

static void menu_gui(bool init, bool change_page, bool toggle)
{
	static bool first_time	  = true;
	static bool change_dected = false;
	static bool change_string = false;
	static menu_keys_t keys;
	
	static uint16_t coords_x[2] = {0, 165};
	static uint16_t coords_y[2] = {0, 125};
	
	if (lrQueue != 0)
	{
		if (xQueueReceive( lrQueue, &keys.lr, (TickType_t) 2 ))
		{
			change_dected = true;
		}
	}
	
	if (udQueue != 0)
	{
		if (xQueueReceive( udQueue, &keys.ud, (TickType_t) 2 ))
		{
			change_dected = true;
		}
	}
	
	if (first_time || init || change_page || toggle)
	{
		et024006_DrawFilledRect(0, 0, 320, 240, BLACK);
		et024006_DrawVertLine(160,0,120,WHITE);
		et024006_DrawVertLine(160,120,120,WHITE);
	
		et024006_DrawHorizLine(0,120,160,WHITE);
		et024006_DrawHorizLine(160,120,160,WHITE);
	
		/* Place four images */
		// FIXME: Read images via SD card, and make this loopable
		uint8_t index_page = et_data.actual_page * 4;
		if (et_data.actual_page == 0)
		{
			et024006_PutPixmap(song_image[0], 70, 0, 0, 30, 20, 70, 70);
			et024006_PrintString(song_info[index_page].duration, (const unsigned char *) &FONT8x8, 30, 100, WHITE, -1);
			
			et024006_PutPixmap(song_image[1], 70, 0, 0, 190, 20, 70, 70);
			et024006_PrintString(song_info[index_page + 1].duration, (const unsigned char*) &FONT8x8, 190, 100, WHITE, -1);
			
			et024006_PutPixmap(song_image[2], 70, 0, 0, 30, 140, 70, 70);
			et024006_PrintString(song_info[index_page + 2].duration, (const unsigned char*) &FONT8x8, 30, 220, WHITE, -1);
			
			et024006_PutPixmap(song_image[3], 70, 0, 0, 190, 140, 70, 70);
			et024006_PrintString(song_info[index_page + 3].duration, (const unsigned char*) &FONT8x8, 190, 220, WHITE, -1);
		}
		else if (et_data.actual_page == 1)
		{
			et024006_PutPixmap(song_image[4], 70, 0, 0, 30, 20, 70, 70);
			et024006_PrintString(song_info[index_page].duration, (const unsigned char *) &FONT8x8, 30, 100, WHITE, -1);
			
			et024006_PutPixmap(song_image[5], 70, 0, 0, 190, 20, 70, 70);
			et024006_PrintString(song_info[index_page + 1].duration, (const unsigned char*) &FONT8x8, 190, 100, WHITE, -1);
			
			et024006_PutPixmap(song_image[6], 70, 0, 0, 30, 140, 70, 70);
			et024006_PrintString(song_info[index_page + 2].duration, (const unsigned char*) &FONT8x8, 30, 220, WHITE, -1);
			
			et024006_PutPixmap(song_image[7], 70, 0, 0, 190, 140, 70, 70);
			et024006_PrintString(song_info[index_page + 2].duration, (const unsigned char*) &FONT8x8, 190, 220, WHITE, -1);
		}
		
		uint8_t square = (keys.ud * 2) + keys.lr;
		
		if (toggle)
		{
			change_string = !change_string;
		}
		
		if (change_string)
		{
			et024006_DrawFilledRect(coords_x[keys.lr], coords_y[keys.ud], 150, 110, BLACK);
			et024006_PrintString(song_info[index_page + square].name, (const unsigned char*) &FONT8x8, coords_x[keys.lr]+20, coords_y[keys.ud]+10, WHITE, -1);
			et024006_PrintString(song_info[index_page + square].artist, (const unsigned char*) &FONT8x8, coords_x[keys.lr]+20, coords_y[keys.ud]+25, WHITE, -1);
			et024006_PrintString(song_info[index_page + square].album, (const unsigned char*) &FONT8x8, coords_x[keys.lr]+20, coords_y[keys.ud]+40, WHITE, -1);
			et024006_PrintString(song_info[index_page + square].year, (const unsigned char*) &FONT8x8, coords_x[keys.lr]+20, coords_y[keys.ud]+55, WHITE, -1);
		}
		
	}
	if (first_time || change_dected || init || toggle)
	{
		first_time = false;
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
			//et024006_PrintString("Let?s go outside", (const unsigned char*) &FONT8x8,190, 20, WHITE, -1);
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
	
	
}

static void volumen_gui(bool init)
{
	static bool first_time    = true;
	static bool change_dected = false;
	static bool update_volume = false;
	static menu_keys_t keys;
	static int8_t valueUd = 0;
	static uint8_t volume_val = 0;
	
	if (lrQueue != 0)///////////////
	{
		if (xQueueReceive( lrQueue, &keys.lr, (TickType_t) 2 ))
		{
			change_dected = true;
		}
	}
	
	if (udQueue != 0)
	{
		if (xQueueReceive( udQueue, &keys.ud, (TickType_t) 2 ))
		{
			change_dected = true;
		}
	}
	
	if (volumeUdQueue != 0)
	{
		if (xQueueReceive( volumeUdQueue, &valueUd, (TickType_t) 2 ))
		{
			update_volume = true;
		}
	}
	
	if (first_time || init)
	{
		et024006_DrawFilledRect(0, 0, 320, 240, BLACK);
		et024006_DrawFilledRect(180, 40, 30, 180, WHITE);
		et024006_DrawFilledRect(180, 220-(volume_val * 30), 30, (volume_val * 30), RED); // Update volume
		et024006_PrintString("Regresar", (const unsigned char *) &FONT8x16, 30, 115, WHITE, -1);
	}
	
	if (first_time || change_dected || init)
	{
		first_time = false;
		if (keys.lr == 0){
			et024006_DrawFilledRect(178, 38, 34, 184, BLACK);
			et024006_DrawFilledRect(180, 40, 30, 180, WHITE);
			et024006_DrawFilledRect(180, 220-(volume_val * 30), 30, (volume_val * 30), RED); // Update volume
			et024006_PrintString("Regresar", (const unsigned char *) &FONT8x16, 30, 115, GREEN, -1);
		}
		else if (keys.lr == 1){
			et024006_DrawFilledRect(178, 38, 34, 184, GREEN);
			et024006_DrawFilledRect(180, 40, 30, 180, WHITE);
			et024006_DrawFilledRect(180, 220-(volume_val * 30), 30, (volume_val * 30), RED); // Update volume
			et024006_PrintString("Regresar", (const unsigned char *) &FONT8x16, 30, 115, WHITE, -1);
			if (update_volume)
			{
				if ((valueUd > 0) && (volume_val < 6)) 
				{
					volume_val += valueUd;
				}
				else if ((valueUd < 0) && (volume_val > 0)) 
				{
					volume_val += valueUd;
				}
				volume = volume_val * (0x0A);
				et024006_DrawFilledRect(180, 40, 30, 180, WHITE);
				et024006_DrawFilledRect(180, 220-(volume_val * 30), 30, (volume_val * 30), RED); // Update volume
				//print_dbg_char_hex(volume);
				tpa6130_set_volume(volume);
			}
		}
		update_volume = false;
	}
	
	
}

static void lyrics_gui(bool init)
{
	static bool first_time    = true;
	static bool change_dected = false;
	static bool update_volume = false;
	static menu_keys_t keys;
	static int8_t valueUd = 0;
	
	if (first_time || init)
	{
		et024006_DrawFilledRect(0, 0, 320, 240, BLACK);
		et024006_PrintString("Regresar", (const unsigned char *) &FONT8x16, 130, 10, GREEN, -1);
		
		et024006_PrintString(song_lyrics[selected_song].line1, (const unsigned char *) &FONT8x8, 20, 40, WHITE, -1);
		et024006_PrintString(song_lyrics[selected_song].line2, (const unsigned char *) &FONT8x8, 20, 70, WHITE, -1);
		et024006_PrintString(song_lyrics[selected_song].line3, (const unsigned char *) &FONT8x8, 20, 100, WHITE, -1);
		et024006_PrintString(song_lyrics[selected_song].line4, (const unsigned char *) &FONT8x8, 20, 130, WHITE, -1);
		et024006_PrintString(song_lyrics[selected_song].line5, (const unsigned char *) &FONT8x8, 20, 160, WHITE, -1);
		et024006_PrintString(song_lyrics[selected_song].line6, (const unsigned char *) &FONT8x8, 20, 190, WHITE, -1);
	}
	
}

static void init_sdram(void)
{
	// Initialize the external SDRAM chip.
	sdramc_init(PBA_HZ);
	print_dbg("\r\nSDRAM initialized\r\n");
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

/*-----------------------------------------------------------*/
/*							 MAIN							 */
/*-----------------------------------------------------------*/

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
	//et024006_PrintString("****** INIT SD/MMC ******",(const unsigned char*)&FONT8x8,10,10,BLUE,-1);

	init_qt_interrupt();

	init_fs();
	
	init_sdram();

	print_dbg(MSG_WELCOME);

	/* Insert application code here, after the board has been initialized. */

	//xTaskCreate(qtButtonTask,  "tQT",        256,  (void *) 0, mainCOM_TEST_PRIORITY, &qtHandle);
	//xTaskCreate(playAudioTask, "tPlayAudio", 2048, (void *) 0, mainLED_TASK_PRIORITY, &audioHandle);
	//xTaskCreate(etTask,		   "tET",		 512,  (void *) 0, mainLED_TASK_PRIORITY, &etHandle);
	xTaskCreate(fsTask,		   "tFS",		 1024,  (void *) 0, mainLED_TASK_PRIORITY, &fsHandle);
	xTaskCreate(sdramTask,     "tSDRAM",	 256,  (void *) 0, mainLED_TASK_PRIORITY + 1, &sdramHandle);
	
	vTaskStartScheduler();

	while (1)
	{

	}

	return 0;

}

/*-----------------------------------------------------------*/
