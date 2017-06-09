// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"

#if defined(EMBER_SCRIPTED_TEST)
#include "rf4ce-gdp-test.h"
#endif

#include "app/framework/plugin/rf4ce-profile/rf4ce-profile.h"
#include "rf4ce-gdp.h"
#include "rf4ce-gdp-identification.h"
#include "rf4ce-gdp-poll.h"
#include "rf4ce-gdp-internal.h"

uint8_t emAfRf4ceGdpOutgoingCommandFrameControl =
    GDP_HEADER_FRAME_CONTROL_GDP_COMMAND_FRAME_MASK;

static PGM uint8_t minimumMessageLengths[] = {
  GENERIC_RESPONSE_LENGTH,
  CONFIGURATION_COMPLETE_LENGTH,
  HEARTBEAT_LENGTH,
  GET_ATTRIBUTES_LENGTH,
  GET_ATTRIBUTES_RESPONSE_LENGTH,
  PUSH_ATTRIBUTES_LENGTH,
  SET_ATTRIBUTES_LENGTH,
  PULL_ATTRIBUTES_LENGTH,
  PULL_ATTRIBUTES_RESPONSE_LENGTH,
  CHECK_VALIDATION_LENGTH,
  CLIENT_NOTIFICATION_LENGTH,
  KEY_EXCHANGE_LENGTH,
};

typedef uint8_t Decision;
enum {
  ACCEPT, // return true
  IGNORE, // return false
  DROP,   // return true
};

#define COMMAND_CODE(message)                    \
  (message[GDP_HEADER_FRAME_CONTROL_OFFSET]      \
   & GDP_HEADER_FRAME_CONTROL_COMMAND_CODE_MASK)

#define GDP_COMMAND_FRAME(message)                    \
  (message[GDP_HEADER_FRAME_CONTROL_OFFSET]           \
   & GDP_HEADER_FRAME_CONTROL_GDP_COMMAND_FRAME_MASK)

#define DATA_PENDING(message)                    \
  (message[GDP_HEADER_FRAME_CONTROL_OFFSET]      \
   & GDP_HEADER_FRAME_CONTROL_DATA_PENDING_MASK)

// Array attributes are in the range 0x90--0x9F for GDP and 0xC0--0xDF for the
// other profiles.  This means that all array attributes have bit 7 set; bit 5
// clear; and bits 4 or 6 or both set.  Note that this macro is just a tad not
// safe because it uses the parameter attributeId twice.
#define IS_ARRAY_ATTRIBUTE(attributeId)                 \
  (READBITS((attributeId), BIT(7) | BIT(5)) == BIT(7)   \
   && READBITS((attributeId), BIT(4) | BIT(6)) != 0x00)

static Decision checkCommand(uint8_t pairingIndex,
                             uint8_t profileId,
                             uint16_t vendorId,
                             const uint8_t *message,
                             uint8_t messageLength,
                             bool rx);
static EmberStatus send(uint8_t pairingIndex, uint8_t profileId, uint16_t vendorId);
static EmberStatus getPullAttributes(EmberAfRf4ceGdpCommandCode commandCode,
                                     uint8_t pairingIndex,
                                     uint8_t profileId,
                                     uint16_t vendorId,
                                     const EmberAfRf4ceGdpAttributeIdentificationRecord *records,
                                     uint8_t recordsLength);
static EmberStatus getPullAttributesResponse(EmberAfRf4ceGdpCommandCode commandCode,
                                             uint8_t pairingIndex,
                                             uint8_t profileId,
                                             uint16_t vendorId,
                                             const EmberAfRf4ceGdpAttributeStatusRecord *records,
                                             uint8_t recordsLength);
static EmberStatus setPushAttributes(EmberAfRf4ceGdpCommandCode commandCode,
                                     uint8_t pairingIndex,
                                     uint8_t profileId,
                                     uint16_t vendorId,
                                     const EmberAfRf4ceGdpAttributeRecord *records,
                                     uint8_t recordsLength);

static uint8_t buffer[EMBER_AF_RF4CE_MAXIMUM_RF4CE_PAYLOAD_LENGTH];
static uint8_t bufferLength = 0;

static const uint8_t *incoming = NULL;
static uint8_t incomingLength = 0;
static uint8_t incomingOffset = 0xFF;

bool emberAfPluginRf4ceProfileGdpMessageSentCallback(uint8_t pairingIndex,
                                                        uint8_t profileId,
                                                        uint16_t vendorId,
                                                        uint8_t messageTag,
                                                        const uint8_t *message,
                                                        uint8_t messageLength,
                                                        EmberStatus status)
{
  // Attempt to process a potential GDP command only if the profile ID is a
  // GDP 2.0 based profile ID.
  return ((profileId == EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE
           || emAfIsProfileGdpBased(profileId, GDP_VERSION_2_0))
          && emberAfRf4ceGdpMessageSent(pairingIndex,
                                        profileId,
                                        vendorId,
                                        message,
                                        messageLength,
                                        status));
}

bool emberAfRf4ceGdpMessageSent(uint8_t pairingIndex,
                                   uint8_t profileId,
                                   uint16_t vendorId,
                                   const uint8_t *message,
                                   uint8_t messageLength,
                                   EmberStatus status)
{
  Decision decision = checkCommand(pairingIndex,
                                   profileId,
                                   vendorId,
                                   message,
                                   messageLength,
                                   false); // TX
  if (decision != ACCEPT) {
    return (decision == DROP);
  }

  // TODO: Handle success and failure of outgoing messages.

  if (profileId == EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE) {
    incoming = (uint8_t*)message;
    incomingLength = messageLength;
    incomingOffset = GDP_PAYLOAD_OFFSET;
    switch (COMMAND_CODE(message)) {
    case EMBER_AF_RF4CE_GDP_COMMAND_CHECK_VALIDATION:
      {
        EmberAfRf4ceGdpCheckValidationSubtype subtype = message[CHECK_VALIDATION_SUBTYPE_OFFSET];
        if (subtype == EMBER_AF_RF4CE_GDP_CHECK_VALIDATION_SUBTYPE_RESPONSE) {
          emAfRf4ceGdpCheckValidationResponseSent(status);
        }
        break;
      }
    case EMBER_AF_RF4CE_GDP_COMMAND_KEY_EXCHANGE:
      {
        EmberAfRf4ceGdpKeyExchangeSubtype subtype = message[KEY_EXCHANGE_SUBTYPE_OFFSET];
        if (subtype == EMBER_AF_RF4CE_GDP_KEY_EXCHANGE_SUBTYPE_RESPONSE) {
          emAfRf4ceGdpKeyExchangeResponseSent(status);
        }
        break;
      }
    case EMBER_AF_RF4CE_GDP_COMMAND_HEARTBEAT:
    {
      emAfRf4ceGdpHeartbeatSent(status);
      break;
    }
    }
  }

  return true;
}

bool emberAfPluginRf4ceProfileGdpIncomingMessageCallback(uint8_t pairingIndex,
                                                            uint8_t profileId,
                                                            uint16_t vendorId,
                                                            EmberRf4ceTxOption txOptions,
                                                            const uint8_t *message,
                                                            uint8_t messageLength)
{
  // Attempt to process a potential GDP command only if the profile ID is a
  // GDP 2.0 based profile ID.
  return ((profileId == EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE
           || emAfIsProfileGdpBased(profileId, GDP_VERSION_2_0))
          && emberAfRf4ceGdpIncomingMessage(pairingIndex,
                                            profileId,
                                            vendorId,
                                            txOptions,
                                            message,
                                            messageLength));
}

