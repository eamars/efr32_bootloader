// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "rf4ce-profile.h"
#include "rf4ce-profile-internal.h"

#ifdef EMBER_AF_LEGACY_CLI
  #error The RF4CE Profile plugin is not compatible with the legacy CLI.
#endif

#if defined(EMBER_AF_GENERATE_CLI) || defined(EMBER_AF_API_COMMAND_INTERPRETER2)

// plugin rf4ce-profile start
void emberAfPluginRf4ceProfileStartCommand(void)
{
  EmberStatus status = emberAfRf4ceStart();
  emberAfAppPrintln("%p 0x%x", "start", status);
}

// plugin rf4ce-profile stop
void emberAfPluginRf4ceProfileStopCommand(void)
{
  EmberStatus status = emberAfRf4ceStop();
  emberAfAppPrintln("%p 0x%x", "stop", status);
}

// plugin rf4ce-profile pair <channel:1> <pan id:2> <eui64:8> <key exchange transfer count:1>
void emberAfPluginRf4ceProfilePairCommand(void)
{
  EmberStatus status;
  EmberEUI64 eui64;
  uint8_t channel = (uint8_t)emberUnsignedCommandArgument(0);
  EmberPanId panId = (EmberPanId)emberUnsignedCommandArgument(1);
  uint8_t keyExchangeTransferCount = (uint8_t)emberUnsignedCommandArgument(3);
  emberCopyBigEndianEui64Argument(2, eui64);
  status = emberAfRf4cePair(channel, panId, eui64, keyExchangeTransferCount, NULL);
  emberAfAppPrintln("%p 0x%x", "pair", status);
}

// plugin rf4ce-profile send <pairing index:1> <profile id:1> <message:n>
void emberAfPluginRf4ceProfileSendCommand(void)
{
  EmberStatus status;
  uint8_t pairingIndex = (uint8_t)emberUnsignedCommandArgument(0);
  uint8_t profileId = (uint8_t)emberUnsignedCommandArgument(1);
  uint8_t messageLength;
  uint8_t *message = emberStringCommandArgument(2, &messageLength);
  status = emberAfRf4ceSend(pairingIndex,
                            profileId,
                            message,
                            messageLength,
                            NULL); // message tag - unused
  emberAfAppPrintln("%p 0x%x", "send", status);
}

// plugin rf4ce-profile vendor <pairing index:1> <profile id:1> <vendor id:2> <message:n>
void emberAfPluginRf4ceProfileVendorCommand(void)
{
  EmberStatus status;
  uint8_t pairingIndex = (uint8_t)emberUnsignedCommandArgument(0);
  uint8_t profileId = (uint8_t)emberUnsignedCommandArgument(1);
  uint16_t vendorId = (uint16_t)emberUnsignedCommandArgument(2);
  uint8_t messageLength;
  uint8_t *message = emberStringCommandArgument(3, &messageLength);
  status = emberAfRf4ceSendVendorSpecific(pairingIndex,
                                          profileId,
                                          vendorId,
                                          message,
                                          messageLength,
                                          NULL); // message tag - unused
  emberAfAppPrintln("%p 0x%x", "vendor", status);
}

// plugin rf4ce-profile unpair <pairing index:1>
void emberAfPluginRf4ceProfileUnpairCommand(void)
{
  EmberStatus status;
  uint8_t pairingIndex = (uint8_t)emberUnsignedCommandArgument(0);
  status = emberAfRf4ceUnpair(pairingIndex);
  emberAfAppPrintln("%p 0x%x", "unpair", status);
}

// plugin rf4ce-profile power <duty cycle:4> <active period:4>
void emberAfPluginRf4ceProfilePowerCommand(void)
{
  EmberStatus status;
  uint32_t dutyCycle = emberUnsignedCommandArgument(0);
  uint32_t activePeriod = emberUnsignedCommandArgument(1);
  status = emberAfRf4ceSetPowerSavingParameters(dutyCycle, activePeriod);
  emberAfAppPrintln("%p 0x%x", "power", status);
}

// plugin rf4ce-profile rx <profile id:1> <enable:1>
void emberAfPluginRf4ceProfileRxCommand(void)
{
  EmberStatus status;
  EmberAfRf4ceProfileId profileId = (EmberAfRf4ceProfileId)emberUnsignedCommandArgument(0);
  bool enable = (bool)emberUnsignedCommandArgument(1);
  status = emberAfRf4ceRxEnable(profileId, enable);
  emberAfAppPrintln("%p 0x%x", "rx", status);
}

