// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "rf4ce-mso.h"
#include "rf4ce-mso-internal.h"
#ifdef EMBER_SCRIPTED_TEST
#include "rf4ce-mso-test.h"
#endif // EMBER_SCRIPTED_TEST
#include "rf4ce-mso-attributes.h"

#ifdef EMBER_AF_PLUGIN_RF4CE_MSO_IS_ORIGINATOR

//------------------------------------------------------------------------------
// Global variables.

EmberEventControl emberAfPluginRf4ceMsoBlackoutEventControl;
EmberEventControl emberAfPluginRf4ceMsoValidationEventControl;
EmberEventControl emberAfPluginRf4ceMsoWatchdogEventControl;
EmberEventControl emberAfPluginRf4ceMsoRestoreEventControl;

EmAfMsoPairingCanditate emAfRf4ceMsoPairingCandidatesList[NWK_MAX_REPORTED_NODE_DESCRIPTORS];

//------------------------------------------------------------------------------
// Local variables.

static EmberAfRf4ceMsoBindingState bindState =
    EMBER_AF_RF4CE_MSO_BINDING_STATE_NOT_BOUND;

// This variable stores the pairing index of the temporary pairing link.
static uint8_t tempPairingIndex;

// This variable keeps track of how many candidates we already tried to pair
// and maybe validate with. This really implement the "truncating" step of the
// candidates list generation procedure.
static uint8_t currentCandidateCount;

// The current state of the binding process.
static uint8_t internalState;

// If binding fails, we need to restore the previous pairing, if one existed.
// This keeps track of the pairing entry for the active binding.
static EmberRf4cePairingTableEntry bindPairingTableEntry;

//------------------------------------------------------------------------------
// Forward declarations.

static bool isValidDiscoveryResponse(const EmberRf4ceApplicationInfo *appInfo);
static bool generatePairingCandidatesList(void);
static uint8_t getPairingCandidatesListSize(void);
static bool pairingCandidatesListEntryInUse(uint8_t index);
static uint8_t getHighestRankedCandidateIndex(void);
static void pairWithCandidate(uint8_t index);
static void failBindingProcedure(EmberAfRf4ceMsoBindingStatus status);
static void startValidationConfigurationRibRetrieval(void);
static void startValidation(void);
static EmberStatus sendCheckValidationRequest(void);
static bool restore(void);
static void unbind(void);

//------------------------------------------------------------------------------
// Public APIs.

EmberStatus emberAfRf4ceMsoBind(void)
{
  EmberStatus status;
  uint8_t profileIdList[] = { EMBER_AF_RF4CE_PROFILE_MSO };
  uint8_t bindPairingIndex = emAfRf4ceMsoGetActiveBindPairingIndex();

  if (bindState == EMBER_AF_RF4CE_MSO_BINDING_STATE_BINDING) {
    return EMBER_INVALID_CALL;
  }

  // Before commencing the binding procedure, if we have an active binding,
  // save a backup of its pairing table entry and then blow it away.  If
  // binding fails, we will restore from the backup.
  bindPairingTableEntry.info = EMBER_RF4CE_PAIRING_TABLE_ENTRY_STATUS_UNUSED;
  if (bindPairingIndex != 0xFF) {
    emberAfRf4ceGetPairingTableEntry(bindPairingIndex, &bindPairingTableEntry);
    emberAfRf4ceSetPairingTableEntry(bindPairingIndex, NULL);
  }

  status = emberAfRf4ceSetDiscoveryLqiThreshold(NWK_DISCOVERY_LQI_THRESHOLD);

  if (status == EMBER_SUCCESS) {
    status = emberAfRf4ceDiscovery(EMBER_RF4CE_BROADCAST_PAN_ID,
                                   EMBER_RF4CE_BROADCAST_ADDRESS,
                                   EMBER_AF_RF4CE_DEVICE_TYPE_WILDCARD,
                                   NWK_DISCOVERY_REPETITION_INTERVAL_MS / 3,
                                   NWK_MAX_DISCOVERY_REPETITIONS,
                                   COUNTOF(profileIdList),
                                   profileIdList);
  }

  if (status == EMBER_SUCCESS) {
    // Initialize the pairing candidates list (this also sets all the entries to
    // be "not in use".
    MEMSET(emAfRf4ceMsoPairingCandidatesList,
           0x00,
           sizeof(EmAfMsoPairingCanditate)*NWK_MAX_REPORTED_NODE_DESCRIPTORS);
    bindState = EMBER_AF_RF4CE_MSO_BINDING_STATE_BINDING;
  } else if (!restore()) {
    unbind();
  }

  return status;
}

EmberStatus emberAfRf4ceMsoWatchdogKick(uint16_t validationWatchdogTimeMs)
{
  return EMBER_INVALID_CALL;
}

EmberStatus emberAfRf4ceMsoValidate(void)
{
  return EMBER_INVALID_CALL;
}

EmberStatus emberAfRf4ceMsoTerminateValidation(void)
{
  return EMBER_INVALID_CALL;
}

