// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"

#if defined(EMBER_SCRIPTED_TEST)
#include "rf4ce-gdp-test.h"
#endif // EMBER_SCRIPTED_TEST

#include "app/framework/plugin/rf4ce-profile/rf4ce-profile.h"
#include "rf4ce-gdp.h"
#include "rf4ce-gdp-attributes.h"
#include "rf4ce-gdp-poll.h"
#include "rf4ce-gdp-identification.h"
#include "rf4ce-gdp-internal.h"

//------------------------------------------------------------------------------
// Local variables.

#ifdef EMBER_AF_PLUGIN_RF4CE_GDP_IS_RECIPIENT
static EmAfDiscoveryOrPairRequestData savedDiscoveryOrPairRequest;
static bool savedDiscoveryRequestValidOrPairCompletePending = false;
static EmberAfRf4ceGdpCheckValidationStatus validationStatus;
static EmberRf4cePairingTableEntry pairingTable[EMBER_RF4CE_PAIRING_TABLE_SIZE];
#endif

//------------------------------------------------------------------------------
// Forward declarations.

#ifdef EMBER_AF_PLUGIN_RF4CE_GDP_IS_RECIPIENT
static bool handleGdp20BasedDiscoveryRequest(const EmberEUI64 srcIeeeAddr,
                                                uint8_t nodeCapabilities,
                                                const EmberRf4ceVendorInfo *vendorInfo,
                                                const EmberRf4ceApplicationInfo *appInfo,
                                                uint8_t searchDevType,
                                                uint8_t rxLinkQuality);

// TODO:
static bool handleGdp1xBasedDiscoveryRequest(void);

static bool handleNonGdpBasedDiscoveryRequest(const uint8_t *profileIdList,
                                                 uint8_t profileIdListLength);

static void clearSavedDiscoveryRequest(void);

static void startValidationProcedure(void);

static void finishBindingProcedure(bool success);

static void syncLocalPairingTable(void);

static uint8_t getLocalPrimaryClassNumber(void);
#endif

//------------------------------------------------------------------------------
// Events (and corresponding handlers) should always be declared.

EmberEventControl emberAfPluginRf4ceGdpPushButtonEventControl;

#define pushButtonStimulusReceivedPending()                                    \
  (emberEventControlGetActive(emberAfPluginRf4ceGdpPushButtonEventControl))

// Turn the radio off when the button stimulus received pending flag is cleared.
#define clearButtonStimulusReceivedPendingFlag()                               \
  do {                                                                         \
    emberEventControlSetInactive(emberAfPluginRf4ceGdpPushButtonEventControl); \
    emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE, false);        \
    emAfRf4ceGdpSetPushButtonPendingReceivedFlag(false);                       \
  } while(0)

void emberAfPluginRf4ceGdpPushButtonEventHandler(void)
{
  clearButtonStimulusReceivedPendingFlag();
}

void emAfPendingCommandEventHandlerRecipient(void)
{
#ifdef EMBER_AF_PLUGIN_RF4CE_GDP_IS_RECIPIENT
  switch(internalGdpState()) {
  // GDP configuration procedure failed.
  case INTERNAL_STATE_RECIPIENT_GDP_CONFIG_WAITING_FOR_PUSH:
  case INTERNAL_STATE_RECIPIENT_GDP_CONFIG_WAITING_FOR_GET:
  case INTERNAL_STATE_RECIPIENT_GDP_CONFIG_WAITING_FOR_PULL:
  case INTERNAL_STATE_RECIPIENT_GDP_CONFIG_WAITING_FOR_COMPLETE:
    debugScriptCheck("Configuration procedure failed");
    finishBindingProcedure(false);
    break;
  case INTERNAL_STATE_RECIPIENT_GDP_VALIDATION:
    // If the aplLinkLostWaitTime timeout expires, the binding originator or
    // recipient detects that it has lost its link with the remote node and
    // the validation procedure is unsuccessful.
    //
    // If the validation procedure was unsuccessful, the binding recipient
    // shall remove its pairing entry (and potentially restore the old one)
    // when the aplLinkLostWaitTime timer has expired.
    debugScriptCheck("Link lost, validation failed");
    finishBindingProcedure(false);
    break;
  default:
    assert(0);
  }
#endif
}

void emAfPluginRf4ceGdpBlackoutTimeEventHandlerRecipient(void)
{
#ifdef EMBER_AF_PLUGIN_RF4CE_GDP_IS_RECIPIENT
  switch(internalGdpState()) {
  case INTERNAL_STATE_RECIPIENT_GDP_RESTORE_PAIRING_ENTRY_PENDING:
    finishBindingProcedure(false);
    break;
  case INTERNAL_STATE_RECIPIENT_GDP_PROFILES_CONFIG:
    // Start the (next) profile-specific configuration procedure if any,
    // otherwise move to the validation procedure.
    if (!emAfRf4ceGdpMaybeStartNextProfileSpecificConfigurationProcedure(false, // isOriginator
                                                                         savedDiscoveryOrPairRequest.appInfo.profileIdList,
                                                                         emberAfRf4ceProfileIdListLength(savedDiscoveryOrPairRequest.appInfo.capabilities))) {
      startValidationProcedure();
    }
    break;
  default:
    assert(0);
  }
#endif
}

