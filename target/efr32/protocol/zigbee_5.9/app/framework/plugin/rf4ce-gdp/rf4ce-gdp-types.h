// Copyright 2014 Silicon Laboratories, Inc.

#ifndef __RF4CE_GDP_TYPES_H__
#define __RF4CE_GDP_TYPES_H__

/**
 * @brief RF4CE GDP command codes.
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberAfRf4ceGdpCommandCode
#else
typedef uint8_t EmberAfRf4ceGdpCommandCode;
enum
#endif
{
  EMBER_AF_RF4CE_GDP_COMMAND_GENERIC_RESPONSE         = 0x00,
  EMBER_AF_RF4CE_GDP_COMMAND_CONFIGURATION_COMPLETE   = 0x01,
  EMBER_AF_RF4CE_GDP_COMMAND_HEARTBEAT                = 0x02,
  EMBER_AF_RF4CE_GDP_COMMAND_GET_ATTRIBUTES           = 0x03,
  EMBER_AF_RF4CE_GDP_COMMAND_GET_ATTRIBUTES_RESPONSE  = 0x04,
  EMBER_AF_RF4CE_GDP_COMMAND_PUSH_ATTRIBUTES          = 0x05,
  EMBER_AF_RF4CE_GDP_COMMAND_SET_ATTRIBUTES           = 0x06,
  EMBER_AF_RF4CE_GDP_COMMAND_PULL_ATTRIBUTES          = 0x07,
  EMBER_AF_RF4CE_GDP_COMMAND_PULL_ATTRIBUTES_RESPONSE = 0x08,
  EMBER_AF_RF4CE_GDP_COMMAND_CHECK_VALIDATION         = 0x09,
  EMBER_AF_RF4CE_GDP_COMMAND_CLIENT_NOTIFICATION      = 0x0A,
  EMBER_AF_RF4CE_GDP_COMMAND_KEY_EXCHANGE             = 0x0B,
};

/**
 * @brief RF4CE GDP response codes.
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberAfRf4ceGdpResponseCode
#else
typedef uint8_t EmberAfRf4ceGdpResponseCode;
enum
#endif
{
  EMBER_AF_RF4CE_GDP_RESPONSE_CODE_SUCCESSFUL            = 0x00,
  EMBER_AF_RF4CE_GDP_RESPONSE_CODE_UNSUPPORTED_REQUEST   = 0x01,
  EMBER_AF_RF4CE_GDP_RESPONSE_CODE_INVALID_PARAMETER     = 0x02,
  EMBER_AF_RF4CE_GDP_RESPONSE_CODE_CONFIGURATION_FAILURE = 0x03,
};

/**
 * @brief RF4CE GDP statuses.
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberAfRf4ceGdpStatus
#else
typedef uint8_t EmberAfRf4ceGdpStatus;
enum
#endif
{
  EMBER_AF_RF4CE_GDP_STATUS_SUCCESSFUL            = 0x00,
  EMBER_AF_RF4CE_GDP_STATUS_CONFIGURATION_FAILURE = 0x03,
};

/**
 * @brief RF4CE GDP heartbeat triggers.
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberAfRf4ceGdpHeartbeatTrigger
#else
typedef uint8_t EmberAfRf4ceGdpHeartbeatTrigger;
enum
#endif
{
  EMBER_AF_RF4CE_GDP_HEARTBEAT_TRIGGER_GENERIC_ACTIVITY               = 0x00,
  EMBER_AF_RF4CE_GDP_HEARTBEAT_TRIGGER_TIME_BASED_POLLING             = 0x01,
  EMBER_AF_RF4CE_GDP_HEARTBEAT_TRIGGER_POLLING_ON_KEY_PRESS           = 0x02,
  EMBER_AF_RF4CE_GDP_HEARTBEAT_TRIGGER_POLLING_ON_PICKUP              = 0x03,
  EMBER_AF_RF4CE_GDP_HEARTBEAT_TRIGGER_POLLING_ON_RESET               = 0x04,
  EMBER_AF_RF4CE_GDP_HEARTBEAT_TRIGGER_POLLING_ON_MICROPHONE_ACTIVITY = 0x05,
  EMBER_AF_RF4CE_GDP_HEARTBEAT_TRIGGER_POLLING_ON_OTHER_USER_ACTIVITY = 0x06,
};

/**
 * @brief RF4CE GDP heartbeat callback. Any module can subscribe to incoming
 * heartbeat commands by using the ::emberAfRf4ceGdpSubscribeToHeartbeat()
 * API. The first parameter is the pairing index, the second parameter is the
 * heartbeat trigger.
 */