typedef struct {
  EmberAfRf4ceProfileId profileId;
  EmberAfRf4ceGdpCommandCode commandCode;
  void (*callback)(void);
} Callback;
Callback callbacks[] = {
  {EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE,     EMBER_AF_RF4CE_GDP_COMMAND_GENERIC_RESPONSE,         (void (*)(void))emAfRf4ceGdpIncomingGenericResponse},
  {EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE,     EMBER_AF_RF4CE_GDP_COMMAND_CONFIGURATION_COMPLETE,   (void (*)(void))emAfRf4ceGdpIncomingConfigurationComplete},
  {EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE,     EMBER_AF_RF4CE_GDP_COMMAND_GET_ATTRIBUTES,           (void (*)(void))emAfRf4ceGdpIncomingGetAttributes},
  {EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE,     EMBER_AF_RF4CE_GDP_COMMAND_GET_ATTRIBUTES_RESPONSE,  (void (*)(void))emAfRf4ceGdpIncomingGetAttributesResponse},
  {EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE,     EMBER_AF_RF4CE_GDP_COMMAND_PUSH_ATTRIBUTES,          (void (*)(void))emAfRf4ceGdpIncomingPushAttributes},
  {EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE,     EMBER_AF_RF4CE_GDP_COMMAND_SET_ATTRIBUTES,           (void (*)(void))emAfRf4ceGdpIncomingSetAttributes},
  {EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE,     EMBER_AF_RF4CE_GDP_COMMAND_PULL_ATTRIBUTES,          (void (*)(void))emAfRf4ceGdpIncomingPullAttributes},
  {EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE,     EMBER_AF_RF4CE_GDP_COMMAND_PULL_ATTRIBUTES_RESPONSE, (void (*)(void))emAfRf4ceGdpIncomingPullAttributesResponse},
  {EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE,     EMBER_AF_RF4CE_GDP_COMMAND_CLIENT_NOTIFICATION,      (void (*)(void))emAfRf4ceGdpIncomingClientNotification},
  {EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0, EMBER_AF_RF4CE_GDP_COMMAND_GENERIC_RESPONSE,         (void (*)(void))emAfRf4ceZrcIncomingGenericResponse},
  {EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0, EMBER_AF_RF4CE_GDP_COMMAND_CONFIGURATION_COMPLETE,   (void (*)(void))emAfRf4ceZrcIncomingConfigurationComplete},
  {EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0, EMBER_AF_RF4CE_GDP_COMMAND_GET_ATTRIBUTES,           (void (*)(void))emAfRf4ceZrcIncomingGetAttributes},
  {EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0, EMBER_AF_RF4CE_GDP_COMMAND_GET_ATTRIBUTES_RESPONSE,  (void (*)(void))emAfRf4ceZrcIncomingGetAttributesResponse},
  {EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0, EMBER_AF_RF4CE_GDP_COMMAND_PUSH_ATTRIBUTES,          (void (*)(void))emAfRf4ceZrcIncomingPushAttributes},
  {EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0, EMBER_AF_RF4CE_GDP_COMMAND_SET_ATTRIBUTES,           (void (*)(void))emAfRf4ceZrcIncomingSetAttributes},
  {EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0, EMBER_AF_RF4CE_GDP_COMMAND_PULL_ATTRIBUTES,          (void (*)(void))emAfRf4ceZrcIncomingPullAttributes},
  {EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0, EMBER_AF_RF4CE_GDP_COMMAND_PULL_ATTRIBUTES_RESPONSE, (void (*)(void))emAfRf4ceZrcIncomingPullAttributesResponse},
  {EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0, EMBER_AF_RF4CE_GDP_COMMAND_CLIENT_NOTIFICATION,      (void (*)(void))emAfRf4ceZrcIncomingClientNotification},
};
static void (*findCallback(EmberAfRf4ceProfileId profileId, EmberAfRf4ceGdpCommandCode commandCode))(void)
{
  uint8_t i;
  for (i = 0; i < COUNTOF(callbacks); i++) {
    if (profileId == callbacks[i].profileId
        && commandCode == callbacks[i].commandCode) {
      return callbacks[i].callback;
    }
  }
  return NULL;
}

