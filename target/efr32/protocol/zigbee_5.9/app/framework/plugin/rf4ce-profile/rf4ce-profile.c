// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "rf4ce-profile.h"
#include "rf4ce-profile-internal.h"

//#define NETWORK_INDEX_DEBUG
#if defined(EMBER_TEST) || defined(NETWORK_INDEX_DEBUG)
  #define NETWORK_INDEX_ASSERT(x) assert(x)
#else
  #define NETWORK_INDEX_ASSERT(x)
#endif

#ifdef EMBER_AF_RF4CE_NODE_TYPE_CONTROLLER
  #define NWKC_MIN_PAIRING_TABLE_SIZE NWKC_MIN_CONTROLLER_PAIRING_TABLE_SIZE
#else
  #define NWKC_MIN_PAIRING_TABLE_SIZE NWKC_MIN_TARGET_PAIRING_TABLE_SIZE
#endif
#if EMBER_RF4CE_PAIRING_TABLE_SIZE < NWKC_MIN_PAIRING_TABLE_SIZE
  #error The device does not support the minimum number of pairing table entries for the node type.
#endif

#define DISCOVERY_IN_PROGRESS() (discoveryProfileIdCount != 0)
#define SET_DISCOVERY_IN_PROGRESS(count, list) \
  do {                                         \
    discoveryProfileIdCount = (count);         \
    MEMMOVE(discoveryProfileIds, (list), discoveryProfileIdCount); \
  } while (false)
#define UNSET_DISCOVERY_IN_PROGRESS() (discoveryProfileIdCount = 0)
#define AUTO_DISCOVERY_IN_PROGRESS() (autoDiscoveryProfileIdCount != 0)
#define SET_AUTO_DISCOVERY_IN_PROGRESS(count, list)                        \
  do {                                                                     \
    autoDiscoveryProfileIdCount = (count);                                 \
    MEMMOVE(autoDiscoveryProfileIds, (list), autoDiscoveryProfileIdCount); \
  } while (false)
#define UNSET_AUTO_DISCOVERY_IN_PROGRESS() (autoDiscoveryProfileIdCount = 0)
#define PAIRING_IN_PROGRESS() (pairing)
#define SET_PAIRING_IN_PROGRESS() (pairing = true)
#define UNSET_PAIRING_IN_PROGRESS() (pairing = false)
#define UNPAIRING_IN_PROGRESS() (unpairing)
#define SET_UNPAIRING_IN_PROGRESS() (unpairing = true)
#define UNSET_UNPAIRING_IN_PROGRESS() (unpairing = false)

#define SET_PAIRING_INDEX(pairingIndex) (currentPairingIndex = (pairingIndex))
#define UNSET_PAIRING_INDEX() (currentPairingIndex = 0xFF)

EmberRf4ceVendorInfo emAfRf4ceVendorInfo = EMBER_AF_RF4CE_VENDOR_INFO;
EmberRf4ceApplicationInfo emAfRf4ceApplicationInfo = EMBER_AF_RF4CE_APPLICATION_INFO;

static uint8_t discoveryProfileIds[EMBER_RF4CE_APPLICATION_PROFILE_ID_LIST_MAX_LENGTH];
static uint8_t discoveryProfileIdCount;
static uint8_t autoDiscoveryProfileIds[EMBER_RF4CE_APPLICATION_PROFILE_ID_LIST_MAX_LENGTH];
static uint8_t autoDiscoveryProfileIdCount;
static bool pairing;
static bool unpairing;

static uint8_t currentPairingIndex;

static uint8_t nextMessageTag = 0;

// The following variables keep track of whether the application and the
// various profiles want the receiver on.  The default state is that the
// application wants the receiver on (via the power-saving state {0, 0}) but
// the profiles do not (via the default FALSEs in
// emAfRf4ceRxOnWhenIdleProfileStates[]).
PGM EmberAfRf4ceProfileId emAfRf4ceProfileIds[] = EMBER_AF_RF4CE_PROFILE_IDS;
bool emAfRf4ceRxOnWhenIdleProfileStates[EMBER_AF_RF4CE_PROFILE_ID_COUNT];
EmAfRf4cePowerSavingState emAfRf4cePowerSavingState = {0, 0}; // rx enabled

#ifdef EMBER_MULTI_NETWORK_STRIPPED
  EmberStatus emberAfPushNetworkIndex(uint8_t networkIndex)
  {
    return EMBER_SUCCESS;
  }

  EmberStatus emberAfPushCallbackNetworkIndex(void)
  {
    return EMBER_SUCCESS;
  }

  EmberStatus emberAfPopNetworkIndex(void)
  {
    return EMBER_SUCCESS;
  }

  bool emberAfRf4ceIsCurrentNetwork(void)
  {
    return true;
  }

  EmberStatus emberAfRf4cePushNetworkIndex(void)
  {
    return EMBER_SUCCESS;
  }

  static void setRf4ceNetwork(void)
  {
  }
