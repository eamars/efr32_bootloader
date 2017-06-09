// *****************************************************************************
// * end-device-move.c
// *
// * Code common to SOC and host to handle moving (i.e. rejoining) to a new
// * parent device.
// *
// * Copyright 2012 by Ember Corporation. All rights reserved.              *80*
// *****************************************************************************

#include "app/framework/include/af.h"

// *****************************************************************************
// Globals

#if defined(EMBER_SCRIPTED_TEST)
  uint8_t emAfRejoinAttemptsMax = 3;
  #define EMBER_AF_REJOIN_ATTEMPTS_MAX emAfRejoinAttemptsMax
#endif  

extern EmberEventControl emberAfPluginEndDeviceSupportMoveNetworkEventControls[];


typedef struct {
  uint8_t moveAttempts;
  uint16_t totalMoveAttempts;
} State;
static State states[EMBER_SUPPORTED_NETWORKS];

#define NEVER_STOP_ATTEMPTING_REJOIN 0xFF
#define MOVE_DELAY_QS (10 * 4)

// *****************************************************************************
// Functions

static void scheduleMoveEvent(void)
{
  uint8_t networkIndex = emberGetCurrentNetwork();
  State *state = &states[networkIndex];

  if (EMBER_AF_REJOIN_ATTEMPTS_MAX == NEVER_STOP_ATTEMPTING_REJOIN
      || state->moveAttempts < EMBER_AF_REJOIN_ATTEMPTS_MAX) {
    emberAfAppPrintln("Schedule move nwk %d: %d",
                      networkIndex,
                      state->moveAttempts);
    emberAfNetworkEventControlSetDelayQS(emberAfPluginEndDeviceSupportMoveNetworkEventControls,
                                         (state->moveAttempts == 0
                                          ? 0
                                          : MOVE_DELAY_QS));
  } else {
    emberAfAppPrintln("Max move limit reached nwk %d: %d",
                      networkIndex,
                      state->moveAttempts);
    emberAfStopMoveCallback();
  }
}

bool emberAfMoveInProgressCallback(void)
{
  return emberAfNetworkEventControlGetActive(emberAfPluginEndDeviceSupportMoveNetworkEventControls);
}

bool emberAfStartMoveCallback(void)
{
  if (!emberAfMoveInProgressCallback()) {
    scheduleMoveEvent();
    return true;
  }
  return false;
}

void emberAfStopMoveCallback(void)
{
  uint8_t networkIndex = emberGetCurrentNetwork();
  states[networkIndex].moveAttempts = 0;
  emberEventControlSetInactive(emberAfPluginEndDeviceSupportMoveNetworkEventControls[networkIndex]);
}

static bool checkForWellKnownTrustCenterLinkKey(void)
{
#if !defined(EMBER_AF_PLUGIN_END_DEVICE_SUPPORT_ALLOW_REJOINS_WITH_WELL_KNOWN_LINK_KEY) 

  EmberKeyStruct keyStruct;
  EmberStatus status = emberGetKey(EMBER_TRUST_CENTER_LINK_KEY, &keyStruct);

  const EmberKeyData smartEnergyWellKnownTestKey = SE_SECURITY_TEST_LINK_KEY;
  const EmberKeyData zigbeeAlliance09Key = ZIGBEE_PROFILE_INTEROPERABILITY_LINK_KEY;

  if (status != EMBER_SUCCESS) {
    // Assume by default we have a well-known key if we failed to retrieve it.
    // This will prevent soliciting a TC rejoin that might expose the network
    // key such that a passive attacker can obtain the key.  Better to be
    // conservative in this circumstance.
    return true;
  }

  if ((0 == MEMCOMPARE(emberKeyContents(&(keyStruct.key)), 
                       emberKeyContents(&(smartEnergyWellKnownTestKey)),
                       EMBER_ENCRYPTION_KEY_SIZE))
      || (0 == MEMCOMPARE(emberKeyContents(&(keyStruct.key)),
                          emberKeyContents(&(zigbeeAlliance09Key)),
                          EMBER_ENCRYPTION_KEY_SIZE))) {
    return true;
  }
#endif

  return false;
}

void emberAfPluginEndDeviceSupportMoveNetworkEventHandler(void)
{
  uint8_t networkIndex = emberGetCurrentNetwork();
  State *state = &states[networkIndex];
  EmberStatus status;
  bool secure = (state->moveAttempts == 0);
  if (!secure && checkForWellKnownTrustCenterLinkKey()) {
    emberAfDebugPrintln("Will not attempt TC rejoin due to well-known key.");
    secure = true;
  }
  uint32_t channels = (state->moveAttempts == 0
                     ? 0 // current channel
                     : EMBER_ALL_802_15_4_CHANNELS_MASK);
  status = emberFindAndRejoinNetworkWithReason(secure, 
                                               channels,
                                               EMBER_AF_REJOIN_DUE_TO_END_DEVICE_MOVE);
  emberAfDebugPrintln("Move attempt %d nwk %d: 0x%x",
                      state->moveAttempts,
                      networkIndex,
                      status);
  if (status == EMBER_SUCCESS) {
    state->moveAttempts++;
    state->totalMoveAttempts++;
  } else {
    scheduleMoveEvent();
  }
}

void emberAfPluginEndDeviceSupportStackStatusCallback(EmberStatus status)
{
  if (status == EMBER_NETWORK_UP) {
    emberAfStopMoveCallback();
    return;
  }

  if (!emberStackIsPerformingRejoin()) {
    EmberNetworkStatus state = emberAfNetworkState();
    if (state == EMBER_JOINED_NETWORK_NO_PARENT) {
      emberAfStartMoveCallback();
    } else if (state == EMBER_NO_NETWORK) {
      emberAfStopMoveCallback();
    }
  }
}
