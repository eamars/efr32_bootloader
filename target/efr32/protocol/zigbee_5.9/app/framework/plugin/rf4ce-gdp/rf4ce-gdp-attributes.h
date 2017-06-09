// Copyright 2014 Silicon Laboratories, Inc.


#ifndef __RF4CE_GDP_ATTRIBUTES_H__
#define __RF4CE_GDP_ATTRIBUTES_H__

// Defines the maximum number of attributes records contained in any
// attribute-related command.
#define MAX_COMMAND_ATTRIBUTES                                    5

// These are specified in Table 9.
#define APL_GDP_VERSION_DEFAULT                                   0x0200
#define APL_GDP_KEY_EXCHANGE_TRANSFER_COUNT_DEFAULT               3
#define APL_GDP_POWER_STATUS_DEFAULT                              0
#define APL_GDP_MAX_PAIRING_CANDIDATES_DEFAULT                    3
#define APL_GDP_AUTO_CHECK_VALIDATION_PERIOD_DEFAULT              500   // msec
#define APL_GDP_LINK_LOST_WAIT_TIME_DEFAULT                       5000  // msec
#define APL_GDP_IDENTIFICATION_CAPABILITIES_DEFAULT               0x00
#define APL_GDP_BINDING_RECIPIENT_VALIDATION_WAIT_TIME_DEFAULT    15000 // msec

// If the Binding Originator does not support extended validation (see section
// 7.2.7, this value shall be set to aplcMaxNormalValidationDuration. If the
// Binding Originator supports extended validation,
// aplBindingOriginatorValidationWaitTime shall be set to
// aplcMaxExtendedValidationDuration.
#if defined(EMBER_AF_PLUGIN_RF4CE_GDP_EXTENDED_VALIDATION)
#define APL_GDP_BINDING_ORIGINATOR_VALIDATION_WAIT_TIME_DEFAULT                \
  APLC_MAX_EXTENDED_VALIDATION_DURATION_MS
#else
#define APL_GDP_BINDING_ORIGINATOR_VALIDATION_WAIT_TIME_DEFAULT                \
  APLC_MAX_NORMAL_VALIDATION_DURATION_MS
#endif

#define APL_GDP_VERSION_SIZE                                      2
#define APL_GDP_CAPABILITIES_SIZE                                 4
#define APL_GDP_KEY_EXCHANGE_TRANSFER_COUNT_SIZE                  1
#define APL_GDP_POWER_STATUS_SIZE                                 1
// See section 6.2.5 (for now we have only one record since there is only one
// polling method defined).
// number of polling methods supports (1 byte)
// + polling constraint record (13 bytes)
#define APL_GDP_POLL_CONSTRAINTS_SIZE                             14
#define APL_GDP_POLL_CONFIGURATION_SIZE                           9
#define APL_GDP_MAX_PAIRING_CANDIDATES_SIZE                       1
#define APL_GDP_AUTO_CHECK_VALIDATION_PERIOD_SIZE                 2
#define APL_GDP_BINDING_RECIPIENT_VALIDATION_WAIT_TIME_SIZE       2
#define APL_GDP_BINDING_ORIGINATOR_VALIDATION_WAIT_TIME_SIZE      2
#define APL_GDP_LINK_LOST_WAIT_TIME_SIZE                          2
#define APL_GDP_IDENTIFICATION_CAPABILITIES_SIZE                  1

// Make sure to keep these updated.
#define MAX_GDP_ATTRIBUTE_SIZE                                    13

#if defined(EMBER_SCRIPTED_TEST)
#define GDP_ATTRIBUTES_COUNT        12 + 4
#else
#define GDP_ATTRIBUTES_COUNT        12
#endif

#define MIN_GDP_ATTRIBUTE_ID                                      0x80
#define MAX_GDP_ATTRIBUTE_ID                                      0x8B

#define POLL_CONSTRAINT_RECORD_NUMBER_OFFSET                      0
#define POLL_CONSTRAINT_RECORD_NUMBER_LENGTH                      1
#define POLL_CONSTRAINT_RECORD_OFFSET                             1
#define POLL_CONSTRAINT_RECORD_LENGTH                             13

