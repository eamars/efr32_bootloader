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

#ifdef EMBER_AF_PLUGIN_RF4CE_GDP_IS_ORIGINATOR

//------------------------------------------------------------------------------
// Global variables.

EmberEventControl emberAfPluginRf4ceGdpCheckBindingProcedureEventControl;

//------------------------------------------------------------------------------
// Local variables.

static EmAfGdpPairingCanditate pairingCandidatesList[APL_MAX_PAIRING_CANDIDATES];

static uint8_t savedProfileIdList[7];
static uint8_t savedProfileIdListLength;
static uint8_t savedSearchDevType;
static EmberRf4cePairingTableEntry savedPairingEntry;
static uint8_t savedPairingEntryIndex;

static bool bindProcessAborted;

//------------------------------------------------------------------------------
// Macros

#define isPairingCandidateEntryInUse(index)                                    \
  ((pairingCandidatesList[(index)].info & CANDIDATE_INFO_ENTRY_IN_USE_BIT) > 0)

//------------------------------------------------------------------------------
// Forward declarations.

// Start the discovery process. If the passed profileIdList is NULL, the
// function will reuse the profile ID list stored from the most recent call
// with a non-NULL profileIdList parameter.
static EmberStatus startDiscoveryProcess(uint8_t* profileIdList,
                                         uint8_t profileIdListLength,
                                         uint8_t searchDevType);

static EmberStatus startPairingProcess(void);

static void startGdpConfigurationProcedureOrFail(void);

static void startValidationProcedure(void);

// Start the binding procedure timer.
static void startBindingProcedureTimer(void);

// Returns true is the timer is still running or it is the first time we check
// the timer since it expired.
static bool checkBindingProcedureTimer(void);

static bool maybeRestoreSavedPairingEntry(void);

static void tryRestartDiscoveryProcessOrFail(EmberAfRf4ceGdpBindingStatus failStatus);

static void failBindingProcedure(EmberAfRf4ceGdpBindingStatus status);

static void bindingProcedureComplete(EmberAfRf4ceGdpBindingStatus status,
                                     uint8_t pairingIndex);

// Returns true if pc1 ranks higher than pc2, false otherwise. Flag abort is
// set to true if a duplicate was detected and any of the duplicates had the
// duplicate handling field set to 'abort' or 'reserved'. In this case, the
// returning value is meaningless.
static bool comparePairingCandidates(EmAfGdpPairingCanditate *pc1,
                                        EmAfGdpPairingCanditate *pc2,
                                        bool *abort,
                                        bool *lqiTieBreaker);

// Returns the highest or lowest ranked candidate index. If during the ranking
// process, at least a duplicate is detected such that one of the two candidates
// have the duplicate handling field set to 'abort' or 'reserved', the abort
// flag is set and the returning index is meaningless.
static uint8_t getLowestOrHighestRankedCandidateIndex(bool highest,
                                                    bool freshOnly,
                                                    bool *abort);

#define HIGHEST           true
#define LOWEST            false
#define FRESH_ONLY        true
#define ALL_CANDIDATES    false

//------------------------------------------------------------------------------
// Public APIs.

EmberStatus emberAfRf4ceGdpBind(uint8_t *profileIdList,
                                uint8_t profileIdListLength,
                                uint8_t searchDevType)
{
  EmberStatus status;

  if (publicBindState() == EMBER_AF_RF4CE_GDP_BINDING_STATE_BINDING) {
    return EMBER_INVALID_CALL;
  }

  emAfRf4ceGdpSetProxyBindingFlag(false);

  status = startDiscoveryProcess(profileIdList,
                                 profileIdListLength,
                                 searchDevType);

  if (status == EMBER_SUCCESS) {
    // Initialize the pairing candidates list (this also sets all the entries to
    // be "not in use".
    MEMSET(pairingCandidatesList,
           0x00,
           sizeof(EmAfGdpPairingCanditate)*APL_MAX_PAIRING_CANDIDATES);

    setPublicState(EMBER_AF_RF4CE_GDP_BINDING_STATE_BINDING, false);

    bindProcessAborted = false;
    savedPairingEntryIndex = 0xFF;

    startBindingProcedureTimer();
  }

  return status;
}

EmberStatus emberAfRf4ceGdpProxyBind(EmberPanId panId,
                                     EmberEUI64 ieeeAddr,
                                     uint8_t *profileIdList,
                                     uint8_t profileIdListLength)
{
  EmberStatus status;

  emAfRf4ceGdpSetProxyBindingFlag(true);

  // Initialize the pairing candidates list (this also sets all the entries to
  // be "not in use".
  MEMSET(pairingCandidatesList,
         0x00,
         sizeof(EmAfGdpPairingCanditate)*APL_MAX_PAIRING_CANDIDATES);

  // Set the entry at index 0 as 'in use' and mark it as a 'proxy' candidate.
  pairingCandidatesList[0].info = (CANDIDATE_INFO_PROXY_CANDIDATE_BIT
                                   | CANDIDATE_INFO_ENTRY_IN_USE_BIT);
  // TODO: we always start from channel 15. If we want to support non-standard
  // channels we will have to update this.
  pairingCandidatesList[0].channel = 15;
  pairingCandidatesList[0].panId = panId;
  pairingCandidatesList[0].rxLqi = 0xFF;
  MEMMOVE(pairingCandidatesList[0].ieeeAddr, ieeeAddr, EUI64_SIZE);
  // Save the passed profile ID list so that we can correctly set the app info
  // in the pair request we are about to send.
  savedProfileIdListLength = profileIdListLength;
  MEMMOVE(savedProfileIdList, profileIdList, profileIdListLength);

  // Set the primary class number to "button press indication".
  pairingCandidatesList[0].primaryClassDescriptor =
                            ((CLASS_NUMBER_BUTTON_PRESS_INDICATION
                              << CLASS_DESCRIPTOR_NUMBER_OFFSET)
                             | (CLASS_DESCRIPTOR_DUPLICATE_HANDLING_ABORT
                                << CLASS_DESCRIPTOR_DUPLICATE_HANDLING_OFFSET));

  status = startPairingProcess();

  if (status == EMBER_SUCCESS) {
    setPublicState(EMBER_AF_RF4CE_GDP_BINDING_STATE_BINDING, false);
  }

  return status;
}

//------------------------------------------------------------------------------
// Callbacks.

