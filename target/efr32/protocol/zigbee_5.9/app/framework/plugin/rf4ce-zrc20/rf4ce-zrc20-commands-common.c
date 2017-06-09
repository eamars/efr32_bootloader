// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "app/framework/plugin/rf4ce-profile/rf4ce-profile.h"
#include "app/framework/plugin/rf4ce-gdp/rf4ce-gdp.h"
#include "../rf4ce-gdp/rf4ce-gdp-internal.h"
#include "rf4ce-zrc20.h"
#include "rf4ce-zrc20-attributes.h"
#include "rf4ce-zrc20-internal.h"

#if defined(EMBER_SCRIPTED_TEST)
#include "rf4ce-zrc20-test.h"
#endif

EmberEventControl emberAfPluginRf4ceZrc20LegacyCommandDiscoveryEventControl;

EmberStatus emberAfRf4ceZrc20LegacyCommandDiscovery(uint8_t pairingIndex)
{
  EmberStatus status;

  uint8_t buffer[COMMAND_DISCOVERY_REQUEST_LENGTH] = {
    EMBER_AF_RF4CE_ZRC_COMMAND_COMMAND_DISCOVERY_REQUEST, // commandId
    0x00,                                                 // reserved byte
  };

  // - We only support one legacy command discovery at a time.
  // - The legacy command discovery procedure can only be executed if the node
  //   has previously established a binding with the peer node.
  // - We allow legacy 1.1 command discovery on if the destination pairing
  //   supports 1.1 and does not support 2.0.
  if (emberEventControlGetActive(emberAfPluginRf4ceZrc20LegacyCommandDiscoveryEventControl)
      || ((emAfRf4ceGdpGetPairingBindStatus(pairingIndex)
           & PAIRING_ENTRY_BINDING_STATUS_MASK) == PAIRING_ENTRY_BINDING_STATUS_NOT_BOUND)
      || emAfRf4ceZrc20GetPeerZrcVersion(pairingIndex) != ZRC_VERSION_1_1) {
    return EMBER_INVALID_CALL;
  }

  status = emberAfRf4ceSend(pairingIndex,
                            EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_1_1,
                            buffer,
                            COMMAND_DISCOVERY_REQUEST_LENGTH,
                            NULL); // message tag - unused

  if (status == EMBER_SUCCESS) {
    emberEventControlSetDelayMS(emberAfPluginRf4ceZrc20LegacyCommandDiscoveryEventControl,
                                APLC_MAX_RESPONSE_WAIT_TIME_MS);
    emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0, true);
  }

  return status;
}

void emberAfPluginRf4ceZrc20LegacyCommandDiscoveryEventHandler(void)
{
  emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0, false);
  emberEventControlSetInactive(emberAfPluginRf4ceZrc20LegacyCommandDiscoveryEventControl);
  emberAfPluginRf4ceZrc20LegacyCommandDiscoveryCompleteCallback(EMBER_NO_RESPONSE,
                                                                NULL);
}

void emAfRf4ceZrc20IncomingMessageRecipient(uint8_t pairingIndex,
                                            uint16_t vendorId,
                                            EmberAfRf4ceZrcCommandCode commandCode,
                                            const uint8_t *message,
                                            uint8_t messageLength);

void emberAfPluginRf4ceProfileRemoteControl11MessageSentCallback(uint8_t pairingIndex,
                                                                 uint16_t vendorId,
                                                                 uint8_t messageTag,
                                                                 const uint8_t *message,
                                                                 uint8_t messageLength,
                                                                 EmberStatus status)
{
}

void emberAfPluginRf4ceProfileZrc20MessageSentCallback(uint8_t pairingIndex,
                                                       uint16_t vendorId,
                                                       uint8_t messageTag,
                                                       const uint8_t *message,
                                                       uint8_t messageLength,
                                                       EmberStatus status)
{
}

