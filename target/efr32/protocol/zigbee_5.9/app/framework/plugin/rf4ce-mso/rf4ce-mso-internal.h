// Copyright 2013 Silicon Laboratories, Inc.

#include "app/framework/plugin/rf4ce-profile/rf4ce-profile.h"

#define NWK_DISCOVERY_LQI_THRESHOLD 0
#define NWK_DISCOVERY_REPETITION_INTERVAL_MS 600
#define NWK_INDICATE_DISCOVERY_REQUEST true

// The maximum number of repetitions performed during discovery.
#define NWK_MAX_DISCOVERY_REPETITIONS 2

// The maximum number of node descriptors reported during discovery.
#define NWK_MAX_REPORTED_NODE_DESCRIPTORS 16

// The maximum number of pairing candidates selected from the node descriptor
// list.
#define APLC_MAX_PAIRING_CANDIDATES 3

// The maximum time between consecutive user control repeated command frame
// transmissions.
#define APLC_MAX_KEY_REPEAT_INTERVAL_MS 120

// The maximum size in octets of the elements of the attributes in the RIB.  At
// the same time, the maximum size in octets of the value field in the set
// attribute request and get attribute response command frames.
#define APLC_MAX_RIB_ATTRIBUTE_SIZE 92

// The time a device SHALL wait after the successful transmission of a request
// command frame before enabling its receiver to receive a response command
// frame.
#define APLC_RESPONSE_IDLE_TIME_MS 50

// The time at the start of the validation procedure during which packets shall
// not be transmitted.
#define APLC_BLACK_OUT_TIME_MS 100

// The minimum value of the KeyExTransferCount parameter passed to the pair
// request primitive during the push button pairing procedure.
#define APLC_MIN_KEY_EXCHANGE_TRANSFER_COUNT 3

#define NULL_PAIRING_INDEX 0xFF

#define MSO_USER_STRING_LENGTH                                                 9

// Discovery request user string fields
#define MSO_DISCOVERY_REQUEST_MSO_USER_STRING_OFFSET                          0
#define MSO_DISCOVERY_REQUEST_MSO_USER_STRING_LENGTH                          MSO_USER_STRING_LENGTH
#define MSO_DISCOVERY_REQUEST_BINDING_INITIATION_INDICATOR_OFFSET             14

// Binding initiation indicator field
#define MSO_BINDING_INITIATION_INDICATOR_DEDICATED_KEY_COMBO_BIND           0x00
#define MSO_BINDING_INITIATION_INDICATOR_ANY_BUTTON_BIND                    0x01

// Discovery response user string fields
#define MSO_DISCOVERY_RESPONSE_MSO_USER_STRING_OFFSET                         0
#define MSO_DISCOVERY_RESPONSE_MSO_USER_STRING_LENGTH                         MSO_USER_STRING_LENGTH
#define MSO_DISCOVERY_RESPONSE_TERTIARY_CLASS_DESCRIPTOR_OFFSET               10
#define MSO_DISCOVERY_RESPONSE_SECONDARY_CLASS_DESCRIPTOR_OFFSET              11
#define MSO_DISCOVERY_RESPONSE_PRIMARY_CLASS_DESCRIPTOR_OFFSET                12
#define MSO_DISCOVERY_RESPONSE_STRICT_LQI_THRESHOLD_OFFSET                    13
#define MSO_DISCOVERY_RESPONSE_BASIC_LQI_THRESHOLD_OFFSET                     14

// Class descriptor fields
#define MSO_CLASS_DESCRIPTOR_CLASS_NUMBER_MASK                              0x0F
#define MSO_CLASS_DESCRIPTOR_CLASS_NUMBER_OFFSET                            0
#define MSO_CLASS_DESCRIPTOR_DUPLICATE_CLASS_NUMBER_HANDLING_MASK           0x30
#define MSO_CLASS_DESCRIPTOR_DUPLICATE_CLASS_NUMBER_HANDLING_OFFSET         4
#define MSO_CLASS_DESCRIPTOR_APPLY_STRICT_LQI_THRESHOLD_BIT                 0x40
#define MSO_CLASS_DESCRIPTOR_ENABLE_REQUEST_AUTO_VALIDATION_BIT             0x80