bool emberAfPluginRf4ceProfileGdpDiscoveryResponseCallback(bool atCapacity,
                                                              uint8_t channel,
                                                              EmberPanId panId,
                                                              const EmberEUI64 ieeeAddr,
                                                              uint8_t nodeCapabilities,
                                                              const EmberRf4ceVendorInfo *vendorInfo,
                                                              const EmberRf4ceApplicationInfo *appInfo,
                                                              uint8_t rxLinkQuality,
                                                              uint8_t discRequestLqi)
{
  EmAfGdpPairingCanditate candidate;
  uint8_t matchingProfileIdList[EMBER_RF4CE_APPLICATION_PROFILE_ID_LIST_MAX_LENGTH];
  uint8_t matchingProfileIdListLength;
  uint8_t sameRecipientEntryIndex = 0xFF;
  uint8_t freeEntryIndex = 0xFF;
  uint8_t minLqiThreshold;
  uint8_t gdpVersion;
  uint8_t i;

  if (publicBindState() != EMBER_AF_RF4CE_GDP_BINDING_STATE_BINDING) {
    debugDiscoveryResponseDrop("not binding");
    return false;
  }

  if (bindProcessAborted) {
    debugDiscoveryResponseDrop("bind process aborted already");
    return false;
  }

  if (atCapacity) {
    debugDiscoveryResponseDrop("'at capacity' flag set");
    return true;
  }

  matchingProfileIdListLength =
      emAfCheckDeviceTypeAndProfileIdMatch(savedSearchDevType,
                                           (uint8_t*)appInfo->deviceTypeList,
                                           emberAfRf4ceDeviceTypeListLength(appInfo->capabilities),
                                           savedProfileIdList,
                                           savedProfileIdListLength,
                                           (uint8_t*)appInfo->profileIdList,
                                           emberAfRf4ceProfileIdListLength(appInfo->capabilities),
                                           matchingProfileIdList);

  // - Profile/device type filtering
  // The binding originator shall check that at least one device type contained
  // in the device type list field matches supplied by the application and that
  // at least one profile ID contained in the profile ID list matches at least
  // one profile ID from the discovery profile ID list supplied by the
  // application.
  if (matchingProfileIdListLength == 0) {
    debugDiscoveryResponseDrop("no match");
    return true;
  }

  // - Is the response from a node already in the pairing candidate list?
  // - Is there a free entry in the pairing candidate list?
  for(i=0; i<APL_MAX_PAIRING_CANDIDATES;i++) {
    if (isPairingCandidateEntryInUse(i)
        && MEMCOMPARE(pairingCandidatesList[i].ieeeAddr,
                      ieeeAddr,
                      EUI64_SIZE) == 0) {
      sameRecipientEntryIndex = i;
    } else if (!isPairingCandidateEntryInUse(i)) {
      freeEntryIndex = i;
    }
  }

  gdpVersion = emAfRf4ceGdpGetGdpVersion(matchingProfileIdList,
                                         matchingProfileIdListLength);

  // This implicitly set the candidate to "fresh".
  candidate.info = CANDIDATE_INFO_ENTRY_IN_USE_BIT;
  candidate.channel = channel;
  candidate.panId = panId;
  candidate.rxLqi = rxLinkQuality;
  MEMMOVE(candidate.ieeeAddr, ieeeAddr, EUI64_SIZE);
  candidate.supportedProfilesLength =
      emberAfRf4ceProfileIdListLength(appInfo->capabilities);
  MEMMOVE(candidate.supportedProfiles,
          appInfo->profileIdList,
          candidate.supportedProfilesLength);

  if (gdpVersion == GDP_VERSION_2_0) {
    candidate.primaryClassDescriptor =
        appInfo->userString[USER_STRING_DISC_RESPONSE_PRIMARY_CLASS_DESCRIPTOR_OFFSET];
    candidate.secondaryClassDescriptor =
        appInfo->userString[USER_STRING_DISC_RESPONSE_SECONDARY_CLASS_DESCRIPTOR_OFFSET];
    candidate.tertiaryClassDescriptor =
        appInfo->userString[USER_STRING_DISC_RESPONSE_TERTIARY_CLASS_DESCRIPTOR_OFFSET];
    minLqiThreshold =
        appInfo->userString[USER_STRING_DISC_RESPONSE_DISCOVERY_LQI_THRESHOLD_OFFSET];
  } else {
    // For discovery responses received from a ZRC 1.1 profile or a GDP 1.x
    // based profile, the user string shall not be processed but the following
    // information shall be assumed:
    // - minLqiThreshold shall be assumed to 0 (no filtering)
    // - The primary class descriptor shall be assumed to be [classNumber =
    //   "ButtonPressIndication", duplicate class number handling = "abort"]
    // - The secondary and tertiary class descriptors shall be assumed to be
    //   empty.
    candidate.primaryClassDescriptor =
        ((CLASS_NUMBER_BUTTON_PRESS_INDICATION
          << CLASS_DESCRIPTOR_NUMBER_OFFSET)
         | (CLASS_DESCRIPTOR_DUPLICATE_HANDLING_ABORT
            << CLASS_DESCRIPTOR_DUPLICATE_HANDLING_OFFSET));
    minLqiThreshold = 0;
  }

  // - LQI threshold filtering
  // The binding originator shall discard the discovery response if the LQI
  // of the discovery request is below the LQI threshold as indicated in the
  // user string of the discovery response.
  if (discRequestLqi < minLqiThreshold) {
    debugDiscoveryResponseDrop("LQI below threshold");
    return true;
  }

  // If the binding recipient that transmitted the discovery response is not yet
  // in the list of pairing candidates:
  if (sameRecipientEntryIndex == 0xFF) {
    // If the primary class descriptor's class number is set to "discoverable
    // only" this discovery response shall be dropped and the binding recipient
    // shall not be considered for pairing.
    if (((candidate.primaryClassDescriptor & CLASS_DESCRIPTOR_NUMBER_MASK)
         >> CLASS_DESCRIPTOR_NUMBER_OFFSET) == CLASS_NUMBER_DISCOVERABLE_ONLY) {
      debugDiscoveryResponseDrop("primary class discoverable only");
      return true;
    }

    if (freeEntryIndex == 0xFF) {
      // Table is already full: find the lowest ranked candidate, compare it
      // with the current candidate and replace it if the current candidate
      // ranks higher.
      uint8_t lowestCandidateIndex =
          getLowestOrHighestRankedCandidateIndex(LOWEST,
                                                 ALL_CANDIDATES,
                                                 &bindProcessAborted);

      if (!bindProcessAborted) {
        bool newCandidateRanksHigher =
            comparePairingCandidates(&candidate,
                                     &pairingCandidatesList[lowestCandidateIndex],
                                     &bindProcessAborted,
                                     NULL);
        if (!bindProcessAborted && newCandidateRanksHigher) {
          MEMMOVE(&pairingCandidatesList[lowestCandidateIndex],
                  &candidate,
                  sizeof(EmAfGdpPairingCanditate));
          debugCandidateAdded("lower ranked candidate found");
        } else {
          debugDiscoveryResponseDrop("table full, no lower ranked candidate");
        }
      }
    } else {
      // Add the new candidate to the pairing candidate list.
      MEMMOVE(&pairingCandidatesList[freeEntryIndex],
              &candidate,
              sizeof(EmAfGdpPairingCanditate));
      debugCandidateAdded("free entry");

      // This is really to check whether the newly added entry causes the
      // binding process to be aborted.
      getLowestOrHighestRankedCandidateIndex(LOWEST,
                                             ALL_CANDIDATES,
                                             &bindProcessAborted);
    }
  } else {
    bool lqiTieBreak;
    debugScriptCheck("duplicate recipient");
    // If the Binding Recipient that transmitted the discovery response is
    // already in the list of binding candidates, the following procedure is
    // used:
    // - The Binding Originator shall check if this discovery response results
    //   in a better ranking for the Binding Recipient than the current ranking,
    //   by using the ranking algorithm as explained above.
    // - If this discovery response indeed results in a better ranking, this
    //   Binding Recipient shall be assigned this better ranking, and in case
    //   the higher ranking is caused by a lower class number, and not just by a
    //   better LQI used for tie breaking, the Binding Recipient shall be marked
    //   as fresh again.
    // - If this discovery response does not result in a better ranking, the
    //   Binding Recipient preserves its current ranking in the list of binding
    //   candidates.
    if (comparePairingCandidates(&candidate,
                                 &pairingCandidatesList[sameRecipientEntryIndex],
                                 NULL,
                                 &lqiTieBreak)) {
      if (lqiTieBreak
          && (pairingCandidatesList[sameRecipientEntryIndex].info
              & CANDIDATE_INFO_PAIRING_ATTEMPTED_BIT)) {
        debugScriptCheck("LQI tie breaker, old entry not refreshed");
        candidate.info |= CANDIDATE_INFO_PAIRING_ATTEMPTED_BIT;
      }
      MEMMOVE(&pairingCandidatesList[sameRecipientEntryIndex],
              &candidate,
              sizeof(EmAfGdpPairingCanditate));
    }
  }

  // If the bind process was aborted, we can terminate the discovery process
  // prematurely.
  return !bindProcessAborted;
}