EmberStatus emberAfRf4ceMsoAbortValidation(bool fullAbort)
{
  return EMBER_INVALID_CALL;
}

//------------------------------------------------------------------------------
// Internal APIs.

bool emAfRf4ceMsoIsBlackedOut(uint8_t pairingIndex)
{
  return (tempPairingIndex == pairingIndex
          && emberEventControlGetActive(emberAfPluginRf4ceMsoBlackoutEventControl));
}

//------------------------------------------------------------------------------
// Callbacks.

void emberAfPluginRf4ceMsoInitCallback(void)
{
  emAfRf4ceMsoInitCommands();
  emAfRf4ceMsoInitAttributes();

  // The state is unbound until the network comes up and we can verify the
  // pairing is still active.
  bindState = EMBER_AF_RF4CE_MSO_BINDING_STATE_NOT_BOUND;
}

void emberAfPluginRf4ceMsoStackStatusCallback(EmberStatus status)
{
  // When the RF4CE network comes up, typically right after startup, we go to
  // the bound state if we have a binding and its pairing entry is active.
  // Otherwise, we are unbound.  Note that when the RF4CE network goes down, it
  // means operations have stopped and the pairing table is no longer valid.
  // When that happens, any binding we may have had is lost, so we have to go
  // back to the unbound state.
  if (emberAfRf4ceIsCurrentNetwork()) {
    uint8_t bindPairingIndex = emAfRf4ceMsoGetActiveBindPairingIndex();
    if (status == EMBER_NETWORK_UP
        && bindPairingIndex != 0xFF
        && (emberAfRf4ceGetPairingTableEntry(bindPairingIndex,
                                             &bindPairingTableEntry)
            == EMBER_SUCCESS)
        && emberAfRf4cePairingTableEntryIsActive(&bindPairingTableEntry)) {
      bindState = EMBER_AF_RF4CE_MSO_BINDING_STATE_BOUND;
    } else {
      unbind();
    }
  }
}

bool emberAfPluginRf4ceProfileMsoDiscoveryRequestCallback(const EmberEUI64 ieeeAddr,
                                                             uint8_t nodeCapabilities,
                                                             const EmberRf4ceVendorInfo *vendorInfo,
                                                             const EmberRf4ceApplicationInfo *appInfo,
                                                             uint8_t searchDevType,
                                                             uint8_t rxLinkQuality)
{
  return false;
}

bool emberAfPluginRf4ceProfileMsoDiscoveryResponseCallback(bool atCapacity,
                                                              uint8_t channel,
                                                              EmberPanId panId,
                                                              const EmberEUI64 ieeeAddr,
                                                              uint8_t nodeCapabilities,
                                                              const EmberRf4ceVendorInfo *vendorInfo,
                                                              const EmberRf4ceApplicationInfo *appInfo,
                                                              uint8_t rxLinkQuality,
                                                              uint8_t discRequestLqi)
{
  if (bindState != EMBER_AF_RF4CE_MSO_BINDING_STATE_BINDING) {
    return false;
  }

  if (getPairingCandidatesListSize() < NWK_MAX_REPORTED_NODE_DESCRIPTORS
      && !atCapacity
      && (nodeCapabilities & EMBER_RF4CE_NODE_CAPABILITIES_IS_TARGET_BIT)
      && isValidDiscoveryResponse(appInfo)) {
    uint8_t i;
    bool duplicateResponse = false;
    uint8_t freeEntryIndex = 0xFF;

    for(i=NWK_MAX_REPORTED_NODE_DESCRIPTORS - 1;
        i<NWK_MAX_REPORTED_NODE_DESCRIPTORS;
        i--) {
      bool entryInUse = pairingCandidatesListEntryInUse(i);
      if (entryInUse
          && MEMCOMPARE(emAfRf4ceMsoPairingCandidatesList[i].ieeeAddr,
                        ieeeAddr,
                        EUI64_SIZE) == 0) {
        duplicateResponse = true;
      } else if (!entryInUse) {
        freeEntryIndex = i;
      }
    }

    if (!duplicateResponse && freeEntryIndex != 0xFF) {
      MEMMOVE(emAfRf4ceMsoPairingCandidatesList[freeEntryIndex].ieeeAddr,
              ieeeAddr,
              EUI64_SIZE);
      emAfRf4ceMsoPairingCandidatesList[freeEntryIndex].channel = channel;
      emAfRf4ceMsoPairingCandidatesList[freeEntryIndex].panId = panId;
      emAfRf4ceMsoPairingCandidatesList[freeEntryIndex].primaryClassDescriptor =
          appInfo->userString[MSO_DISCOVERY_RESPONSE_PRIMARY_CLASS_DESCRIPTOR_OFFSET];
      emAfRf4ceMsoPairingCandidatesList[freeEntryIndex].secondaryClassDescriptor =
          appInfo->userString[MSO_DISCOVERY_RESPONSE_SECONDARY_CLASS_DESCRIPTOR_OFFSET];
      emAfRf4ceMsoPairingCandidatesList[freeEntryIndex].tertiaryClassDescriptor =
          appInfo->userString[MSO_DISCOVERY_RESPONSE_TERTIARY_CLASS_DESCRIPTOR_OFFSET];
      emAfRf4ceMsoPairingCandidatesList[freeEntryIndex].basicLqiThreshold =
          appInfo->userString[MSO_DISCOVERY_RESPONSE_BASIC_LQI_THRESHOLD_OFFSET];
      emAfRf4ceMsoPairingCandidatesList[freeEntryIndex].strictLqiThreshold =
          appInfo->userString[MSO_DISCOVERY_RESPONSE_STRICT_LQI_THRESHOLD_OFFSET];
      emAfRf4ceMsoPairingCandidatesList[freeEntryIndex].rxLqi = rxLinkQuality;
      emAfRf4ceMsoPairingCandidatesList[freeEntryIndex].control =
          (MSO_NODE_DESCRIPTOR_CONTROL_CURRENT_CLASS_DESCRIPTOR_PRIMARY
           | MSO_NODE_DESCRIPTOR_CONTROL_ENTRY_IN_USE_BIT);
    }
  }

  return (getPairingCandidatesListSize() < NWK_MAX_REPORTED_NODE_DESCRIPTORS);
}

