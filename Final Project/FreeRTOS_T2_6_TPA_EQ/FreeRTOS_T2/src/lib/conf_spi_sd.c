/*
 * conf_spi_sd.c
 *
 * Created: 10/11/2020 14:27:14
 *  Author: Fernando Zaragoza
 */ 

#include "conf_spi_sd.h"

// SPI
void sd_mmc_resources_init(void)
{
	// GPIO pins used for SD/MMC interface
	static const gpio_map_t SD_MMC_SPI_GPIO_MAP =
	{
		{SD_MMC_SPI_SCK_PIN,  SD_MMC_SPI_SCK_FUNCTION },  // SPI Clock.
		{SD_MMC_SPI_MISO_PIN, SD_MMC_SPI_MISO_FUNCTION},  // MISO.
		{SD_MMC_SPI_MOSI_PIN, SD_MMC_SPI_MOSI_FUNCTION},  // MOSI.
		{SD_MMC_SPI_NPCS_PIN, SD_MMC_SPI_NPCS_FUNCTION}   // Chip Select NPCS.
	};

	// SPI options.
	spi_options_t spiOptions =
	{
		.reg          = SD_MMC_SPI_NPCS,
		.baudrate     = SD_MMC_SPI_MASTER_SPEED,  // Defined in conf_sd_mmc_spi.h.
		.bits         = SD_MMC_SPI_BITS,          // Defined in conf_sd_mmc_spi.h.
		.spck_delay   = 0,
		.trans_delay  = 0,
		.stay_act     = 1,
		.spi_mode     = 0,
		.modfdis      = 1
	};

	// Assign I/Os to SPI.
	gpio_enable_module(SD_MMC_SPI_GPIO_MAP,
	sizeof(SD_MMC_SPI_GPIO_MAP) / sizeof(SD_MMC_SPI_GPIO_MAP[0]));

	// Initialize as master.
	spi_initMaster(SD_MMC_SPI, &spiOptions);

	// Set SPI selection mode: variable_ps, pcs_decode, delay.
	spi_selectionMode(SD_MMC_SPI, 0, 0, 0);

	// Enable SPI module.
	spi_enable(SD_MMC_SPI);

	// Initialize SD/MMC driver with SPI clock (PBA).
	sd_mmc_spi_init(spiOptions, PBA_HZ);
	
}

void init_sd_spi_pdca(void)
{
	/* SPI - SD */
	// this PDCA channel is used for data reception from the SPI
	pdca_channel_options_t pdca_options_SPI_RX =
	{ // pdca channel options

		.addr		   = spi_rx_ram_buffer,		  // memory address. We take here the address of the string dummy_data. This string is located in the file dummy.h
		.size		   = SD_SECTOR_SIZE,   // transfer counter: here the size of the string
		.r_addr		   = NULL,                    // next memory address after 1st transfer complete
		.r_size		   = 0,                       // next transfer counter not used here
		.pid		   = AVR32_PDCA_PID_SPI0_RX,  // select peripheral ID - data are on reception from SPI1 RX line
		.transfer_size = PDCA_TRANSFER_SIZE_BYTE  // select size of the transfer: 8,16,32 bits
	};

	// this channel is used to activate the clock of the SPI by sending a dummy variables
	pdca_channel_options_t pdca_options_SPI_TX =
	{ // pdca channel options

		.addr		   = (void *)&spi_tx_ram_buffer, // memory address.
		.size		   = SD_SECTOR_SIZE,   // transfer counter: here the size of the string
		.r_addr		   = NULL,                    // next memory address after 1st transfer complete
		.r_size		   = 0,                       // next transfer counter not used here
		.pid		   = AVR32_PDCA_PID_SPI0_TX,  // select peripheral ID - data are on reception from SPI1 RX line
		.transfer_size = PDCA_TRANSFER_SIZE_BYTE  // select size of the transfer: 8,16,32 bits
	};


	// Init PDCA transmission channel
	pdca_init_channel(AVR32_PDCA_CHANNEL_SPI_RX, &pdca_options_SPI_RX); // AVR32_PDCA_CHANNEL_SPI_TX
	// Init PDCA Reception channel
	pdca_init_channel(AVR32_PDCA_CHANNEL_SPI_TX, &pdca_options_SPI_TX);
	
}

void check_sd_card(void)
{
	// NECESSARY: Initialize USART first.
	print_dbg("\r\nInit SD/MMC Driver");
	print_dbg("\r\nInsert SD/MMC...");
		
	while (!sd_mmc_spi_mem_check());
	print_dbg("\r\nCard detected!");
		
	// Read Card capacity
	sd_mmc_spi_get_capacity();
	print_dbg("Capacity = ");
	print_dbg_ulong(capacity >> 20);
	print_dbg(" MBytes");
}

void wait(void)
{
	volatile int i;
	for(i = 0 ; i < 5000; i++);
}

void ISR_pdca1_irq_97(void)
{
	// Disable all interrupts.
	Disable_global_interrupt();

	// Disable interrupt channel.
	pdca_disable_interrupt_transfer_complete(AVR32_PDCA_CHANNEL_SPI_RX);
	
	sd_mmc_spi_read_close_PDCA(); // unselects the SD/MMC memory.
	wait();
	// Disable unnecessary channel
	pdca_disable(AVR32_PDCA_CHANNEL_SPI_RX);
	
	// Enable all interrupts.
	Enable_global_interrupt();
	
	INTC_PDCA_FLAG.ch1 = true;
	end_of_rx_transfer = true;
}

void ISR_pdca2_irq_98(void)
{
	// Disable all interrupts.
	Disable_global_interrupt();

	// Disable interrupt channel.
	pdca_disable_interrupt_transfer_complete(AVR32_PDCA_CHANNEL_SPI_TX);
	// Disable unnecessary channel
	pdca_disable(AVR32_PDCA_CHANNEL_SPI_TX);
	
	// Enable all interrupts.
	Enable_global_interrupt();
	
	INTC_PDCA_FLAG.ch2 = true;
}