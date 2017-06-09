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
#include "rf4ce-zrc20-action-mapping.h"

//------------------------------------------------------------------------------
// Forward declarations.

void emAfZrcActionMappingEventHandlerClient(void);

//------------------------------------------------------------------------------
// Common code (events and dispatchers)

EmberEventControl emberAfPluginRf4ceZrc20ActionMappingEventControl;

void emberAfPluginRf4ceZrc20ActionMappingEventHandler(void)
{
  emberEventControlSetInactive(emberAfPluginRf4ceZrc20ActionMappingEventControl);

#if defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_IS_ACTION_MAPPING_CLIENT) || defined(EMBER_SCRIPTED_TEST)
  emAfZrcActionMappingEventHandlerClient();
#endif // EMBER_AF_PLUGIN_RF4CE_ZRC20_IS_ACTION_MAPPING_CLIENT

  // The server doesn't need any event.
}

void emAfRf4ceZrcActionMappingBindingCompleteCallback(uint8_t pairingIndex)
{
  // The node is an action mapping client. Kick off the the action mapping
  // negotiation procedure if the peer node is an action mapping server.
#if defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_IS_ACTION_MAPPING_CLIENT) || defined(EMBER_SCRIPTED_TEST)
  emberAfRf4ceZrc20StartActionMappingsNegotiation(pairingIndex);
#endif // EMBER_AF_PLUGIN_RF4CE_ZRC20_IS_ACTION_MAPPING_CLIENT

  // Nothing to be kicked off at the server. We just serve incoming attributes
  // commands as they come in.
}

//------------------------------------------------------------------------------
// Action Mapping Client

#if defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_IS_ACTION_MAPPING_CLIENT) || defined(EMBER_SCRIPTED_TEST)

static uint8_t negotiationPairingIndex;
static uint16_t mappableActionIndex = 0;
static bool pushMappableActionsToServer(void);
static void pullActionMappingsFromServer(void);
static void finishNegotiation(EmberStatus status);

#if defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_LOCAL_IRDB_VENDOR_ATTRIBUTE_SUPPORT) || defined(EMBER_SCRIPTED_TEST)
static void pushIrdbVendorSupportToServer(void);
#endif

EmberStatus emberAfRf4ceZrc20StartActionMappingsNegotiation(uint8_t pairingIndex)
{
  // - Check that the pairing index is valid
  // - Check that the pairing is in a 'bound' status
  // - Check that the peer node is an action mapping server
  // - Check that we are not already performing an action mapping negotiation
  if (pairingIndex >= EMBER_RF4CE_PAIRING_TABLE_SIZE
      || ((emAfRf4ceGdpGetPairingBindStatus(pairingIndex)
           & PAIRING_ENTRY_BINDING_STATUS_MASK)
          == PAIRING_ENTRY_BINDING_STATUS_NOT_BOUND)
      || !(emAfRf4ceZrcGetRemoteNodeCapabilities(pairingIndex)
           & EMBER_AF_RF4CE_ZRC_CAPABILITY_SUPPORT_ACTION_MAPPING_SERVER)
      || emberEventControlGetActive(emberAfPluginRf4ceZrc20ActionMappingEventControl)) {
    return EMBER_INVALID_CALL;
  }

  emAfRf4ceZrcActionMappingStartNegotiationClient(pairingIndex);
  return EMBER_SUCCESS;
}