void emberAfPluginRf4ceProfileMsoDiscoveryCompleteCallback(EmberStatus status)
{
  if (bindState != EMBER_AF_RF4CE_MSO_BINDING_STATE_BINDING) {
    return;
  }

  if (getPairingCandidatesListSize() == 0) {
    failBindingProcedure(EMBER_AF_RF4CE_MSO_BINDING_STATUS_NO_VALID_RESPONSE);
  } else {
    if (generatePairingCandidatesList()) {
      // If no suitable candidate has been found in the candidates list fail
      // with NO_VALID_CANDIDATE status.
      if (getPairingCandidatesListSize() == 0) {
        failBindingProcedure(EMBER_AF_RF4CE_MSO_BINDING_STATUS_NO_VALID_CANDIDATE);
      } else {
        uint8_t index = getHighestRankedCandidateIndex();
        // Since the list size is > 0, we should never get 0xFF here.
        assert(index != 0xFF);
        currentCandidateCount = 1;
        // Start the pairing process with the highest rank candidate in the list.
        pairWithCandidate(index);
      }
    } else {
      failBindingProcedure(EMBER_AF_RF4CE_MSO_BINDING_STATUS_DUPLICATE_CLASS_ABORT);
    }
  }
}

void emberAfPluginRf4ceProfileMsoAutoDiscoveryResponseCompleteCallback(EmberStatus status,
                                                                       const EmberEUI64 srcIeeeAddr,
                                                                       uint8_t nodeCapabilities,
                                                                       const EmberRf4ceVendorInfo *vendorInfo,
                                                                       const EmberRf4ceApplicationInfo *appInfo,
                                                                       uint8_t searchDevType)
{
}

bool emberAfPluginRf4ceProfileMsoPairRequestCallback(EmberStatus status,
                                                        uint8_t pairingIndex,
                                                        const EmberEUI64 sourceIeeeAddr,
                                                        uint8_t nodeCapabilities,
                                                        const EmberRf4ceVendorInfo *vendorInfo,
                                                        const EmberRf4ceApplicationInfo *appInfo,
                                                        uint8_t keyExchangeTransferCount)
{
  return false;
}

// TODO: some code in the following two callbacks can be shared.

void emberAfPluginRf4ceProfileMsoPairCompleteCallback(EmberStatus status,
                                                      uint8_t pairingIndex,
                                                      const EmberRf4ceVendorInfo *vendorInfo,
                                                      const EmberRf4ceApplicationInfo *appInfo)
{
  if (bindState != EMBER_AF_RF4CE_MSO_BINDING_STATE_BINDING) {
    return;
  }

  if (status == EMBER_SUCCESS || status == EMBER_DUPLICATE_ENTRY) {
    // On receipt of a successful confirmation of pairing the controller
    // shall start the validation procedure.
    // The validation procedure starts with blackout time interval during which
    // both controller and target shall not transmit any packets, allowing the
    // implementation to complete the temporary pairing.
    emberEventControlSetDelayMS(emberAfPluginRf4ceMsoBlackoutEventControl,
                                APLC_BLACK_OUT_TIME_MS);
    tempPairingIndex = pairingIndex;
  } else {
    uint8_t index = getHighestRankedCandidateIndex();
    // Set the entry corresponding to the candidate we tried to pair with
    // unsuccessfully to be unused and get the next highest ranked candidate
    // (if any).
    emAfRf4ceMsoPairingCandidatesList[index].control &=
        ~(MSO_NODE_DESCRIPTOR_CONTROL_ENTRY_IN_USE_BIT);
    index = getHighestRankedCandidateIndex();
    currentCandidateCount++;

    // On receipt of an unsuccessful confirmation of pairing the controller
    // shall attempt to setup a new temporary pairing with the next highest
    // ranked target in the pairing candidates list.
    // If there are no more targets left in the pairing candidates list the
    // controller shall terminate the procedure.
    if (currentCandidateCount <= EMBER_AF_PLUGIN_RF4CE_MSO_MAX_PAIRING_CANDIDATES
        && index < 0xFF) {
      pairWithCandidate(index);
    } else {
      failBindingProcedure(EMBER_AF_RF4CE_MSO_BINDING_STATUS_PAIRING_FAILED);
    }
  }
}