void emberAfPluginRf4ceProfileZrc20IncomingMessageCallback(uint8_t pairingIndex,
                                                           uint16_t vendorId,
                                                           EmberRf4ceTxOption txOptions,
                                                           const uint8_t *message,
                                                           uint8_t messageLength)
{
  EmberAfRf4ceZrcCommandCode commandCode;

  // Every ZRC command has a ZRC header.
  if (messageLength < ZRC_HEADER_LENGTH) {
    emberAfDebugPrintln("%p: %cX: %p %p",
                        "ZRC",
                        'R',
                        "Invalid",
                        "message");
    return;
  }

  commandCode = READBITS(message[ZRC_HEADER_FRAME_CONTROL_OFFSET],
                         ZRC_HEADER_FRAME_CONTROL_COMMAND_CODE_MASK);

  // Legacy discovery requests and responses are handled based on the role of
  // the local node.  Everything else is dispatched to the recipient code.
  if (commandCode == EMBER_AF_RF4CE_ZRC_COMMAND_COMMAND_DISCOVERY_REQUEST) {
    if (COMMAND_DISCOVERY_REQUEST_LENGTH <= messageLength) {
      EmberRf4cePairingTableEntry entry;
      if (emberAfRf4ceGetPairingTableEntry(pairingIndex, &entry)
          == EMBER_SUCCESS) {
            uint8_t *actionCodesPtr;
        uint8_t buffer[COMMAND_DISCOVERY_RESPONSE_LENGTH];
        uint8_t bufferLength = 0;
        buffer[bufferLength++] = EMBER_AF_RF4CE_ZRC_COMMAND_COMMAND_DISCOVERY_RESPONSE;
        buffer[bufferLength++] = 0; // reserved

        actionCodesPtr
          = emAfRf4ceZrcGetActionCodesAttributePointer((emberAfRf4cePairingTableEntryIsPairingInitiator(&entry)
                                                        ? EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_CODES_SUPPORTED_TX
                                                        : EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_CODES_SUPPORTED_RX),
                                                       EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC,
                                                       0xFF); // local attribute
        if (actionCodesPtr) {
          MEMCOPY(buffer + bufferLength, actionCodesPtr, ZRC_BITMASK_SIZE);
        } else {
          MEMSET(buffer + bufferLength, 0x00, ZRC_BITMASK_SIZE);
        }

        bufferLength += ZRC_BITMASK_SIZE;
        emberAfRf4ceSend(pairingIndex,
                         EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_1_1,
                         buffer,
                         bufferLength,
                         NULL); // message tag - unused
      }
    }
  } else if (commandCode == EMBER_AF_RF4CE_ZRC_COMMAND_COMMAND_DISCOVERY_RESPONSE) {
    if (emberEventControlGetActive(emberAfPluginRf4ceZrc20LegacyCommandDiscoveryEventControl)) {
      EmberRf4cePairingTableEntry entry;
      if (emberAfRf4ceGetPairingTableEntry(pairingIndex, &entry)
          == EMBER_SUCCESS) {
        EmberAfRf4ceZrcCommandsSupported commandsSupported;
        MEMCOPY(emberAfRf4ceZrcCommandsSupportedContents(&commandsSupported),
                message + COMMAND_DISCOVERY_RESPONSE_COMMANDS_SUPPORTED_OFFSET,
                EMBER_AF_RF4CE_ZRC_COMMANDS_SUPPORTED_SIZE);
        emAfRf4ceZrcWriteRemoteAttribute(pairingIndex,
                                         (emberAfRf4cePairingTableEntryIsPairingInitiator(&entry)
                                          ? EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_CODES_SUPPORTED_RX
                                          : EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_CODES_SUPPORTED_TX),
                                         EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC,
                                         emberAfRf4ceZrcCommandsSupportedContents(&commandsSupported));
        emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0, false);
        emberEventControlSetInactive(emberAfPluginRf4ceZrc20LegacyCommandDiscoveryEventControl);
        emberAfPluginRf4ceZrc20LegacyCommandDiscoveryCompleteCallback(EMBER_SUCCESS,
                                                                      &commandsSupported);
      }
    }
  } else {
    emAfRf4ceZrc20IncomingMessageRecipient(pairingIndex,
                                           vendorId,
                                           commandCode,
                                           message,
                                           messageLength);
  }
}

void emberAfPluginRf4ceProfileRemoteControl11IncomingMessageCallback(uint8_t pairingIndex,
                                                                     uint16_t vendorId,
                                                                     EmberRf4ceTxOption txOptions,
                                                                     const uint8_t *message,
                                                                     uint8_t messageLength)
{
  emberAfPluginRf4ceProfileZrc20IncomingMessageCallback(pairingIndex,
                                                        vendorId,
                                                        txOptions,
                                                        message,
                                                        messageLength);
}

