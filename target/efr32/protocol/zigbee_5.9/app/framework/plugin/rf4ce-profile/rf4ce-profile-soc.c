// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "rf4ce-profile-internal.h"

void emberAfPluginRf4ceProfileNcpInitCallback(bool memoryAllocation)
{
}

bool emberRf4ceDiscoveryRequestHandler(EmberEUI64 srcIeeeAddr,
                                          uint8_t nodeCapabilities,
                                          EmberRf4ceVendorInfo *vendorInfo,
                                          EmberRf4ceApplicationInfo *appInfo,
                                          uint8_t searchDevType,
                                          uint8_t rxLinkQuality)
{
  return emAfRf4ceDiscoveryRequestHandler(srcIeeeAddr,
                                          nodeCapabilities,
                                          vendorInfo,
                                          appInfo,
                                          searchDevType,
                                          rxLinkQuality);
}

bool emberRf4ceDiscoveryResponseHandler(bool atCapacity,
                                           uint8_t channel,
                                           EmberPanId panId,
                                           EmberEUI64 srcIeeeAddr,
                                           uint8_t nodeCapabilities,
                                           EmberRf4ceVendorInfo *vendorInfo,
                                           EmberRf4ceApplicationInfo *appInfo,
                                           uint8_t rxLinkQuality,
                                           uint8_t discRequestLqi)
{
  return emAfRf4ceDiscoveryResponseHandler(atCapacity,
                                           channel,
                                           panId,
                                           srcIeeeAddr,
                                           nodeCapabilities,
                                           vendorInfo,
                                           appInfo,
                                           rxLinkQuality,
                                           discRequestLqi);
}

void emberRf4ceDiscoveryCompleteHandler(EmberStatus status)
{
  emAfRf4ceDiscoveryCompleteHandler(status);
}

void emberRf4ceAutoDiscoveryResponseCompleteHandler(EmberStatus status,
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

bool emberRf4cePairRequestHandler(EmberStatus status,
                                     uint8_t pairingIndex,
                                     EmberEUI64 srcIeeeAddr,
                                     uint8_t nodeCapabilities,
                                     EmberRf4ceVendorInfo *vendorInfo,
                                     EmberRf4ceApplicationInfo *appInfo,
                                     uint8_t keyExchangeTransferCount)
{
  return emAfRf4cePairRequestHandler(status,
                                     pairingIndex,
                                     srcIeeeAddr,
                                     nodeCapabilities,
                                     vendorInfo,
                                     appInfo,
                                     keyExchangeTransferCount);
}

void emberRf4cePairCompleteHandler(EmberStatus status,
                                   uint8_t pairingIndex,
                                   EmberRf4ceVendorInfo *vendorInfo,
                                   EmberRf4ceApplicationInfo *appInfo)
{
  emAfRf4cePairCompleteHandler(status, pairingIndex, vendorInfo, appInfo);
}

void emberRf4ceMessageSentHandler(EmberStatus status,
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

void emberRf4ceIncomingMessageHandler(uint8_t pairingIndex,
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

void emberRf4ceUnpairHandler(uint8_t pairingIndex)
{
  emAfRf4ceUnpairHandler(pairingIndex);
}

void emberRf4ceUnpairCompleteHandler(uint8_t pairingIndex)
{
  emAfRf4ceUnpairCompleteHandler(pairingIndex);
}
