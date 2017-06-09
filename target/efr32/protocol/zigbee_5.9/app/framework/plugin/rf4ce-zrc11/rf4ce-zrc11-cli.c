// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "rf4ce-zrc11.h"

#ifdef EMBER_AF_LEGACY_CLI
  #error The RF4CE ZRC1.1 plugin is not compatible with the legacy CLI.
#endif

#if defined(EMBER_AF_GENERATE_CLI) || defined(EMBER_AF_API_COMMAND_INTERPRETER2)

// plugin rf4ce-zrc11 pair originator <pan id:2> <node id:2> <search dev type:1>
// plugin rf4ce-zrc11 pair recipient
void emberAfPluginRf4ceZrc11PairCommand(void)
{
  EmberStatus status;
  bool originator = (emberStringCommandArgument(-1, NULL)[0] == 'o');
  if (originator) {
    EmberPanId panId = (EmberPanId)emberUnsignedCommandArgument(0);
    EmberNodeId nodeId = (EmberNodeId)emberUnsignedCommandArgument(1);
    EmberAfRf4ceDeviceType searchDevType
      = (EmberAfRf4ceDeviceType)emberUnsignedCommandArgument(2);
    status = emberAfRf4ceZrc11Discovery(panId, nodeId, searchDevType);
  } else {
    status = emberAfRf4ceZrc11EnableAutoDiscoveryResponse();
  }
  emberAfAppPrintln("%p 0x%x", "pair", status);
}

// plugin rf4ce-zrc11 press <pairing index:1> <rc command code:1> <rc command payload:n> <atomic:1>
void emberAfPluginRf4ceZrc11PressCommand(void)
{
  EmberStatus status;
  uint8_t pairingIndex = (uint8_t)emberUnsignedCommandArgument(0);
  EmberAfRf4ceUserControlCode rcCommandCode
    = (EmberAfRf4ceUserControlCode)emberUnsignedCommandArgument(1);
  uint8_t rcCommandPayloadLength;
  uint8_t *rcCommandPayload
    = emberStringCommandArgument(2, &rcCommandPayloadLength);
  bool atomic = (bool)emberUnsignedCommandArgument(3);
  status = emberAfRf4ceZrc11UserControlPress(pairingIndex,
                                             rcCommandCode,
                                             rcCommandPayload,
                                             rcCommandPayloadLength,
                                             atomic);
  emberAfAppPrintln("%p 0x%x", "press", status);
}

// plugin rf4ce-zrc11 release <pairing index:1> <rc command code:1>
void emberAfPluginRf4ceZrc11ReleaseCommand(void)
{
  EmberStatus status;
  uint8_t pairingIndex = (uint8_t)emberUnsignedCommandArgument(0);
  EmberAfRf4ceUserControlCode rcCommandCode
    = (EmberAfRf4ceUserControlCode)emberUnsignedCommandArgument(1);
  status = emberAfRf4ceZrc11UserControlRelease(pairingIndex, rcCommandCode);
  emberAfAppPrintln("%p 0x%x", "release", status);
}

// plugin rf4ce-zrc11 discovery <pairing index:1>
void emberAfPluginRf4ceZrc11DiscoveryCommand(void)
{
  EmberStatus status;
  uint8_t pairingIndex = (uint8_t)emberUnsignedCommandArgument(0);
  status = emberAfRf4ceZrc11CommandDiscoveryRequest(pairingIndex);
  emberAfAppPrintln("%p 0x%x", "discovery", status);
}

#endif