void emAfPluginRf4ceGdpValidationEventHandlerRecipient(void)
{
#ifdef EMBER_AF_PLUGIN_RF4CE_GDP_IS_RECIPIENT
  // According to the specs, we don't remove the temporary pairing here. We just
  // leave everything as is. If we receive a new CheckValidationRequest we will
  // now reply with a 'timeout' status since this timer is expired. If this is
  // the case, and the 'timeout' response is correctly delivered, we will then
  // remove the pairing.
#endif
}

#ifdef EMBER_AF_PLUGIN_RF4CE_GDP_IS_RECIPIENT

//------------------------------------------------------------------------------
// Init and StackStatus callbacks.

void emAfRf4ceGdpRecipientInitCallback(void)
{
  emAfRf4ceGdpUpdatePublicStatus(true);
  // Set the state machine to wait for the stack status handler.
  setInternalState(INTERNAL_STATE_RECIPIENT_GDP_STACK_STATUS_NETWORK_UP_PENDING);
}

// Populate the local copy of the pairing table.
void emAfRf4ceGdpRecipientStackStatusCallback(EmberStatus status)
{
  if (status == EMBER_NETWORK_UP) {
    syncLocalPairingTable();
    setInternalState(INTERNAL_STATE_NONE);
  }
}

//------------------------------------------------------------------------------
// Public APIs.

void emberAfRf4ceGdpPushButton(bool setPending)
{
  // If a push button stimulus is received at the binding recipient, the "push
  // button stimulus received" flag becomes pending. If no other push button
  // stimuli are received later, this flag shall be cleared automatically
  // after aplcBindWindowDuration. The flag shall also be cleared [...] and may
  // also be cleared on request of the application.
  if (setPending) {
    clearSavedDiscoveryRequest();
    emberEventControlSetDelayMS(emberAfPluginRf4ceGdpPushButtonEventControl,
                                APLC_BIND_WINDOW_DURATION_MS);
  } else {
    clearButtonStimulusReceivedPendingFlag();
  }

  // If the target is currently in power saving mode and the application is
  // setting the button stimulus flag, this call causes the node to exit the
  // power mode and start accepting incoming discovery and pair requests.
  emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE, setPending);

  emAfRf4ceGdpSetPushButtonPendingReceivedFlag(setPending);
}

void emberAfRf4ceGdpSetValidationStatus(EmberAfRf4ceGdpCheckValidationStatus status)
{
  validationStatus = status;
}

//------------------------------------------------------------------------------
// Callbacks.

bool emberAfPluginRf4ceProfileGdpDiscoveryRequestCallback(const EmberEUI64 srcIeeeAddr,
                                                             uint8_t nodeCapabilities,
                                                             const EmberRf4ceVendorInfo *vendorInfo,
                                                             const EmberRf4ceApplicationInfo *appInfo,
                                                             uint8_t searchDevType,
                                                             uint8_t rxLinkQuality)
{
  uint8_t gdpVersion;
  uint8_t matchingProfileIdList[EMBER_RF4CE_APPLICATION_PROFILE_ID_LIST_MAX_LENGTH];
  uint8_t matchingProfileIdListLength;

  // On receipt of a discovery request command frame, the binding recipient
  // shall check that the requested device type field matches one of the device
  // types supplied by the application and that at least one of the profile IDs
  // listed in the profile IDs list matches one of the profile IDs supplied by
  // the application.
  matchingProfileIdListLength =
      emAfCheckDeviceTypeAndProfileIdMatch(searchDevType,
                                           (uint8_t*)emAfRf4ceApplicationInfo.deviceTypeList,
                                           emberAfRf4ceDeviceTypeListLength(emAfRf4ceApplicationInfo.capabilities),
                                           (uint8_t*)appInfo->profileIdList,
                                           emberAfRf4ceProfileIdListLength(appInfo->capabilities),
                                           (uint8_t*)emAfRf4ceApplicationInfo.profileIdList,
                                           emberAfRf4ceProfileIdListLength(emAfRf4ceApplicationInfo.capabilities),
                                           matchingProfileIdList);

  if (matchingProfileIdListLength == 0) {
    return false;
  }

  gdpVersion = emAfRf4ceGdpGetGdpVersion(matchingProfileIdList,
                                         matchingProfileIdListLength);

  if (gdpVersion == GDP_VERSION_2_0) {
    return handleGdp20BasedDiscoveryRequest(srcIeeeAddr,
                                            nodeCapabilities,
                                            vendorInfo,
                                            appInfo,
                                            searchDevType,
                                            rxLinkQuality);
  } else if (gdpVersion == GDP_VERSION_1_X) {
    return handleGdp1xBasedDiscoveryRequest();
  } else {
    return handleNonGdpBasedDiscoveryRequest(matchingProfileIdList,
                                             matchingProfileIdListLength);
  }
}

