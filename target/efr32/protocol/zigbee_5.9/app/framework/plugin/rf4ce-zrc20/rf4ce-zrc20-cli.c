// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "rf4ce-zrc20.h"

#ifdef EMBER_AF_LEGACY_CLI
  #error The RF4CE ZRC2.0 plugin is not compatible with the legacy CLI.
#endif

#if defined(EMBER_AF_GENERATE_CLI) || defined(EMBER_AF_API_COMMAND_INTERPRETER2)

// plugin rf4ce-zrc20 bind <search dev type:1>
void emberAfPluginRf4ceZrc20BindCommand(void)
{
  EmberAfRf4ceDeviceType searchDevType = (EmberAfRf4ceDeviceType)emberUnsignedCommandArgument(0);
  EmberStatus status = emberAfRf4ceZrc20Bind(searchDevType);
  emberAfAppPrintln("%p 0x%x", "bind", status);
}

// plugin rf4ce-zrc20 proxy-bind <pan id:2> <ieee:8>
void emberAfPluginRf4ceZrc20ProxyBindCommand(void)
{
  EmberStatus status;
  EmberEUI64 ieeeAddr;
  EmberPanId panId = (EmberPanId)emberUnsignedCommandArgument(0);
  emberCopyBigEndianEui64Argument(1, ieeeAddr);
  status = emberAfRf4ceZrc20ProxyBind(panId, ieeeAddr);
  emberAfAppPrintln("%p 0x%x", "proxy", status);
}

// plugin rf4ce-zrc20 start <pairing index:1> <action bank:1> <action code:1> <action modifier:1> <action vendor id:2> <action data:n> <atomic:1>
void emberAfPluginRf4ceZrc20StartCommand(void)
{
  EmberStatus status;
  uint8_t pairingIndex = (uint8_t)emberUnsignedCommandArgument(0);
  EmberAfRf4ceZrcActionBank actionBank = (EmberAfRf4ceZrcActionBank)emberUnsignedCommandArgument(1);
  EmberAfRf4ceZrcActionCode actionCode = (EmberAfRf4ceZrcActionCode)emberUnsignedCommandArgument(2);
  EmberAfRf4ceZrcModifierBit actionModifier = (EmberAfRf4ceZrcModifierBit)emberUnsignedCommandArgument(3);
  uint16_t actionVendorId = (uint16_t)emberUnsignedCommandArgument(4);
  uint8_t actionDataLength;
  uint8_t *actionData = emberStringCommandArgument(5, &actionDataLength);
  bool atomic = (bool)emberUnsignedCommandArgument(6);
  status = emberAfRf4ceZrc20ActionStart(pairingIndex,
                                        actionBank,
                                        actionCode,
                                        actionModifier,
                                        actionVendorId,
                                        actionData,
                                        actionDataLength,
                                        atomic);
  emberAfAppPrintln("%p 0x%x", "start", status);
}

// plugin rf4ce-zrc20 stop <pairing index:1> <action bank:1> <action code:1> <action modifier:1> <action vendor id:2>
void emberAfPluginRf4ceZrc20StopCommand(void)
{
  EmberStatus status;
  uint8_t pairingIndex = (uint8_t)emberUnsignedCommandArgument(0);
  EmberAfRf4ceZrcActionBank actionBank = (EmberAfRf4ceZrcActionBank)emberUnsignedCommandArgument(1);
  EmberAfRf4ceZrcActionCode actionCode = (EmberAfRf4ceZrcActionCode)emberUnsignedCommandArgument(2);
  EmberAfRf4ceZrcModifierBit actionModifier = (EmberAfRf4ceZrcModifierBit)emberUnsignedCommandArgument(3);
  uint16_t actionVendorId = (uint16_t)emberUnsignedCommandArgument(4);
  status = emberAfRf4ceZrc20ActionStop(pairingIndex,
                                       actionBank,
                                       actionCode,
                                       actionModifier,
                                       actionVendorId);
  emberAfAppPrintln("%p 0x%x", "stop", status);
}

#endif
