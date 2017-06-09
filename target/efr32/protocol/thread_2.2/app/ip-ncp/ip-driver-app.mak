# This Makefile defines how to build 'ip-driver-app' for a unix host.
#

# Variables


GLOBAL_BASE_DIR := ../../platform/base

E_CC ?= gcc
CC = $(E_CC)
LD = $(E_CC)
SHELL = /bin/sh
STD ?= c99

INCLUDES= \
  -I. \
  -I$(GLOBAL_BASE_DIR) \
  -I$(GLOBAL_BASE_DIR)/hal \
  -I$(GLOBAL_BASE_DIR)/hal/plugin \
  -I$(GLOBAL_BASE_DIR)/hal/micro/generic \
  -I$(GLOBAL_BASE_DIR)/hal/micro/unix/host \
  -I$(GLOBAL_BASE_DIR)/hal/micro/unix/simulation \
  -I./phy \
  -I./app/ip-ncp \
  -I./app/test \
  -I./app/udp \
  -I./app/util/ip \
  -I./app/util \
  -I./app/util/serial \
  -I./stack \
  -I./stack/core \
  -I./stack/framework \
  -I./stack/ip \
  -I./stack/ip/host \
  -I./stack/platform/micro/generic

DEFINES= \
  -DBOARD_HEADER=\"$(GLOBAL_BASE_DIR)/hal/micro/unix/host/board/host.h\" \
  -DPLATFORM_HEADER=\"$(GLOBAL_BASE_DIR)/hal/micro/unix/compiler/gcc.h\" \
  -DEMBER_STACK_IP \
  -DHAL_MICRO \
  -DUNIX \
  -DUNIX_HOST \
  -DPHY_NULL \
  -DBOARD_HOST \
  -DCONFIGURATION_HEADER=\"app/ip-ncp/ip-driver-app-configuration.h\" \
  -DEMBER_RIP_STACK \
  -DAPP_SERIAL=0 \
  -DEMBER_ASSERT_SERIAL_PORT=0 \
  -DEMBER_SERIAL0_BLOCKING \
  -DEMBER_SERIAL0_MODE=EMBER_SERIAL_FIFO \
  -DEMBER_SERIAL0_TX_QUEUE_SIZE=128 \
  -DEMBER_SERIAL0_RX_QUEUE_SIZE=128

# Negate -Werror for some specific warnings.  Clang warns about many more
# things than GCC, so it gets some additional negations.
WNO_ERRORS = \
  -Wno-error=cast-align \
  -Wno-error=deprecated-declarations
ifeq "$(shell $(CC) 2>&1 | grep -iqs clang ; echo $$?)" "0"
  WNO_ERRORS += \
    -Wno-error=pointer-sign \
    -Wno-error=tautological-constant-out-of-range-compare
endif

OPTIONS= \
  -std=$(STD) \
  -Wcast-align \
  -Wformat \
  -Wimplicit \
  -Wimplicit-int \
  -Wimplicit-function-declaration \
  -Winline \
  -Wno-long-long \
  -Wmain \
  -Wnested-externs \
  -Wno-import \
  -Wparentheses \
  -Wpointer-arith \
  -Wredundant-decls \
  -Wreturn-type \
  -Wstrict-prototypes \
  -Wswitch \
  -Wunused-label \
  -Wunused-value \
  -Werror \
  $(WNO_ERRORS) \
  -ggdb \
  -g \
  -O0

# These two files contain platform-specific code for connecting
# to the management client and IP stack.
APPLICATION_FILES= \
  app/ip-ncp/ip-driver-app.c \
  app/ip-ncp/host-stream.c \

STACK_FILES= \
  $(GLOBAL_BASE_DIR)/hal/micro/generic/ash-v3.c \
  $(GLOBAL_BASE_DIR)/hal/micro/generic/crc.c \
  $(GLOBAL_BASE_DIR)/hal/micro/generic/mem-util.c \
  $(GLOBAL_BASE_DIR)/hal/micro/generic/random.c \
  $(GLOBAL_BASE_DIR)/hal/micro/generic/system-timer.c \
  $(GLOBAL_BASE_DIR)/hal/micro/unix/host/micro.c \
  app/ip-ncp/binary-management.c \
  app/ip-ncp/ip-driver-app-unix-host.c \
  app/ip-ncp/ip-driver-log.c \
  app/ip-ncp/ip-driver.c \
  app/ip-ncp/ncp-uart-interface.c \
  app/test/scan-test-stub.c \
  app/udp/thread-app-stubs.c \
  app/util/ip/counters.c \
  app/util/serial/command-interpreter2-binary.c \
  app/util/serial/command-interpreter2-error.c \
  app/util/serial/command-interpreter2-util.c \
  app/util/serial/command-interpreter2.c \
  app/util/serial/simple-linux-serial.c \
  stack/config/ember-ip-configuration.c \
  stack/core/log.c \
  stack/framework/byte-utilities.c \
  stack/framework/event-control.c \
  stack/framework/event-queue.c \
  stack/ip/host/unix-driver-utilities.c \
  stack/ip/ip-address.c \

OUTPUT_DIR= build/ip-driver-app
# Build a list of object files from the source file list, but all objects
# live in the $(OUTPUT_DIR) above.  The list of object files
# created assumes that the file part of the filepath is unique
# (i.e. the bar.c of foo/bar.c is unique across all sub-directories included).
APPLICATION_OBJECTS = $(addprefix $(OUTPUT_DIR)/, $(notdir $(APPLICATION_FILES:.c=.o)))
STACK_OBJECTS = $(addprefix $(OUTPUT_DIR)/, $(notdir $(STACK_FILES:.c=.o)))
VPATH = $(dir $(APPLICATION_FILES)) $(dir $(STACK_FILES))

APP_FILE= $(OUTPUT_DIR)/ip-driver-app

APP_LIBRARIES= \

CPPFLAGS= $(INCLUDES) $(DEFINES) $(OPTIONS)
LINK_FLAGS= \
  -L/opt/local/lib

# Rules

all: $(APP_FILE)

ifneq ($(MAKECMDGOALS),clean)
-include $(APPLICATION_OBJECTS:.o=.d)
-include $(STACK_OBJECTS:.o=.d)
endif

$(APP_FILE): $(APPLICATION_OBJECTS) $(STACK_OBJECTS) $(APP_LIBRARIES) | $(OUTPUT_DIR)
	$(LD) $^ $(LINK_FLAGS) $(APP_LIBRARIES) -o $(APP_FILE)
	@echo -e '\n$@ build success'

$(OUTPUT_DIR)/%.o: %.c | $(OUTPUT_DIR)
	$(CC) $(CPPFLAGS) -MF $(@:.o=.d) -MMD -MP -c $< -o $@

clean:
	rm -rf $(OUTPUT_DIR)

$(OUTPUT_DIR):
	@mkdir -p $(OUTPUT_DIR)