bool emberAfPluginRf4ceProfileGdpPairRequestCallback(EmberStatus status,
                                                        uint8_t pairingIndex,
                                                        const EmberEUI64 sourceIeeeAddr,
                                                        uint8_t nodeCapabilities,
                                                        const EmberRf4ceVendorInfo *vendorInfo,
                                                        const EmberRf4ceApplicationInfo *appInfo,
                                                        uint8_t keyExchangeTransferCount)
{
  uint8_t gdpVersion = emAfRf4ceGdpGetGdpVersion(appInfo->profileIdList,
                                               emberAfRf4ceProfileIdListLength(appInfo->capabilities));

  bool bindingProxySupported = ((gdpVersion == GDP_VERSION_2_0)
                                   ? ((appInfo->userString[USER_STRING_PAIR_REQUEST_ADVANCED_BINDING_SUPPORT_OFFSET]
                                       & ADVANCED_BINDING_SUPPORT_FIELD_BINDING_PROXY_SUPPORTED_BIT) > 0)
                                   : false);

  savedDiscoveryRequestValidOrPairCompletePending = false;

  // The procedure for deciding whether to respond to a pair request is out of
  // the scope of the specification.
  // TODO: we might want to add a callback to let the application decide.

  // - If the notification indicates a keyExchange transfer count that was less
  //   than aplcMinKeyExchangeTransferCount, the application shall respond
  //   indicating that the pair request was not permitted and perform no further
  //   processing.
  // - If the notification indicates that there is no capacity for the new entry
  //   in the table, the application shall respond indicating an identical
  //   (i.e., no capacity) status and perform no further processing.
  // - If the notification indicates that the pair request corresponded to a
  //   duplicate entry, the application shall respond with successful status.
  if (keyExchangeTransferCount >= APLC_MIN_KEY_EXCHANGE_TRANSFER_COUNT
      && (status == EMBER_SUCCESS || status == EMBER_DUPLICATE_ENTRY)) {

    // Section 7.3: "The binding recipient, upon receiving a pair request with
    // the binding proxy supported bit set, shall verify that the binding
    // recipient and the binding originator have communicated through an out of
    // band means, and the binding recipient allows it to pair via binding
    // proxy. If the binding recipient receives an unsolicited pair request
    // with the binding proxy bit set, and has no record of allowing the binding
    // originator to bind via binding proxy, it shall ignore the request.
    if (bindingProxySupported
        && !emberAfPluginRf4ceGdpIncomingBindProxyCallback(sourceIeeeAddr)) {
      goto pairRequestDiscarded;
    }

    // Save the app info.
    MEMMOVE(&savedDiscoveryOrPairRequest.appInfo,
            appInfo,
            sizeof(EmberRf4ceApplicationInfo));

    // TODO: The application info might be slightly different if the originator
    // supported binding proxy.
    // From section 7.3: "The profile ID of the pair response shall contain all
    // the GDP 2.0-based profile IDs that are supported by the binding initiator
    // or a subset of them if configured so via the out of band means".
    if (emAfRf4ceGdpSetPairResponseAppInfo(appInfo) == EMBER_SUCCESS) {
      savedDiscoveryRequestValidOrPairCompletePending = true;
      return true;
    }
    // Fall-through
  }

pairRequestDiscarded:
  if (status == EMBER_DUPLICATE_ENTRY) {
    // Schedule event (we reuse the blackout event) and set a state for
    // restoring the pairing entry.
    setInternalState(INTERNAL_STATE_RECIPIENT_GDP_RESTORE_PAIRING_ENTRY_PENDING);
    emberEventControlSetActive(emberAfPluginRf4ceGdpBlackoutTimeEventControl);
    emAfTemporaryPairingIndex = pairingIndex;
  }
  return false;
}

void emAfRf4ceGdpPairCompleteRecipientCallback(EmberStatus status,
                                               uint8_t pairingIndex,
                                               const EmberRf4ceVendorInfo *vendorInfo,
                                               const EmberRf4ceApplicationInfo *appInfo)
{
  if (savedDiscoveryRequestValidOrPairCompletePending) {
    emAfTemporaryPairingIndex = pairingIndex;

    if (status == EMBER_SUCCESS || status == EMBER_DUPLICATE_ENTRY) {
      // First, we clear the remote node attributes
      emAfRf4ceGdpClearRemoteAttributes();

      // If this is a GDP 1.x or a nonGDP pair complete, we don't go any further
      // into configuration/validation.
      if (emAfRf4ceGdpGetGdpVersion(appInfo->profileIdList,
                                    emberAfRf4ceProfileIdListLength(appInfo->capabilities))
          != GDP_VERSION_2_0) {
        finishBindingProcedure(true);
        return;
      }

      // The recipient is now engaged in the binding process.
      setPublicState(EMBER_AF_RF4CE_GDP_BINDING_STATE_BINDING, false);

      // Clear the "binding complete" bit in the corresponding pairing entry status
      // table.
      emAfRf4ceGdpSetPairingBindStatus(emAfTemporaryPairingIndex,
          (emAfRf4ceGdpGetPairingBindStatus(emAfTemporaryPairingIndex)
           & ~PAIRING_ENTRY_BINDING_COMPLETE_BIT));

      // Start the GDP configuration procedure. Wait for a pushAttributes
      // command that contains both aplGDPVersion and aplGDPCapabilities
      // attributes. We need to account for the blackout period at the
      // originator here.
      emAfGdpStartCommandPendingTimer(INTERNAL_STATE_RECIPIENT_GDP_CONFIG_WAITING_FOR_PUSH,
                                      APLC_MAX_CONFIG_WAIT_TIME_MS + BLACKOUT_TIME_RECIPIENT_MS);
    } else {
      // This is to restore a potential existing pairing entry that would be
      // wiped out from the current (failed) pairing process.
      finishBindingProcedure(false);
    }
  }
}

