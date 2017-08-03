/**
 * @file handle_instruction.c
 * @brief Function that handle incoming data message
 * @author Ran Bao (ran.bao@wirelessguard.co.nz)
 * @date March, 2017
 */

#include <stdint.h>
#include <string.h> // memset

#include "bootloader_api.h"
#include "em_msc.h"

/**
 * global variables
 */
extern bootloader_config_t bootloader_config;

/**
 * Callback function for handling received data packet
 * @param payload_index current payload index
 * @param data          data stripped from payload
 * @param size          the length of data
 */
void comm_cb_data(uint16_t block_offset, uint8_t *data, uint8_t size)
{
	// calculate absolute data address
	uint32_t addr = bootloader_config.base_addr + (uint32_t) block_offset * (1UL << bootloader_config.block_size_exp);

	// TODO: unlock and lock the flash memory after writing to device for better security

	// write to flash memory
	// the flash driver has its own internal page buffer
	MSC_WriteWord((uint32_t *)addr, data, size);
}
