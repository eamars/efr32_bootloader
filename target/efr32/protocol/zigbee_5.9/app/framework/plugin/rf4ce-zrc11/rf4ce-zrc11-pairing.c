// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "app/framework/plugin/rf4ce-profile/rf4ce-profile.h"
#include "rf4ce-zrc11.h"
#include "rf4ce-zrc11-internal.h"

#ifdef EMBER_AF_RF4CE_ZRC_IS_ORIGINATOR
  // This plugin assumes that only one remote node will be discovered and
  // paired to during push-button pairing.  This restriction is in line with
  // the CERC and ZRC 1.1 profile specifications.  If this requirement changes,
  // the plugin would presumably need an array of node description and then
  // attempt to pair with each sequentially.
  #if NWK_MAX_REPORTED_NODE_DESCRIPTORS != 1
    #error The ZigBee Remote Control 1.1 plugin supports exactly one discovered node descriptor
  #endif
#endif

// Internal pairing state.
enum {
  INITIAL       = 0,
  ORG_DISCOVERY = 1,
  ORG_PAIRING   = 2,
  RCP_DISCOVERY = 3,
  RCP_PAIRING   = 4,
};

// Internal node descriptor struct.
typedef struct {
#ifdef EMBER_AF_RF4CE_ZRC_IS_ORIGINATOR
  uint8_t channel;
  EmberPanId panId;
#endif
  EmberEUI64 ieeeAddr;
  EmberRf4ceVendorInfo vendorInfo;
  EmberRf4ceApplicationInfo appInfo;
} NodeDescriptor;

static uint8_t state = INITIAL;

#ifdef EMBER_AF_RF4CE_ZRC_IS_ORIGINATOR
static NodeDescriptor recipient;
static uint8_t recipientCount;
#endif

#ifdef EMBER_AF_RF4CE_ZRC_IS_RECIPIENT
static NodeDescriptor originator;
#endif

EmberEventControl emberAfPluginRf4ceZrc11PairingEventControl;

//------------------------------------------------------------------------------
// Application framework APIs.

EmberStatus emberAfRf4ceZrc11Discovery(EmberPanId panId,
                                       EmberNodeId nodeId,
                                       EmberAfRf4ceDeviceType searchDevType)
{
  EmberStatus status = EMBER_INVALID_CALL;
#ifdef EMBER_AF_RF4CE_ZRC_IS_ORIGINATOR
  if (state == INITIAL) {
    uint8_t profileIdList[] = {
      EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_1_1,
    };
    status = emberAfRf4ceDiscovery(panId,
                                   nodeId,
                                   searchDevType,
                                   APLC_DISCOVERY_DURATION_MS,
                                   NWK_MAX_DISCOVERY_REPETITIONS,
                                   COUNTOF(profileIdList),
                                   profileIdList);
    if (status == EMBER_SUCCESS) {
      state = ORG_DISCOVERY;
      recipientCount = 0;
    }
  }
#endif
  return status;
}

//------------------------------------------------------------------------------
// Plugin callbacks.

bool emberAfPluginRf4ceProfileRemoteControl11DiscoveryRequestCallback(const EmberEUI64 ieeeAddr,
                                                                         uint8_t nodeCapabilities,
                                                                         const EmberRf4ceVendorInfo *vendorInfo,
                                                                         const EmberRf4ceApplicationInfo *appInfo,
                                                                         uint8_t searchDevType,
                                                                         uint8_t rxLinkQuality)
{
  // Originators and recipients do not respond to discovery requests at the
  // application level.  During auto discovery, the stack will automatically
  // handle responses for recipients.
  return false;
}

bool emberAfPluginRf4ceProfileRemoteControl11DiscoveryResponseCallback(bool atCapacity,
                                                                          uint8_t channel,
                                                                          EmberPanId panId,
                                                                          const EmberEUI64 ieeeAddr,
                                                                          uint8_t nodeCapabilities,
                                                                          const EmberRf4ceVendorInfo *vendorInfo,
                                                                          const EmberRf4ceApplicationInfo *appInfo,
                                                                          uint8_t rxLinkQuality,
                                                                          uint8_t discRequestLqi)
{
#ifdef EMBER_AF_RF4CE_ZRC_IS_ORIGINATOR
  if (state == ORG_DISCOVERY) {
    recipient.channel = channel;
    recipient.panId = panId;
    MEMMOVE(recipient.ieeeAddr, ieeeAddr, EUI64_SIZE);
    MEMMOVE(&recipient.vendorInfo, vendorInfo, sizeof(EmberRf4ceVendorInfo));
    MEMMOVE(&recipient.appInfo, appInfo, sizeof(EmberRf4ceApplicationInfo));
    recipientCount++;
    return (recipientCount < NWK_MAX_REPORTED_NODE_DESCRIPTORS);
  }
#endif
  return false;
}