typedef void (*EmberAfRf4ceGdpHeartbeatCallback)(uint8_t, EmberAfRf4ceGdpHeartbeatTrigger);

/**
 * @brief RF4CE GDP attribute ids.
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberAfRf4ceGdpAttributeId
#else
typedef uint8_t EmberAfRf4ceGdpAttributeId;
enum
#endif
{
  EMBER_AF_RF4CE_GDP_ATTRIBUTE_VERSION                                = 0x80,
  EMBER_AF_RF4CE_GDP_ATTRIBUTE_CAPABILITIES                           = 0x81,
  EMBER_AF_RF4CE_GDP_ATTRIBUTE_KEY_EXCHANGE_TRANSFER_COUNT            = 0x82,
  EMBER_AF_RF4CE_GDP_ATTRIBUTE_POWER_STATUS                           = 0x83,
  EMBER_AF_RF4CE_GDP_ATTRIBUTE_POLL_CONSTRAINTS                       = 0x84,
  EMBER_AF_RF4CE_GDP_ATTRIBUTE_POLL_CONFIGURATION                     = 0x85,
  EMBER_AF_RF4CE_GDP_ATTRIBUTE_MAX_PAIRING_CANDIDATES                 = 0x86,
  EMBER_AF_RF4CE_GDP_ATTRIBUTE_AUTO_CHECK_VALIDATION_PERIOD           = 0x87,
  EMBER_AF_RF4CE_GDP_ATTRIBUTE_BINDING_RECIPIENT_VALIDATION_WAIT_TIME = 0x88,
  EMBER_AF_RF4CE_GDP_ATTRIBUTE_BINDING_INITIATOR_VALIDATION_WAIT_TIME = 0x89,
  EMBER_AF_RF4CE_GDP_ATTRIBUTE_LINK_LOST_WAIT_TIME                    = 0x8A,
  EMBER_AF_RF4CE_GDP_ATTRIBUTE_IDENTIFICATION_CAPABILITIES            = 0x8B,
};

/**
 * @brief RF4CE GDP attribute statuses.
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberAfRf4ceGdpAttributeStatus
#else
typedef uint8_t EmberAfRf4ceGdpAttributeStatus;
enum
#endif
{
  EMBER_AF_RF4CE_GDP_ATTRIBUTE_STATUS_SUCCESS               = 0x00,
  EMBER_AF_RF4CE_GDP_ATTRIBUTE_STATUS_UNSUPPORTED_ATTRIBUTE = 0x01,
  EMBER_AF_RF4CE_GDP_ATTRIBUTE_STATUS_ILLEGAL_REQUEST       = 0x02,
  EMBER_AF_RF4CE_GDP_ATTRIBUTE_STATUS_INVALID_ENTRY         = 0x03,

  // Non over-the-air internal values.
  EMBER_AF_RF4CE_GDP_ATTRIBUTE_STATUS_NO_RESPONSE           = 0xF0,
  EMBER_AF_RF4CE_GDP_ATTRIBUTE_STATUS_INVALID_RESPONSE      = 0xF1,
};

/**
 * @brief RF4CE GDP attribute identification record for Get Attributes and Pull
 * Attributes messages.
 */
typedef struct {
  EmberAfRf4ceGdpAttributeId attributeId;
  uint16_t entryId;
} EmberAfRf4ceGdpAttributeIdentificationRecord;

/**
 * @brief RF4CE GDP attribute identification record for Get Attributes Response
 * and Pull Attributes Response messages.
 */
typedef struct {
  EmberAfRf4ceGdpAttributeId attributeId;
  uint16_t entryId;
  EmberAfRf4ceGdpAttributeStatus status;
  uint8_t valueLength;
  const uint8_t *value;
} EmberAfRf4ceGdpAttributeStatusRecord;

/**
 * @brief RF4CE GDP attribute identification record for Set Attributes and Push
 * Attributes messages.
 */
typedef struct {
  EmberAfRf4ceGdpAttributeId attributeId;
  uint16_t entryId;
  uint8_t valueLength;
  const uint8_t *value;
} EmberAfRf4ceGdpAttributeRecord;

