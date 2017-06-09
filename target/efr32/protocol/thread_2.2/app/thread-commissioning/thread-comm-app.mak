# This Makefile defines how to build 'thread-comm-app' for a unix host.
#

# Variables


GLOBAL_BASE_DIR := ../../platform/base
GLOBAL_MBEDTLS_DIR := ../../util/third_party/mbedtls

E_CC ?= gcc
CC = $(E_CC)
LD = $(E_CC)
SHELL = /bin/sh
STD ?= c99

INCLUDES= \
  -I. \
  -I$(GLOBAL_BASE_DIR) \
  -I$(GLOBAL_BASE_DIR)/hal \
  -I$(GLOBAL_BASE_DIR)/hal/micro/generic/aes \
  -I$(GLOBAL_BASE_DIR)/hal/micro/unix/host \
  -I$(GLOBAL_BASE_DIR)/hal/micro/unix/simulation \
  -I./app/thread-commissioning \
  -I./app/coap \
  -I./app/util \
  -I./app/util/serial \
  -I./stack \
  -I./stack/config \
  -I./stack/core \
  -I./stack/framework \
  -I./stack/ip \
  -I./stack/ip/host \
  -I./stack/ip/tls \
  -I./stack/ip/tls/credentials \
  -I./stack/ip/tls/small-aes \
  -I./stack/mac/802.15.4 \
  -I./stack/platform/micro/generic \
  -I$(GLOBAL_MBEDTLS_DIR)/include \
  -I$(GLOBAL_MBEDTLS_DIR)/sl_crypto/include \

DEFINES= \
  -DBOARD_HEADER=\"$(GLOBAL_BASE_DIR)/hal/micro/unix/host/board/host.h\" \
  -DPLATFORM_HEADER=\"$(GLOBAL_BASE_DIR)/hal/micro/unix/compiler/gcc.h\" \
  -DCONFIGURATION_HEADER=\"app/thread-commissioning/thread-comm-app-configuration.h\" \
  -DMBEDTLS_CONFIG_FILE=\"stack/ip/tls/mbedtls/config.h\" \
  -DEMBER_STACK_IP \
  -DHAL_MICRO \
  -DUNIX \
  -DUNIX_HOST \
  -DPHY_NULL \
  -DBOARD_HOST \
  -DEMBER_RIP_STACK \
  -DAPP_SERIAL=0 \
  -DEMBER_ASSERT_SERIAL_PORT=0 \
  -DEMBER_SERIAL0_BLOCKING \
  -DEMBER_SERIAL0_MODE=EMBER_SERIAL_FIFO \
  -DEMBER_SERIAL0_TX_QUEUE_SIZE=128 \
  -DEMBER_SERIAL0_RX_QUEUE_SIZE=128 \
  -DHAVE_TLS_CBC \
  -DHAVE_TLS_ECDHE_ECDSA \
  -DHAVE_TLS_JPAKE \
  -DHAVE_TLS_PSK \

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

#################################################

APPLICATION_FILES = app/thread-commissioning/thread-comm-app.c \

TESTER_FILES = stack/ip/tls/commission-tester.c \

STACK_FILES= \
  $(GLOBAL_BASE_DIR)/hal/micro/generic/crc.c \
  $(GLOBAL_BASE_DIR)/hal/micro/generic/mem-util.c \
  $(GLOBAL_BASE_DIR)/hal/micro/generic/random.c \
  $(GLOBAL_BASE_DIR)/hal/micro/unix/host/micro.c \
  app/coap/coap-stack.c \
  app/coap/coap.c \
  app/ip-ncp/binary-management.c \
  app/ip-ncp/host-stream.c \
  app/util/serial/command-interpreter2-binary.c \
  app/util/serial/command-interpreter2-error.c \
  app/util/serial/command-interpreter2-util.c \
  app/util/serial/command-interpreter2.c \
  app/util/serial/simple-linux-serial.c \
  stack/config/ember-ip-configuration.c \
  stack/framework/byte-utilities.c \
  stack/framework/event-queue.c \
  stack/framework/ip-packet-header.c \
  stack/ip/6lowpan-header.c \
  stack/ip/commission.c \
  stack/ip/connection.c \
  stack/ip/global-prefix.c \
  stack/ip/ip-address.c \
  stack/ip/ip-header.c \
  stack/ip/network-data.c \
  stack/ip/tcp.c \
  stack/ip/thread-key-stretch.c \
  stack/ip/transport-header.c \
  stack/ip/udp.c \
  stack/ip/host/unix-ip-utilities.c \
  stack/ip/host/unix-udp-wrapper.c \
  stack/ip/host/unix-listeners.c \
  stack/ip/host/host-listener-table.c \
  stack/ip/host/management-client.c \
  stack/ip/tls/public-key-stubs.c \
  stack/ip/tls/tls-ccm-record.c \
  stack/mac/802.15.4/802-15-4-ccm.c \
  stack/mac/802.15.4/mac-header-util.c \