void emberAfPluginRf4ceProfileGdpDiscoveryCompleteCallback(EmberStatus status)
{
  uint8_t highestCandidateIndex;
  bool abort;

  if (publicBindState() != EMBER_AF_RF4CE_GDP_BINDING_STATE_BINDING) {
    return;
  }

  highestCandidateIndex =
      getLowestOrHighestRankedCandidateIndex(HIGHEST, ALL_CANDIDATES, &abort);

  // Section 7.2.7.1
  // The Binding Originator shall then use the procedure described in 7.2.4,
  // to select the best binding candidate from the updated list of Binding
  // Candidates.  If this best binding candidate is different from the binding
  // candidate is currently validating with, and if the best binding candidate
  // has the class number set to Button Press Indication, the current
  // validation procedure shall be immediately terminated, the Binding
  // Originator shall remove the pairing entry (HERE WE ACTUALLY DO THE RESTORE
  // OR DELETE AS WE DO WHEN A BINDING FAILS) of the binding candidate it was
  // validating with and the Binding Originator shall enter the Select best
  // binding candidate state, which will result in an attempt to pair/bind with
  // the new best binding candidate.
#if (defined(EMBER_AF_PLUGIN_RF4CE_GDP_EXTENDED_VALIDATION) || defined(EMBER_SCRIPTED_TEST))
  if (internalGdpState() == INTERNAL_STATE_ORIGINATOR_GDP_VALIDATION) {
    if (highestCandidateIndex != 0xFF
        && emAfGdpPeerInfo.candidateIndex != highestCandidateIndex) {
      uint8_t classNumber =
          (pairingCandidatesList[highestCandidateIndex].primaryClassDescriptor
           & CLASS_DESCRIPTOR_NUMBER_MASK) >> CLASS_DESCRIPTOR_NUMBER_OFFSET;

      if (classNumber == CLASS_NUMBER_BUTTON_PRESS_INDICATION) {
        setInternalState(INTERNAL_STATE_NONE);

        if (!maybeRestoreSavedPairingEntry()
            && emAfTemporaryPairingIndex != 0xFF) {
          emberAfRf4ceSetPairingTableEntry(emAfTemporaryPairingIndex, NULL);
        }

        if (startPairingProcess() != EMBER_SUCCESS) {
          tryRestartDiscoveryProcessOrFail(EMBER_AF_RF4CE_GDP_BINDING_STATUS_PAIRING_FAILED);
        }

        return;
      }
    }

    // Restart the previous validation process. We simply reschedule the
    // blackout timer and leave the internal state set to validation.
    emberEventControlSetActive(emberAfPluginRf4ceGdpBlackoutTimeEventControl);
    debugScriptCheck("No better candidate, restart suspended validation process");

    return;
  }
#endif

  if (bindProcessAborted || abort) {
    failBindingProcedure(EMBER_AF_RF4CE_GDP_BINDING_STATUS_DUPLICATE_CLASS_ABORT);
    return;
  }

  if (highestCandidateIndex == 0xFF) {
    tryRestartDiscoveryProcessOrFail(EMBER_AF_RF4CE_GDP_BINDING_STATUS_NO_VALID_RESPONSE);
  } else if (startPairingProcess() != EMBER_SUCCESS) {
    // At least a valid discovery response was received. If the pairing process
    // doesn't start successfully, we fall back to the check timer procedure.
    tryRestartDiscoveryProcessOrFail(EMBER_AF_RF4CE_GDP_BINDING_STATUS_PAIRING_FAILED);
  }
}

void emAfRf4ceGdpPairCompleteOriginatorCallback(EmberStatus status,
                                                uint8_t pairingIndex,
                                                const EmberRf4ceVendorInfo *vendorInfo,
                                                const EmberRf4ceApplicationInfo *appInfo)
{
  if (publicBindState() != EMBER_AF_RF4CE_GDP_BINDING_STATE_BINDING) {
    return;
  }

  if (status == EMBER_SUCCESS || status == EMBER_DUPLICATE_ENTRY) {
    uint8_t gdpVersion = emAfRf4ceGdpGetGdpVersion(appInfo->profileIdList,
                                                 emberAfRf4ceProfileIdListLength(appInfo->capabilities));

    // First, we clear the remote node attributes
    emAfRf4ceGdpClearRemoteAttributes();

    if (gdpVersion == GDP_VERSION_2_0) {
      uint8_t bindStatus = (emAfRf4ceGdpGetPairingBindStatus(pairingIndex)
                          & ~PAIRING_ENTRY_BINDING_COMPLETE_BIT);

      // Clear the 'bind complete' bit. This is to detect that a node that was
      // already bound rebooted during the validation procedure, thus without
      // having restored the previous pairing entry. In this case, upon init we
      // just clear the corresponding pairing entry.
      emAfRf4ceGdpSetPairingBindStatus(pairingIndex, bindStatus);

      // Save/update the supported profile ID list
      pairingCandidatesList[emAfGdpPeerInfo.candidateIndex].supportedProfilesLength =
          emberAfRf4ceProfileIdListLength(appInfo->capabilities);
      MEMMOVE(pairingCandidatesList[emAfGdpPeerInfo.candidateIndex].supportedProfiles,
              appInfo->profileIdList,
              pairingCandidatesList[emAfGdpPeerInfo.candidateIndex].supportedProfilesLength);

      // After pairing and after the configuration procedure of each profile the
      // node shall wait for aplcConfigBlackoutTime.
      emAfGdpStartBlackoutTimer(INTERNAL_STATE_ORIGINATOR_GDP_CONFIG_PUSH_PENDING);
      emAfTemporaryPairingIndex = pairingIndex;
    } else { // We completed a non-GDP or a GDP 1.x pairing process. We finish
             // the binding process here.
      // Enter the 'bound' state.
      setPublicState(EMBER_AF_RF4CE_GDP_BINDING_STATE_BOUND, false);

      // Write the bind status for the corresponding pairing entry.
      emAfRf4ceGdpSetPairingBindStatus(pairingIndex,
                                       ((PAIRING_ENTRY_BINDING_STATUS_BOUND_ORIGINATOR
                                         << PAIRING_ENTRY_BINDING_STATUS_OFFSET)
                                        | PAIRING_ENTRY_BINDING_COMPLETE_BIT));

      // We are done with binding, call bindingComplete() callback.
      bindingProcedureComplete(EMBER_AF_RF4CE_GDP_BINDING_STATUS_SUCCESS,
                               pairingIndex);
    }
  } else {
    // We failed proxy binding on the current channel.
    if (pairingCandidatesList[emAfGdpPeerInfo.candidateIndex].info
        & CANDIDATE_INFO_PROXY_CANDIDATE_BIT) {
      EmberStatus pairingStatus = EMBER_ERR_FATAL;
      // Try the next channel if all the channels haven't been attempted yet.
      // Otherwise fail the proxy bind.
      if (pairingCandidatesList[emAfGdpPeerInfo.candidateIndex].channel <= 20) {
        // Make the candidate fresh again.
        pairingCandidatesList[emAfGdpPeerInfo.candidateIndex].info &=
            ~CANDIDATE_INFO_PAIRING_ATTEMPTED_BIT;
        // Go to the next channel.
        pairingCandidatesList[emAfGdpPeerInfo.candidateIndex].channel += 5;
        pairingStatus = startPairingProcess();
      }

      if (pairingStatus != EMBER_SUCCESS
          && pairingStatus != EMBER_DUPLICATE_ENTRY) {
        failBindingProcedure(EMBER_AF_RF4CE_GDP_BINDING_STATUS_PAIRING_FAILED);
      }
    } else {
      // On receipt of an unsuccessful confirmation of pairing from the network
      // layer, the binding originator shall enter the check binding procedure
      // timer state.
      tryRestartDiscoveryProcessOrFail(EMBER_AF_RF4CE_GDP_BINDING_STATUS_PAIRING_FAILED);
    }
  }
}

void emAfRf4ceGdpIncomingGenericResponseOriginatorCallback(EmberAfRf4ceGdpResponseCode status)
{
  if (internalGdpState()
      == INTERNAL_STATE_ORIGINATOR_GDP_CONFIG_PUSH_PENDING
      && emAfTemporaryPairingIndex == emberAfRf4ceGetPairingIndex()) {
    EmberAfRf4ceGdpAttributeIdentificationRecord attributes[2];

    // Fill the fields of the two records (entry ID is non-relevant for
    // scalar type attributes).
    attributes[0].attributeId = EMBER_AF_RF4CE_GDP_ATTRIBUTE_VERSION;
    attributes[1].attributeId = EMBER_AF_RF4CE_GDP_ATTRIBUTE_CAPABILITIES;

    // Successful generic response received: send a getAttributes for the
    // attributes aplGDPVersion and aplGDPCapabilities.
    if (status == EMBER_AF_RF4CE_GDP_RESPONSE_CODE_SUCCESSFUL) {
      emberAfRf4ceGdpGetAttributes(emAfTemporaryPairingIndex,
                                   EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE,
                                   EMBER_RF4CE_NULL_VENDOR_ID,
                                   attributes,
                                   2);
      emAfGdpStartCommandPendingTimer(INTERNAL_STATE_ORIGINATOR_GDP_CONFIG_GET_PENDING,
                                      APLC_MAX_CONFIG_WAIT_TIME_MS);
    } else {
      tryRestartDiscoveryProcessOrFail(EMBER_AF_RF4CE_GDP_BINDING_STATUS_CONFIG_FAILED);
    }
  } else if (internalGdpState()
             == INTERNAL_STATE_ORIGINATOR_GDP_CONFIG_COMPLETE_PENDING) {
    // If both the Configuration Complete command frame and the corresponding
    // Generic Response frame have their status field set to SUCCESS, the
    // binding originator shall consider the configuration a success. Otherwise
    // it shall consider the configuration a failure.
    if (emAfGdpPeerInfo.localConfigurationStatus
        == EMBER_AF_RF4CE_GDP_STATUS_SUCCESSFUL
        && status == EMBER_AF_RF4CE_GDP_RESPONSE_CODE_SUCCESSFUL) {
      debugScriptCheck("GDP configuration procedure completed");

      emberEventControlSetInactive(emberAfPluginRf4ceGdpPendingCommandEventControl);

      // Configuration procedure has been completed successfully. Move to the
      // profile-specific configuration procedures.
      // After pairing and after the configuration procedure of each profile the
      // node shall wait for aplcConfigBlackoutTime.
      emAfGdpStartBlackoutTimer(INTERNAL_STATE_ORIGINATOR_GDP_PROFILES_CONFIG);

      emAfRf4ceGdpNoteProfileSpecificConfigurationStart();
    } else {
      tryRestartDiscoveryProcessOrFail(EMBER_AF_RF4CE_GDP_BINDING_STATUS_CONFIG_FAILED);
    }
  }
}