/**
 * @brief RF4CE GDP Check Validation subtypes.
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberAfRf4ceGdpCheckValidationSubtype
#else
typedef uint8_t EmberAfRf4ceGdpCheckValidationSubtype;
enum
#endif
{
  EMBER_AF_RF4CE_GDP_CHECK_VALIDATION_SUBTYPE_REQUEST  = 0x00,
  EMBER_AF_RF4CE_GDP_CHECK_VALIDATION_SUBTYPE_RESPONSE = 0x01,
};

/**
 * @brief RF4CE GDP Check Validation statuses.
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberAfRf4ceGdpCheckValidationStatus
#else
typedef uint8_t EmberAfRf4ceGdpCheckValidationStatus;
enum
#endif
{
  EMBER_AF_RF4CE_GDP_CHECK_VALIDATION_STATUS_SUCCESS = 0x00,
  EMBER_AF_RF4CE_GDP_CHECK_VALIDATION_STATUS_PENDING = 0x01,
  EMBER_AF_RF4CE_GDP_CHECK_VALIDATION_STATUS_TIMEOUT = 0x02,
  EMBER_AF_RF4CE_GDP_CHECK_VALIDATION_STATUS_FAILURE = 0x03,
};

/**
 * @brief RF4CE GDP Client Notification subtypes.
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberAfRf4ceGdpClientNotificationSubtype
#else
typedef uint8_t EmberAfRf4ceGdpClientNotificationSubtype;
enum
#endif
{
  EMBER_AF_RF4CE_GDP_CLIENT_NOTIFICATION_SUBTYPE_IDENTIFY                 = 0x00,
  EMBER_AF_RF4CE_GDP_CLIENT_NOTIFICATION_SUBTYPE_REQUEST_POLL_NEGOTIATION = 0x01,
};

/**
 * @brief RF4CE GDP Client Notification Identify flags.
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberAfRf4ceGdpClientNotificationIdentifyFlags
#else
typedef uint8_t EmberAfRf4ceGdpClientNotificationIdentifyFlags;
enum
#endif
{
  EMBER_AF_RF4CE_GDP_CLIENT_NOTIFICATION_IDENTIFY_FLAG_STOP_ON_ACTION = 0x01,
  EMBER_AF_RF4CE_GDP_CLIENT_NOTIFICATION_IDENTIFY_FLAG_FLASH_LIGHT    = 0x02,
  EMBER_AF_RF4CE_GDP_CLIENT_NOTIFICATION_IDENTIFY_FLAG_MAKE_SOUND     = 0x04,
  EMBER_AF_RF4CE_GDP_CLIENT_NOTIFICATION_IDENTIFY_FLAG_VIBRATE        = 0x08,
};

/**
 * @brief RF4CE GDP Key Exchange subtypes.
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberAfRf4ceGdpKeyExchangeSubtype
#else
typedef uint8_t EmberAfRf4ceGdpKeyExchangeSubtype;
enum
#endif
{
  EMBER_AF_RF4CE_GDP_KEY_EXCHANGE_SUBTYPE_CHALLENGE          = 0x00,
  EMBER_AF_RF4CE_GDP_KEY_EXCHANGE_SUBTYPE_CHALLENGE_RESPONSE = 0x01,
  EMBER_AF_RF4CE_GDP_KEY_EXCHANGE_SUBTYPE_RESPONSE           = 0x02,
  EMBER_AF_RF4CE_GDP_KEY_EXCHANGE_SUBTYPE_CONFIRM            = 0x03,
};

/**
 * @brief RF4CE GDP Key Exchange flags.
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberAfRf4ceGdpKeyExchangeFlags
#else
typedef uint16_t EmberAfRf4ceGdpKeyExchangeFlags;
enum
#endif
{
  EMBER_AF_RF4CE_GDP_KEY_EXCHANGE_FLAG_STANDARD_SHARED_SECRET           = 0x0001,
  EMBER_AF_RF4CE_GDP_KEY_EXCHANGE_FLAG_INITIATOR_VENDOR_SPECIFIC_SECRET = 0x0002,
  EMBER_AF_RF4CE_GDP_KEY_EXCHANGE_FLAG_RESPONDER_VENDOR_SPECIFIC_SECRET = 0x0004,
  EMBER_AF_RF4CE_GDP_KEY_EXCHANGE_FLAG_VENDOR_SPECIFIC_PARAMETER_MASK   = 0xFF00,
  EMBER_AF_RF4CE_GDP_KEY_EXCHANGE_FLAG_VENDOR_SPECIFIC_PARAMETER_OFFSET = 8,
};

/**
 * @brief Size of the GDP random byte string in bytes (8).
 */
