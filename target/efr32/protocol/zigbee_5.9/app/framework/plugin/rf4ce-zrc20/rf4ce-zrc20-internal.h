// Copyright 2014 Silicon Laboratories, Inc.

// The maximum time the Binding Recipient shall wait to receive a command frame
// from a Binding Initiator during its configuration phase. 
#define APLC_MAX_CONFIG_WAIT_TIME_MS 100

// The maximum time a node shall wait for a response command frame following a
// request command frame.
#define APLC_MAX_RESPONSE_WAIT_TIME_MS 100

// The maximum time between consecutive actions command frame transmissions
// indicating a repeated action.
#define APLC_MAX_ACTION_REPEAT_TRIGGER_INTERVAL_MS 200

// The time that an action control record should be repeated, see Table 10.
#define APLC_SHORT_RETRY_DURATION_MS 100

void emAfRf4ceZrc20StartConfigurationOriginator(uint8_t pairingIndex);
void emAfRf4ceZrc20StartConfigurationRecipient(uint8_t pairingIndex);

void emAfRf4ceZrcClearActionBank(uint8_t *actionBanksSupported,
                                 EmberAfRf4ceZrcActionBank actionBank);
bool emAfRf4ceZrcReadActionBank(const uint8_t *actionBanksSupported,
                                   EmberAfRf4ceZrcActionBank actionBank);
void emAfRf4ceZrcSetActionBank(uint8_t *actionBanksSupported,
                               EmberAfRf4ceZrcActionBank actionBank);
#define emAfRf4ceZrcClearActionCode emAfRf4ceZrcClearActionBank
#define emAfRf4ceZrcReadActionCode emAfRf4ceZrcReadActionBank
#define emAfRf4ceZrcSetActionCode emAfRf4ceZrcSetActionBank
bool emAfRf4ceZrcHasRemainingActionBanks(const uint8_t *actionBanksSupported);
#define emAfRf4ceZrcExchangeActionBanks(originatorCapabilities,    \
                                        recipientCapabilities)     \
  (((originatorCapabilities) | (recipientCapabilities))            \
   & EMBER_AF_RF4CE_ZRC_CAPABILITY_INFORM_ABOUT_SUPPORTED_ACTIONS)
void emAfRf4ceZrcGetExchangeableActionBanks(const uint8_t *actionBanksSupportedTx,
                                            EmberAfRf4ceZrcCapability originatorCapabilities,
                                            const uint8_t *actionBanksSupportedRx,
                                            EmberAfRf4ceZrcCapability recipientCapabilities,
                                            uint8_t *actionBanksSupportedRxExchange,
                                            uint8_t *actionBanksSupportedTxExchange);
uint8_t *emAfRf4ceZrcGetActionCodesAttributePointer(uint8_t attrId,
                                                  uint16_t entryId,
                                                  uint8_t pairingIndex);


// Each action codes supported record has a one-byte attribute id, two-byte
// entry id, one-byte length, and 32-byte value.  After accounting for the
// overhead of attribute commands in general, there is only enough room in an
// RF4CE data command for two records.
#define ACTION_CODES_SUPPORTED_RECORDS_MAX 2

void emAfRf4ceZrc20IncomingMessage(uint8_t pairingIndex,
                                   uint16_t vendorId,
                                   const uint8_t *message,
                                   uint8_t messageLength);

void emAfRf4ceZrc20InitRecipient(void);
void emAfRf4ceZrc20InitOriginator(void);
void emAfRf4ceZrc20AttributesInit(void);

#define ACTION_TYPE_MASK   0x03
#define MODIFIER_BITS_MASK 0xF0

// The action control field has the type in the lower nibble and the modifier
// bits in the upper nibble.  We store the type and modifiers separately, so
// the lower nibble of modifiers is usable for bookkeeping purposes.
#define MODIFIER_BITS_SPECIAL_MASK 0x0F
#define MODIFIER_BITS_SPECIAL_MARK 0x01

#define ZRC_VERSION_NONE        0x00
#define ZRC_VERSION_1_1         0x01
#define ZRC_VERSION_2_0         0x02

uint8_t emAfRf4ceZrc20GetPeerZrcVersion(uint8_t pairingIndex);

