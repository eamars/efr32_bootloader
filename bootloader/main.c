/**
 * @brief
 *
 */
#include <stdint.h>

#include "em_chip.h"

#include "irq.h"
#include "bootloader.h"
#include "bootloader_api.h"

int main(void)
{
    uint32_t app_addr;

    // runtime patch
    CHIP_Init();

    // check if button is pressed
    if (is_button_override())
    {
        bootloader();
    }

    // check if hardware failure
    if (is_hardware_failure())
    {
        trap();
    }

    // check if other request to boot into bootloader
    if (is_boot_to_bootloader())
    {
        bootloader();
    }

    // if there is no request to boot to bootloader, we then check if there is special request for booting into
    // specific application
    app_addr = INVALID_BASE_ADDR;
    if (is_boot_to_app(&app_addr))
    {
        clear_boot_request();
        branch_to_addr(app_addr);
    }

    // depending on the last boot partition, unless there is special request that requires to boot to other partition,
    // (has handled in previous case, the bootloader should boot the application to its last boot address, or default
    // address.
    app_addr = INVALID_BASE_ADDR;
    if (is_boot_to_prev_app(&app_addr))
    {
        branch_to_addr(app_addr);
    }

    // otherwise boot to bootloader
    bootloader();

    // trap here

    while (1)
    {

    }
}


__attribute__ ((optimize("-O0")))
void HardFault_Handler(void)
{
    HFSR_t hfsr;
    hfsr.HFSR = SCB->HFSR;

    while (1)
    {

    }
}
