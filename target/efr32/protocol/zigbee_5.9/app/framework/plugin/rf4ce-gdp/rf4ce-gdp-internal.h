// Copyright 2014 Silicon Laboratories, Inc.

#define NWK_DISCOVERY_REPETITION_INTERVAL_MS 500
#define NWK_MAX_DISCOVERY_REPETITIONS        2
#define NWK_MAX_REPORTED_NODE_DESCRIPTORS    0xFF

// TODO: according to the specs, this parameter is implementation specific,
// therefore it will eventually become one of the plugin options.
#define NWK_DISCOVERY_LQI_THRESHOLD          0x01

// TODO: according to the specs, this value defaults to 3, however it can still
// be configured by the application, therefore it will eventually become one of
// the plugin options.
#define APL_MAX_PAIRING_CANDIDATES           3

// The length of time after completing the pairing procedure or a configuration
// phase of a profile during which no packets shall be sent to allow the remote
// nodes to perform internal housekeeping tasks.
#define APLC_CONFIG_BLACKOUT_TIME_MS 100

#define BLACKOUT_TIME_DELTA_MS       5

// At the originator the blackout time is increased by a delta value, to ensure
// that the recipient is already on listening for packets.
#define BLACKOUT_TIME_ORIGINATOR_MS   (APLC_CONFIG_BLACKOUT_TIME_MS + BLACKOUT_TIME_DELTA_MS)
// At the recipient the blackout time is reduced by a delta value, to ensure
// that we are listening for incoming packets from the originator.
#define BLACKOUT_TIME_RECIPIENT_MS    (APLC_CONFIG_BLACKOUT_TIME_MS - BLACKOUT_TIME_DELTA_MS)

// The duration of the binding window.  On a Binding Originator, section 7.2.1
// describes how this constant is used.  On the Binding Recipient, section
// 7.2.3.2 describes how this constant is used.
#define APLC_BIND_WINDOW_DURATION_MS (30 * MILLISECOND_TICKS_PER_SECOND)

// The maximum allowed value for the aplMaxAutoCheckValidationPeriod attribute.
#define APLC_MAX_AUTO_CHECK_VALIDATION_PERIOD_MS (10 * MILLISECOND_TICKS_PER_SECOND)

// The maximum time the Binding Recipient shall wait to receive a command frame
// from a Binding Originator during its configuration phase. 
#define APLC_MAX_CONFIG_WAIT_TIME_MS 100

// The maximal time the validation can take in normal validation mode (see
// section 7.2.7).
#define APLC_MAX_NORMAL_VALIDATION_DURATION_MS    25000

// The maximal time the validation can take in extended validation mode (see
// section 7.2.7).
#define APLC_MAX_EXTENDED_VALIDATION_DURATION_MS  65000

// The maximum allowed value to configure the polling timeout in the
// aplPollConfiguration attribute.
#define APLC_MAX_POLLING_TIMEOUT_MS 100

// The maximum time a node shall leave its receiver on in order to receive data
// indicated via the data pending subfield of the frame control field of an
// incoming frame.
#define APLC_MAX_RX_ON_WAIT_TIME_MS 100

// The maximum time a node shall wait for a response command frame following a
// request command frame.
#define APLC_MAX_RESPONSE_WAIT_TIME_MS 100

// The minimum value of the KeyExchangeTransferCount parameter passed to the
// pair request primitive during the validation based pairing procedure.
#define APLC_MIN_KEY_EXCHANGE_TRANSFER_COUNT 3

// GDP header
// - Frame control (1 byte)
#define GDP_HEADER_LENGTH                               1
#define GDP_HEADER_FRAME_CONTROL_OFFSET                 0
#define GDP_HEADER_FRAME_CONTROL_COMMAND_CODE_MASK      0x0F
#define GDP_HEADER_FRAME_CONTROL_GDP_COMMAND_FRAME_MASK 0x40
#define GDP_HEADER_FRAME_CONTROL_DATA_PENDING_MASK      0x80
#define GDP_PAYLOAD_OFFSET                              1

// Generic Response
// - Response code (1 byte)
#define GENERIC_RESPONSE_LENGTH               (GDP_HEADER_LENGTH + 1)
#define GENERIC_RESPONSE_RESPONSE_CODE_OFFSET (GDP_HEADER_LENGTH)

// Configuration Complete
// - Status (1 byte)
#define CONFIGURATION_COMPLETE_LENGTH        (GDP_HEADER_LENGTH + 1)
#define CONFIGURATION_COMPLETE_STATUS_OFFSET (GDP_HEADER_LENGTH)

// Heartbeat
// - Trigger (1 byte)
#define HEARTBEAT_LENGTH         (GDP_HEADER_LENGTH + 1)
#define HEARTBEAT_TRIGGER_OFFSET (GDP_HEADER_LENGTH)

// Get Attributes
// - Attribute identification record (1/3 bytes)
//   - Attribute identifier (1 byte)
//   - Entry identifier (0/2 bytes)
// - ...
#define GET_ATTRIBUTES_LENGTH (GDP_HEADER_LENGTH + 1)

