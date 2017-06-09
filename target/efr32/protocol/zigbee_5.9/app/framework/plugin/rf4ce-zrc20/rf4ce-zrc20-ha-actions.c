// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"

#if defined(EMBER_SCRIPTED_TEST)
#include "rf4ce-zrc20-test.h"
#endif // EMBER_SCRIPTED_TEST

#include "app/framework/plugin/rf4ce-profile/rf4ce-profile.h"
#include "app/framework/plugin/rf4ce-gdp/rf4ce-gdp.h"
#include "app/framework/plugin/rf4ce-gdp/rf4ce-gdp-internal.h"
#include "rf4ce-zrc20.h"
#include "rf4ce-zrc20-internal.h"
#include "rf4ce-zrc20-attributes.h"
#include "rf4ce-zrc20-ha-actions.h"

//------------------------------------------------------------------------------
// Forward declarations.

void emAfZrcHomeAutomationActionsEventHandlerClient(void);

//------------------------------------------------------------------------------
// Common code (events and dispatchers)

EmberEventControl emberAfPluginRf4ceZrc20HomeAutomationActionsEventControl;

void emberAfPluginRf4ceZrc20HomeAutomationActionsEventHandler(void)
{
  emberEventControlSetInactive(emberAfPluginRf4ceZrc20HomeAutomationActionsEventControl);

#if defined(EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_ORIGINATOR) || defined(EMBER_SCRIPTED_TEST)
  emAfZrcHomeAutomationActionsEventHandlerClient();
#endif // EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_ORIGINATOR

  // The recipient doesn't need any event.
}

void emAfRf4ceZrcHAActionsBindingCompleteCallback(uint8_t pairingIndex)
{
#if defined(EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_ORIGINATOR) || defined(EMBER_SCRIPTED_TEST)
  emberAfRf4ceZrc20StartHomeAutomationSupportedAnnouncement(pairingIndex);
#endif // EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_ORIGINATOR

  // Nothing to be kicked off at the recipient. We just serve incoming
  // attributes commands as they come in.
}

//------------------------------------------------------------------------------
// HA Actions originator

#if defined(EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_ORIGINATOR) || defined(EMBER_SCRIPTED_TEST)

static uint8_t announcementOrPullRequestPairingIndex;
static uint8_t haSupportedInstanceIndex;
static uint8_t haAttributeIdPull;
static uint8_t pullRequestDirtyFlags[ZRC_BITMASK_SIZE];

static bool isApiCallValid(uint8_t pairingIndex);
static void finishAnnouncement(EmberStatus status);
static bool pushHomeAutomationSupportedToServer(void);
static void continueServingHomeAutomationPullRequest(void);

EmberStatus emberAfRf4ceZrc20StartHomeAutomationSupportedAnnouncement(uint8_t pairingIndex)
{
  // - Check that the pairing index is valid
  // - Check that the pairing is in a 'bound' status
  // - Check that the peer node is an HA recipient
  // - Check that we are not already performing an HA announcement procedure
  // - or pulling an HA attribute.
  if (!isApiCallValid(pairingIndex)
      || emberEventControlGetActive(emberAfPluginRf4ceZrc20HomeAutomationActionsEventControl)) {
    return EMBER_INVALID_CALL;
  }

  emAfRf4ceZrcStartHomeAutomationAnnouncementClient(pairingIndex);
  return EMBER_SUCCESS;
}