#else
  static uint8_t rf4ceNetworkIndex = 0xFF;

  bool emberAfRf4ceIsCurrentNetwork(void)
  {
    NETWORK_INDEX_ASSERT(rf4ceNetworkIndex != 0xFF);
    return (emberGetCurrentNetwork() == rf4ceNetworkIndex);
  }

  EmberStatus emberAfRf4cePushNetworkIndex(void)
  {
    NETWORK_INDEX_ASSERT(rf4ceNetworkIndex != 0xFF);
    return emberAfPushNetworkIndex(rf4ceNetworkIndex);
  }

  static void setRf4ceNetwork(void)
  {
    uint8_t i;
    for (i = 0; i < EMBER_SUPPORTED_NETWORKS; i++) {
      if (emAfNetworkTypes[i] == EM_AF_NETWORK_TYPE_ZIGBEE_RF4CE) {
        NETWORK_INDEX_ASSERT(rf4ceNetworkIndex == 0xFF);
        rf4ceNetworkIndex = i;
      }
    }
    NETWORK_INDEX_ASSERT(rf4ceNetworkIndex != 0xFF);
  }
#endif

void emberAfPluginRf4ceProfileInitCallback(void)
{
  setRf4ceNetwork();

  UNSET_DISCOVERY_IN_PROGRESS();
  UNSET_AUTO_DISCOVERY_IN_PROGRESS();
  UNSET_PAIRING_IN_PROGRESS();
  UNSET_UNPAIRING_IN_PROGRESS();

  UNSET_PAIRING_INDEX();

  emAfRf4ceSetApplicationInfo(&emAfRf4ceApplicationInfo);
}

EmberStatus emberAfRf4ceStart(void)
{
  EmberStatus status = emberAfRf4cePushNetworkIndex();
  if (status == EMBER_SUCCESS) {
    status = emAfRf4ceSetApplicationInfo(&emAfRf4ceApplicationInfo);
    if (status == EMBER_SUCCESS) {
      status = emAfRf4ceStart(EMBER_AF_RF4CE_NODE_CAPABILITIES,
                              &emAfRf4ceVendorInfo,
                              EMBER_AF_RF4CE_POWER);
    }
    emberAfPopNetworkIndex();
  }
  return status;
}

EmberStatus emberAfRf4ceStop(void)
{
  EmberStatus status = emberAfRf4cePushNetworkIndex();
  if (status == EMBER_SUCCESS) {
    status = emAfRf4ceStop();
    emberAfPopNetworkIndex();
  }
  return status;
}

uint8_t emberAfRf4ceGetMaxPayload(uint8_t pairingIndex,
                                EmberRf4ceTxOption txOptions)
{
  EmberStatus status = emberAfRf4cePushNetworkIndex();
  uint8_t maxLength = 0;

  if (status == EMBER_SUCCESS) {
    maxLength = emAfRf4ceGetMaxPayload(pairingIndex, txOptions);
    emberAfPopNetworkIndex();
  }

  return maxLength;
}

static bool rxEnabled(void)
{
  // The receiver should be enabled if the power saving is disabled (i.e., the
  // duty cycle is zero) or if any of the profiles wants the receiver on.
  // Otherwise, the receiver can be turned off, either until further notice or
  // using power saving.  The wildcard profile is used to set the default state
  // for the application, so it is not checked along with the other profiles.
  uint8_t i;
  if (emAfRf4cePowerSavingState.dutyCycleMs == 0) {
    return true;
  }
  for (i = 0; i < EMBER_AF_RF4CE_PROFILE_ID_COUNT; i++) {
    if (emAfRf4ceProfileIds[i] != EMBER_AF_RF4CE_PROFILE_WILDCARD
        && emAfRf4ceRxOnWhenIdleProfileStates[i]) {
      return true;
    }
  }
  return false;
}

static EmberStatus rxEnable(EmberAfRf4ceProfileId profileId, bool enable)
{
  // Each profile can indicate whether it needs the receiver on or not and the
  // wildcard profile can be used to set the default state for the application.
  // Specifying enabled for the wildcard profile simply disables power saving
  // completely and will leave the receiver on.  Conversely, setting the
  // wildcard profile to disabled will allow the receiver to be turned off when
  // no other profile needs it to be on.  Because the wildcard profile is used
  // to set the default state for the application, it is not set like the other
  // profiles are.
  uint8_t i;
  if (profileId == EMBER_AF_RF4CE_PROFILE_WILDCARD) {
    emAfRf4cePowerSavingState.dutyCycleMs = (enable ? 0 : 1);
    emAfRf4cePowerSavingState.activePeriodMs = 0;
    return EMBER_SUCCESS;
  }
  for (i = 0; i < EMBER_AF_RF4CE_PROFILE_ID_COUNT; i++) {
    // A check for the wildcard profile is not required here because of the
    // return statement in the if statement above.
    if (emAfRf4ceProfileIds[i] == profileId) {
      emAfRf4ceRxOnWhenIdleProfileStates[i] = enable;
      return EMBER_SUCCESS;
    }
  }
  return EMBER_INVALID_CALL;
}

static EmberStatus setPowerSavingParameters(bool wasEnabled)
{
  EmberStatus status = emberAfRf4cePushNetworkIndex();
  if (status == EMBER_SUCCESS) {
    // If the receiver was enabled and is staying enabled, we have nothing to
    // do.  Otherwise, enable the receiver if we're now on or set the power-
    // saving parameters if not.  If the receiver doesn't have to be enabled,
    // we always set the power-saving parameters even if the receiver wasn't
    // enabled before.  This is because we may have got here via a call to
    // emberAfRf4ceSetPowerSavingParameters that set new parameters.  This
    // ensures the parameters set in the stack are up to date.
    if (!rxEnabled()) {
      status = emAfRf4ceSetPowerSavingParameters(emAfRf4cePowerSavingState.dutyCycleMs,
                                                 emAfRf4cePowerSavingState.activePeriodMs);
    } else if (!wasEnabled) {
      status = emAfRf4ceSetPowerSavingParameters(0, 0); // enable
    }
    emberAfPopNetworkIndex();
  }
  return status;
}

