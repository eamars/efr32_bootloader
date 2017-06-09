// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"

#if defined(EMBER_SCRIPTED_TEST)
#include "rf4ce-gdp-test.h"
#endif // EMBER_SCRIPTED_TEST

#include "app/framework/plugin/rf4ce-profile/rf4ce-profile.h"
#include "rf4ce-gdp.h"
#include "rf4ce-gdp-internal.h"
#include "rf4ce-gdp-poll.h"

//------------------------------------------------------------------------------
// Global variables.

// TODO: For now we assume that the node is already up and running, thus we skip
// the 'dormant' status.
uint16_t emAfGdpState = EMBER_AF_RF4CE_GDP_BINDING_STATE_NOT_BOUND;

uint8_t emAfTemporaryPairingIndex = 0xFF;

uint8_t emAfCurrentProfileSpecificIndex = 0xFF;

EmberEventControl emberAfPluginRf4ceGdpPendingCommandEventControl;
EmberEventControl emberAfPluginRf4ceGdpBlackoutTimeEventControl;
EmberEventControl emberAfPluginRf4ceGdpValidationEventControl;

EmAfBindingInfo emAfGdpPeerInfo;

const uint8_t emAfRf4ceGdpApplicationSpecificUserString[USER_STRING_APPLICATION_SPECIFIC_USER_STRING_LENGTH]
            = EMBER_AF_PLUGIN_RF4CE_GDP_APPLICATION_SPECIFIC_USER_STRING;

//------------------------------------------------------------------------------
// Local variables.

static const uint8_t gdp1xProfiles[] = GDP_1_X_BASED_PROFILE_ID_LIST;
static const uint8_t gdp20Profiles[] = GDP_2_0_BASED_PROFILE_ID_LIST;
static uint8_t currentProfileSpecificIndex = 0xFF;

//------------------------------------------------------------------------------
// External declarations.

void emAfRf4ceGdpPairCompleteOriginatorCallback(EmberStatus status,
                                                uint8_t pairingIndex,
                                                const EmberRf4ceVendorInfo *vendorInfo,
                                                const EmberRf4ceApplicationInfo *appInfo);
void emAfRf4ceGdpBasedProfileConfigurationCompleteOriginatorCallback(bool success);
#ifdef EMBER_AF_PLUGIN_RF4CE_GDP_IS_RECIPIENT
void emAfRf4ceGdpPairCompleteRecipientCallback(EmberStatus status,
                                               uint8_t pairingIndex,
                                               const EmberRf4ceVendorInfo *vendorInfo,
                                               const EmberRf4ceApplicationInfo *appInfo);
void emAfRf4ceGdpBasedProfileConfigurationCompleteRecipientCallback(bool success);
#endif

// GDP messages dispatchers.
void emAfRf4ceGdpIncomingGenericResponseOriginatorCallback(EmberAfRf4ceGdpResponseCode status);
void emAfRf4ceGdpIncomingGenericResponsePollNegotiationCallback(EmberAfRf4ceGdpResponseCode responseCode);
void emAfRf4ceGdpIncomingGenericResponseIdentificationCallback(EmberAfRf4ceGdpResponseCode responseCode);
void emAfRf4ceGdpIncomingCheckValidationResponseOriginator(EmberAfRf4ceGdpCheckValidationStatus status);
#ifdef EMBER_AF_PLUGIN_RF4CE_GDP_IS_RECIPIENT
void emAfRf4ceGdpIncomingConfigurationCompleteRecipientCallback(EmberAfRf4ceGdpStatus status);
void emAfRf4ceGdpIncomingCheckValidationRequestRecipientCallback(uint8_t control);
void emAfRf4ceGdpCheckValidationResponseSentRecipient(EmberStatus status);
#endif

// Event handler dispatchers.
void emAfPendingCommandEventHandlerOriginator(void);
void emAfPendingCommandEventHandlerRecipient(void);
void emAfPendingCommandEventHandlerSecurity(void);
void emAfPendingCommandEventHandlerPollNegotiation(void);
void emAfPendingCommandEventHandlerIdentification(void);
void emAfPluginRf4ceGdpBlackoutTimeEventHandlerOriginator(void);
void emAfPluginRf4ceGdpBlackoutTimeEventHandlerRecipient(void);
void emAfPluginRf4ceGdpValidationEventHandlerOriginator(void);
void emAfPluginRf4ceGdpValidationEventHandlerRecipient(void);

//------------------------------------------------------------------------------
// Event handlers.