void emAfRf4ceMsoValidationConfigurationResponseCallback(EmberAfRf4ceStatus status)
{
  // If this property exchange is omitted or unsuccessful, the controller shall
  // continue the validation procedure with its default parameters.
  // So regardless of the status, we kick off the validation procedure.
  startValidation();
}

void emAfPluginRf4ceMsoValidationCompleteCallback(EmberAfRf4ceMsoCheckValidationStatus status)
{
  emberEventControlSetInactive(emberAfPluginRf4ceMsoCheckValidationEventControl);
  emberEventControlSetInactive(emberAfPluginRf4ceMsoValidationEventControl);
  emberEventControlSetInactive(emberAfPluginRf4ceMsoBlackoutEventControl);
  emberEventControlSetInactive(emberAfPluginRf4ceMsoWatchdogEventControl);

  if (status == EMBER_AF_RF4CE_MSO_CHECK_VALIDATION_STATUS_SUCCESS) {
    // If the validation is successful, the pairing shall considered permanent
    // and the controller is bound with the target.
    emAfRf4ceMsoSetActiveBindPairingIndex(tempPairingIndex);
    bindState = EMBER_AF_RF4CE_MSO_BINDING_STATE_BOUND;

    // Binding procedure completed, turn the radio back off.
    emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_MSO, false);

    emberAfPluginRf4ceMsoBindingCompleteCallback(status,
                                                 tempPairingIndex);
  } else {
    // If the validation is unsuccessful the controller shall remove the pairing
    // entry of the target from its pairing table.
    emberAfRf4ceSetPairingTableEntry(tempPairingIndex, NULL);

    // If the validation is unsuccessful with "full abort" status, the profile
    // shall terminate the procedure.
    // Otherwise, the controller shall attempt to setup a new pairing link with
    // the next highest ranked target in the pairing candidates list.
    // If there are no more targets left in the pairing candidates list, the
    // controller shall terminate the procedure.
    if (status == EMBER_AF_RF4CE_MSO_CHECK_VALIDATION_STATUS_FULL_ABORT) {
      failBindingProcedure(EMBER_AF_RF4CE_MSO_BINDING_STATUS_VALIDATION_FULL_ABORT);
    } else {
      uint8_t index = getHighestRankedCandidateIndex();
      // Set the entry corresponding to the candidate we tried to validate
      // unsuccessfully to be unused and get the next highest ranked candidate
      // (if any).
      emAfRf4ceMsoPairingCandidatesList[index].control &=
          ~(MSO_NODE_DESCRIPTOR_CONTROL_ENTRY_IN_USE_BIT);
      index = getHighestRankedCandidateIndex();
      currentCandidateCount++;

      if (currentCandidateCount <= EMBER_AF_PLUGIN_RF4CE_MSO_MAX_PAIRING_CANDIDATES
          && index < 0xFF) {
        pairWithCandidate(index);
      } else {
        failBindingProcedure(status);
      }
    }
  }
}

void emAfPluginRf4ceMsoCheckValidationRequestSentCallback(EmberStatus status,
                                                          uint8_t pairingIndex)
{
  if (status == EMBER_SUCCESS
      && internalState == BINDING_STATE_CHECK_VALIDATION_SENDING_REQUEST) {
    internalState = BINDING_STATE_CHECK_VALIDATION_IDLING;

    // If the transmission of the check validation request frame was successful
    // the controller shall wait aplcResponseIdleTime with its receiver
    // disabled.
    emberEventControlSetDelayMS(emberAfPluginRf4ceMsoWatchdogEventControl,
                                APLC_RESPONSE_IDLE_TIME_MS);
    // Turn off the radio
    emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_MSO, false);
  } else {
    // If transmission was unsuccessful, we will send eventually a new request.
    internalState = BINDING_STATE_CHECK_VALIDATION_INITIAL;
  }
}

void emAfPluginRf4ceMsoIncomingCheckValidationResponseCallback(EmberAfRf4ceMsoCheckValidationStatus status,
                                                               uint8_t pairingIndex)
{
  // If the controller receives a Check Validation Response, it shall disable
  // the receiver.
  if(internalState == BINDING_STATE_CHECK_VALIDATION_WAIT_FOR_RESPONSE) {
    // Turn off the radio
    emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_MSO, false);

    emberEventControlSetInactive(emberAfPluginRf4ceMsoWatchdogEventControl);

    // If the validation status field is PENDING, the validation shall continue.
    if (status == EMBER_AF_RF4CE_MSO_CHECK_VALIDATION_STATUS_PENDING) {
      internalState = BINDING_STATE_CHECK_VALIDATION_INITIAL;
    } else {
      emAfPluginRf4ceMsoValidationCompleteCallback(status);
    }
  }
}

//------------------------------------------------------------------------------
// Event handlers.

