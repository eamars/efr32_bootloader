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
void trap(void);

bool button_override(void);
bool boot_to_bootloader(void);
bool boot_to_application(uint32_t * aat_addr);
bool boot_to_prev_application(uint32_t * aat_addr);

void rmu_reset_reason_dump(void);
void trap_on_hardware_failure(void);
bool validate_firmware(uint32_t aat_addr);

#ifdef __cplusplus
}
#endif

#endif