bool emAfRf4ceGdpIncomingPushAttributesRecipientCallback(void)
{
  EmberAfRf4ceGdpAttributeRecord versionRecord;
  EmberAfRf4ceGdpAttributeRecord capabilitiesRecord;

  if (internalGdpState()
      == INTERNAL_STATE_RECIPIENT_GDP_CONFIG_WAITING_FOR_PUSH
      && emAfTemporaryPairingIndex == emberAfRf4ceGetPairingIndex()
      && emAfRf4ceGdpFetchAttributeRecord(&versionRecord)
      && versionRecord.attributeId == EMBER_AF_RF4CE_GDP_ATTRIBUTE_VERSION
      && emAfRf4ceGdpFetchAttributeRecord(&capabilitiesRecord)
      && capabilitiesRecord.attributeId
         == EMBER_AF_RF4CE_GDP_ATTRIBUTE_CAPABILITIES) {
    // Wait for GetAttributes from the originator.
    emAfGdpStartCommandPendingTimer(INTERNAL_STATE_RECIPIENT_GDP_CONFIG_WAITING_FOR_GET,
                                    APLC_MAX_CONFIG_WAIT_TIME_MS);
    return true;
  }

  return false;
}

bool emAfRf4ceGdpIncomingGetAttributesRecipientCallback(void)
{
  EmberAfRf4ceGdpAttributeIdentificationRecord tempIdRecord;

  if (internalGdpState() == INTERNAL_STATE_RECIPIENT_GDP_CONFIG_WAITING_FOR_GET
      && emAfTemporaryPairingIndex == emberAfRf4ceGetPairingIndex()
      // Check 'version' attribute
      && emAfRf4ceGdpFetchAttributeIdentificationRecord(&tempIdRecord)
      && tempIdRecord.attributeId == EMBER_AF_RF4CE_GDP_ATTRIBUTE_VERSION
      // Check 'capabilities' attribute
      && emAfRf4ceGdpFetchAttributeIdentificationRecord(&tempIdRecord)
      && tempIdRecord.attributeId == EMBER_AF_RF4CE_GDP_ATTRIBUTE_CAPABILITIES) {
    // Wait for PullAttributes from the originator.
    setInternalState(INTERNAL_STATE_RECIPIENT_GDP_CONFIG_WAITING_FOR_PULL);
    emAfGdpStartCommandPendingTimer(INTERNAL_STATE_RECIPIENT_GDP_CONFIG_WAITING_FOR_PULL,
                                    APLC_MAX_CONFIG_WAIT_TIME_MS);
    return true;
  }

  return false;
}

bool emAfRf4ceGdpIncomingPullAttributesRecipientCallback(void)
{
  EmberAfRf4ceGdpAttributeIdentificationRecord tempIdRecord;
  uint8_t pairingIndex = emberAfRf4ceGetPairingIndex();

  if (internalGdpState()
      == INTERNAL_STATE_RECIPIENT_GDP_CONFIG_WAITING_FOR_PULL
      && emAfTemporaryPairingIndex == pairingIndex
      && emAfRf4ceGdpFetchAttributeIdentificationRecord(&tempIdRecord)
      && tempIdRecord.attributeId
         == EMBER_AF_RF4CE_GDP_ATTRIBUTE_AUTO_CHECK_VALIDATION_PERIOD
      && emAfRf4ceGdpFetchAttributeIdentificationRecord(&tempIdRecord)
      && tempIdRecord.attributeId
         == EMBER_AF_RF4CE_GDP_ATTRIBUTE_LINK_LOST_WAIT_TIME) {

    // Initialize the aplAutoCheckValidationPeriod and aplLinkLostWaitTime
    // remote attributes.
    emAfRf4ceGdpRemoteNodeAttributes.autoCheckValidationPeriod =
        APL_GDP_AUTO_CHECK_VALIDATION_PERIOD_DEFAULT;
    emAfRf4ceGdpRemoteNodeAttributes.linkLostWaitTime =
        APL_GDP_LINK_LOST_WAIT_TIME_DEFAULT;

    // Wait for ConfigurationComplete from the originator.
    emAfGdpStartCommandPendingTimer(INTERNAL_STATE_RECIPIENT_GDP_CONFIG_WAITING_FOR_COMPLETE,
                                    APLC_MAX_CONFIG_WAIT_TIME_MS);
    return true;
  }

  return false;
}

void emAfRf4ceGdpIncomingConfigurationCompleteRecipientCallback(EmberAfRf4ceGdpStatus status)
{
  if (internalGdpState()
      == INTERNAL_STATE_RECIPIENT_GDP_CONFIG_WAITING_FOR_COMPLETE
      && emAfTemporaryPairingIndex == emberAfRf4ceGetPairingIndex()) {

    // TODO: check the retrieved peer attributes and set the response code
    // accordingly. For now we just accept any value, we need to clarify what
    // are the criteria for deciding whether the retrieved parameters are
    // satisfactory or not.
    emAfGdpPeerInfo.localConfigurationStatus =
        EMBER_AF_RF4CE_GDP_RESPONSE_CODE_SUCCESSFUL;

    // Send a generic response.
    emberAfRf4ceGdpGenericResponse(emAfTemporaryPairingIndex,
                                           EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE,
                                           EMBER_RF4CE_NULL_VENDOR_ID,
                                           emAfGdpPeerInfo.localConfigurationStatus);
    if (emAfGdpPeerInfo.localConfigurationStatus
        == EMBER_AF_RF4CE_GDP_RESPONSE_CODE_SUCCESSFUL
        && status == EMBER_AF_RF4CE_GDP_STATUS_SUCCESSFUL) {
        // If both the Configuration Complete command frame and the
        // corresponding Generic Response frame have their status field set to
        // SUCCESS, the binding recipient shall consider the configuration a
        // success. Otherwise it shall consider the configuration a failure.
        debugScriptCheck("GDP configuration procedure completed");

        emberEventControlSetInactive(emberAfPluginRf4ceGdpPendingCommandEventControl);

        // Configuration procedure has been completed successfully. Move to the
        // profile-specific configuration procedures.
        // After pairing and after the configuration procedure of each profile the
        // node shall wait for aplcConfigBlackoutTime.
        emAfGdpStartBlackoutTimer(INTERNAL_STATE_RECIPIENT_GDP_PROFILES_CONFIG);

        emAfRf4ceGdpNoteProfileSpecificConfigurationStart();
    }

    // If either of the two states is not successful, we fall through here.
    // Eventually we will timeout and declare the configuration procedure failed.
  }
}