void emAfZrcActionMappingEventHandlerClient(void)
{
  switch(emAfZrcState) {

#if defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_LOCAL_IRDB_VENDOR_ATTRIBUTE_SUPPORT) || defined(EMBER_SCRIPTED_TEST)
  case ZRC_STATE_AM_CLIENT_PUSHING_IRDB_VENDOR_SUPPORT_TO_SERVER:
#endif // EMBER_AF_PLUGIN_RF4CE_ZRC20_LOCAL_IRDB_VENDOR_ATTRIBUTE_SUPPORT

  case ZRC_STATE_AM_CLIENT_PUSHING_MAPPABLE_ACTIONS_TO_SERVER:
  case ZRC_STATE_AM_CLIENT_PULLING_ACTION_MAPPINGS_FROM_SERVER:
    // timed-out waiting for GenericResponse() or
    // timed-out waiting for PullAttributesResponse()
    finishNegotiation(EMBER_NO_RESPONSE);
    break;
  case ZRC_STATE_INITIAL:
    if (!emAfRf4ceGdpIsBusy()) {

#if defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_LOCAL_IRDB_VENDOR_ATTRIBUTE_SUPPORT) || defined(EMBER_SCRIPTED_TEST)
      pushIrdbVendorSupportToServer();
#endif // EMBER_AF_PLUGIN_RF4CE_ZRC20_LOCAL_IRDB_VENDOR_ATTRIBUTE_SUPPORT

#if !defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_LOCAL_IRDB_VENDOR_ATTRIBUTE_SUPPORT) || defined(EMBER_SCRIPTED_TEST)
#if defined(EMBER_SCRIPTED_TEST)
    if (emAfRf4ceZrcGetLocalNodeCapabilities()
        & EMBER_AF_RF4CE_ZRC_CAPABILITY_SUPPORT_VENDOR_SPECIFIC_IRDB_FORMATS) {
      return;
    }
#endif // EMBER_SCRIPTED_TEST

      if (!pushMappableActionsToServer()) {
        finishNegotiation(EMBER_ERR_FATAL);
      }
#endif // !EMBER_AF_PLUGIN_RF4CE_ZRC20_LOCAL_IRDB_VENDOR_ATTRIBUTE_SUPPORT

      break;
    }
    // Fall-through to default
  default:
    // GDP is busy or ZRC state machine is not in its initial state: reschedule
    // the event and try later.
    emberEventControlSetDelayMS(emberAfPluginRf4ceZrc20ActionMappingEventControl,
                                ACTION_MAPPING_NEGOTIATION_PROCEDURE_DELAY_MSEC);
  }
}

void emAfRf4ceZrcActionMappingStartNegotiationClient(uint8_t pairingIndex)
{
#if defined(EMBER_SCRIPTED_TEST)
  if (!(emAfRf4ceZrcGetLocalNodeCapabilities()
        & EMBER_AF_RF4CE_ZRC_CAPABILITY_SUPPORT_ACTION_MAPPING_CLIENT)) {
    return;
  }
#endif // EMBER_SCRIPTED_TEST

  debugScriptCheck("Starting Action Mapping Negotiation (client)");

  // Turn the receiver on.
  emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0, true);

  mappableActionIndex = 0;
  negotiationPairingIndex = pairingIndex;
  emberEventControlSetActive(emberAfPluginRf4ceZrc20ActionMappingEventControl);
}

bool emAfRf4ceZrcIncomingPullAttributesResponseActionMappingClientCallback(void)
{
  EmberAfRf4ceGdpAttributeStatusRecord record;

  if (emAfZrcState == ZRC_STATE_AM_CLIENT_PULLING_ACTION_MAPPINGS_FROM_SERVER
      && emAfRf4ceGdpFetchAttributeStatusRecord(&record)
      && record.attributeId == EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_MAPPINGS
      && record.status == EMBER_AF_RF4CE_GDP_ATTRIBUTE_STATUS_SUCCESS
      && record.entryId == mappableActionIndex) {

    mappableActionIndex++;

    if (mappableActionIndex
        < emberAfPluginRf4ceZrc20GetMappableActionCountCallback(negotiationPairingIndex)) {
      // Still more action mappings to be pulled.
      pullActionMappingsFromServer();
    } else {
      // Done!
      finishNegotiation(EMBER_SUCCESS);
    }

    return true;
  }

  return false;
}