// Get Attributes Response
// - Attribute status record (n bytes)
//   - Attribute identifier (1 byte)
//   - Entry identifier (0/2 bytes)
//   - Attribute status (1 byte)
//   - Attribute length (0/1 byte)
//   - Attribute value (n bytes)
// - ...
#define GET_ATTRIBUTES_RESPONSE_LENGTH (GDP_HEADER_LENGTH + 2)

// Push Attributes
// - Attribute record (n bytes)
//   - Attribute identifier (1 byte)
//   - Entry identifier (0/2 bytes)
//   - Attribute length (1 byte)
//   - Attribute value (n bytes)
// - ...
#define PUSH_ATTRIBUTES_LENGTH (GDP_HEADER_LENGTH + 2)

// Set Attributes
// - Attribute record (n bytes)
//   - Attribute identifier (1 byte)
//   - Entry identifier (0/2 bytes)
//   - Attribute length (1 byte)
//   - Attribute value (n bytes)
// - ...
#define SET_ATTRIBUTES_LENGTH (GDP_HEADER_LENGTH + 2)

// Pull Attributes
// - Attribute identification record (1/3 bytes)
//   - Attribute identifier (1 byte)
//   - Entry identifier (0/2 bytes)
// - ...
#define PULL_ATTRIBUTES_LENGTH (GDP_HEADER_LENGTH + 1)

// Pull Attributes Response
// - Attribute record (n bytes)
//   - Attribute identifier (1 byte)
//   - Entry identifier (0/2 bytes)
//   - Attribute status (1 byte)
//   - Attribute length (0/1 byte)
//   - Attribute value (n bytes)
// - ...
#define PULL_ATTRIBUTES_RESPONSE_LENGTH (GDP_HEADER_LENGTH + 2)

// Check Validation
// - Check validation subtype (1 byte)
// - Check validation payload (n bytes)
#define CHECK_VALIDATION_LENGTH                         (GDP_HEADER_LENGTH + 1)
#define CHECK_VALIDATION_SUBTYPE_OFFSET                 (GDP_HEADER_LENGTH)
#define CHECK_VALIDATION_PAYLOAD_OFFSET                 (GDP_HEADER_LENGTH + 1)
#define CHECK_VALIDATION_SUBTYPE_REQUEST_LENGTH         (GDP_HEADER_LENGTH + 2)
#define CHECK_VALIDATION_SUBTYPE_REQUEST_CONTROL_OFFSET (GDP_HEADER_LENGTH + 1)
#define CHECK_VALIDATION_SUBTYPE_RESPONSE_LENGTH        (GDP_HEADER_LENGTH + 2)
#define CHECK_VALIDATION_SUBTYPE_RESPONSE_STATUS_OFFSET (GDP_HEADER_LENGTH + 1)

// Client Notification
// - Client notification subtype (1 byte)
// - Client notification payload (n bytes)
#define CLIENT_NOTIFICATION_LENGTH                                  (GDP_HEADER_LENGTH + 1)
#define CLIENT_NOTIFICATION_SUBTYPE_OFFSET                          (GDP_HEADER_LENGTH)
#define CLIENT_NOTIFICATION_PAYLOAD_OFFSET                          (GDP_HEADER_LENGTH + 1)
#define CLIENT_NOTIFICATION_SUBTYPE_IDENTIFY_LENGTH                 (GDP_HEADER_LENGTH + 4)
#define CLIENT_NOTIFICATION_SUBTYPE_IDENTIFY_PAYLOAD_LENGTH         3
#define CLIENT_NOTIFICATION_SUBTYPE_IDENTIFY_PAYLOAD_FLAGS_OFFSET   0
#define CLIENT_NOTIFICATION_SUBTYPE_IDENTIFY_PAYLOD_TIME_OFFSET     1
#define CLIENT_NOTIFICATION_SUBTYPE_REQUEST_POLL_NEGOTIATION_LENGTH (GDP_HEADER_LENGTH + 1)
#define CLIENT_NOTIFICATION_SUBTYPE_REQUEST_POLL_NEGOTIATION_PAYLOAD_LENGTH    0

// Key Exchange
// - Key exchange subtype (1 byte)
// - Key exchange payload (n bytes)
#define KEY_EXCHANGE_LENGTH                                   (GDP_HEADER_LENGTH + 1)
#define KEY_EXCHANGE_SUBTYPE_OFFSET                           (GDP_HEADER_LENGTH)
#define KEY_EXCHANGE_PAYLOAD_OFFSET                           (GDP_HEADER_LENGTH + 1)
#define KEY_EXCHANGE_SUBTYPE_CHALLENGE_LENGTH                 (GDP_HEADER_LENGTH + 11)
#define KEY_EXCHANGE_SUBTYPE_CHALLENGE_FLAGS_OFFSET           (GDP_HEADER_LENGTH + 1)
#define KEY_EXCHANGE_SUBTYPE_CHALLENGE_RAND_A_OFFSET          (GDP_HEADER_LENGTH + 3)
#define KEY_EXCHANGE_SUBTYPE_CHALLENGE_RESPONSE_LENGTH        (GDP_HEADER_LENGTH + 15)
#define KEY_EXCHANGE_SUBTYPE_CHALLENGE_RESPONSE_FLAGS_OFFSET  (GDP_HEADER_LENGTH + 1)
#define KEY_EXCHANGE_SUBTYPE_CHALLENGE_RESPONSE_RAND_B_OFFSET (GDP_HEADER_LENGTH + 3)
#define KEY_EXCHANGE_SUBTYPE_CHALLENGE_RESPONSE_TAG_B_OFFSET  (GDP_HEADER_LENGTH + 11)
#define KEY_EXCHANGE_SUBTYPE_RESPONSE_LENGTH                  (GDP_HEADER_LENGTH + 5)
#define KEY_EXCHANGE_SUBTYPE_RESPONSE_TAG_A_OFFSET            (GDP_HEADER_LENGTH + 1)
#define KEY_EXCHANGE_SUBTYPE_CONFIRM_LENGTH                   (GDP_HEADER_LENGTH + 1)

