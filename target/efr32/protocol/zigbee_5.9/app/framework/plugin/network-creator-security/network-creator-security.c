// Copyright 2015 Silicon Laboratories, Inc.

#include "app/framework/include/af.h"

#include "network-creator-security.h"

#include "app/framework/security/af-security.h" // emAfAllowTrustCenterRejoins
#include "app/util/zigbee-framework/zigbee-device-common.h" // emberLeaveRequest

#ifdef EZSP_HOST
  // NCP
  #define addTransientLinkKey        ezspAddTransientLinkKey
  #define setInitialSecurityState    ezspSetInitialSecurityState
  #define clearTransientLinkKeys     ezspClearTransientLinkKeys
  #define lookupNodeIdByEui64        ezspLookupNodeIdByEui64

  #define allowTrustCenterLinkKeyRequests()                           \
    ezspSetPolicy(EZSP_TC_KEY_REQUEST_POLICY, EZSP_GENERATE_NEW_TC_LINK_KEY)
  #define setTransientKeyTimeout(timeS)                               \
    ezspSetConfigurationValue(EZSP_CONFIG_TRANSIENT_KEY_TIMEOUT_S, (timeS));

  #define zigbeeKeyEstablishmentHandler ezspZigbeeKeyEstablishmentHandler
#else
  // SoC
  #define addTransientLinkKey        emberAddTransientLinkKey
  #define clearTransientLinkKeys     emberClearTransientLinkKeys
  #define setInitialSecurityState    emberSetInitialSecurityState
  #define lookupNodeIdByEui64        emberLookupNodeIdByEui64

  #define allowTrustCenterLinkKeyRequests()                           \
    emberTrustCenterLinkKeyRequestPolicy = EMBER_GENERATE_NEW_TC_LINK_KEY
  #define setTransientKeyTimeout(timeS)                               \
    emberTransientKeyTimeoutS = (timeS)

  #define zigbeeKeyEstablishmentHandler emberZigbeeKeyEstablishmentHandler
#endif

#define ZIGBEE_3_CENTRALIZED_SECURITY_LINK_KEY    \
  {                                               \
    {0x5A,0x69,0x67,0x42,0x65,0x65,0x41,0x6C,     \
     0x6C,0x69,0x61,0x6E,0x63,0x65,0x30,0x39}     \
  }

#define ZIGBEE_3_DISTRIBUTED_SECURITY_LINK_KEY    \
  {                                               \
    {0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,     \
     0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF}     \
  }

#ifndef EMBER_AF_PLUGIN_NETWORK_CREATOR_SECURITY_NETWORK_OPEN_TIME_S
  #define EMBER_AF_PLUGIN_NETWORK_CREATOR_SECURITY_NETWORK_OPEN_TIME_S (300)
#endif

#define NETWORK_OPEN_TIME_S (EMBER_AF_PLUGIN_NETWORK_CREATOR_SECURITY_NETWORK_OPEN_TIME_S)

// -----------------------------------------------------------------------------
// Internal Declarations

EmberEventControl emberAfPluginNetworkCreatorSecurityOpenNetworkEventControl;
#define openNetworkEventControl (emberAfPluginNetworkCreatorSecurityOpenNetworkEventControl)

static uint16_t openNetworkTimeRemainingS;

// -----------------------------------------------------------------------------
// Framework Callbacks

void emberAfPluginNetworkCreatorSecurityStackStatusCallback(EmberStatus status)
{
#ifdef EMBER_AF_PLUGIN_NETWORK_CREATOR_SECURITY_TRUST_CENTER_SUPPORT
  if (status == EMBER_NETWORK_UP
      && emberAfGetNodeId() == EMBER_TRUST_CENTER_NODE_ID) {
    EmberExtendedSecurityBitmask extended;

    // If we form a centralized network, make sure our trust center policy is
    // correctly set to respond to a key request.
    allowTrustCenterLinkKeyRequests();

    // This bit is not saved to a token, so make sure that our security bitmask
    // has this bit set on reboot.
    emberGetExtendedSecurityBitmask(&extended);
    extended |= EMBER_NWK_LEAVE_REQUEST_NOT_ALLOWED;
    emberSetExtendedSecurityBitmask(extended);

    // We can allow trust center rejoins, since our Z3 centralized network
    // has unique trust center link keys for all the devices.
    emAfAllowTrustCenterRejoins = true;
  }
#endif /* EMBER_AF_PLUGIN_NETWORK_CREATOR_SECURITY_TRUST_CENTER_SUPPORT */
}

