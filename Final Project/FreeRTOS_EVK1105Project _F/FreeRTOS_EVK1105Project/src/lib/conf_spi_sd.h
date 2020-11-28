/*
 * conf_spi_sd.h
 *
 * Created: 10/11/2020 14:26:57
 *  Author: Fernando Zaragoza
 */ 


#ifndef CONF_SPI_SD_H_
#define CONF_SPI_SD_H_

#include "pdca.h"
#include "usart.h"
#include "print_funcs.h"

#include "conf_sd_mmc_spi.h"
#include "conf_interrupt.h"
#include "conf_utils.h"

#define AVR32_PDCA_CHANNEL_SPI_RX		1
#define AVR32_PDCA_CHANNEL_SPI_TX		2
#define BUFFER_SIZE_SD_SPI_RX			6
#define BUFFER_SIZE_SD_SPI_TX			6
#define SD_SECTOR_SIZE					512

extern char spi_rx_ram_buffer[SD_SECTOR_SIZE];
extern char spi_tx_ram_buffer[SD_SECTOR_SIZE];

void sd_mmc_resources_init(void);
void init_sd_spi_pdca(void);
void check_sd_card(void);
void wait(void);
//__attribute__ ((__interrupt__)) void ISR_pdca1_irq_97(void);
//__attribute__ ((__interrupt__)) void ISR_pdca2_irq_98(void);

#endif /* CONF_SPI_SD_H_ */