#define COMMAND_CODE_MAXIMUM EMBER_AF_RF4CE_GDP_COMMAND_KEY_EXCHANGE

// GDP capabilities
#define GDP_CAPABILITIES_SUPPORT_EXTENDED_VALIDATION_BIT               0x00000001
#define GDP_CAPABILITIES_SUPPORT_EXTENDED_VALIDATION_OFFSET            0
#define GDP_CAPABILITIES_SUPPORT_POLL_SERVER_BIT                       0x00000002
#define GDP_CAPABILITIES_SUPPORT_POLL_SERVER_OFFSET                    1
#define GDP_CAPABILITIES_SUPPORT_POLL_CLIENT_BIT                       0x00000004
#define GDP_CAPABILITIES_SUPPORT_POLL_CLIENT_OFFSET                    2
#define GDP_CAPABILITIES_SUPPORT_IDENTIFICATION_SERVER_BIT             0x00000008
#define GDP_CAPABILITIES_SUPPORT_IDENTIFICATION_SERVER_OFFSET          3
#define GDP_CAPABILITIES_SUPPORT_IDENTIFICATION_CLIENT_BIT             0x00000010
#define GDP_CAPABILITIES_SUPPORT_IDENTIFICATION_CLIENT_OFFSET          4
#define GDP_CAPABILITIES_SUPPORT_ENHANCED_SECURITY_BIT                 0x00000020
#define GDP_CAPABILITIES_SUPPORT_ENHANCED_SECURITY_OFFSET              5
#define GDP_CAPABILITIES_SUPPORT_SHARED_SECRET_OF_LOCAL_VENDOR_BIT     0x00000040
#define GDP_CAPABILITIES_SUPPORT_SHARED_SECRET_OF_LOCAL_VENDOR_OFFSET  6
#define GDP_CAPABILITIES_SUPPORT_SHARED_SECRET_OF_REMOTE_VENDOR_BIT    0x00000080
#define GDP_CAPABILITIES_SUPPORT_SHARED_SECRET_OF_REMOTE_VENDOR_OFFSET 7

// Bits 8-31 are reserved

#define GDP_STANDARD_SHARED_SECRET {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66,  \
                                    0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD,  \
                                    0xEE, 0xFF}
#define GDP_SHARED_SECRET_SIZE     16

// Application specific user string.
#define USER_STRING_APPLICATION_SPECIFIC_USER_STRING_OFFSET   0
#define USER_STRING_APPLICATION_SPECIFIC_USER_STRING_LENGTH   8

extern const uint8_t emAfRf4ceGdpApplicationSpecificUserString[USER_STRING_APPLICATION_SPECIFIC_USER_STRING_LENGTH];

// Null byte delimiter
#define USER_STRING_NULL_BYTE_OFFSET                          8

// Discovery request user string bytes.
#define USER_STRING_DISC_REQUEST_VENDOR_ID_FILTER_OFFSET                    9
#define USER_STRING_DISC_REQUEST_VENDOR_ID_FILTER_LENGTH                    2
#define USER_STRING_DISC_REQUEST_MIN_MAX_CLASS_FILTER_OFFSET                11
#define USER_STRING_DISC_REQUEST_MIN_MAX_CLASS_FILTER_MIN_CLASS_NUM_MASK    0x0F
#define USER_STRING_DISC_REQUEST_MIN_MAX_CLASS_FILTER_MIN_CLASS_NUM_OFFSET  0
#define USER_STRING_DISC_REQUEST_MIN_MAX_CLASS_FILTER_MAX_CLASS_NUM_MASK    0xF0
#define USER_STRING_DISC_REQUEST_MIN_MAX_CLASS_FILTER_MAX_CLASS_NUM_OFFSET  4
#define USER_STRING_DISC_REQUEST_MIN_LQI_FILTER_OFFSET                      12
#define USER_STRING_DISC_REQUEST_RESERVED_BYTES_OFFSET                      13
#define USER_STRING_DISC_REQUEST_RESERVED_BYTES_LENGTH                      2

