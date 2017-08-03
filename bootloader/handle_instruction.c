/**
 * @file handle_instruction.c
 * @brief Function that handle incoming instruction message
 * @author Ran Bao (ran.bao@wirelessguard.co.nz)
 * @date March, 2017
 */
#include <stdint.h>
#include <string.h> // memset
#include <stdlib.h> // atoi
#include <assert.h>

#include "em_device.h"
#include "em_system.h"
#include "em_msc.h"

#include "bootloader_api.h"

#include "sip.h"
#include "dscrc8.h"

#include "bootloader_api.h"
#include "bootloader_config.h"
#include "communication.h"
#include "convert.h"
#include "stack32.h"


/**
 * global variables
 */
extern bootloader_config_t bootloader_config;

void inst_set_base_addr(const communication_t *comm, uint8_t *data, uint8_t size);
void inst_branch_to_addr(const communication_t *comm, uint8_t *data, uint8_t size);
void inst_push_base_addr(const communication_t *comm, uint8_t *data, uint8_t size);
void inst_pop_base_addr(const communication_t *comm, uint8_t *data, uint8_t size);
void inst_erase_page(const communication_t *comm, uint8_t *data, uint8_t size);
void inst_erase_range(const communication_t *comm, uint8_t *data, uint8_t size);
void inst_query_proto_ver(const communication_t *comm, uint8_t *data, uint8_t size);
void inst_reboot(const communication_t *comm, uint8_t *data, uint8_t size);
void inst_query_device_info(const communication_t *comm, uint8_t *data, uint8_t size);
void inst_query_chip_info(const communication_t *comm, uint8_t *data, uint8_t size);
void inst_get_base_addr(const communication_t *comm, uint8_t *data, uint8_t size);

/**
 * @brief Determine whether the given address falls
 * @param address given address
 * @return true if the address falls within either FLASH range or SRAM
 */
bool is_valid_address(uint32_t address);


/**
 * Callback function for handling received instruction packet
 *
 * On receiving an instruction packet, the instruction handler will read
 * instruction <- payload[0]
 *
 * instruction -> INST_SET_REGION:
 * 		region <- payload[1]
 * 		block size exponential <- payload[2]
 *
 * 		Note: the region is selected based on bootloader_region_t. The block size exponential is calculated by
 * 			block_size = 1 << block_size_exponential
 *
 * 	instruction -> INST_BRANCH_TO_REGION
 * 		region <- payload[1]
 *
 * 		Note: the instruction will be executed immediately without sending acknowledgement
 *
 * 	instruction -> INST_COPY_REGION
 * 		not implemented
 *
 * 	instruction -> INST_FLUSH
 * 		not implemented
 *
 * 	instruction -> INST_REBOOT (<203ffff0d>)
 * 		reboot to last boot partition, the information is supposed to be stored in persistent storage region
 *
 * 	instruction -> INST_QUERY_DEVICE_INFO
 * 		Note: The partition information will be encapsulated in a data packet, followed by the acknowledgement
 *
 * 	instruction -> INST_QUERY_CHIP_INFO
 * 		Note: the chip information will be encapsulated in a data packet, followed by the acknowledgement
 *
 * @param data data stripped from payload (without packet number)
 * @param size the length of data
 */
void comm_cb_inst(const void *handler, uint8_t *data, uint8_t size)
{
	// statically cast handler to communication_t
	const communication_t *comm = (communication_t *)handler;

	// read instruction from payload
	bootloader_instruction_t instruction;
	instruction = (bootloader_instruction_t) data[0];

	switch (instruction)
	{
		case INST_SET_BASE_ADDR:
		{
			inst_set_base_addr(comm, data + 1, (uint8_t )(size - 1));
			break;
		}
		case INST_BRANCH_TO_ADDR:
		{
			inst_branch_to_addr(comm, data + 1, (uint8_t )(size - 1));
			break;
		}
		case INST_PUSH_BASE_ADDR:
		{
			inst_push_base_addr(comm, data + 1, (uint8_t )(size - 1));
			break;
		}
		case INST_POP_BASE_ADDR:
		{
			inst_pop_base_addr(comm, data + 1, (uint8_t )(size - 1));
			break;
		}
		case INST_ERASE_PAGES:
		{
			inst_erase_page(comm, data + 1, (uint8_t )(size - 1));
			break;
		}
		case INST_ERASE_RANGE:
		{
			inst_erase_range(comm, data + 1, (uint8_t )(size - 1));
			break;
		}
		case INST_QUERY_PROTO_VER:
		{
			inst_query_proto_ver(comm, data + 1, (uint8_t )(size - 1));
			break;
		}
		case INST_REBOOT:
		{
			inst_reboot(comm, data + 1, (uint8_t )(size - 1));
			break;
		}
		case INST_QUERY_DEVICE_INFO:
		{
			inst_query_device_info(comm, data + 1, (uint8_t )(size - 1));
			break;
		}
		case INST_QUERY_CHIP_INFO:
		{
			inst_query_chip_info(comm, data + 1, (uint8_t )(size - 1));
			break;
		}
		case INST_GET_BASE_ADDR:
		{
			inst_get_base_addr(comm, data + 1, (uint8_t)(size - 1));
		}
		default:
			break;
	}
}

