// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "app/framework/plugin/rf4ce-profile/rf4ce-profile.h"
#include "app/framework/plugin/rf4ce-zrc20/rf4ce-zrc20.h"
#include "rf4ce-zrc20-action-mapping-client.h"

#ifdef EMBER_AF_LEGACY_CLI
  #error The RF4CE ZRC2.0 Action Mapping Client plugin is not compatible with the legacy CLI.
#endif

// Some commands are split into multiple commands. These variables are used to
// hold the data until we get all and call the get/set command.
static EmberAfRf4ceZrcActionMapping actionMapping;
static uint8_t actionData[32];
static uint8_t irCode[32];


void emberAfPluginRf4ceZrc20ActionMappingClientClearAllCommand(void)
{
  emberAfRf4ceZrc20ActionMappingClientClearAllActionMappings();
  emberAfAppPrintln("%p 0x%x", "clear-all", 0x00);
}

void emberAfPluginRf4ceZrc20ActionMappingClientClearCommand(void)
{
  uint8_t pairingIndex = emberUnsignedCommandArgument(0);
  EmberAfRf4ceDeviceType actionDeviceType
    = (EmberAfRf4ceDeviceType)emberUnsignedCommandArgument(1);
  EmberAfRf4ceZrcActionBank actionBank
    = (EmberAfRf4ceZrcActionBank)emberUnsignedCommandArgument(2);
  EmberAfRf4ceZrcActionCode actionCode
    = (EmberAfRf4ceZrcActionCode)emberUnsignedCommandArgument(3);
  EmberStatus status
    = emberAfRf4ceZrc20ActionMappingClientClearActionMapping(pairingIndex,
                                                             actionDeviceType,
                                                             actionBank,
                                                             actionCode);
  emberAfAppPrintln("%p 0x%x", "clear", status);
}

void emberAfPluginRf4ceZrc20ActionMappingClientGetCommand(void)
{
  uint8_t pairingIndex = emberUnsignedCommandArgument(0);
  EmberAfRf4ceDeviceType actionDeviceType
    = (EmberAfRf4ceDeviceType)emberUnsignedCommandArgument(1);
  EmberAfRf4ceZrcActionBank actionBank
    = (EmberAfRf4ceZrcActionBank)emberUnsignedCommandArgument(2);
  EmberAfRf4ceZrcActionCode actionCode
    = (EmberAfRf4ceZrcActionCode)emberUnsignedCommandArgument(3);
  EmberStatus status;

  MEMSET(&actionMapping, 0x00, sizeof(actionMapping));
  if (EMBER_SUCCESS ==
      (status = emberAfRf4ceZrc20ActionMappingClientGetActionMapping(pairingIndex,
                                                                     actionDeviceType,
                                                                     actionBank,
                                                                     actionCode,
                                                                     &actionMapping))) {
    emberAfAppPrintln("%x", actionMapping.mappingFlags);
    if (emberAfRf4ceZrc20ActionMappingEntryHasRfDescriptor(&actionMapping)) {
      emberAfAppPrintln("%x %x",
                        actionMapping.rfConfig,
                        actionMapping.rf4ceTxOptions);
      emberAfAppPrintBuffer(actionMapping.actionData,
                            actionMapping.actionDataLength,
                            true);
      emberAfAppPrintln("");
    }
    if (emberAfRf4ceZrc20ActionMappingEntryHasIrDescriptor(&actionMapping)) {
      emberAfAppPrint("%x", actionMapping.irConfig);
      if (emberAfRf4ceZrc20ActionMappingEntryHasIrVendorId(&actionMapping)) {
          emberAfAppPrintln(" %2x", actionMapping.irVendorId);
      } else {
        emberAfAppPrintln("");
      }
      emberAfAppPrintBuffer(actionMapping.irCode,
                            actionMapping.irCodeLength,
                            true);
      emberAfAppPrintln("");
    }
  }
  emberAfAppPrintln("%p 0x%x", "get", status);
}

void emberAfPluginRf4ceZrc20ActionMappingClientSetFlagsCommand(void)
{
  actionMapping.mappingFlags = emberUnsignedCommandArgument(0);

}

void emberAfPluginRf4ceZrc20ActionMappingClientSetRfCommand(void)
{
  actionMapping.rfConfig = emberUnsignedCommandArgument(0);
  actionMapping.rf4ceTxOptions = emberUnsignedCommandArgument(1);
  actionMapping.actionDataLength = emberCopyStringArgument(2,
                                                           actionData,
                                                           32,
                                                           false);
  actionMapping.actionData = actionData;
}

void emberAfPluginRf4ceZrc20ActionMappingClientSetIrCommand(void)
{
  actionMapping.irConfig = emberUnsignedCommandArgument(0);
  actionMapping.irVendorId = emberUnsignedCommandArgument(1);
  actionMapping.irCodeLength = emberCopyStringArgument(2,
                                                       irCode,
                                                       32,
                                                       false);
  actionMapping.irCode = irCode;
}

void emberAfPluginRf4ceZrc20ActionMappingClientSetCommand(void)
{
  uint8_t pairingIndex = emberUnsignedCommandArgument(0);
  EmberAfRf4ceDeviceType actionDeviceType
    = (EmberAfRf4ceDeviceType)emberUnsignedCommandArgument(1);
  EmberAfRf4ceZrcActionBank actionBank
    = (EmberAfRf4ceZrcActionBank)emberUnsignedCommandArgument(2);
  EmberAfRf4ceZrcActionCode actionCode
    = (EmberAfRf4ceZrcActionCode)emberUnsignedCommandArgument(3);
  EmberStatus status
    = emberAfRf4ceZrc20ActionMappingClientSetActionMapping(pairingIndex,
                                                           actionDeviceType,
                                                           actionBank,
                                                           actionCode,
                                                           &actionMapping);
  emberAfAppPrintln("%p 0x%x", "set", status);
}