// Poll constraint record related macros
#define POLL_CONSTRAINT_RECORD_POLLING_METHOD_ID_OFFSET                     (0 + POLL_CONSTRAINT_RECORD_OFFSET)
#define POLL_CONSTRAINT_RECORD_POLLING_METHOD_ID_LENGTH                     1
#define POLL_CONSTRAINT_RECORD_POLLING_TRIGGERS_CAPABILITIES_OFFSET         (1 + POLL_CONSTRAINT_RECORD_OFFSET)
#define POLL_CONSTRAINT_RECORD_POLLING_TRIGGERS_CAPABILITIES_LENGTH         2
#define POLL_CONSTRAINT_RECORD_MIN_POLLING_KEY_PRESS_COUNT_OFFSET           (3 + POLL_CONSTRAINT_RECORD_OFFSET)
#define POLL_CONSTRAINT_RECORD_MIN_POLLING_KEY_PRESS_COUNT_LENGTH           1
#define POLL_CONSTRAINT_RECORD_MAX_POLLING_KEY_PRESS_COUNT_OFFSET           (4 + POLL_CONSTRAINT_RECORD_OFFSET)
#define POLL_CONSTRAINT_RECORD_MAX_POLLING_KEY_PRESS_COUNT_LENGTH           1
#define POLL_CONSTRAINT_RECORD_MIN_POLLING_TIME_INTERVAL_OFFSET             (5 + POLL_CONSTRAINT_RECORD_OFFSET)
#define POLL_CONSTRAINT_RECORD_MIN_POLLING_TIME_INTERVAL_LENGTH             4
#define POLL_CONSTRAINT_RECORD_MAX_POLLING_TIME_INTERVAL_OFFSET             (9 + POLL_CONSTRAINT_RECORD_OFFSET)
#define POLL_CONSTRAINT_RECORD_MAX_POLLING_TIME_INTERVAL_LENGTH             4

#define POLL_CONSTRAINT_RECORD_MIN_POLLING_TIME_INTERVAL_LOWER_BOUND        50
#define POLL_CONSTRAINT_RECORD_MIN_POLLING_TIME_INTERVAL_UPPER_BOUND        3600000
#define POLL_CONSTRAINT_RECORD_MAX_POLLING_TIME_INTERVAL_LOWER_BOUND        60000
#define POLL_CONSTRAINT_RECORD_MAX_POLLING_TIME_INTERVAL_UPPER_BOUND        86400000

// Poll configuration attribute related macros
#define POLL_CONFIGURATION_POLLING_METHOD_ID_OFFSET                         0
#define POLL_CONFIGURATION_POLLING_METHOD_ID_LENGTH                         1
#define POLL_CONFIGURATION_POLLING_TRIGGER_CONFIG_OFFSET                    1
#define POLL_CONFIGURATION_POLLING_TRIGGER_CONFIG_LENGTH                    2
#define POLL_CONFIGURATION_POLLING_KEY_PRESS_COUNTER_OFFSET                 3
#define POLL_CONFIGURATION_POLLING_KEY_PRESS_COUNTER_LENGTH                 1
#define POLL_CONFIGURATION_POLLING_TIME_INTERVAL_OFFSET                     4
#define POLL_CONFIGURATION_POLLING_TIME_INTERVAL_LENGTH                     4
#define POLL_CONFIGURATION_POLLING_TIMEOUT_OFFSET                           8
#define POLL_CONFIGURATION_POLLING_TIMEOUT_LENGTH                           1

// TODO: poll triggers and min/max values should be set in AppBuilder (only if
// the node is a poll client, this values are meaningless for a poll server).

#define MIN_POLLING_KEY_PRESS_COUNTER     1
#define MAX_POLLING_KEY_PRESS_COUNTER     255
#define SUPPORTED_TRIGGERS                (EMBER_AF_RF4CE_GDP_POLLING_TRIGGER_TIME_BASED_POLLING_ENABLED      \
                                           | EMBER_AF_RF4CE_GDP_POLLING_TRIGGER_POLLING_ON_KEY_PRESS_ENABLED)

