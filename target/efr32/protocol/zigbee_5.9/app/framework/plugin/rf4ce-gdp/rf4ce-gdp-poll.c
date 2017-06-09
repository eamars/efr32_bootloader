// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"

#include "app/framework/plugin/rf4ce-profile/rf4ce-profile.h"
#include "rf4ce-gdp.h"
#include "rf4ce-gdp-attributes.h"
#include "rf4ce-gdp-poll.h"
#include "rf4ce-gdp-internal.h"

#if defined(EMBER_SCRIPTED_TEST)
#include "rf4ce-gdp-test.h"
#endif // EMBER_SCRIPTED_TEST

//------------------------------------------------------------------------------
// Common code (poll=NONE, poll=CLIENT or poll=SERVER).

enum {
  POLLING_STATUS_NONE                              = 0x00,
  POLLING_STATUS_NEGOTIATION_PENDING_CLIENT        = 0x01,
  POLLING_STATUS_NEGOTIATION_IN_PROGRESS_CLIENT    = 0x02,
  POLLING_STATUS_NEGOTIATION_IN_PROGRESS_SERVER    = 0x03,
  POLLING_STATUS_POLL_PENDING                      = 0x04,
  POLLING_STATUS_POLL_SENT_WAITING_FOR_RESPONSE    = 0x05
};

#if (EMBER_AF_PLUGIN_RF4CE_GDP_POLL_SUPPORT != POLL_NONE)
static struct {
  uint8_t status;
  uint8_t pairingIndex;
  uint8_t retries;
} pollingInfo = {POLLING_STATUS_NONE, 0xFF, 0};
#endif

// Polling event (for now is common to both the client and server code).
EmberEventControl emberAfPluginRf4ceGdpPollingEventControl;

#if defined(EMBER_SCRIPTED_TEST)
bool nodeIsPollingServer = true;
#endif

// Forward declarations
void emAfRf4ceGdpPollingClientEventHandler(void);
void emAfRf4ceGdpPollingServerEventHandler(void);
void emAfRf4ceGdpPollingClientStackStatusCallback(EmberStatus status);
void emAfRf4ceGdpPollingIncomingCommandClientCallback(bool framePending);
void emAfPendingCommandEventHandlerPollNegotiationClient(void);
bool emAfRf4ceGdpIncomingPushAttributesPollServerCallback(void);
bool emAfRf4ceGdpIncomingPullAttributesPollServerCallback(void);
void emAfRf4ceGdpIncomingGenericResponsePollClientCallback(EmberAfRf4ceGdpResponseCode responseCode);
bool emAfRf4ceGdpIncomingPullAttributesResponsePollClientCallback(void);
void emAfRf4ceGdpIncomingHeartbeatPollServerCallback(EmberAfRf4ceGdpHeartbeatTrigger trigger);
void emAfRf4ceGdpHeartbeatSentPollClientCallback(EmberStatus status);
void emAfRf4ceGdpPollingNotifyBindingCompletePollClientCallback(uint8_t pairingIndex);
void emAfRf4ceGdpPollingNotifyBindingCompletePollServerCallback(uint8_t pairingIndex);
EmberStatus emAfRf4ceGdpPollInternal(uint8_t pairingIndex, uint16_t vendorId, EmberAfRf4ceGdpHeartbeatTrigger trigger);

void emberAfPluginRf4ceGdpPollingEventHandler(void)
{
  emberEventControlSetInactive(emberAfPluginRf4ceGdpPollingEventControl);

#if (defined(EMBER_AF_PLUGIN_RF4CE_GDP_IS_POLL_CLIENT) || defined(EMBER_SCRIPTED_TEST))
  emAfRf4ceGdpPollingClientEventHandler();
#endif

#if (defined(EMBER_AF_PLUGIN_RF4CE_GDP_IS_POLL_SERVER) || defined(EMBER_SCRIPTED_TEST))
  emAfRf4ceGdpPollingServerEventHandler();
#endif
}

void emAfPendingCommandEventHandlerPollNegotiation(void)
{
#if (defined(EMBER_AF_PLUGIN_RF4CE_GDP_IS_POLL_CLIENT) || defined(EMBER_SCRIPTED_TEST))
  emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE, false);
  emAfPendingCommandEventHandlerPollNegotiationClient();
#endif
}

void emAfRf4ceGdpPollingStackStatusCallback(EmberStatus status)
{
#if (defined(EMBER_AF_PLUGIN_RF4CE_GDP_IS_POLL_CLIENT) || defined(EMBER_SCRIPTED_TEST))
  emAfRf4ceGdpPollingClientStackStatusCallback(status);
#endif
}

void emAfRf4ceGdpPollingIncomingCommandCallback(bool framePending)
{
#if (defined(EMBER_AF_PLUGIN_RF4CE_GDP_IS_POLL_CLIENT) || defined(EMBER_SCRIPTED_TEST))
  emAfRf4ceGdpPollingIncomingCommandClientCallback(framePending);
#endif
}

bool emAfRf4ceGdpIncomingPushAttributesPollNegotiationCallback(void)
{
#if (defined(EMBER_AF_PLUGIN_RF4CE_GDP_IS_POLL_SERVER) || defined(EMBER_SCRIPTED_TEST))
  return emAfRf4ceGdpIncomingPushAttributesPollServerCallback();
#else
  return false;
#endif
}

bool emAfRf4ceGdpIncomingPullAttributesPollNegotiationCallback(void)
{
#if (defined(EMBER_AF_PLUGIN_RF4CE_GDP_IS_POLL_SERVER) || defined(EMBER_SCRIPTED_TEST))
  return emAfRf4ceGdpIncomingPullAttributesPollServerCallback();
#else
  return false;
#endif
}