void emberAfPluginRf4ceMsoCheckValidationEventHandler(void)
{
  emberEventControlSetInactive(emberAfPluginRf4ceMsoCheckValidationEventControl);

  if (internalState == BINDING_STATE_CHECK_VALIDATION_INITIAL) {
    // To verify if the validation is successful, the controller shall generate
    // and transmit a check validation request command frame to the target
    // device every aplAutoCheckValidationPeriod.
    // As first step we send out the validation request and wait for the
    // messageSent() callback to confirm that the message was sent out.
    // If transmission fails, we just wait for the next period to send a new
    // request.
    if (sendCheckValidationRequest() == EMBER_SUCCESS) {
      internalState = BINDING_STATE_CHECK_VALIDATION_SENDING_REQUEST;
    }

    // We also start the aplLinkLostWaitTime timer if the event is not pending
    // already (we start it regardless on whether the request was actually
    // received at the target).
    if (!emberEventControlGetActive(emberAfPluginRf4ceMsoValidationEventControl)) {
      emberEventControlSetDelayMS(emberAfPluginRf4ceMsoValidationEventControl,
                                  HIGH_LOW_TO_INT(emAfRf4ceMsoLocalRibAttributes.validationConfiguration[1],
                                                  emAfRf4ceMsoLocalRibAttributes.validationConfiguration[0]));
    }

    emberEventControlSetDelayMS(emberAfPluginRf4ceMsoCheckValidationEventControl,
                                HIGH_LOW_TO_INT(emAfRf4ceMsoLocalRibAttributes.validationConfiguration[3],
                                                emAfRf4ceMsoLocalRibAttributes.validationConfiguration[2]));
  } else {
    // We did not receive a validation response within the timeout. We send a
    // new request.
    internalState = BINDING_STATE_CHECK_VALIDATION_INITIAL;
    emberEventControlSetActive(emberAfPluginRf4ceMsoCheckValidationEventControl);
  }
}

void emberAfPluginRf4ceMsoBlackoutEventHandler(void)
{
  emberEventControlSetInactive(emberAfPluginRf4ceMsoBlackoutEventControl);

  // After the blackout interval, the validation procedure MAY start with an
  // optional "Validation Configuration RIB retrieval" step which exchanges
  // updated values of the properties used for the validation procedure on the
  // controller side. These updated properties, if used, shall be stored on the
  // target and can be retrieved by the controller via a RIB request.
  // If this property exchange is omitted or unsuccessful, the controller shall
  // continue the validation procedure with its default parameters.
  // If Validation Configuration RIB Retrieval is non attempted, the first Check
  // Validation Request shall be sent aplAutoCheckValidationPeriod after the
  // blackout interval. Otherwise aplAutoCheckValidationPeriod after the
  // Validation Configuration RIB Retrieval.
  startValidationConfigurationRibRetrieval();
}

void emberAfPluginRf4ceMsoValidationEventHandler(void)
{
  emberEventControlSetInactive(emberAfPluginRf4ceMsoValidationEventControl);

  emAfPluginRf4ceMsoValidationCompleteCallback(EMBER_AF_RF4CE_MSO_CHECK_VALIDATION_STATUS_TIMEOUT);
}

// At the controller we use the watchdog event to time the radio on/off
// intervals.
void emberAfPluginRf4ceMsoWatchdogEventHandler(void)
{
  emberEventControlSetInactive(emberAfPluginRf4ceMsoWatchdogEventControl);

  if (internalState == BINDING_STATE_CHECK_VALIDATION_IDLING) {
    // Turn the radio back on
    emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_MSO, true);
    internalState = BINDING_STATE_CHECK_VALIDATION_WAIT_FOR_RESPONSE;

    // After waiting for aplResponseIdleTime the controller shall enable its
    // receiver and wait aplResponseWaitTime for the corresponding check
    // validation response command frame to arrive.
    emberEventControlSetDelayMS(emberAfPluginRf4ceMsoWatchdogEventControl,
                                EMBER_AF_PLUGIN_RF4CE_MSO_RESPONSE_WAIT_TIME_MS);
  } else if (internalState == BINDING_STATE_CHECK_VALIDATION_WAIT_FOR_RESPONSE) {
    // If the controller doesn't receive the check validation response command
    // frame within the aplResponseWaitTime interval, the receiver shall be
    // disabled.

    // Turn off the radio
    emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_MSO, false);
  }
}

void emberAfPluginRf4ceMsoRestoreEventHandler(void)
{
  emberEventControlSetInactive(emberAfPluginRf4ceMsoRestoreEventControl);
}

//------------------------------------------------------------------------------
// Static functions.