void inst_set_base_addr(const communication_t *comm, uint8_t *data, uint8_t size)
{
	assert(size == 4);

	uint32_t received_base_addr = 0xffffffff;

	// convert serial data to uint32
	uint8_to_uint32(data, &received_base_addr);

	// sanity check: make sure the base addr falls within flash or sram
	assert(is_valid_address(received_base_addr));

	// store base address
	bootloader_config.base_addr = received_base_addr;
}

void inst_branch_to_addr(const communication_t *comm, uint8_t *data, uint8_t size)
{
	// if there is no address to boot, then use pre-configured base address、
	if (size == 0)
	{
		reboot_to_addr(bootloader_config.base_addr, true);
	}

	// otherwise, use address specified in command
	else
	{
		assert(size == 4);

		uint32_t received_base_addr = 0xffffffff;

		// convert serial data to uint32
		uint8_to_uint32(data, &received_base_addr);

		// sanity check: make sure the base addr falls within flash or sram
		assert(is_valid_address(received_base_addr));

		// branch to address
		reboot_to_addr(received_base_addr, true);
	}
}

void inst_push_base_addr(const communication_t *comm, uint8_t *data, uint8_t size)
{
	assert(size == 0);

	// push current bootloader address to stack
	stack32_push(&(bootloader_config.base_addr_stack), bootloader_config.base_addr);
}

void inst_pop_base_addr(const communication_t *comm, uint8_t *data, uint8_t size)
{
	assert(size == 0);

	// pop previously pushed base address
	bootloader_config.base_addr = stack32_pop(&(bootloader_config.base_addr_stack));
}

void inst_erase_page(const communication_t *comm, uint8_t *data, uint8_t size)
{
	assert(size == 2);

	// convert 2 byte block array to uint16 value
	uint16_t pages_to_erase = 0;
	uint8_to_uint16(data, &pages_to_erase);

	// erase the page
	for (uint16_t page = 0; page < pages_to_erase; page++)
	{
		MSC_ErasePage((uint32_t *) (bootloader_config.base_addr + page * FLASH_PAGE_SIZE));
	}
}

void inst_erase_range(const communication_t *comm, uint8_t *data, uint8_t size)
{
	assert(size == 8);

	// read start and finish address
	uint32_t start_addr = 0;
	uint32_t end_addr = 0;

	uint8_to_uint32(data + 0, &start_addr);
	uint8_to_uint32(data + 4, &end_addr);

	assert((end_addr - start_addr) >= FLASH_PAGE_SIZE);

	// align the base address down to nearest flash page boundry
	start_addr = (start_addr / FLASH_PAGE_SIZE) * FLASH_PAGE_SIZE;

	// erase the page
	while (start_addr < end_addr)
	{
		MSC_ErasePage((uint32_t *) start_addr);
		start_addr += FLASH_PAGE_SIZE;
	}
}

void inst_query_proto_ver(const communication_t *comm, uint8_t *data, uint8_t size)
{
	assert(size == 0);

	uint16_t bl_proto_ver = BL_PROTO_VER;
	uint8_t bl_proto_ver_array[2];
	sip_t sip;
	crc8_t crc8 = 0;

	// set sip response message
	sip_init(&sip);
	sip_set(&sip, SIP_CMD_S, COMM_DATA);

	// set ackno=0xffff
	sip_set(&sip, SIP_PAYLOAD_0_S, 0xff);
	sip_set(&sip, SIP_PAYLOAD_1_S, 0xff);

	// convert 32bit base address to bytes
	uint16_to_uint8(&bl_proto_ver, bl_proto_ver_array);

	for (uint8_t i = 0; i < 2; i++)
	{
		sip_set(&sip, (sip_symbol_t) 2 + i, bl_proto_ver_array[i]);
	}

	// calculate crc
	for (uint8_t i = SIP_PAYLOAD_0_S; i < 2 + 2; i++)
	{
		crc8 = dscrc8_byte(crc8, sip_get(&sip, (sip_symbol_t) i));
	}

	// set crc
	sip_set(&sip, (sip_symbol_t) 4, crc8);

	// set total length (includes CRC)
	sip_set(&sip, SIP_PAYLOAD_LENGTH_S, 5);

	// get raw pointer
	uint8_t *raw_ptr = sip_get_raw_ptr(&sip);

	// send to io device
	for (uint8_t i = 0; i < SIP_FIXED_OVERHEAD + 10; i++)	///< 4 prefix characters + 5*2 payload characters
	{
		comm->serial_device->put(comm->serial_device->device, *(raw_ptr + i));
	}

	// send last terminator
	comm->serial_device->put(comm->serial_device->device, '>');

	// flush the buffer
	comm->serial_device->flush(comm->serial_device->device);
}

