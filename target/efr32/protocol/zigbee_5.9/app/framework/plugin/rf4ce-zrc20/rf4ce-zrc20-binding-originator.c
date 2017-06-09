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
// Debug stuff

#ifdef EMBER_AF_PLUGIN_RF4CE_ZRC20_DEBUG_BINDING
extern PGM_P zrc20StateNames[];
  static void reallySetState(uint8_t newState, int line)
  {
    emberAfCorePrintln("ZRC BO STATE: %p -> %p (line %d)",
                       zrc20StateNames[emAfZrcState],
                       zrc20StateNames[newState],
                       line);
    emAfZrcState = newState;
  }
#endif // EMBER_AF_PLUGIN_RF4CE_ZRC20_DEBUG_BINDING

//------------------------------------------------------------------------------
// Events declaration

EmberEventControl emberAfPluginRf4ceZrc20OriginatorEventControl;

#if (defined(EMBER_AF_RF4CE_ZRC_IS_ORIGINATOR) || defined(EMBER_SCRIPTED_TEST))

//------------------------------------------------------------------------------
// Static variables and forward declarations.

// Information about the current binding recipient.
static uint8_t recipientPairingIndex = 0xFF;

// Action banks transmitted by the originator that are capable of being
// received by the recipient.  These are basically (actionBanksSupportedTx AND
// recipientActionBanksSupportedRx), except that HA action recipients are
// assumed to support all HA action codes, so the HA bits are explicitly unset
// in actionBanksSupportedRxExchange.
static uint8_t actionBanksSupportedTxExchange[ZRC_BITMASK_SIZE];
static uint8_t actionBanksSupportedRxExchange[ZRC_BITMASK_SIZE];

// Used to perform sanity check for the incoming getAttributesResponse()
// commands related to action codes.
static uint16_t pendingGetActionCodes[ACTION_CODES_SUPPORTED_RECORDS_MAX];

static void pushVersionAndOriginatorCapabilitiesAndActionBanksVersion(void);
static void getVersionAndRecipientCapabilitiesAndActionBanksVersion(void);
static void getActionBanksSupportedRx(void);
static void pushActionBanksSupportedTx(void);
static void getActionCodesSupportedRx(void);
static void pushActionCodesSupportedTx(void);
static void configurationComplete(void);
bool incomingActionCodesResponseIsValid(void);

//------------------------------------------------------------------------------
// Public APIs

EmberStatus emberAfRf4ceZrc20Bind(EmberAfRf4ceDeviceType searchDevType)
{
  EmberStatus status;
  uint8_t profileIdList[] = {
    EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_1_1,
    EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0,
  };

  status = emberAfRf4ceGdpBind(profileIdList,
                               COUNTOF(profileIdList),
                               searchDevType);
  return status;
}

EmberStatus emberAfRf4ceZrc20ProxyBind(EmberPanId panId,
                                       EmberEUI64 ieeeAddr)
{
  EmberStatus status;
  uint8_t profileIdList[] = {
    EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0,
  };
  status = emberAfRf4ceGdpProxyBind(panId,
                                    ieeeAddr,
                                    profileIdList,
                                    COUNTOF(profileIdList));
  return status;
}

//------------------------------------------------------------------------------
// Internal APIs and callbacks

void emAfRf4ceZrc20StartConfigurationOriginator(uint8_t pairingIndex)
{
  recipientPairingIndex = pairingIndex;
  emAfRf4ceZrcSetRemoteNodeCapabilities(pairingIndex, 0);
  MEMSET(emAfRf4ceZrcRemoteNodeAttributes.actionBanksSupportedRx->contents,
         0,
         ZRC_BITMASK_SIZE);
  pushVersionAndOriginatorCapabilitiesAndActionBanksVersion();
}

