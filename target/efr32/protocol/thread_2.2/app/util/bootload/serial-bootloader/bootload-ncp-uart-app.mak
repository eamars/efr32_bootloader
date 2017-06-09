# This Makefile defines how to build 'bootload-ncp-uart' for a unix host.
#

# Variables


GLOBAL_BASE_DIR := ../../platform/base

.SUFFIXES:

ifdef E_CC
  CC = $(E_CC)
endif
LD = $(CC)

SHELL = /bin/sh

DEFINES = \
  -DUNIX \
  -DUNIX_HOST \
  -DPHY_NULL \
  -DBOARD_HEADER=\"$(GLOBAL_BASE_DIR)/hal/micro/unix/host/board/host.h\" \
  -DCONFIGURATION_HEADER=\"app/util/bootload/serial-bootloader/bootload-ncp-uart-configuration.h\" \
  -DPLATFORM_HEADER=\"$(GLOBAL_BASE_DIR)/hal/micro/unix/compiler/gcc.h\" \
  -DBOARD_HOST \
  -DEMBER_STACK_IP \
  -DEZSP_HOST \
  -DEZSP_ASH \


INCLUDES = \
  -I. \
  -I$(GLOBAL_BASE_DIR) \
  -I$(GLOBAL_BASE_DIR)/hal \
  -I./app/util \
  -I./stack \


FILES = \
  app/ezsp-host/ash/ash-host-ui.c \
  app/ezsp-host/ash/ash-host.c \
  app/ezsp-host/ezsp-host-io.c \
  app/ezsp-host/ezsp-host-queues.c \
  app/ezsp-host/ezsp-host-ui.c \
  app/util/bootload/serial-bootloader/bootload-ncp-app.c \
  app/util/bootload/serial-bootloader/bootload-ncp-uart.c \
  app/util/bootload/serial-bootloader/bootload-ncp.c \
  app/util/bootload/serial-bootloader/bootload-xmodem.c \
  app/util/gateway/backchannel-stub.c \
  app/util/serial/simple-linux-serial.c \
  stack/framework/byte-utilities.c \
  $(GLOBAL_BASE_DIR)/hal/micro/generic/ash-common.c \
  $(GLOBAL_BASE_DIR)/hal/micro/generic/crc.c \
  $(GLOBAL_BASE_DIR)/hal/micro/generic/system-timer.c \


LIBRARIES = \

STD ?= c99

CFLAGS = -ggdb -O0 -std=$(STD) -Wall
CPPFLAGS = $(DEFINES) $(INCLUDES) -MF $(BUILD_PATH)/$(@F:.o=.d) -MMD -MP

ifeq ($(shell uname -s | sed -e 's/^\(CYGWIN\).*/\1/'),Darwin)
  LDFLAGS =
else
  LDFLAGS = -Wl,-export-dynamic
endif
LDLIBS =

BUILD_PATH = $(CURDIR)/build/bootload-ncp-uart-app

DEPENDENCIES = $(addprefix $(BUILD_PATH)/, $(notdir $(FILES:.c=.d)))
EXECUTABLE = $(BUILD_PATH)/bootload-ncp-uart-app
OBJECTS = $(addprefix $(BUILD_PATH)/, $(notdir $(FILES:.c=.o)))

VPATH = $(dir $(FILES))

.PHONY: all
all: $(EXECUTABLE)

.PHONY: clean
clean:
	@-rm -f $(DEPENDENCIES) $(EXECUTABLE) $(OBJECTS)

ifneq ($(MAKECMDGOALS),clean)
  -include $(DEPENDENCIES)
endif

$(EXECUTABLE): $(OBJECTS) $(LIBRARIES) | $(BUILD_PATH)
ifeq ($(shell uname -s | sed -e 's/^\(CYGWIN\).*/\1/'),Darwin)
	$(LD) $(LDFLAGS) $^ $(LDLIBS) -o $@
else
	$(LD) $(LDFLAGS) -Xlinker --start-group $^ $(LDLIBS) -Xlinker --end-group -o $@
endif

$(BUILD_PATH)/%.o: %.c | $(BUILD_PATH)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(BUILD_PATH):
	@mkdir $(BUILD_PATH)
