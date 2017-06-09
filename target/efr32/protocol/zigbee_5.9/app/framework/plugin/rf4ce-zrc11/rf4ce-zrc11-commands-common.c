// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "app/framework/plugin/rf4ce-profile/rf4ce-profile.h"
#include "rf4ce-zrc11.h"
#include "rf4ce-zrc11-internal.h"

static PGM uint8_t minimumMessageLengths[] = {
  0, // placeholder
  USER_CONTROL_PRESSED_LENGTH,
  USER_CONTROL_REPEATED_1_0_LENGTH,
  USER_CONTROL_RELEASED_1_0_LENGTH,
  COMMAND_DISCOVERY_REQUEST_LENGTH,
  COMMAND_DISCOVERY_RESPONSE_LENGTH,
};

uint8_t emAfRf4ceZrc11Buffer[EMBER_AF_PLUGIN_RF4CE_ZRC11_MAXIMUM_PAYLOAD_LENGTH];
uint8_t emAfRf4ceZrc11BufferLength;

EmberEventControl emberAfPluginRf4ceZrc11CommandDiscoveryEventControl;
static uint8_t commandDiscoveryPairingIndex = 0xFF;

EmberStatus emberAfRf4ceZrc11CommandDiscoveryRequest(uint8_t pairingIndex)
{
  EmberStatus status = EMBER_INVALID_CALL;
  if (!emberEventControlGetActive(emberAfPluginRf4ceZrc11CommandDiscoveryEventControl)) {
    emAfRf4ceZrc11BufferLength = 0;
    emAfRf4ceZrc11Buffer[emAfRf4ceZrc11BufferLength++]
      = EMBER_AF_RF4CE_ZRC_COMMAND_COMMAND_DISCOVERY_REQUEST;
    emAfRf4ceZrc11Buffer[emAfRf4ceZrc11BufferLength++] = 0; // reserved
    status = emAfRf4ceZrc11Send(pairingIndex);
    if (status == EMBER_SUCCESS) {
      commandDiscoveryPairingIndex = pairingIndex;
      emberEventControlSetDelayMS(emberAfPluginRf4ceZrc11CommandDiscoveryEventControl,
                                  APLC_MAX_RESPONSE_WAIT_TIME_MS);
      emAfRf4ceZrc11RxEnable();
    }
  }
  return status;
}

void emberAfPluginRf4ceZrc11InitCallback(void)
{
  emAfRf4ceZrc11InitOriginator();
  emAfRf4ceZrc11InitRecipient();
}

void emberAfPluginRf4ceProfileRemoteControl11MessageSentCallback(uint8_t pairingIndex,
                                                                 uint16_t vendorId,
                                                                 uint8_t messageTag,
                                                                 const uint8_t *message,
                                                                 uint8_t messageLength,
                                                                 EmberStatus status)
{
  // TODO: Handle success and failure of outgoing messages.
}