void emAfRf4ceZrcIncomingGenericResponseBindingOriginatorCallback(EmberAfRf4ceGdpResponseCode responseCode)
{
  uint8_t pairingIndex = emberAfRf4ceGetPairingIndex();

  if (pairingIndex == recipientPairingIndex) {
    if (emAfZrcState == ZRC_STATE_ORIGINATOR_CONFIGURATION_COMPLETE) {
      emAfZrcSetState(ZRC_STATE_INITIAL);
      recipientPairingIndex = 0xFF;
      emberEventControlSetInactive(emberAfPluginRf4ceZrc20OriginatorEventControl);
      // Turn the radio back off at the ZRC plugin.
      emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0, false);
      emberAfRf4ceGdpConfigurationProcedureComplete(responseCode == EMBER_AF_RF4CE_GDP_RESPONSE_CODE_SUCCESSFUL);
    } else if (responseCode != EMBER_AF_RF4CE_GDP_RESPONSE_CODE_SUCCESSFUL) {
      emberEventControlSetActive(emberAfPluginRf4ceZrc20OriginatorEventControl);
    } else if (emAfZrcState == ZRC_STATE_ORIGINATOR_PUSH_VERSION_AND_CAPABILITIES_AND_ACTION_BANKS_VERSION) {
      getVersionAndRecipientCapabilitiesAndActionBanksVersion();
    } else if (emAfZrcState == ZRC_STATE_ORIGINATOR_PUSH_ACTION_BANKS_SUPPORTED_TX) {
      // Determine the commonalities between the transmitted and received action
      // banks and exchange the action codes if there are any.  Otherwise,
      // configuration is done.
      emAfRf4ceZrcGetExchangeableActionBanks(emAfRf4ceZrcLocalNodeAttributes.actionBanksSupportedTx->contents,
                                             emAfRf4ceZrcGetLocalNodeCapabilities(),
                                             emAfRf4ceZrcRemoteNodeAttributes.actionBanksSupportedRx->contents,
                                             emAfRf4ceZrcGetRemoteNodeCapabilities(pairingIndex),
                                             actionBanksSupportedRxExchange,
                                             actionBanksSupportedTxExchange);
      if (emAfRf4ceZrcHasRemainingActionBanks(actionBanksSupportedRxExchange)) {
        getActionCodesSupportedRx();
      } else if (emAfRf4ceZrcHasRemainingActionBanks(actionBanksSupportedTxExchange)) {
        pushActionCodesSupportedTx();
      } else {
        configurationComplete();
      }
    } else if (emAfZrcState == ZRC_STATE_ORIGINATOR_PUSH_ACTION_CODES_SUPPORTED_TX) {
      if (emAfRf4ceZrcHasRemainingActionBanks(actionBanksSupportedTxExchange)) {
        pushActionCodesSupportedTx();
      } else {
        configurationComplete();
      }
    } else {
      emberEventControlSetActive(emberAfPluginRf4ceZrc20OriginatorEventControl);
    }
  }
}