EmberStatus emberAfRf4ceSetPowerSavingParameters(uint32_t dutyCycleMs,
                                                 uint32_t activePeriodMs)
{
  bool wasEnabled;

  // This duplicates the check in emberRf4ceSetPowerSavingParameters in the
  // stack.  We need to be able to validate the parameters without actually
  // setting them in the stack.  This can happen when the application is
  // setting new parameters at the same time that some other profile needs the
  // receiver to be on.  In that case, the receiver should stay on until the
  // profile is finished and only then should the device enter power-saving
  // mode.  We want to validate and then remember the parameters so that we can
  // use them later without worrying about what to do if they were bogus.
  if (dutyCycleMs != 0 && activePeriodMs != 0
      && (dutyCycleMs > EMBER_RF4CE_MAX_DUTY_CYCLE_MS
          || activePeriodMs < EMBER_RF4CE_MIN_ACTIVE_PERIOD_MS
          || activePeriodMs >= dutyCycleMs)) {
    return EMBER_INVALID_CALL;
  }

  // Check the current global state, save the new power-saving parameters, then
  // set the new global state.
  wasEnabled = rxEnabled();
  emAfRf4cePowerSavingState.dutyCycleMs = dutyCycleMs;
  emAfRf4cePowerSavingState.activePeriodMs = activePeriodMs;
  return setPowerSavingParameters(wasEnabled);
}

EmberStatus emberAfRf4ceRxEnable(EmberAfRf4ceProfileId profileId,
                                 bool enable)
{
  // Check the current global state, set the profile state, then set the new
  // global state.
  bool wasEnabled = rxEnabled();
  EmberStatus status = rxEnable(profileId, enable);
  if (status == EMBER_SUCCESS) {
    status = setPowerSavingParameters(wasEnabled);
  }
  return status;
}

EmberStatus emberAfRf4ceSetFrequencyAgilityParameters(uint8_t rssiWindowSize,
                                                      uint8_t channelChangeReads,
                                                      int8_t rssiThreshold,
                                                      uint16_t readIntervalS,
                                                      uint8_t readDuration)
{
  EmberStatus status = emberAfRf4cePushNetworkIndex();
  if (status == EMBER_SUCCESS) {
    status = emAfRf4ceSetFrequencyAgilityParameters(rssiWindowSize,
                                                    channelChangeReads,
                                                    rssiThreshold,
                                                    readIntervalS,
                                                    readDuration);
    emberAfPopNetworkIndex();
  }
  return status;
}

EmberStatus emberAfRf4ceDiscovery(EmberPanId panId,
                                  EmberNodeId nodeId,
                                  uint8_t searchDevType,
                                  uint16_t discDuration,
                                  uint8_t maxDiscRepetitions,
                                  uint8_t discProfileIdListLength,
                                  uint8_t *discProfileIdList)
{
  EmberStatus status = EMBER_INVALID_CALL;
  if (!DISCOVERY_IN_PROGRESS()) {
    status = emberAfRf4cePushNetworkIndex();
    if (status == EMBER_SUCCESS) {
      status = emAfRf4ceDiscovery(panId,
                                  nodeId,
                                  searchDevType,
                                  discDuration,
                                  maxDiscRepetitions,
                                  discProfileIdListLength,
                                  discProfileIdList);
      if (status == EMBER_SUCCESS) {
        SET_DISCOVERY_IN_PROGRESS(discProfileIdListLength, discProfileIdList);
      }
      emberAfPopNetworkIndex();
    }
  }
  return status;
}

bool emAfRf4ceDiscoveryRequestHandler(EmberEUI64 srcIeeeAddr,
                                         uint8_t nodeCapabilities,
                                         const EmberRf4ceVendorInfo *vendorInfo,
                                         const EmberRf4ceApplicationInfo *appInfo,
                                         uint8_t searchDevType,
                                         uint8_t rxLinkQuality)
{
  uint8_t i;
  uint8_t profileIdListLength = emberAfRf4ceProfileIdListLength(appInfo->capabilities);
  bool retVal = false;

  emberAfPushCallbackNetworkIndex();

  for (i = 0; i < profileIdListLength && !retVal; i++) {
    switch (appInfo->profileIdList[i]) {
    case EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_1_1:
      retVal = emberAfPluginRf4ceProfileRemoteControl11DiscoveryRequestCallback(srcIeeeAddr,
                                                                                nodeCapabilities,
                                                                                vendorInfo,
                                                                                appInfo,
                                                                                searchDevType,
                                                                                rxLinkQuality);
      break;
    case EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0:
      retVal = emberAfPluginRf4ceProfileZrc20DiscoveryRequestCallback(srcIeeeAddr,
                                                                      nodeCapabilities,
                                                                      vendorInfo,
                                                                      appInfo,
                                                                      searchDevType,
                                                                      rxLinkQuality);
      break;
    case EMBER_AF_RF4CE_PROFILE_MSO:
      retVal = emberAfPluginRf4ceProfileMsoDiscoveryRequestCallback(srcIeeeAddr,
                                                                    nodeCapabilities,
                                                                    vendorInfo,
                                                                    appInfo,
                                                                    searchDevType,
                                                                    rxLinkQuality);
      break;
    default:
      // TODO: Handle other profiles.
      break;
    }
  }

  emberAfPopNetworkIndex();
  return retVal;
}