EmberStatus emberAfRf4ceZrc20PullHomeAutomationAttribute(uint8_t pairingIndex,
                                                         uint16_t vendorId,
                                                         uint8_t haInstanceId,
                                                         uint8_t haAttributeId)
{
  EmberStatus status;
  EmberAfRf4ceGdpAttributeIdentificationRecord recordId;
  // - Check that the pairing index is valid
  // - Check that the pairing is in a 'bound' status
  // - Check that the peer node is an HA recipient
  // - Check that we are not already performing an HA announcement procedure
  // - or pulling an HA attribute.
  if (!isApiCallValid(pairingIndex)) {
    return EMBER_INVALID_CALL;
  }

  recordId.attributeId = EMBER_AF_RF4CE_ZRC_ATTRIBUTE_HOME_AUTOMATION;
  recordId.entryId = HIGH_LOW_TO_INT(haAttributeId, haInstanceId);

  haAttributeIdPull = haAttributeId;
  haSupportedInstanceIndex = haInstanceId;

  status = emberAfRf4ceGdpPullAttributes(pairingIndex,
                                         EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0,
                                         vendorId,
                                         &recordId,
                                         1);
  if (status == EMBER_SUCCESS) {
    emAfZrcSetState(ZRC_STATE_HA_ORIGINATOR_PULLING_HA_ATTRIBUTE_FROM_RECIPIENT);

    // Turn the radio on and wait for a pull response.
    emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0, true);
    emberEventControlSetDelayMS(emberAfPluginRf4ceZrc20HomeAutomationActionsEventControl,
                                APLC_MAX_RESPONSE_WAIT_TIME_MS);
  }

  return status;
}

void emAfZrcHomeAutomationActionsEventHandlerClient(void)
{
  switch(emAfZrcState) {
  case ZRC_STATE_HA_ORIGINATOR_PUSHING_HA_SUPPORTED_TO_RECIPIENT:
    // timed-out waiting for a GenericResponse()
    finishAnnouncement(EMBER_NO_RESPONSE);
    break;
  case ZRC_STATE_HA_ORIGINATOR_PULLING_HA_ATTRIBUTE_FROM_RECIPIENT:
    // timed-out waiting for a PullAttributesResponse(), turn the radio back
    // off.
    emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0, false);
    emAfZrcSetState(ZRC_STATE_INITIAL);
    emberAfPluginRf4ceZrc20PullHomeAutomationAttributeCompleteCallback(EMBER_AF_RF4CE_GDP_ATTRIBUTE_STATUS_NO_RESPONSE,
                                                                       NULL);
    break;
  case ZRC_STATE_HA_ORIGINATOR_PULLING_HA_ATTRIBUTES_ON_REQUEST_FROM_RECIPIENT:
    // timed-out waiting for a PullAttributesResponse(), turn the radio back
    // off.
    emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0, false);
    emAfZrcSetState(ZRC_STATE_INITIAL);
    debugScriptCheck("Pulling HA attributes on request failed");
   break;
  case ZRC_STATE_INITIAL:
    if (!emAfRf4ceGdpIsBusy()) {
      if (!pushHomeAutomationSupportedToServer()) {
        finishAnnouncement(EMBER_ERR_FATAL);
      }
      break;
    }
    // Fall-through to default
  default:
    // GDP is busy or ZRC state machine is not in its initial state: reschedule
    // the event and try later.
    emberEventControlSetDelayMS(emberAfPluginRf4ceZrc20HomeAutomationActionsEventControl,
                                HA_ACTIONS_ANNOUNCEMENT_PROCEDURE_DELAY_MSEC);
  }
}

void emAfRf4ceZrcStartHomeAutomationAnnouncementClient(uint8_t pairingIndex)
{
#if defined(EMBER_SCRIPTED_TEST)
  if (!(emAfRf4ceZrcGetLocalNodeCapabilities()
        & EMBER_AF_RF4CE_ZRC_CAPABILITY_SUPPORT_HA_ACTIONS_ORIGINATOR)) {
    return;
  }
#endif // EMBER_SCRIPTED_TEST

  debugScriptCheck("Starting HA Actions Announcement (originator)");

  // Turn the receiver on.
  emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0, true);

  haSupportedInstanceIndex = 0;
  announcementOrPullRequestPairingIndex = pairingIndex;
  emberEventControlSetActive(emberAfPluginRf4ceZrc20HomeAutomationActionsEventControl);
}

