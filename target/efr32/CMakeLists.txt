# configuration for target
MESSAGE("ENTER ${CMAKE_CURRENT_SOURCE_DIR}")

# get module name
GET_FILENAME_COMPONENT(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

# Collect all source file
FILE (GLOB EFR32_SRC *.c)


# define C flags
SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g3 -W -gdwarf-2 -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=softfp -mthumb -std=gnu11 -Wall -Wextra -Wno-unused -Wno-unused-parameter -fmessage-length=0 -ffunction-sections -fdata-sections")
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g3 -W -gdwarf-2 -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=softfp -mthumb -std=gnu11 -Wall -Wextra -Wno-unused -Wno-unused-parameter -fmessage-length=0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti")
SET(CMAKE_ASM_FLAGS_DEBUG "${CMAKE_ASM_FLAGS_DEBUG} -gdwarf-2 -mcpu=cortex-m4 -mthumb -c -x assembler-with-cpp")

# DEBUG LD FLAGS
SET(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -mthumb -Wl,--no-wchar-size-warning,--Map=symbols.map,--gc-sections -gdwarf-2 -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=softfp -lc -lm")

SET(TARGET_SPECIFIC_INCLUDE_DIRECTORY
		"${TARGET_SPECIFIC_INCLUDE_DIRECTORY}"

		"${CMAKE_CURRENT_SOURCE_DIR}"
		"${CMAKE_CURRENT_SOURCE_DIR}/CMSIS/Include"
		"${CMAKE_CURRENT_SOURCE_DIR}/emlib/Inc"
		"${CMAKE_CURRENT_SOURCE_DIR}/hal/inc"
		"${CMAKE_CURRENT_SOURCE_DIR}/rail/Inc"
		"${CMAKE_CURRENT_SOURCE_DIR}/efr32mg12p"

		"${CMAKE_CURRENT_SOURCE_DIR}/middleware/glib"
		"${CMAKE_CURRENT_SOURCE_DIR}/middleware/glib/glib"

		"${CMAKE_CURRENT_SOURCE_DIR}/protocol"
		"${CMAKE_CURRENT_SOURCE_DIR}/protocol/zigbee_5.9"
		"${CMAKE_CURRENT_SOURCE_DIR}/protocol/zigbee_5.9/stack"
		"${CMAKE_CURRENT_SOURCE_DIR}/protocol/zigbee_5.9/app"
		"${CMAKE_CURRENT_SOURCE_DIR}/protocol/zigbee_5.9/framework"
		"${CMAKE_CURRENT_SOURCE_DIR}/protocol/zigbee_5.9/app/framework/security"
		"${CMAKE_CURRENT_SOURCE_DIR}/protocol/zigbee_5.9/app/framework/include"
		"${CMAKE_CURRENT_SOURCE_DIR}/protocol/zigbee_5.9/app/framework/util"
		"${CMAKE_CURRENT_SOURCE_DIR}/protocol/zigbee_5.9/app/framework/plugin/network-steering"
		"${CMAKE_CURRENT_SOURCE_DIR}/protocol/zigbee_5.9/app/framework/plugin/scan-dispatch"
		"${CMAKE_CURRENT_SOURCE_DIR}/protocol/zigbee_5.9/app/framework/plugin/update-tc-link-key"
		"${CMAKE_CURRENT_SOURCE_DIR}/protocol/zigbee_5.9/app/framework/plugin/reporting"
		"${CMAKE_CURRENT_SOURCE_DIR}/protocol/zigbee_5.9/app/framework/plugin/zll-commissioning"
		"${CMAKE_CURRENT_SOURCE_DIR}/protocol/zigbee_5.9/app/framework/plugin/zll-on-off-server"
		"${CMAKE_CURRENT_SOURCE_DIR}/protocol/zigbee_5.9/app/framework/plugin/zll-level-control-server"
		"${CMAKE_CURRENT_SOURCE_DIR}/protocol/zigbee_5.9/app/framework/plugin/find-and-bind-initiator"
		"${CMAKE_CURRENT_SOURCE_DIR}/protocol/zigbee_5.9/app/framework/plugin/counters"
		"${CMAKE_CURRENT_SOURCE_DIR}/protocol/zigbee_5.9/app/framework/cli"
		"${CMAKE_CURRENT_SOURCE_DIR}/protocol/zigbee_5.9/app/util/zigbee-framework"
		"${CMAKE_CURRENT_SOURCE_DIR}/protocol/zigbee_5.9/app/util/counters"
		"${CMAKE_CURRENT_SOURCE_DIR}/protocol/zigbee_5.9/app/util/common"
		"${CMAKE_CURRENT_SOURCE_DIR}/protocol/zigbee_5.9/app/framework/plugin/interpan"

		"${CMAKE_CURRENT_SOURCE_DIR}/platform/base"
		"${CMAKE_CURRENT_SOURCE_DIR}/platform/base/hal"
		"${CMAKE_CURRENT_SOURCE_DIR}/platform/base/hal/micro"
		"${CMAKE_CURRENT_SOURCE_DIR}/platform/base/hal/micro/generic/compiler"
		"${CMAKE_CURRENT_SOURCE_DIR}/platform/base/hal/micro/cortexm3/usb/cdc"
		"${CMAKE_CURRENT_SOURCE_DIR}/platform/base/hal/micro/cortexm3/compiler"
		"${CMAKE_CURRENT_SOURCE_DIR}/platform/base/hal/micro/cortexm3/efm32"
		"${CMAKE_CURRENT_SOURCE_DIR}/platform/base/hal/micro/cortexm3/efm32/config"
		"${CMAKE_CURRENT_SOURCE_DIR}/platform/base/hal/plugin/debug/rtt"
		"${CMAKE_CURRENT_SOURCE_DIR}/platform/base/hal/plugin"
		"${CMAKE_CURRENT_SOURCE_DIR}/platform/base/hal/plugin/serial"

		"${CMAKE_CURRENT_SOURCE_DIR}/hardware/kit/common/bsp"
		)

SET(BOARD_SPECIFIC_INCLUDE_DIRECTORY
		${BOARD_SPECIFIC_INCLUDE_DIRECTORY}
		${CMAKE_CURRENT_SOURCE_DIR}/bsp/common
		${CMAKE_CURRENT_SOURCE_DIR}/bsp/board
		)

# include essential driver headers
INCLUDE_DIRECTORIES(${TARGET_SPECIFIC_INCLUDE_DIRECTORY})
INCLUDE_DIRECTORIES(${BOARD_SPECIFIC_INCLUDE_DIRECTORY})

# build drivers
ADD_SUBDIRECTORY(efr32mg12p)
ADD_SUBDIRECTORY(rail)
ADD_SUBDIRECTORY(emlib)
ADD_SUBDIRECTORY(emdrv)
ADD_SUBDIRECTORY(middleware)
# ADD_SUBDIRECTORY(platform)
# ADD_SUBDIRECTORY(bsp)
# ADD_SUBDIRECTORY(protocol)

# build sam4s library
ADD_LIBRARY(${LIB_NAME} ${EFR32_SRC})

ADD_DEPENDENCIES(${LIB_NAME}
		efr32mg12p
		rail
		emlib
		emdrv
		middleware
		)

TARGET_LINK_LIBRARIES(${LIB_NAME}
		efr32mg12p
		rail
		emlib
		emdrv
		middleware
		)

# set those variables visible to parents
SET(CMAKE_ASM_FLAGS_DEBUG ${CMAKE_ASM_FLAGS_DEBUG} PARENT_SCOPE)
SET(CMAKE_C_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG} PARENT_SCOPE)
SET(CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG} PARENT_SCOPE)
SET(CMAKE_EXE_LINKER_FLAGS_DEBUG ${CMAKE_EXE_LINKER_FLAGS_DEBUG} PARENT_SCOPE)

SET(CMAKE_ASM_FLAGS_RELEASE ${CMAKE_ASM_FLAGS_RELEASE} PARENT_SCOPE)
SET(CMAKE_C_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE} PARENT_SCOPE)
SET(CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE} PARENT_SCOPE)
SET(CMAKE_EXE_LINKER_FLAGS_RELEASE ${CMAKE_EXE_LINKER_FLAGS_RELEASE} PARENT_SCOPE)

# forward include header files
SET(TARGET_SPECIFIC_INCLUDE_DIRECTORY ${TARGET_SPECIFIC_INCLUDE_DIRECTORY} PARENT_SCOPE)
SET(BOARD_SPECIFIC_INCLUDE_DIRECTORY ${BOARD_SPECIFIC_INCLUDE_DIRECTORY} PARENT_SCOPE)
