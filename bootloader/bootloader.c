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

#include "drv_debug.h"
#include "reset_info.h"

#include "io_device.h"
#include "communication.h"
#include "uart_device.h"
#include "bits.h"
#include "yield.h"


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

#if (BOARD_DEV == 1 || BOARD_HATCH_OUTDOOR_V2 == 1 || BOARD_NCP_V2 == 1)
        GPIO_PinModeSet(BTL_LED1_PORT,
                        BTL_LED1_PIN,
                        gpioModeInputPull,
                        (uint32_t) green_led_state);
#endif

        green_led_state = !green_led_state;
    }
}

__attribute__((noreturn))
void bootloader(void)
{
    // clear previous reset information
    reset_info_clear();

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
#if (BOARD_HATCH_OUTDOOR_V2 == 1 || BOARD_NCP_V2 == 1)
            // Blue LED
            GPIO_PinModeSet(BTL_LED2_PORT,
                            BTL_LED2_PIN,
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

#if (BOARD_HATCH_OUTDOOR_V2 == 1 || BOARD_DEV == 1 || BOARD_NCP_V2 == 1)
    // enable gpio
    CMU_ClockEnable(cmuClock_GPIO, true);

    // enable LED0 to indicate the error
    GPIO_PinModeSet(BTL_LED0_PORT,
                    BTL_LED0_PIN,
                    gpioModeInputPull,
                    0);
#endif

    // enable debug port
    DBG_SWOEnable(1);

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


bool button_override(void)
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


bool boot_to_bootloader(void)
{
    boot_info_map_t * boot_info_map = (boot_info_map_t *) reset_info_read();

    if (reset_info_get_valid() &&
            boot_info_map->reset_info_common.reset_reason == RESET_BOOTLOADER_BOOTLOAD)
    {
        return true;
    }

    return false;
}


bool boot_to_application(uint32_t * aat_addr)
{
    boot_info_map_t * boot_info_map = (boot_info_map_t *) reset_info_read();

    if (reset_info_get_valid() &&
            boot_info_map->reset_info_common.reset_reason == RESET_BOOTLOADER_GO)
    {
        *aat_addr = boot_info_map->next_aat_addr;
        return true;
    }

    return false;
}


bool boot_to_prev_application(uint32_t * aat_addr)
{
    // TODO: read from persist memory
    *aat_addr = 0x100;
    return true;
}


void rmu_reset_reason_dump(void)
{
    // read reset caused by rmu
    uint32_t rmu_reset_cause = RMU_ResetCauseGet();

    // clear the previous reset reasons
    RMU_ResetCauseClear();

    // store the full rmu reset reason
    reset_info_map_t * reset_info_map = (reset_info_map_t *) reset_info_read();
    reset_info_map->reset_info_common.rmu_reset_cause.RMU_RESET_CAUSE = rmu_reset_cause;

    // if the reset info is invalid or unknown, then it will try to decode reset reason and write to reset info region
    if (!reset_info_get_valid() || reset_info_map->reset_info_common.reset_reason == RESET_UNKNOWN)
    {
        if (reset_info_map->reset_info_common.rmu_reset_cause.PORST)
        {
            reset_info_map->reset_info_common.reset_reason = RESET_POWERON_HV; // TODO: what's this?
        }

        if (reset_info_map->reset_info_common.rmu_reset_cause.AVDDBOD)
        {
            reset_info_map->reset_info_common.reset_reason = RESET_BROWNOUT_AVDD0;
        }

        if (reset_info_map->reset_info_common.rmu_reset_cause.DVDDBOD)
        {
            reset_info_map->reset_info_common.reset_reason = RESET_BROWNOUT_DVDD;
        }

        if (reset_info_map->reset_info_common.rmu_reset_cause.DECBOD)
        {
            reset_info_map->reset_info_common.reset_reason = RESET_BROWNOUT_DEC;
        }

        if (reset_info_map->reset_info_common.rmu_reset_cause.EXTRST)
        {
            reset_info_map->reset_info_common.reset_reason = RESET_EXTERNAL_PIN;
        }

        if (reset_info_map->reset_info_common.rmu_reset_cause.LOCKUPRST)
        {
            reset_info_map->reset_info_common.reset_reason = RESET_FATAL_LOCKUP;
        }

        if (reset_info_map->reset_info_common.rmu_reset_cause.SYSREQRST)
        {
            reset_info_map->reset_info_common.reset_reason = RESET_SOFTWARE_REBOOT;
        }

        if (reset_info_map->reset_info_common.rmu_reset_cause.WDOGRST)
        {
            reset_info_map->reset_info_common.reset_reason = RESET_WATCHDOG_EXPIRED;
        }

        if (reset_info_map->reset_info_common.rmu_reset_cause.EM4RST)
        {
            reset_info_map->reset_info_common.reset_reason = RESET_SOFTWARE_EM4;
        }

        reset_info_set_valid();
    }
}

void trap_on_hardware_failure(void)
{
    // currently the hardware failure is detected by value read from RMU
    reset_info_map_t * reset_info_map = (reset_info_map_t *) reset_info_read();

    YIELD
    (
        if (reset_info_map->reset_info_common.rmu_reset_cause.AVDDBOD)
        {
            break;
        }

        if (reset_info_map->reset_info_common.rmu_reset_cause.DVDDBOD)
        {
            break;
        }

        if (reset_info_map->reset_info_common.rmu_reset_cause.DECBOD)
        {
            break;
        }

        return;
    );

    trap();
}