void emAfRf4ceZrcIncomingGenericResponseActionMappingClientCallback(EmberAfRf4ceGdpResponseCode responseCode)
{
  switch(emAfZrcState) {

#if defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_LOCAL_IRDB_VENDOR_ATTRIBUTE_SUPPORT) || defined(EMBER_SCRIPTED_TEST)
  case ZRC_STATE_AM_CLIENT_PUSHING_IRDB_VENDOR_SUPPORT_TO_SERVER:
    if (responseCode != EMBER_AF_RF4CE_GDP_RESPONSE_CODE_SUCCESSFUL
        || !pushMappableActionsToServer()) {
      finishNegotiation(EMBER_ERR_FATAL);
    }
    break;
#endif // EMBER_AF_PLUGIN_RF4CE_ZRC20_LOCAL_IRDB_VENDOR_ATTRIBUTE_SUPPORT

  case ZRC_STATE_AM_CLIENT_PUSHING_MAPPABLE_ACTIONS_TO_SERVER:
    if (responseCode != EMBER_AF_RF4CE_GDP_RESPONSE_CODE_SUCCESSFUL) {
      finishNegotiation(EMBER_ERR_FATAL);
    } else if (!pushMappableActionsToServer()) {
      // If we are done pushing mappable actions, start pulling action mappings.
      mappableActionIndex = 0;
      // There is always at least one mappable action in the table, otherwise
      // we would have already failed the negotiation procedure.
      pullActionMappingsFromServer();
    }
  }
}

void emAfRf4ceZrcIncomingRequestActionMappingNegotiation(void)
{
  // The action mapping client shall update its action mapping entries for all
  // its indices by invoking the Action Mapping Negotiation procedure.
  emberAfRf4ceZrc20StartActionMappingsNegotiation(emberAfRf4ceGetPairingIndex());
}

void emAfRf4ceZrcIncomingRequestSelectiveActionMappingUpdate(const uint8_t *mappableActionsList,
                                                             uint8_t mappableActionsListLength)
{
  // The action mapping client shall update its action mapping entries for at
  // least the corresponding indices before the next trigger corresponding
  // to those indices occurs.
  // TODO: for now we update all the entries (so we save some code here).
  emAfRf4ceZrcIncomingRequestActionMappingNegotiation();
}

static void finishNegotiation(EmberStatus status)
{
  // Negotiation completed, turn the radio back off.
  emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0, false);

  emberEventControlSetInactive(emberAfPluginRf4ceZrc20ActionMappingEventControl);
  emAfZrcSetState(ZRC_STATE_INITIAL);

  emberAfPluginRf4ceZrc20ActionMappingsNegotiationCompleteCallback(status);
}

// Returns true if there is at least one Mappable Action still to be sent,
// false otherwise.
static bool pushMappableActionsToServer(void)
{
  EmberAfRf4ceGdpAttributeRecord record;
  EmberAfRf4ceZrcMappableAction mappableAction;
  uint16_t vendorId = EMBER_RF4CE_NULL_VENDOR_ID; // TODO: vendor ID?
  EmberRf4ceTxOption txOptions;
  bool sendPush = false;
  uint8_t bytesLeft;

  record.attributeId = EMBER_AF_RF4CE_ZRC_ATTRIBUTE_MAPPABLE_ACTIONS;
  record.valueLength = APL_MAPPABLE_ACTIONS_SIZE;
  record.value = (uint8_t*)(&mappableAction);

  emAfRf4ceGdpStartAttributesCommand(EMBER_AF_RF4CE_GDP_COMMAND_PUSH_ATTRIBUTES);

  emberAfRf4ceGdpGetCommandTxOptions(EMBER_AF_RF4CE_GDP_COMMAND_PUSH_ATTRIBUTES,
                                     negotiationPairingIndex,
                                     vendorId,
                                     &txOptions);
  bytesLeft = emberAfRf4ceGetMaxPayload(negotiationPairingIndex, txOptions)
              - GDP_HEADER_LENGTH;

  // - Do we have more mappable actions to add?
  // - Can we fit any more mappable actions?
  while(mappableActionIndex
        < emberAfPluginRf4ceZrc20GetMappableActionCountCallback(negotiationPairingIndex)
        && bytesLeft >= MAPPABLE_ACTION_RECORD_SIZE) {

    assert(emberAfPluginRf4ceZrc20GetMappableActionCallback(negotiationPairingIndex,
                                                            mappableActionIndex,
                                                            &mappableAction) == EMBER_SUCCESS);
    record.entryId = mappableActionIndex;
    emAfRf4ceGdpAppendAttributeRecord(&record);

    mappableActionIndex++;
    bytesLeft -= MAPPABLE_ACTION_RECORD_SIZE;
    sendPush = true;
  }

  if (sendPush) {
    emAfRf4ceGdpSendAttributesCommand(negotiationPairingIndex,
                                      EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0,
                                      vendorId);

    emAfZrcSetState(ZRC_STATE_AM_CLIENT_PUSHING_MAPPABLE_ACTIONS_TO_SERVER);

    // Turn the radio on and wait for generic response.
    emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0, true);
    emberEventControlSetDelayMS(emberAfPluginRf4ceZrc20ActionMappingEventControl,
                                APLC_MAX_RESPONSE_WAIT_TIME_MS);
  }

  return sendPush;
}