// Duplicate class number handling field
#define MSO_DUPLICATE_CLASS_NUMBER_HANDLING_USE_NODE_AS_IS                  0x00
#define MSO_DUPLICATE_CLASS_NUMBER_HANDLING_REMOVE_NODE_DESCRIPTOR          0x01
#define MSO_DUPLICATE_CLASS_NUMBER_HANDLING_RECLASSIFY_NODE_DESCRIPTOR      0x02
#define MSO_DUPLICATE_CLASS_NUMBER_HANDLING_ABORT_BINDING                   0x03

// These come from the plugin option and need to be translated to appropriate
// values.
#define USE_NODE_DESCRIPTOR_AS_IS \
  MSO_DUPLICATE_CLASS_NUMBER_HANDLING_USE_NODE_AS_IS
#define REMOVE_NODE_DESCRIPTOR \
  MSO_DUPLICATE_CLASS_NUMBER_HANDLING_REMOVE_NODE_DESCRIPTOR
#define RECLASSIFY_NODE_DESCRIPTOR \
  MSO_DUPLICATE_CLASS_NUMBER_HANDLING_RECLASSIFY_NODE_DESCRIPTOR
#define ABORT_BINDING \
  MSO_DUPLICATE_CLASS_NUMBER_HANDLING_ABORT_BINDING

#ifdef EMBER_AF_PLUGIN_RF4CE_MSO_FULL_ROLL_BACK_ENABLED
  #define MSO_FULL_ROLL_BACK_ENABLED_BIT BIT(0)
#else
  #define MSO_FULL_ROLL_BACK_ENABLED_BIT 0
#endif

#ifdef EMBER_AF_PLUGIN_RF4CE_MSO_PRIMARY_APPLY_STRICT_LQI_THRESHOLD
  #define MSO_PRIMARY_APPLY_STRICT_LQI_THRESHOLD_BIT \
    MSO_CLASS_DESCRIPTOR_APPLY_STRICT_LQI_THRESHOLD_BIT
#else
  #define MSO_PRIMARY_APPLY_STRICT_LQI_THRESHOLD_BIT 0
#endif
#ifdef EMBER_AF_PLUGIN_RF4CE_MSO_PRIMARY_ENABLE_REQUEST_AUTO_VALIDATION
  #define MSO_PRIMARY_ENABLE_REQUEST_AUTO_VALIDATION_BIT \
    MSO_CLASS_DESCRIPTOR_ENABLE_REQUEST_AUTO_VALIDATION_BIT
#else
  #define MSO_PRIMARY_ENABLE_REQUEST_AUTO_VALIDATION_BIT 0
#endif

#ifdef EMBER_AF_PLUGIN_RF4CE_MSO_SECONDARY_APPLY_STRICT_LQI_THRESHOLD
  #define MSO_SECONDARY_APPLY_STRICT_LQI_THRESHOLD_BIT \
    MSO_CLASS_DESCRIPTOR_APPLY_STRICT_LQI_THRESHOLD_BIT
#else
  #define MSO_SECONDARY_APPLY_STRICT_LQI_THRESHOLD_BIT 0
#endif
#ifdef EMBER_AF_PLUGIN_RF4CE_MSO_SECONDARY_ENABLE_REQUEST_AUTO_VALIDATION
  #define MSO_SECONDARY_ENABLE_REQUEST_AUTO_VALIDATION_BIT \
    MSO_CLASS_DESCRIPTOR_ENABLE_REQUEST_AUTO_VALIDATION_BIT
#else
  #define MSO_SECONDARY_ENABLE_REQUEST_AUTO_VALIDATION_BIT 0
#endif

#ifdef EMBER_AF_PLUGIN_RF4CE_MSO_TERTIARY_APPLY_STRICT_LQI_THRESHOLD
  #define MSO_TERTIARY_APPLY_STRICT_LQI_THRESHOLD_BIT \
    MSO_CLASS_DESCRIPTOR_APPLY_STRICT_LQI_THRESHOLD_BIT
#else
  #define MSO_TERTIARY_APPLY_STRICT_LQI_THRESHOLD_BIT 0
#endif

