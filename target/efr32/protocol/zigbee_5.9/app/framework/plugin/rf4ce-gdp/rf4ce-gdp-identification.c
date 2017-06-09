// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"

#include "app/framework/plugin/rf4ce-profile/rf4ce-profile.h"
#include "rf4ce-gdp.h"
#include "rf4ce-gdp-attributes.h"
#include "rf4ce-gdp-identification.h"
#include "rf4ce-gdp-internal.h"

#if defined(EMBER_SCRIPTED_TEST)
#include "rf4ce-gdp-test.h"
#endif // EMBER_SCRIPTED_TEST

//------------------------------------------------------------------------------
// Common code (identification=NONE, identification=CLIENT or identification=SERVER).

#if defined(EMBER_SCRIPTED_TEST)
bool nodeIsIdentificationServer = true;
#endif

// Identification event (for now is common to both the client and server code).
EmberEventControl emberAfPluginRf4ceGdpIdentificationEventControl;

// Forward declarations
void emAfRf4ceGdpNotifyBindingCompleteIdentificationClientCallback(uint8_t pairingIndex);
void emAfRf4ceGdpNotifyBindingCompleteIdentificationServerCallback(uint8_t pairingIndex);
void emAfRf4ceGdpIdentificationClientEventHandler(void);
void emAfRf4ceGdpIdentificationServerEventHandler(void);
void emAfPendingCommandEventHandlerIdentificationClient(void);
void emAfRf4ceGdpIncomingGenericResponseIdentificationClientCallback(EmberAfRf4ceGdpResponseCode responseCode);
bool emAfRf4ceGdpIncomingPushAttributesIdentificationServerCallback(void);
EmberStatus emAfPluginRf4ceGdpIdentifyClient(uint8_t pairingIndex);
void emAfRf4ceGdpIncomingIdentifyCallbackIdentificationClient(EmberAfRf4ceGdpClientNotificationIdentifyFlags flags,
                                                              uint16_t timeS);

void emberAfPluginRf4ceGdpIdentificationEventHandler(void)
{
  emberEventControlSetInactive(emberAfPluginRf4ceGdpIdentificationEventControl);

#if (defined(EMBER_AF_PLUGIN_RF4CE_GDP_IS_IDENTIFICATION_CLIENT) || defined(EMBER_SCRIPTED_TEST))
  emAfRf4ceGdpIdentificationClientEventHandler();
#endif

#if (defined(EMBER_AF_PLUGIN_RF4CE_GDP_IS_IDENTIFICATION_SERVER) || defined(EMBER_SCRIPTED_TEST))
  emAfRf4ceGdpIdentificationServerEventHandler();
#endif
}

void emAfPendingCommandEventHandlerIdentification(void)
{
#if (defined(EMBER_AF_PLUGIN_RF4CE_GDP_IS_IDENTIFICATION_CLIENT) || defined(EMBER_SCRIPTED_TEST))
  emAfPendingCommandEventHandlerIdentificationClient();
#endif
}

void emAfRf4ceGdpIdentificationNotifyBindingComplete(uint8_t pairingIndex)
{
#if (defined(EMBER_AF_PLUGIN_RF4CE_GDP_IS_IDENTIFICATION_CLIENT) || defined(EMBER_SCRIPTED_TEST))
  emAfRf4ceGdpNotifyBindingCompleteIdentificationClientCallback(pairingIndex);
#endif

#if (defined(EMBER_AF_PLUGIN_RF4CE_GDP_IS_IDENTIFICATION_SERVER) || defined(EMBER_SCRIPTED_TEST))
  emAfRf4ceGdpNotifyBindingCompleteIdentificationServerCallback(pairingIndex);
#endif
}

void emAfRf4ceGdpIncomingIdentifyCallback(EmberAfRf4ceGdpClientNotificationIdentifyFlags flags,
                                          uint16_t timeS)
{
#if (defined(EMBER_AF_PLUGIN_RF4CE_GDP_IS_IDENTIFICATION_CLIENT) || defined(EMBER_SCRIPTED_TEST))
  emAfRf4ceGdpIncomingIdentifyCallbackIdentificationClient(flags, timeS);
#endif
}

