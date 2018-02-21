/**
 * @brief Simmulated EEPROM Application Token
 * @author Ran Bao
 * @date 16/Jan/2018
 * @file token.h
 *
 * All data that are stored in SimEEPROM must have token id smaller than 256
 */

/**
* Custom Application Tokens
*/
// Define token names here
#define CREATOR_HATCH_BOOT_COUNT (0x000A)
#define CREATOR_HATCH_BOOT_INFO (0x000E)

#ifdef DEFINETYPES
// Include or define any typedef for tokens here
#include "bootloader_api.h"

#endif //DEFINETYPES


#ifdef DEFINETOKENS
// Define the actual token storage information here
DEFINE_COUNTER_TOKEN(HATCH_BOOT_COUNT, uint32_t, 0)
DEFINE_BASIC_TOKEN(HATCH_BOOT_INFO, hatch_boot_info_t, {0})

#endif //DEFINETOKENS