void emberAfPluginRf4ceGdpPendingCommandEventHandler(void)
{
  emberEventControlSetInactive(emberAfPluginRf4ceGdpPendingCommandEventControl);

  if (isInternalStateBindingOriginator()) {
    emAfPendingCommandEventHandlerOriginator();
  } else if (isInternalStateBindingRecipient()) {
    emAfPendingCommandEventHandlerRecipient();
  } else if (isInternalStateSecurity()) {
    emAfPendingCommandEventHandlerSecurity();
  } else if (isInternalStatePollNegotiation()) {
    emAfPendingCommandEventHandlerPollNegotiation();
  } else if (isInternalStateIdentification()) {
    emAfPendingCommandEventHandlerIdentification();
  } else {
    assert(0);
  }
}

void emberAfPluginRf4ceGdpBlackoutTimeEventHandler(void)
{
  emberEventControlSetInactive(emberAfPluginRf4ceGdpBlackoutTimeEventControl);

  if (isInternalStateBindingOriginator()) {
    emAfPluginRf4ceGdpBlackoutTimeEventHandlerOriginator();
  } else if (isInternalStateBindingRecipient()) {
    emAfPluginRf4ceGdpBlackoutTimeEventHandlerRecipient();
  } else {
    assert(0);
  }
}

void emberAfPluginRf4ceGdpValidationEventHandler(void)
{
  emberEventControlSetInactive(emberAfPluginRf4ceGdpValidationEventControl);

  if (isInternalStateBindingOriginator()) {
    emAfPluginRf4ceGdpValidationEventHandlerOriginator();
  } else if (isInternalStateBindingRecipient()) {
    emAfPluginRf4ceGdpValidationEventHandlerRecipient();
  } else {
    assert(0);
  }
}

//------------------------------------------------------------------------------
// Common callbacks.

// Init callback
void emberAfPluginRf4ceGdpInitCallback(void)
{
  uint8_t i;

  emAfRf4ceGdpAttributesInitCallback();

  // If we detect an entry in 'bound' status with the 'binding complete' not
  // set, we delete the corresponding pairing entry and set the status back to
  // 'not bound'. This would happen if a node is re-binding with a node it
  // already bound previously and it reboots after a temporary pairing is
  // established, before the validation process completes.
  for(i=0; i<EMBER_RF4CE_PAIRING_TABLE_SIZE; i++) {
    uint8_t bindStatus = emAfRf4ceGdpGetPairingBindStatus(i);

    if (((bindStatus & PAIRING_ENTRY_BINDING_STATUS_MASK)
         << PAIRING_ENTRY_BINDING_STATUS_OFFSET)
        != PAIRING_ENTRY_BINDING_STATUS_NOT_BOUND
        && !(bindStatus & PAIRING_ENTRY_BINDING_COMPLETE_BIT)) {
      emberAfRf4ceSetPairingTableEntry(i, NULL);
      emAfRf4ceGdpSetPairingBindStatus(i, (PAIRING_ENTRY_BINDING_STATUS_NOT_BOUND
                                           << PAIRING_ENTRY_BINDING_STATUS_OFFSET));
    }
  }
}

void emberAfPluginRf4ceGdpStackStatusCallback(EmberStatus status)
{
  // Add originator stack status function call here if needed

#ifdef EMBER_AF_PLUGIN_RF4CE_GDP_IS_RECIPIENT
  emAfRf4ceGdpRecipientStackStatusCallback(status);
#endif

  emAfRf4ceGdpPollingStackStatusCallback(status);
}

void emberAfPluginRf4ceProfileGdpAutoDiscoveryResponseCompleteCallback(EmberStatus status,
                                                                       const EmberEUI64 srcIeeeAddr,
                                                                       uint8_t nodeCapabilities,
                                                                       const EmberRf4ceVendorInfo *vendorInfo,
                                                                       const EmberRf4ceApplicationInfo *appInfo,
                                                                       uint8_t searchDevType)
{
}

void emberAfPluginRf4ceProfileGdpPairCompleteCallback(EmberStatus status,
                                                      uint8_t pairingIndex,
                                                      const EmberRf4ceVendorInfo *vendorInfo,
                                                      const EmberRf4ceApplicationInfo *appInfo)
{
  // The originator passes the function pointer for the pairComplete() callback
  // directly in the pair() API.

#ifdef EMBER_AF_PLUGIN_RF4CE_GDP_IS_RECIPIENT
  emAfRf4ceGdpPairCompleteRecipientCallback(status,
                                            pairingIndex,
                                            vendorInfo,
                                            appInfo);
#endif
}