bool emAfRf4ceGdpIncomingGetAttributesResponseOriginatorCallback(void)
{
  EmberAfRf4ceGdpAttributeStatusRecord statusRecord;

  if (internalGdpState() == INTERNAL_STATE_ORIGINATOR_GDP_CONFIG_GET_PENDING
      && emAfTemporaryPairingIndex == emberAfRf4ceGetPairingIndex()
      && emAfRf4ceGdpFetchAttributeStatusRecord(&statusRecord)
      && statusRecord.attributeId == EMBER_AF_RF4CE_GDP_ATTRIBUTE_VERSION
      && statusRecord.status == EMBER_AF_RF4CE_GDP_ATTRIBUTE_STATUS_SUCCESS
      && emAfRf4ceGdpFetchAttributeStatusRecord(&statusRecord)
      && statusRecord.attributeId == EMBER_AF_RF4CE_GDP_ATTRIBUTE_CAPABILITIES
      && statusRecord.status == EMBER_AF_RF4CE_GDP_ATTRIBUTE_STATUS_SUCCESS) {
    EmberAfRf4ceGdpAttributeIdentificationRecord attributes[2];

    // Send a pullAttributes for the attributes aplAutoCheckValidationPeriod
    // and aplLinkLostWaitTime.
    attributes[0].attributeId =
        EMBER_AF_RF4CE_GDP_ATTRIBUTE_AUTO_CHECK_VALIDATION_PERIOD;
    attributes[1].attributeId =
        EMBER_AF_RF4CE_GDP_ATTRIBUTE_LINK_LOST_WAIT_TIME;

    emberAfRf4ceGdpPullAttributes(emAfTemporaryPairingIndex,
                                  EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE,
                                  EMBER_RF4CE_NULL_VENDOR_ID,
                                  attributes,
                                  2);
    emAfGdpStartCommandPendingTimer(INTERNAL_STATE_ORIGINATOR_GDP_CONFIG_PULL_PENDING,
                                    APLC_MAX_CONFIG_WAIT_TIME_MS);
    return true;
  }

  return false;
}

bool emAfRf4ceGdpIncomingPullAttributesResponseOriginatorCallback(void)
{
  EmberAfRf4ceGdpAttributeStatusRecord statusRecord;

  if (internalGdpState() == INTERNAL_STATE_ORIGINATOR_GDP_CONFIG_PULL_PENDING
      && emAfTemporaryPairingIndex == emberAfRf4ceGetPairingIndex()
      && emAfRf4ceGdpFetchAttributeStatusRecord(&statusRecord)
      && statusRecord.attributeId
         == EMBER_AF_RF4CE_GDP_ATTRIBUTE_AUTO_CHECK_VALIDATION_PERIOD
      && statusRecord.status == EMBER_AF_RF4CE_GDP_ATTRIBUTE_STATUS_SUCCESS
      && emAfRf4ceGdpFetchAttributeStatusRecord(&statusRecord)
      && statusRecord.attributeId
         == EMBER_AF_RF4CE_GDP_ATTRIBUTE_LINK_LOST_WAIT_TIME
      && statusRecord.status == EMBER_AF_RF4CE_GDP_ATTRIBUTE_STATUS_SUCCESS) {

    // TODO: for now we just accept any value, we need to clarify what are the
    // criteria for deciding whether the retrieved parameters are satisfactory
    // or not.
    emAfGdpPeerInfo.localConfigurationStatus =
        EMBER_AF_RF4CE_GDP_RESPONSE_CODE_SUCCESSFUL;

    emberAfRf4ceGdpConfigurationComplete(emAfTemporaryPairingIndex,
                                         EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE,
                                         EMBER_RF4CE_NULL_VENDOR_ID,
                                         emAfGdpPeerInfo.localConfigurationStatus);
    emAfGdpStartCommandPendingTimer(INTERNAL_STATE_ORIGINATOR_GDP_CONFIG_COMPLETE_PENDING,
                                    APLC_MAX_CONFIG_WAIT_TIME_MS);

    return true;
  }

  return false;
}

void emAfRf4ceGdpBasedProfileConfigurationCompleteOriginatorCallback(bool success)
{
  if (success) {
    // After pairing and after the configuration procedure of each profile the
    // node shall wait for aplcConfigBlackoutTime.
    emAfGdpStartBlackoutTimer(internalGdpState());
  } else {
    tryRestartDiscoveryProcessOrFail(EMBER_AF_RF4CE_GDP_BINDING_STATUS_PROFILE_SPECIFIC_CONFIG_FAILED);
  }
}

void emAfRf4ceGdpIncomingCheckValidationResponseOriginator(EmberAfRf4ceGdpCheckValidationStatus status)
{
  // The binding originator shall start the aplLinkLostWaitTime timeout when
  // the validation phase starts. If a check validation response command frame
  // is received from the binding recipient, the timeout shall restarted.
  // We reuse the commandPending event for handling this.
  emberEventControlSetDelayMS(emberAfPluginRf4ceGdpPendingCommandEventControl,
                              emAfRf4ceGdpLocalNodeAttributes.linkLostWaitTime);

  // If the CheckValidationStatus field was set to 'success', the validation has
  // succeeded. If it was set to 'failure' or 'timeout' the validation is
  // unsuccessful. If it was set to 'pending' the validation will continue.
  if (emAfTemporaryPairingIndex == emberAfRf4ceGetPairingIndex()
      && status == EMBER_AF_RF4CE_GDP_CHECK_VALIDATION_STATUS_SUCCESS) {

    uint8_t bindStatus = ((PAIRING_ENTRY_BINDING_STATUS_BOUND_ORIGINATOR
                         << PAIRING_ENTRY_BINDING_STATUS_OFFSET)
                        | PAIRING_ENTRY_BINDING_COMPLETE_BIT
                        | ((emAfRf4ceGdpRemoteNodeAttributes.gdpCapabilities
                            & GDP_CAPABILITIES_SUPPORT_ENHANCED_SECURITY_BIT)
                           ? PAIRING_ENTRY_REMOTE_NODE_SUPPORTS_ENHANCED_SECURITY_BIT
                           : 0));

    // Write the bind status for the corresponding pairing entry.
    emAfRf4ceGdpSetPairingBindStatus(emAfTemporaryPairingIndex, bindStatus);

    emberEventControlSetInactive(emberAfPluginRf4ceGdpBlackoutTimeEventControl);
    emberEventControlSetInactive(emberAfPluginRf4ceGdpValidationEventControl);
    emberEventControlSetInactive(emberAfPluginRf4ceGdpPendingCommandEventControl);

    // If the validation procedure was successful, the binding initiator shall
    // enter the 'bound' state.
    setPublicState(EMBER_AF_RF4CE_GDP_BINDING_STATE_BOUND, false);

    // Let the security code save the pairing link key.
    emAfRf4ceGdpSecurityValidationCompleteCallback(emAfTemporaryPairingIndex);

#if defined(EMBER_AF_PLUGIN_RF4CE_GDP_ENHANCED_SECURITY) || defined(EMBER_SCRIPTED_TEST)
    // Section 7.4.1: "The key exchange procedure must be initiated by the
    // originator node immediately after the binding process is completed, if
    // both nodes have indicated support for enhanced security."
    if ((emAfRf4ceGdpRemoteNodeAttributes.gdpCapabilities
         & GDP_CAPABILITIES_SUPPORT_ENHANCED_SECURITY_BIT)
        && (emAfRf4ceGdpLocalNodeAttributes.gdpCapabilities
            & GDP_CAPABILITIES_SUPPORT_ENHANCED_SECURITY_BIT)) {
      // We observe blackout period before kicking off the key exchange
      // procedure.
      emAfGdpStartBlackoutTimer(INTERNAL_STATE_ORIGINATOR_GDP_KEY_EXCHANGE_BLACKOUT_PENDING);
    }
#endif // EMBER_AF_PLUGIN_RF4CE_GDP_ENHANCED_SECURITY

    // Notify the polling code that a binding has been established/updated.
    emAfRf4ceGdpPollingNotifyBindingComplete(emAfTemporaryPairingIndex);

    // Notify the identification code that a binding has been established or
    // updated.
    emAfRf4ceGdpIdentificationNotifyBindingComplete(emAfTemporaryPairingIndex);

    // We are done with binding, call bindingComplete() callback.
    bindingProcedureComplete(EMBER_AF_RF4CE_GDP_BINDING_STATUS_SUCCESS,
                             emAfTemporaryPairingIndex);
  } else if (status == EMBER_AF_RF4CE_GDP_CHECK_VALIDATION_STATUS_PENDING) {
    // nothing to do, wait for the timer to expire and send a new
    // checkValidationRequest command.
  } else {
    debugScriptCheck("Failure/timeout response status, validation failed");
    tryRestartDiscoveryProcessOrFail(EMBER_AF_RF4CE_GDP_BINDING_STATUS_PROFILE_SPECIFIC_CONFIG_FAILED);
  }
}