void emberAfPluginRf4ceProfileRemoteControl11DiscoveryCompleteCallback(EmberStatus status)
{
#ifdef EMBER_AF_RF4CE_ZRC_IS_ORIGINATOR
  if (state == ORG_DISCOVERY) {
    if (status == EMBER_SUCCESS) {
      if (recipientCount == 0) {
        status = EMBER_DISCOVERY_TIMEOUT;
      } else if (recipientCount <= NWK_MAX_REPORTED_NODE_DESCRIPTORS) {
        status = emberAfRf4cePair(recipient.channel,
                                  recipient.panId,
                                  recipient.ieeeAddr,
                                  EMBER_AF_PLUGIN_RF4CE_ZRC11_KEY_EXCHANGE_TRANSFER_COUNT,
                                  NULL);
      } else {
        status = EMBER_DISCOVERY_ERROR;
      }
    }
    if (status == EMBER_SUCCESS || status == EMBER_DUPLICATE_ENTRY) {
      state = ORG_PAIRING;
    } else {
      state = INITIAL;
      emberAfPluginRf4ceZrc11PairingCompleteCallback(status,
                                                     0xFF,  // pairing index
                                                     NULL,  // eui64
                                                     NULL,  // vendor info
                                                     NULL); // application info
    }
  }
#endif
}

EmberStatus emberAfRf4ceZrc11EnableAutoDiscoveryResponse(void)
{
  EmberStatus status = EMBER_INVALID_CALL;
#ifdef EMBER_AF_RF4CE_ZRC_IS_RECIPIENT
  if (state == INITIAL) {
    uint8_t profileIdList[] = {
      EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_1_1,
    };
    status = emberAfRf4ceEnableAutoDiscoveryResponse(APLC_AUTO_DISCOVERY_RESPONSE_MODE_DURATION_MS,
                                                     COUNTOF(profileIdList),
                                                     profileIdList);
    if (status == EMBER_SUCCESS) {
      state = RCP_DISCOVERY;
    }
  }
#endif
  return status;
}

void emberAfPluginRf4ceProfileRemoteControl11AutoDiscoveryResponseCompleteCallback(EmberStatus status,
                                                                                   const EmberEUI64 srcIeeeAddr,
                                                                                   uint8_t nodeCapabilities,
                                                                                   const EmberRf4ceVendorInfo *vendorInfo,
                                                                                   const EmberRf4ceApplicationInfo *appInfo,
                                                                                   uint8_t searchDevType)
{
#ifdef EMBER_AF_RF4CE_ZRC_IS_RECIPIENT
  if (state == RCP_DISCOVERY) {
    if (status == EMBER_SUCCESS) {
      // The application shall request that the NWK layer enable its receiver
      // and wait either for aplcMaxPairIndicationWaitTime or until a
      // corresponding pairing request notification is received from the NLME,
      // via the NLME-PAIR.indication primitive.
      state = RCP_PAIRING;
      MEMMOVE(originator.ieeeAddr, srcIeeeAddr, EUI64_SIZE);
      MEMMOVE(&originator.vendorInfo, vendorInfo, sizeof(EmberRf4ceVendorInfo));
      MEMMOVE(&originator.appInfo, appInfo, sizeof(EmberRf4ceApplicationInfo));
      emberEventControlSetDelayMS(emberAfPluginRf4ceZrc11PairingEventControl,
                                  APLC_MAX_PAIR_INDICATION_WAIT_TIME_MS);
      emAfRf4ceZrc11RxEnable();
    } else {
      state = INITIAL;
      emberAfPluginRf4ceZrc11PairingCompleteCallback(status,
                                                     0xFF,  // pairing index
                                                     NULL,  // eui64
                                                     NULL,  // vendor info
                                                     NULL); // application info
    }
  }
#endif
}

