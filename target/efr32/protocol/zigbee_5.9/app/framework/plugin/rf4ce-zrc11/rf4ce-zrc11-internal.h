// Copyright 2014 Silicon Laboratories, Inc.

// The maximum number of repetitions performed during discovery.
#define NWK_MAX_DISCOVERY_REPETITIONS 30

// The maximum number of node descriptors reported during discovery.
#define NWK_MAX_REPORTED_NODE_DESCRIPTORS 1

// The maximum duration that the receiver is enabled on a controller after
// pairing to receive any command discovery request command frames.
#define APLC_MAX_CMD_DISC_RX_ON_DURATION_MS 200

// The maximum time between consecutive user control repeated command frame
// transmissions.
#define APLC_MAX_KEY_REPEAT_INTERVAL_MS 100

// The maximum amount of time a device waits after receiving a successful
// NLME-AUTODISCOVERY.confirm primitive for a pair indication to arrive from
// the pairing initiator.
#define APLC_MAX_PAIR_INDICATION_WAIT_TIME_MS MILLISECOND_TICKS_PER_SECOND

// The maximum time a device shall wait for a response command frame following
// a request command frame.
#define APLC_MAX_RESPONSE_WAIT_TIME_MS 200

// The minimum value of the KeyExTransferCount parameter passed to the pair
// request primitive during the push button pairing procedure.
#define APLC_MIN_KEY_EXCHANGE_TRANSFER_COUNT 3

// The minimum amount of time a device must wait after a successful pairing
// attempt with a target before attempting command discovery.
#define APLC_MIN_TARGET_BLACKOUT_PERIOD_MS 500

// The amount of time a device must perform discovery.
#define APLC_DISCOVERY_DURATION_MS 100

// The amount of time a device must wait in automatic discovery response mode.
#define APLC_AUTO_DISCOVERY_RESPONSE_MODE_DURATION_MS \
  (30 * MILLISECOND_TICKS_PER_SECOND)

// ZRC header
// - Frame control (1 byte)
#define ZRC_FRAME_CONTROL_LENGTH 1
#define ZRC_FRAME_CONTROL_COMMAND_CODE_MASK 0x1F
#define ZRC_OVERHEAD (ZRC_FRAME_CONTROL_LENGTH)

#define COMMAND_CODE_MINIMUM EMBER_AF_RF4CE_ZRC_COMMAND_USER_CONTROL_PRESSED
#define COMMAND_CODE_MAXIMUM EMBER_AF_RF4CE_ZRC_COMMAND_COMMAND_DISCOVERY_RESPONSE

// User Control Pressed
// - RC command code (1 byte)
// - RC command payload (n bytes)
#define USER_CONTROL_PRESSED_LENGTH                    (ZRC_OVERHEAD + 1)
#define USER_CONTROL_PRESSED_RC_COMMAND_CODE_OFFSET    (ZRC_OVERHEAD)
#define USER_CONTROL_PRESSED_RC_COMMAND_PAYLOAD_OFFSET (ZRC_OVERHEAD + 1)

// User Control Repeated
// - RC command code (1 byte, 1.1 only)
// - RC command payload (n bytes, 1.1 only)
#define USER_CONTROL_REPEATED_1_0_LENGTH (ZRC_OVERHEAD)
#define USER_CONTROL_REPEATED_1_1_LENGTH (ZRC_OVERHEAD + 1)
#define USER_CONTROL_REPEATED_1_1_RC_COMMAND_CODE_OFFSET    (ZRC_OVERHEAD)
#define USER_CONTROL_REPEATED_1_1_RC_COMMAND_PAYLOAD_OFFSET (ZRC_OVERHEAD + 1)

// User Control Released
// - RC command code (1 byte, 1.1 only)
#define USER_CONTROL_RELEASED_1_0_LENGTH (ZRC_OVERHEAD)
#define USER_CONTROL_RELEASED_1_1_LENGTH (ZRC_OVERHEAD + 1)
#define USER_CONTROL_RELEASED_1_1_RC_COMMAND_CODE_OFFSET (ZRC_OVERHEAD)

// Command Discovery Request
// - Reserved (1 byte)
#define COMMAND_DISCOVERY_REQUEST_LENGTH (ZRC_OVERHEAD + 1)

// Command Discovery Response
// - Reserved (1 byte)
// - Commands supported (32 bytes)
#define COMMANDS_SUPPORTED_LENGTH 32
#define COMMAND_DISCOVERY_RESPONSE_LENGTH (ZRC_OVERHEAD + 1 + COMMANDS_SUPPORTED_LENGTH)
#define COMMAND_DISCOVERY_RESPONSE_COMMANDS_SUPPORTED_OFFSET (ZRC_OVERHEAD + 1)

// The User Control Pressed and User Control Repeated commmands theoretically
// take an unbounded additional payload, but the longest additional operand in
// HDMI 1.3a is just four bytes.  Still, just in case, leave an opening for the
// user to override the buffer size.
#ifndef EMBER_AF_PLUGIN_RF4CE_ZRC11_MAXIMUM_RC_COMMAND_PAYLOAD_LENGTH
  #define EMBER_AF_PLUGIN_RF4CE_ZRC11_MAXIMUM_RC_COMMAND_PAYLOAD_LENGTH 4
#endif
#define MAXIMUM_USER_CONTROL_X_LENGTH                               \
  (USER_CONTROL_PRESSED_LENGTH                                      \
   + EMBER_AF_PLUGIN_RF4CE_ZRC11_MAXIMUM_RC_COMMAND_PAYLOAD_LENGTH)

// Assuming the standard operands are used, the Discovery Response command is
// the command with the longest payload in the ZRC profile.
#ifndef EMBER_AF_PLUGIN_RF4CE_ZRC11_MAXIMUM_PAYLOAD_LENGTH
  #if MAXIMUM_USER_CONTROL_X_LENGTH < COMMAND_DISCOVERY_RESPONSE_LENGTH
    #define EMBER_AF_PLUGIN_RF4CE_ZRC11_MAXIMUM_PAYLOAD_LENGTH COMMAND_DISCOVERY_RESPONSE_LENGTH
  #else
    #define EMBER_AF_PLUGIN_RF4CE_ZRC11_MAXIMUM_PAYLOAD_LENGTH MAXIMUM_USER_CONTROL_X_LENGTH
  #endif
#endif

void emAfRf4ceZrc11InitOriginator(void);
void emAfRf4ceZrc11InitRecipient(void);

extern uint8_t emAfRf4ceZrc11Buffer[];
extern uint8_t emAfRf4ceZrc11BufferLength;
EmberStatus emAfRf4ceZrc11Send(uint8_t pairingIndex);

extern EmberEventControl emberAfPluginRf4ceZrc11PairingEventControl;
extern EmberEventControl emberAfPluginRf4ceZrc11IncomingUserControlEventControl;
extern EmberEventControl emberAfPluginRf4ceZrc11OutgoingUserControlEventControl;
extern EmberEventControl emberAfPluginRf4ceZrc11CommandDiscoveryEventControl;

void emAfRf4ceZrc11RxEnable(void);

void emAfRf4ceZrc11IncomingUserControl(uint8_t pairingIndex,
                                       EmberAfRf4ceZrcCommandCode commandCode,
                                       const uint8_t *message,
                                       uint8_t messageLength);
