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
#ifndef BOARD_DEV_H_
#define BOARD_DEV_H_


// Board hardware specification
#define HATCH_HARDWARE_REV 0x0
#define HATCH_HARDWARE_PRODUCT_ID 0x0

// -----------------------------------------------------------------------------
/* BUTTON */
// Enable two buttons, 0 and 1
#define HAL_BUTTON_COUNT     2
#define HAL_BUTTON_ENABLE    { 0, 1 }
// Board has two buttons
#define BSP_BUTTON_COUNT     2
#define BSP_BUTTON_INIT                    \
  {                                        \
    { BSP_BUTTON0_PORT, BSP_BUTTON0_PIN }, \
    { BSP_BUTTON1_PORT, BSP_BUTTON1_PIN }  \
  }
// Initialize button GPIO DOUT to 0
#define BSP_BUTTON_GPIO_DOUT HAL_GPIO_DOUT_LOW
// Initialize button GPIO mode as input
#define BSP_BUTTON_GPIO_MODE HAL_GPIO_MODE_INPUT
// Define individual button GPIO port/pin
#define BSP_BUTTON0_PORT     gpioPortF
#define BSP_BUTTON0_PIN      6
#define BSP_BUTTON1_PORT     gpioPortF
#define BSP_BUTTON1_PIN      7

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
#define BSP_DCDC_INIT    EMU_DCDCINIT_DEFAULT
// Do not enable bypass mode
#define HAL_DCDC_BYPASS  0

// -----------------------------------------------------------------------------
/* EXTFLASH */
#define HAL_EXTFLASH_USART_BAUDRATE 6400000U
#define BSP_EXTFLASH_CS_LOC         _USART_ROUTELOC0_CSLOC_LOC1
#define BSP_EXTFLASH_CS_PIN         4
#define BSP_EXTFLASH_CS_PORT        gpioPortA
#define BSP_EXTFLASH_MISO_LOC       _USART_ROUTELOC0_RXLOC_LOC11
#define BSP_EXTFLASH_MISO_PIN       7
#define BSP_EXTFLASH_MISO_PORT      gpioPortC
#define BSP_EXTFLASH_MOSI_LOC       _USART_ROUTELOC0_TXLOC_LOC11
#define BSP_EXTFLASH_MOSI_PIN       6
#define BSP_EXTFLASH_MOSI_PORT      gpioPortC
#define BSP_EXTFLASH_SCLK_LOC       _USART_ROUTELOC0_CLKLOC_LOC11
#define BSP_EXTFLASH_SCLK_PIN       8
#define BSP_EXTFLASH_SCLK_PORT      gpioPortC
#define BSP_EXTFLASH_USART          USART1
#define BSP_EXTFLASH_USART_CLK      cmuClock_USART1

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
#define BSP_LED0_PORT        gpioPortF
#define BSP_LED0_PIN         4
#define BSP_LED1_PORT        gpioPortF
#define BSP_LED1_PIN         5

// -----------------------------------------------------------------------------
/* PA */
#define HAL_PA_ENABLE                                 (1)

#define HAL_PA_RAMP                                   (10)
#define HAL_PA_2P4_LOWPOWER                           (0)
#define HAL_PA_POWER                                  (252)
#define HAL_PA_VOLTAGE                                (1800)
#define HAL_PA_CURVE_HEADER                            "pa_curves_efr32.h"

// -----------------------------------------------------------------------------
/* PTI */
#define PORTIO_PTI_DFRAME_PIN                         (13)
#define PORTIO_PTI_DFRAME_PORT                        (gpioPortB)
#define PORTIO_PTI_DFRAME_LOC                         (6)

#define PORTIO_PTI_DOUT_PIN                           (12)
#define PORTIO_PTI_DOUT_PORT                          (gpioPortB)
#define PORTIO_PTI_DOUT_LOC                           (6)

#define HAL_PTI_ENABLE                                (1)

#define BSP_PTI_DFRAME_PIN                            (13)
#define BSP_PTI_DFRAME_PORT                           (gpioPortB)
#define BSP_PTI_DFRAME_LOC                            (6)

#define BSP_PTI_DOUT_PIN                              (12)
#define BSP_PTI_DOUT_PORT                             (gpioPortB)
#define BSP_PTI_DOUT_LOC                              (6)

