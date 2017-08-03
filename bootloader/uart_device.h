/**
 * @brief UART device adapter for IO_Device super class
 * @file uart_device.h
 * @author Ran Bao
 * @date Aug, 2017
 */

#ifndef UART_DEVICE_H_
#define UART_DEVICE_H_

#include "io_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get an instance of uart device
 * @return the pointer to the uart device
 */
io_device * get_uart_io_device(void);

/**
 * @brief Initialize uart device. The function must be called before invoking any
 * uart function calls
 */
void uart_device_init(void);

#ifdef __cplusplus
}
#endif

#endif