void emAfRf4ceZrcIncomingClientNotification(EmberAfRf4ceGdpClientNotificationSubtype subType,
                                            const uint8_t *clientNotificationPayload,
                                            uint8_t clientNotificationPayloadLength)
{
  switch(subType)
  {
#if defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_IS_ACTION_MAPPING_CLIENT) || defined(EMBER_SCRIPTED_TEST)

  case CLIENT_NOTIFICATION_SUBTYPE_REQUEST_ACTION_MAPPING_NEGOTIATION:
    if (clientNotificationPayloadLength
        == CLIENT_NOTIFICATION_REQUEST_ACTION_MAPPING_NEGOTIATION_PAYLOAD_LENGTH) {
      emAfRf4ceZrcIncomingRequestActionMappingNegotiation();
    }
    break;
  case CLIENT_NOTIFICATION_SUBTYPE_REQUEST_SELECTIVE_ACTION_MAPPING_UPDATE:
    if (clientNotificationPayloadLength > 0
        && clientNotificationPayloadLength == (2*clientNotificationPayload[0] + 1)) {
      emAfRf4ceZrcIncomingRequestSelectiveActionMappingUpdate(clientNotificationPayload + 1,
                                                              clientNotificationPayloadLength);
    }
    break;

#endif // EMBER_AF_PLUGIN_RF4CE_ZRC20_IS_ACTION_MAPPING_CLIENT

#if defined(EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_ORIGINATOR) || defined(EMBER_SCRIPTED_TEST)

  case CLIENT_NOTIFICATION_SUBTYPE_REQUEST_HA_PULL:
    if (clientNotificationPayloadLength == CLIENT_NOTIFICATION_REQUEST_HA_PULL_PAYLOAD_LENGTH) {
      emAfRf4ceZrcIncomingRequestHomeAutomationPull(clientNotificationPayload[0],
                                                    clientNotificationPayload + CLIENT_NOTIFICATION_REQUEST_HA_PULL_HA_ATTRIBUTE_DIRTY_FLAGS_OFFSET);
    }
    break;

#endif // EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_ORIGINATOR
  }
}

uint8_t emAfRf4ceZrc20GetPeerZrcVersion(uint8_t pairingIndex)
{
  uint8_t version = ZRC_VERSION_NONE;
  EmberRf4cePairingTableEntry entry;
  if (emberAfRf4ceGetPairingTableEntry(pairingIndex, &entry) == EMBER_SUCCESS
      && (READBITS(entry.info, EMBER_RF4CE_PAIRING_TABLE_ENTRY_INFO_STATUS_MASK)
          == EMBER_RF4CE_PAIRING_TABLE_ENTRY_STATUS_ACTIVE)) {
    uint8_t i;
    for(i=0; i<entry.destProfileIdListLength; i++) {
      if (entry.destProfileIdList[i] == EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0) {
        version = ZRC_VERSION_2_0;
      }

      if (entry.destProfileIdList[i] == EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_1_1
          && version == ZRC_VERSION_NONE) {
        version = ZRC_VERSION_1_1;
      }
    }
  }

  return version;
}

#if defined(EMBER_SCRIPTED_TEST)

#include "stack/core/ember-stack.h"
#include "stack/core/parcel.h"

Parcel *makeActionRecordParcel(EmberAfRf4ceZrcActionRecord *record)
{
  if (record->actionVendorId == EMBER_RF4CE_NULL_VENDOR_ID) {
    if (record->actionPayloadLength == 0) { // no vendor ID, no action payload
      return makeMessage("1111",
                         ((record->actionType
                           << ACTION_RECORD_ACTION_CONTROL_ACTION_TYPE_OFFSET)
                          | (record->modifierBits
                             << ACTION_RECORD_ACTION_CONTROL_MODIFIER_BITS_OFFSET)),
                         record->actionPayloadLength,
                         record->actionBank,
                         record->actionCode);
    } else { // no vendor ID, has action payload
      return makeMessage("1111p",
                         ((record->actionType
                           << ACTION_RECORD_ACTION_CONTROL_ACTION_TYPE_OFFSET)
                          | (record->modifierBits
                             << ACTION_RECORD_ACTION_CONTROL_MODIFIER_BITS_OFFSET)),
                         record->actionPayloadLength,
                         record->actionBank,
                         record->actionCode,
                         makeMessage("s", record->actionPayload, record->actionPayloadLength));
    }
  } else {
    if (record->actionPayloadLength == 0) { // has vendor ID, no action payload
      return makeMessage("1111<2",
                         ((record->actionType
                           << ACTION_RECORD_ACTION_CONTROL_ACTION_TYPE_OFFSET)
                          | (record->modifierBits
                             << ACTION_RECORD_ACTION_CONTROL_MODIFIER_BITS_OFFSET)),
                         record->actionPayloadLength,
                         record->actionBank,
                         record->actionCode,
                         record->actionVendorId);
    } else { // has vendor ID, has action payload
      return makeMessage("1111<2p",
                         ((record->actionType
                           << ACTION_RECORD_ACTION_CONTROL_ACTION_TYPE_OFFSET)
                          | (record->modifierBits
                             << ACTION_RECORD_ACTION_CONTROL_MODIFIER_BITS_OFFSET)),
                         record->actionPayloadLength,
                         record->actionBank,
                         record->actionCode,
                         record->actionVendorId,
                         makeMessage("s", record->actionPayload, record->actionPayloadLength));
    }
  }
}