static void pullActionMappingsFromServer(void)
{
  EmberAfRf4ceGdpAttributeIdentificationRecord recordId;

  recordId.attributeId = EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_MAPPINGS;
  recordId.entryId = mappableActionIndex;

  emberAfRf4ceGdpPullAttributes(negotiationPairingIndex,
                                EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0,
                                EMBER_RF4CE_NULL_VENDOR_ID, // TODO: vendor ID?
                                &recordId,
                                1);

  emAfZrcSetState(ZRC_STATE_AM_CLIENT_PULLING_ACTION_MAPPINGS_FROM_SERVER);

  // Turn the radio on and wait for pull response.
  emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0, true);
  emberEventControlSetDelayMS(emberAfPluginRf4ceZrc20ActionMappingEventControl,
                              APLC_MAX_RESPONSE_WAIT_TIME_MS);
}

#if defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_LOCAL_IRDB_VENDOR_ATTRIBUTE_SUPPORT) || defined(EMBER_SCRIPTED_TEST)

static void pushIrdbVendorSupportToServer(void)
{
  EmberAfRf4ceGdpAttributeRecord record;
  uint8_t irdbVendorSupport[EMBER_AF_RF4CE_ZRC_IRDB_VENDOR_ID_COUNT*2];

#if defined(EMBER_SCRIPTED_TEST)
  if (!(emAfRf4ceZrcGetLocalNodeCapabilities()
        & EMBER_AF_RF4CE_ZRC_CAPABILITY_SUPPORT_VENDOR_SPECIFIC_IRDB_FORMATS)) {
    return;
  }
#endif

  record.attributeId = EMBER_AF_RF4CE_ZRC_ATTRIBUTE_IRDB_VENDOR_SUPPORT;
  record.valueLength = EMBER_AF_RF4CE_ZRC_IRDB_VENDOR_ID_COUNT*2;
  record.value = irdbVendorSupport;

  emAfRf4ceZrcReadLocalAttribute(EMBER_AF_RF4CE_ZRC_ATTRIBUTE_IRDB_VENDOR_SUPPORT,
                                 EMBER_AF_RF4CE_ZRC_IRDB_VENDOR_ID_COUNT*2,
                                 irdbVendorSupport);

  emAfZrcSetState(ZRC_STATE_AM_CLIENT_PUSHING_IRDB_VENDOR_SUPPORT_TO_SERVER);

  emberAfRf4ceGdpPushAttributes(negotiationPairingIndex,
                                EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0,
                                EMBER_RF4CE_NULL_VENDOR_ID, // TODO: vendor ID?
                                &record,
                                1);

  // Turn the radio on and wait for generic response.
  emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0, true);
  emberEventControlSetDelayMS(emberAfPluginRf4ceZrc20ActionMappingEventControl,
                              APLC_MAX_RESPONSE_WAIT_TIME_MS);
}

#endif // EMBER_AF_PLUGIN_RF4CE_ZRC20_LOCAL_IRDB_VENDOR_ATTRIBUTE_SUPPORT

