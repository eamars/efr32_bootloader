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

#if (BOARD_DEV == 1 || BOARD_HATCH == 1 || BOARD_HATCH_OUTDOOR == 1)
		// Green LED
		GPIO_PinModeSet(BSP_LED0_PORT,
		                BSP_LED0_PIN,
		                gpioModeInputPull,
		                (uint32_t) green_led_state);
#endif

		green_led_state = !green_led_state;
	}
}

void bootloader(void)
{
	// runtime patch
	CHIP_Init();

	// enable clock to the GPIO to allow input to be configured
	CMU_ClockEnable(cmuClock_GPIO, true);

#if (BOARD_HATCH == 1 || BOARD_HATCH_OUTDOOR == 1)
	// only valid for hatch and hatch outdoor
	// LED Anode
	GPIO_PinModeSet(BSP_LED_EN_PORT,
	                BSP_LED_EN_PIN,
	                gpioModePushPull,
	                1);

	// enable logic shifter
	GPIO_PinModeSet(BSP_UART_LS_EN_PORT,
	                BSP_UART_LS_EN_PIN,
	                gpioModePushPull,
	                1);
#endif

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
#if (BOARD_DEV == 1 || BOARD_HATCH == 1 || BOARD_HATCH_OUTDOOR == 1)
			// Blue LED
			GPIO_PinModeSet(BSP_LED1_PORT,
			                BSP_LED1_PIN,
			                gpioModeInputPull,
			                (uint32_t) blue_led_state);
			blue_led_state = !blue_led_state;
#endif

			communication_receive(&comm);
		}
	}
}

bool is_button_override(void)
{
#if (BOARD_HATCH == 1 || BOARD_HATCH_OUTDOOR == 1 || BOARD_NCP == 1 || BOARD_DEV == 1)
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
#if (BOARD_HATCH == 1 || BOARD_HATCH_OUTDOOR == 1)
	bool valid_app = false;

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
	eeprom_cat24c16_selective_read(&eeprom_device, 0x0, sizeof(ExtendedBootloaderResetCause_t), &reset_cause);

	// read reset cause, make sure address falls within valid flash range
	if (reset_cause.basicResetCause.signature == BOOTLOADER_RESET_SIGNATURE_VALID &&
	    reset_cause.basicResetCause.reason == BOOTLOADER_RESET_REASON_GO &&
	    reset_cause.app_signature == APP_SIGNATURE &&
	    is_valid_address(reset_cause.app_addr))
	{
		// verify the CRC?

		// set address
		*app_addr = reset_cause.app_addr;

		valid_app = true;
	}

	// disable eeprom
	eeprom_cat24c16_deinit(&eeprom_device);

	// disable i2c
	i2cdrv_deinit(&i2c_device);

	// disable GPIO clock
	CMU_ClockEnable(cmuClock_GPIO, false);

	return valid_app;

#elif BOARD_DEV == 1
	// make up the AAT address for development board
	*app_addr = 0x100UL;
	return true;

#elif BOARD_NCP
	// make up the AAT address for network co-processor
	// the application address starts at 0x0UL
	*app_addr = 0x0UL;
	return true;
#elif BOARD_NCP_MODULE == 1
	// For testing purpose, we load firmware at 0x100
	*app_addr = 0x100UL;
	return true;
#else
#error "Unknown board"
#endif
}

void clear_boot_request(void)
{
	// map reset cause structure to the begin of crash info memory
	ExtendedBootloaderResetCause_t * reset_cause = (ExtendedBootloaderResetCause_t *) &__CRASHINFO__begin;

	reset_cause->app_signature = 0UL;
	reset_cause->app_addr = INVALID_BASE_ADDR;
}