void emAfRf4ceGdpIncomingGenericResponsePollNegotiationCallback(EmberAfRf4ceGdpResponseCode responseCode)
{
#if (defined(EMBER_AF_PLUGIN_RF4CE_GDP_IS_POLL_CLIENT) || defined(EMBER_SCRIPTED_TEST))
  emAfRf4ceGdpIncomingGenericResponsePollClientCallback(responseCode);
#endif
}

bool emAfRf4ceGdpIncomingPullAttributesResponsePollNegotiationCallback(void)
{
#if (defined(EMBER_AF_PLUGIN_RF4CE_GDP_IS_POLL_CLIENT) || defined(EMBER_SCRIPTED_TEST))
  return emAfRf4ceGdpIncomingPullAttributesResponsePollClientCallback();
#else
  return false;
#endif
}

void emAfRf4ceGdpIncomingHeartbeat(EmberAfRf4ceGdpHeartbeatTrigger trigger)
{
#if (defined(EMBER_AF_PLUGIN_RF4CE_GDP_IS_POLL_SERVER) || defined(EMBER_SCRIPTED_TEST))
  emAfRf4ceGdpIncomingHeartbeatPollServerCallback(trigger);
#endif
}

void emAfRf4ceGdpHeartbeatSent(EmberStatus status)
{
#if (defined(EMBER_AF_PLUGIN_RF4CE_GDP_IS_POLL_CLIENT) || defined(EMBER_SCRIPTED_TEST))
  emAfRf4ceGdpHeartbeatSentPollClientCallback(status);
#endif
}

void emAfRf4ceGdpPollingNotifyBindingComplete(uint8_t pairingIndex)
{
#if (defined(EMBER_AF_PLUGIN_RF4CE_GDP_IS_POLL_CLIENT) || defined(EMBER_SCRIPTED_TEST))
  emAfRf4ceGdpPollingNotifyBindingCompletePollClientCallback(pairingIndex);
#endif

#if (defined(EMBER_AF_PLUGIN_RF4CE_GDP_IS_POLL_SERVER) || defined(EMBER_SCRIPTED_TEST))
  emAfRf4ceGdpPollingNotifyBindingCompletePollServerCallback(pairingIndex);
#endif
}

EmberStatus emberAfRf4ceGdpPoll(uint8_t pairingIndex,
                                uint16_t vendorId,
                                EmberAfRf4ceGdpHeartbeatTrigger trigger)
{
#if (defined(EMBER_AF_PLUGIN_RF4CE_GDP_IS_POLL_CLIENT) || defined(EMBER_SCRIPTED_TEST))
  return emAfRf4ceGdpPollInternal(pairingIndex, vendorId, trigger);
#else
  return EMBER_INVALID_CALL;
#endif
}

//------------------------------------------------------------------------------
// Poll Client

#if (defined(EMBER_AF_PLUGIN_RF4CE_GDP_IS_POLL_CLIENT) || defined(EMBER_SCRIPTED_TEST))

// Forward declarations
static void startPollNegotiationProcedure(void);
static void finishPollNegotiationProcedure(bool success);
static void scheduleNextTimeBasedPoll(uint8_t newPairingIndex, bool init);

// Static variables.
static uint8_t currentPollingPairingIndex = 0xFF;
// Here we store for each pairing when the next poll will happen.
static uint32_t nextPollTimeMs[EMBER_RF4CE_PAIRING_TABLE_SIZE];

EmberStatus emAfRf4ceGdpPollInternal(uint8_t pairingIndex,
                                     uint16_t vendorId,
                                     EmberAfRf4ceGdpHeartbeatTrigger trigger)
{
  EmberStatus status;
  uint8_t pollConfiguration[APL_GDP_POLL_CONFIGURATION_SIZE];

  emAfRf4ceGdpGetPollConfigurationAttribute(pairingIndex, pollConfiguration);

  // - Check that the node is not currently involved in a negotiation procedure
  // - Check that there is no poll pending already
  // - Check polling is active on the passed pairing index
  // - Heartbeat is the only supported polling method
  // - Check passed trigger value is not reserved
  // - Check passed trigger is either generic activity or is enabled in the
  //     local poll configuration attribute
  if (pollingInfo.status != POLLING_STATUS_NONE
      || pairingIndex >= EMBER_RF4CE_PAIRING_TABLE_SIZE
      || !(emAfRf4ceGdpGetPairingBindStatus(pairingIndex) & PAIRING_ENTRY_POLLING_ACTIVE_BIT)
      || (pollConfiguration[POLL_CONFIGURATION_POLLING_METHOD_ID_OFFSET]
          != EMBER_AF_RF4CE_GDP_POLLING_METHOD_HEARTBEAT)
      || !emAfRf4ceGdpIsPollTriggerValid(trigger)
      ||  (trigger != EMBER_AF_RF4CE_GDP_HEARTBEAT_TRIGGER_GENERIC_ACTIVITY
           && !(pollConfiguration[POLL_CONFIGURATION_POLLING_TRIGGER_CONFIG_OFFSET]
                & BIT(trigger - 1)))) {
    return EMBER_INVALID_CALL;
  }

  status = emberAfRf4ceGdpHeartbeat(pairingIndex, vendorId, trigger);

  if (status == EMBER_SUCCESS) {
    pollingInfo.status = POLLING_STATUS_POLL_PENDING;

    // Keep the receiver on.
    emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE, true);
  }

  return status;
}