MEMORY_MANAGER_FILES = \
  stack/framework/buffer-management.c \
  stack/framework/buffer-queue.c \
  stack/framework/buffer-malloc.c \

#################################################
# TLS:

TLS_HAL_FILES = \
  stack/ip/tls/native-test-util.c \
  stack/ip/tls/tls-debug.c \
  stack/core/log.c \

TLS_CRYPTO_FILES = \
  stack/ip/tls/sha256.c \

TLS_ECC_CREDENTIAL_FILES = \
  stack/ip/tls/credentials/ecc-test-ca.c \
  stack/ip/tls/credentials/ecc-test-certificate-0.c \
  stack/ip/tls/credentials/ecc-test-certificate-1.c \
  stack/ip/tls/tls-test-credentials.c \

TLS_FILES = \
  $(MEMORY_MANAGER_FILES) \
  stack/ip/tls/tls-handshake.c \
  stack/ip/tls/tls-handshake-crypto.c \
  stack/ip/tls/tls-tcp.c \
  stack/ip/tls/tls-record-util.c \
  stack/ip/tls/tls-session-state.c \
  stack/ip/tls/dtls.c

TLS_JOIN_FILES = stack/ip/tls/dtls-join.c \

TLS_JPAKE_FILES = stack/ip/tls/tls-jpake.c \

TLS_SUBSYSTEM_FILES = \
  $(TLS_ECC_CREDENTIAL_FILES) \
  $(TLS_FILES) \
  $(TLS_JOIN_FILES) \
  $(TLS_JPAKE_FILES) \

#################################################
# Other:

AES_FILES = \
  $(GLOBAL_BASE_DIR)/hal/micro/generic/aes/rijndael-alg-fst.c \
  $(GLOBAL_BASE_DIR)/hal/micro/generic/aes/rijndael-api-fst.c \
  stack/platform/micro/generic/aes.c \

MBEDTLS_JPAKE_FILES= \
  stack/ip/tls/jpake-ecc-mbedtls.c \
  $(GLOBAL_MBEDTLS_DIR)/library/bignum.c \
  $(GLOBAL_MBEDTLS_DIR)/library/ecjpake.c \
  $(GLOBAL_MBEDTLS_DIR)/library/ecp.c \
  $(GLOBAL_MBEDTLS_DIR)/library/ecp_curves.c \
  $(GLOBAL_MBEDTLS_DIR)/library/md.c \
  $(GLOBAL_MBEDTLS_DIR)/library/md_wrap.c \
  $(GLOBAL_MBEDTLS_DIR)/library/sha256.c \
  $(GLOBAL_MBEDTLS_DIR)/sl_crypto/src/sl_ecp.c \
  $(GLOBAL_MBEDTLS_DIR)/sl_crypto/src/sl_sha256.c \

#################################################

OUTPUT_DIR = build/thread-comm-app

APPLICATION_OBJECTS = $(addprefix $(OUTPUT_DIR)/, $(notdir $(APPLICATION_FILES:.c=.o)))

TESTER_OBJECTS = $(addprefix $(OUTPUT_DIR)/, $(notdir $(TESTER_FILES:.c=.o)))

STACK_OBJECTS = $(addprefix $(OUTPUT_DIR)/, $(notdir $(STACK_FILES:.c=.o)))

TLS_HAL_OBJECTS = $(addprefix $(OUTPUT_DIR)/, $(notdir $(TLS_HAL_FILES:.c=.o)))

