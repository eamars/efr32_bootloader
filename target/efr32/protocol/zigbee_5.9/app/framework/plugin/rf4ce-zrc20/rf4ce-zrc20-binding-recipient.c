// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "app/framework/plugin/rf4ce-profile/rf4ce-profile.h"
#include "app/framework/plugin/rf4ce-gdp/rf4ce-gdp.h"
#include "app/framework/plugin/rf4ce-gdp/rf4ce-gdp-internal.h"
#include "rf4ce-zrc20.h"
#include "rf4ce-zrc20-internal.h"
#include "rf4ce-zrc20-attributes.h"

#if defined(EMBER_SCRIPTED_TEST)
#include "rf4ce-zrc20-test.h"
#endif

//------------------------------------------------------------------------------
// Events declaration

EmberEventControl emberAfPluginRf4ceZrc20RecipientEventControl;

#if (defined(EMBER_AF_RF4CE_ZRC_IS_RECIPIENT) || defined(EMBER_SCRIPTED_TEST))

//------------------------------------------------------------------------------
// Static variables

// Information about the current binding originator.
static uint8_t originatorPairingIndex = 0xFF;

// Action banks transmitted by the originator that are capable of being
// received by the recipient.  These are basically (actionBanksSupportedTx AND
// recipientActionBanksSupportedRx), except that HA action recipients are
// assumed to support all HA action codes, so the HA bits are explicitly unset
// in actionBanksSupportedRxExchange.
static uint8_t actionBanksSupportedTxExchange[ZRC_BITMASK_SIZE];
static uint8_t actionBanksSupportedRxExchange[ZRC_BITMASK_SIZE];

//------------------------------------------------------------------------------
// Debug stuff

#ifdef EMBER_AF_PLUGIN_RF4CE_ZRC20_DEBUG_BINDING
extern PGM_P zrc20StateNames[];

  static void printState(PGM_P command)
  {
    emberAfCorePrintln("ZRC BR RX: %p: state=%p originator=%d source=%d",
                       command,
                       zrc20StateNames[emAfZrcState],
                       originatorPairingIndex,
                       emberAfRf4ceGetPairingIndex());
  }
  static void printStateWithStatus(PGM_P command, uint8_t status)
  {
    emberAfCorePrintln("ZRC BR RX: %p (0x%x): state=%p originator=%d source=%d",
                       command,
                       status,
                       zrc20StateNames[emAfZrcState],
                       originatorPairingIndex,
                       emberAfRf4ceGetPairingIndex());
  }
  static void reallySetState(uint8_t newState, int line)
  {
    emberAfCorePrintln("ZRC BR STATE: %p (was: %p, line: %d)",
                       zrc20StateNames[newState],
                       zrc20StateNames[emAfZrcState],
                       line);
    emAfZrcState = newState;
  }

  typedef struct {
    EmberAfRf4ceZrcAttributeId attributeId;
    PGM_P attributeName;
  } AttributeNames;
  static PGM AttributeNames attributeNames[] = {
    EMBER_AF_RF4CE_ZRC_ATTRIBUTE_NAMES
  };

  static void printAttribute(EmberAfRf4ceZrcAttributeId attributeId,
                             PGM_P access)
  {
    uint8_t i;
    emberAfCorePrint("ZRC BR %p: ", access);
    for (i = 0; i < COUNTOF(attributeNames); i++) {
      if (attributeId == attributeNames[i].attributeId) {
        break;
      }
    }
    emberAfCorePrintln("%p (0x%x)",
                       (i < COUNTOF(attributeNames)
                        ? attributeNames[i].attributeName
                        : "Unknown"),
                       attributeId);
  }
#endif // EMBER_AF_PLUGIN_RF4CE_ZRC20_DEBUG_BINDING

static void startRecipientTimer(uint8_t state)
{
  emAfZrcSetState(state);
  emberEventControlSetDelayMS(emberAfPluginRf4ceZrc20RecipientEventControl,
                              APLC_MAX_CONFIG_WAIT_TIME_MS);
  emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0, true);
}

//------------------------------------------------------------------------------
// Internal APIs and callbacks

void emAfRf4ceZrc20StartConfigurationRecipient(uint8_t pairingIndex)
{
  originatorPairingIndex = pairingIndex;
  emAfRf4ceZrcSetRemoteNodeCapabilities(pairingIndex, 0);
  MEMSET(emAfRf4ceZrcRemoteNodeAttributes.actionBanksSupportedTx->contents,
         0,
         ZRC_BITMASK_SIZE);
  startRecipientTimer(ZRC_STATE_RECIPIENT_PUSH_VERSION_AND_CAPABILITIES_AND_ACTION_BANKS_VERSION);
}