#ifdef EMBER_AF_PLUGIN_RF4CE_MSO_TERTIARY_ENABLE_REQUEST_AUTO_VALIDATION
  #define MSO_TERTIARY_ENABLE_REQUEST_AUTO_VALIDATION_BIT \
    MSO_CLASS_DESCRIPTOR_ENABLE_REQUEST_AUTO_VALIDATION_BIT
#else
  #define MSO_TERTIARY_ENABLE_REQUEST_AUTO_VALIDATION_BIT 0
#endif

// Internal node descriptor control byte.
#define MSO_NODE_DESCRIPTOR_CONTROL_CURRENT_CLASS_DESCRIPTOR_MASK           0x03
#define MSO_NODE_DESCRIPTOR_CONTROL_CURRENT_CLASS_DESCRIPTOR_OFFSET            0
#define MSO_NODE_DESCRIPTOR_CONTROL_CURRENT_CLASS_DESCRIPTOR_PRIMARY        0x00
#define MSO_NODE_DESCRIPTOR_CONTROL_CURRENT_CLASS_DESCRIPTOR_SECONDARY      0x01
#define MSO_NODE_DESCRIPTOR_CONTROL_CURRENT_CLASS_DESCRIPTOR_TERTIARY       0x02
#define MSO_NODE_DESCRIPTOR_CONTROL_CURRENT_CLASS_DESCRIPTOR_RESERVED       0x03
#define MSO_NODE_DESCRIPTOR_CONTROL_ENTRY_IN_USE_BIT                        0x04

// MSO header
// - Frame control (1 byte)
#define MSO_HEADER_LENGTH 1
#define MSO_HEADER_FRAME_CONTROL_OFFSET 0
#define MSO_HEADER_FRAME_CONTROL_COMMAND_CODE_MASK 0xFF

// User Control Pressed
// - RC command code (1 byte)
// - RC command payload (n bytes)
#define USER_CONTROL_PRESSED_LENGTH                    (MSO_HEADER_LENGTH + 1)
#define USER_CONTROL_PRESSED_RC_COMMAND_CODE_OFFSET    (MSO_HEADER_LENGTH)
#define USER_CONTROL_PRESSED_RC_COMMAND_PAYLOAD_OFFSET (MSO_HEADER_LENGTH + 1)

// User Control Repeated
// - RC command code (1 byte, 1.1 only)
// - RC command payload (n bytes, 1.1 only)
#define USER_CONTROL_REPEATED_1_0_LENGTH (MSO_HEADER_LENGTH)
#define USER_CONTROL_REPEATED_1_1_LENGTH (MSO_HEADER_LENGTH + 1)
#define USER_CONTROL_REPEATED_1_1_RC_COMMAND_CODE_OFFSET    (MSO_HEADER_LENGTH)
#define USER_CONTROL_REPEATED_1_1_RC_COMMAND_PAYLOAD_OFFSET (MSO_HEADER_LENGTH + 1)

// User Control Released
// - RC command code (1 byte, 1.1 only)
#define USER_CONTROL_RELEASED_1_0_LENGTH (MSO_HEADER_LENGTH)
#define USER_CONTROL_RELEASED_1_1_LENGTH (MSO_HEADER_LENGTH + 1)
#define USER_CONTROL_RELEASED_1_1_RC_COMMAND_CODE_OFFSET (MSO_HEADER_LENGTH)

// Check Validation Request
// - Check validation control (1 byte)
#define CHECK_VALIDATION_REQUEST_LENGTH (MSO_HEADER_LENGTH + 1)
#define CHECK_VALIDATION_REQUEST_CHECK_VALIDATION_CONTROL_OFFSET (MSO_HEADER_LENGTH)

#define CHECK_VALIDATION_CONTROL_REQUEST_AUTOMATIC_VALIDATION_BIT           0x01

// Check Validation Response
// - Check validation status (1 byte)
#define CHECK_VALIDATION_RESPONSE_LENGTH (MSO_HEADER_LENGTH + 1)
#define CHECK_VALIDATION_RESPONSE_CHECK_VALIDATION_STATUS_OFFSET (MSO_HEADER_LENGTH)