void emAfRf4ceGdpBasedProfileConfigurationCompleteRecipientCallback(bool success)
{
  if (success) {
    // After pairing and after the configuration procedure of each profile the
    // node shall wait for aplcConfigBlackoutTime.
    emAfGdpStartBlackoutTimer(internalGdpState());

  } else {
    debugScriptCheck("Profile-specific configuration procedure failed");
    finishBindingProcedure(false);
  }
}

// On receipt of a checkValidationRequest command frame, the binding
// recipient checks the result of the validation. The binding recipient
// shall generate and transmit a checkValidationResponse command frame to the
// binding originator containing the result of the validation.
void emAfRf4ceGdpIncomingCheckValidationRequestRecipientCallback(uint8_t control)
{
  EmberAfRf4ceGdpCheckValidationStatus status;
  uint8_t bindStatus =
      emAfRf4ceGdpGetPairingBindStatus(emberAfRf4ceGetPairingIndex());

  // The binding recipient shall start the aplLinkLostWaitTime timeout when
  // the validation phase starts. If a check validation request command frame
  // is received from the binding originator, the timeout shall restarted.
  // We reuse the commandPending event for handling this.
  emberEventControlSetDelayMS(emberAfPluginRf4ceGdpPendingCommandEventControl,
                              emAfRf4ceGdpLocalNodeAttributes.linkLostWaitTime);

  // If the binding recipient receives a CheckValidationRequest later on from a
  // binding initiator that was successfully validated before and we are not in
  // the middle of a new validation procedure, it shall respond with a
  // CheckValidationResponse command containing the 'success' status.
  if ((bindStatus & PAIRING_ENTRY_BINDING_STATUS_MASK)
       == PAIRING_ENTRY_BINDING_STATUS_BOUND_RECIPIENT
      && (bindStatus & PAIRING_ENTRY_BINDING_COMPLETE_BIT)) {
      status = EMBER_AF_RF4CE_GDP_CHECK_VALIDATION_STATUS_SUCCESS;
  }
  // If the "push button stimulus received" flag is pending at the binding
  // recipient, the validation shall be successful.
  // Also respond with success if the pair request had the proxy supported bit
  // set in the advanced binding support field.
  else if (pushButtonStimulusReceivedPending()
           || (savedDiscoveryOrPairRequest.appInfo.userString[USER_STRING_PAIR_REQUEST_ADVANCED_BINDING_SUPPORT_OFFSET]
               & ADVANCED_BINDING_SUPPORT_FIELD_BINDING_PROXY_SUPPORTED_BIT) > 0) {
    status = EMBER_AF_RF4CE_GDP_CHECK_VALIDATION_STATUS_SUCCESS;
  }
  // If the validation phase is not completed in the
  // aplBindingRecipientValidationTime interval, the validation shall be
  // unsuccessful (timeout).
  else if (!emberEventControlGetActive(emberAfPluginRf4ceGdpValidationEventControl)) {
    status = EMBER_AF_RF4CE_GDP_CHECK_VALIDATION_STATUS_TIMEOUT;
  }
  // A binding recipient may also terminate the validation procedure on its own
  // behalf. In this case the validation is unsuccessful (failure).
  // This is done at the application by calling
  // emAfRf4ceGdpSetValidationStatus() with FAILURE status.
  else {
    status = validationStatus;
  }

  emberAfRf4ceGdpCheckValidationResponse(emberAfRf4ceGetPairingIndex(),
                                         EMBER_RF4CE_NULL_VENDOR_ID,
                                         status);

  // We save the status in the static variable validationStatus for future use.
  validationStatus = status;
}

void emAfRf4ceGdpCheckValidationResponseSentRecipient(EmberStatus status)
{
  // If the validation procedure was successful, the binding recipient shall
  // send a CheckValidationResponse command. After receiving the
  // NLDE-DATA.confirm, independent of the status, the binding procedure for the
  // binding recipient is completed.
  //
  // If the validation procedure was unsuccessful, the binding recipient
  // shall remove its pairing entry (and potentially restore the old one)
  // if the recipient has received a NLDE-DATA.confirm with status 'success'
  // and and the validation status was 'failure' or 'timeout'.

  if (validationStatus == EMBER_AF_RF4CE_GDP_CHECK_VALIDATION_STATUS_SUCCESS
      || (status == EMBER_SUCCESS
          && (validationStatus
              == EMBER_AF_RF4CE_GDP_CHECK_VALIDATION_STATUS_TIMEOUT
              || validationStatus
                 == EMBER_AF_RF4CE_GDP_CHECK_VALIDATION_STATUS_FAILURE))) {
    finishBindingProcedure(validationStatus
                           == EMBER_AF_RF4CE_GDP_CHECK_VALIDATION_STATUS_SUCCESS);
  }
}

//------------------------------------------------------------------------------
// Static functions.

static bool handleGdp1xBasedDiscoveryRequest(void)
{
  debugScriptCheck("handleGdp1xBasedDiscoveryRequest");

  // TODO (make sure the discovery response goes out with only the GDP 1.x based
  // profile IDs in the profile ID list).

  return true;
}