// Action record related macros
#define ACTION_RECORD_ACTION_CONTROL_OFFSET                 0
#define ACTION_RECORD_ACTION_CONTROL_LENGTH                 1
#define ACTION_RECORD_ACTION_CONTROL_ACTION_TYPE_MASK       0x03
#define ACTION_RECORD_ACTION_CONTROL_ACTION_TYPE_OFFSET     0
// Bits 2-3 are reserved
#define ACTION_RECORD_ACTION_CONTROL_MODIFIER_BITS_MASK     0xF0
#define ACTION_RECORD_ACTION_CONTROL_MODIFIER_BITS_OFFSET   4
#define ACTION_RECORD_ACTION_PAYLOAD_LENGTH_OFFSET          1
#define ACTION_RECORD_ACTION_PAYLOAD_LENGTH_LENGTH          1
#define ACTION_RECORD_ACTION_BANK_OFFSET                    2
#define ACTION_RECORD_ACTION_BANK_LENGTH                    1
#define ACTION_RECORD_ACTION_CODE_OFFSET                    3
#define ACTION_RECORD_ACTION_CODE_LENGTH                    1
#define ACTION_RECORD_ACTION_VENDOR_OFFSET                  4
#define ACTION_RECORD_ACTION_VENDOR_LENGTH                  2

// ZRC 1.1 misc defines
#define ZRC11_MAX_USER_CONTROL_COMMAND_PAYLOAD_LENGTH   4
#define ZRC11_MAX_USER_CONTROL_COMMAND_LENGTH           (2 + ZRC11_MAX_USER_CONTROL_COMMAND_PAYLOAD_LENGTH)
#define ZRC11_MAX_RESPONSE_WAIT_TIME


// ZRC header
// - Frame control (1 byte)
#define ZRC_HEADER_LENGTH                               1
#define ZRC_HEADER_FRAME_CONTROL_OFFSET                 0
#define ZRC_HEADER_FRAME_CONTROL_COMMAND_CODE_MASK      0x0F
#define ZRC_PAYLOAD_OFFSET                              (ZRC_HEADER_LENGTH)

// User Control Pressed
// - RC command code (1 byte)
// - RC command payload (n bytes)
#define USER_CONTROL_PRESSED_LENGTH                    (ZRC_HEADER_LENGTH + 1)
#define USER_CONTROL_PRESSED_RC_COMMAND_CODE_OFFSET    (ZRC_PAYLOAD_OFFSET)
#define USER_CONTROL_PRESSED_RC_COMMAND_PAYLOAD_OFFSET (ZRC_PAYLOAD_OFFSET + 1)

// User Control Repeated
// - RC command code (1 byte, 1.1 only)
// - RC command payload (n bytes, 1.1 only)
#define USER_CONTROL_REPEATED_1_0_LENGTH (ZRC_HEADER_LENGTH)
#define USER_CONTROL_REPEATED_1_1_LENGTH (ZRC_HEADER_LENGTH + 1)
#define USER_CONTROL_REPEATED_1_1_RC_COMMAND_CODE_OFFSET    (ZRC_PAYLOAD_OFFSET)
#define USER_CONTROL_REPEATED_1_1_RC_COMMAND_PAYLOAD_OFFSET (ZRC_PAYLOAD_OFFSET + 1)

// User Control Released
// - RC command code (1 byte, 1.1 only)
#define USER_CONTROL_RELEASED_1_0_LENGTH (ZRC_HEADER_LENGTH)
#define USER_CONTROL_RELEASED_1_1_LENGTH (ZRC_HEADER_LENGTH + 1)
#define USER_CONTROL_RELEASED_1_1_RC_COMMAND_CODE_OFFSET (ZRC_PAYLOAD_OFFSET)

// Command Discovery Request
// - Reserved (1 byte)
#define COMMAND_DISCOVERY_REQUEST_LENGTH (ZRC_HEADER_LENGTH + 1)

// Command Discovery Response
// - Reserved (1 byte)
// - Commands supported (32 bytes)
#define COMMANDS_SUPPORTED_LENGTH 32
#define COMMAND_DISCOVERY_RESPONSE_LENGTH (ZRC_HEADER_LENGTH + 1 + COMMANDS_SUPPORTED_LENGTH)
#define COMMAND_DISCOVERY_RESPONSE_COMMANDS_SUPPORTED_OFFSET (ZRC_PAYLOAD_OFFSET + 1)

