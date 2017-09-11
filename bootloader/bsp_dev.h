//
// Created by Ran Bao on 4/09/17.
//

#ifndef BSP_DEV_H_
#define BSP_DEV_H_

// override debug macros for driver
#include <assert.h>
#define DRV_ASSERT(x) assert((x))

// override debug macro for apps
#define APP_ASSERT(x) assert((x))

#endif //EFR32_BOOTLOADER_BSP_DEV_H_H