void emAfRf4ceZrcIncomingGenericResponseHomeAutomationActionsCallbackOriginator(EmberAfRf4ceGdpResponseCode responseCode)
{
  switch(emAfZrcState) {
  case ZRC_STATE_HA_ORIGINATOR_PUSHING_HA_SUPPORTED_TO_RECIPIENT:
    if (responseCode != EMBER_AF_RF4CE_GDP_RESPONSE_CODE_SUCCESSFUL) {
      finishAnnouncement(EMBER_ERR_FATAL);
    } else if (!pushHomeAutomationSupportedToServer()) {
      // Done!
      finishAnnouncement(EMBER_SUCCESS);
    }
    break;
  }
}

bool emAfRf4ceZrcIncomingPullAttributesResponseHomeAutomationOriginatorCallback(void)
{
  EmberAfRf4ceGdpAttributeStatusRecord record;

  if ((emAfZrcState == ZRC_STATE_HA_ORIGINATOR_PULLING_HA_ATTRIBUTE_FROM_RECIPIENT
       || emAfZrcState == ZRC_STATE_HA_ORIGINATOR_PULLING_HA_ATTRIBUTES_ON_REQUEST_FROM_RECIPIENT)
      && emAfRf4ceGdpFetchAttributeStatusRecord(&record)
      && record.attributeId == EMBER_AF_RF4CE_ZRC_ATTRIBUTE_HOME_AUTOMATION) {
    uint8_t savedZrcState = emAfZrcState;
    EmberAfRf4ceZrcHomeAutomationAttribute haAttribute;
    bool attributeContentIsValid = false;

    // Got the expected pull response, turn the radio back off.
    emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0, false);

    emberEventControlSetInactive(emberAfPluginRf4ceZrc20HomeAutomationActionsEventControl);
    emAfZrcSetState(ZRC_STATE_INITIAL);
    if (record.status == EMBER_AF_RF4CE_GDP_ATTRIBUTE_STATUS_SUCCESS) {
      if (record.valueLength > 1
          && (record.value[HA_ATTRIBUTE_STATUS_OFFSET]
              & HA_ATTRIBUTE_STATUS_VALUE_AVAILABLE_FLAG)
          && LOW_BYTE(record.entryId) == haSupportedInstanceIndex
          && HIGH_BYTE(record.entryId) == haAttributeIdPull) {
      attributeContentIsValid = true;
      haAttribute.contentsLength = record.valueLength - 1;
      haAttribute.contents = (uint8_t*)(record.value + 1);
      haAttribute.instanceId = LOW_BYTE(record.entryId);
      haAttribute.attributeId = HIGH_BYTE(record.entryId);
      } else if (record.valueLength > 0
                 && !(record.value[HA_ATTRIBUTE_STATUS_OFFSET]
                      & HA_ATTRIBUTE_STATUS_VALUE_AVAILABLE_FLAG)) {
        record.status = EMBER_AF_RF4CE_GDP_ATTRIBUTE_STATUS_UNSUPPORTED_ATTRIBUTE;
      } else {
        record.status = EMBER_AF_RF4CE_GDP_ATTRIBUTE_STATUS_INVALID_RESPONSE;
      }
    }

    // If we are pulling as result of an API call, we always call the complete
    // callback. If we are pulling as result of a client notification request,
    // we call the complete callback only if the attribute content is valid.
    if (savedZrcState == ZRC_STATE_HA_ORIGINATOR_PULLING_HA_ATTRIBUTE_FROM_RECIPIENT
        || attributeContentIsValid) {
      emberAfPluginRf4ceZrc20PullHomeAutomationAttributeCompleteCallback(record.status,
                                                                         (attributeContentIsValid
                                                                          ? &haAttribute
                                                                          : NULL));
    }

    if (savedZrcState == ZRC_STATE_HA_ORIGINATOR_PULLING_HA_ATTRIBUTES_ON_REQUEST_FROM_RECIPIENT
        && attributeContentIsValid) {
      emAfRf4ceZrcClearActionBank(pullRequestDirtyFlags, haAttribute.attributeId);
      continueServingHomeAutomationPullRequest();
    }

#if defined(EMBER_SCRIPTED_TEST)
    if (savedZrcState == ZRC_STATE_HA_ORIGINATOR_PULLING_HA_ATTRIBUTES_ON_REQUEST_FROM_RECIPIENT
        && !attributeContentIsValid) {
      debugScriptCheck("Pulling HA attributes on request failed");
    }
#endif // EMBER_SCRIPTED_TEST
  }

  // This is handled here, so never let the attribute code process this.
  return false;
}