// Client Notification sub-types
#define CLIENT_NOTIFICATION_SUBTYPE_REQUEST_ACTION_MAPPING_NEGOTIATION      0x40
#define CLIENT_NOTIFICATION_SUBTYPE_REQUEST_HA_PULL                         0x41
#define CLIENT_NOTIFICATION_SUBTYPE_REQUEST_SELECTIVE_ACTION_MAPPING_UPDATE 0x42

// Client Notification - Request Action Mapping Negotiation (no payload)
#define CLIENT_NOTIFICATION_REQUEST_ACTION_MAPPING_NEGOTIATION_PAYLOAD_LENGTH  0

// Client Notification - Request Home Automation Pull
// - HA instance ID (1 byte)
// - HA Attribute Dirty Flags (32 bytes)
#define CLIENT_NOTIFICATION_REQUEST_HA_PULL_PAYLOAD_LENGTH                    33
#define CLIENT_NOTIFICATION_REQUEST_HA_PULL_HA_INSTANCE_ID_OFFSET              0
#define CLIENT_NOTIFICATION_REQUEST_HA_PULL_HA_INSTANCE_ID_LENGTH              1
#define CLIENT_NOTIFICATION_REQUEST_HA_PULL_HA_ATTRIBUTE_DIRTY_FLAGS_OFFSET    1
#define CLIENT_NOTIFICATION_REQUEST_HA_PULL_HA_ATTRIBUTE_DIRTY_FLAGS_LENGTH   32

// Client Notification - Request Selective Action  Mapping Update
// - Indices for Action Mapping Client to inform Action Mapping Server about
// - Mappable Action Index List Length (1 byte)
// - Mappable Action Index (2 bytes)*(List Length)
#define CLIENT_NOTIFICATION_REQUEST_SELECTIVE_AM_UPDATE_INDEX_LIST_LENGTH_OFFSET  0
#define CLIENT_NOTIFICATION_REQUEST_SELECTIVE_AM_UPDATE_INDEX_LIST_LENGTH_LENGTH  1
#define CLIENT_NOTIFICATION_REQUEST_SELECTIVE_AM_UPDATE_INDEX_LIST_OFFSET         1

void emAfRf4ceZrcIncomingRequestActionMappingNegotiation(void);
void emAfRf4ceZrcIncomingRequestSelectiveActionMappingUpdate(const uint8_t *mappableActionsList,
                                                             uint8_t mappableActionsListLength);
void emAfRf4ceZrcIncomingRequestHomeAutomationPull(uint8_t haInstanceId,
                                                   const uint8_t *haAttributeDirtyFlags);


// Internal state machine states
#define ZRC_STATE_INITIAL                                                             0x00
#define ZRC_STATE_ORIGINATOR_PUSH_VERSION_AND_CAPABILITIES_AND_ACTION_BANKS_VERSION   0x01
#define ZRC_STATE_ORIGINATOR_GET_VERSION_AND_CAPABILITIES_AND_ACTION_BANKS_VERSION    0x02
#define ZRC_STATE_ORIGINATOR_GET_ACTION_BANKS_SUPPORTED_RX                            0x03
#define ZRC_STATE_ORIGINATOR_PUSH_ACTION_BANKS_SUPPORTED_TX                           0x04
#define ZRC_STATE_ORIGINATOR_GET_ACTION_CODES_SUPPORTED_RX                            0x05
#define ZRC_STATE_ORIGINATOR_PUSH_ACTION_CODES_SUPPORTED_TX                           0x06
#define ZRC_STATE_ORIGINATOR_CONFIGURATION_COMPLETE                                   0x07
#define ZRC_STATE_RECIPIENT_PUSH_VERSION_AND_CAPABILITIES_AND_ACTION_BANKS_VERSION    0x08
#define ZRC_STATE_RECIPIENT_GET_VERSION_AND_CAPABILITIES_AND_ACTION_BANKS_VERSION     0x09
#define ZRC_STATE_RECIPIENT_GET_ACTION_BANKS_SUPPORTED_RX                             0x0A
#define ZRC_STATE_RECIPIENT_PUSH_ACTION_BANKS_SUPPORTED_TX                            0x0B
#define ZRC_STATE_RECIPIENT_GET_ACTION_CODES_SUPPORTED_RX                             0x0C
#define ZRC_STATE_RECIPIENT_PUSH_ACTION_CODES_SUPPORTED_TX                            0x0D
#define ZRC_STATE_RECIPIENT_CONFIGURATION_COMPLETE                                    0x0E
#define ZRC_STATE_AM_CLIENT_PUSHING_IRDB_VENDOR_SUPPORT_TO_SERVER                     0x10
#define ZRC_STATE_AM_CLIENT_PUSHING_MAPPABLE_ACTIONS_TO_SERVER                        0x20
#define ZRC_STATE_AM_CLIENT_PULLING_ACTION_MAPPINGS_FROM_SERVER                       0x30
#define ZRC_STATE_HA_ORIGINATOR_PUSHING_HA_SUPPORTED_TO_RECIPIENT                     0x40
#define ZRC_STATE_HA_ORIGINATOR_PULLING_HA_ATTRIBUTE_FROM_RECIPIENT                   0x50
#define ZRC_STATE_HA_ORIGINATOR_PULLING_HA_ATTRIBUTES_ON_REQUEST_FROM_RECIPIENT       0x60