static bool wantsDiscoveryProfileId(uint8_t profileId)
{
  uint8_t i;
  for (i = 0; i < discoveryProfileIdCount; i++) {
    if (discoveryProfileIds[i] == profileId) {
      return true;
    }
  }
  return false;
}

bool emAfRf4ceDiscoveryResponseHandler(bool atCapacity,
                                          uint8_t channel,
                                          EmberPanId panId,
                                          EmberEUI64 srcIeeeAddr,
                                          uint8_t nodeCapabilities,
                                          const EmberRf4ceVendorInfo *vendorInfo,
                                          const EmberRf4ceApplicationInfo *appInfo,
                                          uint8_t rxLinkQuality,
                                          uint8_t discRequestLqi)
{
  bool retVal = false;
  if (DISCOVERY_IN_PROGRESS()) {
    uint8_t i;
    uint8_t profileIdListLength = emberAfRf4ceProfileIdListLength(appInfo->capabilities);

    emberAfPushCallbackNetworkIndex();

    for (i = 0; i < profileIdListLength && !retVal; i++) {
      if (wantsDiscoveryProfileId(appInfo->profileIdList[i])) {
        switch (appInfo->profileIdList[i]) {
        case EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_1_1:
          retVal = emberAfPluginRf4ceProfileRemoteControl11DiscoveryResponseCallback(atCapacity,
                                                                                     channel,
                                                                                     panId,
                                                                                     srcIeeeAddr,
                                                                                     nodeCapabilities,
                                                                                     vendorInfo,
                                                                                     appInfo,
                                                                                     rxLinkQuality,
                                                                                     discRequestLqi);
          break;
        case EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0:
          retVal = emberAfPluginRf4ceProfileZrc20DiscoveryResponseCallback(atCapacity,
                                                                           channel,
                                                                           panId,
                                                                           srcIeeeAddr,
                                                                           nodeCapabilities,
                                                                           vendorInfo,
                                                                           appInfo,
                                                                           rxLinkQuality,
                                                                           discRequestLqi);
          break;
        case EMBER_AF_RF4CE_PROFILE_MSO:
          retVal = emberAfPluginRf4ceProfileMsoDiscoveryResponseCallback(atCapacity,
                                                                         channel,
                                                                         panId,
                                                                         srcIeeeAddr,
                                                                         nodeCapabilities,
                                                                         vendorInfo,
                                                                         appInfo,
                                                                         rxLinkQuality,
                                                                         discRequestLqi);
          break;
        default:
          // TODO: Handle other profiles.
          break;
        }
      }
    }

    emberAfPopNetworkIndex();
  }
  return retVal;
}

void emAfRf4ceDiscoveryCompleteHandler(EmberStatus status)
{
  if (DISCOVERY_IN_PROGRESS()) {
    uint8_t i;

    // Clear the state (after saving a copy) so that the application is able to
    // perform another discovery right away.
    uint8_t profileIdCount = discoveryProfileIdCount;
    uint8_t profileIds[EMBER_RF4CE_APPLICATION_PROFILE_ID_LIST_MAX_LENGTH];
    MEMMOVE(profileIds, discoveryProfileIds, profileIdCount);
    UNSET_DISCOVERY_IN_PROGRESS();

    emberAfPushCallbackNetworkIndex();
    for (i = 0; i < profileIdCount; i++) {
      switch (profileIds[i]) {
      case EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_1_1:
        emberAfPluginRf4ceProfileRemoteControl11DiscoveryCompleteCallback(status);
        break;
      case EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0:
        emberAfPluginRf4ceProfileZrc20DiscoveryCompleteCallback(status);
        break;
      case EMBER_AF_RF4CE_PROFILE_MSO:
        emberAfPluginRf4ceProfileMsoDiscoveryCompleteCallback(status);
        break;
      default:
        // TODO: Handle other profiles.
        break;
      }
    }
    emberAfPopNetworkIndex();
  }
}

EmberStatus emberAfRf4ceEnableAutoDiscoveryResponse(uint16_t durationMs,
                                                    uint8_t autoDiscProfileIdListLength,
                                                    uint8_t *autoDiscProfileIdList)
{
  EmberStatus status = EMBER_INVALID_CALL;
  if (!AUTO_DISCOVERY_IN_PROGRESS()) {
    status = emberAfRf4cePushNetworkIndex();
    if (status == EMBER_SUCCESS) {
      status = emAfRf4ceEnableAutoDiscoveryResponse(durationMs);
      if (status == EMBER_SUCCESS) {
        SET_AUTO_DISCOVERY_IN_PROGRESS(autoDiscProfileIdListLength,
                                       autoDiscProfileIdList);
      }
      emberAfPopNetworkIndex();
    }
  }
  return status;
}

static bool wantsAutoDiscoveryProfileIds(const EmberRf4ceApplicationInfo *appInfo)
{
  uint8_t i, j;
  uint8_t profileIdListLength = emberAfRf4ceProfileIdListLength(appInfo->capabilities);
  for (i = 0; i < profileIdListLength; i++) {
    for (j = 0; i < autoDiscoveryProfileIdCount; j++) {
      if (appInfo->profileIdList[i] == autoDiscoveryProfileIds[j]) {
        return true;
      }
    }
  }
  return false;
}