bool emberAfRf4ceGdpIncomingMessage(uint8_t pairingIndex,
                                       uint8_t profileId,
                                       uint16_t vendorId,
                                       EmberRf4ceTxOption txOptions,
                                       const uint8_t *message,
                                       uint8_t messageLength)
{
  EmberAfRf4ceGdpCommandCode commandCode = COMMAND_CODE(message);
  Decision decision = checkCommand(pairingIndex,
                                   profileId,
                                   vendorId,
                                   message,
                                   messageLength,
                                   true); // RX

  if (decision != ACCEPT) {
    return (decision == DROP);
  }

  incoming = (uint8_t*)message;
  incomingLength = messageLength;
  incomingOffset = GDP_PAYLOAD_OFFSET;

  // On receipt of a packet from an RF4CE Controller device, an RF4CE Target
  // device that supports this version of the GDP profile shall set the
  // Destination Logical Channel of the pairing entry of that RF4CE Controller
  // to the channel the packet was received on (being the nwkBaseChannel the
  // RF4CE Target is operating on). This guarantees that packets that are
  // transmitted by the RF4CE Target to the RF4CE Controller in response to the
  // packet received by the RF4CE Target device are sent on the channel the
  // RF4CE Controller device is listening on.
#if defined(EMBER_AF_RF4CE_NODE_TYPE_TARGET)
  {
    EmberRf4cePairingTableEntry entry;
    if (!(txOptions & EMBER_RF4CE_TX_OPTIONS_BROADCAST_BIT)
        && emberAfRf4ceGetPairingTableEntry(pairingIndex, &entry) == EMBER_SUCCESS
        && !(entry.capabilities & EMBER_RF4CE_NODE_CAPABILITIES_IS_TARGET_BIT)
        && entry.channel != emberAfRf4ceGetBaseChannel()) {
      entry.channel = emberAfRf4ceGetBaseChannel();
      emberAfRf4ceSetPairingTableEntry(pairingIndex, &entry);
    }
  }
#endif // EMBER_AF_RF4CE_NODE_TYPE_TARGET

  switch (commandCode) {
  case EMBER_AF_RF4CE_GDP_COMMAND_HEARTBEAT:
    emAfRf4ceGdpIncomingHeartbeat(message[HEARTBEAT_TRIGGER_OFFSET]);
    break;
  case EMBER_AF_RF4CE_GDP_COMMAND_CHECK_VALIDATION:
    {
      EmberAfRf4ceGdpCheckValidationSubtype subtype = message[CHECK_VALIDATION_SUBTYPE_OFFSET];
      if (subtype == EMBER_AF_RF4CE_GDP_CHECK_VALIDATION_SUBTYPE_REQUEST
          && CHECK_VALIDATION_SUBTYPE_REQUEST_LENGTH <= messageLength) {
        emAfRf4ceGdpIncomingCheckValidationRequest(message[CHECK_VALIDATION_SUBTYPE_REQUEST_CONTROL_OFFSET]);
      } else if (subtype == EMBER_AF_RF4CE_GDP_CHECK_VALIDATION_SUBTYPE_RESPONSE
                 && CHECK_VALIDATION_SUBTYPE_RESPONSE_LENGTH <= messageLength) {
        emAfRf4ceGdpIncomingCheckValidationResponse(message[CHECK_VALIDATION_SUBTYPE_RESPONSE_STATUS_OFFSET]);
      }
      break;
    }
  case EMBER_AF_RF4CE_GDP_COMMAND_KEY_EXCHANGE:
    {
      EmberAfRf4ceGdpKeyExchangeSubtype subtype = message[KEY_EXCHANGE_SUBTYPE_OFFSET];
      if (subtype == EMBER_AF_RF4CE_GDP_KEY_EXCHANGE_SUBTYPE_CHALLENGE
          && KEY_EXCHANGE_SUBTYPE_CHALLENGE_LENGTH <= messageLength) {
        EmberAfRf4ceGdpRand randA;
        MEMMOVE(emberAfRf4ceGdpRandContents(&randA),
                message + KEY_EXCHANGE_SUBTYPE_CHALLENGE_RAND_A_OFFSET,
                EMBER_AF_RF4CE_GDP_RAND_SIZE);
        emAfRf4ceGdpIncomingKeyExchangeChallenge(HIGH_LOW_TO_INT(message[KEY_EXCHANGE_SUBTYPE_CHALLENGE_FLAGS_OFFSET + 1],
                                                                 message[KEY_EXCHANGE_SUBTYPE_CHALLENGE_FLAGS_OFFSET]),
                                                 &randA);
      } else if (subtype == EMBER_AF_RF4CE_GDP_KEY_EXCHANGE_SUBTYPE_CHALLENGE_RESPONSE
                 && KEY_EXCHANGE_SUBTYPE_CHALLENGE_RESPONSE_LENGTH <= messageLength) {
        EmberAfRf4ceGdpRand randB;
        EmberAfRf4ceGdpTag tagB;
        MEMMOVE(emberAfRf4ceGdpRandContents(&randB),
                message + KEY_EXCHANGE_SUBTYPE_CHALLENGE_RESPONSE_RAND_B_OFFSET,
                EMBER_AF_RF4CE_GDP_RAND_SIZE);
        MEMMOVE(emberAfRf4ceGdpTagContents(&tagB),
                message + KEY_EXCHANGE_SUBTYPE_CHALLENGE_RESPONSE_TAG_B_OFFSET,
                EMBER_AF_RF4CE_GDP_TAG_SIZE);
        emAfRf4ceGdpIncomingKeyExchangeChallengeResponse(HIGH_LOW_TO_INT(message[KEY_EXCHANGE_SUBTYPE_CHALLENGE_RESPONSE_FLAGS_OFFSET + 1],
                                                                         message[KEY_EXCHANGE_SUBTYPE_CHALLENGE_RESPONSE_FLAGS_OFFSET]),
                                                         &randB,
                                                         &tagB);
      } else if (subtype == EMBER_AF_RF4CE_GDP_KEY_EXCHANGE_SUBTYPE_RESPONSE
                 && KEY_EXCHANGE_SUBTYPE_RESPONSE_LENGTH <= messageLength) {
        EmberAfRf4ceGdpTag tagA;
        MEMMOVE(emberAfRf4ceGdpTagContents(&tagA),
                message + KEY_EXCHANGE_SUBTYPE_RESPONSE_TAG_A_OFFSET,
                EMBER_AF_RF4CE_GDP_TAG_SIZE);
        emAfRf4ceGdpIncomingKeyExchangeResponse(&tagA);
      } else if (subtype == EMBER_AF_RF4CE_GDP_KEY_EXCHANGE_SUBTYPE_CONFIRM
                 && KEY_EXCHANGE_SUBTYPE_CONFIRM_LENGTH <= messageLength) {
        bool secured = (txOptions & EMBER_RF4CE_TX_OPTIONS_SECURITY_ENABLED_BIT) > 0;
        emAfRf4ceGdpIncomingKeyExchangeConfirm(secured);
      }
      break;
    }
  default:
    break;
  }

  void (*callback)(void) = findCallback(profileId, commandCode);
  if (callback != NULL) {
    switch (commandCode) {
    case EMBER_AF_RF4CE_GDP_COMMAND_GENERIC_RESPONSE:
      ((void (*)(EmberAfRf4ceGdpResponseCode))*callback)(message[GENERIC_RESPONSE_RESPONSE_CODE_OFFSET]);
      break;
    case EMBER_AF_RF4CE_GDP_COMMAND_CONFIGURATION_COMPLETE:
      ((void (*)(EmberAfRf4ceGdpStatus))*callback)(message[CONFIGURATION_COMPLETE_STATUS_OFFSET]);
      break;
    case EMBER_AF_RF4CE_GDP_COMMAND_CLIENT_NOTIFICATION:
      if (CLIENT_NOTIFICATION_LENGTH <= messageLength) {
        ((void (*) (EmberAfRf4ceGdpClientNotificationSubtype, const uint8_t*, uint8_t))*callback)
         (message[CLIENT_NOTIFICATION_SUBTYPE_OFFSET],
          message + CLIENT_NOTIFICATION_PAYLOAD_OFFSET,
          messageLength - CLIENT_NOTIFICATION_PAYLOAD_OFFSET);
      }
      break;
    case EMBER_AF_RF4CE_GDP_COMMAND_GET_ATTRIBUTES:
    case EMBER_AF_RF4CE_GDP_COMMAND_GET_ATTRIBUTES_RESPONSE:
    case EMBER_AF_RF4CE_GDP_COMMAND_PUSH_ATTRIBUTES:
    case EMBER_AF_RF4CE_GDP_COMMAND_SET_ATTRIBUTES:
    case EMBER_AF_RF4CE_GDP_COMMAND_PULL_ATTRIBUTES:
    case EMBER_AF_RF4CE_GDP_COMMAND_PULL_ATTRIBUTES_RESPONSE:
      ((void (*)(void))*callback)();
      break;
    default:
      break;
    }
  }

  incoming = NULL;
  incomingLength = 0;
  incomingOffset = 0xFF;

  // Notify the polling code that a command was received.
  emAfRf4ceGdpPollingIncomingCommandCallback(DATA_PENDING(message) > 0);

  return true;
}

void emAfRf4ceGdpIncomingClientNotification(EmberAfRf4ceGdpClientNotificationSubtype subType,
                                            const uint8_t *clientNotificationPayload,
                                            uint8_t clientNotificationPayloadLength)
{
  if (subType == EMBER_AF_RF4CE_GDP_CLIENT_NOTIFICATION_SUBTYPE_IDENTIFY
      && clientNotificationPayloadLength == CLIENT_NOTIFICATION_SUBTYPE_IDENTIFY_PAYLOAD_LENGTH) {
    emAfRf4ceGdpIncomingIdentifyCallback(clientNotificationPayload[CLIENT_NOTIFICATION_SUBTYPE_IDENTIFY_PAYLOAD_FLAGS_OFFSET],
                                         HIGH_LOW_TO_INT(clientNotificationPayload[CLIENT_NOTIFICATION_SUBTYPE_IDENTIFY_PAYLOD_TIME_OFFSET + 1],
                                                         clientNotificationPayload[CLIENT_NOTIFICATION_SUBTYPE_IDENTIFY_PAYLOD_TIME_OFFSET]));
  } else if (subType == EMBER_AF_RF4CE_GDP_CLIENT_NOTIFICATION_SUBTYPE_REQUEST_POLL_NEGOTIATION
             && clientNotificationPayloadLength == CLIENT_NOTIFICATION_SUBTYPE_REQUEST_POLL_NEGOTIATION_PAYLOAD_LENGTH) {
    // Re-trigger the Poll Negotiation procedure.
    emAfRf4ceGdpPollingNotifyBindingComplete(emberAfRf4ceGetPairingIndex());
  }
}

