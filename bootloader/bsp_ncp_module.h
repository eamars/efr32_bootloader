/***************************************************************************//**
 * @file
 * @brief Provide HAL configuration parameters.
 * @version 5.2.2
 *******************************************************************************
 * @section License
 * <b>Copyright 2017 Silicon Laboratories, Inc. http://www.silabs.com</b>
 *******************************************************************************
 *
 * This file is licensed under the Silicon Labs License Agreement. See the file
 * "Silabs_License_Agreement.txt" for details. Before using this software for
 * any purpose, you must agree to the terms of that agreement.
 *
 ******************************************************************************/
#ifndef BOARD_NCP_MODULE_H_
#define BOARD_NCP_MODULE_H_

#include "pio_defs.h"

// Board hardware specification
#define HATCH_HARDWARE_REV 0x0
#define HATCH_HARDWARE_PRODUCT_ID 0x4

// -----------------------------------------------------------------------------
/* BUTTON */
// Enable two buttons, 0 and 1
#define HAL_BUTTON_COUNT     0

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

// -----------------------------------------------------------------------------
/* DCDC */
// MCU is wired for DCDC mode
#define BSP_DCDC_PRESENT 1
// Use emlib default DCDC initialization
#define BSP_DCDC_INIT                                                               \
  {                                                                                 \
    emuPowerConfig_DcdcToDvdd,   /* DCDC to DVDD */                                 \
    emuDcdcMode_LowNoise,        /* Low-niose mode in EM0 */                        \
    1800,                        /* Nominal output voltage for DVDD mode, 1.8V  */  \
    15,                          /* Nominal EM0/1 load current of less than 15mA */ \
    10,                          /* Nominal EM2/3/4 load current less than 10uA  */ \
    200,                         /* Maximum average current of 200mA
                                    (assume strong battery or other power source) */      \
    emuDcdcAnaPeripheralPower_DCDC,/* Select DCDC as analog power supply (lower power) */ \
    160,                         /* Maximum reverse current of 160mA */                   \
    emuDcdcLnCompCtrl_1u0F,      /* 1uF DCDC capacitor */                               \
  }
// Do not enable bypass mode
#define HAL_DCDC_BYPASS  0


// -----------------------------------------------------------------------------
/* LED */
#define HAL_LED_COUNT        0

// -----------------------------------------------------------------------------
/* PA */
#define HAL_PA_2P4_ENABLE      1
#define HAL_PA_2P4_VOLTMODE    PA_VOLTMODE_VBAT
#define HAL_PA_2P4_POWER       190
#define HAL_PA_2P4_OFFSET      0
#define HAL_PA_2P4_RAMP        10

// -----------------------------------------------------------------------------
/* USART0 */


// -----------------------------------------------------------------------------
/* VCOM */


/**
 * @brief Port A peripherals
 */

// Soft reset from host on PA4
#define BSP_SW_RESET_PIN    4
#define BSP_SW_RESET_PORT   gpioPortA

// Wake port to host on PA5
#define BSP_WAKE_PIN        5
#define BSP_WAKE_PORT       gpioPortA


/**
 * @brief Port B peripherals
 */

// Interrupt out to host on PB11
#define BSP_INT_OUT_PIN     11
#define BSP_INT_OUT_PORT    gpioPortB

// USART0 RX on PB12
#define BSP_USART0_RX_LOC      _USART_ROUTELOC0_RXLOC_LOC6
#define BSP_USART0_RX_PIN      12
#define BSP_USART0_RX_PORT     gpioPortB

// USART0 CLK on PB13
#define BSP_USART0_CLK_LOC      _USART_ROUTELOC0_CLKLOC_LOC6
#define BSP_USART0_CLK_PIN      13
#define BSP_USART0_CLK_PORT     gpioPortB

/**
 * @brief Port C peripherals
 */


// USART0 TX on PC10
#define BSP_USART0_TX_LOC      _USART_ROUTELOC0_TXLOC_LOC15
#define BSP_USART0_TX_PIN      10
#define BSP_USART0_TX_PORT     gpioPortC

// USART0 CS on PC11
#define BSP_USART0_CS_LOC       _USART_ROUTELOC0_CSLOC_LOC13
#define BSP_USART0_CS_PIN       11
#define BSP_USART0_CS_PORT      gpioPortC

// override debug macros for driver
#include <assert.h>
#define DRV_ASSERT(x) assert((x))

// override debug macro for apps
#define APP_ASSERT(x) assert((x))

#endif //BOARD_NCP_MODULE_H_
