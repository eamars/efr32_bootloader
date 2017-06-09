// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "rf4ce-mso.h"
#include "rf4ce-mso-internal.h"
#include "rf4ce-mso-attributes.h"

#ifdef EMBER_AF_PLUGIN_RF4CE_MSO_IS_RECIPIENT

#if (MSO_DISCOVERY_RESPONSE_MSO_USER_STRING_LENGTH \
     < EMBER_AF_PLUGIN_RF4CE_MSO_USER_STRING_LENGTH)
  #error The MSO user string is too long.
#endif

// Values of zero for aplValidationWaitTime or aplValidationInitialWatchdogTime
// are interpreted as infinity.
#if EMBER_AF_PLUGIN_RF4CE_MSO_VALIDATION_WAIT_TIME_MS == 0
  #define SET_VALIDATION_TIMEOUT()
#else
  #define SET_VALIDATION_TIMEOUT()                                                 \
    emberEventControlSetDelayMS(emberAfPluginRf4ceMsoValidationEventControl,       \
                                EMBER_AF_PLUGIN_RF4CE_MSO_VALIDATION_WAIT_TIME_MS)
#endif
#if EMBER_AF_PLUGIN_RF4CE_MSO_VALIDATION_INITIAL_WATCHDOG_TIME_MS == 0
  #define SET_INITIAL_WATCHDOG_TIMEOUT()
#else
  #define SET_INITIAL_WATCHDOG_TIMEOUT()                                                       \
    emberEventControlSetDelayMS(emberAfPluginRf4ceMsoWatchdogEventControl,                     \
                                EMBER_AF_PLUGIN_RF4CE_MSO_VALIDATION_INITIAL_WATCHDOG_TIME_MS)
#endif

static bool isValidDiscoveryOrPairRequest(uint8_t nodeCapabilities,
                                             const EmberRf4ceVendorInfo *vendorInfo,
                                             const EmberRf4ceApplicationInfo *appInfo,
                                             uint8_t searchDevType);
static void restoreOrRemove(uint8_t pairingIndex);
static void sync(bool up);
static EmberStatus terminate(EmberAfRf4ceMsoCheckValidationStatus status);
#define isValidated(pairingIndex)                   \
  (EMBER_AF_RF4CE_MSO_VALIDATION_STATE_VALIDATED    \
   == emAfRf4ceMsoGetValidationState(pairingIndex))
#define wasValidated(pairingIndex)                  \
  (EMBER_AF_RF4CE_MSO_VALIDATION_STATE_REVALIDATING \
   <= emAfRf4ceMsoGetValidationState(pairingIndex))
#define hasBackup(pairingIndex) \
  emberAfRf4cePairingTableEntryIsActive(&pairingTable[pairingIndex])
#define forgetBackup(pairingIndex) \
  pairingTable[pairingIndex].info = EMBER_RF4CE_PAIRING_TABLE_ENTRY_STATUS_UNUSED

// The recipient can only bind with one originator at a time.  This is that
// originator.
static uint8_t originatorPairingIndex = NULL_PAIRING_INDEX;

// If binding fails, we need to restore the previous pairing, if one existed.
// We know whether a previous pairing existed by the status in PairRequest.
// However, by the time PairRequest is called, the existing pairing has already
// been wiped out and replaced by the new temporary pairing.  Because of that,
// we need to copy the pairings before that happens.
static EmberRf4cePairingTableEntry pairingTable[EMBER_RF4CE_PAIRING_TABLE_SIZE];

EmberEventControl emberAfPluginRf4ceMsoBlackoutEventControl;
EmberEventControl emberAfPluginRf4ceMsoValidationEventControl;
EmberEventControl emberAfPluginRf4ceMsoWatchdogEventControl;
EmberEventControl emberAfPluginRf4ceMsoRestoreEventControl;

// DONE: On receipt of a discovery indication notification from the NLME
// through the NLME-DISCOVERY.indication primitive, the profile of the target
// SHALL reply with a successful discovery response if all the received
// discovery indications meet the following conditions:
// * The Vendor identifier matches the Vendor ID of the target device (see list
//   of Vendor IDs in [RF4CE CODE]
// * The RF4CE MSO Profile (0xc0) is specified in [RF4CE ID].
// * The Requested Device Type matches device type of target device or is set
//   to 0xFF.

// DONE: If one of these conditions is not met, the target profile SHALL ignore
// the discovery indication.