bool emAfRf4ceGdpIncomingPushAttributesIdentificationCallback(void)
{
#if (defined(EMBER_AF_PLUGIN_RF4CE_GDP_IS_IDENTIFICATION_SERVER) || defined(EMBER_SCRIPTED_TEST))
  return emAfRf4ceGdpIncomingPushAttributesIdentificationServerCallback();
#else
  return false;
#endif
}

void emAfRf4ceGdpIncomingGenericResponseIdentificationCallback(EmberAfRf4ceGdpResponseCode responseCode)
{
#if (defined(EMBER_AF_PLUGIN_RF4CE_GDP_IS_IDENTIFICATION_CLIENT) || defined(EMBER_SCRIPTED_TEST))
  emAfRf4ceGdpIncomingGenericResponseIdentificationClientCallback(responseCode);
#endif
}

//------------------------------------------------------------------------------
// Identification Client

#if (defined(EMBER_AF_PLUGIN_RF4CE_GDP_IS_IDENTIFICATION_CLIENT) || defined(EMBER_SCRIPTED_TEST))

// Forward declaractions
static void startIdentificationProcedure(void);
static void finishIdentificationProcedure(bool success);

static struct {
  uint8_t pairingIndex;
  uint8_t retries;
} identificationInfo;

void emAfRf4ceGdpIdentificationClientEventHandler(void)
{
#if defined(EMBER_SCRIPTED_TEST)
  if (nodeIsIdentificationServer) {
    return;
  }
#endif

  // Check that the binding status for the saved pairing index is still 'bound'.
  if ((emAfRf4ceGdpGetPairingBindStatus(identificationInfo.pairingIndex)
       & PAIRING_ENTRY_BINDING_STATUS_MASK)
      != PAIRING_ENTRY_BINDING_STATUS_NOT_BOUND) {
    // If the internal state is in the initial state, start the identification
    // procedure, otherwise re-schedule the event and try again later on.
    if (internalGdpState() == INTERNAL_STATE_NONE) {
      startIdentificationProcedure();
    } else {
      emberEventControlSetDelayMS(emberAfPluginRf4ceGdpIdentificationEventControl,
                                  IDENTIFICATION_PROCEDURE_DELAY_MSEC);
    }
  }
}

void emAfPendingCommandEventHandlerIdentificationClient(void)
{
#if defined(EMBER_SCRIPTED_TEST)
  if (nodeIsIdentificationServer) {
    return;
  }
#endif

  // If the Generic Response command frame is not received from the
  // Identification Server [...] the Identification Client shall consider the
  // identification capabilities announcement procedure a failure and the
  // procedure shall be terminated.
  finishIdentificationProcedure(false);
}

void emAfRf4ceGdpIncomingGenericResponseIdentificationClientCallback(EmberAfRf4ceGdpResponseCode responseCode)
{
  // [...] if the response code of the Generic Response command frame is not
  // indicating success, the Identification Client shall consider the
  // identification capabilities announcement procedure a failure and the
  // procedure shall be terminated.
  finishIdentificationProcedure(responseCode
                                == EMBER_AF_RF4CE_GDP_RESPONSE_CODE_SUCCESSFUL);
}

void emAfRf4ceGdpNotifyBindingCompleteIdentificationClientCallback(uint8_t pairingIndex)
{
#if defined(EMBER_SCRIPTED_TEST)
  if (nodeIsIdentificationServer) {
    return;
  }
#endif

  // If the remote node is an identification server, we schedule the
  // identification procedure. We also remember that the remote supports
  // identification in the persistent pairing status bitmask.
  if (emAfRf4ceGdpRemoteNodeAttributes.gdpCapabilities
      & GDP_CAPABILITIES_SUPPORT_IDENTIFICATION_SERVER_BIT) {
    identificationInfo.pairingIndex = pairingIndex;
    identificationInfo.retries = 0;

    emAfRf4ceGdpSetPairingBindStatus(pairingIndex,
                                     (emAfRf4ceGdpGetPairingBindStatus(pairingIndex)
                                      | PAIRING_ENTRY_REMOTE_NODE_SUPPORTS_IDENTIFICATION_BIT));

    // Observe blackout period prior to kicking off the identification
    // configuration procedure.
    emberEventControlSetDelayMS(emberAfPluginRf4ceGdpIdentificationEventControl,
                                APLC_CONFIG_BLACKOUT_TIME_MS);
  }
}

