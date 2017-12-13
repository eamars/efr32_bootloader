/**
 * @brief Bootloader implementation
 * @file bootloader.c
 * @author Ran Bao
 * @date Aug, 2017
 */
#include <string.h>

#include BOARD_HEADER
#include PLATFORM_HEADER
#include "bootloader.h"
#include "bootloader_api.h"

#include "em_device.h"
#include "em_cmu.h"
#include "em_chip.h"
#include "em_rmu.h"
#include "em_gpio.h"
#include "em_msc.h"
#include "em_dbg.h"

#include "btl_reset.h"
#include "btl_reset_info.h"

#include "io_device.h"
#include "communication.h"
#include "uart_device.h"
#include "eeprom_cat24c16.h"
#include "bits.h"


// function that is implemented in another file
extern void comm_cb_inst(const void *handler, uint8_t *data, uint8_t size);
extern void comm_cb_data(uint16_t block_offset, uint8_t *data, uint8_t size);
extern bool is_valid_address(uint32_t address);

// global variables
bootloader_config_t bootloader_config;

// Blink LED related
static uint32_t led_counter = 0;
static bool green_led_state = 0;
static bool blue_led_state = 0;

// systick counter
static volatile uint32_t systick_counter = 0UL;

void SysTick_Handler(void)
{
    systick_counter += 1;

    if (systick_counter >= led_counter)
    {
        led_counter += 1000;

#if (BOARD_DEV == 1 || BOARD_HATCH_OUTDOOR_V2 == 1)
        GPIO_PinModeSet(BTL_LED0_PORT,
                        BTL_LED0_PIN,
                        gpioModeInputPull,
                        (uint32_t) green_led_state);
#endif

        green_led_state = !green_led_state;
    }
}


void store_reset_reason(void)
{
    uint32_t reset_reason;

    ExtendedBootloaderResetCause_t * reset_cause = (ExtendedBootloaderResetCause_t *) &__RESETINFO__begin;

    reset_cause->basicResetCause.reason |= (uint16_t) (RMU_ResetCauseGet() & 0xffff);
    RMU_ResetCauseClear();
}


void bootloader(void)
{
    // keep the reset reason
    // store_reset_reason();

    // enable clock to the GPIO to allow input to be configured
    CMU_ClockEnable(cmuClock_GPIO, true);

    // enable systick
    SysTick_Config(CMU_ClockFreqGet( cmuClock_CORE ) / 1000);

    // initialize flash driver
    MSC_Init();

    uart_device_init();
    io_device * uart_device = get_uart_io_device();

    // create communication module
    communication_t comm;

    // initialize communication module (send and receive program)
    communication_init(&comm, uart_device, comm_cb_data, comm_cb_inst);

    // initialize bootloader config
    bootloader_config.base_addr = INVALID_BASE_ADDR;
    bootloader_config.block_size_exp = 0;
    stack32_init(&bootloader_config.base_addr_stack, 2);


    while (1)
    {
        while (communication_ready(&comm))
        {
#if (BOARD_DEV == 1 || BOARD_HATCH_OUTDOOR_V2 == 1)
            // Blue LED
            GPIO_PinModeSet(BTL_LED1_PORT,
                            BTL_LED1_PIN,
                            gpioModeInputPull,
                            (uint32_t) blue_led_state);
            blue_led_state = !blue_led_state;
#endif

            communication_receive(&comm);
        }
    }
}


__attribute__ ((optimize("-O0")))
void trap(void)
{
    // disable watchdog timer
    BITS_CLEAR(WDOG0->CTRL, WDOG_CTRL_EN);
    BITS_CLEAR(WDOG1->CTRL, WDOG_CTRL_EN);

    // enable debug port
    DBG_SWOEnable(1);

    // expose internal reset cause
    register volatile const ExtendedBootloaderResetCause_t * reset_cause = (ExtendedBootloaderResetCause_t *) &__RESETINFO__begin;

    // add breakpoint if debugger attached
    if (DBG_Connected())
    {
        asm volatile ("bkpt #0");
    }

    // enter loop
    while (1)
    {

    }
}


bool is_button_override(void)
{
#if (BOARD_HATCH_OUTDOOR_V2 == 1 || BOARD_DEV == 1)
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
#else
    // if no button present, then we skip the button override
    return false;
#endif
}


bool is_hardware_failure(void)
{
    // keep reset reason in SRAM
    ExtendedBootloaderResetCause_t * reset_cause = (ExtendedBootloaderResetCause_t *) &__RESETINFO__begin;
    reset_cause->rmu_reset_cause.RMU_RESET_CAUSE = RMU_ResetCauseGet();

    // clear the rmu reset reason
    RMU_ResetCauseClear();

    // read rmu reset reason
    if (reset_cause->rmu_reset_cause.EM4RST)
    {

    }

    if (reset_cause->rmu_reset_cause.WDOGRST)
    {

    }

    if (reset_cause->rmu_reset_cause.SYSREQRST)
    {

    }

    if (reset_cause->rmu_reset_cause.EXTRST)
    {

    }

    if (reset_cause->rmu_reset_cause.DECBOD)
    {
        return true;
    }

    if (reset_cause->rmu_reset_cause.DVDDBOD)
    {
        return true;
    }

    if (reset_cause->rmu_reset_cause.AVDDBOD)
    {
        return true;
    }

    if (reset_cause->rmu_reset_cause.PORST)
    {

    }

    return false;
}


bool is_boot_to_bootloader(void)
{
    uint16_t reset_reason = 0;

    // map reset cause structure to the begin of crash info memory
    const BootloaderResetCause_t * reset_cause = (BootloaderResetCause_t *) &__RESETINFO__begin;

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

bool is_boot_to_app(uint32_t * app_addr)
{
    // map reset cause structure to the begin of crash info memory
    const ExtendedBootloaderResetCause_t * reset_cause = (ExtendedBootloaderResetCause_t *) &__RESETINFO__begin;

    if (reset_cause->basicResetCause.signature == BOOTLOADER_RESET_SIGNATURE_VALID &&
        reset_cause->basicResetCause.reason == BOOTLOADER_RESET_REASON_GO &&
        reset_cause->app_signature == APP_SIGNATURE &&
        reset_cause->app_addr != INVALID_BASE_ADDR)
    {
        *app_addr = reset_cause->app_addr;

        return true;
    }

    return false;
}


bool is_boot_to_prev_app(uint32_t * app_addr)
{
    *app_addr = 0x100UL;

    return true;
}

void clear_boot_request(void)
{
    // map reset cause structure to the begin of crash info memory
    ExtendedBootloaderResetCause_t * reset_cause = (ExtendedBootloaderResetCause_t *) &__RESETINFO__begin;

    reset_cause->app_signature = 0UL;
    reset_cause->app_addr = INVALID_BASE_ADDR;
}