static Decision checkCommand(uint8_t pairingIndex,
                             uint8_t profileId,
                             uint16_t vendorId,
                             const uint8_t *message,
                             uint8_t messageLength,
                             bool rx)
{
  uint8_t commandCode;

  // Every GDP command has a GDP header.
  if (messageLength < GDP_HEADER_LENGTH) {
    emberAfDebugPrintln("%p: %cX: %p %p",
                        "GDP",
                        (rx ? 'R' : 'T'),
                        "Invalid",
                        "message");
    return DROP;
  }

  // Only GDP commands, either those specifically carrying the GDP profile id
  // or those from other profiles with the GDP command frame bit set, are
  // handled here.
  if (profileId != EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE
      && !GDP_COMMAND_FRAME(message)) {
    emberAfDebugPrintln("%p: %cX: %p %p",
                        "GDP",
                        (rx ? 'R' : 'T'),
                        "Non-GDP",
                        "message");
    return IGNORE;
  }

  commandCode = COMMAND_CODE(message);
  if (COMMAND_CODE_MAXIMUM < commandCode) {
    emberAfDebugPrint("%p: %cX: %p %p",
                      "GDP",
                      (rx ? 'R' : 'T'),
                      "Invalid",
                      "command code");
    emberAfDebugPrintln(": 0x%x", commandCode);
    return DROP;
  }

  if (messageLength < minimumMessageLengths[commandCode]) {
    emberAfDebugPrint("%p: %cX: %p %p",
                      "GDP",
                      (rx ? 'R' : 'T'),
                      "Invalid",
                      "command length");
    emberAfDebugPrintln(": %d", messageLength);
    return DROP;
  }

  // Some commands cannot be sent with a non-GDP profile.
  if (profileId != EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE
      && (commandCode == EMBER_AF_RF4CE_GDP_COMMAND_HEARTBEAT
          || commandCode == EMBER_AF_RF4CE_GDP_COMMAND_CHECK_VALIDATION
          || commandCode == EMBER_AF_RF4CE_GDP_COMMAND_KEY_EXCHANGE)) {
    emberAfDebugPrint("%p: %cX: %p %p",
                      "GDP",
                      (rx ? 'R' : 'T'),
                      "Invalid",
                      "specific profile");
    emberAfDebugPrintln(": 0x%2x", profileId);
    return DROP;
  }

  return ACCEPT;
}

static EmberStatus send(uint8_t pairingIndex, uint8_t profileId, uint16_t vendorId)
{
  EmberRf4ceTxOption txOptions;
  EmberAfRf4ceGdpCommandCode commandCode = COMMAND_CODE(buffer);
  EmberStatus status = emberAfRf4ceGdpGetCommandTxOptions(commandCode,
                                                          pairingIndex,
                                                          vendorId,
                                                          &txOptions);
  if (status != EMBER_SUCCESS) {
    return status;
  }

  return emberAfRf4ceSendExtended(pairingIndex,
                                  profileId,
                                  vendorId,
                                  txOptions,
                                  buffer,
                                  bufferLength,
                                  NULL); // message tag - unused
}

EmberStatus emberAfRf4ceGdpGenericResponse(uint8_t pairingIndex,
                                           uint8_t profileId,
                                           uint16_t vendorId,
                                           EmberAfRf4ceGdpResponseCode responseCode)
{
  bufferLength = 0;
  buffer[bufferLength++] = (EMBER_AF_RF4CE_GDP_COMMAND_GENERIC_RESPONSE
                            | emAfRf4ceGdpOutgoingCommandFrameControl);
  buffer[bufferLength++] = responseCode;
  return send(pairingIndex, profileId, vendorId);
}

EmberStatus emberAfRf4ceGdpConfigurationComplete(uint8_t pairingIndex,
                                                 uint8_t profileId,
                                                 uint16_t vendorId,
                                                 EmberAfRf4ceGdpStatus status)
{
  bufferLength = 0;
  buffer[bufferLength++] = (EMBER_AF_RF4CE_GDP_COMMAND_CONFIGURATION_COMPLETE
                            | emAfRf4ceGdpOutgoingCommandFrameControl);
  buffer[bufferLength++] = status;
  return send(pairingIndex, profileId, vendorId);
}

EmberStatus emberAfRf4ceGdpHeartbeat(uint8_t pairingIndex,
                                     uint16_t vendorId,
                                     EmberAfRf4ceGdpHeartbeatTrigger trigger)
{
  bufferLength = 0;
  buffer[bufferLength++] = (EMBER_AF_RF4CE_GDP_COMMAND_HEARTBEAT
                            | emAfRf4ceGdpOutgoingCommandFrameControl);
  buffer[bufferLength++] = trigger;
  return send(pairingIndex, EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE, vendorId);
}

EmberStatus emberAfRf4ceGdpGetAttributes(uint8_t pairingIndex,
                                         uint8_t profileId,
                                         uint16_t vendorId,
                                         const EmberAfRf4ceGdpAttributeIdentificationRecord *records,
                                         uint8_t recordsLength)
{
  return getPullAttributes(EMBER_AF_RF4CE_GDP_COMMAND_GET_ATTRIBUTES,
                           pairingIndex,
                           profileId,
                           vendorId,
                           records,
                           recordsLength);
}

EmberStatus emberAfRf4ceGdpGetAttributesResponse(uint8_t pairingIndex,
                                                 uint8_t profileId,
                                                 uint16_t vendorId,
                                                 const EmberAfRf4ceGdpAttributeStatusRecord *records,
                                                 uint8_t recordsLength)
{
  return getPullAttributesResponse(EMBER_AF_RF4CE_GDP_COMMAND_GET_ATTRIBUTES_RESPONSE,
                                   pairingIndex,
                                   profileId,
                                   vendorId,
                                   records,
                                   recordsLength);
}

EmberStatus emberAfRf4ceGdpPushAttributes(uint8_t pairingIndex,
                                          uint8_t profileId,
                                          uint16_t vendorId,
                                          const EmberAfRf4ceGdpAttributeRecord *records,
                                          uint8_t recordsLength)
{
  return setPushAttributes(EMBER_AF_RF4CE_GDP_COMMAND_PUSH_ATTRIBUTES,
                           pairingIndex,
                           profileId,
                           vendorId,
                           records,
                           recordsLength);
}

EmberStatus emberAfRf4ceGdpSetAttributes(uint8_t pairingIndex,
                                         uint8_t profileId,
                                         uint16_t vendorId,
                                         const EmberAfRf4ceGdpAttributeRecord *records,
                                         uint8_t recordsLength)
{
  return setPushAttributes(EMBER_AF_RF4CE_GDP_COMMAND_SET_ATTRIBUTES,
                           pairingIndex,
                           profileId,
                           vendorId,
                           records,
                           recordsLength);
}

EmberStatus emberAfRf4ceGdpPullAttributes(uint8_t pairingIndex,
                                          uint8_t profileId,
                                          uint16_t vendorId,
                                          const EmberAfRf4ceGdpAttributeIdentificationRecord *records,
                                          uint8_t recordsLength)
{
  return getPullAttributes(EMBER_AF_RF4CE_GDP_COMMAND_PULL_ATTRIBUTES,
                           pairingIndex,
                           profileId,
                           vendorId,
                           records,
                           recordsLength);
}

EmberStatus emberAfRf4ceGdpPullAttributesResponse(uint8_t pairingIndex,
                                                  uint8_t profileId,
                                                  uint16_t vendorId,
                                                  const EmberAfRf4ceGdpAttributeStatusRecord *records,
                                                  uint8_t recordsLength)
{
  return getPullAttributesResponse(EMBER_AF_RF4CE_GDP_COMMAND_PULL_ATTRIBUTES_RESPONSE,
                                   pairingIndex,
                                   profileId,
                                   vendorId,
                                   records,
                                   recordsLength);
}