// -----------------------------------------------------------------------------
// Stack Callbacks

#ifdef EMBER_AF_PLUGIN_NETWORK_CREATOR_SECURITY_TRUST_CENTER_SUPPORT

void zigbeeKeyEstablishmentHandler(EmberEUI64 eui64, EmberKeyStatus keyStatus)
{
  // If we are notified that a joining node failed to verify their
  // TCLK properly, then we are going to kick them off the network,
  // as they pose a potential security hazard.
  if (keyStatus == EMBER_TC_REQUESTER_VERIFY_KEY_TIMEOUT
      || keyStatus == EMBER_TC_REQUESTER_VERIFY_KEY_FAILURE) {
    EmberStatus status = EMBER_NOT_FOUND;
    EmberNodeId destinationId = lookupNodeIdByEui64(eui64);
    if (destinationId != EMBER_NULL_NODE_ID) {
      // We do not tell the node to rejoin, since they may be malicious.
      status = emberLeaveRequest(destinationId,
                                 eui64,
                                 0, // no leave flags
                                 (EMBER_APS_OPTION_RETRY
                                  | EMBER_APS_OPTION_ENABLE_ROUTE_DISCOVERY));
    }
    emberAfCorePrintln("%p: Remove node 0x%2X for failed key verification: 0x%X",
                       EMBER_AF_PLUGIN_NETWORK_CREATOR_SECURITY_PLUGIN_NAME,
                       destinationId,
                       status);
  }
}

#endif /* EMBER_AF_PLUGIN_NETWORK_CREATOR_SECURITY_TRUST_CENTER_SUPPORT */

// -----------------------------------------------------------------------------
// API

EmberStatus emberAfPluginNetworkCreatorSecurityStart(bool centralizedNetwork)
{
  EmberInitialSecurityState state;
  EmberExtendedSecurityBitmask extended;
  EmberStatus status = EMBER_SUCCESS;

  MEMSET(&state, 0, sizeof(state));
  state.bitmask = (EMBER_TRUST_CENTER_GLOBAL_LINK_KEY
                   | EMBER_HAVE_PRECONFIGURED_KEY
                   | EMBER_HAVE_NETWORK_KEY
                   | EMBER_NO_FRAME_COUNTER_RESET
                   | EMBER_REQUIRE_ENCRYPTED_KEY);

  extended = EMBER_JOINER_GLOBAL_LINK_KEY;

  if (!centralizedNetwork) {
    EmberKeyData distributedKey = ZIGBEE_3_DISTRIBUTED_SECURITY_LINK_KEY;

    // Use the distributed global link key.
    MEMMOVE(&(state.preconfiguredKey),
            emberKeyContents(&distributedKey),
            EMBER_ENCRYPTION_KEY_SIZE);

    // Use distributed trust center mode.
    state.bitmask |= EMBER_DISTRIBUTED_TRUST_CENTER_MODE;
  }
#if defined(EMBER_AF_PLUGIN_NETWORK_CREATOR_SECURITY_TRUST_CENTER_SUPPORT) \
    || defined(EMBER_AF_PLUGIN_TEST_HARNESS_Z3)
  else { // centralizedNetwork
    // Generate a random global link key.
    // This is the key the trust center will send to a joining node when it
    // updates its link key.
    status = emberAfGenerateRandomKey(&(state.preconfiguredKey));
    if (status != EMBER_SUCCESS) {
      goto kickout;
    }

    // Use hashed link keys for improved storage and speed.
    state.bitmask |= EMBER_TRUST_CENTER_USES_HASHED_LINK_KEY;

    // Tell the trust center to ignore leave requests.
    extended |= EMBER_NWK_LEAVE_REQUEST_NOT_ALLOWED;

    // We can allow trust center rejoins, since our Z3 centralized network
    // has unique trust center link keys for all the devices.
    emAfAllowTrustCenterRejoins = true;
  }
#endif /* defined(EMBER_AF_PLUGIN_NETWORK_CREATOR_SECURITY_TRUST_CENTER_SUPPORT) \
          || defined(EMBER_AF_PLUGIN_TEST_HARNESS_Z3) */

  // Generate a random network key.
  status = emberAfGenerateRandomKey(&(state.networkKey));
  if (status != EMBER_SUCCESS) {
    goto kickout;
  }

  // Set the initial security data.
  status = setInitialSecurityState(&state);
  if (status != EMBER_SUCCESS) {
    goto kickout;
  }
  status = emberSetExtendedSecurityBitmask(extended);

 kickout:
  emberAfCorePrintln("%p: %p: 0x%X",
                     EMBER_AF_PLUGIN_NETWORK_CREATOR_SECURITY_PLUGIN_NAME,
                     "Start",
                     status);
  return status;
}

