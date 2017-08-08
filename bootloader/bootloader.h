/**
 * @brief Bootloader implementation
 * @file bootloader.h
 * @author Ran Bao
 * @date Aug, 2017
 */


#ifndef BOOTLOADER_H_
#define BOOTLOADER_H_

#include <stdbool.h>
#include <stdint.h>


#define INVALID_BASE_ADDR 0xffffffff

#ifdef __cplusplus
extern "C" {
#endif

void bootloader(void);

bool is_button_override(void);
bool is_sw_reset(void);
bool is_request_override(uint32_t * app_addr);
bool is_prev_app_valid(uint32_t * app_addr);

#ifdef __cplusplus
}
#endif

#endif