//------------------------------------------------------------------------------
// Static functions.

static EmberStatus setDiscoveryRequestAppInfo(void)
{
  EmberRf4ceApplicationInfo appInfo;
  uint8_t appUserString[USER_STRING_APPLICATION_SPECIFIC_USER_STRING_LENGTH] =
      EMBER_AF_PLUGIN_RF4CE_GDP_APPLICATION_SPECIFIC_USER_STRING;
  uint8_t *finger = appInfo.userString;

  MEMMOVE(&appInfo,
          &emAfRf4ceApplicationInfo,
          sizeof(EmberRf4ceApplicationInfo));

  // Bytes 0-7 are "Application specific user string".
  MEMMOVE(finger,
          appUserString,
          USER_STRING_APPLICATION_SPECIFIC_USER_STRING_LENGTH);
  finger += USER_STRING_APPLICATION_SPECIFIC_USER_STRING_LENGTH;

  // Byte 8 is a NULL octet.
  *finger++ = 0x00;

  // Bytes 9-10 are the vendor ID filter.
  *finger++ = LOW_BYTE(EMBER_AF_PLUGIN_RF4CE_GDP_VENDOR_ID_FILTER);
  *finger++ = HIGH_BYTE(EMBER_AF_PLUGIN_RF4CE_GDP_VENDOR_ID_FILTER);

  // Byte 11: Min/Max class filter.
  *finger++ = ((EMBER_AF_PLUGIN_RF4CE_GDP_MIN_CLASS_FILTER
                << USER_STRING_DISC_REQUEST_MIN_MAX_CLASS_FILTER_MIN_CLASS_NUM_OFFSET)
               | (EMBER_AF_PLUGIN_RF4CE_GDP_MAX_CLASS_FILTER
                  << USER_STRING_DISC_REQUEST_MIN_MAX_CLASS_FILTER_MAX_CLASS_NUM_OFFSET));

  // Byte 12: Min LQI filter.
  *finger++ = EMBER_AF_PLUGIN_RF4CE_GDP_MIN_LQI_FILTER;

  // Bytes 13-14 are reserved bytes (we set them to 0).
  *finger++ = 0;
  *finger = 0;

  // Set the profile ID list to that passed in the bind() API.
  MEMMOVE(appInfo.profileIdList, savedProfileIdList, savedProfileIdListLength);
  appInfo.capabilities &=
      ~EMBER_RF4CE_APP_CAPABILITIES_SUPPORTED_PROFILES_MASK;
  appInfo.capabilities |=
      (savedProfileIdListLength
       << EMBER_RF4CE_APP_CAPABILITIES_SUPPORTED_PROFILES_OFFSET);

  appInfo.capabilities |= EMBER_RF4CE_APP_CAPABILITIES_USER_STRING_BIT;

  return emberAfRf4ceSetApplicationInfo(&appInfo);
}

static EmberStatus setPairRequestAppInfo(uint8_t *targetProfileIdList,
                                         uint8_t targetProfileIdListLength,
                                         uint8_t gdpVersion,
                                         bool isProxy)
{
  EmberRf4ceApplicationInfo appInfo;
  uint8_t appUserString[USER_STRING_APPLICATION_SPECIFIC_USER_STRING_LENGTH] =
      EMBER_AF_PLUGIN_RF4CE_GDP_APPLICATION_SPECIFIC_USER_STRING;
  uint8_t *finger = appInfo.userString;

  MEMMOVE(&appInfo,
          &emAfRf4ceApplicationInfo,
          sizeof(EmberRf4ceApplicationInfo));


  // Bytes 0-7 are "Application specific user string".
  MEMMOVE(finger,
          appUserString,
          USER_STRING_APPLICATION_SPECIFIC_USER_STRING_LENGTH);
  finger += USER_STRING_APPLICATION_SPECIFIC_USER_STRING_LENGTH;

  // Byte 8 is a NULL octet.
  *finger++ = 0x00;

  // Byte 9: advanced binding support
  *finger++ = ((isProxy)
               ? ADVANCED_BINDING_SUPPORT_FIELD_BINDING_PROXY_SUPPORTED_BIT
               : 0x00);

  // Bytes 10-14 are reserved bytes (we set them to 0).
  MEMSET(finger, 0x00, USER_STRING_PAIR_REQUEST_RESERVED_BYTES_LENGTH);

  appInfo.capabilities |= EMBER_RF4CE_APP_CAPABILITIES_USER_STRING_BIT;

  // Set the profile ID list to 0.
  appInfo.capabilities &= ~EMBER_RF4CE_APP_CAPABILITIES_SUPPORTED_PROFILES_MASK;

  // If this is a proxy bind, we add all the GDP 2.0-based profile IDs passed
  // in the API.
  if (isProxy) {
    emAfGdpAddToProfileIdList(savedProfileIdList,
                              savedProfileIdListLength,
                              &appInfo,
                              gdpVersion);
  } else {
    // In the pair request we only include those supported profiles IDs based on
    // the passed version.
    uint8_t matchingProfileIdList[EMBER_RF4CE_APPLICATION_PROFILE_ID_LIST_MAX_LENGTH];
    uint8_t matchingProfileIdListLength =
        emAfCheckDeviceTypeAndProfileIdMatch(0xFF, // we don't bother matching the device type here.
                                             (uint8_t*)emAfRf4ceApplicationInfo.deviceTypeList,
                                             emberAfRf4ceDeviceTypeListLength(emAfRf4ceApplicationInfo.capabilities),
                                             emAfRf4ceApplicationInfo.profileIdList,
                                             emberAfRf4ceProfileIdListLength(emAfRf4ceApplicationInfo.capabilities),
                                             targetProfileIdList,
                                             targetProfileIdListLength,
                                             matchingProfileIdList);

    emAfGdpAddToProfileIdList(matchingProfileIdList,
                              matchingProfileIdListLength,
                              &appInfo,
                              gdpVersion);
  }

  return emberAfRf4ceSetApplicationInfo(&appInfo);
}

// The following two functions implement the wording stated at section 7.2.2
// of the GDP 2.0 specs: "In case the binding originator re-paired with a
// binding recipient to which it was bound before the binding procedure started,
// the corresponding pairing table entry shall be restored with the values it
// had before the binding procedure started".

static void maybeSavePairingEntry(EmberEUI64 ieeeAddr)
{
  uint8_t i;

  savedPairingEntryIndex = 0xFF;

  for(i=0; i<EMBER_RF4CE_PAIRING_TABLE_SIZE; i++) {
    uint8_t bindStatus = (emAfRf4ceGdpGetPairingBindStatus(i)
                        & EMBER_RF4CE_PAIRING_TABLE_ENTRY_INFO_STATUS_MASK);
    emberAfRf4ceGetPairingTableEntry(i, &savedPairingEntry);

    // if the bind status for this pairing entry is "bound", the pairing entry
    // is active and the IEEE address matches the passed IEEE address, save the
    // pairing entry.
    if ((bindStatus == PAIRING_ENTRY_BINDING_STATUS_BOUND_ORIGINATOR
         || bindStatus == PAIRING_ENTRY_BINDING_STATUS_BOUND_RECIPIENT)
        && (savedPairingEntry.info & EMBER_RF4CE_PAIRING_TABLE_ENTRY_INFO_STATUS_MASK)
           == EMBER_RF4CE_PAIRING_TABLE_ENTRY_STATUS_ACTIVE
        && MEMCOMPARE(ieeeAddr, savedPairingEntry.destLongId, EUI64_SIZE) == 0) {
      savedPairingEntryIndex = i;
      debugScriptCheck("Saving pairing entry");
      return;
    }
  }
}