static bool isValidDiscoveryResponse(const EmberRf4ceApplicationInfo *appInfo)
{
  // - From section 6.5 MSO specs: The list of profile identifiers by which
  //   incoming discovery response command frames are matched shall contain
  //   the RF4CE MSO Profile Identifier.
  // - Check that the response has an user string.
  // - Check the tertiary class descriptor field not to be "reclassify".
  uint8_t profileIdListLength = emberAfRf4ceProfileIdListLength(appInfo->capabilities);
  uint8_t i;
  bool supportsMso = false;
  for (i = 0; i < profileIdListLength; i++) {
    if (appInfo->profileIdList[i] == EMBER_AF_RF4CE_PROFILE_MSO) {
      supportsMso = true;
      break;
    }
  }
  return (supportsMso
          && (appInfo->capabilities & EMBER_RF4CE_APP_CAPABILITIES_USER_STRING_BIT) > 0
          && ((appInfo->userString[MSO_DISCOVERY_RESPONSE_TERTIARY_CLASS_DESCRIPTOR_OFFSET]
               & MSO_CLASS_DESCRIPTOR_DUPLICATE_CLASS_NUMBER_HANDLING_MASK)
              >> MSO_CLASS_DESCRIPTOR_DUPLICATE_CLASS_NUMBER_HANDLING_OFFSET)
             != MSO_DUPLICATE_CLASS_NUMBER_HANDLING_RECLASSIFY_NODE_DESCRIPTOR);
}

static bool pairingCandidatesListEntryInUse(uint8_t index)
{
  assert(index < NWK_MAX_REPORTED_NODE_DESCRIPTORS);

  return (emAfRf4ceMsoPairingCandidatesList[index].control
          & MSO_NODE_DESCRIPTOR_CONTROL_ENTRY_IN_USE_BIT);
}

static uint8_t getPairingCandidatesListSize(void)
{
  uint8_t i, count;
  for(i=0, count=0; i<NWK_MAX_REPORTED_NODE_DESCRIPTORS; i++) {
    if(pairingCandidatesListEntryInUse(i)) {
      count++;
    }
  }
  return count;
}

static uint8_t getCurrentClassDescriptor(uint8_t index)
{
  uint8_t classDescriptorInUse =
      (emAfRf4ceMsoPairingCandidatesList[index].control
       & MSO_NODE_DESCRIPTOR_CONTROL_CURRENT_CLASS_DESCRIPTOR_MASK);

  if (classDescriptorInUse
      == MSO_NODE_DESCRIPTOR_CONTROL_CURRENT_CLASS_DESCRIPTOR_PRIMARY) {
    return emAfRf4ceMsoPairingCandidatesList[index].primaryClassDescriptor;
  } else if (classDescriptorInUse
      == MSO_NODE_DESCRIPTOR_CONTROL_CURRENT_CLASS_DESCRIPTOR_SECONDARY) {
    return emAfRf4ceMsoPairingCandidatesList[index].secondaryClassDescriptor;
  } else {
    return emAfRf4ceMsoPairingCandidatesList[index].tertiaryClassDescriptor;
  }
}

static uint8_t getHighestRankedCandidateIndex(void)
{
  uint8_t highestRankedEntryIndex = 0xFF;
  uint8_t i;

  for(i=0; i<NWK_MAX_REPORTED_NODE_DESCRIPTORS; i++) {
    if (pairingCandidatesListEntryInUse(i)) {
      if (highestRankedEntryIndex == 0xFF) {
        highestRankedEntryIndex = i;
      } else {
        uint8_t highestRankedClassNumber =
            (getCurrentClassDescriptor(highestRankedEntryIndex)
             & MSO_CLASS_DESCRIPTOR_CLASS_NUMBER_MASK);
        uint8_t iClassNumber = (getCurrentClassDescriptor(i)
                              & MSO_CLASS_DESCRIPTOR_CLASS_NUMBER_MASK);

        // There shall be an inverse relationship between a node descriptor's
        // class number and its position in the node descriptor list. If two
        // controllers have the same class number, the LQI value shall be used
        // as the tie breaker.

        if (iClassNumber < highestRankedClassNumber
            || (iClassNumber == highestRankedClassNumber
                && emAfRf4ceMsoPairingCandidatesList[highestRankedEntryIndex].rxLqi
                   < emAfRf4ceMsoPairingCandidatesList[i].rxLqi)) {
          highestRankedEntryIndex = i;
        }
      }
    }
  }

  return highestRankedEntryIndex;
}