EmberStatus emberAfPluginNetworkCreatorSecurityOpenNetwork(void)
{
  if (emberAfNetworkState() != EMBER_JOINED_NETWORK) {
    return EMBER_ERR_FATAL;
  }

  openNetworkTimeRemainingS = NETWORK_OPEN_TIME_S;
  setTransientKeyTimeout(openNetworkTimeRemainingS);
  emberEventControlSetActive(openNetworkEventControl);

  return EMBER_SUCCESS;
}

EmberStatus emberAfPluginNetworkCreatorSecurityCloseNetwork(void)
{
  EmberStatus status = EMBER_ERR_FATAL;

  if (emberAfNetworkState() == EMBER_JOINED_NETWORK) {
    clearTransientLinkKeys();
    emberEventControlSetInactive(openNetworkEventControl);
    status = emberAfPermitJoin(0, true); // broadcast
  }

  return status;
}

// -----------------------------------------------------------------------------
// Internal Definitions

void emberAfPluginNetworkCreatorSecurityOpenNetworkEventHandler(void)
{
  EmberStatus status = EMBER_SUCCESS;
  uint8_t permitJoinTime;

  emberEventControlSetInactive(openNetworkEventControl);

  // If we have left the network, then we don't need to proceed further.
  if (emberAfNetworkState() != EMBER_JOINED_NETWORK) {
    return;
  }

#if defined(EMBER_AF_PLUGIN_NETWORK_CREATOR_SECURITY_TRUST_CENTER_SUPPORT) \
    || defined(EMBER_AF_PLUGIN_TEST_HARNESS_Z3)
  if (emberAfGetNodeId() == EMBER_TRUST_CENTER_NODE_ID
      && openNetworkTimeRemainingS == NETWORK_OPEN_TIME_S) {
    EmberEUI64 wildcardEui64 = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,};
    EmberKeyData centralizedKey = ZIGBEE_3_CENTRALIZED_SECURITY_LINK_KEY;
    status = addTransientLinkKey(wildcardEui64, &centralizedKey);
  }
#endif /* defined(EMBER_AF_PLUGIN_NETWORK_CREATOR_SECURITY_TRUST_CENTER_SUPPORT) \
          || defined(EMBER_AF_PLUGIN_TEST_HARNESS_Z3) */

  if (openNetworkTimeRemainingS > EMBER_AF_PERMIT_JOIN_MAX_TIMEOUT) {
    permitJoinTime = EMBER_AF_PERMIT_JOIN_MAX_TIMEOUT;
    openNetworkTimeRemainingS -= EMBER_AF_PERMIT_JOIN_MAX_TIMEOUT;
    emberEventControlSetDelayQS(openNetworkEventControl,
                                EMBER_AF_PERMIT_JOIN_MAX_TIMEOUT << 2);
  } else {
    permitJoinTime = openNetworkTimeRemainingS;
    openNetworkTimeRemainingS = 0;
  }

  if (status == EMBER_SUCCESS) {
    status = emberAfPermitJoin(permitJoinTime, true); // broadcast permit join
  }

  emberAfCorePrintln("%p: %p: 0x%X",
                     EMBER_AF_PLUGIN_NETWORK_CREATOR_SECURITY_PLUGIN_NAME,
                     "Open network",
                     status);

}