// DONE: After a transmission of a successful discovery response to the NLME,
// the target profile SHALL wait for a corresponding pairing request
// notification from the NLME through the NLME-PAIR.indication primitive.
// - DONE: How long should the target wait?  The target waits forever.
// - DONE: What happens if a new discovery request is received while waiting?
//         The new discovery request is processed.

// DONE: The profile SHALL respond to the pair request, and SHALL notify the
// network layer by issuing the NLME-PAIR.response primitive to the NLME.

// DONE: If the transmission of the subsequent pair response command and
// subsequent link key exchange -- if required -- was successful, indicated
// through the NLME-COMM-STATUS.indication primitive from the NLME, the profile
// SHALL be considered temporary paired to the device with the indicated
// reference into the pairing table.

// DONE: After a successful temporary pairing, the target profile SHALL enter
// the validation procedure specified in Section 6.5.6.

// DONE: The validation procedure starts with an aplcBlackOutTime interval
// during which both controller and target SHALL NOT transmit any packets,
// allowing the implementation to complete the temporary pairing.
// - DONE: Can the target transit to other controllers?  Yes, the target is
//         only required to be blacked out with the controller it is
//         validating.

// DONE: Upon receipt of a check validation request command frame, the target
// SHALL check the result of the validation.

// DONE: After the aplcResponseIdleTime interval, the target SHALL generate and
// transmit a check validation response command frame to the controller device,
// containing the result of the validation, as specified in Table 7.

// DONE: If the first validation watchdog kick is not performed on the target
// in the aplInitialValidationWatchdogTime interval (starting at the beginning
// of the validation procedure), the validation SHALL fail (Time Out).

// DONE: After each validation watchdog kick, the watchdog time out interval
// SHALL be restarted with the value specified in the validation watchdog kick.

// DONE: If the validation step is not performed on the target in the
// aplValidationWaitTime interval (starting at the beginning of the validation
// procedure), the validation SHALL fail (Time Out).

// TODO: A target MAY also terminate the validation procedure on its own behalf
// (for example if a user control frame is received from a previously bound
// controller).

// DONE: If the target terminates the validation phase, the validation SHALL
// fail (Failure).

// DONE: The target SHALL interpret the key as a (full) abort request, and the
// validation SHALL be unsuccessful.  This SHALL be communicated back to the
// controller via the check validation request/responses.  The ValidationStatus
// field MAY be set to either:
// * 'Abort', to abort the validation with this target only.  In this case, the
//   validation is unsuccessful.
// * 'Full Abort', to abort the complete binding procedure.  In this case, the
//   validation is unsuccessful.
// - Are the key codes for the AbortValidation keys vendor specific?

// DONE: If the validation is unsuccessful ('Failure', 'Time Out', 'Collision',
// 'Abort', 'Full Abort'), the target SHALL remove the pairing entry of the
// controller from its pairing table only and only if the controller was not
// validated before the binding procedure started.

// DONE: If the validation is 'Successful', the binding SHALL be considered
// permanent and the target is bound with the controller.

// DONE: If the target receives a new discovery or pair request with the
// conditions specified in requirement MP-50-220 during the validation
// procedure, the validation SHALL be terminated.
// - DONE: Does the target process the new request?  No, the request is
//         ignored.

// DONE: The target SHALL remove the pairing entry of the controller from its
// pairing table only and only if the controller was not validated before the
// binding procedure started.

// TODO: A garbage collection algorithm SHALL be used to avoid the condition of
// the pairing table of the target being filled with old controllers,
// preventing new controllers from binding.  The exact implementation of the
// garbage collection algorithm SHALL be implementation-specific.

void emberAfPluginRf4ceMsoInitCallback(void)
{
  emAfRf4ceMsoInitializeValidationStates();
  emAfRf4ceMsoInitCommands();
  emAfRf4ceMsoInitAttributes();
}

void emberAfPluginRf4ceMsoStackStatusCallback(EmberStatus status)
{
  if (emberAfRf4ceIsCurrentNetwork()) {
    sync(status == EMBER_NETWORK_UP);
  }
}

EmberStatus emberAfRf4ceMsoBind(void)
{
  // MSO recipients cannot also be originators, so we do not initiate binding.
  return EMBER_INVALID_CALL;
}