void emberAfPluginRf4ceProfileRemoteControl11IncomingMessageCallback(uint8_t pairingIndex,
                                                                     uint16_t vendorId,
                                                                     EmberRf4ceTxOption txOptions,
                                                                     const uint8_t *message,
                                                                     uint8_t messageLength)
{
  EmberAfRf4ceZrcCommandCode commandCode;
  uint8_t frameControl;

  // Every ZRC 1.0 or 1.1 command has a ZRC header.
  if (messageLength < ZRC_OVERHEAD) {
    emberAfDebugPrintln("%p: %cX: %p %p",
                        "ZRC",
                        'R',
                        "Invalid",
                        "message");
    return;
  }

  // All eight bits of the header make up the frame control field, the lowest
  // five bits of which contain the command code.
  frameControl = message[0];
  commandCode = frameControl & ZRC_FRAME_CONTROL_COMMAND_CODE_MASK;

  if (commandCode < COMMAND_CODE_MINIMUM
      || COMMAND_CODE_MAXIMUM < commandCode) {
    emberAfDebugPrint("%p: %cX: %p %p",
                      "ZRC",
                      'R',
                      "Invalid",
                      "command code");
    emberAfDebugPrintln(": 0x%x", commandCode);
    return;
  }

  if (messageLength < minimumMessageLengths[commandCode]) {
    emberAfDebugPrint("%p: %cX: %p %p",
                      "ZRC",
                      'R',
                      "Invalid",
                      "command length");
    emberAfDebugPrintln(": %d", messageLength);
    return;
  }

  switch (commandCode) {
  case EMBER_AF_RF4CE_ZRC_COMMAND_USER_CONTROL_PRESSED:
  case EMBER_AF_RF4CE_ZRC_COMMAND_USER_CONTROL_REPEATED:
  case EMBER_AF_RF4CE_ZRC_COMMAND_USER_CONTROL_RELEASED:
    emAfRf4ceZrc11IncomingUserControl(pairingIndex,
                                      commandCode,
                                      message,
                                      messageLength);
    break;

  case EMBER_AF_RF4CE_ZRC_COMMAND_COMMAND_DISCOVERY_REQUEST:
    {
      EmberRf4cePairingTableEntry entry;
      EmberStatus status = emberAfRf4ceGetPairingTableEntry(pairingIndex,
                                                            &entry);
      if (status != EMBER_SUCCESS) {
        break;
      }

      emAfRf4ceZrc11BufferLength = 0;
      emAfRf4ceZrc11Buffer[emAfRf4ceZrc11BufferLength++]
        = EMBER_AF_RF4CE_ZRC_COMMAND_COMMAND_DISCOVERY_RESPONSE;
      emAfRf4ceZrc11Buffer[emAfRf4ceZrc11BufferLength++] = 0; // reserved

      MEMSET(emAfRf4ceZrc11Buffer + emAfRf4ceZrc11BufferLength,
             0,
             COMMANDS_SUPPORTED_LENGTH);

      if (emberAfRf4cePairingTableEntryIsPairingInitiator(&entry)) {
#ifdef EMBER_AF_RF4CE_ZRC_IS_ORIGINATOR
        uint8_t commandsSupported[]
          = EMBER_AF_RF4CE_ZRC_USER_CONTROL_COMMAND_CODES_TX;
        MEMCOPY(emAfRf4ceZrc11Buffer + emAfRf4ceZrc11BufferLength,
                commandsSupported,
                COMMANDS_SUPPORTED_LENGTH);
#endif
      } else {
#ifdef EMBER_AF_RF4CE_ZRC_IS_RECIPIENT
        uint8_t commandsSupported[]
          = EMBER_AF_RF4CE_ZRC_USER_CONTROL_COMMAND_CODES_RX;
        MEMCOPY(emAfRf4ceZrc11Buffer + emAfRf4ceZrc11BufferLength,
                commandsSupported,
                COMMANDS_SUPPORTED_LENGTH);
#endif
      }

      emAfRf4ceZrc11BufferLength += COMMANDS_SUPPORTED_LENGTH;
      emAfRf4ceZrc11Send(pairingIndex);
      break;
    }

  case EMBER_AF_RF4CE_ZRC_COMMAND_COMMAND_DISCOVERY_RESPONSE:
    if (pairingIndex == commandDiscoveryPairingIndex) {
      // Command Discovery Response is the same in ZRC 1.0 and 1.1.
      EmberAfRf4ceZrcCommandsSupported commandsSupported;
      MEMCOPY(emberAfRf4ceZrcCommandsSupportedContents(&commandsSupported),
              message + COMMAND_DISCOVERY_RESPONSE_COMMANDS_SUPPORTED_OFFSET,
              EMBER_AF_RF4CE_ZRC_COMMANDS_SUPPORTED_SIZE);
      commandDiscoveryPairingIndex = 0xFF;
      emberEventControlSetInactive(emberAfPluginRf4ceZrc11CommandDiscoveryEventControl);
      emAfRf4ceZrc11RxEnable();
      emberAfPluginRf4ceZrc11CommandDiscoveryResponseCallback(EMBER_SUCCESS,
                                                              &commandsSupported);
      break;
    }
  }
}

void emberAfPluginRf4ceZrc11CommandDiscoveryEventHandler(void)
{
  commandDiscoveryPairingIndex = 0xFF;
  emberEventControlSetInactive(emberAfPluginRf4ceZrc11CommandDiscoveryEventControl);
  emAfRf4ceZrc11RxEnable();
  emberAfPluginRf4ceZrc11CommandDiscoveryResponseCallback(EMBER_NO_RESPONSE,
                                                          NULL);
}

EmberStatus emAfRf4ceZrc11Send(uint8_t pairingIndex)
{
  return emberAfRf4ceSend(pairingIndex,
                          EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_1_1,
                          emAfRf4ceZrc11Buffer,
                          emAfRf4ceZrc11BufferLength,
                          NULL); // message tag - unused
}