void emAfRf4ceGdpPollingClientStackStatusCallback(EmberStatus status)
{
#if defined(EMBER_SCRIPTED_TEST)
  if (nodeIsPollingServer) {
    return;
  }
#endif

  if (status == EMBER_NETWORK_UP) {
    scheduleNextTimeBasedPoll(0xFF, true);
  }
}

void emAfRf4ceGdpHeartbeatSentPollClientCallback(EmberStatus status)
{
  // Upon successful transmission of the Heartbeat frame, the poll client shall
  // request that the NWK layer enable its receiver and wait either for a
  // maximum duration of timeout value or until a command frame is received
  // from the recipient. The receiver was already enabled when we sent out
  // the heartbeat command.
  if (status == EMBER_SUCCESS) {
    pollingInfo.status = POLLING_STATUS_POLL_SENT_WAITING_FOR_RESPONSE;
    emberEventControlSetDelayMS(emberAfPluginRf4ceGdpPollingEventControl,
                                APLC_MAX_RX_ON_WAIT_TIME_MS);
  } else {
    EmberRf4cePairingTableEntry entry;

    // If the heartbeat command frame is not successfully transmitted, the
    // poll client shall read the pairing table entry corresponding to the
    // poll server from the NWK layer, update the value of destination logical
    // channel field to the next value in the sequence of RF4CE channels and
    // write back the resulting entry to the NWK layer. This is to ensure that
    // the next Heartbeat command frame is transmitted on a different channel.
    // This code supports non-standard RF4CE channels (for test events).
    emberAfRf4ceGetPairingTableEntry(currentPollingPairingIndex, &entry);
    if (entry.channel > 20) {
      entry.channel -= 10;
    } else {
      entry.channel += 5;
    }
    emberAfRf4ceSetPairingTableEntry(currentPollingPairingIndex, &entry);

    // We failed to send out the hearbeat command: turn the receiver off.
    emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE, false);

    // The poll client MAY then transmit another heartbeat command to the same
    // poll server until all channels have been attempted. If the heartbeat
    // command transmission is not successful on any attempted channel, the poll
    // client should not attempt further transmissions of the heartbeat command
    // to that poll server until the next time when it wishes to check for
    // pending transmissions. Behavior is implementation-dependent in this state.
    // We chose to simply reschedule the next poll.
    scheduleNextTimeBasedPoll(0xFF, false);
  }
}

void emAfRf4ceGdpPollingIncomingCommandClientCallback(bool framePending)
{
#if defined(EMBER_SCRIPTED_TEST)
  if (nodeIsPollingServer) {
    return;
  }
#endif

  if (pollingInfo.status == POLLING_STATUS_POLL_SENT_WAITING_FOR_RESPONSE) {
    // We received a command with the frame pending bit set: "enable the
    // receiver for a maximum of aplcMaxRxOnWaitTime". The receiver was already
    // on, so we just refresh the timer.
    if (framePending) {
      emberEventControlSetDelayMS(emberAfPluginRf4ceGdpPollingEventControl,
                                  APLC_MAX_RX_ON_WAIT_TIME_MS);
    } else {
      // We received a command with the frame pending bit cleared. Turn the
      // receiver off and reschedule the next time based poll.
      emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE, false);
      scheduleNextTimeBasedPoll(0xFF, false);
    }
  }
}

void emAfRf4ceGdpPollingClientEventHandler(void)
{
#if defined(EMBER_SCRIPTED_TEST)
  if (nodeIsPollingServer) {
    return;
  }
#endif

  // Handle time-based polling
  if (pollingInfo.status == POLLING_STATUS_NONE) {
    // Send poll
    if (emAfRf4ceGdpPollInternal(currentPollingPairingIndex,
                                 0, // TODO: vendor ID
                                 EMBER_AF_RF4CE_GDP_HEARTBEAT_TRIGGER_TIME_BASED_POLLING)
        != EMBER_SUCCESS) {
      scheduleNextTimeBasedPoll(0xFF, false);
    }
  // Check that the binding status for the saved pairing index is still 'bound'.
  } else if (pollingInfo.status == POLLING_STATUS_NEGOTIATION_PENDING_CLIENT
      && ((emAfRf4ceGdpGetPairingBindStatus(pollingInfo.pairingIndex)
           & PAIRING_ENTRY_BINDING_STATUS_MASK)
          != PAIRING_ENTRY_BINDING_STATUS_NOT_BOUND)) {
    // If the internal state is in the initial state, start the poll negotiation
    // procedure, otherwise re-schedule the event and try again later on.
    if (internalGdpState() == INTERNAL_STATE_NONE) {
      startPollNegotiationProcedure();
    } else {
      emberEventControlSetDelayMS(emberAfPluginRf4ceGdpPollingEventControl,
                                  POLLING_NEGOTIATION_PROCEDURE_DELAY_MSEC);
    }
  } else if (pollingInfo.status == POLLING_STATUS_POLL_SENT_WAITING_FOR_RESPONSE) {
    // We timed out waiting for a response to the heartbeat command: turn the
    // receiver off and reschedule the next heartbeat.
    emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE, false);
    scheduleNextTimeBasedPoll(0xFF, false);
  }
}

void emAfPendingCommandEventHandlerPollNegotiationClient(void)
{
#if defined(EMBER_SCRIPTED_TEST)
  if (nodeIsPollingServer) {
    return;
  }
#endif

  if (pollingInfo.status == POLLING_STATUS_NEGOTIATION_IN_PROGRESS_CLIENT) {
    // - If the generic response is not received from the poll server [...], the
    //   poll client shall consider the poll negotiation procedure a failure and
    //   the procedure shall be terminated.
    //
    // - If no PullResponse command frame is received from the poll server
    //   [...], the poll client shall consider the poll negotiation procedure a
    //   failure and the procedure shall be terminated.
    finishPollNegotiationProcedure(false);
  }
}

