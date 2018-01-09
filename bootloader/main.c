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
    uint32_t aat_addr;

    // runtime patch
    CHIP_Init();

    // read hardware reset reason from
    rmu_reset_reason_dump();

    // trap on hardware failure
    trap_on_hardware_failure();

    // check if button is pressed
    // if the button is pressed then
    if (button_override())
    {
        bootloader();
    }

    if (boot_to_bootloader())
    {
        bootloader();
    }

    aat_addr = 0;
    if (boot_to_application(&aat_addr))
    {
        branch_to_addr(aat_addr);
    }

    aat_addr = 0;
    if (boot_to_prev_application(&aat_addr))
    {
        branch_to_addr(aat_addr);
    }

    bootloader();

    trap();
}

// override default fault handler for smaller code size
void fault(void)
{
    while(1);
}