// Discovery response user string bytes.
#define USER_STRING_DISC_RESPONSE_RESERVED_BYTES_OFFSET                     9
#define USER_STRING_DISC_RESPONSE_RESERVED_BYTES_LENGTH                     2
#define USER_STRING_DISC_RESPONSE_TERTIARY_CLASS_DESCRIPTOR_OFFSET          11
#define USER_STRING_DISC_RESPONSE_SECONDARY_CLASS_DESCRIPTOR_OFFSET         12
#define USER_STRING_DISC_RESPONSE_PRIMARY_CLASS_DESCRIPTOR_OFFSET           13
#define USER_STRING_DISC_RESPONSE_DISCOVERY_LQI_THRESHOLD_OFFSET            14
#define USER_STRING_PAIR_REQUEST_ADVANCED_BINDING_SUPPORT_OFFSET            9
#define USER_STRING_PAIR_REQUEST_RESERVED_BYTES_OFFSET                      10
#define USER_STRING_PAIR_REQUEST_RESERVED_BYTES_LENGTH                      5

// Advanced binding support field
#define ADVANCED_BINDING_SUPPORT_FIELD_BINDING_PROXY_SUPPORTED_BIT          0x01
// Bits 1-7 are reserved

// Class descriptor
#define CLASS_DESCRIPTOR_NUMBER_MASK                                        0x0F
#define CLASS_DESCRIPTOR_NUMBER_OFFSET                                      0
#define CLASS_DESCRIPTOR_DUPLICATE_HANDLING_MASK                            0x30
#define CLASS_DESCRIPTOR_DUPLICATE_HANDLING_OFFSET                          4
#define CLASS_DESCRIPTOR_RESERVED_MASK                                      0xC0
// Class numbers.
#define CLASS_NUMBER_PRE_COMMISSIONED                                       0x00
#define CLASS_NUMBER_BUTTON_PRESS_INDICATION                                0x01
// Values 0x2-0xE are implementation specific.
#define CLASS_NUMBER_DISCOVERABLE_ONLY                                      0x0F

// Class descriptor duplicate handling criteria.
#define CLASS_DESCRIPTOR_DUPLICATE_HANDLING_AS_IS                           0x00
#define CLASS_DESCRIPTOR_DUPLICATE_HANDLING_RECLASSIFY                      0x01
#define CLASS_DESCRIPTOR_DUPLICATE_HANDLING_ABORT                           0x02
#define CLASS_DESCRIPTOR_DUPLICATE_HANDLING_RESERVED                        0x03

// These are used in App Builder.
#define AS_IS         CLASS_DESCRIPTOR_DUPLICATE_HANDLING_AS_IS
#define RECLASSIFY    CLASS_DESCRIPTOR_DUPLICATE_HANDLING_RECLASSIFY
#define ABORT         CLASS_DESCRIPTOR_DUPLICATE_HANDLING_ABORT

#define GDP_VERSION_NONE   0x00
#define GDP_VERSION_1_X    0x01
#define GDP_VERSION_2_0    0x02

// For now the only GDP 1.x based profile is ZID
// When adding new profile IDs to this list, make sure the list stays sorted.
#define GDP_1_X_BASED_PROFILE_ID_LIST                                          \
  {EMBER_AF_RF4CE_PROFILE_INPUT_DEVICE_1_0}
#define GDP_1_X_BASED_PROFILE_ID_LIST_LENGTH      1

// For now the only GDP 2.0 based profile is ZRC 2.0
// When adding new profile IDs to this list, make sure the list stays sorted.
#define GDP_2_0_BASED_PROFILE_ID_LIST                                          \
  {EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0}
#define GDP_2_0_BASED_PROFILE_ID_LIST_LENGTH      1

extern const uint8_t emAfGdp1xProfiles[];
extern const uint8_t emAfGdp20Profiles[];

bool emAfIsProfileGdpBased(uint8_t profileId, uint8_t gdpCheckVersion);

bool emAfRf4ceIsProfileSupported(uint8_t profileId,
                                    const uint8_t *profileIdList,
                                    uint8_t profileIdListLength);

uint8_t emAfCheckDeviceTypeAndProfileIdMatch(uint8_t checkDeviceType,
                                           uint8_t *compareDevTypeList,
                                           uint8_t compareDevTypeListLength,
                                           uint8_t *checkProfileIdList,
                                           uint8_t checkProfileIdListLength,
                                           uint8_t *compareProfileIdList,
                                           uint8_t compareProfileIdListLength,
                                           uint8_t *matchingProfileIdList);

bool emAfRf4ceGdpMaybeStartNextProfileSpecificConfigurationProcedure(bool isOriginator,
                                                                        const uint8_t *remoteProfileIdList,
                                                                        uint8_t remoteProfileIdListLength);

void emAfRf4ceGdpNotifyBindingCompleteToProfiles(EmberAfRf4ceGdpBindingStatus status,
                                                 uint8_t pairingIndex,
                                                 const uint8_t *remoteProfileIdList,
                                                 uint8_t remoteProfileIdListLength);

EmberStatus emAfRf4ceGdpInitiateKeyExchangeInternal(uint8_t pairingIndex,
                                                    bool intCall);

void emAfRf4ceGdpNoteProfileSpecificConfigurationStart(void);

void emAfGdpAddToProfileIdList(uint8_t *srcProfileIdList,
                               uint8_t srcProfileIdListLength,
                               EmberRf4ceApplicationInfo *destAppInfo,
                               uint8_t gdpVersion);