void emAfRf4ceZrcIncomingConfigurationComplete(EmberAfRf4ceGdpStatus status)
{
  printStateWithStatus("COMPLETE", status);
  if (originatorPairingIndex == emberAfRf4ceGetPairingIndex()
      && emAfZrcState == ZRC_STATE_RECIPIENT_CONFIGURATION_COMPLETE) {
    emberAfRf4ceGdpGenericResponse(originatorPairingIndex,
                                   EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0,
                                   EMBER_RF4CE_NULL_VENDOR_ID,
                                   EMBER_AF_RF4CE_GDP_RESPONSE_CODE_SUCCESSFUL);
    emAfZrcSetState(ZRC_STATE_INITIAL);
    originatorPairingIndex = 0xFF;
    emberEventControlSetInactive(emberAfPluginRf4ceZrc20RecipientEventControl);
    // Turn the receiver back off at the end of the configuration procedure.
    emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0, false);
    emberAfRf4ceGdpConfigurationProcedureComplete(status == EMBER_AF_RF4CE_GDP_STATUS_SUCCESSFUL);
  }
}

bool emAfRf4ceZrcIncomingGetAttributesRecipientCallback(void)
{
  EmberAfRf4ceGdpAttributeIdentificationRecord record;
  uint8_t pairingIndex = emberAfRf4ceGetPairingIndex();

  printState("GET");

  if (originatorPairingIndex != pairingIndex) {
    return false;
  }

  if (emAfZrcState == ZRC_STATE_RECIPIENT_GET_VERSION_AND_CAPABILITIES_AND_ACTION_BANKS_VERSION
      && emAfRf4ceGdpFetchAttributeIdentificationRecord(&record)
      && record.attributeId == EMBER_AF_RF4CE_ZRC_ATTRIBUTE_VERSION
      && emAfRf4ceGdpFetchAttributeIdentificationRecord(&record)
      && record.attributeId == EMBER_AF_RF4CE_ZRC_ATTRIBUTE_CAPABILITIES
      && emAfRf4ceGdpFetchAttributeIdentificationRecord(&record)
      && record.attributeId == EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_BANKS_VERSION) {
    if (emAfRf4ceZrcExchangeActionBanks(emAfRf4ceZrcGetRemoteNodeCapabilities(pairingIndex),
                                        emAfRf4ceZrcGetLocalNodeCapabilities())) {
      startRecipientTimer(ZRC_STATE_RECIPIENT_GET_ACTION_BANKS_SUPPORTED_RX);
    } else {
      startRecipientTimer(ZRC_STATE_RECIPIENT_CONFIGURATION_COMPLETE);
    }
  } else if (emAfZrcState == ZRC_STATE_RECIPIENT_GET_ACTION_BANKS_SUPPORTED_RX
             && emAfRf4ceGdpFetchAttributeIdentificationRecord(&record)
             && record.attributeId == EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_BANKS_SUPPORTED_RX) {
    startRecipientTimer(ZRC_STATE_RECIPIENT_PUSH_ACTION_BANKS_SUPPORTED_TX);
  } else if (emAfZrcState == ZRC_STATE_RECIPIENT_GET_ACTION_CODES_SUPPORTED_RX
             && emAfRf4ceGdpFetchAttributeIdentificationRecord(&record)
             && record.attributeId == EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_CODES_SUPPORTED_RX) {
    emAfRf4ceGdpResetFetchAttributeFinger();
    while (emAfRf4ceGdpFetchAttributeIdentificationRecord(&record)) {
      EmberAfRf4ceZrcActionBank actionBank = (EmberAfRf4ceZrcActionBank)record.entryId;
      emAfRf4ceZrcClearActionBank(actionBanksSupportedRxExchange,
                                  actionBank);
    }
    // If there are still received action banks that the originator must
    // get, we stay in this state.  Otherwise, we move on.
    if (!emAfRf4ceZrcHasRemainingActionBanks(actionBanksSupportedRxExchange)) {
      if (emAfRf4ceZrcHasRemainingActionBanks(actionBanksSupportedTxExchange)) {
        startRecipientTimer(ZRC_STATE_RECIPIENT_PUSH_ACTION_CODES_SUPPORTED_TX);
      } else {
        startRecipientTimer(ZRC_STATE_RECIPIENT_CONFIGURATION_COMPLETE);
      }
    }
  } else {
    emberEventControlSetActive(emberAfPluginRf4ceZrc20RecipientEventControl);
    return false;
  }

  return true;
}