// Return true if the binding procedure can continue, false if it should be
// aborted.
static bool handleDuplicateClassNumber(uint8_t index, bool *reclassified)
{
  uint8_t handlingField = (getCurrentClassDescriptor(index)
                         & MSO_CLASS_DESCRIPTOR_DUPLICATE_CLASS_NUMBER_HANDLING_MASK)
                         >> MSO_CLASS_DESCRIPTOR_DUPLICATE_CLASS_NUMBER_HANDLING_OFFSET;

  switch(handlingField) {
  case MSO_DUPLICATE_CLASS_NUMBER_HANDLING_USE_NODE_AS_IS:
    break;
  case MSO_DUPLICATE_CLASS_NUMBER_HANDLING_REMOVE_NODE_DESCRIPTOR:
    emAfRf4ceMsoPairingCandidatesList[index].control &=
        ~(MSO_NODE_DESCRIPTOR_CONTROL_ENTRY_IN_USE_BIT);
    break;
  case MSO_DUPLICATE_CLASS_NUMBER_HANDLING_RECLASSIFY_NODE_DESCRIPTOR:
  {
    uint8_t descriptorInUse =
        (emAfRf4ceMsoPairingCandidatesList[index].control
         & MSO_NODE_DESCRIPTOR_CONTROL_CURRENT_CLASS_DESCRIPTOR_MASK);

    *reclassified = true;

    // Clear the class descriptor in use field.
    emAfRf4ceMsoPairingCandidatesList[index].control &=
        ~(MSO_NODE_DESCRIPTOR_CONTROL_CURRENT_CLASS_DESCRIPTOR_MASK);

    if (descriptorInUse ==
        MSO_NODE_DESCRIPTOR_CONTROL_CURRENT_CLASS_DESCRIPTOR_PRIMARY) {
      emAfRf4ceMsoPairingCandidatesList[index].control |=
          MSO_NODE_DESCRIPTOR_CONTROL_CURRENT_CLASS_DESCRIPTOR_SECONDARY;
    } else if (descriptorInUse ==
        MSO_NODE_DESCRIPTOR_CONTROL_CURRENT_CLASS_DESCRIPTOR_SECONDARY) {
      emAfRf4ceMsoPairingCandidatesList[index].control |=
          MSO_NODE_DESCRIPTOR_CONTROL_CURRENT_CLASS_DESCRIPTOR_TERTIARY;
    } else {
      // We already filtered out responses with "reclassify" field set in the
      // tertiary class descriptor.
      assert(0);
    }
    break;
  }
  case MSO_DUPLICATE_CLASS_NUMBER_HANDLING_ABORT_BINDING:
    return false;
  }

  return true;
}

// Return true if the binding procedure can continue, false if it should be
// aborted.
static bool duplicateClassNumberHandlingProcedure(void)
{
  bool reclassified;
  uint8_t i,j;

  do {
    reclassified = false;

    for(i=0; i<NWK_MAX_REPORTED_NODE_DESCRIPTORS; i++) {
      uint8_t iClassNumber = (getCurrentClassDescriptor(i)
                            & MSO_CLASS_DESCRIPTOR_CLASS_NUMBER_MASK);
      bool isDuplicate = false;

      if (!pairingCandidatesListEntryInUse(i)) {
        continue;
      }

      for(j=i+1; j<NWK_MAX_REPORTED_NODE_DESCRIPTORS; j++) {
        uint8_t jClassNumber = (getCurrentClassDescriptor(j)
                              & MSO_CLASS_DESCRIPTOR_CLASS_NUMBER_MASK);

        if (!pairingCandidatesListEntryInUse(j)) {
          continue;
        }

        // Duplicate class number detected.
        if (iClassNumber == jClassNumber) {
          isDuplicate = true;
          if (!handleDuplicateClassNumber(j, &reclassified)) {
            return false;
          }
        }
      }

      if (isDuplicate) {
        if (!handleDuplicateClassNumber(i, &reclassified)) {
          return false;
        }
        break;
      }
    }
  } while (reclassified);

  return true;
}

// Returns true if the ranking procedure succeeded, false there was a duplicate
// class number that was resolved with an abort.
static bool generatePairingCandidatesList(void)
{
  uint8_t i;

  // Basic LQI Threshold filtering: The controller shall remove all node
  // descriptors for which the LQI of the discovery response is below the "Basic
  // LQI Threshold" as indicated in the user string of the discovery response.
  for(i=0; i<NWK_MAX_REPORTED_NODE_DESCRIPTORS; i++) {
    if (pairingCandidatesListEntryInUse(i)
        && emAfRf4ceMsoPairingCandidatesList[i].basicLqiThreshold
           > emAfRf4ceMsoPairingCandidatesList[i].rxLqi) {
      emAfRf4ceMsoPairingCandidatesList[i].control &=
          ~(MSO_NODE_DESCRIPTOR_CONTROL_ENTRY_IN_USE_BIT);
    }
  }

  // Handle duplicate class numbers.
  if (!duplicateClassNumberHandlingProcedure()) {
    return false;
  }

  // Strict LQI Threshold filtering: If 'Apply Strict LQI Threshold' is set in
  // the final class descriptor that was used in the ranking process, the
  // controller shall remove all node descriptors for which the LQI of the
  // discovery response is below the the Strict LQI threshold after duplicate
  // class number handling.
  for(i=0; i<NWK_MAX_REPORTED_NODE_DESCRIPTORS; i++) {
    if (pairingCandidatesListEntryInUse(i)
        && (getCurrentClassDescriptor(i)
            & MSO_CLASS_DESCRIPTOR_APPLY_STRICT_LQI_THRESHOLD_BIT) > 0
        && emAfRf4ceMsoPairingCandidatesList[i].strictLqiThreshold
           > emAfRf4ceMsoPairingCandidatesList[i].rxLqi) {
      emAfRf4ceMsoPairingCandidatesList[i].control &=
          ~(MSO_NODE_DESCRIPTOR_CONTROL_ENTRY_IN_USE_BIT);
    }
  }

  return true;
}