#if defined(EMBER_SCRIPTED_TEST)
bool isZrcTest = false;
#endif

void emberAfRf4ceGdpConfigurationProcedureComplete(bool success)
{
#if defined(EMBER_SCRIPTED_TEST)
  if (isZrcTest) {
    if (success) {
      debugScriptCheck("Configuration succeeded");
    } else {
      debugScriptCheck("Configuration failed");
    }
    return;
  }
#endif // EMBER_SCRIPTED_TEST

  if (isInternalStateBindingOriginator()) {
    emAfRf4ceGdpBasedProfileConfigurationCompleteOriginatorCallback(success);
  }
#ifdef EMBER_AF_PLUGIN_RF4CE_GDP_IS_RECIPIENT
  else if (isInternalStateBindingRecipient()) {
    emAfRf4ceGdpBasedProfileConfigurationCompleteRecipientCallback(success);
  }
#endif
  else {
    assert(0);
  }
}

//------------------------------------------------------------------------------
// Common internal APIs.

void emAfRf4ceGdpUpdatePublicStatus(bool init)
{
  uint8_t i;
  bool bound = false;

  for(i=0; i<EMBER_RF4CE_PAIRING_TABLE_SIZE; i++) {
    uint8_t bindStatus =
        (emAfRf4ceGdpGetPairingBindStatus(i) & PAIRING_ENTRY_BINDING_STATUS_MASK);

    if (bindStatus == PAIRING_ENTRY_BINDING_STATUS_BOUND_RECIPIENT
        || bindStatus == PAIRING_ENTRY_BINDING_STATUS_BOUND_ORIGINATOR) {
      bound = true;
      break;
    }
  }

  // Set the public state to "bound" if there is at least a pairing entry whose
  // status is "bound as recipient" or "bound as originator", otherwise set it
  // to "not bound".
  setPublicState(((bound)
                  ? EMBER_AF_RF4CE_GDP_BINDING_STATE_BOUND
                  : EMBER_AF_RF4CE_GDP_BINDING_STATE_NOT_BOUND),
                 init);
}

bool emAfIsProfileGdpBased(uint8_t profileId, uint8_t gdpCheckVersion)
{
  uint8_t gdpVersion = GDP_VERSION_NONE;
  uint8_t i;

  for(i=0; i<GDP_1_X_BASED_PROFILE_ID_LIST_LENGTH; i++) {
    if (gdp1xProfiles[i] == profileId) {
      gdpVersion = GDP_VERSION_1_X;
    }
  }

  for(i=0; i<GDP_2_0_BASED_PROFILE_ID_LIST_LENGTH; i++) {
    if (gdp20Profiles[i] == profileId) {
      gdpVersion = GDP_VERSION_2_0;
    }
  }

  return (gdpVersion == gdpCheckVersion);
}

// Check that the passed "checkDeviceType" matches one of the device types in
// the passed "compareDeviceTypeList" and that at least one of the profile IDs
// stored in the passed "checkProfileIdList" matches one of the profile IDs
// stored in the passed "compareProfileIdList".
// Returns the number of profile IDs that matched and those IDs are stored in
// the passed "matchingProfileIdList".
uint8_t emAfCheckDeviceTypeAndProfileIdMatch(uint8_t checkDeviceType,
                                           uint8_t *compareDevTypeList,
                                           uint8_t compareDevTypeListLength,
                                           uint8_t *checkProfileIdList,
                                           uint8_t checkProfileIdListLength,
                                           uint8_t *compareProfileIdList,
                                           uint8_t compareProfileIdListLength,
                                           uint8_t *matchingProfileIdList)
{
  uint8_t i, j;
  bool matchFound = false;
  uint8_t matchingProfileIdListLength = 0;

  // Check that "at least one device type contained in the device type list
  // field matches the search device type supplied by the application".
  for(i=0; i<compareDevTypeListLength; i++) {
    if (checkDeviceType == 0xFF  // wildcard
        || checkDeviceType == compareDevTypeList[i]) {
      matchFound = true;
      break;
    }
  }

  if (!matchFound) {
    return 0;
  }

  // Initialize the list of the matching profile IDs to all 0xFFs.
  MEMSET(matchingProfileIdList, 0xFF, EMBER_RF4CE_APPLICATION_PROFILE_ID_LIST_MAX_LENGTH);

  // Check that "at least one profile identifier contained in the profile
  // identifier list matches at least one profile identifier from the discovery
  // profile identifier list supplied by the application.
  for(i=0; i<checkProfileIdListLength; i++) {
    for(j=0; j<compareProfileIdListLength; j++) {
      if (checkProfileIdList[i] == compareProfileIdList[j]) {
        matchingProfileIdList[matchingProfileIdListLength++] =
            checkProfileIdList[i];
      }
    }
  }

  return matchingProfileIdListLength;
}

