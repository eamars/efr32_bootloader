MESSAGE("ENTER ${CMAKE_CURRENT_SOURCE_DIR}")

# override build flags
SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -mfloat-abi=softfp -Os")
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -mfloat-abi=softfp -Os")
SET(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -Os --specs=nano.specs -mfloat-abi=softfp -T${TARGET_ROOT_DIRECTORY}/scripts/efr32mg12p_bootloader.ld -Wl,--Map=symbols.map,--gc-sections")

# override executable
SET(HATCH_EXECUTABLE "wg_bootloader")

# depending on boards, we apply different build definitons
ADD_DEFINITIONS(
		-D${BOARD}=1
		-DCORTEXM3=1
		-DCORTEXM3_EFM32_MICRO=1
		-DCORTEXM3_EFR32=1
		-DCORTEXM3_EFR32_MICRO=1
		-DEFR32_SERIES1_CONFIG2_MICRO=1
		-DEFR32MG12P=1
		-DPLATFORM_HEADER="platform/base/hal/micro/cortexm3/compiler/gcc.h"
		-DBTL_CONFIG_FILE="bootloader_config.h"
)
IF ("${BOARD}" STREQUAL "BOARD_DEV")
	MESSAGE("USING: EFR32MG12P432F1024GL125")
	ADD_DEFINITIONS(
			-DBOARD_HEADER="board_dev.h"
			-DCORTEXM3_EFR32MG12P433F1024GL125=1
			-DCORTEXM3_EFR32MG12P433F1024GL125_MICRO=1
			-DEFR32MG12P433F1024GL125=1
	)

ELSEIF("${BOARD}" STREQUAL "BOARD_HATCH_OUTDOOR_V2")
	MESSAGE("USING: EFR32MG12P433F1024GM48")
	ADD_DEFINITIONS(
			-DBOARD_HEADER="board_hatch_outdoor_v2.h"
			-DCORTEXM3_EFR32MG12P433F1024GL125=1
			-DCORTEXM3_EFR32MG12P433F1024GL125_MICRO=1
			-DEFR32MG12P433F1024GL125=1
	)
ENDIF()

# Includes
SET(TARGET_SPECIFIC_INCLUDE_DIRECTORY
		${TARGET_SPECIFIC_INCLUDE_DIRECTORY}


		${TARGET_ROOT_DIRECTORY}/drivers
		${TARGET_ROOT_DIRECTORY}/freertos_trace/Include
		${TARGET_ROOT_DIRECTORY}/platform/emlib/inc
		${TARGET_ROOT_DIRECTORY}/platform/Device/SiliconLabs/EFR32MG12P/Include
		${TARGET_ROOT_DIRECTORY}/platform/CMSIS/Include

		${TARGET_ROOT_DIRECTORY}/platform/emdrv/common/inc
		${TARGET_ROOT_DIRECTORY}/platform/emdrv/spidrv/inc
		${TARGET_ROOT_DIRECTORY}/platform/emdrv/ustimer/inc
		${TARGET_ROOT_DIRECTORY}/platform/emdrv/gpiointerrupt/inc
		${TARGET_ROOT_DIRECTORY}/platform/emdrv/rtcdrv/inc
		${TARGET_ROOT_DIRECTORY}/platform/emdrv/dmadrv/inc
		${TARGET_ROOT_DIRECTORY}/platform/emdrv/sleep/inc
		${TARGET_ROOT_DIRECTORY}/platform/emdrv/tempdrv/inc
		${TARGET_ROOT_DIRECTORY}/platform/emdrv/uartdrv/inc

		${TARGET_ROOT_DIRECTORY}/platform/radio/rail_lib/chip/efr32/rf/common/cortex
		${TARGET_ROOT_DIRECTORY}/platform/radio/rail_lib/chip/efr32
		${TARGET_ROOT_DIRECTORY}/platform/radio/rail_lib/common
		${TARGET_ROOT_DIRECTORY}/platform/radio/rail_lib/protocol/ieee802154
		${TARGET_ROOT_DIRECTORY}/platform/radio/rail_lib/plugin/pa-conversions

		${TARGET_ROOT_DIRECTORY}/platform/halconfig/inc/hal-config



		"${TARGET_ROOT_DIRECTORY}/middleware/glib"
		"${TARGET_ROOT_DIRECTORY}/middleware/glib/glib"

		"${TARGET_ROOT_DIRECTORY}/protocol"
		${TARGET_ROOT_DIRECTORY}/protocol/flex_1.2
		${TARGET_ROOT_DIRECTORY}/util/silicon_labs/silabs_core/memory_manager/

		"${TARGET_ROOT_DIRECTORY}/platform"
		"${TARGET_ROOT_DIRECTORY}/platform/base"
		"${TARGET_ROOT_DIRECTORY}/platform/base/hal"
		"${TARGET_ROOT_DIRECTORY}/platform/base/hal/micro"
		"${TARGET_ROOT_DIRECTORY}/platform/base/hal/micro/generic/compiler"
		"${TARGET_ROOT_DIRECTORY}/platform/base/hal/micro/cortexm3/usb/cdc"
		"${TARGET_ROOT_DIRECTORY}/platform/base/hal/micro/cortexm3/compiler"
		"${TARGET_ROOT_DIRECTORY}/platform/base/hal/micro/cortexm3/efm32"
		"${TARGET_ROOT_DIRECTORY}/platform/base/hal/micro/cortexm3/efm32/config"
		"${TARGET_ROOT_DIRECTORY}/platform/base/hal/plugin/debug/rtt"
		"${TARGET_ROOT_DIRECTORY}/platform/base/hal/plugin"
		"${TARGET_ROOT_DIRECTORY}/platform/base/hal/plugin/serial"

		${TARGET_ROOT_DIRECTORY}/platform/bootloader
		${TARGET_ROOT_DIRECTORY}/platform/bootloader/driver
		${TARGET_ROOT_DIRECTORY}/platform/bootloader/api
		${TARGET_ROOT_DIRECTORY}/platform/bootloader/core

		"${TARGET_ROOT_DIRECTORY}/hardware/kit/common/bsp"
		"${TARGET_ROOT_DIRECTORY}/hardware/kit/common/drivers"
		)

# add configure file
CONFIGURE_FILE(${TARGET_ROOT_DIRECTORY}/bl_header_block.c.in header_block.c)

# Collect all source file
FILE (GLOB LOCAL_SRC *.c)

SET(EXT_SRC
		${TARGET_ROOT_DIRECTORY}/platform/emlib/src/em_adc.c
		${TARGET_ROOT_DIRECTORY}/platform/emlib/src/em_cmu.c
		${TARGET_ROOT_DIRECTORY}/platform/emlib/src/em_core.c
		${TARGET_ROOT_DIRECTORY}/platform/emlib/src/em_emu.c
		${TARGET_ROOT_DIRECTORY}/platform/emlib/src/em_gpio.c
		${TARGET_ROOT_DIRECTORY}/platform/emlib/src/em_i2c.c
		${TARGET_ROOT_DIRECTORY}/platform/emlib/src/em_ldma.c
		${TARGET_ROOT_DIRECTORY}/platform/emlib/src/em_leuart.c
		${TARGET_ROOT_DIRECTORY}/platform/emlib/src/em_mpu.c
		${TARGET_ROOT_DIRECTORY}/platform/emlib/src/em_msc.c
		${TARGET_ROOT_DIRECTORY}/platform/emlib/src/em_prs.c
		${TARGET_ROOT_DIRECTORY}/platform/emlib/src/em_rmu.c
		${TARGET_ROOT_DIRECTORY}/platform/emlib/src/em_rtcc.c
		${TARGET_ROOT_DIRECTORY}/platform/emlib/src/em_system.c
		${TARGET_ROOT_DIRECTORY}/platform/emlib/src/em_timer.c
		${TARGET_ROOT_DIRECTORY}/platform/emlib/src/em_usart.c
		${TARGET_ROOT_DIRECTORY}/platform/emlib/src/em_wdog.c

		${TARGET_ROOT_DIRECTORY}/platform/emdrv/dmadrv/src/dmadrv.c
		${TARGET_ROOT_DIRECTORY}/platform/emdrv/gpiointerrupt/src/gpiointerrupt.c
		${TARGET_ROOT_DIRECTORY}/platform/emdrv/rtcdrv/src/rtcdriver.c
		${TARGET_ROOT_DIRECTORY}/platform/emdrv/sleep/src/sleep.c
		${TARGET_ROOT_DIRECTORY}/platform/emdrv/tempdrv/src/tempdrv.c
		${TARGET_ROOT_DIRECTORY}/platform/emdrv/ustimer/src/ustimer.c
		${TARGET_ROOT_DIRECTORY}/platform/emdrv/uartdrv/src/uartdrv.c
		${TARGET_ROOT_DIRECTORY}/platform/emdrv/spidrv/src/spidrv.c

		${TARGET_ROOT_DIRECTORY}/platform/bootloader/driver/btl_driver_uart.c
		${TARGET_ROOT_DIRECTORY}/platform/bootloader/driver/btl_driver_delay.c

		${TARGET_ROOT_DIRECTORY}/platform/Device/SiliconLabs/EFR32MG12P/Source/system_efr32mg12p.c
		)

# include configurations from current folder
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

# add dependency
ADD_SUBDIRECTORY(${PROJECT_SOURCE_DIR}/target ${CMAKE_CURRENT_BINARY_DIR}/__target)

# include device specific configurations
INCLUDE_DIRECTORIES(${TARGET_SPECIFIC_INCLUDE_DIRECTORY})

# include board specific configurations
INCLUDE_DIRECTORIES(${BOARD_SPECIFIC_INCLUDE_DIRECTORY})

# include cmsis definitions
INCLUDE_DIRECTORIES(${CMSIS_INCLUDE_DIRECTORY})

# build source code as library
ADD_EXECUTABLE (${HATCH_EXECUTABLE}.${ELF_EXTENSION} ${LOCAL_SRC} ${EXT_SRC})
ADD_DEPENDENCIES(${HATCH_EXECUTABLE}.${ELF_EXTENSION} target)
TARGET_LINK_LIBRARIES(${HATCH_EXECUTABLE}.${ELF_EXTENSION} drivers efr32)

# include size command
INCLUDE("${PROJECT_SOURCE_DIR}/cmake/size.cmake")

# strip debug info from executable
ADD_CUSTOM_COMMAND(
		TARGET
		${HATCH_EXECUTABLE}.${ELF_EXTENSION}
		POST_BUILD
		COMMAND ${CMAKE_OBJCOPY} -O binary "${EXECUTABLE_OUTPUT_PATH}/${HATCH_EXECUTABLE}.${ELF_EXTENSION}" "${EXECUTABLE_OUTPUT_PATH}/${HATCH_EXECUTABLE}.${BINARY_EXTENSION}"
)

ADD_CUSTOM_COMMAND(
		TARGET
		${HATCH_EXECUTABLE}.${ELF_EXTENSION}
		POST_BUILD
		COMMAND ${CMAKE_OBJCOPY} -O srec "${EXECUTABLE_OUTPUT_PATH}/${HATCH_EXECUTABLE}.${ELF_EXTENSION}" "${EXECUTABLE_OUTPUT_PATH}/${HATCH_EXECUTABLE}.${SREC_EXTENSION}"
)

ADD_CUSTOM_COMMAND(
		TARGET
		${HATCH_EXECUTABLE}.${ELF_EXTENSION}
		POST_BUILD
		COMMAND ${CMAKE_OBJCOPY} -O ihex "${EXECUTABLE_OUTPUT_PATH}/${HATCH_EXECUTABLE}.${ELF_EXTENSION}" "${EXECUTABLE_OUTPUT_PATH}/${HATCH_EXECUTABLE}.${HEX_EXTENSION}"
)
