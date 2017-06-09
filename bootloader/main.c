/**
 * @brief
 *
 */
#include <stdint.h>
#include <stdbool.h>
#include "em_device.h"

#define USER_APPLICATION_ADDR 0x100UL

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
	binary_exec((void *) USER_APPLICATION_ADDR, false);

	while (1)
	{

	}
}
