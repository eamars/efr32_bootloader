//
// Created by Ran Bao on 4/09/17.
//

#ifndef BSP_HATCH_H_
#define BSP_HATCH_H_

// -----------------------------------------------------------------------------
/* CLK */
// Set up HFCLK source as HFXO
#define HAL_CLK_HFCLK_SOURCE HAL_CLK_HFCLK_SOURCE_HFXO
// Setup LFCLK source as LFRCO
#define HAL_CLK_LFCLK_SOURCE HAL_CLK_LFCLK_SOURCE_LFXO
// Set HFXO frequency as 38.4MHz
#define BSP_CLK_HFXO_FREQ 38400000UL
// HFXO initialization settings
#define BSP_CLK_HFXO_INIT                                                  \
  {                                                                        \
    false,      /* Low-noise mode for EFR32 */                             \
    false,      /* Disable auto-start on EM0/1 entry */                    \
    false,      /* Disable auto-select on EM0/1 entry */                   \
    false,      /* Disable auto-start and select on RAC wakeup */          \
    _CMU_HFXOSTARTUPCTRL_CTUNE_DEFAULT,                                    \
    0x142,      /* Steady-state CTUNE for WSTK boards without load caps */ \
    _CMU_HFXOSTEADYSTATECTRL_REGISH_DEFAULT,                               \
    0x20,       /* Matching errata fix in CHIP_Init() */                   \
    0x7,        /* Recommended steady-state osc core bias current */       \
    0x6,        /* Recommended peak detection threshold */                 \
    _CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_DEFAULT,                          \
    0xA,        /* Recommended peak detection timeout  */                  \
    0x4,        /* Recommended steady timeout */                           \
    _CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_DEFAULT,                           \
    cmuOscMode_Crystal,                                                    \
  }
// Board has HFXO
#define BSP_CLK_HFXO_PRESENT 1
// Set LFXO frequency as 32.768kHz
#define BSP_CLK_LFXO_FREQ 32768UL
// Board has LFXO
#define BSP_CLK_LFXO_PRESENT 1

// EEPROM Load Switch on PC11
#define BSP_EEPROM_EN_PIN      11
#define BSP_EEPROM_EN_PORT     gpioPortC

// I2C SCL on PF4
#define BSP_I2C_SCL_LOC         _I2C_ROUTELOC0_SCLLOC_LOC27
#define BSP_I2C_SCL_PIN         4
#define BSP_I2C_SCL_PORT        gpioPortF

// I2C Enable Pin on PF5
#define BSP_I2C_EN_PIN          5
#define BSP_I2C_EN_PORT         gpioPortF

// I2C SDA on PF6
#define BSP_I2C_SDA_LOC         _I2C_ROUTELOC0_SCLLOC_LOC30
#define BSP_I2C_SDA_PIN         6
#define BSP_I2C_SDA_PORT        gpioPortF

// override debug macros for driver
#include <assert.h>
#define DRV_ASSERT(x) assert((x))

// override debug macro for apps
#define APP_ASSERT(x) assert((x))

#endif //EFR32_BOOTLOADER_BSP_HATCH_H_H
