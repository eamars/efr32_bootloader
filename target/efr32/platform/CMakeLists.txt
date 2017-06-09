MESSAGE("ENTER ${CMAKE_CURRENT_SOURCE_DIR}")

# get module name
GET_FILENAME_COMPONENT(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

# Collect all source file
SET (SRC
		"base/hal/plugin/sim-eeprom1/sim-eeprom.c"
		"base/hal/plugin/coexistence/coexistence-efr32.c"
		"base/hal/plugin/serial/ember-printf.c"
		"base/hal/micro/cortexm3/efm32/assert-crash-handlers.c"
		"base/hal/micro/cortexm3/efm32/button.c"
		"base/hal/plugin/buzzer/buzzer-efr32.c"
		"base/hal/plugin/serial/cortexm/efm32/com.c"
		"base/hal/micro/cortexm3/efm32/cstartup-common.c"
		"base/hal/micro/cortexm3/efm32/diagnostic.c"
		"base/hal/micro/cortexm3/efm32/ext-device.c"
		"base/hal/micro/cortexm3/efm32/led.c"
		"base/hal/micro/cortexm3/efm32/mfg-token.c"
		"base/hal/micro/cortexm3/efm32/micro-common.c"
		"base/hal/micro/cortexm3/efm32/micro.c"
		"base/hal/micro/cortexm3/efm32/mpu.c"
		"base/hal/micro/cortexm3/efm32/sleep-efm32.c"
		"base/hal/micro/cortexm3/efm32/token.c"
		"base/hal/plugin/debug/rtt/SEGGER_RTT.c"
		"base/hal/plugin/antenna-stub/antenna-stub.c"
		"base/hal/plugin/adc/adc-efr32.c"
		"base/hal/micro/cortexm3/usb/cdc/callbacks.c"
		"base/hal/micro/generic/crc.c"
		"base/hal/micro/cortexm3/usb/cdc/descriptors.c"
		"base/hal/micro/cortexm3/usb/em_usbd.c"
		"base/hal/micro/cortexm3/usb/em_usbdch9.c"
		"base/hal/micro/cortexm3/usb/em_usbdep.c"
		"base/hal/micro/cortexm3/usb/em_usbhal.c"
		"base/hal/micro/cortexm3/usb/em_usbint.c"
		"base/hal/micro/cortexm3/usb/em_usbtimer.c"
		"base/hal/ember-base-configuration.c"
		"base/hal/micro/generic/endian.c"
		"base/hal/micro/generic/mem-util.c"
		"base/hal/micro/generic/random.c"
		"base/hal/plugin/serial/serial.c"
		"base/hal/micro/generic/token-def.c"
)

# include essential driver headers
INCLUDE_DIRECTORIES(${TARGET_SPECIFIC_INCLUDE_DIRECTORY})
INCLUDE_DIRECTORIES(${BOARD_SPECIFIC_INCLUDE_DIRECTORY})

# build library
ADD_LIBRARY (${LIB_NAME} ${SRC})