TLS_CRYPTO_OBJECTS = $(addprefix $(OUTPUT_DIR)/tls-crypto/, $(notdir $(TLS_CRYPTO_FILES:.c=.o)))

TLS_SUBSYSTEM_OBJECTS = $(addprefix $(OUTPUT_DIR)/, $(notdir $(TLS_SUBSYSTEM_FILES:.c=.o)))

AES_OBJECTS = $(addprefix $(OUTPUT_DIR)/, $(notdir $(AES_FILES:.c=.o)))

MBEDTLS_OBJECTS = $(addprefix $(OUTPUT_DIR)/, $(notdir $(MBEDTLS_JPAKE_FILES:.c=.o)))

VPATH = \
  $(dir $(APPLICATION_FILES)) \
  $(dir $(TESTER_FILES)) \
  $(dir $(STACK_FILES)) \
  $(dir $(TLS_HAL_FILES)) \
  $(dir $(TLS_CRYPTO_FILES)) \
  $(dir $(TLS_SUBSYSTEM_FILES)) \
  $(dir $(AES_FILES)) \
  $(dir $(MBEDTLS_JPAKE_FILES)) \

APP_FILE= $(OUTPUT_DIR)/thread-comm-app
TESTER_FILE= $(OUTPUT_DIR)/commission-tester

APP_LIBRARIES= \

CPPFLAGS= $(INCLUDES) $(DEFINES) $(OPTIONS)
LINK_FLAGS= -L/opt/local/lib

# Rules

all: $(APP_FILE)

tester: $(TESTER_FILE)

ifneq ($(MAKECMDGOALS),clean)
-include $(APPLICATION_OBJECTS:.o=.d)
-include $(TESTER_OBJECTS:.o=.d)
-include $(STACK_OBJECTS:.o=.d)
-include $(TLS_HAL_OBJECTS:.o=.d)
-include $(TLS_CRYPTO_OBJECTS:.o=.d)
-include $(TLS_SUBSYSTEM_OBJECTS:.o=.d)
-include $(AES_OBJECTS:.o=.d)
-include $(MBEDTLS_OBJECTS:.o=.d)
endif

$(APP_FILE): \
  $(APPLICATION_OBJECTS) \
  $(STACK_OBJECTS) \
  $(TLS_HAL_OBJECTS) \
  $(TLS_CRYPTO_OBJECTS) \
  $(TLS_SUBSYSTEM_OBJECTS) \
  $(AES_OBJECTS) \
  $(MBEDTLS_OBJECTS) \
  $(APP_LIBRARIES) | $(OUTPUT_DIR)
	$(LD) $^ $(LINK_FLAGS) $(APP_LIBRARIES) -o $(APP_FILE)
	@echo -e '\n$@ build success'

$(TESTER_FILE): \
  $(TESTER_OBJECTS) \
  $(STACK_OBJECTS) \
  $(TLS_HAL_OBJECTS) \
  $(TLS_CRYPTO_OBJECTS) \
  $(TLS_SUBSYSTEM_OBJECTS) \
  $(AES_OBJECTS) \
  $(MBEDTLS_OBJECTS) \
  $(APP_LIBRARIES) | $(OUTPUT_DIR)
	$(LD) $^ $(LINK_FLAGS) $(APP_LIBRARIES) -o $(TESTER_FILE)
	@echo -e '\n$@ build success'

# Resolve an issue with similarly named files

$(OUTPUT_DIR)/tls-crypto/sha256.o: stack/ip/tls/sha256.c
	$(CC) $(CPPFLAGS) -MF $(@:.o=.d) -MMD -MP -c $< -o $@

$(OUTPUT_DIR)/sha256.o: $(GLOBAL_MBEDTLS_DIR)/library/sha256.c
	$(CC) $(CPPFLAGS) -MF $(@:.o=.d) -MMD -MP -c $< -o $@

$(OUTPUT_DIR)/%.o: %.c | $(OUTPUT_DIR)
	$(CC) $(CPPFLAGS) -MF $(@:.o=.d) -MMD -MP -c $< -o $@

clean:
	rm -rf $(OUTPUT_DIR)

$(OUTPUT_DIR):
	@mkdir -p $(OUTPUT_DIR)/tls-crypto