void emAfRf4ceGdpIncomingGenericResponsePollClientCallback(EmberAfRf4ceGdpResponseCode responseCode)
{
#if defined(EMBER_SCRIPTED_TEST)
  if (nodeIsPollingServer) {
    return;
  }
#endif

  if (internalGdpState() == INTERNAL_STATE_GDP_POLL_CONFIG_CLIENT_PUSH_PENDING
      && pollingInfo.status == POLLING_STATUS_NEGOTIATION_IN_PROGRESS_CLIENT
      && pollingInfo.pairingIndex == emberAfRf4ceGetPairingIndex()) {
    if (responseCode == EMBER_AF_RF4CE_GDP_RESPONSE_CODE_SUCCESSFUL) {
      EmberAfRf4ceGdpAttributeIdentificationRecord record;

      // Next, the poll client shall interrogate the poll server about the polling
      // procedure it shall use. To do this, the poll client shall generate and
      // transmit a PullAttributes command frame containing the
      // aplPollConfiguration attribute to the poll server.
      record.attributeId = EMBER_AF_RF4CE_GDP_ATTRIBUTE_POLL_CONFIGURATION;

      if (emberAfRf4ceGdpPullAttributes(emberAfRf4ceGetPairingIndex(),
                                        EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE,
                                        EMBER_RF4CE_NULL_VENDOR_ID,
                                        &record,
                                        1) == EMBER_SUCCESS) {
        emAfGdpStartCommandPendingTimer(INTERNAL_STATE_GDP_POLL_CONFIG_CLIENT_PULL_PENDING,
                                        APLC_MAX_CONFIG_WAIT_TIME_MS);
      } else {
        finishPollNegotiationProcedure(false);
      }
    } else {
      // If the generic response is not received from the poll server [...] or if
      // the response code of the generic response command frame is not indicating
      // success, the poll client shall consider the poll negotiation procedure a
      // failure and the the procedure shall be terminated.
      finishPollNegotiationProcedure(false);
    }
  }
}

bool emAfRf4ceGdpIncomingPullAttributesResponsePollClientCallback(void)
{
#if defined(EMBER_SCRIPTED_TEST)
  if (nodeIsPollingServer) {
    return false;
  }
#endif

  if (internalGdpState() == INTERNAL_STATE_GDP_POLL_CONFIG_CLIENT_PULL_PENDING
      && pollingInfo.status == POLLING_STATUS_NEGOTIATION_IN_PROGRESS_CLIENT
      && pollingInfo.pairingIndex == emberAfRf4ceGetPairingIndex()) {
    EmberAfRf4ceGdpAttributeStatusRecord record;
    if (emAfRf4ceGdpFetchAttributeStatusRecord(&record)
        && record.attributeId == EMBER_AF_RF4CE_GDP_ATTRIBUTE_POLL_CONFIGURATION
        && record.status == EMBER_AF_RF4CE_GDP_ATTRIBUTE_STATUS_SUCCESS) {
      // Save the poll configuration attribute in persistent storage.
      emAfRf4ceGdpSetPollConfigurationAttribute(pollingInfo.pairingIndex,
                                                record.value);
      finishPollNegotiationProcedure(true);
    } else {
      finishPollNegotiationProcedure(false);
    }

    return true;
  }

  return false;
}

// Section 7.2.9.2.1: "After binding is successfully completed, the poll client
// shall inform the poll server about the polling procedure it supports".
//
// If the node is a poll client, we schedule a timer and start the poll
// negotiation procedure upon expiration. We delay the procedure in case the
// enhanced security procedure takes place, which should happen right after
// binding has completed.
void emAfRf4ceGdpPollingNotifyBindingCompletePollClientCallback(uint8_t pairingIndex)
{
#if defined(EMBER_SCRIPTED_TEST)
  if (nodeIsPollingServer) {
    return;
  }
#endif

  // If the remote node is a poll server, we schedule the poll negotiation
  // procedure. We also remember that the remote supports polling in the
  // persistent pairing status bitmask.
  if (emAfRf4ceGdpRemoteNodeAttributes.gdpCapabilities
      & GDP_CAPABILITIES_SUPPORT_POLL_SERVER_BIT) {

    pollingInfo.status = POLLING_STATUS_NEGOTIATION_PENDING_CLIENT;
    pollingInfo.pairingIndex = pairingIndex;
    pollingInfo.retries = 0;

    emAfRf4ceGdpSetPairingBindStatus(pairingIndex,
                                     (emAfRf4ceGdpGetPairingBindStatus(pairingIndex)
                                      | PAIRING_ENTRY_REMOTE_NODE_SUPPORTS_POLLING_BIT));

    // Observe blackout period prior to kicking off the poll configuration
    // procedure.
    emberEventControlSetDelayMS(emberAfPluginRf4ceGdpPollingEventControl,
                                APLC_CONFIG_BLACKOUT_TIME_MS);
  }
}