bool emAfRf4ceZrcIncomingPushAttributesRecipientCallback(void)
{
  EmberAfRf4ceGdpAttributeRecord record;
  uint8_t pairingIndex = emberAfRf4ceGetPairingIndex();

  if (originatorPairingIndex != pairingIndex) {
    return false;
  }

  if (emAfZrcState == ZRC_STATE_RECIPIENT_PUSH_VERSION_AND_CAPABILITIES_AND_ACTION_BANKS_VERSION
      && emAfRf4ceGdpFetchAttributeRecord(&record)
      && record.attributeId == EMBER_AF_RF4CE_ZRC_ATTRIBUTE_VERSION
      && emAfRf4ceGdpFetchAttributeRecord(&record)
      && record.attributeId == EMBER_AF_RF4CE_ZRC_ATTRIBUTE_CAPABILITIES
      && emAfRf4ceGdpFetchAttributeRecord(&record)
      && record.attributeId == EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_BANKS_VERSION) {
    emAfZrcSetState(ZRC_STATE_RECIPIENT_GET_VERSION_AND_CAPABILITIES_AND_ACTION_BANKS_VERSION);
  } else if (emAfZrcState == ZRC_STATE_RECIPIENT_PUSH_ACTION_BANKS_SUPPORTED_TX
             && emAfRf4ceGdpFetchAttributeRecord(&record)
             && record.attributeId == EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_BANKS_SUPPORTED_TX) {
    // Determine the commonalities between the transmitted and received
    // action banks and exchange the action codes if there are any.
    // Otherwise, configuration is done.
    emAfRf4ceZrcGetExchangeableActionBanks(record.value,
                                           emAfRf4ceZrcGetRemoteNodeCapabilities(pairingIndex),
                                           emAfRf4ceZrcLocalNodeAttributes.actionBanksSupportedRx->contents,
                                           emAfRf4ceZrcGetLocalNodeCapabilities(),
                                           actionBanksSupportedRxExchange,
                                           actionBanksSupportedTxExchange);
    if (emAfRf4ceZrcHasRemainingActionBanks(actionBanksSupportedRxExchange)) {
      emAfZrcSetState(ZRC_STATE_RECIPIENT_GET_ACTION_CODES_SUPPORTED_RX);
    } else if (emAfRf4ceZrcHasRemainingActionBanks(actionBanksSupportedTxExchange)) {
      emAfZrcSetState(ZRC_STATE_RECIPIENT_PUSH_ACTION_CODES_SUPPORTED_TX);
    } else {
      emAfZrcSetState(ZRC_STATE_RECIPIENT_CONFIGURATION_COMPLETE);
    }
  } else if (emAfZrcState == ZRC_STATE_RECIPIENT_PUSH_ACTION_CODES_SUPPORTED_TX
             && emAfRf4ceGdpFetchAttributeRecord(&record)
             && record.attributeId == EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_CODES_SUPPORTED_TX) {
    emAfRf4ceGdpResetFetchAttributeFinger();
    while (emAfRf4ceGdpFetchAttributeRecord(&record)) {
      emAfRf4ceZrcClearActionBank(actionBanksSupportedTxExchange,
                                  (uint8_t)record.entryId);
    }

    // If there are still transmitted action banks that the originator is
    // pushing, we stay in this state.  Otherwise, we move on.
    if (!emAfRf4ceZrcHasRemainingActionBanks(actionBanksSupportedTxExchange)) {
      emAfZrcSetState(ZRC_STATE_RECIPIENT_CONFIGURATION_COMPLETE);
    }
  } else {
    emberEventControlSetActive(emberAfPluginRf4ceZrc20RecipientEventControl);
    return false;
  }

  emberEventControlSetDelayMS(emberAfPluginRf4ceZrc20RecipientEventControl,
                              APLC_MAX_CONFIG_WAIT_TIME_MS);
  return true;
}

void emberAfPluginRf4ceZrc20RecipientEventHandler(void)
{
  printState("TIMEOUT");
  emAfZrcSetState(ZRC_STATE_INITIAL);
  originatorPairingIndex = 0xFF;
  emberEventControlSetInactive(emberAfPluginRf4ceZrc20RecipientEventControl);
  // Turn the receiver back off at the end of the configuration procedure.
  emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0, false);
  emberAfRf4ceGdpConfigurationProcedureComplete(false);
}

#else // EMBER_AF_RF4CE_ZRC_IS_RECIPIENT

void emAfRf4ceZrc20StartConfigurationRecipient(uint8_t pairingIndex)
{
}

void emAfRf4ceZrc20BindingCompleteRecipient(uint8_t pairingIndex)
{
}

void emAfRf4ceZrcIncomingConfigurationComplete(EmberAfRf4ceGdpStatus status)
{
}

void emberAfPluginRf4ceZrc20RecipientEventHandler(void)
{
  emberEventControlSetInactive(emberAfPluginRf4ceZrc20RecipientEventControl);
}

#endif // EMBER_AF_RF4CE_ZRC_IS_RECIPIENT