void emAfRf4ceGdpIncomingIdentifyCallbackIdentificationClient(EmberAfRf4ceGdpClientNotificationIdentifyFlags flags,
                                                              uint16_t timeS)
{
  uint8_t bindStatusMask =
      emAfRf4ceGdpGetPairingBindStatus(emberAfRf4ceGetPairingIndex());

  // Call the public callback only if:
  // - identification is active on the pairing entry we received the message from.
  // - The node supports at least one of the identification actions requested
  //   by the server.
  if (bindStatusMask & PAIRING_ENTRY_IDENTIFICATION_ACTIVE_BIT
      && (flags & EMBER_AF_PLUGIN_RF4CE_GDP_IDENTIFICATION_CAPABILITIES)) {
    emberAfPluginRf4ceGdpIdentifyCallback(flags, timeS);
  }
}

// After the binding is successfully completed, the Identification Client shall
// inform the Identification  Server about the identification capabilities it
// supports. To do this, the Identification Client shall generate and transmit a
// Push Attributes command frame, containing the aplIdentificationCapabilities
// GDP attribute, to the Identification Server.
static void startIdentificationProcedure(void)
{
  EmberAfRf4ceGdpAttributeRecord record;

  record.attributeId = EMBER_AF_RF4CE_GDP_ATTRIBUTE_IDENTIFICATION_CAPABILITIES;
  record.valueLength = APL_GDP_IDENTIFICATION_CAPABILITIES_SIZE;
  record.value = &emAfRf4ceGdpLocalNodeAttributes.identificationCapabilities;

  if (emberAfRf4ceGdpPushAttributes(identificationInfo.pairingIndex,
                                    EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE,
                                    EMBER_RF4CE_NULL_VENDOR_ID,
                                    &record,
                                    1) == EMBER_SUCCESS) {
    identificationInfo.retries++;
    emAfGdpStartCommandPendingTimer(INTERNAL_STATE_GDP_IDENTIFICATION_CLIENT_PUSH_PENDING,
                                    APLC_MAX_CONFIG_WAIT_TIME_MS);
  }
}

static void finishIdentificationProcedure(bool success)
{
  setInternalState(INTERNAL_STATE_NONE);

  // Cancel the command and identification events.
  emberEventControlSetInactive(emberAfPluginRf4ceGdpPendingCommandEventControl);
  emberEventControlSetInactive(emberAfPluginRf4ceGdpIdentificationEventControl);

  if (success) {
    uint8_t bindStatusMask =
        emAfRf4ceGdpGetPairingBindStatus(identificationInfo.pairingIndex);

    emAfRf4ceGdpSetPairingBindStatus(identificationInfo.pairingIndex,
                                     (bindStatusMask
                                      | PAIRING_ENTRY_IDENTIFICATION_ACTIVE_BIT));

    debugScriptCheck("Identification procedure succeeded");
  } else {
    debugScriptCheck("Identification procedure failed");

    // If the identification capabilities announcement procedure fails, the
    // Identification Client should retry the identification capabilities
    // announcement procedure later.
    // In our implementation we have a maximum number of retries, after which
    // the negotiation procedure won't be restarted anymore.
    if (identificationInfo.retries < IDENTIFICATION_PROCEDURE_CLIENT_MAX_RETIRES) {
      debugScriptCheck("Identification: retry procedure after delay");
      emberEventControlSetDelayQS(emberAfPluginRf4ceGdpIdentificationEventControl,
                                  IDENTIFICATION_PROCEDURE_AFTER_FAILURE_DELAY_SEC*4);
    }
#if defined(EMBER_SCRIPTED_TEST)
    if (identificationInfo.retries == IDENTIFICATION_PROCEDURE_CLIENT_MAX_RETIRES) {
      debugScriptCheck("Identification: reached maximum retries, give up");
    }
#endif
  }
}