uint8_t emAfRf4ceGdpGetGdpVersion(const uint8_t *profileIdList,
                                uint8_t profileIdListLength);

extern uint8_t emAfTemporaryPairingIndex;
extern uint8_t emAfCurrentProfileSpecificIndex;
extern uint16_t emAfGdpState;
extern uint32_t emberAfPluginRf4ceGdpCapabilities;
extern EmberEventControl emberAfPluginRf4ceGdpPendingCommandEventControl;
extern EmberEventControl emberAfPluginRf4ceGdpBlackoutTimeEventControl;
extern EmberEventControl emberAfPluginRf4ceGdpValidationEventControl;

// We use the emAfGdpState to store both the public (dormant, not-bound,
// binding and bound) state and the internal states.
#define PUBLIC_STATE_MASK                                         0x03
#define INTERNAL_STATE_MASK                                       0xFFFC
#define INTERNAL_STATE_OFFSET                                     2

// Internal states
#define INTERNAL_STATE_NONE                                           (0x00 << INTERNAL_STATE_OFFSET)
// Originator binding states
#define INTERNAL_STATE_ORIGINATOR_GDP_CONFIG_PUSH_PENDING             (0x01 << INTERNAL_STATE_OFFSET)
#define INTERNAL_STATE_ORIGINATOR_GDP_CONFIG_GET_PENDING              (0x02 << INTERNAL_STATE_OFFSET)
#define INTERNAL_STATE_ORIGINATOR_GDP_CONFIG_PULL_PENDING             (0x03 << INTERNAL_STATE_OFFSET)
#define INTERNAL_STATE_ORIGINATOR_GDP_CONFIG_COMPLETE_PENDING         (0x04 << INTERNAL_STATE_OFFSET)
#define INTERNAL_STATE_ORIGINATOR_GDP_PROFILES_CONFIG                 (0x05 << INTERNAL_STATE_OFFSET)
#define INTERNAL_STATE_ORIGINATOR_GDP_VALIDATION                      (0x06 << INTERNAL_STATE_OFFSET)
#define INTERNAL_STATE_ORIGINATOR_GDP_KEY_EXCHANGE_BLACKOUT_PENDING   (0x07 << INTERNAL_STATE_OFFSET)
// Recipient binding states
#define INTERNAL_STATE_RECIPIENT_GDP_STACK_STATUS_NETWORK_UP_PENDING  (0x08 << INTERNAL_STATE_OFFSET)
#define INTERNAL_STATE_RECIPIENT_GDP_RESTORE_PAIRING_ENTRY_PENDING    (0x09 << INTERNAL_STATE_OFFSET)
#define INTERNAL_STATE_RECIPIENT_GDP_CONFIG_WAITING_FOR_PUSH          (0x0A << INTERNAL_STATE_OFFSET)
#define INTERNAL_STATE_RECIPIENT_GDP_CONFIG_WAITING_FOR_GET           (0x0B << INTERNAL_STATE_OFFSET)
#define INTERNAL_STATE_RECIPIENT_GDP_CONFIG_WAITING_FOR_PULL          (0x0C << INTERNAL_STATE_OFFSET)
#define INTERNAL_STATE_RECIPIENT_GDP_CONFIG_WAITING_FOR_COMPLETE      (0x0D << INTERNAL_STATE_OFFSET)
#define INTERNAL_STATE_RECIPIENT_GDP_PROFILES_CONFIG                  (0x0E << INTERNAL_STATE_OFFSET)
#define INTERNAL_STATE_RECIPIENT_GDP_VALIDATION                       (0x0F << INTERNAL_STATE_OFFSET)
// Security key exchange procedure states
#define INTERNAL_STATE_GDP_SECURITY_KEY_CHALLENGE_PENDING             (0x10 << INTERNAL_STATE_OFFSET)
#define INTERNAL_STATE_GDP_SECURITY_KEY_CHALLENGE_RESPONSE_PENDING    (0x11 << INTERNAL_STATE_OFFSET)
#define INTERNAL_STATE_GDP_SECURITY_KEY_RESPONSE_PENDING              (0x12 << INTERNAL_STATE_OFFSET)
// Poll negotation procedure states
#define INTERNAL_STATE_GDP_POLL_CONFIG_CLIENT_PUSH_PENDING            (0x13 << INTERNAL_STATE_OFFSET)
#define INTERNAL_STATE_GDP_POLL_CONFIG_CLIENT_PULL_PENDING            (0x14 << INTERNAL_STATE_OFFSET)
// Identification procedure states
#define INTERNAL_STATE_GDP_IDENTIFICATION_CLIENT_PUSH_PENDING         (0x15 << INTERNAL_STATE_OFFSET)
// Poll states
#define INTERNAL_STATE_GDP_POLL_CLIENT_HEARTBEAT_PENDING              (0x16 << INTERNAL_STATE_OFFSET)

#define publicBindState()     (emAfGdpState & PUBLIC_STATE_MASK)
#define internalGdpState()   (emAfGdpState & INTERNAL_STATE_MASK)