EmberStatus emberAfRf4ceMsoWatchdogKick(uint16_t validationWatchdogTimeMs)
{
  if (originatorPairingIndex == NULL_PAIRING_INDEX) {
    return EMBER_INVALID_CALL;
  } else if (validationWatchdogTimeMs == 0) {
    emberEventControlSetInactive(emberAfPluginRf4ceMsoWatchdogEventControl);
  } else {
    emberEventControlSetDelayMS(emberAfPluginRf4ceMsoWatchdogEventControl,
                                validationWatchdogTimeMs);
  }
  return EMBER_SUCCESS;
}

EmberStatus emberAfRf4ceMsoValidate(void)
{
  return terminate(EMBER_AF_RF4CE_MSO_CHECK_VALIDATION_STATUS_SUCCESS);
}

EmberStatus emberAfRf4ceMsoTerminateValidation(void)
{
  return terminate(EMBER_AF_RF4CE_MSO_CHECK_VALIDATION_STATUS_FAILURE);
}

EmberStatus emberAfRf4ceMsoAbortValidation(bool fullAbort)
{
  return terminate(fullAbort
                   ? EMBER_AF_RF4CE_MSO_CHECK_VALIDATION_STATUS_FULL_ABORT
                   : EMBER_AF_RF4CE_MSO_CHECK_VALIDATION_STATUS_ABORT);
}

bool emberAfPluginRf4ceProfileMsoDiscoveryRequestCallback(const EmberEUI64 ieeeAddr,
                                                             uint8_t nodeCapabilities,
                                                             const EmberRf4ceVendorInfo *vendorInfo,
                                                             const EmberRf4ceApplicationInfo *appInfo,
                                                             uint8_t searchDevType,
                                                             uint8_t rxLinkQuality)
{
  // Discovery requests are only responded to if they are valid and there is no
  // collision with another validation procedure.  If there is a collision, the
  // validation procedure is killed and this request is ignored.
  if (isValidDiscoveryOrPairRequest(nodeCapabilities,
                                    vendorInfo,
                                    appInfo,
                                    searchDevType)) {
    if (originatorPairingIndex == NULL_PAIRING_INDEX) {
      sync(true); // network is up
      return (emAfRf4ceMsoSetDiscoveryResponseUserString() == EMBER_SUCCESS);
    } else {
      // When an in-progress validation is terminated, the pairing table entry
      // and validation status of the originator is immediately restored to its
      // state before pairing and validating began.  This is safe to do here
      // because the stack will not fiddle with that pairing when we return
      // from this callback.
      terminate(EMBER_AF_RF4CE_MSO_CHECK_VALIDATION_STATUS_COLLISION);
    }
  }
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
  // MSO recipients cannot also be originators, so we do not send discovery
  // requests and therefore do not expect discovery responses.
  return false;
}

void emberAfPluginRf4ceProfileMsoDiscoveryCompleteCallback(EmberStatus status)
{
  // MSO recipients cannot also be originators, so we do not send discovery
  // requests and therefore do not expect discovery complete notifications.
}

void emberAfPluginRf4ceProfileMsoAutoDiscoveryResponseCompleteCallback(EmberStatus status,
                                                                       const EmberEUI64 srcIeeeAddr,
                                                                       uint8_t nodeCapabilities,
                                                                       const EmberRf4ceVendorInfo *vendorInfo,
                                                                       const EmberRf4ceApplicationInfo *appInfo,
                                                                       uint8_t searchDevType)
{
  // MSO does not use auto discovery, so we do not expect auto discovery
  // complete notifications.
}

