/**
 * @brief UART device adapter for IO_Device super class
 * @file uart_device.c
 * @author Ran Bao
 * @date Aug, 2017
 */

#include <stdint.h>
#include <stdbool.h>
#include "io_device.h"
#include "btl_driver_uart.h"


io_device uart_device_private;


/**
 * @brief Get an instance of uart device
 * @return the pointer to the uart device
 */
io_device * get_uart_io_device(void)
{
	return &uart_device_private;
}

int uart_putc(void * device, int ch)
{
	uart_sendByte((uint8_t) ch);
	return ch;
}

int uart_getc(void * device)
{
	uint8_t byte = 0;
	uart_receiveByte(&byte);

	return byte;
}

bool uart_read_ready(void * device)
{
	return (uart_getRxAvailableBytes() > 0);
}

void uart_flush_tx_buffer(void * device)
{
	uart_flush(true, false);
}

size_t uart_read(void * device, void * buffer, size_t size)
{
	size_t actual_received_bytes = 0;

	uart_receiveBuffer(
			buffer,
			size,
			&actual_received_bytes,
			true,
			50 // 50 ms delay
	);

	return actual_received_bytes;
}

size_t uart_write(void * device, void * buffer, size_t size)
{
	size_t actual_transmitted_bytes = 0;

	uart_sendBuffer(buffer, size, true);

	return actual_transmitted_bytes; // uart platform doesn't support this feature
}


/**
 * @brief Initialize uart device. The function must be called before invoking any
 * uart function calls
 */
void uart_device_init(void)
{
	uart_init();

	// safely initialize io device
	io_device_init(&uart_device_private);

	// assign function handlers
	uart_device_private.device = NULL;
	uart_device_private.put = (void *) uart_putc;
	uart_device_private.get = (void *) uart_getc;
	uart_device_private.write = (void *) uart_write;
	uart_device_private.read = (void *) uart_read;
	uart_device_private.read_ready = (void *) uart_read_ready;
	uart_device_private.flush = (void *) uart_flush_tx_buffer;
}
