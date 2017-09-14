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
#ifndef BOARD_HATCH_H_
#define BOARD_HATCH_H_

#include "pio_defs.h"

// Board hardware specification
#define HATCH_HARDWARE_REV 0x0
#define HATCH_HARDWARE_PRODUCT_ID 0x1

// -----------------------------------------------------------------------------
/* BUTTON */
// Enable two buttons, 0 and 1
#define HAL_BUTTON_COUNT     1
#define HAL_BUTTON_ENABLE    { 0 }
// Board has two buttons
#define BSP_BUTTON_COUNT     1
#define BSP_BUTTON_INIT                    \
  {                                        \
    { BSP_BUTTON0_PORT, BSP_BUTTON0_PIN }  \
  }
// Initialize button GPIO DOUT to 0
#define BSP_BUTTON_GPIO_DOUT HAL_GPIO_DOUT_LOW
// Initialize button GPIO mode as input
#define BSP_BUTTON_GPIO_MODE HAL_GPIO_MODE_INPUT
// Define individual button GPIO port/pin
#define BSP_BUTTON0_PORT     BSP_USER_BUTTON_PORT
#define BSP_BUTTON0_PIN      BSP_USER_BUTTON_PIN

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
#define HAL_LED_COUNT        2
#define HAL_LED_ENABLE       { 0, 1 }
#define BSP_LED_COUNT        2
#define BSP_LED_INIT                 \
  {                                  \
    { BSP_LED0_PORT, BSP_LED0_PIN }, \
    { BSP_LED1_PORT, BSP_LED1_PIN }  \
  }
// Define individual LED GPIO port/pin
#define BSP_LED0_PORT        BSP_LED_GREEN_PORT
#define BSP_LED0_PIN         BSP_LED_GREEN_PIN
#define BSP_LED1_PORT        BSP_LED_BLUE_PORT
#define BSP_LED1_PIN         BSP_LED_BLUE_PIN

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

// LED Anode on PA0
#define BSP_LED_EN_PIN      0
#define BSP_LED_EN_PORT     gpioPortA

// Green LED on PA1
#define BSP_LED_GREEN_PIN   1
#define BSP_LED_GREEN_PORT  gpioPortA

// Blue LED on PA2
#define BSP_LED_BLUE_PIN    2
#define BSP_LED_BLUE_PORT   gpioPortA

// Temperature Alert interrupt on PA5
#define BSP_TEMP_ALERT_INT_PIN  5
#define BSP_TEMP_ALERT_INT_PORT gpioPortA



/**
 * @brief Port B peripherals
 */

// Battery ADC on PB11 (CH27)
// adcPosSelAPORT3YCH27 or adcPosSelAPORT4XCH27
#define BSP_BAT_ADC_PIN     11
#define BSP_BAT_ADC_PORT    gpioPortB

// Battery monitor enable pin
#define BSP_BAT_MON_EN_PIN     12
#define BSP_BAT_MON_EN_PORT    gpioPortB

// User button on PB13
#define BSP_USER_BUTTON_PIN     13
#define BSP_USER_BUTTON_PORT    gpioPortB


/**
 * @brief Port C peripherals
 */

// USB Detect in PC6
#define BSP_USB_DETECT_PIN      6
#define BSP_USB_DETECT_PORT     gpioPortC

// USB Load Switch on PC7
#define BSP_USB_DETECT_EN_PIN     7
#define BSP_USB_DETECT_EN_PORT    gpioPortC

// UART Level Shifter (3v3 to 5v) on PC8
#define BSP_UART_LS_EN_PIN      8
#define BSP_UART_LS_EN_PORT     gpioPortC

// USART0 RX on PC9
#define BSP_USART0_RX_LOC      _USART_ROUTELOC0_RXLOC_LOC13
#define BSP_USART0_RX_PIN      9
#define BSP_USART0_RX_PORT     gpioPortC

// USART TX on PC10
#define BSP_USART0_TX_LOC      _USART_ROUTELOC0_TXLOC_LOC15
#define BSP_USART0_TX_PIN      10
#define BSP_USART0_TX_PORT     gpioPortC

// EEPROM Load Switch on PC11
#define BSP_EEPROM_EN_PIN      11
#define BSP_EEPROM_EN_PORT     gpioPortC


/**
 * @brief Port D peripherals
 */

// LDC Interrupt on PD12
#define BSP_LDC_INT_PIN         12
#define BSP_LDC_INT_PORT        gpioPortD

// LDC Clock Enable on PD13
#define BSP_LDC_CLK_EN_PIN      13
#define BSP_LDC_CLK_EN_PORT     gpioPortD

// LDC Shutdown on PD14
#define BSP_LDC_SD_PIN          14
#define BSP_LDC_SD_PORT        gpioPortD

// LDC Load Switch on PD15
#define BSP_LDC_EN_PIN     15
#define BSP_LDC_EN_PORT    gpioPortD


/**
 * @brief Port F Peripherals
 */

// SWD SWCLK on PF0
// Note that this function is enabled to the pin out of reset and has a built-in pull down
#define BSP_DBG_SWCLKTCK_LOC    0
#define BSP_DBG_SWCLKTCK_PIN    0
#define BSP_DBG_SWCLKTCK_PORT   gpioPortF

// SWD SWDIO on PF1
// Note that this function is enabled to the pin out of reset and has a built-in pull up
#define BSP_DBG_SWDIOTMS_LOC    0
#define BSP_DBG_SWDIOTMS_PIN    1
#define BSP_DBG_SWDIOTMS_PORT   gpioPortF

// IMU Interrupt 2 on PF2
#define BSP_IMU_INT_2_PIN       2
#define BSP_IMU_INT_2_PORT      gpioPortF

// IMU Interrupt 1 on PF3
#define BSP_IMU_INT_1_PIN       3
#define BSP_IMU_INT_1_PORT      gpioPortF

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

// IMU Load Switch on PF 7
#define BSP_IMU_EN_PIN     7
#define BSP_IMU_EN_PORT    gpioPortF

// override debug macros for driver
#include <assert.h>
#define DRV_ASSERT(x) assert((x))

// override debug macro for apps
#define APP_ASSERT(x) assert((x))

#endif //BOARD_HATCH_H_
