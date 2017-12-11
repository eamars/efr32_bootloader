/**
 * @brief Board specific peripheral information for hatch outdoor version 2
 * @author Ran Bao
 * @date 5/Dec/2017
 * @file board_hatch_outdoor_v2.h
 */

#ifndef BOARD_HATCH_OUTDOOR_V2_H_
#define BOARD_HATCH_OUTDOOR_V2_H_

// Board hardware specification
#define HATCH_HARDWARE_REV 0x2
#define HATCH_HARDWARE_PRODUCT_ID 0x4

// LORA Co-processor
#define RFM9X_DIO0_PORT gpioPortA
#define RFM9X_DIO0_PIN  0

#define RFM9X_RST_PORT gpioPortA
#define RFM9X_RST_PIN  5

// LORA Co-processor SPI
#define RFM9X_SPI_ENGINE USART1

#define RFM9X_SPI_MISO_PORT gpioPortA
#define RFM9X_SPI_MISO_PIN  1
#define RFM9X_SPI_MISO_LOC  _USART_ROUTELOC0_RXLOC_LOC0

#define RFM9X_SPI_MOSI_PORT gpioPortA
#define RFM9X_SPI_MOSI_PIN  2
#define RFM9X_SPI_MOSI_LOC  _USART_ROUTELOC0_RXLOC_LOC2

#define RFM9X_SPI_CLK_PORT  gpioPortA
#define RFM9X_SPI_CLK_PIN   3
#define RFM9X_SPI_CLK_LOC   _USART_ROUTELOC0_RXLOC_LOC1

#define RFM9X_SPI_CS_PORT   gpioPortA
#define RFM9X_SPI_CS_PIN    4
#define RFM9X_SPI_CS_LOC    _USART_ROUTELOC0_RXLOC_LOC1

// LEDs
// Red LED on PD13
#define BSP_LED_RED_PORT    gpioPortD
#define BSP_LED_RED_PIN     15

// Green LED on PD14
#define BSP_LED_GREEN_PORT  gpioPortD
#define BSP_LED_GREEN_PIN   14

// Blue LED on PD15
#define BSP_LED_BLUE_PORT   gpioPortD
#define BSP_LED_BLUE_PIN    13

// Buttons
#define BSP_BUTTON0_PORT     gpioPortB
#define BSP_BUTTON0_PIN      11

// UART
#define UART_ENGINE USART0

#define UART_TX_PORT        gpioPortC
#define UART_TX_PIN         10
#define UART_TX_LOC         _USART_ROUTELOC0_RXLOC_LOC15

#define UART_RX_PORT        gpioPortC
#define UART_RX_PIN         9
#define UART_RX_LOC         _USART_ROUTELOC0_RXLOC_LOC13


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

// bootloader specific definitions
#define BTL_GPIO_ACTIVATION_PORT BSP_BUTTON0_PORT
#define BTL_GPIO_ACTIVATION_PIN BSP_BUTTON0_PIN
#define BTL_GPIO_ACTIVATION_POLARITY 1
#define BTL_SERIAL_APP_PORT HAL_SERIAL_PORT_USART0
#define BTL_SERIAL_APP_BAUD_RATE 115200
#define BTL_SERIAL_APP_TX_PORT UART_TX_PORT
#define BTL_SERIAL_APP_TX_PIN UART_TX_PIN
#define BTL_SERIAL_APP_TX_LOC UART_TX_LOC
#define BTL_SERIAL_APP_RX_PORT UART_RX_PORT
#define BTL_SERIAL_APP_RX_PIN UART_RX_PORT
#define BTL_SERIAL_APP_RX_LOC UART_RX_LOC
#define BTL_LED0_PORT BSP_LED_RED_PORT
#define BTL_LED0_PIN BSP_LED_RED_PIN
#define BTL_LED1_PORT BSP_LED_GREEN_PORT
#define BTL_LED1_PIN BSP_LED_GREEN_PIN
#define BTL_LED2_PORT BSP_LED_BLUE_PORT
#define BTL_LED2_PIN BSP_LED_BLUE_PIN

// override debug macros for driver
#include <assert.h>
#define DRV_ASSERT(x) assert((x))

// override debug macro for apps
#define APP_ASSERT(x) assert((x))

#endif // BOARD_HATCH_OUTDOOR_V2_H_