void emAfRf4ceZrcIncomingRequestHomeAutomationPull(uint8_t haInstanceId,
                                                   const uint8_t *haAttributeDirtyFlags)
{
  uint8_t pairingIndex = emberAfRf4ceGetPairingIndex();

  // If we are doing something else, just ignore this request.
  if (isApiCallValid(pairingIndex)) {
    announcementOrPullRequestPairingIndex = pairingIndex;
    haSupportedInstanceIndex = haInstanceId;
    MEMCOPY(pullRequestDirtyFlags, haAttributeDirtyFlags, ZRC_BITMASK_SIZE);
    continueServingHomeAutomationPullRequest();
  }
}

static bool isApiCallValid(uint8_t pairingIndex)
{
  return (pairingIndex < EMBER_RF4CE_PAIRING_TABLE_SIZE
          && ((emAfRf4ceGdpGetPairingBindStatus(pairingIndex)
               & PAIRING_ENTRY_BINDING_STATUS_MASK)
              != PAIRING_ENTRY_BINDING_STATUS_NOT_BOUND)
          && (emAfRf4ceZrcGetRemoteNodeCapabilities(pairingIndex)
              & EMBER_AF_RF4CE_ZRC_CAPABILITY_SUPPORT_HA_ACTIONS_RECIPIENT)
          && !emberEventControlGetActive(emberAfPluginRf4ceZrc20HomeAutomationActionsEventControl));
}

static void finishAnnouncement(EmberStatus status)
{
  // Announcement procedure completed, turn the radio back off.
  emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0, false);

  emberEventControlSetInactive(emberAfPluginRf4ceZrc20HomeAutomationActionsEventControl);
  emAfZrcSetState(ZRC_STATE_INITIAL);

  emberAfPluginRf4ceZrc20HomeAutomationSupportedAnnouncementCompleteCallback(status);
}

// Returns true if a Home Automation Supported attribute was pushed, false
// otherwise.
static bool pushHomeAutomationSupportedToServer(void)
{
  EmberAfRf4ceZrcHomeAutomationSupported haSupported;
  EmberAfRf4ceGdpAttributeRecord record;

  while(haSupportedInstanceIndex <= HA_SUPPORTED_MAX_INSTANCE_ID_INDEX
        && emberAfPluginRf4ceZrc20GetHomeAutomationSupportedCallback(announcementOrPullRequestPairingIndex,
                                                                     haSupportedInstanceIndex++,
                                                                     &haSupported)
           == EMBER_SUCCESS) {
    record.attributeId = EMBER_AF_RF4CE_ZRC_ATTRIBUTE_HOME_AUTOMATION_SUPPORTED;
    record.entryId = haSupportedInstanceIndex - 1;
    record.valueLength = ZRC_BITMASK_SIZE;
    record.value = (uint8_t*)haSupported.contents;

    emberAfRf4ceGdpPushAttributes(announcementOrPullRequestPairingIndex,
                                  EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0,
                                  EMBER_RF4CE_NULL_VENDOR_ID, // TODO: vendor ID?
                                  &record,
                                  1);

    emAfZrcSetState(ZRC_STATE_HA_ORIGINATOR_PUSHING_HA_SUPPORTED_TO_RECIPIENT);

    // Turn the radio ON and wait for generic response.
    emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0, true);
    emberEventControlSetDelayMS(emberAfPluginRf4ceZrc20HomeAutomationActionsEventControl,
                                APLC_MAX_RESPONSE_WAIT_TIME_MS);

    return true;
  }

  return false;
}