static void startPollNegotiationProcedure(void)
{
  EmberAfRf4ceGdpAttributeRecord record;

  record.attributeId = EMBER_AF_RF4CE_GDP_ATTRIBUTE_POLL_CONSTRAINTS;
  record.valueLength = APL_GDP_POLL_CONSTRAINTS_SIZE;
  record.value = (uint8_t*)emAfRf4ceGdpLocalNodeAttributes.pollConstraints;

  if (emberAfRf4ceGdpPushAttributes(pollingInfo.pairingIndex,
                                    EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE,
                                    EMBER_RF4CE_NULL_VENDOR_ID,
                                    &record,
                                    1) == EMBER_SUCCESS) {
    pollingInfo.status = POLLING_STATUS_NEGOTIATION_IN_PROGRESS_CLIENT;
    pollingInfo.retries++;
    emAfGdpStartCommandPendingTimer(INTERNAL_STATE_GDP_POLL_CONFIG_CLIENT_PUSH_PENDING,
                                    APLC_MAX_CONFIG_WAIT_TIME_MS);
  }
}

static void finishPollNegotiationProcedure(bool success)
{
  uint8_t bindStatusMask =
      emAfRf4ceGdpGetPairingBindStatus(pollingInfo.pairingIndex);

  setInternalState(INTERNAL_STATE_NONE);
  pollingInfo.status = POLLING_STATUS_NONE;

  // Cancel the command and the polling events.
  emberEventControlSetInactive(emberAfPluginRf4ceGdpPendingCommandEventControl);
  emberEventControlSetInactive(emberAfPluginRf4ceGdpPollingEventControl);

  if (success) {
    debugScriptCheck("Poll negotiation procedure succeeded");

    emAfRf4ceGdpSetPairingBindStatus(pollingInfo.pairingIndex,
                                     (bindStatusMask
                                      | PAIRING_ENTRY_POLLING_ACTIVE_BIT));
    // If time-based polling is supported, the plugin code takes care of
    // periodically polling.
    scheduleNextTimeBasedPoll(pollingInfo.pairingIndex, false);
  } else {
    debugScriptCheck("Poll negotiation procedure failed");

    // If the poll negotiation fails, the poll client should retry the polling
    // negotiation procedure later.
    // In our implementation we have a maximum number of retries, after which
    // the negotiation procedure won't be restarted anymore.
    if (pollingInfo.retries < POLLING_NEGOTIATION_PROCEDURE_CLIENT_MAX_RETIRES) {
      debugScriptCheck("Retry procedure after delay");
      pollingInfo.status = POLLING_STATUS_NEGOTIATION_PENDING_CLIENT;
      emberEventControlSetDelayQS(emberAfPluginRf4ceGdpPollingEventControl,
                                  POLLING_NEGOTIATION_PROCEDURE_AFTER_FAILURE_DELAY_SEC*4);
    }
#if defined(EMBER_SCRIPTED_TEST)
    if (pollingInfo.retries == POLLING_NEGOTIATION_PROCEDURE_CLIENT_MAX_RETIRES) {
      debugScriptCheck("Reached maximum retries, give up");
    }
#endif
  }
}

static uint32_t getPollInterval(uint8_t pairingIndex)
{
  uint8_t pollConfiguration[APL_GDP_POLL_CONFIGURATION_SIZE];

  emAfRf4ceGdpGetPollConfigurationAttribute(pairingIndex,
                                            pollConfiguration);

  return emberFetchLowHighInt32u(pollConfiguration
                                 + POLL_CONFIGURATION_POLLING_TIME_INTERVAL_OFFSET);
}

// This function updates the next poll time for the last server polled, computes
// the closest in time next server to be polled and schedule the polling event.
// If the passed newPairingIndex is a valid one, this function initializes the
// corresponding entry in the next poll time table.
// If the passed init is true, this function initializes all the entries in the
// next poll time table that correspond to a binding with active polling.
static void scheduleNextTimeBasedPoll(uint8_t newPairingIndex, bool init)
{
  uint32_t nowMs = halCommonGetInt32uMillisecondTick();
  uint32_t minNextPollTimeMs;
  uint8_t i;

  pollingInfo.status = POLLING_STATUS_NONE;

  // At init time we initialize the next polling time for all the entries for
  // which polling is active.
  if (init) {
    for (i=0; i<EMBER_RF4CE_PAIRING_TABLE_SIZE; i++) {
      if (emAfRf4ceGdpGetPairingBindStatus(i) & PAIRING_ENTRY_POLLING_ACTIVE_BIT) {
        nextPollTimeMs[i] = nowMs + getPollInterval(i);
      }
    }
  }

  // If a new poll server was added, we initialize the next polling time table
  // for it.
  if (newPairingIndex < EMBER_RF4CE_PAIRING_TABLE_SIZE) {
    nextPollTimeMs[newPairingIndex] = nowMs + getPollInterval(newPairingIndex);
  }

  // If we still have an active polling process with the server we just polled,
  // we update the corresponding entry in the next poll time table.
  if (currentPollingPairingIndex < EMBER_RF4CE_PAIRING_TABLE_SIZE
      && (emAfRf4ceGdpGetPairingBindStatus(currentPollingPairingIndex)
          & PAIRING_ENTRY_POLLING_ACTIVE_BIT)) {
    nextPollTimeMs[currentPollingPairingIndex] =
        nowMs + getPollInterval(currentPollingPairingIndex);
  }

  currentPollingPairingIndex = 0xFF;

  for (i=0; i<EMBER_RF4CE_PAIRING_TABLE_SIZE; i++) {
    if (emAfRf4ceGdpGetPairingBindStatus(i) & PAIRING_ENTRY_POLLING_ACTIVE_BIT) {
      if (currentPollingPairingIndex == 0xFF
          || timeGTorEqualInt32u(minNextPollTimeMs, nextPollTimeMs[i])) {
        minNextPollTimeMs = nextPollTimeMs[i];
        currentPollingPairingIndex = i;
      }
    }
  }

  // Schedule the next poll.
  if (currentPollingPairingIndex < EMBER_RF4CE_PAIRING_TABLE_SIZE) {
    minNextPollTimeMs = (timeGTorEqualInt32u(minNextPollTimeMs, nowMs)
                         ? elapsedTimeInt32u(nowMs, minNextPollTimeMs)
                         : 0);
    if (EMBER_MAX_EVENT_CONTROL_DELAY_MS < minNextPollTimeMs) {
      minNextPollTimeMs = EMBER_MAX_EVENT_CONTROL_DELAY_MS;
    }
    emberEventControlSetDelayMS(emberAfPluginRf4ceGdpPollingEventControl,
                                minNextPollTimeMs);
  } else {
    emberEventControlSetInactive(emberAfPluginRf4ceGdpPollingEventControl);
  }
}