// By default we set the poll client to support the heartbeat method and to
// support time-based and key press triggers.
#define APL_POLL_CONSTRAINTS_DEFAULT      {0x01, /*number of methods supported*/\
                                           EMBER_AF_RF4CE_GDP_POLLING_METHOD_HEARTBEAT, \
                                           LOW_BYTE(SUPPORTED_TRIGGERS),       \
                                           HIGH_BYTE(SUPPORTED_TRIGGERS),      \
                                           MIN_POLLING_KEY_PRESS_COUNTER,      \
                                           MAX_POLLING_KEY_PRESS_COUNTER,      \
                                           BYTE_0(EMBER_AF_PLUGIN_RF4CE_GDP_MIN_POLLING_INTERVAL_MS),  \
                                           BYTE_1(EMBER_AF_PLUGIN_RF4CE_GDP_MIN_POLLING_INTERVAL_MS),  \
                                           BYTE_2(EMBER_AF_PLUGIN_RF4CE_GDP_MIN_POLLING_INTERVAL_MS),  \
                                           BYTE_3(EMBER_AF_PLUGIN_RF4CE_GDP_MIN_POLLING_INTERVAL_MS),  \
                                           BYTE_0(EMBER_AF_PLUGIN_RF4CE_GDP_MAX_POLLING_INTERVAL_MS),  \
                                           BYTE_1(EMBER_AF_PLUGIN_RF4CE_GDP_MAX_POLLING_INTERVAL_MS),  \
                                           BYTE_2(EMBER_AF_PLUGIN_RF4CE_GDP_MAX_POLLING_INTERVAL_MS),  \
                                           BYTE_3(EMBER_AF_PLUGIN_RF4CE_GDP_MAX_POLLING_INTERVAL_MS)}

// Identification capabilities attribute related macros
#define IDENTIFICATION_CAPABILITIES_RESERVED_MASK                           0xF1
#define IDENTIFICATION_CAPABILITIES_SUPPORT_FLASH_LIGHT_BIT                 0x02
#define IDENTIFICATION_CAPABILITIES_SUPPORT_MAKE_SOUND_BIT                  0x04
#define IDENTIFICATION_CAPABILITIES_SUPPORT_VIBRATE_BIT                     0x08

// We maintain an info table for each GDP attribute defined in Table 9.
typedef struct {
  uint8_t id;
  uint8_t size;
  uint8_t bitmask;
  uint16_t dimension;  // only for arrayed attributes
} EmAfRf4ceGdpAttributeDescriptor;

// Bitmask field definitions.
#define ATTRIBUTE_HAS_REMOTE_GET_ACCESS_BIT                       0x01
#define ATTRIBUTE_HAS_REMOTE_SET_ACCESS_BIT                       0x02
#define ATTRIBUTE_HAS_REMOTE_PUSH_ACCESS_BIT                      0x04
#define ATTRIBUTE_HAS_REMOTE_PULL_ACCESS_BIT                      0x08
#define ATTRIBUTE_IS_TWO_DIMENSIONAL_ARRAYED                      0x10

// The following attributes are not currently included in the attribute struct,
// they have no remote access and we treat them as constants, some of them
// configurable from the plugin options.
// - aplKeyExchangeTransferCount
// - aplMaxPairingCandidates
// - aplBindingRecipientValidationWaitTime
// - aplBindingOriginatorValidationWaitTime

// Array attributes are in the range 0x90--0x9F for GDP and 0xC0--0xDF for the
// other profiles.  This means that all array attributes have bit 7 set; bit 5
// clear; and bits 4 or 6 or both set.  Note that this macro is just a tad not
// safe because it uses the parameter attributeId twice.
#define IS_ARRAY_ATTRIBUTE(attributeId)                 \
  (READBITS((attributeId), BIT(7) | BIT(5)) == BIT(7)   \
   && READBITS((attributeId), BIT(4) | BIT(6)) != 0x00)