#endif // EMBER_AF_PLUGIN_RF4CE_GDP_IS_IDENTIFICATION_CLIENT

//------------------------------------------------------------------------------
// Identification Server

#if (defined(EMBER_AF_PLUGIN_RF4CE_GDP_IS_IDENTIFICATION_SERVER) || defined(EMBER_SCRIPTED_TEST))

void emAfRf4ceGdpNotifyBindingCompleteIdentificationServerCallback(uint8_t pairingIndex)
{
#if defined(EMBER_SCRIPTED_TEST)
  if (!nodeIsIdentificationServer) {
    return;
  }
#endif

  if (emAfRf4ceGdpRemoteNodeAttributes.gdpCapabilities
      & GDP_CAPABILITIES_SUPPORT_IDENTIFICATION_CLIENT_BIT) {

    emAfRf4ceGdpSetPairingBindStatus(pairingIndex,
                                     (emAfRf4ceGdpGetPairingBindStatus(pairingIndex)
                                      | PAIRING_ENTRY_REMOTE_NODE_SUPPORTS_IDENTIFICATION_BIT));
  }
}

void emAfRf4ceGdpIdentificationServerEventHandler(void)
{
#if defined(EMBER_SCRIPTED_TEST)
  if (!nodeIsIdentificationServer) {
    return;
  }
#endif

  // TODO
}

bool emAfRf4ceGdpIncomingPushAttributesIdentificationServerCallback(void)
{
  uint8_t pairingIndex = emberAfRf4ceGetPairingIndex();

#if defined(EMBER_SCRIPTED_TEST)
  if (!nodeIsIdentificationServer) {
    return false;
  }
#endif

  if (internalGdpState() == INTERNAL_STATE_NONE) {
    EmberAfRf4ceGdpAttributeRecord record;

    while(emAfRf4ceGdpFetchAttributeRecord(&record)) {
      // If the poll server receives a PushAttributes command frame from the
      // poll client that contains the aplPollConstraint GDP attribute, the
      // poll server shall attempt to process it [...].
      // The poll server shall use the information in the aplPollConstraints
      // attribute to make a decision on the poll procedure that the poll client
      // shall use.
      // Check that the identification capabilities attributes has at least one
      // supported method bit set.
      if (record.attributeId == EMBER_AF_RF4CE_GDP_ATTRIBUTE_IDENTIFICATION_CAPABILITIES
          && record.valueLength == APL_GDP_IDENTIFICATION_CAPABILITIES_SIZE
          && (*record.value & ~IDENTIFICATION_CAPABILITIES_RESERVED_MASK)) {

        uint8_t bindStatusMask =
            emAfRf4ceGdpGetPairingBindStatus(pairingIndex);

        // Does the remote node support identification?
        if (bindStatusMask
            & PAIRING_ENTRY_REMOTE_NODE_SUPPORTS_IDENTIFICATION_BIT) {

          emAfRf4ceGdpSetPairingBindStatus(pairingIndex,
                                           (bindStatusMask
                                            | PAIRING_ENTRY_IDENTIFICATION_ACTIVE_BIT));

          // Notify the application that an identify client has been discovered.
          emberAfPluginRf4ceGdpIdentifyClientFoundCallback(*record.value & ~IDENTIFICATION_CAPABILITIES_RESERVED_MASK);

          debugScriptCheck("Identification procedure succeeded");

          return true;
        }
      }
    }
  }

  return false;
}

#endif // EMBER_AF_PLUGIN_RF4CE_GDP_IS_IDENTIFICATION_SERVER