#endif // EMBER_AF_PLUGIN_RF4CE_GDP_IS_POLL_CLIENT

//------------------------------------------------------------------------------
// Poll Server

#if (defined(EMBER_AF_PLUGIN_RF4CE_GDP_IS_POLL_SERVER) || defined(EMBER_SCRIPTED_TEST))

// The poll server shall use the information in the aplPollConstraints attribute
// to make a decision on the poll procedure that the poll client shall use.
// It shall select one of the polling methods, it shall select a polling period
// from the range that the poll client indicated to support for the given
// polling method and shall select a polling timeout.
// (For now the only polling method provided by the GDP specs is the Heartbeat
// based polling scheme).
static void selectClientPollProcedure(uint8_t pairingIndex,
                                      const uint8_t *clientPollConstraints) {
  uint8_t clientPollConfiguration[APL_GDP_POLL_CONFIGURATION_SIZE];
  uint16_t clientTriggersCapabilities =
      HIGH_LOW_TO_INT(clientPollConstraints[POLL_CONSTRAINT_RECORD_POLLING_TRIGGERS_CAPABILITIES_OFFSET + 1],
                      clientPollConstraints[POLL_CONSTRAINT_RECORD_POLLING_TRIGGERS_CAPABILITIES_OFFSET]);

  MEMSET(clientPollConfiguration, 0x00, APL_GDP_POLL_CONFIGURATION_SIZE);

  // Does the client support heartbeat?
  if (clientPollConstraints[POLL_CONSTRAINT_RECORD_POLLING_METHOD_ID_OFFSET]
      == EMBER_AF_RF4CE_GDP_POLLING_METHOD_HEARTBEAT) {
    clientPollConfiguration[POLL_CONFIGURATION_POLLING_METHOD_ID_OFFSET] =
        EMBER_AF_RF4CE_GDP_POLLING_METHOD_HEARTBEAT;

    // The polling triggers are just copied from the client poll constraints.
    clientPollConfiguration[POLL_CONFIGURATION_POLLING_TRIGGER_CONFIG_OFFSET] =
        clientPollConstraints[POLL_CONSTRAINT_RECORD_POLLING_TRIGGERS_CAPABILITIES_OFFSET];
    clientPollConfiguration[POLL_CONFIGURATION_POLLING_TRIGGER_CONFIG_OFFSET + 1] =
        clientPollConstraints[POLL_CONSTRAINT_RECORD_POLLING_TRIGGERS_CAPABILITIES_OFFSET + 1];

    // Polling key press counter: this field shall be ignored [...] when the
    // polling on key press enabled bit is set to 0.
    // Check that the max value is always >= than the min value.
    if (clientTriggersCapabilities & EMBER_AF_RF4CE_GDP_POLLING_TRIGGER_POLLING_ON_KEY_PRESS_ENABLED) {
      uint8_t minClientVal =
          clientPollConstraints[POLL_CONSTRAINT_RECORD_MIN_POLLING_KEY_PRESS_COUNT_OFFSET];
      uint8_t maxClientVal =
          clientPollConstraints[POLL_CONSTRAINT_RECORD_MAX_POLLING_KEY_PRESS_COUNT_OFFSET];

      if (minClientVal <= maxClientVal) {
        // TODO: We always select the minimum value (totally arbitrary).
      clientPollConfiguration[POLL_CONFIGURATION_POLLING_KEY_PRESS_COUNTER_OFFSET] =
          minClientVal;
      } else {
        // If min > max remove the key-press trigger as supported trigger.
        clientPollConfiguration[POLL_CONFIGURATION_POLLING_TRIGGER_CONFIG_OFFSET] &=
            ~EMBER_AF_RF4CE_GDP_POLLING_TRIGGER_POLLING_ON_KEY_PRESS_ENABLED;
      }
    }

    // Polling time interval: this field shall be ignored [...] when the time
    // based polling enabled bit is set to 0.
    // Check that the max value is always >= than the min value.
    // Check that the min/max parameters from the client fall in the allowed
    // ranges.
    if (clientTriggersCapabilities & EMBER_AF_RF4CE_GDP_POLLING_TRIGGER_TIME_BASED_POLLING_ENABLED) {
      uint32_t minClientVal =
          emberFetchLowHighInt32u((uint8_t*)clientPollConstraints
                                  + POLL_CONSTRAINT_RECORD_MIN_POLLING_TIME_INTERVAL_OFFSET);
      uint32_t maxClientVal =
          emberFetchLowHighInt32u((uint8_t*)clientPollConstraints
                                  + POLL_CONSTRAINT_RECORD_MAX_POLLING_TIME_INTERVAL_OFFSET);

      if (minClientVal <= maxClientVal
          && minClientVal >= POLL_CONSTRAINT_RECORD_MIN_POLLING_TIME_INTERVAL_LOWER_BOUND
          && minClientVal <= POLL_CONSTRAINT_RECORD_MIN_POLLING_TIME_INTERVAL_UPPER_BOUND
          && maxClientVal >= POLL_CONSTRAINT_RECORD_MAX_POLLING_TIME_INTERVAL_LOWER_BOUND
          && maxClientVal <= POLL_CONSTRAINT_RECORD_MAX_POLLING_TIME_INTERVAL_UPPER_BOUND) {
        // We always select the minimum value supported by the client.
        clientPollConfiguration[POLL_CONFIGURATION_POLLING_TIME_INTERVAL_OFFSET]
                                = BYTE_0(minClientVal);
        clientPollConfiguration[POLL_CONFIGURATION_POLLING_TIME_INTERVAL_OFFSET + 1]
                                = BYTE_1(minClientVal);
        clientPollConfiguration[POLL_CONFIGURATION_POLLING_TIME_INTERVAL_OFFSET + 2]
                                = BYTE_2(minClientVal);
        clientPollConfiguration[POLL_CONFIGURATION_POLLING_TIME_INTERVAL_OFFSET + 3]
                                = BYTE_3(minClientVal);
      } else {
        // If the min/max parameters do not have legal values, remove the
        // time-based trigger as supported trigger.
        clientPollConfiguration[POLL_CONFIGURATION_POLLING_TRIGGER_CONFIG_OFFSET] &=
            ~EMBER_AF_RF4CE_GDP_POLLING_TRIGGER_TIME_BASED_POLLING_ENABLED;
      }
    }

    // Polling timeout: this value shall be smaller (or equal) than
    // aplcMaxPollingTimeout. We always set it to aplcMaxPollingTimeout.
    clientPollConfiguration[POLL_CONFIGURATION_POLLING_TIMEOUT_OFFSET] =
        APLC_MAX_POLLING_TIMEOUT_MS;

    // If no poll triggers are supported, just set the method back to DISABLED.
    if (clientPollConfiguration[POLL_CONFIGURATION_POLLING_TRIGGER_CONFIG_OFFSET] == 0x00) {
      MEMSET(clientPollConfiguration, 0x00, APL_GDP_POLL_CONFIGURATION_SIZE);
      clientPollConfiguration[POLL_CONFIGURATION_POLLING_METHOD_ID_OFFSET] =
          EMBER_AF_RF4CE_GDP_POLLING_METHOD_DISABLED;
    }
  }

  emAfRf4ceGdpSetRemoteAttribute(EMBER_AF_RF4CE_GDP_ATTRIBUTE_POLL_CONFIGURATION,
                                 0,
                                 clientPollConfiguration);
}