// Setting the public state clears the internal state (implicitly set to NONE).
#define setPublicState(state, init)                                            \
  do {                                                                         \
    emAfGdpState = (state);                                                    \
    if (!init) {                                                               \
      emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE, false);      \
    }                                                                          \
  } while(0)

// For any state other than NONE we keep the receiver ON.
// The "STACK_STATUS_NETWORK_UP_PENDING" is a special state that is set in the
// init function at the recipient. Since we can not control the order of the
// plugin init functions calls, we might end up calling the RxEnable before the
// profile plugin has been initialized. For this reason, we don't call the
// RxEnable() in this state.
#define setInternalState(state)                                                \
  do {                                                                         \
    emAfGdpState = ((emAfGdpState & PUBLIC_STATE_MASK) | (state));             \
    if ((state) != INTERNAL_STATE_RECIPIENT_GDP_STACK_STATUS_NETWORK_UP_PENDING) { \
      emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE,              \
                           ((state) != INTERNAL_STATE_NONE));                  \
    }                                                                          \
  } while(0)

#define isInternalStateBindingOriginator()                                     \
  (internalGdpState() >= INTERNAL_STATE_ORIGINATOR_GDP_CONFIG_PUSH_PENDING     \
   && internalGdpState() <= INTERNAL_STATE_ORIGINATOR_GDP_KEY_EXCHANGE_BLACKOUT_PENDING)

#define isInternalStateBindingRecipient()                                      \
  (internalGdpState() >= INTERNAL_STATE_RECIPIENT_GDP_STACK_STATUS_NETWORK_UP_PENDING \
   && internalGdpState() <= INTERNAL_STATE_RECIPIENT_GDP_VALIDATION)

#define isInternalStateSecurity()                                              \
  (internalGdpState() >= INTERNAL_STATE_GDP_SECURITY_KEY_CHALLENGE_PENDING     \
   && internalGdpState() <= INTERNAL_STATE_GDP_SECURITY_KEY_RESPONSE_PENDING)

#define isInternalStatePollNegotiation()                                       \
  (internalGdpState() >= INTERNAL_STATE_GDP_POLL_CONFIG_CLIENT_PUSH_PENDING    \
   && internalGdpState() <= INTERNAL_STATE_GDP_POLL_CONFIG_CLIENT_PULL_PENDING)

#define isInternalStateIdentification()                                        \
  (internalGdpState() == INTERNAL_STATE_GDP_IDENTIFICATION_CLIENT_PUSH_PENDING)

// Pairing candidate info byte.

#define CANDIDATE_INFO_ENTRY_IN_USE_BIT             0x01
#define CANDIDATE_INFO_ENTRY_IN_USE_OFFSET          0
#define CANDIDATE_INFO_PAIRING_ATTEMPTED_BIT        0x02
#define CANDIDATE_INFO_PAIRING_ATTEMPTED_OFFSET     1
#define CANDIDATE_INFO_PROXY_CANDIDATE_BIT          0x04
#define CANDIDATE_INFO_PROXY_CANDIDATE_OFFSET       2

typedef struct {
  EmberEUI64 ieeeAddr;
  EmberPanId panId;
  uint8_t supportedProfiles[EMBER_RF4CE_APPLICATION_PROFILE_ID_LIST_MAX_LENGTH];
  uint8_t supportedProfilesLength;
  uint8_t channel;
  uint8_t primaryClassDescriptor;
  uint8_t secondaryClassDescriptor;
  uint8_t tertiaryClassDescriptor;
  uint8_t rxLqi;
  uint8_t info;
} EmAfGdpPairingCanditate;

typedef struct {
  EmberEUI64 srcIeeeAddr;
  uint8_t nodeCapabilities;
  EmberRf4ceVendorInfo vendorInfo;
  EmberRf4ceApplicationInfo appInfo;
  uint8_t searchDevType;
} EmAfDiscoveryOrPairRequestData;

typedef struct {
  uint8_t localConfigurationStatus;
  uint8_t candidateIndex;
} EmAfBindingInfo;

extern EmAfBindingInfo emAfGdpPeerInfo;

// Pairing table entry bind status
#define PAIRING_ENTRY_BINDING_STATUS_MASK                             0x03
#define PAIRING_ENTRY_BINDING_STATUS_OFFSET                           0
#define PAIRING_ENTRY_BINDING_STATUS_NOT_BOUND                        0x00
#define PAIRING_ENTRY_BINDING_STATUS_BOUND_ORIGINATOR                 0x01
#define PAIRING_ENTRY_BINDING_STATUS_BOUND_RECIPIENT                  0x02
#define PAIRING_ENTRY_BINDING_COMPLETE_BIT                            0x04
#define PAIRING_ENTRY_BINDING_COMPLETE_OFFSET                         2
#define PAIRING_ENTRY_POLLING_ACTIVE_BIT                              0x08
#define PAIRING_ENTRY_POLLING_ACTIVE_OFFSET                           3
#define PAIRING_ENTRY_IDENTIFICATION_ACTIVE_BIT                       0x10
#define PAIRING_ENTRY_IDENTIFICATION_ACTIVE_OFFSET                    4
#define PAIRING_ENTRY_REMOTE_NODE_SUPPORTS_ENHANCED_SECURITY_BIT      0x20
#define PAIRING_ENTRY_REMOTE_NODE_SUPPORTS_ENHANCED_SECURITY_OFFSET   5
#define PAIRING_ENTRY_REMOTE_NODE_SUPPORTS_IDENTIFICATION_BIT         0x40
#define PAIRING_ENTRY_REMOTE_NODE_SUPPORTS_IDENTIFICATION_OFFSET      6
#define PAIRING_ENTRY_REMOTE_NODE_SUPPORTS_POLLING_BIT                0x80
#define PAIRING_ENTRY_REMOTE_NODE_SUPPORTS_POLLING_OFFSET             7

