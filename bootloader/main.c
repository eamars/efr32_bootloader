/**
 * @brief
 *
 */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "em_chip.h"
#include "io_device.h"
#include "em_device.h"
#include "em_msc.h"
#include "uart_device.h"
#include "communication.h"
#include "bootloader_config.h"
#include "bootloader_api.h"

#define USER_APPLICATION_ADDR 0x100UL

// function that is implemented in another file
extern void comm_cb_inst(const void *handler, uint8_t *data, uint8_t size);
extern void comm_cb_data(uint16_t block_offset, uint8_t *data, uint8_t size);

// global variables
bootloader_config_t bootloader_config;

/**
 * @brief Execute interrupt vector table aligned at specific address
 * @param addr address of target interrupt vector table
 */
__attribute__ ((naked))
void _binary_exec(void *addr __attribute__((unused)))
{
	asm volatile (
		"mov r1, r0\n"			// r0 is the first argument
		"ldr r0, [r1, #4]\n"	// load the address of static interrupt vector with offset 4 (Reset_handler)
		"ldr sp, [r1]\n"		// reset stack pointer
		"blx r0\n"				// branch to Reset_handler
	);
}

void binary_exec(void *addr, bool loader)
{
	// disable global interrupts
	__disable_irq();

	// disable irqs
	for (register uint32_t i = 0; i < 8; i++)
	{
		NVIC->ICER[i] = 0xFFFFFFFF;
	}
	for (register uint32_t i = 0; i < 8; i++)
	{
		NVIC->ICPR[i] = 0xFFFFFFFF;
	}

	// flush registers and memories
	__DSB();	// data memory barrier @see __DSB()
	__ISB();	// instruction memory barrier @__ISB()

	// change vector table
	SCB->VTOR = ((uint32_t) addr & SCB_VTOR_TBLOFF_Msk);

	// barriers
	__DSB();
	__ISB();

	// enable interrupts
	__enable_irq();

	// load stack and pc
	_binary_exec(addr);

	// should never run beyond this point
	while (1)
	{

	}
}

int main(void)
{
	CHIP_Init();

	// initialize flash driver
	MSC_Init();

	uart_device_init();
	io_device * uart_device = get_uart_io_device();

	// create communication module
	communication_t comm;

	// initialize communication module (send and receive program)
	communication_init(&comm, uart_device, comm_cb_data, comm_cb_inst);

	// initialize flash driver
	MSC_Init();

	// initialize bootloader config
	bootloader_config.bootloader_mode = BOOTLOADER_WAIT_FOR_BYTES;
	bootloader_config.base_addr = 0xffffffff;
	bootloader_config.block_size_exp = 0;
	stack32_init(&bootloader_config.base_addr_stack, 2);


	while (1)
	{
		while (communication_ready(&comm))
		{
			communication_receive(&comm);
		}
	}
}