EmberStatus emberAfRf4ceGdpCheckValidationRequest(uint8_t pairingIndex,
                                                  uint16_t vendorId,
                                                  uint8_t control)
{
  bufferLength = 0;
  buffer[bufferLength++] = (EMBER_AF_RF4CE_GDP_COMMAND_CHECK_VALIDATION
                            | emAfRf4ceGdpOutgoingCommandFrameControl);
  buffer[bufferLength++] = EMBER_AF_RF4CE_GDP_CHECK_VALIDATION_SUBTYPE_REQUEST;
  buffer[bufferLength++] = control;
  return send(pairingIndex, EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE, vendorId);
}

EmberStatus emberAfRf4ceGdpCheckValidationResponse(uint8_t pairingIndex,
                                                   uint16_t vendorId,
                                                   EmberAfRf4ceGdpCheckValidationStatus status)
{
  bufferLength = 0;
  buffer[bufferLength++] = (EMBER_AF_RF4CE_GDP_COMMAND_CHECK_VALIDATION
                            | emAfRf4ceGdpOutgoingCommandFrameControl);
  buffer[bufferLength++] = EMBER_AF_RF4CE_GDP_CHECK_VALIDATION_SUBTYPE_RESPONSE;
  buffer[bufferLength++] = status;
  return send(pairingIndex, EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE, vendorId);
}

EmberStatus emberAfRf4ceGdpClientNotificationIdentify(uint8_t pairingIndex,
                                                      uint16_t vendorId,
                                                      EmberAfRf4ceGdpClientNotificationIdentifyFlags flags,
                                                      uint16_t timeS)
{
  bufferLength = 0;
  buffer[bufferLength++] = (EMBER_AF_RF4CE_GDP_COMMAND_CLIENT_NOTIFICATION
                            | emAfRf4ceGdpOutgoingCommandFrameControl);
  buffer[bufferLength++] = EMBER_AF_RF4CE_GDP_CLIENT_NOTIFICATION_SUBTYPE_IDENTIFY;
  buffer[bufferLength++] = flags;
  buffer[bufferLength++] = LOW_BYTE(timeS);
  buffer[bufferLength++] = HIGH_BYTE(timeS);
  return send(pairingIndex, EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE, vendorId);
}

EmberStatus emberAfRf4ceGdpClientNotificationRequestPollNegotiation(uint8_t pairingIndex,
                                                                    uint16_t vendorId)
{
  bufferLength = 0;
  buffer[bufferLength++] = (EMBER_AF_RF4CE_GDP_COMMAND_CLIENT_NOTIFICATION
                            | emAfRf4ceGdpOutgoingCommandFrameControl);
  buffer[bufferLength++] = EMBER_AF_RF4CE_GDP_CLIENT_NOTIFICATION_SUBTYPE_REQUEST_POLL_NEGOTIATION;
  return send(pairingIndex, EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE, vendorId);
}

EmberStatus emberAfRf4ceGdpClientNotification(uint8_t pairingIndex,
                                              uint8_t profileId,
                                              uint16_t vendorId,
                                              uint8_t subtype,
                                              const uint8_t *payload,
                                              uint8_t payloadLength)
{
  bufferLength = 0;
  buffer[bufferLength++] = (EMBER_AF_RF4CE_GDP_COMMAND_CLIENT_NOTIFICATION
                            | emAfRf4ceGdpOutgoingCommandFrameControl);
  buffer[bufferLength++] = subtype;
  if (sizeof(buffer) < bufferLength + payloadLength) {
    return EMBER_MESSAGE_TOO_LONG;
  }
  if (payload != NULL) {
    MEMCOPY(buffer + bufferLength, payload, payloadLength);
  }
  bufferLength += payloadLength;
  return send(pairingIndex, profileId, vendorId);
}

EmberStatus emAfRf4ceGdpKeyExchangeChallenge(uint8_t pairingIndex,
                                             uint16_t vendorId,
                                             EmberAfRf4ceGdpKeyExchangeFlags flags,
                                             const EmberAfRf4ceGdpRand *randA)
{
  bufferLength = 0;
  buffer[bufferLength++] = (EMBER_AF_RF4CE_GDP_COMMAND_KEY_EXCHANGE
                            | emAfRf4ceGdpOutgoingCommandFrameControl);
  buffer[bufferLength++] = EMBER_AF_RF4CE_GDP_KEY_EXCHANGE_SUBTYPE_CHALLENGE;
  buffer[bufferLength++] = LOW_BYTE(flags);
  buffer[bufferLength++] = HIGH_BYTE(flags);
  MEMMOVE(buffer + bufferLength,
          emberAfRf4ceGdpRandContents(randA),
          EMBER_AF_RF4CE_GDP_RAND_SIZE);
  bufferLength += EMBER_AF_RF4CE_GDP_RAND_SIZE;
  return send(pairingIndex, EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE, vendorId);
}

EmberStatus emAfRf4ceGdpKeyExchangeChallengeResponse(uint8_t pairingIndex,
                                                     uint16_t vendorId,
                                                     EmberAfRf4ceGdpKeyExchangeFlags flags,
                                                     const EmberAfRf4ceGdpRand *randB,
                                                     const EmberAfRf4ceGdpTag *tagB)
{
  bufferLength = 0;
  buffer[bufferLength++] = (EMBER_AF_RF4CE_GDP_COMMAND_KEY_EXCHANGE
                            | emAfRf4ceGdpOutgoingCommandFrameControl);
  buffer[bufferLength++] = EMBER_AF_RF4CE_GDP_KEY_EXCHANGE_SUBTYPE_CHALLENGE_RESPONSE;
  buffer[bufferLength++] = LOW_BYTE(flags);
  buffer[bufferLength++] = HIGH_BYTE(flags);
  MEMMOVE(buffer + bufferLength,
          emberAfRf4ceGdpRandContents(randB),
          EMBER_AF_RF4CE_GDP_RAND_SIZE);
  bufferLength += EMBER_AF_RF4CE_GDP_RAND_SIZE;
  MEMMOVE(buffer + bufferLength,
          emberAfRf4ceGdpTagContents(tagB),
          EMBER_AF_RF4CE_GDP_TAG_SIZE);
  bufferLength += EMBER_AF_RF4CE_GDP_TAG_SIZE;
  return send(pairingIndex, EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE, vendorId);
}

EmberStatus emAfRf4ceGdpKeyExchangeResponse(uint8_t pairingIndex,
                                            uint16_t vendorId,
                                            const EmberAfRf4ceGdpTag *tagA)
{
  bufferLength = 0;
  buffer[bufferLength++] = (EMBER_AF_RF4CE_GDP_COMMAND_KEY_EXCHANGE
                            | emAfRf4ceGdpOutgoingCommandFrameControl);
  buffer[bufferLength++] = EMBER_AF_RF4CE_GDP_KEY_EXCHANGE_SUBTYPE_RESPONSE;
  MEMMOVE(buffer + bufferLength,
          emberAfRf4ceGdpTagContents(tagA),
          EMBER_AF_RF4CE_GDP_TAG_SIZE);
  bufferLength += EMBER_AF_RF4CE_GDP_TAG_SIZE;
  return send(pairingIndex, EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE, vendorId);
}

EmberStatus emAfRf4ceGdpKeyExchangeConfirm(uint8_t pairingIndex,
                                           uint16_t vendorId)
{
  bufferLength = 0;
  buffer[bufferLength++] = (EMBER_AF_RF4CE_GDP_COMMAND_KEY_EXCHANGE
                            | emAfRf4ceGdpOutgoingCommandFrameControl);
  buffer[bufferLength++] = EMBER_AF_RF4CE_GDP_KEY_EXCHANGE_SUBTYPE_CONFIRM;
  return send(pairingIndex, EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE, vendorId);
}