// Test related stuff
#define GDP_ATTRIBUTE_ONE_DIMENSIONAL_ARRAY_TEST_SIZE                 2
#define GDP_ATTRIBUTE_ONE_DIMENSIONAL_ARRAY_TEST_DIMENSION            3
#define GDP_ATTRIBUTE_TWO_DIMENSIONAL_ARRAY_TEST_SIZE                 4
#define GDP_ATTRIBUTE_TWO_DIMENSIONAL_ARRAY_TEST_FIRST_DIMENSION      4
#define GDP_ATTRIBUTE_TWO_DIMENSIONAL_ARRAY_TEST_SECOND_DIMENSION     5

typedef struct {
  uint16_t gdpVersion;
  uint32_t gdpCapabilities;
  uint8_t powerStatus;
  uint8_t pollConstraints[APL_GDP_POLL_CONSTRAINTS_SIZE];
  uint8_t pollConfiguration[APL_GDP_POLL_CONFIGURATION_SIZE];
  uint16_t autoCheckValidationPeriod;
  uint16_t linkLostWaitTime;
  uint8_t identificationCapabilities;

  // Test arrayed attributes
#if defined(EMBER_SCRIPTED_TEST)
  uint16_t settableScalarTest1;
  uint16_t settableScalarTest2;
  uint16_t oneDimensionalTestAttribute[GDP_ATTRIBUTE_ONE_DIMENSIONAL_ARRAY_TEST_DIMENSION];
  uint32_t twoDimensionalTestAttribute[GDP_ATTRIBUTE_TWO_DIMENSIONAL_ARRAY_TEST_FIRST_DIMENSION][GDP_ATTRIBUTE_TWO_DIMENSIONAL_ARRAY_TEST_SECOND_DIMENSION];
#endif
} EmAfRf4ceGdpAttributes;

extern EmAfRf4ceGdpAttributes emAfRf4ceGdpLocalNodeAttributes;
extern EmAfRf4ceGdpAttributes emAfRf4ceGdpRemoteNodeAttributes;

#define WRITE_ACCESS  true
#define READ_ACCESS   false

void emAfRf4ceGdpClearRemoteAttributes(void);

void emAfRf4ceGdpGetOrSetAttribute(EmAfRf4ceGdpAttributes *attributes,
                                   uint8_t attrId,
                                   uint16_t entryId,
                                   bool isGet,
                                   uint8_t *val);

#define emAfRf4ceGdpSetLocalAttribute(attrId, entryId, val)                    \
    emAfRf4ceGdpGetOrSetAttribute(&emAfRf4ceGdpLocalNodeAttributes,            \
                                  (attrId),                                    \
                                  (entryId),                                   \
                                  false,                                       \
                                  (uint8_t*)(val))

#define emAfRf4ceGdpGetLocalAttribute(attrId, entryId, val)                    \
    emAfRf4ceGdpGetOrSetAttribute(&emAfRf4ceGdpLocalNodeAttributes,            \
                                  (attrId),                                    \
                                  (entryId),                                   \
                                  true,                                        \
                                  (uint8_t*)(val))

#define emAfRf4ceGdpSetRemoteAttribute(attrId, entryId, val)                   \
    emAfRf4ceGdpGetOrSetAttribute(&emAfRf4ceGdpRemoteNodeAttributes,           \
                                  (attrId),                                    \
                                  (entryId),                                   \
                                  false,                                       \
                                  (uint8_t*)(val))

#define emAfRf4ceGdpGetRemoteAttribute(attrId, entryId, val)                   \
    emAfRf4ceGdpGetOrSetAttribute(&emAfRf4ceGdpRemoteNodeAttributes,           \
                                  (attrId),                                    \
                                  (entryId),                                   \
                                  true,                                        \
                                  (uint8_t*)(val))

#endif //__RF4CE_GDP_ATTRIBUTES_H__
