// Copyright 2013 Silicon Laboratories, Inc.

#define NWKC_MIN_CONTROLLER_PAIRING_TABLE_SIZE 1
#define NWKC_MIN_TARGET_PAIRING_TABLE_SIZE     5

bool emAfRf4ceDiscoveryRequestHandler(EmberEUI64 srcIeeeAddr,
                                         uint8_t nodeCapabilities,
                                         const EmberRf4ceVendorInfo *vendorInfo,
                                         const EmberRf4ceApplicationInfo *appInfo,
                                         uint8_t searchDevType,
                                         uint8_t rxLinkQuality);

bool emAfRf4ceDiscoveryResponseHandler(bool atCapacity,
                                          uint8_t channel,
                                          EmberPanId panId,
                                          EmberEUI64 srcIeeeAddr,
                                          uint8_t nodeCapabilities,
                                          const EmberRf4ceVendorInfo *vendorInfo,
                                          const EmberRf4ceApplicationInfo *appInfo,
                                          uint8_t rxLinkQuality,
                                          uint8_t discRequestLqi);

void emAfRf4ceDiscoveryCompleteHandler(EmberStatus status);

void emAfRf4ceAutoDiscoveryResponseCompleteHandler(EmberStatus status,
                                                   EmberEUI64 srcIeeeAddr,
                                                   uint8_t nodeCapabilities,
                                                   const EmberRf4ceVendorInfo *vendorInfo,
                                                   const EmberRf4ceApplicationInfo *appInfo,
                                                   uint8_t searchDevType);

bool emAfRf4cePairRequestHandler(EmberStatus status,
                                    uint8_t pairingIndex,
                                    EmberEUI64 srcIeeeAddr,
                                    uint8_t nodeCapabilities,
                                    const EmberRf4ceVendorInfo *vendorInfo,
                                    const EmberRf4ceApplicationInfo *appInfo,
                                    uint8_t keyExchangeTransferCount);

void emAfRf4cePairCompleteHandler(EmberStatus status,
                                  uint8_t pairingIndex,
                                  const EmberRf4ceVendorInfo *vendorInfo,
                                  const EmberRf4ceApplicationInfo *appInfo);

void emAfRf4ceMessageSentHandler(EmberStatus status,
                                 uint8_t pairingIndex,
                                 uint8_t txOptions,
                                 uint8_t profileId,
                                 uint16_t vendorId,
                                 uint8_t messageTag,
                                 uint8_t messageLength,
                                 const uint8_t *message);

void emAfRf4ceIncomingMessageHandler(uint8_t pairingIndex,
                                     uint8_t profileId,
                                     uint16_t vendorId,
                                     EmberRf4ceTxOption txOptions,
                                     uint8_t messageLength,
                                     const uint8_t *message);

void emAfRf4ceUnpairHandler(uint8_t pairingIndex);

void emAfRf4ceUnpairCompleteHandler(uint8_t pairingIndex);

uint8_t emAfRf4ceGetBaseChannel(void);

#ifdef EZSP_HOST
  #define emAfRf4ceStart                         ezspRf4ceStart
  #define emAfRf4ceSetPowerSavingParameters      ezspRf4ceSetPowerSavingParameters
  #define emAfRf4ceSetFrequencyAgilityParameters ezspRf4ceSetFrequencyAgilityParameters
  #define emAfRf4ceSetDiscoveryLqiThreshold(threshold)                      \
    ezspSetValue(EZSP_VALUE_RF4CE_DISCOVERY_LQI_THRESHOLD, 1, &(threshold))
  // emAfRf4ceGetBaseChannel is a function defined in rf4ce-profile-host.c
  #define emAfRf4ceDiscovery                     ezspRf4ceDiscovery
  #define emAfRf4ceEnableAutoDiscoveryResponse   ezspRf4ceEnableAutoDiscoveryResponse
  #define emAfRf4cePair                          ezspRf4cePair
  #define emAfRf4ceSetPairingTableEntry(pairingIndex, entry)                   \
  (entry == NULL) ? ezspRf4ceDeletePairingTableEntry(pairingIndex)             \
                  : ezspRf4ceSetPairingTableEntry(pairingIndex, entry)
  #define emAfRf4ceGetPairingTableEntry          ezspRf4ceGetPairingTableEntry
  #define emAfRf4ceSetApplicationInfo            ezspRf4ceSetApplicationInfo
  #define emAfRf4ceGetApplicationInfo            ezspRf4ceGetApplicationInfo
  #define emAfRf4ceKeyUpdate                     ezspRf4ceKeyUpdate
  #define emAfRf4ceSend                          ezspRf4ceSend
  #define emAfRf4ceUnpair                        ezspRf4ceUnpair
  #define emAfRf4ceStop                          ezspRf4ceStop
  #define emAfRf4ceGetMaxPayload                 ezspRf4ceGetMaxPayload
#else
  #define emAfRf4ceStart                         emberRf4ceStart
  #define emAfRf4ceSetPowerSavingParameters      emberRf4ceSetPowerSavingParameters
  #define emAfRf4ceSetFrequencyAgilityParameters emberRf4ceSetFrequencyAgilityParameters
  #define emAfRf4ceSetDiscoveryLqiThreshold      emberRf4ceSetDiscoveryLqiThreshold
  #define emAfRf4ceGetBaseChannel                emberRf4ceGetBaseChannel
  #define emAfRf4ceDiscovery                     emberRf4ceDiscovery
  #define emAfRf4ceEnableAutoDiscoveryResponse   emberRf4ceEnableAutoDiscoveryResponse
  #define emAfRf4cePair                          emberRf4cePair
  #define emAfRf4ceSetPairingTableEntry          emberRf4ceSetPairingTableEntry
  #define emAfRf4ceGetPairingTableEntry          emberRf4ceGetPairingTableEntry
  #define emAfRf4ceSetApplicationInfo            emberRf4ceSetApplicationInfo
  #define emAfRf4ceGetApplicationInfo            emberRf4ceGetApplicationInfo
  #define emAfRf4ceKeyUpdate                     emberRf4ceKeyUpdate
  #define emAfRf4ceSend                          emberRf4ceSend
  #define emAfRf4ceUnpair                        emberRf4ceUnpair
  #define emAfRf4ceStop                          emberRf4ceStop
  #define emAfRf4ceGetMaxPayload                 emberRf4ceGetMaxPayload
#endif

typedef struct {
  uint32_t dutyCycleMs;
  uint32_t activePeriodMs;
} EmAfRf4cePowerSavingState;

extern PGM EmberAfRf4ceProfileId emAfRf4ceProfileIds[];
extern bool emAfRf4ceRxOnWhenIdleProfileStates[];
extern EmAfRf4cePowerSavingState emAfRf4cePowerSavingState;