bool emberAfPluginRf4ceProfileMsoPairRequestCallback(EmberStatus status,
                                                        uint8_t pairingIndex,
                                                        const EmberEUI64 sourceIeeeAddr,
                                                        uint8_t nodeCapabilities,
                                                        const EmberRf4ceVendorInfo *vendorInfo,
                                                        const EmberRf4ceApplicationInfo *appInfo,
                                                        uint8_t keyExchangeTransferCount)
{
  // Pair requests are only responded to if they are valid and there is no
  // collision with another validation procedure.  If there is a collision, the
  // validation procedure is killed and this request is ignored.
  if (isValidDiscoveryOrPairRequest(nodeCapabilities,
                                    vendorInfo,
                                    appInfo,
                                    EMBER_AF_RF4CE_DEVICE_TYPE_WILDCARD)) {
    if (originatorPairingIndex == NULL_PAIRING_INDEX) {
      if ((status == EMBER_SUCCESS || status == EMBER_DUPLICATE_ENTRY)
          && APLC_MIN_KEY_EXCHANGE_TRANSFER_COUNT <= keyExchangeTransferCount) {
        originatorPairingIndex = pairingIndex;
        return true;
      }
    } else if (originatorPairingIndex != pairingIndex) {
      // When an in-progress validation is terminated, the pairing table entry
      // and validation status of the originator is immediately restored to its
      // state before pairing and validating began.  This is safe to do here
      // because the stack will not fiddle with that pairing when we return
      // from this callback.  This is not done if the collider and the collidee
      // are the same because restoring won't work in that case.  See the note
      // below.
      terminate(EMBER_AF_RF4CE_MSO_CHECK_VALIDATION_STATUS_COLLISION);
    }
  }

  // When we receive a pair request, the stack creates a new provisional
  // pairing table entry for the node, overwriting the existing one in the case
  // of a duplicate.  If we reject the pair request, the stack will
  // automatically remove that pairing entry.  That means a previously-paired
  // node will not be paired after we return false.  In that case, we need to
  // restore the previous entry.  However, we cannot do that here because the
  // stack will remove the pairing after we return, so any restoration we do
  // here would be in vain.  Instead, we schedule an event to do it later.
  if (status == EMBER_DUPLICATE_ENTRY
      && wasValidated(pairingIndex)
      && hasBackup(pairingIndex)) {
    originatorPairingIndex = pairingIndex;
    emberEventControlSetActive(emberAfPluginRf4ceMsoRestoreEventControl);
  }

  return false;
}

void emberAfPluginRf4ceProfileMsoPairCompleteCallback(EmberStatus status,
                                                      uint8_t pairingIndex,
                                                      const EmberRf4ceVendorInfo *vendorInfo,
                                                      const EmberRf4ceApplicationInfo *appInfo)
{
  if (originatorPairingIndex != NULL_PAIRING_INDEX) {
    if (status == EMBER_SUCCESS || status == EMBER_DUPLICATE_ENTRY) {
      emAfRf4ceMsoSetValidation(originatorPairingIndex,
                                (isValidated(originatorPairingIndex)
                                 ? EMBER_AF_RF4CE_MSO_VALIDATION_STATE_REVALIDATING
                                 : EMBER_AF_RF4CE_MSO_VALIDATION_STATE_VALIDATING),
                                EMBER_AF_RF4CE_MSO_CHECK_VALIDATION_STATUS_PENDING);
      emberEventControlSetDelayMS(emberAfPluginRf4ceMsoBlackoutEventControl,
                                  APLC_BLACK_OUT_TIME_MS);
      SET_VALIDATION_TIMEOUT();
      SET_INITIAL_WATCHDOG_TIMEOUT();
      // Turn the radio on for incoming check validation requests.
      emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_MSO, true);
      emberAfPluginRf4ceMsoStartValidationCallback(originatorPairingIndex);
    } else {
      // If the pairing failed, the stack will have already removed the
      // temporary pairing by this point.  However, if we had an active and
      // validated binding before this one failed, we need to restore the old
      // pairing.  This is safe to do here because the stack will not fiddle
      // with the pairing when we return from this callback.
      restoreOrRemove(originatorPairingIndex);

      // If pairing failed, binding is complete, whether we restored or not.
      originatorPairingIndex = NULL_PAIRING_INDEX;
    }
  }
}

void emberAfPluginRf4ceMsoBlackoutEventHandler(void)
{
  emberEventControlSetInactive(emberAfPluginRf4ceMsoBlackoutEventControl);
}

void emberAfPluginRf4ceMsoValidationEventHandler(void)
{
  terminate(EMBER_AF_RF4CE_MSO_CHECK_VALIDATION_STATUS_TIMEOUT);
}

void emberAfPluginRf4ceMsoWatchdogEventHandler(void)
{
  terminate(EMBER_AF_RF4CE_MSO_CHECK_VALIDATION_STATUS_TIMEOUT);
}

void emberAfPluginRf4ceMsoRestoreEventHandler(void)
{
  // This event fires when we are restoring a specific pairing entry.
  if (originatorPairingIndex != NULL_PAIRING_INDEX) {
    restoreOrRemove(originatorPairingIndex);
    originatorPairingIndex = NULL_PAIRING_INDEX;
  }
  emberEventControlSetInactive(emberAfPluginRf4ceMsoRestoreEventControl);
}