bool emAfRf4ceIsProfileSupported(uint8_t profileId,
                                    const uint8_t *profileIdList,
                                    uint8_t profileIdListLength)
{
  uint8_t i;

  for(i=0; i<profileIdListLength; i++) {
    if (profileId == profileIdList[i]) {
      return true;
    }
  }

  return false;
}

bool emAfRf4ceGdpMaybeStartNextProfileSpecificConfigurationProcedure(bool isOriginator,
                                                                        const uint8_t *remoteProfileIdList,
                                                                        uint8_t remoteProfileIdListLength)
{
  uint8_t i = ((currentProfileSpecificIndex == 0xFF)
             ? 0
             : currentProfileSpecificIndex + 1);

  for(; i<GDP_2_0_BASED_PROFILE_ID_LIST_LENGTH; i++) {
    // If we find a GDP 2.0 profile ID that is supported by both nodes, start
    // the corresponding configuration procedure.
    if (emAfRf4ceIsProfileSupported(gdp20Profiles[i],
                                    emAfRf4ceApplicationInfo.profileIdList,
                                    emberAfRf4ceProfileIdListLength(emAfRf4ceApplicationInfo.capabilities))
        && emAfRf4ceIsProfileSupported(gdp20Profiles[i],
                                       remoteProfileIdList,
                                       remoteProfileIdListLength)) {
      bool configurationProcedureStarted = false;
      switch(gdp20Profiles[i]) {
      case EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0:
        configurationProcedureStarted =
            emberAfPluginRf4ceGdpZrc20StartConfigurationCallback(isOriginator,
                                                                 emAfTemporaryPairingIndex);
        break;
      default:
        assert(0);
      }

      if (configurationProcedureStarted) {
        currentProfileSpecificIndex = i;
        return true;
      }
    }
  }

  currentProfileSpecificIndex = 0xFF;
  return false;
}

void emAfRf4ceGdpNotifyBindingCompleteToProfiles(EmberAfRf4ceGdpBindingStatus status,
                                                 uint8_t pairingIndex,
                                                 const uint8_t *remoteProfileIdList,
                                                 uint8_t remoteProfileIdListLength)
{
  uint8_t i;
  for(i=0; i<GDP_2_0_BASED_PROFILE_ID_LIST_LENGTH; i++) {
    // If we find a GDP 2.0 profile ID that is supported by both nodes, we
    // notify the profile.
    if (emAfRf4ceIsProfileSupported(gdp20Profiles[i],
                                    emAfRf4ceApplicationInfo.profileIdList,
                                    emberAfRf4ceProfileIdListLength(emAfRf4ceApplicationInfo.capabilities))
        && emAfRf4ceIsProfileSupported(gdp20Profiles[i],
                                       remoteProfileIdList,
                                       remoteProfileIdListLength)) {
      switch(gdp20Profiles[i]) {
      case EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0:
        emberAfPluginRf4ceGdpZrc20BindingCompleteCallback(status, pairingIndex);
        break;
      default:
        assert(0);
      }
    }
  }
}

void emAfRf4ceGdpNoteProfileSpecificConfigurationStart(void)
{
  currentProfileSpecificIndex = 0xFF;
}

// It adds to the profile ID list of destAppInfo the profile IDs in srcAppInfo
// that are based on the passed GDP version (it never includes the GDP profile
// 0x00).
void emAfGdpAddToProfileIdList(uint8_t *srcProfileIdList,
                               uint8_t srcProfileIdListLength,
                               EmberRf4ceApplicationInfo *destAppInfo,
                               uint8_t gdpVersion)
{
  uint8_t i;
  uint8_t profileIdCount =
      emberAfRf4ceProfileIdListLength(destAppInfo->capabilities);

  for(i=0; i<srcProfileIdListLength; i++) {
    // Never include the GDP 2.0 profile ID.
    if (srcProfileIdList[i] == EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE) {
      continue;
    }

    if (emAfIsProfileGdpBased(srcProfileIdList[i], gdpVersion)) {
      destAppInfo->profileIdList[profileIdCount++] = srcProfileIdList[i];
    }
  }

  // Set the number of supported profiles in the appInfo capabilities byte.
  destAppInfo->capabilities &=
      ~EMBER_RF4CE_APP_CAPABILITIES_SUPPORTED_PROFILES_MASK;
  destAppInfo->capabilities |=
      (profileIdCount << EMBER_RF4CE_APP_CAPABILITIES_SUPPORTED_PROFILES_OFFSET);
}

