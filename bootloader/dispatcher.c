/**
 * @file dispatcher.c
 * @brief Dispatch command with corresponding functions
 * @date Aug, 2017
 * @author Ran Bao
 */

#include "dispatcher.h"
#include "bootloader_api.h"
#include "command_handler.h"

static bl_inst_handler_entry inst_handler_table[] =
{
		{ INST_SET_BASE_ADDR, inst_set_base_addr },
		{ INST_GET_BASE_ADDR, inst_get_base_addr },
		{ INST_PUSH_BASE_ADDR, inst_push_base_addr },
		{ INST_POP_BASE_ADDR, inst_pop_base_addr },
		{ INST_SET_BLOCK_EXP, inst_set_block_exp },
		{ INST_GET_BLOCK_EXP, inst_get_block_exp },
		{ INST_BRANCH_TO_ADDR, inst_branch_to_addr },
		{ INST_ERASE_PAGES, inst_erase_page },
		{ INST_ERASE_RANGE, inst_erase_range },
		{ INST_REBOOT, inst_reboot },
		{ INST_QUERY_PROTO_VER, inst_query_proto_ver },
		{ INST_QUERY_DEVICE_INFO, inst_query_device_info },
		{ INST_QUERY_CHIP_INFO, inst_query_chip_info },
		{ INST_INVALID, NULL }
};

void bl_instruction_dispacher(bootloader_instruction_t inst, const communication_t *comm, uint8_t *payload, uint8_t size)
{
	for (bl_inst_handler_entry * entry = &inst_handler_table[0]; entry->inst != INST_INVALID; entry++)
	{
		if (entry->inst == inst)
		{
			entry->handler(comm, payload, size);
			break;
		}
	}
}
