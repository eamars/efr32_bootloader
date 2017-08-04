/**
 * @file handle_instruction.c
 * @brief Function that handle incoming instruction message
 * @author Ran Bao (ran.bao@wirelessguard.co.nz)
 * @date March, 2017
 */
#include <stdint.h>
#include <string.h> // memset
#include <stdlib.h> // atoi



#include "bootloader_api.h"




#include "dispatcher.h"



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
	bootloader_instruction_t instruction = (bootloader_instruction_t) data[0];

	// call dispatcher to execute corresponding function
	bl_instruction_dispacher(instruction, comm, data + 1, (uint8_t) (size - 1));
}