Parcel *makeActionsCommandParcel(EmberAfRf4ceZrcActionRecord *records,
                                 uint8_t recordsLength)
{
  Parcel *currParcel = NULL;

  if (recordsLength == 0) {
    return makeMessage("1",
                       EMBER_AF_RF4CE_ZRC_COMMAND_ACTIONS);
  } else {
    uint8_t i;
    for(i=0; i<recordsLength; i++) {
      Parcel *recordParcel =
          makeActionRecordParcel(&records[i]);
      if (currParcel == NULL) {
        currParcel = recordParcel;
      } else {
        currParcel = makeMessage("pp", currParcel, recordParcel);
      }
    }

    return makeMessage("1p",
                       EMBER_AF_RF4CE_ZRC_COMMAND_ACTIONS,
                       currParcel);
  }
}

Parcel *makeUserControlCommandParcel(uint8_t commandId,
                                     uint8_t commandCode,
                                     uint8_t *commandPayload,
                                     uint8_t commandPayloadLength)
{
  if (commandPayload != NULL && commandPayloadLength > 0) {
    return makeMessage("11p",
                       commandId,
                       commandCode,
                       makeMessage("s", commandPayload, commandPayloadLength));
  } else {
    return makeMessage("11",
                       commandId,
                       commandCode);
  }
}

Parcel *makeCommandDiscoveryRequestParcel(void)
{
  return makeMessage("11",
                     EMBER_AF_RF4CE_ZRC_COMMAND_COMMAND_DISCOVERY_REQUEST,
                     0x00);
}

Parcel *makeCommandDiscoveryResponseParcel(uint8_t *supportedCommands,
                                           uint8_t supportedCommandsLength)
{
  return makeMessage("11p",
                     EMBER_AF_RF4CE_ZRC_COMMAND_COMMAND_DISCOVERY_RESPONSE,
                     0x00,
                     makeMessage("s", supportedCommands, supportedCommandsLength));
}

Parcel *makeNotificationClientRequestHAPullParcel(uint8_t haInstanceId,
                                                  uint8_t *dirtyFlags)
{
  return makeMessage("111p",
                     (EMBER_AF_RF4CE_GDP_COMMAND_CLIENT_NOTIFICATION
                      | GDP_HEADER_FRAME_CONTROL_GDP_COMMAND_FRAME_MASK),
                     CLIENT_NOTIFICATION_SUBTYPE_REQUEST_HA_PULL,
                     haInstanceId,
                     makeMessage("s", dirtyFlags, CLIENT_NOTIFICATION_REQUEST_HA_PULL_HA_ATTRIBUTE_DIRTY_FLAGS_LENGTH));
}

Parcel *makeNotificationClientRequestActionMappingNegotiationParcel(void)
{
  return makeMessage("11",
                     (EMBER_AF_RF4CE_GDP_COMMAND_CLIENT_NOTIFICATION
                      | GDP_HEADER_FRAME_CONTROL_GDP_COMMAND_FRAME_MASK),
                      CLIENT_NOTIFICATION_SUBTYPE_REQUEST_ACTION_MAPPING_NEGOTIATION);
}

Parcel *makeNotificationClientRequestSelectiveActionMappingUpdateParcel(uint8_t actionsListLength,
                                                                        uint16_t *actionsList)
{
  Parcel *currParcel = NULL;
  uint8_t i;

  for(i=0; i<actionsListLength; i++) {
    Parcel *actionEntry =  makeMessage("<2", actionsList[i]);

    if (currParcel == NULL) {
      currParcel = actionEntry;
    } else {
      currParcel = makeMessage("pp", currParcel, actionEntry);
    }
  }

  return makeMessage("111p",
                     (EMBER_AF_RF4CE_GDP_COMMAND_CLIENT_NOTIFICATION
                      | GDP_HEADER_FRAME_CONTROL_GDP_COMMAND_FRAME_MASK),
                      CLIENT_NOTIFICATION_SUBTYPE_REQUEST_SELECTIVE_ACTION_MAPPING_UPDATE,
                      actionsListLength,
                      currParcel);
}

#endif //EMBER_SCRIPTED_TEST
