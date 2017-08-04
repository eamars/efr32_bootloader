/**
 * @brief Handler for process bootloader command
 * @@file command_handler.h
 * @author Ran Bao
 * @date Aug, 2017
 */

#ifndef COMMAND_HANDLER_H_
#define COMMAND_HANDLER_H_

#include <stdint.h>
#include <stdbool.h>
#include "communication.h"

#ifdef __cplusplus
extern "C" {
#endif

void inst_set_base_addr(const communication_t *comm, uint8_t *data, uint8_t size);
void inst_get_base_addr(const communication_t *comm, uint8_t *data, uint8_t size);
void inst_push_base_addr(const communication_t *comm, uint8_t *data, uint8_t size);
void inst_pop_base_addr(const communication_t *comm, uint8_t *data, uint8_t size);
void inst_set_block_exp(const communication_t *comm, uint8_t *data, uint8_t size);
void inst_get_block_exp(const communication_t *comm, uint8_t *data, uint8_t size);
void inst_branch_to_addr(const communication_t *comm, uint8_t *data, uint8_t size);
void inst_erase_page(const communication_t *comm, uint8_t *data, uint8_t size);
void inst_erase_range(const communication_t *comm, uint8_t *data, uint8_t size);
void inst_reboot(const communication_t *comm, uint8_t *data, uint8_t size);
void inst_query_proto_ver(const communication_t *comm, uint8_t *data, uint8_t size);
void inst_query_device_info(const communication_t *comm, uint8_t *data, uint8_t size);
void inst_query_chip_info(const communication_t *comm, uint8_t *data, uint8_t size);

#ifdef __cplusplus
}
#endif

#endif