void emAfRf4ceAutoDiscoveryResponseCompleteHandler(EmberStatus status,
                                                   EmberEUI64 srcIeeeAddr,
                                                   uint8_t nodeCapabilities,
                                                   const EmberRf4ceVendorInfo *vendorInfo,
                                                   const EmberRf4ceApplicationInfo *appInfo,
                                                   uint8_t searchDevType)
{
  if (AUTO_DISCOVERY_IN_PROGRESS()) {
    uint8_t i;

    // Clear the state (after saving a copy) so that the application is able to
    // perform another auto discovery right away.
    uint8_t profileIdCount = autoDiscoveryProfileIdCount;
    uint8_t profileIds[EMBER_RF4CE_APPLICATION_PROFILE_ID_LIST_MAX_LENGTH];
    MEMMOVE(profileIds, autoDiscoveryProfileIds, profileIdCount);
    if (status == EMBER_SUCCESS && !wantsAutoDiscoveryProfileIds(appInfo)) {
      status = EMBER_DISCOVERY_TIMEOUT;
    }
    UNSET_AUTO_DISCOVERY_IN_PROGRESS();

    emberAfPushCallbackNetworkIndex();

    for (i = 0; i < profileIdCount; i++) {
      switch (profileIds[i]) {
      case EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_1_1:
        emberAfPluginRf4ceProfileRemoteControl11AutoDiscoveryResponseCompleteCallback(status,
                                                                                      srcIeeeAddr,
                                                                                      nodeCapabilities,
                                                                                      vendorInfo,
                                                                                      appInfo,
                                                                                      searchDevType);
        break;
      case EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0:
        emberAfPluginRf4ceProfileZrc20AutoDiscoveryResponseCompleteCallback(status,
                                                                            srcIeeeAddr,
                                                                            nodeCapabilities,
                                                                            vendorInfo,
                                                                            appInfo,
                                                                            searchDevType);
        break;
      case EMBER_AF_RF4CE_PROFILE_MSO:
        emberAfPluginRf4ceProfileMsoAutoDiscoveryResponseCompleteCallback(status,
                                                                          srcIeeeAddr,
                                                                          nodeCapabilities,
                                                                          vendorInfo,
                                                                          appInfo,
                                                                          searchDevType);
        break;
      default:
        // TODO: Handle other profiles.
        break;
      }
    }

    emberAfPopNetworkIndex();
  }
}

static EmberAfRf4cePairCompleteCallback pairCompletePtr = NULL;

EmberStatus emberAfRf4cePair(uint8_t channel,
                             EmberPanId panId,
                             EmberEUI64 ieeeAddr,
                             uint8_t keyExchangeTransferCount,
                             EmberAfRf4cePairCompleteCallback pairCompleteCallback)
{
  EmberStatus status = EMBER_INVALID_CALL;
  if (!PAIRING_IN_PROGRESS()) {
    status = emberAfRf4cePushNetworkIndex();
    if (status == EMBER_SUCCESS) {
      status = emAfRf4cePair(channel,
                             panId,
                             ieeeAddr,
                             keyExchangeTransferCount);
      if (status == EMBER_SUCCESS || status == EMBER_DUPLICATE_ENTRY) {
        SET_PAIRING_IN_PROGRESS();
        pairCompletePtr = pairCompleteCallback;
      }
      emberAfPopNetworkIndex();
    }
  }
  return status;
}

bool emAfRf4cePairRequestHandler(EmberStatus status,
                                    uint8_t pairingIndex,
                                    EmberEUI64 srcIeeeAddr,
                                    uint8_t nodeCapabilities,
                                    const EmberRf4ceVendorInfo *vendorInfo,
                                    const EmberRf4ceApplicationInfo *appInfo,
                                    uint8_t keyExchangeTransferCount)
{
  bool retVal = false;

  if (!PAIRING_IN_PROGRESS()) {
    uint8_t i;
    uint8_t profileIdListLength = emberAfRf4ceProfileIdListLength(appInfo->capabilities);

    emberAfPushCallbackNetworkIndex();

    for (i = 0; i < profileIdListLength && !retVal; i++) {
      switch (appInfo->profileIdList[i]) {
      case EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_1_1:
        retVal = emberAfPluginRf4ceProfileRemoteControl11PairRequestCallback(status,
                                                                             pairingIndex,
                                                                             srcIeeeAddr,
                                                                             nodeCapabilities,
                                                                             vendorInfo,
                                                                             appInfo,
                                                                             keyExchangeTransferCount);
        break;
      case EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0:
        retVal = emberAfPluginRf4ceProfileZrc20PairRequestCallback(status,
                                                                   pairingIndex,
                                                                   srcIeeeAddr,
                                                                   nodeCapabilities,
                                                                   vendorInfo,
                                                                   appInfo,
                                                                   keyExchangeTransferCount);
        break;
      case EMBER_AF_RF4CE_PROFILE_MSO:
        retVal = emberAfPluginRf4ceProfileMsoPairRequestCallback(status,
                                                                 pairingIndex,
                                                                 srcIeeeAddr,
                                                                 nodeCapabilities,
                                                                 vendorInfo,
                                                                 appInfo,
                                                                 keyExchangeTransferCount);
        break;
      default:
        // TODO: Handle other profiles.
        break;
      }
    }

    if (retVal) {
      SET_PAIRING_IN_PROGRESS();
    }

    emberAfPopNetworkIndex();
  }
  return retVal;
}