static void clearSavedDiscoveryRequest(void)
{
  savedDiscoveryRequestValidOrPairCompletePending = false;
}

// Returns true if the passed struct is identical to the saved discovery data.
static bool discoveryRequestDataCompare(EmAfDiscoveryOrPairRequestData *discData)
{
  return (MEMCOMPARE(discData->srcIeeeAddr,
                     savedDiscoveryOrPairRequest.srcIeeeAddr,
                     EUI64_SIZE) == 0
          && discData->vendorInfo.vendorId ==
              savedDiscoveryOrPairRequest.vendorInfo.vendorId
          && MEMCOMPARE(discData->vendorInfo.vendorString,
                        savedDiscoveryOrPairRequest.vendorInfo.vendorString,
                        EMBER_RF4CE_VENDOR_STRING_LENGTH) == 0
          && discData->appInfo.capabilities ==
              savedDiscoveryOrPairRequest.appInfo.capabilities
          && MEMCOMPARE(discData->appInfo.userString,
                        savedDiscoveryOrPairRequest.appInfo.userString,
                        EMBER_RF4CE_APPLICATION_USER_STRING_LENGTH) == 0
          && MEMCOMPARE(discData->appInfo.deviceTypeList,
                        savedDiscoveryOrPairRequest.appInfo.deviceTypeList,
                        EMBER_RF4CE_APPLICATION_DEVICE_TYPE_LIST_MAX_LENGTH) == 0
         && MEMCOMPARE(discData->appInfo.profileIdList,
                       savedDiscoveryOrPairRequest.appInfo.profileIdList,
                       EMBER_RF4CE_APPLICATION_PROFILE_ID_LIST_MAX_LENGTH) == 0
         && discData->searchDevType == savedDiscoveryOrPairRequest.searchDevType);
}

static bool handleGdp20BasedDiscoveryRequest(const EmberEUI64 srcIeeeAddr,
                                                uint8_t nodeCapabilities,
                                                const EmberRf4ceVendorInfo *vendorInfo,
                                                const EmberRf4ceApplicationInfo *appInfo,
                                                uint8_t searchDevType,
                                                uint8_t rxLinkQuality)
{
  debugScriptCheck("handleGdp20BasedDiscoveryRequest");

  EmAfDiscoveryOrPairRequestData tempDiscoveryData;
  uint16_t vendorIdFilter =
      HIGH_LOW_TO_INT(appInfo->userString[USER_STRING_DISC_REQUEST_VENDOR_ID_FILTER_OFFSET + 1],
                      appInfo->userString[USER_STRING_DISC_REQUEST_VENDOR_ID_FILTER_OFFSET]);
  uint8_t minClassFilter =
    ((appInfo->userString[USER_STRING_DISC_REQUEST_MIN_MAX_CLASS_FILTER_OFFSET]
      & USER_STRING_DISC_REQUEST_MIN_MAX_CLASS_FILTER_MIN_CLASS_NUM_MASK)
     >> USER_STRING_DISC_REQUEST_MIN_MAX_CLASS_FILTER_MIN_CLASS_NUM_OFFSET);
  uint8_t maxClassFilter =
    ((appInfo->userString[USER_STRING_DISC_REQUEST_MIN_MAX_CLASS_FILTER_OFFSET]
      & USER_STRING_DISC_REQUEST_MIN_MAX_CLASS_FILTER_MAX_CLASS_NUM_MASK)
     >> USER_STRING_DISC_REQUEST_MIN_MAX_CLASS_FILTER_MAX_CLASS_NUM_OFFSET);
  uint8_t minLqiFilter =
      appInfo->userString[USER_STRING_DISC_REQUEST_MIN_LQI_FILTER_OFFSET];

  // - The user string of the discovery request will be checked for filters that
  //   the binding recipient shall conform to. If the binding recipient does not
  //   conform to all filters specified in the user string, the binding
  //   recipient shall NOT respond to the discovery request.
  // - Responding nodes shall have a primary class number that falls within the
  //   min/max class filter value.
  // - If the LQI of the discovery request measured at the binding recipient is
  //   less than the min LQI filter field in that discovery request the binding
  //   recipient shall NOT respond to the discovery request.
  // - TODO: The binding recipient may check additional implementation specific
  //         conditions. If these are not fulfilled, the binding recipient may
  //         decide NOT to respond to the discovery request.
  if ((vendorIdFilter != 0xFFFF && vendorIdFilter != emberAfRf4ceVendorId())
      || (getLocalPrimaryClassNumber() < minClassFilter)
      || (maxClassFilter < getLocalPrimaryClassNumber())
      || (rxLinkQuality < minLqiFilter)) {
    return false;
  }

  MEMMOVE(tempDiscoveryData.srcIeeeAddr, srcIeeeAddr, EUI64_SIZE);
  tempDiscoveryData.nodeCapabilities = nodeCapabilities;
  MEMMOVE(&(tempDiscoveryData.vendorInfo),
          vendorInfo,
          sizeof(EmberRf4ceVendorInfo));
  MEMMOVE(&(tempDiscoveryData.appInfo),
          appInfo,
          sizeof(EmberRf4ceApplicationInfo));
  tempDiscoveryData.searchDevType = searchDevType;

  // If the "push button stimulus received" flag is pending, the binding
  // recipient shall not send a discovery response after the reception of the
  // first discovery request, but it shall wait for the reception of a second
  // identical discovery request command frame from the same node and shall
  // again check for a match. It shall abort waiting for the second identical
  // discovery request command frame at the moment the "push button stimulus
  // received" flag is cleared.
  if (pushButtonStimulusReceivedPending()) {
    if (savedDiscoveryRequestValidOrPairCompletePending) {
      // Compare the current incoming discovery request with the one we
      // previously saved. Respond if the two are identical.
      if (discoveryRequestDataCompare(&tempDiscoveryData)) {
        return (emAfRf4ceGdpSetDiscoveryResponseAppInfo(true, GDP_VERSION_2_0)
                == EMBER_SUCCESS);
      } else {
        // If a second discovery request command frame is received that conforms
        // to all the filters described above, but was sent by another binding
        // initiator, the binding recipient shall NOT respond, it shall stop
        // waiting for the reception of identical discovery request command and
        // it shall clear the "push button stimulus received" flag.
        clearButtonStimulusReceivedPendingFlag();
        return false;
      }
    } else {
      // This is the first discovery request since the push button stimulus.
      // Save the discovery request data and wait for a second identical one.
      MEMMOVE(&savedDiscoveryOrPairRequest,
              &tempDiscoveryData,
              sizeof(EmAfDiscoveryOrPairRequestData));
      savedDiscoveryRequestValidOrPairCompletePending = true;
      return false;
    }
  } else {
    // If the "push button stimulus received" flag is not pending the node shall
    // respond [...].
    return (emAfRf4ceGdpSetDiscoveryResponseAppInfo(false, GDP_VERSION_2_0)
            == EMBER_SUCCESS);
  }
}