// Set Attribute Request
// - Attribute id (1 byte)
// - Index (1 byte)
// - Value length (1 byte)
// - Value (n bytes)
#define SET_ATTRIBUTE_REQUEST_LENGTH 4
#define SET_ATTRIBUTE_REQUEST_ATTRIBUTE_ID_OFFSET (MSO_HEADER_LENGTH)
#define SET_ATTRIBUTE_REQUEST_INDEX_OFFSET (MSO_HEADER_LENGTH + 1)
#define SET_ATTRIBUTE_REQUEST_VALUE_LENGTH_OFFSET (MSO_HEADER_LENGTH + 2)
#define SET_ATTRIBUTE_REQUEST_VALUE_OFFSET (MSO_HEADER_LENGTH + 3)

// Set Attribute Response
// - Attribute id (1 byte)
// - Index (1 byte)
// - Status (1 byte)
#define SET_ATTRIBUTE_RESPONSE_LENGTH 4
#define SET_ATTRIBUTE_RESPONSE_ATTRIBUTE_ID_OFFSET (MSO_HEADER_LENGTH)
#define SET_ATTRIBUTE_RESPONSE_INDEX_OFFSET (MSO_HEADER_LENGTH + 1)
#define SET_ATTRIBUTE_RESPONSE_STATUS_OFFSET (MSO_HEADER_LENGTH + 2)

// Get Attribute Request
// - Attribute id (1 byte)
// - Index (1 byte)
// - Value length (1 byte)
#define GET_ATTRIBUTE_REQUEST_LENGTH 4
#define GET_ATTRIBUTE_REQUEST_ATTRIBUTE_ID_OFFSET (MSO_HEADER_LENGTH)
#define GET_ATTRIBUTE_REQUEST_INDEX_OFFSET (MSO_HEADER_LENGTH + 1)
#define GET_ATTRIBUTE_REQUEST_VALUE_LENGTH_OFFSET (MSO_HEADER_LENGTH + 2)

// Get Attribute Response
// - Attribute id (1 byte)
// - Index (1 byte)
// - Status (1 byte)
// - Value length (1 byte)
// - Value (n bytes)
#define GET_ATTRIBUTE_RESPONSE_LENGTH 5
#define GET_ATTRIBUTE_RESPONSE_ATTRIBUTE_ID_OFFSET (MSO_HEADER_LENGTH)
#define GET_ATTRIBUTE_RESPONSE_INDEX_OFFSET (MSO_HEADER_LENGTH + 1)
#define GET_ATTRIBUTE_RESPONSE_STATUS_OFFSET (MSO_HEADER_LENGTH + 2)
#define GET_ATTRIBUTE_RESPONSE_VALUE_LENGTH_OFFSET (MSO_HEADER_LENGTH + 3)
#define GET_ATTRIBUTE_RESPONSE_VALUE_OFFSET (MSO_HEADER_LENGTH + 4)

// The User Control Pressed and User Control Repeated commmands theoretically
// take an unbounded additional payload, but the longest additional operand in
// HDMI 1.3a is just four bytes.  Still, just in case, leave an opening for the
// user to override the buffer size.
#ifndef EMBER_AF_PLUGIN_RF4CE_MSO_MAXIMUM_RC_COMMAND_PAYLOAD_LENGTH
  #define EMBER_AF_PLUGIN_RF4CE_MSO_MAXIMUM_RC_COMMAND_PAYLOAD_LENGTH 4
#endif
#define MAXIMUM_USER_CONTROL_X_LENGTH                             \
  (USER_CONTROL_PRESSED_LENGTH                                    \
   + EMBER_AF_PLUGIN_RF4CE_MSO_MAXIMUM_RC_COMMAND_PAYLOAD_LENGTH)

// Assuming the standard operands are used, the Get Attribute Response command
// is the commands with the longest payload in the MSO profile.
#ifndef EMBER_AF_PLUGIN_RF4CE_MSO_MAXIMUM_PAYLOAD_LENGTH
  #if MAXIMUM_USER_CONTROL_X_LENGTH < GET_ATTRIBUTE_RESPONSE_LENGTH + APLC_MAX_RIB_ATTRIBUTE_SIZE
    #define EMBER_AF_PLUGIN_RF4CE_MSO_MAXIMUM_PAYLOAD_LENGTH (GET_ATTRIBUTE_RESPONSE_LENGTH + APLC_MAX_RIB_ATTRIBUTE_SIZE)
  #else
    #define EMBER_AF_PLUGIN_RF4CE_MSO_MAXIMUM_PAYLOAD_LENGTH MAXIMUM_USER_CONTROL_X_LENGTH
  #endif