void inst_reboot(const communication_t *comm, uint8_t *data, uint8_t size)
{
	assert(size == 0);

	// TODO: Maybe we have better way to implement this?
	NVIC_SystemReset();
}

void inst_query_device_info(const communication_t *comm, uint8_t *data, uint8_t size)
{
	// TODO: Bluetooth/Zigbee, inc stack version?
}

void inst_query_chip_info(const communication_t *comm, uint8_t *data, uint8_t size)
{
	assert(size == 0);

	uint64_t unique_id = SYSTEM_GetUnique();
	uint8_t unique_id_array[8];
	sip_t sip;
	crc8_t crc8 = 0;

	// set sip response message
	sip_init(&sip);
	sip_set(&sip, SIP_CMD_S, COMM_DATA);

	// set ackno=0xffff
	sip_set(&sip, SIP_PAYLOAD_0_S, 0xff);
	sip_set(&sip, SIP_PAYLOAD_1_S, 0xff);

	// convert 64bit unique id to bytes
	uint64_to_uint8(&unique_id, unique_id_array);

	for (uint8_t i = 0; i < 8; i++)
	{
		sip_set(&sip, (sip_symbol_t) 2 + i, unique_id_array[i]);
	}

	// calculate crc
	for (uint8_t i = SIP_PAYLOAD_0_S; i < 2 + 8; i++)
	{
		crc8 = dscrc8_byte(crc8, sip_get(&sip, (sip_symbol_t) i));
	}

	// set crc
	sip_set(&sip, (sip_symbol_t) 10, crc8);

	// set total length (includes CRC)
	sip_set(&sip, SIP_PAYLOAD_LENGTH_S, 11);

	// get raw pointer
	uint8_t *raw_ptr = sip_get_raw_ptr(&sip);

	// send to io device
	for (uint8_t i = 0; i < SIP_FIXED_OVERHEAD + 22; i++)	///< 4 prefix characters + 11*2 payload characters
	{
		comm->serial_device->put(comm->serial_device->device, *(raw_ptr + i));
	}

	// send last terminator
	comm->serial_device->put(comm->serial_device->device, '>');

	// flush the buffer
	comm->serial_device->flush(comm->serial_device->device);
}

void inst_get_base_addr(const communication_t *comm, uint8_t *data, uint8_t size)
{
	assert(size == 0);

	uint8_t base_addr_array[4];
	sip_t sip;
	crc8_t crc8 = 0;

	// set sip response message
	sip_init(&sip);
	sip_set(&sip, SIP_CMD_S, COMM_DATA);

	// set ackno=0xffff
	sip_set(&sip, SIP_PAYLOAD_0_S, 0xff);
	sip_set(&sip, SIP_PAYLOAD_1_S, 0xff);

	// convert 32bit base address to bytes
	uint32_to_uint8(&(bootloader_config.base_addr), base_addr_array);

	for (uint8_t i = 0; i < 4; i++)
	{
		sip_set(&sip, (sip_symbol_t) 2 + i, base_addr_array[i]);
	}

	// calculate crc
	for (uint8_t i = SIP_PAYLOAD_0_S; i < 2 + 4; i++)
	{
		crc8 = dscrc8_byte(crc8, sip_get(&sip, (sip_symbol_t) i));
	}

	// set crc
	sip_set(&sip, (sip_symbol_t) 6, crc8);

	// set total length (includes CRC)
	sip_set(&sip, SIP_PAYLOAD_LENGTH_S, 7);

	// get raw pointer
	uint8_t *raw_ptr = sip_get_raw_ptr(&sip);

	// send to io device
	for (uint8_t i = 0; i < SIP_FIXED_OVERHEAD + 14; i++)	///< 4 prefix characters + 7*2 payload characters
	{
		comm->serial_device->put(comm->serial_device->device, *(raw_ptr + i));
	}

	// send last terminator
	comm->serial_device->put(comm->serial_device->device, '>');

	// flush the buffer
	comm->serial_device->flush(comm->serial_device->device);
}

bool is_valid_address(uint32_t address)
{
	// is that the address within flash?
	if (address >= FLASH_BASE && address < (FLASH_BASE + FLASH_SIZE))
	{
		return true;
	}

	// is that the address within sram?
	if (address >= SRAM_BASE && address < (SRAM_BASE + SRAM_SIZE))
	{
		return true;
	}

	return false;
}