static bool maybeRestoreSavedPairingEntry(void)
{
  if (savedPairingEntryIndex < 0xFF) {
    uint8_t bindStatus = (emAfRf4ceGdpGetPairingBindStatus(savedPairingEntryIndex)
                        | PAIRING_ENTRY_BINDING_COMPLETE_BIT);

    // Set the 'bind complete' bit.
    emAfRf4ceGdpSetPairingBindStatus(savedPairingEntryIndex, bindStatus);

    emberAfRf4ceSetPairingTableEntry(savedPairingEntryIndex,
                                     &savedPairingEntry);
    savedPairingEntryIndex = 0xFF;
    // We set this back to 0xFF here to avoid deleting the pairing entry in
    // the tryRestartDiscoveryProcessOrFail() function.
    emAfTemporaryPairingIndex = 0xFF;
    debugScriptCheck("Restoring pairing entry");
    return true;
  } else {
    return false;
  }
}

static void tryRestartDiscoveryProcessOrFail(EmberAfRf4ceGdpBindingStatus failReason)
{
  // If any of the configuration procedures fail due to some communication
  // error or if the received generic response command frame indicated a
  // configuration error, the binding originator shall remove the pairing
  // entry for the current binding recipient and it shall continue the
  // binding procedure by entering the check binding validation timer
  // procedure.
  // However, we first attempt to restore a previously saved pairing. This
  // covers the case where the binding process failed while re-binding to a
  // previously bound node (see section 7.2.2).
  if (!maybeRestoreSavedPairingEntry()
      && emAfTemporaryPairingIndex != 0xFF) {
    emberAfRf4ceSetPairingTableEntry(emAfTemporaryPairingIndex, NULL);
  }

  // Going back to the discovery procedure always clears the internal state.
  setInternalState(INTERNAL_STATE_NONE);

  // Set timers inactive.
  emberEventControlSetInactive(emberAfPluginRf4ceGdpPendingCommandEventControl);
  emberEventControlSetInactive(emberAfPluginRf4ceGdpBlackoutTimeEventControl);
  emberEventControlSetInactive(emberAfPluginRf4ceGdpValidationEventControl);

  if (!checkBindingProcedureTimer()
      || startDiscoveryProcess(NULL, 0, 0) != EMBER_SUCCESS) {
    failBindingProcedure(failReason);
  }
}

static EmberStatus startDiscoveryProcess(uint8_t* profileIdList,
                                         uint8_t profileIdListLength,
                                         uint8_t searchDevType)
{
  EmberStatus status;

  // Save the profile ID list for potential future use. This must happen before
  // we call setDiscoveryRequestAppInfo().
  if (profileIdList) {
    MEMMOVE(savedProfileIdList, profileIdList, profileIdListLength);
    savedProfileIdListLength = profileIdListLength;
    savedSearchDevType = searchDevType;
  }

  status = emberAfRf4ceSetDiscoveryLqiThreshold(NWK_DISCOVERY_LQI_THRESHOLD);

  // Set the user string according to section 7.2.3.1
  if (status == EMBER_SUCCESS) {
    status = setDiscoveryRequestAppInfo();
  }

  if (status == EMBER_SUCCESS) {
    status = emberAfRf4ceDiscovery(EMBER_RF4CE_BROADCAST_PAN_ID,
                                   EMBER_RF4CE_BROADCAST_ADDRESS,
                                   ((profileIdList)
                                    ? searchDevType
                                    : savedSearchDevType),
                                   NWK_DISCOVERY_REPETITION_INTERVAL_MS / 3,
                                   NWK_MAX_DISCOVERY_REPETITIONS,
                                   ((profileIdList)
                                    ? profileIdListLength
                                    : savedProfileIdListLength),
                                   ((profileIdList)
                                    ? profileIdList
                                    : savedProfileIdList));
  }

  return status;
}

static EmberStatus startPairingProcess(void)
{
  bool abort;
  uint8_t gdpVersion;
  bool isProxy;
  uint8_t highestCandidateIndex =
      getLowestOrHighestRankedCandidateIndex(HIGHEST, FRESH_ONLY, &abort);

  // At this point the pairing candidates list has been checked for abort
  // already, so we assert that abort = false hare.
  assert(!abort);

  if (highestCandidateIndex == 0xFF) {
    return EMBER_INVALID_CALL;
  }

  // Search the pairing table for a pairing entry with the highest candidate
  // IEEE address. If such an entry exists, save it so that we can restore it
  // in case the binding process fails.
  maybeSavePairingEntry(pairingCandidatesList[highestCandidateIndex].ieeeAddr);

  isProxy = ((pairingCandidatesList[highestCandidateIndex].info
              & CANDIDATE_INFO_PROXY_CANDIDATE_BIT) > 0);

  // Proxy recipient is always assumed to support GDP 2.0
  gdpVersion = ((isProxy) ? GDP_VERSION_2_0
                          : emAfRf4ceGdpGetGdpVersion(pairingCandidatesList[highestCandidateIndex].supportedProfiles,
                                                      pairingCandidatesList[highestCandidateIndex].supportedProfilesLength));

  // If the selected pairing candidate references a GDP 2.0 based profile,
  // the binding originator shall enter the "pairing state" in attempt to
  // pair with this pairing candidate, as described in sections 7.2.5,
  // 7.2.6 and 7.2.7.
  if (gdpVersion == GDP_VERSION_NONE) {
    bool zrc11Binding = false;
    uint8_t i;

    // If the selected pairing candidate references a non-GDP-based profile,
    // the binding originator shall attempt pairing with this paring candidate
    // as described in the corresponding profile specification.
    debugScriptCheck("starting non-GDP pairing process");

    for(i=0; i<pairingCandidatesList[highestCandidateIndex].supportedProfilesLength; i++) {
      if (pairingCandidatesList[highestCandidateIndex].supportedProfiles[i]
          == EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_1_1) {
        zrc11Binding = true;
        break;
      }
    }

    // TODO: non-GDP: for now we only support binding to a ZRC 1.1 recipient
    if (!zrc11Binding) {
      return EMBER_SUCCESS;
    }
  } else if (gdpVersion == GDP_VERSION_1_X) {
    debugScriptCheck("starting GDP 1.x pairing process");
  } else {
    debugScriptCheck("starting GDP 2.0 pairing process");
  }

  if (setPairRequestAppInfo(pairingCandidatesList[highestCandidateIndex].supportedProfiles,
                            pairingCandidatesList[highestCandidateIndex].supportedProfilesLength,
                            gdpVersion,
                            isProxy) != EMBER_SUCCESS) {
    return EMBER_ERR_FATAL;
  } else {
    EmberStatus status =
        emberAfRf4cePair(pairingCandidatesList[highestCandidateIndex].channel,
                         pairingCandidatesList[highestCandidateIndex].panId,
                         pairingCandidatesList[highestCandidateIndex].ieeeAddr,
                         APLC_MIN_KEY_EXCHANGE_TRANSFER_COUNT,
                         emAfRf4ceGdpPairCompleteOriginatorCallback);
    // Mark the candidate as "not fresh".
    if (status == EMBER_SUCCESS || status == EMBER_DUPLICATE_ENTRY) {
      pairingCandidatesList[highestCandidateIndex].info |=
          CANDIDATE_INFO_PAIRING_ATTEMPTED_BIT;
      // Save the candidate index in the peer info struct.
      emAfGdpPeerInfo.candidateIndex = highestCandidateIndex;
    }

    if (status == EMBER_DUPLICATE_ENTRY) {
      status = EMBER_SUCCESS;
    }

    return status;
  }
}