extern uint8_t emAfZrcState;

#define isZrcStateBindingOriginator()                                          \
  (emAfZrcState >= ZRC_STATE_ORIGINATOR_PUSH_VERSION_AND_CAPABILITIES_AND_ACTION_BANKS_VERSION  \
   && emAfZrcState <= ZRC_STATE_ORIGINATOR_CONFIGURATION_COMPLETE)

#define isZrcStateBindingRecipient()                                           \
  (emAfZrcState >= ZRC_STATE_RECIPIENT_PUSH_VERSION_AND_CAPABILITIES_AND_ACTION_BANKS_VERSION  \
   && emAfZrcState <= ZRC_STATE_RECIPIENT_CONFIGURATION_COMPLETE)

#define isZrcStateActionMappingClient()                                        \
  (emAfZrcState >= ZRC_STATE_AM_CLIENT_PUSHING_IRDB_VENDOR_SUPPORT_TO_SERVER   \
   && emAfZrcState <= ZRC_STATE_AM_CLIENT_PULLING_ACTION_MAPPINGS_FROM_SERVER)

#define isZrcStateHaActionsOriginator()                                        \
  (emAfZrcState >= ZRC_STATE_HA_ORIGINATOR_PUSHING_HA_SUPPORTED_TO_RECIPIENT   \
   && emAfZrcState <= ZRC_STATE_HA_ORIGINATOR_PULLING_HA_ATTRIBUTES_ON_REQUEST_FROM_RECIPIENT)

// We provide two implementations of these: one that stores the capabilities in
// RAM (for HOST processors) and one that stores the capabilities in NVM (for
// the SoC).
uint16_t emAfRf4ceZrcGetRemoteNodeFlags(uint8_t pairingIndex);
void emAfRf4ceZrcSetRemoteNodeFlags(uint8_t pairingIndex,
                                    uint16_t flags);

// First byte stores the remote nodes ZRC capabilities (only the first byte of
// the ZRC capabilities is used in the 2.0 version).
#define ZRC_INTERNAL_FLAGS_CAPABILITIES_MASK      0x00FF
#define ZRC_INTERNAL_FLAGS_CAPABILITIES_OFFSET    0

//#define EMBER_AF_PLUGIN_RF4CE_ZRC20_DEBUG_BINDING
#if defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_DEBUG_BINDING)
  #define emAfZrcSetState(newState) reallySetState((newState), __LINE__)
  #define printGetAttribute(attributeId)  printAttribute(attributeId, "GET")
  #define printPushAttribute(attributeId) printAttribute(attributeId, "PUSH")
#else
  #define printState(command)
  #define printStateWithStatus(command, status)
  #define printGetAttribute(attributeId)
  #define printPushAttribute(attributeId)
  #define emAfZrcSetState(newState) (emAfZrcState = (newState))
#endif // EMBER_AF_PLUGIN_RF4CE_ZRC20_DEBUG_BINDING

#if defined(EMBER_SCRIPTED_TEST)
#include "core/scripted-stub.h"

#define debugScriptCheck(reason)                                               \
  simpleScriptCheck("scriptCheck", "scriptCheck: " reason, "")
#else
#define debugScriptCheck(reason)
#endif // EMBER_SCRIPTED_TEST
