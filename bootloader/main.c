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
#include "bootloader.h"
#include "bootloader_api.h"


#define USER_APPLICATION_ADDR 0x100UL





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

	// if there is no request to boot to bootloader, we then check if there is special request for booting into
	// specific application
	uint32_t app_addr = INVALID_BASE_ADDR;
	if (is_boot_request_override(&app_addr))
	{
		clear_boot_request();
		branch_to_addr(app_addr);
	}

	// depending on the last boot partition, unless there is special request that requires to boot to other partition,
	// (has handled in previous case, the bootloader should boot the application to its last boot address, or default
	// address.
	app_addr = INVALID_BASE_ADDR;
	if (is_prev_app_valid(&app_addr))
	{
		branch_to_addr(app_addr);
	}

	// otherwise boot to bootloader
	// trap here

	while (1)
	{

	}
}