bool emAfRf4ceMsoIsBlackedOut(uint8_t pairingIndex)
{
  return (emberEventControlGetActive(emberAfPluginRf4ceMsoBlackoutEventControl)
          && originatorPairingIndex == pairingIndex);
}

static bool isValidDiscoveryOrPairRequest(uint8_t nodeCapabilities,
                                             const EmberRf4ceVendorInfo *vendorInfo,
                                             const EmberRf4ceApplicationInfo *appInfo,
                                             uint8_t searchDevType)
{
  // MP-50-220: Discovery and pair requests must be from a controller with a
  // matching vendor id that implements the MSO profile and the device type
  // searched for must be the wildcard device type or a device type implemented
  // by the target.
  return (!(nodeCapabilities & EMBER_RF4CE_NODE_CAPABILITIES_IS_TARGET_BIT)
          && emberAfRf4ceVendorId() == vendorInfo->vendorId
          && emberAfRf4ceIsProfileSupported(appInfo, EMBER_AF_RF4CE_PROFILE_MSO)
          && emberAfRf4ceIsDeviceTypeSupportedLocally(searchDevType));
}

static void restoreOrRemove(uint8_t pairingIndex)
{
  if (wasValidated(pairingIndex)
      && hasBackup(pairingIndex)
      && (emberAfRf4ceSetPairingTableEntry(pairingIndex,
                                           &pairingTable[pairingIndex])
          == EMBER_SUCCESS)) {
    emAfRf4ceMsoSetValidation(originatorPairingIndex,
                              EMBER_AF_RF4CE_MSO_VALIDATION_STATE_VALIDATED,
                              EMBER_AF_RF4CE_MSO_CHECK_VALIDATION_STATUS_SUCCESS);
  } else {
    forgetBackup(pairingIndex);
    emAfRf4ceMsoSetValidation(pairingIndex,
                              EMBER_AF_RF4CE_MSO_VALIDATION_STATE_NOT_VALIDATED,
                              EMBER_AF_RF4CE_MSO_CHECK_VALIDATION_STATUS_FAILURE);
    emberAfRf4ceSetPairingTableEntry(pairingIndex, NULL);
  }
}

static void sync(bool up)
{
  // For each pairing, forget the current backup and then try to get a new one.
  // If we can't get a backup, if the entry is not active, if the originator is
  // not validated, or the network isn't up, we set that originator to
  // unvalidated.
  uint8_t i;
  for (i = 0; i < EMBER_RF4CE_PAIRING_TABLE_SIZE; i++) {
    forgetBackup(i);
    if (emberAfRf4ceGetPairingTableEntry(i, &pairingTable[i]) != EMBER_SUCCESS
        || !hasBackup(i)
        || !isValidated(i)
        || !up) {
      emAfRf4ceMsoSetValidation(i,
                                EMBER_AF_RF4CE_MSO_VALIDATION_STATE_NOT_VALIDATED,
                                EMBER_AF_RF4CE_MSO_CHECK_VALIDATION_STATUS_FAILURE);
    }
  }
}

static EmberStatus terminate(EmberAfRf4ceMsoCheckValidationStatus status)
{
  assert(status != EMBER_AF_RF4CE_MSO_CHECK_VALIDATION_STATUS_PENDING);
  emberEventControlSetInactive(emberAfPluginRf4ceMsoBlackoutEventControl);
  emberEventControlSetInactive(emberAfPluginRf4ceMsoValidationEventControl);
  emberEventControlSetInactive(emberAfPluginRf4ceMsoWatchdogEventControl);

  // Binding procedure completed, turn the radio back off.
  emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_MSO, false);

  if (originatorPairingIndex != NULL_PAIRING_INDEX) {
    if (status == EMBER_AF_RF4CE_MSO_CHECK_VALIDATION_STATUS_SUCCESS) {
      emAfRf4ceMsoSetValidation(originatorPairingIndex,
                                EMBER_AF_RF4CE_MSO_VALIDATION_STATE_VALIDATED,
                                EMBER_AF_RF4CE_MSO_CHECK_VALIDATION_STATUS_SUCCESS);
    } else {
      restoreOrRemove(originatorPairingIndex);
    }
    emberAfPluginRf4ceMsoBindingCompleteCallback(status,
                                                 originatorPairingIndex);
    originatorPairingIndex = NULL_PAIRING_INDEX;
    return EMBER_SUCCESS;
  }
  return EMBER_INVALID_CALL;
}

#endif // EMBER_AF_PLUGIN_RF4CE_MSO_IS_RECIPIENT
