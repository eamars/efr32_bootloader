/**
 * @file dispatcher.h
 * @brief Dispatch command with corresponding functions
 * @date Aug, 2017
 * @author Ran Bao
 */

#ifndef DISPATCHER_H_
#define DISPATCHER_H_

#include "bootloader_api.h"
#include "communication.h"

typedef void (*bl_inst_handler)(const communication_t *comm, uint8_t * data, uint8_t size);

typedef struct
{
	const bootloader_instruction_t inst;
	bl_inst_handler handler;
} bl_inst_handler_entry;


#ifdef __cplusplus
extern "C" {
#endif

void bl_instruction_dispacher(bootloader_instruction_t inst, const communication_t *comm, uint8_t *payload, uint8_t size);

#ifdef __cplusplus
}
#endif

#endif