EmberStatus emberAfRf4ceGdpGetCommandTxOptions(EmberAfRf4ceGdpCommandCode commandCode,
                                               uint8_t pairingIndex,
                                               uint16_t vendorId,
                                               EmberRf4ceTxOption *txOptions)
{
  // All GDP commands are sent unicast and acknowledged.  Heartbeat messages
  // and all messages to controllers use single-channel service.  Otherwise,
  // multiple channels are used.  The Key Exchange commands do not use security
  // except for the Confirm message.  All other commands use security.  See
  // sections 5, 5.3.2, and 5.12.2 of 13-0396.

  EmberRf4cePairingTableEntry destinationEntry;
  EmberStatus status;

  if (pairingIndex >= EMBER_RF4CE_PAIRING_TABLE_SIZE) {
    return EMBER_BAD_ARGUMENT;
  }

  status = emberAfRf4ceGetPairingTableEntry(pairingIndex, &destinationEntry);
  if (status != EMBER_SUCCESS) {
    return status;
  }

  *txOptions = EMBER_RF4CE_TX_OPTIONS_ACK_REQUESTED_BIT;

  if (commandCode == EMBER_AF_RF4CE_GDP_COMMAND_HEARTBEAT
      || !READBITS(destinationEntry.capabilities,
                   EMBER_RF4CE_NODE_CAPABILITIES_IS_TARGET_BIT)) {
    *txOptions |= EMBER_RF4CE_TX_OPTIONS_SINGLE_CHANNEL_BIT;
  }

  if (commandCode != EMBER_AF_RF4CE_GDP_COMMAND_KEY_EXCHANGE
      || (buffer[KEY_EXCHANGE_SUBTYPE_OFFSET]
          == EMBER_AF_RF4CE_GDP_KEY_EXCHANGE_SUBTYPE_CONFIRM)) {
    *txOptions |= EMBER_RF4CE_TX_OPTIONS_SECURITY_ENABLED_BIT;
  }

  if (vendorId != EMBER_RF4CE_NULL_VENDOR_ID) {
    *txOptions |= EMBER_RF4CE_TX_OPTIONS_VENDOR_SPECIFIC_BIT;
  }

  return EMBER_SUCCESS;
}

static EmberStatus getPullAttributes(EmberAfRf4ceGdpCommandCode commandCode,
                                     uint8_t pairingIndex,
                                     uint8_t profileId,
                                     uint16_t vendorId,
                                     const EmberAfRf4ceGdpAttributeIdentificationRecord *records,
                                     uint8_t recordsLength)
{
  uint8_t i;
  emAfRf4ceGdpStartAttributesCommand(commandCode);
  for (i = 0; i < recordsLength; i++) {
    if (!emAfRf4ceGdpAppendAttributeIdentificationRecord(&records[i])) {
      return EMBER_MESSAGE_TOO_LONG;
    }
  }
  return emAfRf4ceGdpSendAttributesCommand(pairingIndex, profileId, vendorId);
}

static EmberStatus getPullAttributesResponse(EmberAfRf4ceGdpCommandCode commandCode,
                                             uint8_t pairingIndex,
                                             uint8_t profileId,
                                             uint16_t vendorId,
                                             const EmberAfRf4ceGdpAttributeStatusRecord *records,
                                             uint8_t recordsLength)
{
  uint8_t i;
  emAfRf4ceGdpStartAttributesCommand(commandCode);
  for (i = 0; i < recordsLength; i++) {
    if (!emAfRf4ceGdpAppendAttributeStatusRecord(&records[i])) {
      return EMBER_MESSAGE_TOO_LONG;
    }
  }
  return emAfRf4ceGdpSendAttributesCommand(pairingIndex, profileId, vendorId);
}

static EmberStatus setPushAttributes(EmberAfRf4ceGdpCommandCode commandCode,
                                     uint8_t pairingIndex,
                                     uint8_t profileId,
                                     uint16_t vendorId,
                                     const EmberAfRf4ceGdpAttributeRecord *records,
                                     uint8_t recordsLength)
{
  uint8_t i;
  emAfRf4ceGdpStartAttributesCommand(commandCode);
  for (i = 0; i < recordsLength; i++) {
    if (!emAfRf4ceGdpAppendAttributeRecord(&records[i])) {
      return EMBER_MESSAGE_TOO_LONG;
    }
  }
  return emAfRf4ceGdpSendAttributesCommand(pairingIndex, profileId, vendorId);
}

void emAfRf4ceGdpStartAttributesCommand(EmberAfRf4ceGdpCommandCode commandCode)
{
  bufferLength = 0;
  buffer[bufferLength++] = (commandCode
                            | emAfRf4ceGdpOutgoingCommandFrameControl);
}

EmberStatus emAfRf4ceGdpSendAttributesCommand(uint8_t pairingIndex,
                                              uint8_t profileId,
                                              uint16_t vendorId)
{
  return send(pairingIndex, profileId, vendorId);
}

EmberStatus emAfRf4ceGdpSendProfileSpecificCommand(uint8_t pairingIndex,
                                                   uint8_t profileId,
                                                   uint16_t vendorId,
                                                   EmberRf4ceTxOption txOptions,
                                                   uint8_t commandId,
                                                   uint8_t *commandPayload,
                                                   uint8_t commandPayloadLength,
                                                   uint8_t *messageTag)
{
  buffer[GDP_HEADER_FRAME_CONTROL_OFFSET] =
      ((commandId | emAfRf4ceGdpOutgoingCommandFrameControl)
       & ~GDP_HEADER_FRAME_CONTROL_GDP_COMMAND_FRAME_MASK);

  MEMCOPY(buffer + GDP_HEADER_LENGTH, commandPayload, commandPayloadLength);

  return emberAfRf4ceSendExtended(pairingIndex,
                                  profileId,
                                  vendorId,
                                  txOptions,
                                  buffer,
                                  GDP_HEADER_LENGTH + commandPayloadLength,
                                  messageTag);
}

bool emAfRf4ceGdpAppendAttributeIdentificationRecord(const EmberAfRf4ceGdpAttributeIdentificationRecord *record)
{
  if (sizeof(buffer) < bufferLength + 1) {
    return false;
  }
  buffer[bufferLength++] = record->attributeId;
  if (IS_ARRAY_ATTRIBUTE(record->attributeId)) {
    if (sizeof(buffer) < bufferLength + 2) {
      return false;
    }
    buffer[bufferLength++] = LOW_BYTE(record->entryId);
    buffer[bufferLength++] = HIGH_BYTE(record->entryId);
  }
  return true;
}

bool emAfRf4ceGdpFetchAttributeIdentificationRecord(EmberAfRf4ceGdpAttributeIdentificationRecord *record)
{
  if (incomingLength < incomingOffset + 1) {
    return false;
  }
  record->attributeId = incoming[incomingOffset++];
  if (IS_ARRAY_ATTRIBUTE(record->attributeId)) {
    if (incomingLength < incomingOffset + 2) {
      return false;
    }
    record->entryId = HIGH_LOW_TO_INT(incoming[incomingOffset + 1],
                                      incoming[incomingOffset]);
    incomingOffset += 2;
  }
  return true;
}

bool emAfRf4ceGdpAppendAttributeStatusRecord(const EmberAfRf4ceGdpAttributeStatusRecord *record)
{
  if (sizeof(buffer) < bufferLength + 1) {
    return false;
  }
  buffer[bufferLength++] = record->attributeId;
  if (IS_ARRAY_ATTRIBUTE(record->attributeId)) {
    if (sizeof(buffer) < bufferLength + 2) {
      return false;
    }
    buffer[bufferLength++] = LOW_BYTE(record->entryId);
    buffer[bufferLength++] = HIGH_BYTE(record->entryId);
  }
  if (sizeof(buffer) < bufferLength + 1) {
    return false;
  }
  buffer[bufferLength++] = record->status;
  if (record->status == EMBER_AF_RF4CE_GDP_ATTRIBUTE_STATUS_SUCCESS) {
    if (sizeof(buffer) < bufferLength + 1 + record->valueLength) {
      return false;
    }
    buffer[bufferLength++] = record->valueLength;
    MEMMOVE(buffer + bufferLength, record->value, record->valueLength);
    bufferLength += record->valueLength;
  }
  return true;
}