// This function handles all the non-GDP discovery requests. For now, we only
// handle ZRC 1.1 requests.
static bool handleNonGdpBasedDiscoveryRequest(const uint8_t *profileIdList,
                                                 uint8_t profileIdListLength)
{
  uint8_t i;

  debugScriptCheck("handleNonGdpBasedDiscoveryRequest");

  for(i=0; i<profileIdListLength; i++) {
    // We respond to a ZRC 1.1 discovery request only if the push button
    // stimulus is pending.
    if (profileIdList[i] == EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_1_1) {
      if (pushButtonStimulusReceivedPending()) {
        return (emAfRf4ceGdpSetDiscoveryResponseAppInfo(true, GDP_VERSION_NONE)
                == EMBER_SUCCESS);
      } else {
        return false;
      }
    }
  }

  return false;
}

static void startValidationProcedure(void)
{
  debugScriptCheck("Starting validation procedure");

  setInternalState(INTERNAL_STATE_RECIPIENT_GDP_VALIDATION);

  // Initialize the validation status to 'pending'.
  validationStatus = EMBER_AF_RF4CE_GDP_CHECK_VALIDATION_STATUS_PENDING;

  // Don't call the startValidation() callback if the push button stimulus
  // received flag is pending or the originator has the "binding proxy supported"
  // bit set.
  if (!pushButtonStimulusReceivedPending()
      && (savedDiscoveryOrPairRequest.appInfo.userString[USER_STRING_PAIR_REQUEST_ADVANCED_BINDING_SUPPORT_OFFSET]
          & ADVANCED_BINDING_SUPPORT_FIELD_BINDING_PROXY_SUPPORTED_BIT) == 0) {
    emberAfPluginRf4ceGdpStartValidationCallback(emAfTemporaryPairingIndex);
  }

  // In normal validation mode, the validation shall be completed within
  // aplcMaxNormalValidationDuration and the binding recipient shall always
  // set its aplBindingRecipientValidationWaitTime to a value <=
  // aplcMaxNormalValidationDuration.
  // In extended validation mode, the validation is allowed to last for more
  // than aplcMaxNormalValidationDuration and the binding recipient may set
  // aplBindingRecipientValidationWaitTime > aplcMaxNormalValidationDuration.
  // TODO: for now we use the maximum values.
  emberEventControlSetDelayMS(emberAfPluginRf4ceGdpValidationEventControl,
                              ((emAfRf4ceGdpRemoteNodeAttributes.gdpCapabilities
                                & GDP_CAPABILITIES_SUPPORT_EXTENDED_VALIDATION_BIT)
                               ? APLC_MAX_EXTENDED_VALIDATION_DURATION_MS
                               : APLC_MAX_NORMAL_VALIDATION_DURATION_MS));

  // The binding recipient shall start the aplLinkLostWaitTime timeout when
  // the validation phase starts. If a check validation request command frame
  // is received from the binding originator, the timeout shall restarted.
  // We reuse the commandPending event for handling this.
  emberEventControlSetDelayMS(emberAfPluginRf4ceGdpPendingCommandEventControl,
                              emAfRf4ceGdpLocalNodeAttributes.linkLostWaitTime);
}

