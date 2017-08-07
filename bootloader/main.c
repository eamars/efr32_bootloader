/**
 * @brief
 *
 */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "em_device.h"
#include "em_cmu.h"
#include "em_chip.h"
#include "em_rmu.h"
#include "em_gpio.h"
#include "em_msc.h"

#include "io_device.h"
#include "uart_device.h"
#include "communication.h"
#include "bootloader_config.h"
#include "bootloader_api.h"
#include "btl_reset.h"
#include "btl_reset_info.h"

#include "simgpbr.h"

#define USER_APPLICATION_ADDR 0x100UL

// function that is implemented in another file
extern void comm_cb_inst(const void *handler, uint8_t *data, uint8_t size);
extern void comm_cb_data(uint16_t block_offset, uint8_t *data, uint8_t size);
extern uint32_t __CRASHINFO__begin;

// global variables
bootloader_config_t bootloader_config;

void bootloader(void)
{
	// initialize flash driver
	MSC_Init();

	uart_device_init();
	io_device * uart_device = get_uart_io_device();

	// create communication module
	communication_t comm;

	// initialize communication module (send and receive program)
	communication_init(&comm, uart_device, comm_cb_data, comm_cb_inst);

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

bool is_button_override(void)
{
	bool pressed;

	// Enable GPIO clock
	CMU->HFBUSCLKEN0 |= CMU_HFBUSCLKEN0_GPIO;

	// Since the button may have decoupling caps, they may not be charged
	// after a power-on and could give a false positive result. To avoid
	// this issue, drive the output as an output for a short time to charge
	// them up as quickly as possible.
	GPIO_PinModeSet(BTL_GPIO_ACTIVATION_PORT,
	                BTL_GPIO_ACTIVATION_PIN,
	                gpioModePushPull,
	                BTL_GPIO_ACTIVATION_POLARITY);
	for (volatile int i = 0; i < 100; i++);

	// Reconfigure as an input with pull(up|down) to read the button state
	GPIO_PinModeSet(BTL_GPIO_ACTIVATION_PORT,
	                BTL_GPIO_ACTIVATION_PIN,
	                gpioModeInputPull,
	                BTL_GPIO_ACTIVATION_POLARITY);

	// We have to delay again here so that if the button is depressed the
	// cap has time to discharge again after being charged up by the above delay
	for (volatile int i = 0; i < 500; i++);

	pressed = GPIO_PinInGet(BTL_GPIO_ACTIVATION_PORT, BTL_GPIO_ACTIVATION_PIN)
	          != BTL_GPIO_ACTIVATION_POLARITY;

	// Disable GPIO pin
	GPIO_PinModeSet(BTL_GPIO_ACTIVATION_PORT,
	                BTL_GPIO_ACTIVATION_PIN,
	                gpioModeDisabled,
	                BTL_GPIO_ACTIVATION_POLARITY);

	// Disable GPIO clock
	CMU->HFBUSCLKEN0 &= ~CMU_HFBUSCLKEN0_GPIO;

	return pressed;
}

bool is_sw_reset(void)
{
	uint16_t reset_reason = 0;

	// map reset cause structure to the begin of crash info memory
	BootloaderResetCause_t * reset_cause = (BootloaderResetCause_t *) &__CRASHINFO__begin;

	// if the signature is valid, then use reset reason direction
	if (reset_cause->signature == BOOTLOADER_RESET_SIGNATURE_VALID)
	{
		reset_reason = reset_cause->reason;
	}

	// otherwise assign some reason
	else
	{
		// if the signature is not set
		if (reset_cause->signature == 0)
		{
			reset_reason = BOOTLOADER_RESET_REASON_BOOTLOAD;
			if (reset_cause->reason != 1)
			{
				reset_reason = BOOTLOADER_RESET_REASON_UNKNOWN;
			}
		}
		else
		{
			reset_reason = BOOTLOADER_RESET_REASON_UNKNOWN;
		}
	}

	// enter the bootloader only if the reboot is requested by application (not any hardware failure)
	if (RMU->RSTCAUSE & RMU_RSTCAUSE_SYSREQRST)
	{
		// depending on reset reason, we will decide whether to enter bootloader
		switch(reset_reason)
		{
			case BOOTLOADER_RESET_REASON_BOOTLOAD:
			case BOOTLOADER_RESET_REASON_FORCE:
			case BOOTLOADER_RESET_REASON_UPGRADE:
			case BOOTLOADER_RESET_REASON_BADAPP:
				return true;

			default:
				break;
		}
	}

	return false;
}

int main(void)
{
	// set high frequency clock
	SystemCoreClock = SystemHfrcoFreq;

	// runtime patch
	CHIP_Init();

	// check if button is pressed
	if (is_button_override())
	{
		bootloader();
	}

	// check if other request to boot into bootloader
	if (is_sw_reset())
	{
		bootloader();
	}

	// in this step we need to boot to application
	// if specific bit in gbpr is set, we are going to boot to specific address, otherwise we are going to boot to default
	// application
	if (simgbpr_is_valid())
	{
		// bit 0 indicates boot to specific address
		if (simgpbr_get(0UL))
		{
			uint32_t vtor_addr = simgpbr_get(1UL);

			// clear the entry for gpbr
			simgpbr_set(0UL, 0UL);
			simgpbr_set(1UL, 0UL);

			branch_to_addr(vtor_addr);
		}
	}

	// depending on the last boot partition, unless there is special request that requires to boot to other partition,
	// (has handled in previous case, the bootloader should boot the application to its last boot address, or default
	// address. 

	// otherwise boot to default application (broken)

	while (1)
	{

	}
}