bool emAfRf4ceZrcIncomingGetAttributesResponseOriginatorCallback(void)
{
  EmberAfRf4ceGdpAttributeStatusRecord records[3];
  uint8_t pairingIndex = emberAfRf4ceGetPairingIndex();

  if (recipientPairingIndex == pairingIndex) {
    if (emAfZrcState == ZRC_STATE_ORIGINATOR_GET_VERSION_AND_CAPABILITIES_AND_ACTION_BANKS_VERSION
        && emAfRf4ceGdpFetchAttributeStatusRecord(&records[0])
        && records[0].attributeId == EMBER_AF_RF4CE_ZRC_ATTRIBUTE_VERSION
        && records[0].status == EMBER_AF_RF4CE_GDP_ATTRIBUTE_STATUS_SUCCESS
        && emAfRf4ceGdpFetchAttributeStatusRecord(&records[1])
        && records[1].attributeId == EMBER_AF_RF4CE_ZRC_ATTRIBUTE_CAPABILITIES
        && records[1].status == EMBER_AF_RF4CE_GDP_ATTRIBUTE_STATUS_SUCCESS
        && emAfRf4ceGdpFetchAttributeStatusRecord(&records[2])
        && records[2].attributeId == EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_BANKS_VERSION
        && records[2].status == EMBER_AF_RF4CE_GDP_ATTRIBUTE_STATUS_SUCCESS) {
      if (emAfRf4ceZrcExchangeActionBanks(emAfRf4ceZrcGetLocalNodeCapabilities(),
                                          emberFetchLowHighInt32u((uint8_t*)records[1].value))) {
        getActionBanksSupportedRx();
      } else {
        configurationComplete();
      }
      return true;
    } else if (emAfZrcState == ZRC_STATE_ORIGINATOR_GET_ACTION_BANKS_SUPPORTED_RX
               && emAfRf4ceGdpFetchAttributeStatusRecord(&records[0])
               && records[0].attributeId == EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_BANKS_SUPPORTED_RX
               && records[0].status == EMBER_AF_RF4CE_GDP_ATTRIBUTE_STATUS_SUCCESS) {
      pushActionBanksSupportedTx();
      return true;
    } else if (emAfZrcState == ZRC_STATE_ORIGINATOR_GET_ACTION_CODES_SUPPORTED_RX
               && incomingActionCodesResponseIsValid()) {
      if (emAfRf4ceZrcHasRemainingActionBanks(actionBanksSupportedRxExchange)) {
        getActionCodesSupportedRx();
      } else if (emAfRf4ceZrcHasRemainingActionBanks(actionBanksSupportedTxExchange)) {
        pushActionCodesSupportedTx();
      } else {
        configurationComplete();
      }
      return true;
    } else {
      emberEventControlSetActive(emberAfPluginRf4ceZrc20OriginatorEventControl);
    }
  }

  return false;
}

void emberAfPluginRf4ceZrc20OriginatorEventHandler(void)
{
  emberEventControlSetInactive(emberAfPluginRf4ceZrc20OriginatorEventControl);

  // Don't send another configuration complete if we don't receive the
  // corresponding GenericResponse in time.
  if (emAfZrcState != ZRC_STATE_ORIGINATOR_CONFIGURATION_COMPLETE) {
    emberAfRf4ceGdpConfigurationComplete(recipientPairingIndex,
                                         EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0,
                                         EMBER_RF4CE_NULL_VENDOR_ID,
                                         EMBER_AF_RF4CE_GDP_STATUS_CONFIGURATION_FAILURE);
  }
  recipientPairingIndex = 0xFF;

  // Turn the radio back off at the ZRC plugin.
  emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0, false);

  emAfZrcSetState(ZRC_STATE_INITIAL);

  emberAfRf4ceGdpConfigurationProcedureComplete(false);
}

//------------------------------------------------------------------------------
// Static functions

static void startOriginatorTimer(uint8_t state)
{
  emAfZrcSetState(state);
  emberEventControlSetDelayMS(emberAfPluginRf4ceZrc20OriginatorEventControl,
                              APLC_MAX_RESPONSE_WAIT_TIME_MS);
  emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0, true);
}

static void pushVersionAndOriginatorCapabilitiesAndActionBanksVersion(void)
{
  EmberAfRf4ceGdpAttributeRecord records[3];
  uint8_t version[APL_ZRC_PROFILE_VERSION_SIZE];
  uint8_t capabilities[APL_ZRC_PROFILE_CAPABILITIES_SIZE];
  uint8_t actionBanksVersion[APL_ZRC_ACTION_BANKS_VERSION_SIZE];

  emAfRf4ceZrcReadLocalAttribute(EMBER_AF_RF4CE_ZRC_ATTRIBUTE_VERSION,
                                 0,
                                 version);
  records[0].attributeId = EMBER_AF_RF4CE_ZRC_ATTRIBUTE_VERSION;
  records[0].valueLength = APL_ZRC_PROFILE_VERSION_SIZE;
  records[0].value = (uint8_t*)version;

  emAfRf4ceZrcReadLocalAttribute(EMBER_AF_RF4CE_ZRC_ATTRIBUTE_CAPABILITIES,
                                 0,
                                 capabilities);
  records[1].attributeId = EMBER_AF_RF4CE_ZRC_ATTRIBUTE_CAPABILITIES;
  records[1].valueLength = APL_ZRC_PROFILE_CAPABILITIES_SIZE;
  records[1].value = (uint8_t*)capabilities;

  emAfRf4ceZrcReadLocalAttribute(EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_BANKS_VERSION,
                                 0,
                                 actionBanksVersion);
  records[2].attributeId = EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_BANKS_VERSION;
  records[2].valueLength = APL_ZRC_ACTION_BANKS_VERSION_SIZE;
  records[2].value = (uint8_t*)actionBanksVersion;

  emberAfRf4ceGdpPushAttributes(recipientPairingIndex,
                                EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0,
                                EMBER_RF4CE_NULL_VENDOR_ID,
                                records,
                                3);

  startOriginatorTimer(ZRC_STATE_ORIGINATOR_PUSH_VERSION_AND_CAPABILITIES_AND_ACTION_BANKS_VERSION);
}