#endif

#define MSO_SET_VALUE_BINDING_RECIPIENT_PARAMETERS_BYTES_LENGTH                5

// Internal binding state machine.
enum {
  BINDING_STATE_GET_VALIDATION_CONFIG_PENDING         = 0x00,
  BINDING_STATE_CHECK_VALIDATION_INITIAL              = 0x01,
  BINDING_STATE_CHECK_VALIDATION_SENDING_REQUEST      = 0x02,
  BINDING_STATE_CHECK_VALIDATION_IDLING               = 0x03,
  BINDING_STATE_CHECK_VALIDATION_WAIT_FOR_RESPONSE    = 0x04
};

typedef struct {
  uint8_t channel;
  EmberPanId panId;
  EmberEUI64 ieeeAddr;
  uint8_t primaryClassDescriptor;
  uint8_t secondaryClassDescriptor;
  uint8_t tertiaryClassDescriptor;
  uint8_t basicLqiThreshold;
  uint8_t strictLqiThreshold;
  uint8_t rxLqi;
  uint8_t control;
} EmAfMsoPairingCanditate;

extern uint8_t emAfRf4ceMsoBuffer[];
extern uint8_t emAfRf4ceMsoBufferLength;
extern EmberEventControl emberAfPluginRf4ceMsoUserControlEventControl;
extern EmberEventControl emberAfPluginRf4ceMsoCheckValidationEventControl;
extern EmberEventControl emberAfPluginRf4ceMsoSetGetAttributeEventControl;

bool emAfRf4ceMsoIsBlackedOut(uint8_t pairingIndex);
EmberStatus emAfRf4ceMsoSend(uint8_t pairingIndex);
EmberStatus emAfRf4ceMsoSendExtended(uint8_t pairingIndex,
                                     EmberRf4ceTxOption txOptions);
void emAfRf4ceMsoMessageSent(uint8_t pairingIndex,
                             EmberAfRf4ceMsoCommandCode commandCode,
                             const uint8_t *message,
                             uint8_t messageLength,
                             EmberStatus status);
void emAfRf4ceMsoIncomingMessage(uint8_t pairingIndex,
                                 EmberAfRf4ceMsoCommandCode commandCode,
                                 const uint8_t *message,
                                 uint8_t messageLength);

void emAfRf4ceMsoSetValidation(uint8_t pairingIndex,
                               EmberAfRf4ceMsoValidationState state,
                               EmberAfRf4ceMsoCheckValidationStatus status);
void emAfRf4ceMsoInitializeValidationStates(void);
EmberAfRf4ceMsoValidationState emAfRf4ceMsoGetValidationState(uint8_t pairingIndex);
EmberAfRf4ceMsoCheckValidationStatus emAfRf4ceMsoGetValidationStatus(uint8_t pairingIndex);
void emAfPluginRf4ceMsoCheckValidationRequestSentCallback(EmberStatus status,
                                                          uint8_t pairingIndex);
void emAfPluginRf4ceMsoIncomingCheckValidationResponseCallback(EmberAfRf4ceMsoCheckValidationStatus status,
                                                               uint8_t pairingIndex);
void emAfRf4ceMsoValidationConfigurationResponseCallback(EmberAfRf4ceStatus status);
uint8_t emAfRf4ceMsoGetActiveBindPairingIndex(void);
void emAfRf4ceMsoSetActiveBindPairingIndex(uint8_t pairingIndex);

EmberStatus emAfRf4ceMsoSetDiscoveryResponseUserString(void);

void emAfRf4ceMsoInitCommands(void);

#if defined(EMBER_SCRIPTED_TEST)
#include "stack/core/ember-stack.h"
#include "core/scripted-stub.h"

#define debugScriptCheck(reason)                                               \
  simpleScriptCheck("scriptCheck", "scriptCheck: " reason, "")
#else
#define debugScriptCheck(reason)
#endif // EMBER_SCRIPTED_TEST