static void continueServingHomeAutomationPullRequest(void)
{
  uint16_t i;
  for (i = 0; i <= MAX_INT8U_VALUE; i++) {
    if (emAfRf4ceZrcReadActionBank(pullRequestDirtyFlags, (uint8_t)i)) {
      emberAfRf4ceZrc20PullHomeAutomationAttribute(announcementOrPullRequestPairingIndex,
                                                   EMBER_RF4CE_NULL_VENDOR_ID, // TODO: vendor ID?
                                                   haSupportedInstanceIndex,
                                                   (uint8_t)i);
      // Override the status set by the public API.
      emAfZrcSetState(ZRC_STATE_HA_ORIGINATOR_PULLING_HA_ATTRIBUTES_ON_REQUEST_FROM_RECIPIENT);
      break;
    }
  }
}

#else

EmberStatus emberAfRf4ceZrc20StartHomeAutomationSupportedAnnouncement(uint8_t pairingIndex)
{
  return EMBER_INVALID_CALL;
}

EmberStatus emberAfRf4ceZrc20PullHomeAutomationAttribute(uint8_t pairingIndex,
                                                         uint16_t vendorId,
                                                         uint8_t haInstanceId,
                                                         uint8_t haAttributeId)
{
  return EMBER_INVALID_CALL;
}

#endif //EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_ORIGINATOR

//------------------------------------------------------------------------------
// HA Actions recipient

#if defined(EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_RECIPIENT) || defined(EMBER_SCRIPTED_TEST)

static bool acceptIncomingAttributeCommand(uint8_t pairingIndex);

bool emAfRf4ceZrcIncomingPushAttributesHomeAutomationRecipientCallback(void)
{
  EmberAfRf4ceGdpAttributeRecord record;
  uint8_t pairingIndex = emberAfRf4ceGetPairingIndex();

  if (acceptIncomingAttributeCommand(pairingIndex)
      && emAfRf4ceGdpFetchAttributeRecord(&record)
      && record.attributeId == EMBER_AF_RF4CE_ZRC_ATTRIBUTE_HOME_AUTOMATION_SUPPORTED
      && record.valueLength == ZRC_BITMASK_SIZE
      && !emAfRf4ceGdpFetchAttributeRecord(&record)) { // Only record
    return true;
  }

  return false;
}

bool emAfRf4ceZrcIncomingPullAttributesHomeAutomationRecipientCallback(void)
{
  EmberAfRf4ceGdpAttributeIdentificationRecord record;
  uint8_t pairingIndex = emberAfRf4ceGetPairingIndex();

  if (acceptIncomingAttributeCommand(pairingIndex)
      && emAfRf4ceGdpFetchAttributeIdentificationRecord(&record)
      && record.attributeId == EMBER_AF_RF4CE_ZRC_ATTRIBUTE_HOME_AUTOMATION
      // Only record
      && !emAfRf4ceGdpFetchAttributeIdentificationRecord(&record)) {
    return true;
  }

  return false;
}

// We accept an attribute command if the pairing is in 'bound' status and if
// the peer node is an HA Actions originator.
static bool acceptIncomingAttributeCommand(uint8_t pairingIndex)
{
  return (pairingIndex < EMBER_RF4CE_PAIRING_TABLE_SIZE
          && ((emAfRf4ceGdpGetPairingBindStatus(pairingIndex)
              & PAIRING_ENTRY_BINDING_STATUS_MASK)
              != PAIRING_ENTRY_BINDING_STATUS_NOT_BOUND)
          && (emAfRf4ceZrcGetRemoteNodeCapabilities(pairingIndex)
              & EMBER_AF_RF4CE_ZRC_CAPABILITY_SUPPORT_HA_ACTIONS_ORIGINATOR));
}

#endif //EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_RECIPIENT

#if defined(EMBER_SCRIPTED_TEST)
void stopHAAnnouncementProcedure(void)
{
  emberEventControlSetInactive(emberAfPluginRf4ceZrc20HomeAutomationActionsEventControl);
  emAfZrcSetState(ZRC_STATE_INITIAL);
}
#endif

