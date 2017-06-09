// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "app/framework/plugin/rf4ce-profile/rf4ce-profile.h"
#include "app/framework/plugin/rf4ce-gdp/rf4ce-gdp-types.h"
#include "app/framework/plugin/rf4ce-gdp/rf4ce-gdp-internal.h"
#include "rf4ce-zrc20.h"
#include "rf4ce-zrc20-internal.h"
#include "rf4ce-zrc20-attributes.h"
#include "rf4ce-zrc20-action-mapping.h"
#include "rf4ce-zrc20-ha-actions.h"

#if defined(EMBER_SCRIPTED_TEST)
#include "rf4ce-zrc20-test.h"
#endif

uint8_t emAfZrcState = ZRC_STATE_INITIAL;

#ifdef EMBER_AF_PLUGIN_RF4CE_ZRC20_DEBUG_BINDING
  PGM_P zrc20StateNames[] = {
    "initial",
    "push version and capabilities and action banks version",
    "get version and capabilities and action banks version",
    "get action banks supported rx",
    "push action banks supported tx",
    "get action codes supported rx",
    "push action codes supported tx",
    "configuration complete",
    "legacy command discovery",
    "push version and capabilities and action banks version",
    "get version and capabilities and action banks version",
    "get action banks supported rx",
    "push action banks supported tx",
    "get action codes supported rx",
    "push action codes supported tx",
    "configuration complete",
  };
#endif // EMBER_AF_PLUGIN_RF4CE_ZRC20_DEBUG_BINDING

//------------------------------------------------------------------------------
//

void emberAfPluginRf4ceZrc20InitCallback(void)
{
  emAfRf4ceZrc20InitOriginator();
  emAfRf4ceZrc20InitRecipient();
  emAfRf4ceZrc20AttributesInit();
}

// TODO: For now, all discovery and pairing callbacks are passed through to the
// GDP plugin.  There should be a better way to handle this, but this is good
// enough for now.

bool emberAfPluginRf4ceProfileRemoteControl11DiscoveryRequestCallback(const EmberEUI64 ieeeAddr,
                                                                         uint8_t nodeCapabilities,
                                                                         const EmberRf4ceVendorInfo *vendorInfo,
                                                                         const EmberRf4ceApplicationInfo *appInfo,
                                                                         uint8_t searchDevType,
                                                                         uint8_t rxLinkQuality)
{
  return emberAfPluginRf4ceProfileGdpDiscoveryRequestCallback(ieeeAddr,
                                                              nodeCapabilities,
                                                              vendorInfo,
                                                              appInfo,
                                                              searchDevType,
                                                              rxLinkQuality);
}

bool emberAfPluginRf4ceProfileZrc20DiscoveryRequestCallback(const EmberEUI64 ieeeAddr,
                                                               uint8_t nodeCapabilities,
                                                               const EmberRf4ceVendorInfo *vendorInfo,
                                                               const EmberRf4ceApplicationInfo *appInfo,
                                                               uint8_t searchDevType,
                                                               uint8_t rxLinkQuality)
{
  return emberAfPluginRf4ceProfileGdpDiscoveryRequestCallback(ieeeAddr,
                                                              nodeCapabilities,
                                                              vendorInfo,
                                                              appInfo,
                                                              searchDevType,
                                                              rxLinkQuality);
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
  return emberAfPluginRf4ceProfileGdpDiscoveryResponseCallback(atCapacity,
                                                               channel,
                                                               panId,
                                                               ieeeAddr,
                                                               nodeCapabilities,
                                                               vendorInfo,
                                                               appInfo,
                                                               rxLinkQuality,
                                                               discRequestLqi);
}

bool emberAfPluginRf4ceProfileZrc20DiscoveryResponseCallback(bool atCapacity,
                                                                uint8_t channel,
                                                                EmberPanId panId,
                                                                const EmberEUI64 ieeeAddr,
                                                                uint8_t nodeCapabilities,
                                                                const EmberRf4ceVendorInfo *vendorInfo,
                                                                const EmberRf4ceApplicationInfo *appInfo,
                                                                uint8_t rxLinkQuality,
                                                                uint8_t discRequestLqi)
{
  return emberAfPluginRf4ceProfileGdpDiscoveryResponseCallback(atCapacity,
                                                               channel,
                                                               panId,
                                                               ieeeAddr,
                                                               nodeCapabilities,
                                                               vendorInfo,
                                                               appInfo,
                                                               rxLinkQuality,
                                                               discRequestLqi);
}

