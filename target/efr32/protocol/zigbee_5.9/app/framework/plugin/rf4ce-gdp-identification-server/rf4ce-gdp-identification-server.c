// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "app/framework/plugin/rf4ce-profile/rf4ce-profile.h"
#include "app/framework/plugin/rf4ce-gdp/rf4ce-gdp.h"
#include "app/framework/plugin/rf4ce-gdp/rf4ce-gdp-internal.h"

#ifndef EMBER_AF_PLUGIN_RF4CE_GDP_IS_IDENTIFICATION_SERVER
  #error The RF4CE GDP Identification Server plugin can only be used on devices configured as identification servers.
#endif

// If we are not a poll server, we will send directly to the identification
// client and therefore do not need to keep state about the clients.
#ifdef EMBER_AF_PLUGIN_RF4CE_GDP_IS_POLL_SERVER
typedef struct {
  EmberAfRf4ceGdpClientNotificationIdentifyFlags flags;
  uint16_t timeS;
} Client;
static Client clients[EMBER_RF4CE_PAIRING_TABLE_SIZE];
#endif

static void heartbeatCallback(uint8_t pairingIndex,
                              EmberAfRf4ceGdpHeartbeatTrigger trigger);

void emberAfPluginRf4ceGdpIdentificationServerInitCallback(void)
{
#ifdef EMBER_AF_PLUGIN_RF4CE_GDP_IS_POLL_SERVER
  assert(emberAfRf4ceGdpSubscribeToHeartbeat(heartbeatCallback)
         == EMBER_SUCCESS);
#endif
}

EmberStatus emberAfRf4ceGdpIdentificationServerIdentify(uint8_t pairingIndex,
                                                        EmberAfRf4ceGdpClientNotificationIdentifyFlags flags,
                                                        uint16_t timeS)
{
  uint8_t status = emAfRf4ceGdpGetPairingBindStatus(pairingIndex);
  bool identificationClient = READBITS(status,
                                          PAIRING_ENTRY_IDENTIFICATION_ACTIVE_BIT);
  if (identificationClient) {
#ifdef EMBER_AF_PLUGIN_RF4CE_GDP_IS_POLL_SERVER
    // If we are a poll server and the identification client is also a poll
    // client, we have to wait for a heartbeat before we can send the identify
    // command.  Otherwise, we can just send the message right away.
    bool pollClient = READBITS(status, PAIRING_ENTRY_POLLING_ACTIVE_BIT);
    if (pollClient) {
      clients[pairingIndex].flags = flags;
      clients[pairingIndex].timeS = timeS;
      return EMBER_SUCCESS;
    }
#endif
    return emberAfRf4ceGdpClientNotificationIdentify(pairingIndex,
                                                     EMBER_RF4CE_NULL_VENDOR_ID,
                                                     flags,
                                                     timeS);
  }
  return EMBER_INVALID_CALL;
}

static void heartbeatCallback(uint8_t pairingIndex,
                              EmberAfRf4ceGdpHeartbeatTrigger trigger)
{
#ifdef EMBER_AF_PLUGIN_RF4CE_GDP_IS_POLL_SERVER
  if (clients[pairingIndex].flags != 0) {
    EmberStatus status = emberAfRf4ceGdpClientNotificationIdentify(pairingIndex,
                                                                   EMBER_RF4CE_NULL_VENDOR_ID,
                                                                   clients[pairingIndex].flags,
                                                                   clients[pairingIndex].timeS);
    if (status == EMBER_SUCCESS) {
      clients[pairingIndex].flags = 0;
    }
  }
#endif
}
