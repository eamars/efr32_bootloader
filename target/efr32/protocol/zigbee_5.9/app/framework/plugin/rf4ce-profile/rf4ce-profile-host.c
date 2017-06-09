// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "rf4ce-profile.h"
#include "rf4ce-profile-internal.h"

void emberAfPluginRf4ceProfileNcpInitCallback(bool memoryAllocation)
{
  if (!memoryAllocation) {
    // Enable built-in support for supported profiles.
    ezspSetPolicy(EZSP_RF4CE_DISCOVERY_AND_PAIRING_PROFILE_BEHAVIOR_POLICY,
                  EZSP_RF4CE_DISCOVERY_AND_PAIRING_PROFILE_BEHAVIOR_ENABLED);

    // Push the list of supported device types to the NCP.
    ezspSetValue(EZSP_VALUE_RF4CE_SUPPORTED_DEVICE_TYPES_LIST,
                 emberAfRf4ceDeviceTypeListLength(emAfRf4ceApplicationInfo.capabilities),
                 emAfRf4ceApplicationInfo.deviceTypeList);

    // Push the list of supported profiles to the NCP.
    ezspSetValue(EZSP_VALUE_RF4CE_SUPPORTED_PROFILES_LIST,
                 emberAfRf4ceProfileIdListLength(emAfRf4ceApplicationInfo.capabilities),
                 emAfRf4ceApplicationInfo.profileIdList);
  }
}

uint8_t emAfRf4ceGetBaseChannel(void)
{
  uint8_t value;
  uint8_t valueLength;

  ezspGetValue(EZSP_VALUE_RF4CE_BASE_CHANNEL, &valueLength, &value);

  return value;
}

void ezspRf4ceDiscoveryRequestHandler(EmberEUI64 ieeeAddr,
                                      uint8_t nodeCapabilities,
                                      EmberRf4ceVendorInfo *vendorInfo,
                                      EmberRf4ceApplicationInfo *appInfo,
                                      uint8_t searchDevType,
                                      uint8_t rxLinkQuality)
{
  emAfRf4ceDiscoveryRequestHandler(ieeeAddr,
                                   nodeCapabilities,
                                   vendorInfo,
                                   appInfo,
                                   searchDevType,
                                   rxLinkQuality);
}

void ezspRf4ceDiscoveryResponseHandler(bool atCapacity,
                                       uint8_t channel,
                                       EmberPanId panId,
                                       EmberEUI64 ieeeAddr,
                                       uint8_t nodeCapabilities,
                                       EmberRf4ceVendorInfo *vendorInfo,
                                       EmberRf4ceApplicationInfo *appInfo,
                                       uint8_t rxLinkQuality,
                                       uint8_t discRequestLqi)
{
  emAfRf4ceDiscoveryResponseHandler(atCapacity,
                                    channel,
                                    panId,
                                    ieeeAddr,
                                    nodeCapabilities,
                                    vendorInfo,
                                    appInfo,
                                    rxLinkQuality,
                                    discRequestLqi);
}

void ezspRf4ceDiscoveryCompleteHandler(EmberStatus status)
{
  emAfRf4ceDiscoveryCompleteHandler(status);
}

void ezspRf4ceAutoDiscoveryResponseCompleteHandler(EmberStatus status,
                                                   EmberEUI64 srcIeeeAddr,
                                                   uint8_t nodeCapabilities,
                                                   EmberRf4ceVendorInfo *vendorInfo,
                                                   EmberRf4ceApplicationInfo *appInfo,
                                                   uint8_t searchDevType)
{
  emAfRf4ceAutoDiscoveryResponseCompleteHandler(status,
                                                srcIeeeAddr,
                                                nodeCapabilities,
                                                vendorInfo,
                                                appInfo,
                                                searchDevType);
}

void ezspRf4cePairRequestHandler(EmberStatus status,
                                 uint8_t pairingIndex,
                                 EmberEUI64 srcIeeeAddr,
                                 uint8_t nodeCapabilities,
                                 EmberRf4ceVendorInfo *vendorInfo,
                                 EmberRf4ceApplicationInfo *appInfo,
                                 uint8_t keyExchangeTransferCount)
{
  emAfRf4cePairRequestHandler(status,
                              pairingIndex,
                              srcIeeeAddr,
                              nodeCapabilities,
                              vendorInfo,
                              appInfo,
                              keyExchangeTransferCount);
}

void ezspRf4cePairCompleteHandler(EmberStatus status,
                                  uint8_t pairingIndex,
                                  EmberRf4ceVendorInfo *vendorInfo,
                                  EmberRf4ceApplicationInfo *appInfo)
{
  emAfRf4cePairCompleteHandler(status, pairingIndex, vendorInfo, appInfo);
}

void ezspRf4ceMessageSentHandler(EmberStatus status,
                                 uint8_t pairingIndex,
                                 uint8_t txOptions,
                                 uint8_t profileId,
                                 uint16_t vendorId,
                                 uint8_t messageTag,
                                 uint8_t messageLength,
                                 uint8_t *message)
{
  emAfRf4ceMessageSentHandler(status,
                              pairingIndex,
                              txOptions,
                              profileId,
                              vendorId,
                              messageTag,
                              messageLength,
                              message);
}

void ezspRf4ceIncomingMessageHandler(uint8_t pairingIndex,
                                     uint8_t profileId,
                                     uint16_t vendorId,
                                     EmberRf4ceTxOption txOptions,
                                     uint8_t messageLength,
                                     uint8_t *message)

{
  emAfRf4ceIncomingMessageHandler(pairingIndex,
                                  profileId,
                                  vendorId,
                                  txOptions,
                                  messageLength,
                                  message);
}

void ezspRf4ceUnpairHandler(uint8_t pairingIndex)
{
  emAfRf4ceUnpairHandler(pairingIndex);
}

void ezspRf4ceUnpairCompleteHandler(uint8_t pairingIndex)
{
  emAfRf4ceUnpairCompleteHandler(pairingIndex);
}