static void startGdpConfigurationProcedureOrFail(void)
{
  EmberAfRf4ceGdpAttributeRecord attributes[2];
  uint8_t version[APL_GDP_VERSION_SIZE];
  uint8_t capabilities[APL_GDP_CAPABILITIES_SIZE];

  emAfRf4ceGdpGetLocalAttribute(EMBER_AF_RF4CE_GDP_ATTRIBUTE_VERSION, 0, version);
  attributes[0].attributeId = EMBER_AF_RF4CE_GDP_ATTRIBUTE_VERSION;
  attributes[0].valueLength = APL_GDP_VERSION_SIZE;
  attributes[0].value = (uint8_t*)version;

  emAfRf4ceGdpGetLocalAttribute(EMBER_AF_RF4CE_GDP_ATTRIBUTE_CAPABILITIES, 0, capabilities);
  attributes[1].attributeId = EMBER_AF_RF4CE_GDP_ATTRIBUTE_CAPABILITIES;
  attributes[1].valueLength = APL_GDP_CAPABILITIES_SIZE;
  attributes[1].value = (uint8_t*)capabilities;

  // Send a pushAttributes command to the recipient for the attributes
  // aplGDPVersion and aplGDPCapabilities.
  emberAfRf4ceGdpPushAttributes(emAfTemporaryPairingIndex,
                                EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE,
                                EMBER_RF4CE_NULL_VENDOR_ID,
                                attributes,
                                2);
  emAfGdpStartCommandPendingTimer(INTERNAL_STATE_ORIGINATOR_GDP_CONFIG_PUSH_PENDING,
                                  APLC_MAX_CONFIG_WAIT_TIME_MS);
}


static void startValidationProcedure(void)
{
  debugScriptCheck("Starting validation procedure");

  setInternalState(INTERNAL_STATE_ORIGINATOR_GDP_VALIDATION);

  // To verify if the validation is successful, the binding originator will
  // generate and transmit a CheckValidationRequest command frame to the
  // binding recipient every aplAutoCheckValidationPeriod. The first
  // CheckValidationRequest should be sent aplAutoCheckValidationPeriod after
  // the validation phase starts.
  // We reuse the blackout event for handling this.
  emberEventControlSetDelayMS(emberAfPluginRf4ceGdpBlackoutTimeEventControl,
                              emAfRf4ceGdpLocalNodeAttributes.autoCheckValidationPeriod);

  // The binding originator shall start the aplLinkLostWaitTime timeout when
  // the validation phase starts. If a check validation response command frame
  // is received from the binding recipient, the timeout shall restarted.
  // We reuse the commandPending event for handling this.
  emberEventControlSetDelayMS(emberAfPluginRf4ceGdpPendingCommandEventControl,
                              emAfRf4ceGdpLocalNodeAttributes.linkLostWaitTime);

#if defined(EMBER_SCRIPTED_TEST)
  if (emAfRf4ceGdpLocalNodeAttributes.gdpCapabilities
      & GDP_CAPABILITIES_SUPPORT_EXTENDED_VALIDATION_BIT) {
    emberEventControlSetDelayMS(emberAfPluginRf4ceGdpValidationEventControl,
                                APLC_MAX_EXTENDED_VALIDATION_DURATION_MS);
  } else {
    emberEventControlSetDelayMS(emberAfPluginRf4ceGdpValidationEventControl,
                                APLC_MAX_NORMAL_VALIDATION_DURATION_MS);
  }
#else
  // The binding originator shall start a timer as soon as the validation phase
  // starts. When the timer reaches aplBindingOriginatorValidationWaitTime,
  // the validation is unsuccessful and the binding originator shall not
  // transmit any more check validation request command frames.
  emberEventControlSetDelayMS(emberAfPluginRf4ceGdpValidationEventControl,
                              APL_GDP_BINDING_ORIGINATOR_VALIDATION_WAIT_TIME_DEFAULT);
#endif
}

static bool firstTimerCheck;
static void startBindingProcedureTimer(void)
{
  emberEventControlSetDelayMS(emberAfPluginRf4ceGdpCheckBindingProcedureEventControl,
                              APLC_BIND_WINDOW_DURATION_MS);
  firstTimerCheck = true;
}

static bool checkBindingProcedureTimer(void)
{
  if (emberEventControlGetActive(emberAfPluginRf4ceGdpCheckBindingProcedureEventControl)) {
    return true;
  }

  if (firstTimerCheck) {
    firstTimerCheck = false;
    return true;
  } else {
    return false;
  }
}

static void bindingProcedureComplete(EmberAfRf4ceGdpBindingStatus status,
                                     uint8_t pairingIndex)
{
  emAfRf4ceGdpNotifyBindingCompleteToProfiles(status,
                                              pairingIndex,
                                              pairingCandidatesList[emAfGdpPeerInfo.candidateIndex].supportedProfiles,
                                              pairingCandidatesList[emAfGdpPeerInfo.candidateIndex].supportedProfilesLength);
  emberAfPluginRf4ceGdpBindingCompleteCallback(status, pairingIndex);
}

static void failBindingProcedure(EmberAfRf4ceGdpBindingStatus status)
{
  uint8_t i;
  bool bound = false;

  for(i=0; i<EMBER_RF4CE_PAIRING_TABLE_SIZE; i++) {
    if ((emAfRf4ceGdpGetPairingBindStatus(i) & PAIRING_ENTRY_BINDING_STATUS_MASK)
        == PAIRING_ENTRY_BINDING_STATUS_BOUND_ORIGINATOR) {
      bound = true;
      break;
    }
  }

  // Set the public state to "bound" if there is at least a pairing entry whose
  // status is "bound as originator", otherwise set it to "not bound".
  setPublicState(((bound)
                  ? EMBER_AF_RF4CE_GDP_BINDING_STATE_BOUND
                  : EMBER_AF_RF4CE_GDP_BINDING_STATE_NOT_BOUND),
                 false);

  emberEventControlSetInactive(emberAfPluginRf4ceGdpPendingCommandEventControl);
  emberEventControlSetInactive(emberAfPluginRf4ceGdpBlackoutTimeEventControl);
  emberEventControlSetInactive(emberAfPluginRf4ceGdpValidationEventControl);

  bindingProcedureComplete(status, 0xFF);
}

#define PRIMARY_CLASS   0x00
#define SECONDARY_CLASS 0x01
#define TERTIARY_CLASS  0x02

// Returns true if pc1 ranks higher than pc2, false otherwise. Flag abort is
// set to true if a duplicate was detected and any of the duplicates had the
// duplicate handling field set to 'abort' or 'reserved'. In this case, the
// returning value is meaningless.
static bool comparePairingCandidates(EmAfGdpPairingCanditate *pc1,
                                        EmAfGdpPairingCanditate *pc2,
                                        bool *abort,
                                        bool *lqiTieBreaker)
{
  uint8_t pc1CurrentClassDescriptor = pc1->primaryClassDescriptor;
  uint8_t pc2CurrentClassDescriptor = pc2->primaryClassDescriptor;
  uint8_t pc1ClassInUse = PRIMARY_CLASS;
  uint8_t pc2ClassInUse = PRIMARY_CLASS;
  uint8_t pc1ClassNumber;
  uint8_t pc2ClassNumber;
  uint8_t i;
  bool dummy;

  if (abort == NULL) {
    abort = &dummy;
  }

  if (lqiTieBreaker == NULL) {
    lqiTieBreaker = &dummy;
  }

  *abort = false;
  *lqiTieBreaker = false;

  // First two passes handle aborts and reclassifying primary to secondary and
  // secondary to tertiary. Third pass is to handle aborts.
  for(i=0; i<3; i++) {
    pc1ClassNumber =
        ((pc1CurrentClassDescriptor & CLASS_DESCRIPTOR_NUMBER_MASK)
         >> CLASS_DESCRIPTOR_NUMBER_OFFSET);
    pc2ClassNumber =
        ((pc2CurrentClassDescriptor & CLASS_DESCRIPTOR_NUMBER_MASK)
         >> CLASS_DESCRIPTOR_NUMBER_OFFSET);

    if (pc1ClassNumber != pc2ClassNumber) {
      break;
    } else { // Same class number
      uint8_t pc1DuplicateHandling =
          ((pc1CurrentClassDescriptor & CLASS_DESCRIPTOR_DUPLICATE_HANDLING_MASK)
           >> CLASS_DESCRIPTOR_DUPLICATE_HANDLING_OFFSET);
      uint8_t pc2DuplicateHandling =
          ((pc2CurrentClassDescriptor & CLASS_DESCRIPTOR_DUPLICATE_HANDLING_MASK)
           >> CLASS_DESCRIPTOR_DUPLICATE_HANDLING_OFFSET);

      if (pc1DuplicateHandling == CLASS_DESCRIPTOR_DUPLICATE_HANDLING_ABORT
          || pc2DuplicateHandling == CLASS_DESCRIPTOR_DUPLICATE_HANDLING_ABORT
          || pc1DuplicateHandling == CLASS_DESCRIPTOR_DUPLICATE_HANDLING_RESERVED
          || pc2DuplicateHandling == CLASS_DESCRIPTOR_DUPLICATE_HANDLING_RESERVED) {
        *abort = true;
        return true; // this return value is meaningless here.
      }

      // TODO: we could save some code here by making a function for the code
      // below.
      if (pc1DuplicateHandling
          == CLASS_DESCRIPTOR_DUPLICATE_HANDLING_RECLASSIFY) {
        if (pc1ClassInUse == PRIMARY_CLASS) {
          pc1CurrentClassDescriptor = pc1->secondaryClassDescriptor;
          pc1ClassInUse = SECONDARY_CLASS;
        } else if (pc1ClassInUse == SECONDARY_CLASS) {
          pc1CurrentClassDescriptor = pc1->tertiaryClassDescriptor;
          pc1ClassInUse = TERTIARY_CLASS;
        }
      }
      if (pc2DuplicateHandling
          == CLASS_DESCRIPTOR_DUPLICATE_HANDLING_RECLASSIFY) {
        if (pc2ClassInUse == PRIMARY_CLASS) {
          pc2CurrentClassDescriptor = pc2->secondaryClassDescriptor;
          pc2ClassInUse = SECONDARY_CLASS;
        } else if (pc2ClassInUse == SECONDARY_CLASS) {
          pc2CurrentClassDescriptor = pc2->tertiaryClassDescriptor;
          pc2ClassInUse = TERTIARY_CLASS;
        }
      }
    }
  }

  if (pc1ClassNumber == pc2ClassNumber) { // Same class numbers, use LQI value
    *lqiTieBreaker = true;
    return (pc1->rxLqi > pc2->rxLqi);     // as tie breaker.
  } else { // Different class numbers, lower class number ranks higher.
    return (pc1ClassNumber < pc2ClassNumber);
  }
}