#define EMBER_AF_RF4CE_GDP_RAND_SIZE 8

/**
 * @brief This data structure contains the GDP random byte string that is
 * passed into various other functions.
 */
typedef struct {
  /** This is the random byte string. */
  uint8_t contents[EMBER_AF_RF4CE_GDP_RAND_SIZE];
} EmberAfRf4ceGdpRand;

/**
 * @brief Size of the GDP tag value in in bytes (4).
 */
#define EMBER_AF_RF4CE_GDP_TAG_SIZE 4

/**
 * @brief This data structure contains the GDP tag value that is passed into
 * various other functions.
 */
typedef struct {
  /** This is the tag value. */
  uint8_t contents[EMBER_AF_RF4CE_GDP_TAG_SIZE];
} EmberAfRf4ceGdpTag;

/**
 * @brief RF4CE GDP binding states.
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberAfRf4ceGdpBindingState
#else
typedef uint8_t EmberAfRf4ceGdpBindingState;
enum
#endif
{
  EMBER_AF_RF4CE_GDP_BINDING_STATE_DORMANT   = 0,
  EMBER_AF_RF4CE_GDP_BINDING_STATE_NOT_BOUND = 1,
  EMBER_AF_RF4CE_GDP_BINDING_STATE_BINDING   = 2,
  EMBER_AF_RF4CE_GDP_BINDING_STATE_BOUND     = 3,
};

/**
 * @brief RF4CE GDP binding statuses.
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberAfRf4ceGdpBindingStatus
#else
typedef uint8_t EmberAfRf4ceGdpBindingStatus;
enum
#endif
{
  EMBER_AF_RF4CE_GDP_BINDING_STATUS_SUCCESS                           = 0x00,
  EMBER_AF_RF4CE_GDP_BINDING_STATUS_DUPLICATE_CLASS_ABORT             = 0x01,
  EMBER_AF_RF4CE_GDP_BINDING_STATUS_NO_VALID_RESPONSE                 = 0x02,
  EMBER_AF_RF4CE_GDP_BINDING_STATUS_PAIRING_FAILED                    = 0x03,
  EMBER_AF_RF4CE_GDP_BINDING_STATUS_CONFIG_FAILED                     = 0x04,
  EMBER_AF_RF4CE_GDP_BINDING_STATUS_PROFILE_SPECIFIC_CONFIG_FAILED    = 0x05,
  EMBER_AF_RF4CE_GDP_BINDING_STATUS_VALIDATION_FAILED                 = 0x06,
};

/**
 * @brief RF4CE GDP polling triggers.
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberAfRf4ceGdpPollingTrigger
#else
typedef uint16_t EmberAfRf4ceGdpPollingTrigger;
enum
#endif
{
  EMBER_AF_RF4CE_GDP_POLLING_TRIGGER_TIME_BASED_POLLING_ENABLED             = 0x0001,
  EMBER_AF_RF4CE_GDP_POLLING_TRIGGER_POLLING_ON_KEY_PRESS_ENABLED           = 0x0002,
  EMBER_AF_RF4CE_GDP_POLLING_TRIGGER_POLLING_ON_PICK_UP_ENABLED             = 0x0004,
  EMBER_AF_RF4CE_GDP_POLLING_TRIGGER_POLLING_ON_RESET_ENABLED               = 0x0008,
  EMBER_AF_RF4CE_GDP_POLLING_TRIGGER_POLLING_ON_MICROPHONE_ACTIVITY_ENABLED = 0x0010,
  EMBER_AF_RF4CE_GDP_POLLING_TRIGGER_POLLING_ON_OTHER_USER_ACTIVITY_ENABLED = 0x0020,
};

/**
 * @brief RF4CE GDP polling methods.
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberAfRf4ceGdpPollingMethod
#else
typedef uint8_t EmberAfRf4ceGdpPollingMethod;
enum
#endif
{
  EMBER_AF_RF4CE_GDP_POLLING_METHOD_DISABLED  = 0x00,
  EMBER_AF_RF4CE_GDP_POLLING_METHOD_HEARTBEAT = 0x01,
};

#endif // __RF4CE_GDP_TYPES_H__
