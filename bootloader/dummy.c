/**
 * @brief Place holder for some default variables
 */

#include <stdint.h>
#include PLATFORM_HEADER

#define RESET_INFO_SIZE 0xFF

VAR_AT_SEGMENT(uint8_t reset_info_sram_place_holder[RESET_INFO_SIZE], __RESETINFO__);