static void dispatchIncomingHeartbeat(EmberAfRf4ceGdpHeartbeatTrigger trigger);

void emAfRf4ceGdpPollingNotifyBindingCompletePollServerCallback(uint8_t pairingIndex)
{
#if defined(EMBER_SCRIPTED_TEST)
  if (!nodeIsPollingServer) {
    return;
  }
#endif

  if (emAfRf4ceGdpRemoteNodeAttributes.gdpCapabilities
      & GDP_CAPABILITIES_SUPPORT_POLL_CLIENT_BIT) {

    emAfRf4ceGdpSetPairingBindStatus(pairingIndex,
                                     (emAfRf4ceGdpGetPairingBindStatus(pairingIndex)
                                      | PAIRING_ENTRY_REMOTE_NODE_SUPPORTS_POLLING_BIT));
  }
}

bool emAfRf4ceGdpIncomingPushAttributesPollServerCallback(void)
{
#if defined(EMBER_SCRIPTED_TEST)
  if (!nodeIsPollingServer) {
    return false;
  }
#endif

  if (internalGdpState() == INTERNAL_STATE_NONE
      && pollingInfo.status == POLLING_STATUS_NONE) {
    EmberAfRf4ceGdpAttributeRecord record;

    while(emAfRf4ceGdpFetchAttributeRecord(&record)) {
      // If the poll server receives a PushAttributes command frame from the
      // poll client that contains the aplPollConstraint GDP attribute, the
      // poll server shall attempt to process it [...].
      // The poll server shall use the information in the aplPollConstraints
      // attribute to make a decision on the poll procedure that the poll client
      // shall use.
      if (record.attributeId == EMBER_AF_RF4CE_GDP_ATTRIBUTE_POLL_CONSTRAINTS
          && record.valueLength == APL_GDP_POLL_CONSTRAINTS_SIZE) {
        uint8_t pairingIndex = emberAfRf4ceGetPairingIndex();

        // Does the remote node support polling?
        if (emAfRf4ceGdpGetPairingBindStatus(pairingIndex)
            & PAIRING_ENTRY_REMOTE_NODE_SUPPORTS_POLLING_BIT) {

          selectClientPollProcedure(pairingIndex,
                                    record.value);
          pollingInfo.pairingIndex = emberAfRf4ceGetPairingIndex();
          pollingInfo.status = POLLING_STATUS_NEGOTIATION_IN_PROGRESS_SERVER;
          emberEventControlSetDelayMS(emberAfPluginRf4ceGdpPollingEventControl,
                                      POLLING_NEGOTIATION_PROCEDURE_SERVER_PULL_TIMEOUT_MSEC);
          return true;
        }
      }
    }
  }

  return false;
}