static void finishBindingProcedure(bool success)
{
  emberEventControlSetInactive(emberAfPluginRf4ceGdpPendingCommandEventControl);
  emberEventControlSetInactive(emberAfPluginRf4ceGdpBlackoutTimeEventControl);
  emberEventControlSetInactive(emberAfPluginRf4ceGdpValidationEventControl);
  clearButtonStimulusReceivedPendingFlag();

  if (success) {
    assert(emAfTemporaryPairingIndex < 0xFF);

    // This pairing entry is already in "binding complete" state. Nothing to
    // do, we might have gotten here because of a duplicate
    // checkIncomingRequest() to which we responded success already.
    if (emAfRf4ceGdpGetPairingBindStatus(emAfTemporaryPairingIndex)
        & PAIRING_ENTRY_BINDING_COMPLETE_BIT) {
      return;
    }

    setPublicState(EMBER_AF_RF4CE_GDP_BINDING_STATE_BOUND, false);

    // Update the local copy of the pairing table
    emberAfRf4ceGetPairingTableEntry(emAfTemporaryPairingIndex,
                                     &(pairingTable[emAfTemporaryPairingIndex]));

    // Write the bind status for the corresponding pairing entry and set the
    // "bind complete" bit. Also remember whether the remote node supports
    // security.
    emAfRf4ceGdpSetPairingBindStatus(emAfTemporaryPairingIndex,
                                     ((PAIRING_ENTRY_BINDING_STATUS_BOUND_RECIPIENT
                                      << PAIRING_ENTRY_BINDING_STATUS_OFFSET)
                                      | PAIRING_ENTRY_BINDING_COMPLETE_BIT
                                      | ((emAfRf4ceGdpRemoteNodeAttributes.gdpCapabilities
                                          & GDP_CAPABILITIES_SUPPORT_ENHANCED_SECURITY_BIT)
                                         ? PAIRING_ENTRY_REMOTE_NODE_SUPPORTS_ENHANCED_SECURITY_BIT
                                         : 0)));

    // Let the security code save the pairing link key.
    emAfRf4ceGdpSecurityValidationCompleteCallback(emAfTemporaryPairingIndex);

    // Notify the polling code that a binding has been established/updated.
    emAfRf4ceGdpPollingNotifyBindingComplete(emAfTemporaryPairingIndex);

    // Notify the identification code that a binding has been established or
    // updated.
    emAfRf4ceGdpIdentificationNotifyBindingComplete(emAfTemporaryPairingIndex);

    // Notify all the profiles that a successful binding has been established.
    emAfRf4ceGdpNotifyBindingCompleteToProfiles(EMBER_AF_RF4CE_GDP_BINDING_STATUS_SUCCESS,
                                                emAfTemporaryPairingIndex,
                                                savedDiscoveryOrPairRequest.appInfo.profileIdList,
                                                emberAfRf4ceProfileIdListLength(savedDiscoveryOrPairRequest.appInfo.capabilities));

    emberAfPluginRf4ceGdpBindingCompleteCallback(EMBER_AF_RF4CE_GDP_BINDING_STATUS_SUCCESS,
                                                 emAfTemporaryPairingIndex);
  } else if (emAfTemporaryPairingIndex < 0xFF) {
    // From section 7.2. of GDP 2.0 specs:
    // - "A binding recipient shall make sure that, if an unsuccessful binding
    //    procedure changed an existing pairing entry in the pairing table, this
    //    pairing entry is restored after the binding procedure has failed".
    // - "A binding recipient shall make sure that, if an unsuccessful binding
    //    procedure added a new pairing entry in the table, this pairing entry
    //    is removed after the binding procedure has failed".
    if ((pairingTable[emAfTemporaryPairingIndex].info
         & EMBER_RF4CE_PAIRING_TABLE_ENTRY_INFO_STATUS_MASK)
        == EMBER_RF4CE_PAIRING_TABLE_ENTRY_STATUS_ACTIVE) {
      emberAfRf4ceSetPairingTableEntry(emAfTemporaryPairingIndex,
                                       &(pairingTable[emAfTemporaryPairingIndex]));
      debugScriptCheck("Restored saved pairing entry");
    } else {
      emberAfRf4ceSetPairingTableEntry(emAfTemporaryPairingIndex, NULL);
    }

    emAfRf4ceGdpUpdatePublicStatus(false);
  }
}

static void syncLocalPairingTable(void)
{
  uint8_t i;

  for (i=0; i<EMBER_RF4CE_PAIRING_TABLE_SIZE; i++) {
    emberAfRf4ceGetPairingTableEntry(i, &(pairingTable[i]));
  }
}

static uint8_t getLocalPrimaryClassNumber(void)
{
  return (pushButtonStimulusReceivedPending()
          ? CLASS_NUMBER_BUTTON_PRESS_INDICATION
          : EMBER_AF_PLUGIN_RF4CE_GDP_PRIMARY_CLASS_NUMBER);
}

#else
//------------------------------------------------------------------------------
// Stubs for nodes that only support the originator side.

void emberAfRf4ceGdpPushButton(bool setPending)
{
}

void emberAfRf4ceGdpSetValidationStatus(EmberAfRf4ceGdpCheckValidationStatus status)
{
}

bool emberAfPluginRf4ceProfileGdpDiscoveryRequestCallback(const EmberEUI64 srcIeeeAddr,
                                                             uint8_t nodeCapabilities,
                                                             const EmberRf4ceVendorInfo *vendorInfo,
                                                             const EmberRf4ceApplicationInfo *appInfo,
                                                             uint8_t searchDevType,
                                                             uint8_t rxLinkQuality)
{
  return false;
}

bool emberAfPluginRf4ceProfileGdpPairRequestCallback(EmberStatus status,
                                                        uint8_t pairingIndex,
                                                        const EmberEUI64 sourceIeeeAddr,
                                                        uint8_t nodeCapabilities,
                                                        const EmberRf4ceVendorInfo *vendorInfo,
                                                        const EmberRf4ceApplicationInfo *appInfo,
                                                        uint8_t keyExchangeTransferCount)
{
  return false;
}

#endif // EMBER_AF_PLUGIN_RF4CE_GDP_IS_RECIPIENT