void emAfRf4ceGdpRecipientInitCallback(void);
void emAfRf4ceGdpOriginatorStackStatusCallback(EmberStatus status);
void emAfRf4ceGdpRecipientStackStatusCallback(EmberStatus status);

void emAfRf4ceGdpUpdatePublicStatus(bool init);

uint8_t emAfRf4ceGdpGetPairingBindStatus(uint8_t pairingIndex);
void emAfRf4ceGdpSetPairingBindStatus(uint8_t pairingIndex, uint8_t status);
void emAfRf4ceGdpGetPairingKey(uint8_t pairingIndex, EmberKeyData *key);
void emAfRf4ceGdpSetPairingKey(uint8_t pairingIndex, EmberKeyData *key);

// returns true if the GDP plugin is busy doing something, false otherwise.
#define emAfRf4ceGdpIsBusy() (internalGdpState() != INTERNAL_STATE_NONE)

bool emAfRf4ceGdpSecurityGetRandomString(EmberAfRf4ceGdpRand *rn);

void emAfRf4ceGdpSecurityValidationCompleteCallback(uint8_t pairingIndex);

void emAfRf4ceGdpAttributesInitCallback(void);
void emAfRf4ceGdpIncomingGenericResponse(EmberAfRf4ceGdpResponseCode responseCode);
void emAfRf4ceGdpIncomingConfigurationComplete(EmberAfRf4ceGdpStatus status);
void emAfRf4ceGdpIncomingHeartbeat(EmberAfRf4ceGdpHeartbeatTrigger trigger);
void emAfRf4ceGdpIncomingGetAttributes(void);
void emAfRf4ceGdpIncomingGetAttributesResponse(void);
void emAfRf4ceGdpIncomingPushAttributes(void);
void emAfRf4ceGdpIncomingSetAttributes(void);
void emAfRf4ceGdpIncomingPullAttributes(void);
void emAfRf4ceGdpIncomingPullAttributesResponse(void);
void emAfRf4ceGdpIncomingCheckValidationRequest(uint8_t control);
void emAfRf4ceGdpIncomingCheckValidationResponse(EmberAfRf4ceGdpCheckValidationStatus status);
void emAfRf4ceGdpCheckValidationResponseSent(EmberStatus status);
void emAfRf4ceGdpHeartbeatSent(EmberStatus status);
void emAfRf4ceGdpIncomingKeyExchangeChallenge(EmberAfRf4ceGdpKeyExchangeFlags flags,
                                              const EmberAfRf4ceGdpRand *randA);
void emAfRf4ceGdpIncomingKeyExchangeChallengeResponse(EmberAfRf4ceGdpKeyExchangeFlags flags,
                                                      const EmberAfRf4ceGdpRand *randB,
                                                      const EmberAfRf4ceGdpTag *tagB);
void emAfRf4ceGdpIncomingKeyExchangeResponse(const EmberAfRf4ceGdpTag *tagA);
void emAfRf4ceGdpKeyExchangeResponseSent(EmberStatus status);
void emAfRf4ceGdpIncomingKeyExchangeConfirm(bool secured);
void emAfRf4ceGdpIncomingClientNotification(EmberAfRf4ceGdpClientNotificationSubtype subType,
                                            const uint8_t *clientNotificationPayload,
                                            uint8_t clientNotificationPayloadLength);

void emAfRf4ceZrcIncomingGenericResponse(EmberAfRf4ceGdpResponseCode responseCode);
void emAfRf4ceZrcIncomingConfigurationComplete(EmberAfRf4ceGdpStatus status);
void emAfRf4ceZrcIncomingGetAttributes(void);
void emAfRf4ceZrcIncomingGetAttributesResponse(void);
void emAfRf4ceZrcIncomingPushAttributes(void);
void emAfRf4ceZrcIncomingSetAttributes(void);
void emAfRf4ceZrcIncomingPullAttributes(void);
void emAfRf4ceZrcIncomingPullAttributesResponse(void);
void emAfRf4ceZrcIncomingClientNotification(EmberAfRf4ceGdpClientNotificationSubtype subType,
                                            const uint8_t *clientNotificationPayload,
                                            uint8_t clientNotificationPayloadLength);