bool emberAfPluginRf4ceProfileRemoteControl11PairRequestCallback(EmberStatus status,
                                                                    uint8_t pairingIndex,
                                                                    const EmberEUI64 sourceIeeeAddr,
                                                                    uint8_t nodeCapabilities,
                                                                    const EmberRf4ceVendorInfo *vendorInfo,
                                                                    const EmberRf4ceApplicationInfo *appInfo,
                                                                    uint8_t keyExchangeTransferCount)
{
#ifdef EMBER_AF_RF4CE_ZRC_IS_RECIPIENT
  if (state == RCP_PAIRING
      && (status == EMBER_SUCCESS || status == EMBER_DUPLICATE_ENTRY)
      && APLC_MIN_KEY_EXCHANGE_TRANSFER_COUNT <= keyExchangeTransferCount
      && MEMCOMPARE(originator.ieeeAddr, sourceIeeeAddr, EUI64_SIZE) == 0) {
    emberEventControlSetInactive(emberAfPluginRf4ceZrc11PairingEventControl);
    emAfRf4ceZrc11RxEnable();
    return true;
  }
#endif
  return false;
}

void emberAfPluginRf4ceProfileRemoteControl11PairCompleteCallback(EmberStatus status,
                                                                  uint8_t pairingIndex,
                                                                  const EmberRf4ceVendorInfo *vendorInfo,
                                                                  const EmberRf4ceApplicationInfo *appInfo)
{
  // Overwriting an existing pairing entry is still a success.
  if (status == EMBER_DUPLICATE_ENTRY) {
    status = EMBER_SUCCESS;
  }
#ifdef EMBER_AF_RF4CE_ZRC_IS_ORIGINATOR
  if (state == ORG_PAIRING) {
    // TODO: If command discovery is supported, the application shall then
    // request that the receiver is enabled for a duration of
    // aplcMaxCmdDiscRxOnDuration by issuing the NLME-RX-ENABLE.request
    // primitive to the NLME.
    state = INITIAL;
    emberAfPluginRf4ceZrc11PairingCompleteCallback(status,
                                                   pairingIndex,
                                                   recipient.ieeeAddr,
                                                   &recipient.vendorInfo,
                                                   &recipient.appInfo);
  }
#endif
#ifdef EMBER_AF_RF4CE_ZRC_IS_RECIPIENT
  if (state == RCP_PAIRING) {
    // TODO: If required, the target may then generate and transmit a command
    // discovery request command frame to the controller in order to determine
    // its supported command set according to the procedure described in sub-
    // clause 5.4.
    state = INITIAL;
    emberAfPluginRf4ceZrc11PairingCompleteCallback(status,
                                                   pairingIndex,
                                                   originator.ieeeAddr,
                                                   &originator.vendorInfo,
                                                   &originator.appInfo);
  }
#endif
}

//------------------------------------------------------------------------------
// Event handlers.

void emberAfPluginRf4ceZrc11PairingEventHandler(void)
{
  emberEventControlSetInactive(emberAfPluginRf4ceZrc11PairingEventControl);
  emAfRf4ceZrc11RxEnable();
#ifdef EMBER_AF_RF4CE_ZRC_IS_RECIPIENT
  if (state == RCP_PAIRING) {
    // TODO: DISCOVERY_TIMEOUT isn't quite right.
    state = INITIAL;
    emberAfPluginRf4ceZrc11PairingCompleteCallback(EMBER_DISCOVERY_TIMEOUT,
                                                   0xFF,  // pairing index
                                                   NULL,  // eui64
                                                   NULL,  // vendor info
                                                   NULL); // application info
  }
#endif
}

void emAfRf4ceZrc11RxEnable(void)
{
  // The receiver must be kept on during the gap between auto discovery and the
  // corresponding pair request as well as while waiting for a command
  // discovery response.  At all other times, the receiver can be turned off.
  bool enable
    = (emberEventControlGetActive(emberAfPluginRf4ceZrc11PairingEventControl)
       || emberEventControlGetActive(emberAfPluginRf4ceZrc11CommandDiscoveryEventControl));
  emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_1_1, enable);
}