void emAfRf4cePairCompleteHandler(EmberStatus status,
                                  uint8_t pairingIndex,
                                  const EmberRf4ceVendorInfo *vendorInfo,
                                  const EmberRf4ceApplicationInfo *appInfo)
{
  if (PAIRING_IN_PROGRESS()) {
    UNSET_PAIRING_IN_PROGRESS();

    emberAfPushCallbackNetworkIndex();

    // If the application passed a function pointer for the pair complete
    // callback, we only call that one. Otherwise we try to dispatch based on
    // the profile ID list.
    if (pairCompletePtr) {
      // In the pair complete callback the application could invoke another
      // pair call. Therefore, we save the function pointer in a local variable
      // and set the global back to NULL prior to actual callback call.
      EmberAfRf4cePairCompleteCallback callbackPtr = pairCompletePtr;
      pairCompletePtr = NULL;
      (callbackPtr)(status, pairingIndex, vendorInfo, appInfo);
      emberAfPopNetworkIndex();
      return;
    }

    // If the status is not a successful status, the application info is not
    // included in the callback, so we just dispatch to all the plugins.
    // TODO: we might just go back to dispatch to all plugins regardless of
    // status.
    if (status != EMBER_SUCCESS && status != EMBER_DUPLICATE_ENTRY) {
      emberAfPluginRf4ceProfileRemoteControl11PairCompleteCallback(status,
                                                                   pairingIndex,
                                                                   NULL,
                                                                   NULL);
      emberAfPluginRf4ceProfileZrc20PairCompleteCallback(status,
                                                         pairingIndex,
                                                         NULL,
                                                         NULL);
      emberAfPluginRf4ceProfileMsoPairCompleteCallback(status,
                                                       pairingIndex,
                                                       NULL,
                                                       NULL);
    } else {
      uint8_t profileIdListLength = emberAfRf4ceProfileIdListLength(appInfo->capabilities);
      uint8_t i;

      for (i = 0; i < profileIdListLength; i++) {
        uint8_t profileId = appInfo->profileIdList[i];
        switch (profileId) {
        case EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_1_1: {
          emberAfPluginRf4ceProfileRemoteControl11PairCompleteCallback(status,
                                                                       pairingIndex,
                                                                       vendorInfo,
                                                                       appInfo);
          break;
        }
        case EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0: {
          emberAfPluginRf4ceProfileZrc20PairCompleteCallback(status,
                                                             pairingIndex,
                                                             vendorInfo,
                                                             appInfo);
          break;
        }
        case EMBER_AF_RF4CE_PROFILE_MSO: {
          emberAfPluginRf4ceProfileMsoPairCompleteCallback(status,
                                                           pairingIndex,
                                                           vendorInfo,
                                                           appInfo);
          break;
        }
        default:
          // TODO: Handle other profiles.
          break;
        }
      }
    }

    emberAfPopNetworkIndex();
  }
}

EmberStatus emberAfRf4ceSetPairingTableEntry(uint8_t pairingIndex,
                                             EmberRf4cePairingTableEntry *entry)
{
  EmberStatus status = emberAfRf4cePushNetworkIndex();
  if (status == EMBER_SUCCESS) {
    status = emAfRf4ceSetPairingTableEntry(pairingIndex, entry);
    emberAfPopNetworkIndex();
  }
  return status;
}

EmberStatus emberAfRf4ceGetPairingTableEntry(uint8_t pairingIndex,
                                             EmberRf4cePairingTableEntry *entry)
{
  EmberStatus status = emberAfRf4cePushNetworkIndex();
  if (status == EMBER_SUCCESS) {
    status = emAfRf4ceGetPairingTableEntry(pairingIndex, entry);
    emberAfPopNetworkIndex();
  }
  return status;
}

EmberStatus emberAfRf4ceSetApplicationInfo(EmberRf4ceApplicationInfo *appInfo)
{
  EmberStatus status = emberAfRf4cePushNetworkIndex();
  if (status == EMBER_SUCCESS) {
    status = emAfRf4ceSetApplicationInfo(appInfo);
    emberAfPopNetworkIndex();
  }
  return status;
}

EmberStatus emberAfRf4ceGetApplicationInfo(EmberRf4ceApplicationInfo *appInfo)
{
  EmberStatus status = emberAfRf4cePushNetworkIndex();
  if (status == EMBER_SUCCESS) {
    status = emAfRf4ceGetApplicationInfo(appInfo);
    emberAfPopNetworkIndex();
  }
  return status;
}

EmberStatus emberAfRf4ceKeyUpdate(uint8_t pairingIndex, EmberKeyData *key)
{
  EmberStatus status = emberAfRf4cePushNetworkIndex();
  if (status == EMBER_SUCCESS) {
    status = emAfRf4ceKeyUpdate(pairingIndex, key);
    emberAfPopNetworkIndex();
  }
  return status;
}

EmberStatus emberAfRf4ceSetDiscoveryLqiThreshold(uint8_t threshold)
{
  EmberStatus status = emberAfRf4cePushNetworkIndex();
  if (status == EMBER_SUCCESS) {
    status = emAfRf4ceSetDiscoveryLqiThreshold(threshold);
    emberAfPopNetworkIndex();
  }
  return status;
}

uint8_t emberAfRf4ceGetBaseChannel(void)
{
  EmberStatus status = emberAfRf4cePushNetworkIndex();
  uint8_t channel = 0xFF;

  if (status == EMBER_SUCCESS) {
    channel =  emAfRf4ceGetBaseChannel();
    emberAfPopNetworkIndex();
  }
  return channel;
}

