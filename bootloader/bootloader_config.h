/**
 * @brief Configuration file for bootloader
 */

#ifndef BOOTLOADER_CONFIG_H_
#define BOOTLOADER_CONFIG_H_

#include <stdint.h>
#include <stdbool.h>
#include "stack32.h"
#include BOARD_HEADER

// Use this macro to check if UART plugin is included
#define EMBER_AF_PLUGIN_UART_DRIVER
// User options for plugin UART
#define BTL_DRIVER_UART_NUMBER 0
#define BTL_DRIVER_UART_BAUDRATE 115200
#define BTL_DRIVER_UART_TX_PORT BSP_USART0_TX_PORT
#define BTL_DRIVER_UART_TX_PIN BSP_USART0_TX_PIN
#define BTL_DRIVER_UART_TX_LOCATION BSP_USART0_TX_LOC
#define BTL_DRIVER_UART_RX_PORT BSP_USART0_RX_PORT
#define BTL_DRIVER_UART_RX_PIN BSP_USART0_RX_PIN
#define BTL_DRIVER_UART_RX_LOCATION BSP_USART0_RX_LOC
//#define BTL_DRIVER_UART_USE_ENABLE
//#define BTL_DRIVER_UART_EN_PORT gpioPortA
//#define BTL_DRIVER_UART_EN_PIN 5
#define BTL_DRIVER_UART_RX_BUFFER_SIZE 512
#define BTL_DRIVER_UART_TX_BUFFER_SIZE 128
#define BTL_DRIVER_UART_LDMA_RX_CHANNEL 0
#define BTL_DRIVER_UART_LDMA_TX_CHANNEL 1

// User options for plugin GPIO activation, valid only if the board have at least one button
#if (HAL_BUTTON_COUNT > 0)
#define BTL_GPIO_ACTIVATION_PORT BSP_BUTTON0_PORT
	#define BTL_GPIO_ACTIVATION_PIN BSP_BUTTON0_PIN
	#define BTL_GPIO_ACTIVATION_POLARITY 1
#endif

#ifdef __cplusplus
extern "C" {
#endif



#ifdef __cplusplus
}
#endif


#endif /* BOOTLOADER_CONFIG_H_ */