void emberAfPluginRf4ceProfileRemoteControl11DiscoveryCompleteCallback(EmberStatus status)
{
  // Not implemented: for a ZRC 2.0 controller, this callback will always fire
  // together with the corresponding zrc20 one.
}

void emberAfPluginRf4ceProfileZrc20DiscoveryCompleteCallback(EmberStatus status)
{
  emberAfPluginRf4ceProfileGdpDiscoveryCompleteCallback(status);
}

void emberAfPluginRf4ceProfileRemoteControl11AutoDiscoveryResponseCompleteCallback(EmberStatus status,
                                                                                   const EmberEUI64 srcIeeeAddr,
                                                                                   uint8_t nodeCapabilities,
                                                                                   const EmberRf4ceVendorInfo *vendorInfo,
                                                                                   const EmberRf4ceApplicationInfo *appInfo,
                                                                                   uint8_t searchDevType)
{
  emberAfPluginRf4ceProfileGdpAutoDiscoveryResponseCompleteCallback(status,
                                                                    srcIeeeAddr,
                                                                    nodeCapabilities,
                                                                    vendorInfo,
                                                                    appInfo,
                                                                    searchDevType);
}

void emberAfPluginRf4ceProfileZrc20AutoDiscoveryResponseCompleteCallback(EmberStatus status,
                                                                         const EmberEUI64 srcIeeeAddr,
                                                                         uint8_t nodeCapabilities,
                                                                         const EmberRf4ceVendorInfo *vendorInfo,
                                                                         const EmberRf4ceApplicationInfo *appInfo,
                                                                         uint8_t searchDevType)
{
  emberAfPluginRf4ceProfileGdpAutoDiscoveryResponseCompleteCallback(status,
                                                                    srcIeeeAddr,
                                                                    nodeCapabilities,
                                                                    vendorInfo,
                                                                    appInfo,
                                                                    searchDevType);
}

bool emberAfPluginRf4ceProfileRemoteControl11PairRequestCallback(EmberStatus status,
                                                                    uint8_t pairingIndex,
                                                                    const EmberEUI64 sourceIeeeAddr,
                                                                    uint8_t nodeCapabilities,
                                                                    const EmberRf4ceVendorInfo *vendorInfo,
                                                                    const EmberRf4ceApplicationInfo *appInfo,
                                                                    uint8_t keyExchangeTransferCount)
{
  return emberAfPluginRf4ceProfileGdpPairRequestCallback(status,
                                                         pairingIndex,
                                                         sourceIeeeAddr,
                                                         nodeCapabilities,
                                                         vendorInfo,
                                                         appInfo,
                                                         keyExchangeTransferCount);
}

bool emberAfPluginRf4ceProfileZrc20PairRequestCallback(EmberStatus status,
                                                          uint8_t pairingIndex,
                                                          const EmberEUI64 sourceIeeeAddr,
                                                          uint8_t nodeCapabilities,
                                                          const EmberRf4ceVendorInfo *vendorInfo,
                                                          const EmberRf4ceApplicationInfo *appInfo,
                                                          uint8_t keyExchangeTransferCount)
{
  return emberAfPluginRf4ceProfileGdpPairRequestCallback(status,
                                                         pairingIndex,
                                                         sourceIeeeAddr,
                                                         nodeCapabilities,
                                                         vendorInfo,
                                                         appInfo,
                                                         keyExchangeTransferCount);
}

void emberAfPluginRf4ceProfileRemoteControl11PairCompleteCallback(EmberStatus status,
                                                                  uint8_t pairingIndex,
                                                                  const EmberRf4ceVendorInfo *vendorInfo,
                                                                  const EmberRf4ceApplicationInfo *appInfo)
{
  emberAfPluginRf4ceProfileGdpPairCompleteCallback(status,
                                                   pairingIndex,
                                                   vendorInfo,
                                                   appInfo);
}

void emberAfPluginRf4ceProfileZrc20PairCompleteCallback(EmberStatus status,
                                                        uint8_t pairingIndex,
                                                        const EmberRf4ceVendorInfo *vendorInfo,
                                                        const EmberRf4ceApplicationInfo *appInfo)
{
  emberAfPluginRf4ceProfileGdpPairCompleteCallback(status,
                                                   pairingIndex,
                                                   vendorInfo,
                                                   appInfo);
}