static void pairWithCandidate(uint8_t index)
{
  emberAfRf4cePair(emAfRf4ceMsoPairingCandidatesList[index].channel,
                   emAfRf4ceMsoPairingCandidatesList[index].panId,
                   emAfRf4ceMsoPairingCandidatesList[index].ieeeAddr,
                   EMBER_AF_PLUGIN_RF4CE_MSO_KEY_EXCHANGE_TRANSFER_COUNT,
                   NULL);
}

static EmberStatus sendCheckValidationRequest(void)
{
  uint8_t index = getHighestRankedCandidateIndex();

  emAfRf4ceMsoBufferLength = 0;
  emAfRf4ceMsoBuffer[emAfRf4ceMsoBufferLength++] =
      EMBER_AF_RF4CE_MSO_COMMAND_CHECK_VALIDATION_REQUEST;

  // If 'Enable Request Automatic Validation' is set in the final class
  // descriptor that was used in the ranking process for a node descriptor, the
  // check validation request frames in the validation procedure for this node
  // descriptor SHALL have the 'Request Automatic Validation' flag set.
  emAfRf4ceMsoBuffer[emAfRf4ceMsoBufferLength++] =
      (getCurrentClassDescriptor(index)
       & MSO_CLASS_DESCRIPTOR_ENABLE_REQUEST_AUTO_VALIDATION_BIT)
       ? CHECK_VALIDATION_CONTROL_REQUEST_AUTOMATIC_VALIDATION_BIT
       : 0x00;

  // Turn the radio on and wait for a check validation response.
  emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_MSO, true);

  return emAfRf4ceMsoSend(tempPairingIndex);
}

static void startValidationConfigurationRibRetrieval(void)
{
  if (emberAfRf4ceMsoGetAttributeRequest(tempPairingIndex,
                                         EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_VALIDATION_CONFIGURATION,
                                         0,
                                         MSO_RIB_ATTRIBUTE_VALIDATION_CONFIGURATION_LENGTH)
      == EMBER_SUCCESS) {
    internalState = BINDING_STATE_GET_VALIDATION_CONFIG_PENDING;
  } else {
    // If this property exchange is omitted or unsuccessful, the controller
    // shall continue the validation procedure with its default parameters.
    startValidation();
  }
}

static void startValidation(void)
{
  debugScriptCheck("Start validation");

  internalState = BINDING_STATE_CHECK_VALIDATION_INITIAL;
  emberEventControlSetDelayMS(emberAfPluginRf4ceMsoCheckValidationEventControl,
                              HIGH_LOW_TO_INT(emAfRf4ceMsoLocalRibAttributes.validationConfiguration[3],
                                              emAfRf4ceMsoLocalRibAttributes.validationConfiguration[2]));
}

static void failBindingProcedure(EmberAfRf4ceMsoBindingStatus status)
{
  // If the validation was unsuccessful and the controller was bound with a
  // target before the binding procedure started, the old binding shall be
  // restored.  If it can't be restored, we have no choice but to go back to
  // the unbound state.
  if (!restore()) {
    unbind();
  }

  // Binding procedure is done, turn the radio back off.
  emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_MSO, false);

  emberAfPluginRf4ceMsoBindingCompleteCallback(status, 0xFF);
}

static bool restore(void)
{
  uint8_t bindPairingIndex = emAfRf4ceMsoGetActiveBindPairingIndex();
  if (bindPairingIndex != 0xFF
      && emberAfRf4cePairingTableEntryIsActive(&bindPairingTableEntry)
      && (emberAfRf4ceSetPairingTableEntry(bindPairingIndex,
                                           &bindPairingTableEntry)
          == EMBER_SUCCESS)) {
    bindState = EMBER_AF_RF4CE_MSO_BINDING_STATE_BOUND;
    return true;
  }
  return false;
}

static void unbind(void)
{
  // Set the state to not bound, clear the backup and the actual pairing entry,
  // and forget the bound pairing index.
  uint8_t bindPairingIndex = emAfRf4ceMsoGetActiveBindPairingIndex();
  bindState = EMBER_AF_RF4CE_MSO_BINDING_STATE_NOT_BOUND;
  bindPairingTableEntry.info = EMBER_RF4CE_PAIRING_TABLE_ENTRY_STATUS_UNUSED;
  emAfRf4ceMsoSetActiveBindPairingIndex(0xFF);
  if (bindPairingIndex != 0xFF) {
    emberAfRf4ceSetPairingTableEntry(bindPairingIndex, NULL);
  }
}

#if defined(EMBER_SCRIPTED_TEST)
void resetMsoInternalState(void)
{
  // TODO: This should probably call unbind(), but doing so breaks the scripted
  // tests because the setPairingTableEntry call is unexpected.
  bindState = EMBER_AF_RF4CE_MSO_BINDING_STATE_NOT_BOUND;
  emAfRf4ceMsoSetActiveBindPairingIndex(0xFF);
}
#endif // EMBER_SCRIPTED_TEST

#endif // EMBER_AF_PLUGIN_RF4CE_MSO_IS_ORIGINATOR