bool emAfRf4ceGdpFetchAttributeStatusRecord(EmberAfRf4ceGdpAttributeStatusRecord *record)
{
  if (incomingLength < incomingOffset + 1) {
    return false;
  }
  record->attributeId = incoming[incomingOffset++];
  if (IS_ARRAY_ATTRIBUTE(record->attributeId)) {
    if (incomingLength < incomingOffset + 2) {
      return false;
    }
    record->entryId = HIGH_LOW_TO_INT(incoming[incomingOffset + 1],
                                      incoming[incomingOffset]);
    incomingOffset += 2;
  }
  if (incomingLength < incomingOffset + 1) {
    return false;
  }
  record->status = incoming[incomingOffset++];
  if (record->status == EMBER_AF_RF4CE_GDP_ATTRIBUTE_STATUS_SUCCESS) {
    if (incomingLength < incomingOffset + 1) {
      return false;
    }
    record->valueLength = incoming[incomingOffset++];
    if (incomingLength < incomingOffset + record->valueLength) {
      return false;
    }
    record->value = incoming + incomingOffset;
    incomingOffset += record->valueLength;
  }
  return true;
}

bool emAfRf4ceGdpAppendAttributeRecord(const EmberAfRf4ceGdpAttributeRecord *record)
{
  if (sizeof(buffer) < bufferLength + 1) {
    return false;
  }
  buffer[bufferLength++] = record->attributeId;
  if (IS_ARRAY_ATTRIBUTE(record->attributeId)) {
    if (sizeof(buffer) < bufferLength + 2) {
      return false;
    }
    buffer[bufferLength++] = LOW_BYTE(record->entryId);
    buffer[bufferLength++] = HIGH_BYTE(record->entryId);
  }
  if (sizeof(buffer) < bufferLength + 1 + record->valueLength) {
    return false;
  }
  buffer[bufferLength++] = record->valueLength;
  MEMMOVE(buffer + bufferLength, record->value, record->valueLength);
  bufferLength += record->valueLength;
  return true;
}

bool emAfRf4ceGdpFetchAttributeRecord(EmberAfRf4ceGdpAttributeRecord *record)
{
  if (incomingLength < incomingOffset + 1) {
    return false;
  }
  record->attributeId = incoming[incomingOffset++];
  if (IS_ARRAY_ATTRIBUTE(record->attributeId)) {
    if (incomingLength < incomingOffset + 2) {
      return false;
    }
    record->entryId = HIGH_LOW_TO_INT(incoming[incomingOffset + 1],
                                      incoming[incomingOffset]);
    incomingOffset += 2;
  }
  if (incomingLength < incomingOffset + 1) {
    return false;
  }
  record->valueLength = incoming[incomingOffset++];
  if (incomingLength < incomingOffset + record->valueLength) {
    return false;
  }
  record->value = incoming + incomingOffset;
  incomingOffset += record->valueLength;
  return true;
}

void emAfRf4ceGdpResetFetchAttributeFinger(void)
{
  incomingOffset = GDP_PAYLOAD_OFFSET;
}

#if defined(EMBER_SCRIPTED_TEST)

#include "stack/core/ember-stack.h"
#include "stack/core/parcel.h"

Parcel *makeAttributeIdentificationRecordParcel(EmberAfRf4ceGdpAttributeIdentificationRecord *record)
{
  if (IS_ARRAY_ATTRIBUTE(record->attributeId)) {
    return makeMessage("1<2", record->attributeId, record->entryId);
  } else {
    return makeMessage("1", record->attributeId);
  }
}

Parcel *makeAttributeStatusRecordParcel(EmberAfRf4ceGdpAttributeStatusRecord *record)
{
  if (IS_ARRAY_ATTRIBUTE(record->attributeId)) {
    if (record->status == EMBER_AF_RF4CE_GDP_ATTRIBUTE_STATUS_SUCCESS) {
      return makeMessage("1<211p",
                         record->attributeId,
                         record->entryId,
                         record->status,
                         record->valueLength,
                         makeMessage("s", record->value, record->valueLength));
    } else {
      return makeMessage("1<21",
                         record->attributeId,
                         record->entryId,
                         record->status);
    }
  } else {
    if (record->status == EMBER_AF_RF4CE_GDP_ATTRIBUTE_STATUS_SUCCESS) {
      return makeMessage("111p",
                         record->attributeId,
                         record->status,
                         record->valueLength,
                         makeMessage("s", record->value, record->valueLength));
    } else {
      return makeMessage("11",
                         record->attributeId,
                         record->status);
    }
  }
}

Parcel *makeAttributeRecordParcel(EmberAfRf4ceGdpAttributeRecord *record)
{
  if (IS_ARRAY_ATTRIBUTE(record->attributeId)) {
    return makeMessage("1<21p",
                       record->attributeId,
                       record->entryId,
                       record->valueLength,
                       makeMessage("s", record->value, record->valueLength));
  } else {
    return makeMessage("11p",
                       record->attributeId,
                       record->valueLength,
                       makeMessage("s", record->value, record->valueLength));
  }
}

Parcel *makeGenericResponseParcel(uint8_t responseCode)
{
  return makeMessage("11",
                     (EMBER_AF_RF4CE_GDP_COMMAND_GENERIC_RESPONSE
                      | GDP_HEADER_FRAME_CONTROL_GDP_COMMAND_FRAME_MASK),
                      responseCode);
}

Parcel *makeConfigurationCompleteParcel(uint8_t status)
{
  return makeMessage("11",
                     (EMBER_AF_RF4CE_GDP_COMMAND_CONFIGURATION_COMPLETE
                      | GDP_HEADER_FRAME_CONTROL_GDP_COMMAND_FRAME_MASK),
                      status);
}

Parcel *makeHeartbeatParcel(EmberAfRf4ceGdpHeartbeatTrigger trigger)
{
  return makeMessage("11",
                     (EMBER_AF_RF4CE_GDP_COMMAND_HEARTBEAT
                      | GDP_HEADER_FRAME_CONTROL_GDP_COMMAND_FRAME_MASK),
                      trigger);
}

Parcel *makeNotificationIdentifyParcel(EmberAfRf4ceGdpClientNotificationIdentifyFlags flags,
                                       uint16_t timeS)
{
  return makeMessage("111<2",
                     (EMBER_AF_RF4CE_GDP_COMMAND_CLIENT_NOTIFICATION
                      | GDP_HEADER_FRAME_CONTROL_GDP_COMMAND_FRAME_MASK),
                     EMBER_AF_RF4CE_GDP_CLIENT_NOTIFICATION_SUBTYPE_IDENTIFY,
                     flags,
                     timeS);
}

Parcel *makeGetAttributesParcel(EmberAfRf4ceGdpAttributeIdentificationRecord *records,
                                uint8_t recordsLength)
{
  Parcel *currParcel = NULL;
  uint8_t i;
  for(i=0; i<recordsLength; i++) {
    Parcel *attrIdRecordParcel =
        makeAttributeIdentificationRecordParcel(&records[i]);
    if (currParcel == NULL) {
      currParcel = attrIdRecordParcel;
    } else {
      currParcel = makeMessage("pp", currParcel, attrIdRecordParcel);
    }
  }

  return makeMessage("1p",
                     (EMBER_AF_RF4CE_GDP_COMMAND_GET_ATTRIBUTES
                      | GDP_HEADER_FRAME_CONTROL_GDP_COMMAND_FRAME_MASK),
                      currParcel);
}