static void getVersionAndRecipientCapabilitiesAndActionBanksVersion(void)
{
  EmberAfRf4ceGdpAttributeIdentificationRecord records[3];

  records[0].attributeId = EMBER_AF_RF4CE_ZRC_ATTRIBUTE_VERSION;
  records[1].attributeId = EMBER_AF_RF4CE_ZRC_ATTRIBUTE_CAPABILITIES;
  records[2].attributeId = EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_BANKS_VERSION;

  emberAfRf4ceGdpGetAttributes(recipientPairingIndex,
                               EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0,
                               EMBER_RF4CE_NULL_VENDOR_ID,
                               records,
                               3);

  startOriginatorTimer(ZRC_STATE_ORIGINATOR_GET_VERSION_AND_CAPABILITIES_AND_ACTION_BANKS_VERSION);
}

static void getActionBanksSupportedRx(void)
{
  EmberAfRf4ceGdpAttributeIdentificationRecord record;

  record.attributeId = EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_BANKS_SUPPORTED_RX;

  emberAfRf4ceGdpGetAttributes(recipientPairingIndex,
                               EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0,
                               EMBER_RF4CE_NULL_VENDOR_ID,
                               &record,
                               1);
  startOriginatorTimer(ZRC_STATE_ORIGINATOR_GET_ACTION_BANKS_SUPPORTED_RX);
}

static void pushActionBanksSupportedTx(void)
{
  EmberAfRf4ceGdpAttributeRecord record;

  record.attributeId = EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_BANKS_SUPPORTED_TX;
  record.valueLength = ZRC_BITMASK_SIZE;
  record.value = emAfRf4ceZrcLocalNodeAttributes.actionBanksSupportedTx->contents;

  emberAfRf4ceGdpPushAttributes(recipientPairingIndex,
                                EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0,
                                EMBER_RF4CE_NULL_VENDOR_ID,
                                &record,
                                1);
  startOriginatorTimer(ZRC_STATE_ORIGINATOR_PUSH_ACTION_BANKS_SUPPORTED_TX);
}

static void getActionCodesSupportedRx(void)
{
  EmberAfRf4ceGdpAttributeIdentificationRecord records[ACTION_CODES_SUPPORTED_RECORDS_MAX];
  uint16_t i;
  uint8_t recordsCount = 0;

  MEMSET(pendingGetActionCodes, 0xFF, sizeof(pendingGetActionCodes));

  // Search for unsent action banks starting from the lowest index.  Add as
  // many as will fit into the corresponding response.
  for (i = 0; i <= MAX_INT8U_VALUE; i++) {
    if (emAfRf4ceZrcReadActionBank(actionBanksSupportedRxExchange, (uint8_t)i)) {
      emAfRf4ceZrcClearActionBank(actionBanksSupportedRxExchange, (uint8_t)i);
      records[recordsCount].attributeId =
          EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_CODES_SUPPORTED_RX;
      records[recordsCount].entryId = i;
      pendingGetActionCodes[recordsCount] = i;
      recordsCount++;
      if (recordsCount == ACTION_CODES_SUPPORTED_RECORDS_MAX) {
        break;
      }
    }
  }

  assert(recordsCount > 0);

  emberAfRf4ceGdpGetAttributes(recipientPairingIndex,
                               EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0,
                               EMBER_RF4CE_NULL_VENDOR_ID,
                               records,
                               recordsCount);
  startOriginatorTimer(ZRC_STATE_ORIGINATOR_GET_ACTION_CODES_SUPPORTED_RX);
}