// - If the profile ID list contains at least a GDP 2.0 profile ID, it returns
//   GDP 2.0.
// - If the profile ID list contains at least a GDP 1.x profile ID, it returns
//   GDP 1.x.
// - Otherwise returns nonGDP.
uint8_t emAfRf4ceGdpGetGdpVersion(const uint8_t *profileIdList,
                                uint8_t profileIdListLength)
{
  uint8_t i;

  // Check if there is at least one profile ID that is GDP 2.0 based first.
 for(i=0; i<profileIdListLength; i++) {
   // At least one of the matching profile IDs is GDP 2.0 based
   if (emAfIsProfileGdpBased(profileIdList[i], GDP_VERSION_2_0)) {
     return GDP_VERSION_2_0;
   }
 }

 // No GDP 2.0 based matching profile ID. Check whether at least one of the
 // matching profile IDs is GDP 1.x based.
 for(i=0; i<profileIdListLength; i++) {
   // At least one of the matching profile IDs is GDP 1.x based
   if (emAfIsProfileGdpBased(profileIdList[i], GDP_VERSION_1_X)) {
     return GDP_VERSION_1_X;
   }
 }

 return GDP_VERSION_NONE;
}

void emAfGdpStartBlackoutTimer(uint8_t state)
{
  setInternalState(state);
  emberEventControlSetDelayMS(emberAfPluginRf4ceGdpBlackoutTimeEventControl,
                              isInternalStateBindingOriginator()
                              ? BLACKOUT_TIME_ORIGINATOR_MS
                              : BLACKOUT_TIME_RECIPIENT_MS);
}

void emAfGdpStartCommandPendingTimer(uint8_t state, uint16_t timeMs)
{
  setInternalState(state);
  emberEventControlSetDelayMS(emberAfPluginRf4ceGdpPendingCommandEventControl,
                              timeMs);
}

//------------------------------------------------------------------------------
// Commands dispatchers (originator/recipient).

void emAfRf4ceGdpIncomingGenericResponse(EmberAfRf4ceGdpResponseCode responseCode)
{
  if (isInternalStatePollNegotiation()) {
    emAfRf4ceGdpIncomingGenericResponsePollNegotiationCallback(responseCode);
  } else if (isInternalStateIdentification()) {
    emAfRf4ceGdpIncomingGenericResponseIdentificationCallback(responseCode);
  } else {
    emAfRf4ceGdpIncomingGenericResponseOriginatorCallback(responseCode);
  }
}

void emAfRf4ceGdpIncomingConfigurationComplete(EmberAfRf4ceGdpStatus status)
{
  // Incoming configuration complete messages are always dispatched to the
  // recipient code.
#ifdef EMBER_AF_PLUGIN_RF4CE_GDP_IS_RECIPIENT
  emAfRf4ceGdpIncomingConfigurationCompleteRecipientCallback(status);
#endif
}

void emAfRf4ceGdpIncomingCheckValidationRequest(uint8_t control)
{
#ifdef EMBER_AF_PLUGIN_RF4CE_GDP_IS_RECIPIENT
  emAfRf4ceGdpIncomingCheckValidationRequestRecipientCallback(control);
#endif
}

void emAfRf4ceGdpIncomingCheckValidationResponse(EmberAfRf4ceGdpCheckValidationStatus status)
{
  emAfRf4ceGdpIncomingCheckValidationResponseOriginator(status);
}

void emAfRf4ceGdpCheckValidationResponseSent(EmberStatus status)
{
#ifdef EMBER_AF_PLUGIN_RF4CE_GDP_IS_RECIPIENT
  emAfRf4ceGdpCheckValidationResponseSentRecipient(status);
#endif
}

#if defined(EMBER_SCRIPTED_TEST)
void setBindOriginatorState(uint8_t state)
{
  emAfGdpState = state;
  if (state == EMBER_AF_RF4CE_GDP_BINDING_STATE_NOT_BOUND) {
    emAfTemporaryPairingIndex = 0xFF;
  }
}
#endif