bool emberAfPluginRf4ceGdpZrc20StartConfigurationCallback(bool isOriginator,
                                                             uint8_t pairingIndex)
{
  if (isOriginator) {
    emAfRf4ceZrc20StartConfigurationOriginator(pairingIndex);
  } else {
    emAfRf4ceZrc20StartConfigurationRecipient(pairingIndex);
  }
  return true;
}

void emberAfPluginRf4ceGdpZrc20BindingCompleteCallback(EmberAfRf4ceGdpBindingStatus status,
                                                       uint8_t pairingIndex)
{
  // We only dispatch if the binding was successful.
  if (status != EMBER_AF_RF4CE_GDP_BINDING_STATUS_SUCCESS) {
    return;
  }

  // Notify the action mapping code.
  emAfRf4ceZrcActionMappingBindingCompleteCallback(pairingIndex);

  // Notify the HA actions code.
  emAfRf4ceZrcHAActionsBindingCompleteCallback(pairingIndex);
}

//#if defined(EMBER_AF_PRINT_ENABLE) && defined(EMBER_AF_PRINT_DEBUG)
//static void printActionBanksSupported(const uint8_t *actionBanksSupported)
//{
//  uint8_t i;
//  for (i = 0; i < ZRC_BITMASK_SIZE; i++) {
//    emberAfDebugPrint(" %x", actionBanksSupported[i]);
//  }
//  emberAfDebugPrintln("");
//}
//#endif

// The action bank needs to be converted to a single bit in a giant 256-bit
// bitmask.  The bitmask is stored as a 32-byte array.  The byte in the array
// where the bit is stored is obtained by dividing the action bank by eight.
// The bit within the byte is obtained by moduling the action bank by eight,
// which is the same as masking all but the low three bits.
// For example, HA instance 17 is action bank 0x91 (145):
//   byte = 145 / 8 = 18
//   bit = 145 % 8 = 1
// The HA instance 17 action bank is bit 1 in byte 18 of the array.
#define ACTION_BYTE(actionBank) (actionBank / 8)
#define ACTION_BIT(actionBank)  (actionBank & 0x07)

void emAfRf4ceZrcClearActionBank(uint8_t *actionBanksSupported,
                                 EmberAfRf4ceZrcActionBank actionBank)
{
  uint8_t byte = ACTION_BYTE(actionBank);
  uint8_t bit = ACTION_BIT(actionBank);
  CLEARBIT(actionBanksSupported[byte], bit);
  //emberAfDebugPrint("remaining:                      ");
  //emberAfDebugDebugExec(printActionBanksSupported(actionBanksSupported));
}

bool emAfRf4ceZrcReadActionBank(const uint8_t *actionBanksSupported,
                                   EmberAfRf4ceZrcActionBank actionBank)
{
  uint8_t byte = ACTION_BYTE(actionBank);
  uint8_t bit = ACTION_BIT(actionBank);
  return READBIT(actionBanksSupported[byte], bit);
}


void emAfRf4ceZrcSetActionBank(uint8_t *actionBanksSupported,
                               EmberAfRf4ceZrcActionBank actionBank)
{
  uint8_t byte = ACTION_BYTE(actionBank);
  uint8_t bit = ACTION_BIT(actionBank);
  SETBIT(actionBanksSupported[byte], bit);
}

bool emAfRf4ceZrcHasRemainingActionBanks(const uint8_t *actionBanksSupported)
{
  uint8_t i;
  for (i = 0; i < ZRC_BITMASK_SIZE; i++) {
    if (actionBanksSupported[i] != 0) {
      return true;
    }
  }
  return false;
}