EmberStatus emberAfRf4ceSendExtended(uint8_t pairingIndex,
                                     uint8_t profileId,
                                     uint16_t vendorId,
                                     EmberRf4ceTxOption txOptions,
                                     uint8_t *message,
                                     uint8_t messageLength,
                                     uint8_t *messageTag)
{
  EmberStatus status;
  uint8_t thisMessageTag;

  thisMessageTag = nextMessageTag++ & EMBER_AF_RF4CE_MESSAGE_TAG_MASK;
  if (messageTag != NULL) {
    *messageTag = thisMessageTag;
  }

  status = emberAfRf4cePushNetworkIndex();
  if (status != EMBER_SUCCESS) {
    return status;
  }

  status = emAfRf4ceSend(pairingIndex,
                         profileId,
                         vendorId,
                         txOptions,
                         thisMessageTag,
                         messageLength,
                         message);

  emberAfPopNetworkIndex();
  return status;
}

EmberStatus emberAfRf4ceSend(uint8_t pairingIndex,
                             uint8_t profileId,
                             uint8_t *message,
                             uint8_t messageLength,
                             uint8_t *messageTag)
{
  EmberRf4ceTxOption txOptions;
  EmberStatus status = emberAfRf4ceGetDefaultTxOptions(pairingIndex,
                                                       &txOptions);

  if (status != EMBER_SUCCESS) {
    return status;
  }

  return emberAfRf4ceSendExtended(pairingIndex,
                                  profileId,
                                  EMBER_RF4CE_NULL_VENDOR_ID,
                                  txOptions,
                                  message,
                                  messageLength,
                                  messageTag);
}

EmberStatus emberAfRf4ceSendVendorSpecific(uint8_t pairingIndex,
                                           uint8_t profileId,
                                           uint16_t vendorId,
                                           uint8_t *message,
                                           uint8_t messageLength,
                                           uint8_t *messageTag)
{
  EmberRf4ceTxOption txOptions;
  EmberStatus status = emberAfRf4ceGetDefaultTxOptions(pairingIndex,
                                                       &txOptions);

  if (status != EMBER_SUCCESS) {
    return status;
  }

  return emberAfRf4ceSendExtended(pairingIndex,
                                  profileId,
                                  vendorId,
                                  (txOptions
                                   | EMBER_RF4CE_TX_OPTIONS_VENDOR_SPECIFIC_BIT),
                                   message,
                                   messageLength,
                                   messageTag);
}

EmberStatus emberAfRf4ceGetDefaultTxOptions(uint8_t pairingIndex,
                                            EmberRf4ceTxOption *txOptions)
{
  EmberStatus status = EMBER_SUCCESS;

  if (pairingIndex == EMBER_RF4CE_PAIRING_TABLE_BROADCAST_INDEX) {
    *txOptions = EMBER_RF4CE_TX_OPTIONS_BROADCAST_BIT;
  } else {
    EmberRf4cePairingTableEntry entry;

    status = emberAfRf4ceGetPairingTableEntry(pairingIndex, &entry);
    if (status == EMBER_SUCCESS) {
      *txOptions = EMBER_RF4CE_TX_OPTIONS_ACK_REQUESTED_BIT;

      if (emberAfRf4cePairingTableEntryHasSecurity(&entry)
          && emberAfRf4cePairingTableEntryHasLinkKey(&entry)) {
        *txOptions |= EMBER_RF4CE_TX_OPTIONS_SECURITY_ENABLED_BIT;
      }

#ifdef EMBER_AF_RF4CE_NODE_TYPE_TARGET
      if (emberAfRf4cePairingTableEntryHasChannelNormalization(&entry)) {
        *txOptions |= EMBER_RF4CE_TX_OPTIONS_CHANNEL_DESIGNATOR_BIT;
      }
#endif

      // TODO: Use long addressing if there is enough room in the payload.
      //*txOptions |= EMBER_RF4CE_TX_OPTIONS_USE_IEEE_ADDRESS_BIT;
    }
  }

  return status;
}

void emAfRf4ceMessageSentHandler(EmberStatus status,
                                 uint8_t pairingIndex,
                                 uint8_t txOptions,
                                 uint8_t profileId,
                                 uint16_t vendorId,
                                 uint8_t messageTag,
                                 uint8_t messageLength,
                                 const uint8_t *message)
{
  emberAfPushCallbackNetworkIndex();
  SET_PAIRING_INDEX(pairingIndex);

  // GDP 2.0 commands are tricky since they can be sent with the GDP 2.0 profile
  // ID or any GDP 2.0 based profile ID. We want to use the common GDP 2.0 code
  // for the commands that carry the GDP profile ID and other GDP-based profile
  // IDs. Therefore, first we try to dispatch to GDP callback. This callback is
  // expected to return true if the passed message is a GDP 2.0 command, false
  // otherwise.
  if (!emberAfPluginRf4ceProfileGdpMessageSentCallback(pairingIndex,
                                                       profileId,
                                                       vendorId,
                                                       messageTag,
                                                       message,
                                                       messageLength,
                                                       status)) {
    switch (profileId) {
    case EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_1_1:
      emberAfPluginRf4ceProfileRemoteControl11MessageSentCallback(pairingIndex,
                                                                  vendorId,
                                                                  messageTag,
                                                                  message,
                                                                  messageLength,
                                                                  status);
      break;
    case EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0:
      emberAfPluginRf4ceProfileZrc20MessageSentCallback(pairingIndex,
                                                        vendorId,
                                                        messageTag,
                                                        message,
                                                        messageLength,
                                                        status);
      break;
    case EMBER_AF_RF4CE_PROFILE_MSO:
      emberAfPluginRf4ceProfileMsoMessageSentCallback(pairingIndex,
                                                      vendorId,
                                                      messageTag,
                                                      message,
                                                      messageLength,
                                                      status);
      break;
    default:
      // TODO: Handle other profiles.
      break;
    }
  }

  emberAfPluginRf4ceProfileMessageSentCallback(pairingIndex,
                                               profileId,
                                               vendorId,
                                               messageTag,
                                               message,
                                               messageLength,
                                               status);

  UNSET_PAIRING_INDEX();
  emberAfPopNetworkIndex();
}