bool emAfRf4ceGdpHasAttributeRecord(void);
bool emAfRf4ceGdpAppendAttributeIdentificationRecord(const EmberAfRf4ceGdpAttributeIdentificationRecord *record);
bool emAfRf4ceGdpFetchAttributeIdentificationRecord(EmberAfRf4ceGdpAttributeIdentificationRecord *record);
bool emAfRf4ceGdpAppendAttributeStatusRecord(const EmberAfRf4ceGdpAttributeStatusRecord *record);
bool emAfRf4ceGdpFetchAttributeStatusRecord(EmberAfRf4ceGdpAttributeStatusRecord *record);
bool emAfRf4ceGdpAppendAttributeRecord(const EmberAfRf4ceGdpAttributeRecord *record);
bool emAfRf4ceGdpFetchAttributeRecord(EmberAfRf4ceGdpAttributeRecord *record);
void emAfRf4ceGdpResetFetchAttributeFinger(void);
void emAfRf4ceGdpStartAttributesCommand(EmberAfRf4ceGdpCommandCode commandCode);
EmberStatus emAfRf4ceGdpSendAttributesCommand(uint8_t pairingIndex,
                                              uint8_t profileId,
                                              uint16_t vendorId);

EmberStatus emAfRf4ceGdpSetDiscoveryResponseAppInfo(bool pushButton,
                                                    uint8_t gdpVersion);
EmberStatus emAfRf4ceGdpSetPairResponseAppInfo(const EmberRf4ceApplicationInfo *pairRequestAppInfo);

EmberStatus emAfRf4ceGdpSendProfileSpecificCommand(uint8_t pairingIndex,
                                                   uint8_t profileId,
                                                   uint16_t vendorId,
                                                   EmberRf4ceTxOption txOptions,
                                                   uint8_t commandId,
                                                   uint8_t *commandPayload,
                                                   uint8_t commandPayloadLength,
                                                   uint8_t *messageTag);

EmberStatus emAfRf4ceGdpKeyExchangeChallenge(uint8_t pairingIndex,
                                             uint16_t vendorId,
                                             EmberAfRf4ceGdpKeyExchangeFlags flags,
                                             const EmberAfRf4ceGdpRand *randA);

EmberStatus emAfRf4ceGdpKeyExchangeChallengeResponse(uint8_t pairingIndex,
                                                     uint16_t vendorId,
                                                     EmberAfRf4ceGdpKeyExchangeFlags flags,
                                                     const EmberAfRf4ceGdpRand *randB,
                                                     const EmberAfRf4ceGdpTag *tagB);

EmberStatus emAfRf4ceGdpKeyExchangeResponse(uint8_t pairingIndex,
                                            uint16_t vendorId,
                                            const EmberAfRf4ceGdpTag *tagA);

EmberStatus emAfRf4ceGdpKeyExchangeConfirm(uint8_t pairingIndex,
                                           uint16_t vendorId);

// Host stuff

// Vendor ID filter (2 bytes) + minMaxClassFilter (1 byte)
// + minLqiFilter (1 byte)
#define GDP_SET_VALUE_BINDING_ORIGINATOR_PARAMETERS_BYTES_LENGTH               4

// Primary class descriptor (1 byte) + secondary class descriptor (1 byte)
// + tertiary class descriptor (1 byte) + discovery LQI threshold (1 byte)
#define GDP_SET_VALUE_BINDING_RECIPIENT_PARAMETERS_BYTES_LENGTH                4

#define GDP_SET_VALUE_FLAG_LENGTH                                              1

void emAfRf4ceGdpSetPushButtonPendingReceivedFlag(bool set);
void emAfRf4ceGdpSetProxyBindingFlag(bool set);

// Starts the blackout timer, turns the radio off and sets the internal state
// the the passed state.
void emAfGdpStartBlackoutTimer(uint8_t state);

// Starts the command pending timer, turns the radio on and sets the internal
// state the the passed state.
void emAfGdpStartCommandPendingTimer(uint8_t state, uint16_t timeMs);

extern uint8_t emAfRf4ceGdpOutgoingCommandFrameControl;
#define emAfRf4ceGdpOutgoingCommandsSetPendingFlag()                           \
  (emAfRf4ceGdpOutgoingCommandFrameControl |= GDP_HEADER_FRAME_CONTROL_DATA_PENDING_MASK)
#define emAfRf4ceGdpOutgoingCommandsClearPendingFlag()                         \
  (emAfRf4ceGdpOutgoingCommandFrameControl &= ~GDP_HEADER_FRAME_CONTROL_DATA_PENDING_MASK)

#if defined(EMBER_SCRIPTED_TEST)
#include "stack/core/ember-stack.h"
#include "core/scripted-stub.h"

#define debugDiscoveryResponseDrop(reason)                                     \
  simpleScriptCheck("discoveryResponseDrop", "discoveryResponseDrop: " reason, "")

#define debugCandidateAdded(reason)                                            \
  simpleScriptCheck("candidateAdded", "candidateAdded: " reason, "")

#define debugScriptCheck(reason)                                               \
  simpleScriptCheck("scriptCheck", "scriptCheck: " reason, "")

void setBindOriginatorState(uint8_t state);
#else
#define debugDiscoveryResponseDrop(reason)
#define debugCandidateAdded(reason)
#define debugScriptCheck(reason)
#endif // EMBER_SCRIPTED_TEST