#define HAL_PTI_MODE                                  (HAL_PTI_MODE_UART)
#define HAL_PTI_BAUD_RATE                             (1600000)

// -----------------------------------------------------------------------------
/* SPINCP */
#define BSP_SPINCP_USART_PORT    2
// Map NHOSTINT to EXP_HEADER7
#define BSP_SPINCP_NHOSTINT_PIN  10
#define BSP_SPINCP_NHOSTINT_PORT gpioPortD
// Map NWAKE to EXP_HEADER9
#define BSP_SPINCP_NWAKE_PIN     11
#define BSP_SPINCP_NWAKE_PORT    gpioPortD

// -----------------------------------------------------------------------------
/* USART0 */
#define BSP_USART0_RX_LOC      _USART_ROUTELOC0_RXLOC_LOC0
#define BSP_USART0_RX_PIN      1
#define BSP_USART0_RX_PORT     gpioPortA
#define BSP_USART0_TX_LOC      _USART_ROUTELOC0_TXLOC_LOC0
#define BSP_USART0_TX_PIN      0
#define BSP_USART0_TX_PORT     gpioPortA

// -----------------------------------------------------------------------------
/* USART2 */
#define BSP_USART2_CLK_LOC     _USART_ROUTELOC0_CLKLOC_LOC1
#define BSP_USART2_CLK_PIN     8
#define BSP_USART2_CLK_PORT    gpioPortA
#define BSP_USART2_CS_LOC      _USART_ROUTELOC0_CSLOC_LOC1
#define BSP_USART2_CS_PIN      9
#define BSP_USART2_CS_PORT     gpioPortA
#define BSP_USART2_MISO_LOC    _USART_ROUTELOC0_RXLOC_LOC1
#define BSP_USART2_MISO_PIN    7
#define BSP_USART2_MISO_PORT   gpioPortA
#define BSP_USART2_MOSI_LOC    _USART_ROUTELOC0_TXLOC_LOC1
#define BSP_USART2_MOSI_PIN    6
#define BSP_USART2_MOSI_PORT   gpioPortA

// -----------------------------------------------------------------------------
/* USART3 */
#define BSP_USART3_CTS_LOC               _USART_ROUTELOC1_CTSLOC_LOC28
#define BSP_USART3_CTS_PIN               8
#define BSP_USART3_CTS_PORT              gpioPortD
#define BSP_USART3_RTS_LOC               _USART_ROUTELOC1_RTSLOC_LOC28
#define BSP_USART3_RTS_PIN               9
#define BSP_USART3_RTS_PORT              gpioPortD
#define BSP_USART3_RX_LOC                _USART_ROUTELOC0_RXLOC_LOC10
#define BSP_USART3_TX_PIN                6
#define BSP_USART3_TX_PORT               gpioPortB
#define BSP_USART3_TX_LOC                _USART_ROUTELOC0_TXLOC_LOC10
#define BSP_USART3_RX_PIN                7
#define BSP_USART3_RX_PORT               gpioPortB

// -----------------------------------------------------------------------------
/* VCOM */
#define BSP_VCOM_PRESENT     1
#define BSP_VCOM_CTS_LOC     _USART_ROUTELOC1_CTSLOC_LOC30
#define BSP_VCOM_CTS_PIN     2
#define BSP_VCOM_CTS_PORT    gpioPortA
#define BSP_VCOM_ENABLE_PIN  5
#define BSP_VCOM_ENABLE_PORT gpioPortA
#define BSP_VCOM_RTS_LOC     _USART_ROUTELOC1_RTSLOC_LOC30
#define BSP_VCOM_RTS_PIN     3
#define BSP_VCOM_RTS_PORT    gpioPortA
#define BSP_VCOM_RX_LOC      _USART_ROUTELOC0_RXLOC_LOC0
#define BSP_VCOM_RX_PIN      1
#define BSP_VCOM_RX_PORT     gpioPortA
#define BSP_VCOM_TX_LOC      _USART_ROUTELOC0_TXLOC_LOC0
#define BSP_VCOM_TX_PIN      0
#define BSP_VCOM_TX_PORT     gpioPortA
#define BSP_VCOM_USART       HAL_SERIAL_PORT_USART0


// override debug macros for driver
#include <assert.h>

#define DRV_ASSERT(x) assert((x))

// override debug macro for apps
#define APP_ASSERT(x) assert((x))

#endif //BOARD_DEV_H_