void emAfRf4ceIncomingMessageHandler(uint8_t pairingIndex,
                                     uint8_t profileId,
                                     uint16_t vendorId,
                                     EmberRf4ceTxOption txOptions,
                                     uint8_t messageLength,
                                     const uint8_t *message)
{
  emberAfPushCallbackNetworkIndex();
  SET_PAIRING_INDEX(pairingIndex);

  // GDP 2.0 commands are tricky since they can be sent with the GDP 2.0 profile
  // ID or any GDP 2.0 based profile ID. We want to use the common GDP 2.0 code
  // for the commands that carry the GDP profile ID and other GDP-based profile
  // IDs. Therefore, first we try to dispatch to the GDP callback.
  // This callback is expected to return true if the passed message was
  // processed by the GDP code, false otherwise.
  if (!emberAfPluginRf4ceProfileGdpIncomingMessageCallback(pairingIndex,
                                                           profileId,
                                                           vendorId,
                                                           txOptions,
                                                           message,
                                                           messageLength)) {
    // Dispatch the incoming message based on the profile ID.
    switch (profileId) {
    case EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_1_1:
      emberAfPluginRf4ceProfileRemoteControl11IncomingMessageCallback(pairingIndex,
                                                                      vendorId,
                                                                      txOptions,
                                                                      message,
                                                                      messageLength);
      break;
    case EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0:
      emberAfPluginRf4ceProfileZrc20IncomingMessageCallback(pairingIndex,
                                                            vendorId,
                                                            txOptions,
                                                            message,
                                                            messageLength);
      break;
    case EMBER_AF_RF4CE_PROFILE_MSO:
      emberAfPluginRf4ceProfileMsoIncomingMessageCallback(pairingIndex,
                                                          vendorId,
                                                          txOptions,
                                                          message,
                                                          messageLength);
      break;
    default:
      // TODO: Handle other profiles.
      break;
    }
  }

  emberAfPluginRf4ceProfileIncomingMessageCallback(pairingIndex,
                                                   profileId,
                                                   vendorId,
                                                   txOptions,
                                                   message,
                                                   messageLength);

  UNSET_PAIRING_INDEX();
  emberAfPopNetworkIndex();
}

uint8_t emberAfRf4ceGetPairingIndex(void)
{
  return currentPairingIndex;
}

EmberStatus emberAfRf4ceUnpair(uint8_t pairingIndex)
{
  EmberStatus status = EMBER_INVALID_CALL;
  if (!UNPAIRING_IN_PROGRESS()) {
    status = emberAfRf4cePushNetworkIndex();
    if (status == EMBER_SUCCESS) {
      status = emAfRf4ceUnpair(pairingIndex);
      if (status == EMBER_SUCCESS) {
        SET_UNPAIRING_IN_PROGRESS();
      }
      emberAfPopNetworkIndex();
    }
  }
  return status;
}

void emAfRf4ceUnpairHandler(uint8_t pairingIndex)
{
  emberAfPushCallbackNetworkIndex();
  // TODO
  emberAfPopNetworkIndex();
}

void emAfRf4ceUnpairCompleteHandler(uint8_t pairingIndex)
{
  if (UNPAIRING_IN_PROGRESS()) {
    UNSET_UNPAIRING_IN_PROGRESS();
    emberAfPushCallbackNetworkIndex();
    // TODO
    emberAfPopNetworkIndex();
  }
}

bool emberAfRf4ceIsDeviceTypeSupported(const EmberRf4ceApplicationInfo *appInfo,
                                          EmberAfRf4ceDeviceType deviceType)
{
  uint8_t deviceTypeListLength = emberAfRf4ceDeviceTypeListLength(appInfo->capabilities);
  uint8_t i;

  // The wildcard device type matches everything.
  if (deviceType == EMBER_AF_RF4CE_DEVICE_TYPE_WILDCARD) {
    return true;
  }

  for (i = 0; i < deviceTypeListLength; i++) {
    if (appInfo->deviceTypeList[i] == deviceType) {
      return true;
    }
  }

  return false;
}

bool emberAfRf4ceIsProfileSupported(const EmberRf4ceApplicationInfo *appInfo,
                                       EmberAfRf4ceProfileId profileId)
{
  uint8_t profileIdListLength = emberAfRf4ceProfileIdListLength(appInfo->capabilities);
  uint8_t i;

  for (i = 0; i < profileIdListLength; i++) {
    if (appInfo->profileIdList[i] == profileId) {
      return true;
    }
  }

  return false;
}
