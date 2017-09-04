/**
 * @brief Bootloader implementation
 * @file bootloader.c
 * @author Ran Bao
 * @date Aug, 2017
 */
#include <string.h>

#include BOARD_HEADER
#include "bootloader.h"
#include "bootloader_config.h"
#include "bootloader_api.h"

#include "em_device.h"
#include "em_cmu.h"
#include "em_chip.h"
#include "em_rmu.h"
#include "em_gpio.h"
#include "em_msc.h"

#include "btl_reset.h"
#include "btl_reset_info.h"

#include "io_device.h"
#include "communication.h"
#include "uart_device.h"
#include "eeprom_cat24c16.h"



// function that is implemented in another file
extern void comm_cb_inst(const void *handler, uint8_t *data, uint8_t size);
extern void comm_cb_data(uint16_t block_offset, uint8_t *data, uint8_t size);
extern bool is_valid_address(uint32_t address);
extern uint32_t __CRASHINFO__begin;

// global variables
bootloader_config_t bootloader_config;

// blue led state
bool blue_led_state = 0;



void bootloader(void)
{
	// runtime patch
	CHIP_Init();

	// enable clock to the GPIO to allow input to be configured
	CMU_ClockEnable(cmuClock_GPIO, true);

	// Anode
	GPIO_PinModeSet(gpioPortA,
	                0,
	                gpioModePushPull,
	                1);

	// enable logic shifter
	GPIO_PinModeSet(gpioPortC,
	                8,
	                gpioModePushPull,
	                1);

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
			// Blue LED
			GPIO_PinModeSet(gpioPortA,
			                2,
			                gpioModeInputPull,
			                (uint32_t) blue_led_state);
			blue_led_state = !blue_led_state;

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

bool is_boot_request_override(uint32_t * app_addr)
{
	// map reset cause structure to the begin of crash info memory
	ExtendedBootloaderResetCause_t * reset_cause = (ExtendedBootloaderResetCause_t *) &__CRASHINFO__begin;

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


bool is_prev_app_valid(uint32_t * app_addr)
{
#if BOARD_HATCH == 1
	// initialize i2c driver and eeprom driver here
	i2cdrv_t i2c_device;
	eeprom_cat24c16_t eeprom_device;
	ExtendedBootloaderResetCause_t reset_cause;

	// initialize i2c driver
	memset(&i2c_device, 0x0, sizeof(i2cdrv_t));
	i2cdrv_init(&i2c_device,
	            PIO_DEF(BSP_I2C_SDA_PORT, BSP_I2C_SDA_PIN),
	            PIO_DEF(BSP_I2C_SCL_PORT, BSP_I2C_SCL_PIN),
	            PIO_DEF(BSP_I2C_EN_PORT, BSP_I2C_EN_PIN)
	);

	// initialize eeprom driver
	memset(&eeprom_device, 0x0, sizeof(eeprom_cat24c16_t));
	eeprom_cat24c16_init(&eeprom_device, &i2c_device, PIO_DEF(BSP_EEPROM_EN_PORT, BSP_EEPROM_EN_PIN));

	// read 16bytes from first page
	memset(&reset_cause, 0x0, sizeof(ExtendedBootloaderResetCause_t));
	eeprom_cat24c16_selective_read(&eeprom_device, 0x0, sizeof(ExtendedBootloaderResetCause_t), (uint8_t *) &reset_cause);

	// read reset cause, make sure address falls within valid flash range
	if (reset_cause.basicResetCause.signature == BOOTLOADER_RESET_SIGNATURE_VALID &&
		reset_cause.basicResetCause.reason == BOOTLOADER_RESET_REASON_GO &&
	    reset_cause.app_signature == APP_SIGNATURE &&
	    is_valid_address(reset_cause.app_addr))
	{
		// verify the CRC?

		// set address
		*app_addr = reset_cause.app_addr;

		return true;
	}

	return false;
#else
	return false;
#endif
}

void clear_boot_request(void)
{
	// map reset cause structure to the begin of crash info memory
	ExtendedBootloaderResetCause_t * reset_cause = (ExtendedBootloaderResetCause_t *) &__CRASHINFO__begin;

	reset_cause->app_signature = 0UL;
	reset_cause->app_addr = INVALID_BASE_ADDR;
}