bool emAfRf4ceGdpIncomingPullAttributesPollServerCallback(void)
{
#if defined(EMBER_SCRIPTED_TEST)
  if (!nodeIsPollingServer) {
    return false;
  }
#endif

  if (internalGdpState() == INTERNAL_STATE_NONE
      && pollingInfo.status == POLLING_STATUS_NEGOTIATION_IN_PROGRESS_SERVER
      && pollingInfo.pairingIndex == emberAfRf4ceGetPairingIndex()) {
    EmberAfRf4ceGdpAttributeIdentificationRecord recordId;

    while (emAfRf4ceGdpFetchAttributeIdentificationRecord(&recordId)) {
      if (recordId.attributeId == EMBER_AF_RF4CE_GDP_ATTRIBUTE_POLL_CONFIGURATION) {
        uint8_t pollingMethod =
            emAfRf4ceGdpRemoteNodeAttributes.pollConfiguration[POLL_CONFIGURATION_POLLING_METHOD_ID_OFFSET];

        EmberAfRf4ceGdpPollingTrigger pollingTriggers =
            HIGH_LOW_TO_INT(emAfRf4ceGdpRemoteNodeAttributes.pollConfiguration[POLL_CONFIGURATION_POLLING_TRIGGER_CONFIG_OFFSET + 1],
                            emAfRf4ceGdpRemoteNodeAttributes.pollConfiguration[POLL_CONFIGURATION_POLLING_TRIGGER_CONFIG_OFFSET]);

        if (pollingMethod != EMBER_AF_RF4CE_GDP_POLLING_METHOD_DISABLED) {
          uint8_t bindStatusMask =
              emAfRf4ceGdpGetPairingBindStatus(pollingInfo.pairingIndex);

          emAfRf4ceGdpSetPairingBindStatus(pollingInfo.pairingIndex,
                                           (bindStatusMask
                                            | PAIRING_ENTRY_POLLING_ACTIVE_BIT));

          // Notify the application that a heartbeat polling has been
          // established.
          emberAfPluginRf4ceGdpHeartbeatPollingEstablishedCallback(pollingInfo.pairingIndex,
                                                                   pollingTriggers);
        }

        pollingInfo.status = POLLING_STATUS_NONE;
        emberEventControlSetInactive(emberAfPluginRf4ceGdpPollingEventControl);

        return true;
      }
    }
  }

  return false;
}

void emAfRf4ceGdpIncomingHeartbeatPollServerCallback(EmberAfRf4ceGdpHeartbeatTrigger trigger)
{
  // Check that the heartbeat comes from a poll client with which we
  // successfully performed the poll negotiation procedure.
  if (emAfRf4ceGdpGetPairingBindStatus(emberAfRf4ceGetPairingIndex())
      & PAIRING_ENTRY_POLLING_ACTIVE_BIT) {
    // Set the frame pending bit for the outgoing commands.
    emAfRf4ceGdpOutgoingCommandsSetPendingFlag();
    // Let the poll subscriber(s) send message(s) in response to the incoming
    // heartbeat.
    dispatchIncomingHeartbeat(trigger);

    // Always send a generic response at the end. This is not what the specs
    // dictates. However, in order to avoid having to deal with the frame
    // pending bit, we always set the frame pending in the messages sent out.
    // This last generic response will have the frame pending bit cleared.
    emAfRf4ceGdpOutgoingCommandsClearPendingFlag();
    emberAfRf4ceGdpGenericResponse(emberAfRf4ceGetPairingIndex(),
                                   EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE,
                                   EMBER_RF4CE_NULL_VENDOR_ID,
                                   EMBER_AF_RF4CE_GDP_RESPONSE_CODE_SUCCESSFUL);
  }
}

void emAfRf4ceGdpPollingServerEventHandler(void)
{
#if defined(EMBER_SCRIPTED_TEST)
  if (!nodeIsPollingServer) {
    return;
  }
#endif

  if (pollingInfo.status == POLLING_STATUS_NEGOTIATION_IN_PROGRESS_SERVER) {
    // Set the remote poll configuration attribute polling method "DISABLED".
    MEMSET(emAfRf4ceGdpRemoteNodeAttributes.pollConfiguration,
           0x00,
           APL_GDP_POLL_CONFIGURATION_SIZE);

    pollingInfo.status = POLLING_STATUS_NONE;
  }
}

// Incoming heartbeat dispatching mechanism. We maintain a list of callbacks.
// Different modules can subscribe to incoming heartbeats by using the
// emberAfRf4ceGdpSubscribeToHeartbeat() API.

// We support up to 5 subscribers.
static EmberAfRf4ceGdpHeartbeatCallback heartbeatCallbacks[] =
  {NULL, NULL, NULL, NULL, NULL};

static void dispatchIncomingHeartbeat(EmberAfRf4ceGdpHeartbeatTrigger trigger)
{
  uint8_t i;
  uint8_t pairingIndex = emberAfRf4ceGetPairingIndex();

  for (i = 0; i < COUNTOF(heartbeatCallbacks); i++) {
    if (heartbeatCallbacks[i]) {
      (*heartbeatCallbacks[i])(pairingIndex, trigger);
    }
  }
}

EmberStatus emberAfRf4ceGdpSubscribeToHeartbeat(EmberAfRf4ceGdpHeartbeatCallback callback)
{
  uint8_t i;
  for (i = 0; i < COUNTOF(heartbeatCallbacks); i++) {
    if (heartbeatCallbacks[i] == NULL) {
      heartbeatCallbacks[i] = callback;

      return EMBER_SUCCESS;
    }
  }

  return EMBER_TABLE_FULL;
}

#else // EMBER_AF_PLUGIN_RF4CE_GDP_IS_POLL_SERVER

EmberStatus emberAfRf4ceGdpSubscribeToHeartbeat(EmberAfRf4ceGdpHeartbeatCallback callback)
{
  return EMBER_INVALID_CALL;
}

#endif // EMBER_AF_PLUGIN_RF4CE_GDP_IS_POLL_SERVER