#else // EMBER_AF_PLUGIN_RF4CE_ZRC20_IS_ACTION_MAPPING_CLIENT

EmberStatus emberAfRf4ceZrc20StartActionMappingsNegotiation(uint8_t pairingIndex)
{
  return EMBER_INVALID_CALL;
}

#endif // EMBER_AF_PLUGIN_RF4CE_ZRC20_IS_ACTION_MAPPING_CLIENT

//------------------------------------------------------------------------------
// Action Mapping Server

#if defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_IS_ACTION_MAPPING_SERVER) || defined(EMBER_SCRIPTED_TEST)

static bool acceptIncomingAttributeCommand(uint8_t pairingIndex);

bool emAfRf4ceZrcIncomingPushAttributesActionMappingServerCallback(void)
{
  EmberAfRf4ceGdpAttributeRecord record;
  uint8_t pairingIndex = emberAfRf4ceGetPairingIndex();

  if (acceptIncomingAttributeCommand(pairingIndex)) {
    if (!emAfRf4ceGdpFetchAttributeRecord(&record)) {
      return false;
    }

    if (record.attributeId == EMBER_AF_RF4CE_ZRC_ATTRIBUTE_IRDB_VENDOR_SUPPORT
        && !emAfRf4ceGdpFetchAttributeRecord(&record) // Only record
        && (emAfRf4ceZrcGetRemoteNodeCapabilities(pairingIndex)
            & EMBER_AF_RF4CE_ZRC_CAPABILITY_SUPPORT_VENDOR_SPECIFIC_IRDB_FORMATS)) {
       return true;
    } else if (record.attributeId == EMBER_AF_RF4CE_ZRC_ATTRIBUTE_MAPPABLE_ACTIONS) {
      emAfRf4ceGdpResetFetchAttributeFinger();
      // We process the PushAttributes() if it contains only mappable actions
      // records.
      while(emAfRf4ceGdpFetchAttributeRecord(&record)) {
        if (record.attributeId != EMBER_AF_RF4CE_ZRC_ATTRIBUTE_MAPPABLE_ACTIONS
            || record.valueLength != APL_MAPPABLE_ACTIONS_SIZE) {
          return false;
        }
      }
      return true;
    }
  }

  return false;
}

bool emAfRf4ceZrcIncomingPullAttributesActionMappingServerCallback(void)
{
  EmberAfRf4ceGdpAttributeIdentificationRecord record;
  uint8_t pairingIndex = emberAfRf4ceGetPairingIndex();

  if (acceptIncomingAttributeCommand(pairingIndex)
      && emAfRf4ceGdpFetchAttributeIdentificationRecord(&record)
      && record.attributeId == EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_MAPPINGS
      // Only record
      && !emAfRf4ceGdpFetchAttributeIdentificationRecord(&record)) {
    return true;
  }

  return false;
}

// We accept an attribute command if the pairing is in 'bound' status and if
// the peer node is an action mapping client.
static bool acceptIncomingAttributeCommand(uint8_t pairingIndex)
{
  return (pairingIndex < EMBER_RF4CE_PAIRING_TABLE_SIZE
          && ((emAfRf4ceGdpGetPairingBindStatus(pairingIndex)
              & PAIRING_ENTRY_BINDING_STATUS_MASK)
              != PAIRING_ENTRY_BINDING_STATUS_NOT_BOUND)
          && (emAfRf4ceZrcGetRemoteNodeCapabilities(pairingIndex)
              & EMBER_AF_RF4CE_ZRC_CAPABILITY_SUPPORT_ACTION_MAPPING_CLIENT));
}

#endif // EMBER_AF_PLUGIN_RF4CE_ZRC20_IS_ACTION_MAPPING_SERVER

#if defined(EMBER_SCRIPTED_TEST)
void stopActionMappingNegotiationProcedure(void)
{
  emberEventControlSetInactive(emberAfPluginRf4ceZrc20ActionMappingEventControl);
  emAfZrcSetState(ZRC_STATE_INITIAL);
}
#endif