static void pushActionCodesSupportedTx(void)
{
  EmberAfRf4ceGdpAttributeRecord records[ACTION_CODES_SUPPORTED_RECORDS_MAX];
  uint8_t recordsNum = 0;
  uint16_t i;

  // Search for unsent action banks starting from the lowest index.  Add as
  // many as will fit into the request.
  for (i = 0; i <= MAX_INT8U_VALUE; i++) {
    if (emAfRf4ceZrcReadActionBank(actionBanksSupportedTxExchange, (uint8_t)i)) {
      emAfRf4ceZrcClearActionBank(actionBanksSupportedTxExchange, (uint8_t)i);
      records[recordsNum].attributeId =
          EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_CODES_SUPPORTED_TX;
      records[recordsNum].entryId = i;
      records[recordsNum].valueLength = ZRC_BITMASK_SIZE;
      records[recordsNum].value =
          emAfRf4ceZrcGetActionCodesAttributePointer(EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_CODES_SUPPORTED_TX,
                                                     i,
                                                     0xFF); // local attribute
      recordsNum++;

      if (recordsNum == ACTION_CODES_SUPPORTED_RECORDS_MAX) {
        break;
      }
    }
  }

  assert(recordsNum > 0);

  emberAfRf4ceGdpPushAttributes(recipientPairingIndex,
                                EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0,
                                EMBER_RF4CE_NULL_VENDOR_ID,
                                records,
                                recordsNum);
  startOriginatorTimer(ZRC_STATE_ORIGINATOR_PUSH_ACTION_CODES_SUPPORTED_TX);
}

static void configurationComplete(void)
{
  emberAfRf4ceGdpConfigurationComplete(recipientPairingIndex,
                                       EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0,
                                       EMBER_RF4CE_NULL_VENDOR_ID,
                                       EMBER_AF_RF4CE_GDP_STATUS_SUCCESSFUL);
  startOriginatorTimer(ZRC_STATE_ORIGINATOR_CONFIGURATION_COMPLETE);
}

bool incomingActionCodesResponseIsValid(void)
{
  EmberAfRf4ceGdpAttributeStatusRecord record;
  uint8_t i;

  for (i=0; i<ACTION_CODES_SUPPORTED_RECORDS_MAX; i++) {
    if (pendingGetActionCodes[i] < 0xFFFF
        && (!emAfRf4ceGdpFetchAttributeStatusRecord(&record)
            || record.attributeId != EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_CODES_SUPPORTED_RX
            || record.status != EMBER_AF_RF4CE_GDP_ATTRIBUTE_STATUS_SUCCESS
            || record.entryId != pendingGetActionCodes[i])) {
      return false;
    }
  }

  return true;
}

#else // EMBER_AF_RF4CE_ZRC_IS_ORIGINATOR

EmberStatus emberAfRf4ceZrc20Bind(EmberAfRf4ceDeviceType searchDevType)
{
  return EMBER_INVALID_CALL;
}

EmberStatus emberAfRf4ceZrc20ProxyBind(EmberPanId panId,
                                       EmberEUI64 ieeeAddr)
{
  return EMBER_INVALID_CALL;
}

void emAfRf4ceZrc20StartConfigurationOriginator(uint8_t pairingIndex)
{
}

void emAfRf4ceZrcIncomingGenericResponseBindingOriginatorCallback(EmberAfRf4ceGdpResponseCode responseCode)
{
}

void emberAfPluginRf4ceZrc20OriginatorEventHandler(void)
{
  emberEventControlSetInactive(emberAfPluginRf4ceZrc20OriginatorEventControl);
}

#endif // EMBER_AF_RF4CE_ZRC_IS_ORIGINATOR