// plugin rf4ce-profile agility <rssi window size:1> <channel change reads:1> <rssi threshold:1> <read interval:2> <read duration:1>
void emberAfPluginRf4ceProfileAgilityCommand(void)
{
  EmberStatus status;
  uint8_t rssiWindowSize = (uint8_t)emberUnsignedCommandArgument(0);
  uint8_t channelChangeReads = (uint8_t)emberUnsignedCommandArgument(1);
  int8_t rssiThreshold = (int8_t)emberSignedCommandArgument(2);
  uint16_t readInterval = (uint16_t)emberUnsignedCommandArgument(3);
  uint8_t readDuration = (uint8_t)emberUnsignedCommandArgument(4);
  status = emberAfRf4ceSetFrequencyAgilityParameters(rssiWindowSize,
                                                     channelChangeReads,
                                                     rssiThreshold,
                                                     readInterval,
                                                     readDuration);
  emberAfAppPrintln("%p 0x%x", "agility", status);
}

// plugin rf4ce-profile lqi <threshold:1>
void emberAfPluginRf4ceProfileLqiCommand(void)
{
  EmberStatus status;
  uint8_t threshold = (uint8_t)emberUnsignedCommandArgument(0);
  status = emberAfRf4ceSetDiscoveryLqiThreshold(threshold);
  emberAfAppPrintln("%p 0x%x", "lqi", status);
}

// plugin rf4ce-profile print
void emberAfPluginRf4ceProfilePrintCommand(void)
{
  uint8_t i;
  uint8_t pairings = 0;

  emberAfAppPrintln("#   status pan    chan local  remote remote              frame      key");
  for (i = 0; i < EMBER_RF4CE_PAIRING_TABLE_SIZE; i++) {
    EmberRf4cePairingTableEntry entry;
    EmberStatus status = emberAfRf4ceGetPairingTableEntry(i, &entry);
    if (status == EMBER_SUCCESS) {
      if (!emberAfRf4cePairingTableEntryIsUnused(&entry)) {
        emberAfAppPrint("%x: ", i);
        if (emberAfRf4cePairingTableEntryIsProvisional(&entry)) {
          emberAfAppPrint("PROVIS");
        } else if (emberAfRf4cePairingTableEntryIsActive(&entry)) {
          emberAfAppPrint("ACTIVE");
        } else {
          emberAfAppPrint("??????");
        }
        emberAfAppPrint(" 0x%2x %d   0x%2x 0x%2x ",
                        entry.destPanId,
                        entry.channel,
                        entry.sourceNodeId,
                        entry.destNodeId);
        emberAfAppDebugExec(emberAfPrintBigEndianEui64(entry.destLongId));
        emberAfAppPrint(" 0x%4x ", entry.frameCounter);
        if (emberAfRf4cePairingTableEntryHasLinkKey(&entry)) {
          emberAfAppDebugExec(emberAfPrintZigbeeKey(emberKeyContents(&entry.securityLinkKey)));
        } else {
          emberAfAppPrintln("");
        }
        pairings++;
      }
    } else {
      emberAfAppPrintln("%x: error: 0x%x", i, status);
    }
  }
  emberAfAppPrintln("%d of %d pairings used",
                    pairings,
                    EMBER_RF4CE_PAIRING_TABLE_SIZE);

  emberAfAppPrintln("profile rx");
  for (i = 0; i < EMBER_AF_RF4CE_PROFILE_ID_COUNT; i++) {
    if (emAfRf4ceProfileIds[i] != EMBER_AF_RF4CE_PROFILE_WILDCARD) {
      emberAfAppPrintln("0x%x    %p",
                        emAfRf4ceProfileIds[i],
                        (emAfRf4ceRxOnWhenIdleProfileStates[i] ? "ON" : "off"));
    }
  }
  emberAfAppPrint("0x%x    %p", EMBER_AF_RF4CE_PROFILE_WILDCARD, "");
  if (emAfRf4cePowerSavingState.dutyCycleMs == 0
      || emAfRf4cePowerSavingState.activePeriodMs == 0) {
    emberAfAppPrintln(emAfRf4cePowerSavingState.dutyCycleMs == 0 ? "ON" : "off");
  } else {
    emberAfAppPrintln("duty (%d/%d ms)",
                      emAfRf4cePowerSavingState.activePeriodMs,
                      emAfRf4cePowerSavingState.dutyCycleMs);
  }
}

#endif