Parcel *makeGetAttributesResponseParcel(EmberAfRf4ceGdpAttributeStatusRecord *records,
                                        uint8_t recordsLength)
{
  Parcel *currParcel = NULL;
  uint8_t i;
  for(i=0; i<recordsLength; i++) {
    Parcel *attrStatusRecordParcel =
        makeAttributeStatusRecordParcel(&records[i]);
    if (currParcel == NULL) {
      currParcel = attrStatusRecordParcel;
    } else {
      currParcel = makeMessage("pp", currParcel, attrStatusRecordParcel);
    }
  }

  return makeMessage("1p",
                     (EMBER_AF_RF4CE_GDP_COMMAND_GET_ATTRIBUTES_RESPONSE
                      | GDP_HEADER_FRAME_CONTROL_GDP_COMMAND_FRAME_MASK),
                      currParcel);
}

Parcel *makePushAttributesParcel(EmberAfRf4ceGdpAttributeRecord *records,
                                 uint8_t recordsLength)
{
  Parcel *currParcel = NULL;
  uint8_t i;
  for(i=0; i<recordsLength; i++) {
    Parcel *attrIdRecordParcel =
        makeAttributeRecordParcel(&records[i]);
    if (currParcel == NULL) {
      currParcel = attrIdRecordParcel;
    } else {
      currParcel = makeMessage("pp", currParcel, attrIdRecordParcel);
    }
  }

  return makeMessage("1p",
                     (EMBER_AF_RF4CE_GDP_COMMAND_PUSH_ATTRIBUTES
                      | GDP_HEADER_FRAME_CONTROL_GDP_COMMAND_FRAME_MASK),
                      currParcel);
}

Parcel *makeSetAttributesParcel(EmberAfRf4ceGdpAttributeRecord *records,
                                uint8_t recordsLength)
{
  Parcel *currParcel = NULL;
  uint8_t i;
  for(i=0; i<recordsLength; i++) {
    Parcel *attrIdRecordParcel =
        makeAttributeRecordParcel(&records[i]);
    if (currParcel == NULL) {
      currParcel = attrIdRecordParcel;
    } else {
      currParcel = makeMessage("pp", currParcel, attrIdRecordParcel);
    }
  }

  return makeMessage("1p",
                     (EMBER_AF_RF4CE_GDP_COMMAND_SET_ATTRIBUTES
                      | GDP_HEADER_FRAME_CONTROL_GDP_COMMAND_FRAME_MASK),
                      currParcel);
}

Parcel *makePullAttributesParcel(EmberAfRf4ceGdpAttributeIdentificationRecord *records,
                                 uint8_t recordsLength)
{
  Parcel *currParcel = NULL;
  uint8_t i;
  for(i=0; i<recordsLength; i++) {
    Parcel *attrIdRecordParcel =
        makeAttributeIdentificationRecordParcel(&records[i]);
    if (currParcel == NULL) {
      currParcel = attrIdRecordParcel;
    } else {
      currParcel = makeMessage("pp", currParcel, attrIdRecordParcel);
    }
  }

  return makeMessage("1p",
                     (EMBER_AF_RF4CE_GDP_COMMAND_PULL_ATTRIBUTES
                      | GDP_HEADER_FRAME_CONTROL_GDP_COMMAND_FRAME_MASK),
                      currParcel);
}

Parcel *makePullAttributesResponseParcel(EmberAfRf4ceGdpAttributeStatusRecord *records,
                                         uint8_t recordsLength)
{
  Parcel *currParcel = NULL;
  uint8_t i;
  for(i=0; i<recordsLength; i++) {
    Parcel *attrStatusRecordParcel =
        makeAttributeStatusRecordParcel(&records[i]);
    if (currParcel == NULL) {
      currParcel = attrStatusRecordParcel;
    } else {
      currParcel = makeMessage("pp", currParcel, attrStatusRecordParcel);
    }
  }

  return makeMessage("1p",
                     (EMBER_AF_RF4CE_GDP_COMMAND_PULL_ATTRIBUTES_RESPONSE
                      | GDP_HEADER_FRAME_CONTROL_GDP_COMMAND_FRAME_MASK),
                      currParcel);
}

Parcel *makeCheckValidationRequestParcel(void)
{
  return makeMessage("111",
                     (EMBER_AF_RF4CE_GDP_COMMAND_CHECK_VALIDATION
                      | GDP_HEADER_FRAME_CONTROL_GDP_COMMAND_FRAME_MASK),
                      EMBER_AF_RF4CE_GDP_CHECK_VALIDATION_SUBTYPE_REQUEST,
                      0x00);
}

Parcel *makeCheckValidationResponseParcel(EmberAfRf4ceGdpCheckValidationStatus status)
{
  return makeMessage("111",
                     (EMBER_AF_RF4CE_GDP_COMMAND_CHECK_VALIDATION
                      | GDP_HEADER_FRAME_CONTROL_GDP_COMMAND_FRAME_MASK),
                      EMBER_AF_RF4CE_GDP_CHECK_VALIDATION_SUBTYPE_RESPONSE,
                      status);
}

Parcel *makeKeyExchangeChallengeParcel(uint16_t flags, uint8_t *randA)
{
  return makeMessage("11<2p", // GDP header, subType, flags, randA
                     (EMBER_AF_RF4CE_GDP_COMMAND_KEY_EXCHANGE
                      | GDP_HEADER_FRAME_CONTROL_GDP_COMMAND_FRAME_MASK),
                      EMBER_AF_RF4CE_GDP_KEY_EXCHANGE_SUBTYPE_CHALLENGE,
                      flags,
                      makeMessage("s", randA, EMBER_AF_RF4CE_GDP_RAND_SIZE));
}

Parcel *makeKeyExchangeChallengeResponseParcel(uint16_t flags, uint8_t *randB, uint8_t *tagB)
{
  return makeMessage("11<2pp", // GDP header, subType, flags, randB, tagB
                     (EMBER_AF_RF4CE_GDP_COMMAND_KEY_EXCHANGE
                      | GDP_HEADER_FRAME_CONTROL_GDP_COMMAND_FRAME_MASK),
                      EMBER_AF_RF4CE_GDP_KEY_EXCHANGE_SUBTYPE_CHALLENGE_RESPONSE,
                      flags,
                      makeMessage("s", randB, EMBER_AF_RF4CE_GDP_RAND_SIZE),
                      makeMessage("s", tagB, EMBER_AF_RF4CE_GDP_TAG_SIZE));
}

Parcel *makeKeyExchangeResponseParcel(uint8_t *tagA)
{
  return makeMessage("11p", // GDP header, subType, tagA
                     (EMBER_AF_RF4CE_GDP_COMMAND_KEY_EXCHANGE
                      | GDP_HEADER_FRAME_CONTROL_GDP_COMMAND_FRAME_MASK),
                      EMBER_AF_RF4CE_GDP_KEY_EXCHANGE_SUBTYPE_RESPONSE,
                      makeMessage("s", tagA, EMBER_AF_RF4CE_GDP_TAG_SIZE));
}

Parcel *makeKeyExchangeConfirmParcel(void)
{
  return makeMessage("11", // GDP header, subType
                     (EMBER_AF_RF4CE_GDP_COMMAND_KEY_EXCHANGE
                      | GDP_HEADER_FRAME_CONTROL_GDP_COMMAND_FRAME_MASK),
                      EMBER_AF_RF4CE_GDP_KEY_EXCHANGE_SUBTYPE_CONFIRM);
}

#endif