// Returns the highest or lowest ranked candidate index. If during the ranking
// process, at least a duplicate is detected such that one of the two candidates
// have the duplicate handling field set to 'abort' or 'reserved', the abort
// flag is set and the returning index is meaningless.
static uint8_t getLowestOrHighestRankedCandidateIndex(bool highest,
                                                    bool freshOnly,
                                                    bool *abort)
{
  uint8_t i;
  uint8_t lowestOrHighestCandidateIndex = 0xFF;

  *abort = false;

  for(i=0; i<APL_MAX_PAIRING_CANDIDATES;i++) {
    if (isPairingCandidateEntryInUse(i)
        && (!freshOnly
            || !(pairingCandidatesList[i].info
                 & CANDIDATE_INFO_PAIRING_ATTEMPTED_BIT))) {
      if (lowestOrHighestCandidateIndex == 0xFF) {
        lowestOrHighestCandidateIndex = i;
      } else {
        bool currentCandidateRanksHigher =
            comparePairingCandidates(&pairingCandidatesList[i],
                                     &pairingCandidatesList[lowestOrHighestCandidateIndex],
                                     abort,
                                     NULL);
        if (*abort) {
          return 0xFF;
        } else if ((highest && currentCandidateRanksHigher)
                   || (!highest && !currentCandidateRanksHigher)) {
          lowestOrHighestCandidateIndex = i;
        }
      }
    }
  }

  return lowestOrHighestCandidateIndex;
}

//------------------------------------------------------------------------------
// Event handlers.

void emberAfPluginRf4ceGdpCheckBindingProcedureEventHandler(void)
{
  // We don't react to the expired timer. Instead, we actively check whether the
  // check binding procedure timer is expired according to Figure 33 of the
  // GDP 2.0 specs.
  emberEventControlSetInactive(emberAfPluginRf4ceGdpCheckBindingProcedureEventControl);

  // Section 7.2.7.1
  // The Binding Originator shall be capable of performing discovery procedures
  // in the validation state, and shall make sure the time between any two
  // discovery procedures (in the validation state or in the discovery state)
  // shall be no longer than aplcBindWindowDuration.
#if (defined(EMBER_AF_PLUGIN_RF4CE_GDP_EXTENDED_VALIDATION) || defined(EMBER_SCRIPTED_TEST))
  if (internalGdpState() == INTERNAL_STATE_ORIGINATOR_GDP_VALIDATION) {
    // Restart the binding procedure timer so that once the bind window closes
    // again, if we are still validating we kick off another discovery.
    emberEventControlSetDelayMS(emberAfPluginRf4ceGdpCheckBindingProcedureEventControl,
                                APLC_BIND_WINDOW_DURATION_MS);
    emberEventControlSetInactive(emberAfPluginRf4ceGdpPendingCommandEventControl);
    emberEventControlSetInactive(emberAfPluginRf4ceGdpBlackoutTimeEventControl);
    startDiscoveryProcess(NULL, 0, 0);
  }
#endif
}

void emAfPluginRf4ceGdpBlackoutTimeEventHandlerOriginator(void)
{
  switch(internalGdpState()) {
  // Blackout time at the end of the pairing procedure.
  case INTERNAL_STATE_ORIGINATOR_GDP_CONFIG_PUSH_PENDING:
    startGdpConfigurationProcedureOrFail();
    break;
  // Blackout time at the end of the GDP configuration procedure.
  case INTERNAL_STATE_ORIGINATOR_GDP_PROFILES_CONFIG:
    // Start the (next) profile-specific configuration procedure if any,
    // otherwise move to the validation procedure.
    if (!emAfRf4ceGdpMaybeStartNextProfileSpecificConfigurationProcedure(true, // isOriginator
                                                                         pairingCandidatesList[emAfGdpPeerInfo.candidateIndex].supportedProfiles,
                                                                         pairingCandidatesList[emAfGdpPeerInfo.candidateIndex].supportedProfilesLength)) {
      startValidationProcedure();
    }
    break;
  case INTERNAL_STATE_ORIGINATOR_GDP_VALIDATION:
    // Send a checkValidationRequest command and reschedule the event.
    // The control byte is all reserved bits, so we set it to zero.
    emberAfRf4ceGdpCheckValidationRequest(emAfTemporaryPairingIndex,
                                          EMBER_RF4CE_NULL_VENDOR_ID,
                                          0x00);
    emberEventControlSetDelayMS(emberAfPluginRf4ceGdpBlackoutTimeEventControl,
                                emAfRf4ceGdpLocalNodeAttributes.autoCheckValidationPeriod);
    break;
  case INTERNAL_STATE_ORIGINATOR_GDP_KEY_EXCHANGE_BLACKOUT_PENDING:
    emAfRf4ceGdpInitiateKeyExchangeInternal(emAfTemporaryPairingIndex, true);
    break;
  default:
    assert(0);
  }
}

void emAfPluginRf4ceGdpValidationEventHandlerOriginator(void)
{
  assert(internalGdpState() == INTERNAL_STATE_ORIGINATOR_GDP_VALIDATION);

  debugScriptCheck("Validation timed out, fail");

  // When the timer reached aplBindingOriginatorValidationWaitTime, the
  // validation is unsuccessful and the binding originator shall not transmit
  // any more CheckValidationRequest command frames.
  emberEventControlSetInactive(emberAfPluginRf4ceGdpBlackoutTimeEventControl);

  tryRestartDiscoveryProcessOrFail(EMBER_AF_RF4CE_GDP_BINDING_STATUS_VALIDATION_FAILED);
}

void emAfPendingCommandEventHandlerOriginator(void)
{
  switch(internalGdpState()) {
  // We timed out waiting for the response of one of the GDP
  // configuration-related messages.
  case INTERNAL_STATE_ORIGINATOR_GDP_CONFIG_PUSH_PENDING:
  case INTERNAL_STATE_ORIGINATOR_GDP_CONFIG_GET_PENDING:
  case INTERNAL_STATE_ORIGINATOR_GDP_CONFIG_PULL_PENDING:
  case INTERNAL_STATE_ORIGINATOR_GDP_CONFIG_COMPLETE_PENDING:
    tryRestartDiscoveryProcessOrFail(EMBER_AF_RF4CE_GDP_BINDING_STATUS_CONFIG_FAILED);
    break;
  case INTERNAL_STATE_ORIGINATOR_GDP_VALIDATION:
    debugScriptCheck("Link lost, validation failed");
    // If the aplLinkLostWaitTime timeout expires, the binding originator or
    // recipient detects that it has lost its link with the remote node and
    // the validation procedure is unsuccessful.
    tryRestartDiscoveryProcessOrFail(EMBER_AF_RF4CE_GDP_BINDING_STATUS_VALIDATION_FAILED);
    break;
  default:
    assert(0);
  }
}

#endif // EMBER_AF_PLUGIN_RF4CE_GDP_IS_ORIGINATOR