void emAfRf4ceZrcGetExchangeableActionBanks(const uint8_t *actionBanksSupportedTx,
                                            EmberAfRf4ceZrcCapability originatorCapabilities,
                                            const uint8_t *actionBanksSupportedRx,
                                            EmberAfRf4ceZrcCapability recipientCapabilities,
                                            uint8_t *actionBanksSupportedRxExchange,
                                            uint8_t *actionBanksSupportedTxExchange)
{
  uint8_t actionBanksSupportedCommon[ZRC_BITMASK_SIZE];
  uint8_t i;

  MEMSET(actionBanksSupportedTxExchange, 0, ZRC_BITMASK_SIZE);
  MEMSET(actionBanksSupportedRxExchange, 0, ZRC_BITMASK_SIZE);

  for (i = 0; i < ZRC_BITMASK_SIZE; i++) {
    actionBanksSupportedCommon[i] = (actionBanksSupportedTx[i]
                                     & actionBanksSupportedRx[i]);
  }

  // Section 4.1.2.2.4: The vendor specific action banks with explicit vendor
  // ID shall NOT be included in the command discovery, since the vendor ID is
  // not known during the command discovery.  These action banks are 0xE0
  // through 0xFF and correspond to indexes 28 through 31 in an action banks
  // array.  They are cleared from the common action banks.
  actionBanksSupportedCommon[28] = 0;
  actionBanksSupportedCommon[29] = 0;
  actionBanksSupportedCommon[30] = 0;
  actionBanksSupportedCommon[31] = 0;

  if (originatorCapabilities
      & EMBER_AF_RF4CE_ZRC_CAPABILITY_INFORM_ABOUT_SUPPORTED_ACTIONS) {
    // Section 7.2 requires an HA actions recipient to support all HA action
    // banks and all HA action codes within those banks.  Because of this, the
    // exchange of received HA action codes is unnecessary.  This is formalized
    // in sections 6.4.1.4 and 6.4.2.4, which say that the HA action banks are
    // "blanked" during the discovery of the action codes supported by the
    // recipient.  The HA action banks are 0x80 through 0x9F and correspond to
    // indexes 16 through 19 in an action banks array.
    MEMCOPY(actionBanksSupportedRxExchange,
            actionBanksSupportedCommon,
            ZRC_BITMASK_SIZE);
    actionBanksSupportedRxExchange[16] = 0;
    actionBanksSupportedRxExchange[17] = 0;
    actionBanksSupportedRxExchange[18] = 0;
    actionBanksSupportedRxExchange[19] = 0;
  }
  if (recipientCapabilities
      & EMBER_AF_RF4CE_ZRC_CAPABILITY_INFORM_ABOUT_SUPPORTED_ACTIONS) {
    // HA action originators do not have to support all HA action banks or
    // codes, so these are exchanged, as described in sections 6.4.1.5 and
    // 6.4.2.5, and are not cleared here.
    MEMCOPY(actionBanksSupportedTxExchange,
            actionBanksSupportedCommon,
            ZRC_BITMASK_SIZE);
  }

  //emberAfDebugPrint("actionBanksSupportedTx:         ");
  //emberAfDebugDebugExec(printActionBanksSupported(actionBanksSupportedTx));
  //emberAfDebugPrint("actionBanksSupportedRx:         ");
  //emberAfDebugDebugExec(printActionBanksSupported(actionBanksSupportedRx));
  //emberAfDebugPrint("actionBanksSupportedCommon:     ");
  //emberAfDebugDebugExec(printActionBanksSupported(actionBanksSupportedCommon));
  //emberAfDebugPrint("actionBanksSupportedTxExchange: ");
  //emberAfDebugDebugExec(printActionBanksSupported(actionBanksSupportedTxExchange));
  //emberAfDebugPrint("actionBanksSupportedRxExchange: ");
  //emberAfDebugDebugExec(printActionBanksSupported(actionBanksSupportedRxExchange));
}

//------------------------------------------------------------------------------
// Command dispatchers

extern void emAfRf4ceZrcIncomingGenericResponseBindingOriginatorCallback(EmberAfRf4ceGdpResponseCode responseCode);
extern void emAfRf4ceZrcIncomingGenericResponseActionMappingClientCallback(EmberAfRf4ceGdpResponseCode responseCode);
extern void emAfRf4ceZrcIncomingGenericResponseHomeAutomationActionsCallbackOriginator(EmberAfRf4ceGdpResponseCode responseCode);

void emAfRf4ceZrcIncomingGenericResponse(EmberAfRf4ceGdpResponseCode responseCode)
{
  if (isZrcStateBindingOriginator()) {
    emAfRf4ceZrcIncomingGenericResponseBindingOriginatorCallback(responseCode);
  }
#if defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_IS_ACTION_MAPPING_CLIENT) || defined(EMBER_SCRIPTED_TEST)
  else if (isZrcStateActionMappingClient()) {
    emAfRf4ceZrcIncomingGenericResponseActionMappingClientCallback(responseCode);
  }
#endif // EMBER_AF_PLUGIN_RF4CE_ZRC20_IS_ACTION_MAPPING_CLIENT

#if defined(EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_ORIGINATOR) || defined(EMBER_SCRIPTED_TEST)
  else if (isZrcStateHaActionsOriginator()) {
    emAfRf4ceZrcIncomingGenericResponseHomeAutomationActionsCallbackOriginator(responseCode);
  }
#endif // EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_ORIGINATOR
}
