// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"

#if defined(EMBER_SCRIPTED_TEST)
#include "rf4ce-zrc20-test.h"
#endif // EMBER_SCRIPTED_TEST

#include "app/framework/plugin/rf4ce-profile/rf4ce-profile.h"
#include "app/framework/plugin/rf4ce-gdp/rf4ce-gdp.h"
#include "app/framework/plugin/rf4ce-gdp/rf4ce-gdp-attributes.h"
#include "app/framework/plugin/rf4ce-gdp/rf4ce-gdp-internal.h"
#include "rf4ce-zrc20.h"
#include "rf4ce-zrc20-attributes.h"
#include "rf4ce-zrc20-internal.h"

#if defined(EMBER_SCRIPTED_TEST)
#define HIDDEN
uint32_t localNodeZrcCapabilities;
#else
#define HIDDEN static
#endif // EMBER_SCRIPTED_TEST

// This code handles all the attributes-related commands that involve ZRC 2.0
// attributes.

//------------------------------------------------------------------------------
// External declarations.

#if (defined(EMBER_AF_RF4CE_ZRC_IS_RECIPIENT) || defined(EMBER_SCRIPTED_TEST))
bool emAfRf4ceZrcIncomingPushAttributesRecipientCallback(void);
bool emAfRf4ceZrcIncomingGetAttributesRecipientCallback(void);
#endif // EMBER_AF_RF4CE_ZRC_IS_RECIPIENT

#if (defined(EMBER_AF_RF4CE_ZRC_IS_ORIGINATOR) || defined(EMBER_SCRIPTED_TEST))
bool emAfRf4ceZrcIncomingGetAttributesResponseOriginatorCallback(void);
#endif // EMBER_AF_RF4CE_ZRC_IS_ORIGINATOR

#if defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_IS_ACTION_MAPPING_CLIENT) || defined(EMBER_SCRIPTED_TEST)
bool emAfRf4ceZrcIncomingPullAttributesResponseActionMappingClientCallback(void);
#endif // EMBER_AF_PLUGIN_RF4CE_ZRC20_IS_ACTION_MAPPING_CLIENT

#if defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_IS_ACTION_MAPPING_SERVER) || defined(EMBER_SCRIPTED_TEST)
bool emAfRf4ceZrcIncomingPushAttributesActionMappingServerCallback(void);
bool emAfRf4ceZrcIncomingPullAttributesActionMappingServerCallback(void);
#endif // EMBER_AF_PLUGIN_RF4CE_ZRC20_IS_ACTION_MAPPING_SERVER

#if defined(EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_RECIPIENT) || defined(EMBER_SCRIPTED_TEST)
bool emAfRf4ceZrcIncomingPushAttributesHomeAutomationRecipientCallback(void);
bool emAfRf4ceZrcIncomingPullAttributesHomeAutomationRecipientCallback(void);
#endif // EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_RECIPIENT

#if defined(EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_ORIGINATOR) || defined(EMBER_SCRIPTED_TEST)
bool emAfRf4ceZrcIncomingPullAttributesResponseHomeAutomationOriginatorCallback(void);
#endif // EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_ORIGINATOR

//------------------------------------------------------------------------------
// Forward declarations and local enums.

static const EmAfRf4ceZrcAttributeDescriptor *getAttributeDescriptor(uint8_t attrId);
static uint16_t getAttributeSize(uint8_t attributeId,
                               uint8_t pairingIndex,
                               uint16_t entryId);
static bool isAttributeSupported(uint8_t attributeId,
                                    bool isLocal);
static void handleIncomingGetOrPullAttributesCommand(bool isGet);
static void handleIncomingGetOrPullAttributesResponseCommand(bool isGet);
static void handleIncomingSetOrPushAttributesCommand(bool isSet);

//------------------------------------------------------------------------------
// Local and remote attribute related variables.

// Local node's ZRC attributes.
EmAfRf4ceZrcAttributes emAfRf4ceZrcLocalNodeAttributes;

// Remote node's ZRC attributes.
EmAfRf4ceZrcAttributes emAfRf4ceZrcRemoteNodeAttributes;

// Local attributes: all the attributes handled here have only push/get rights.
// This implies that the local attributes are read only. Therefore they are all
// stored in CONST.
#if defined(EMBER_AF_RF4CE_ZRC_ACTION_BANKS_RX) || defined(EMBER_SCRIPTED_TEST)
HIDDEN const EmAfZrcBitmask localActionBanksSupportedRx = {EMBER_AF_RF4CE_ZRC_ACTION_BANKS_RX};
#endif // EMBER_AF_RF4CE_ZRC_ACTION_BANKS_RX

#if defined(EMBER_AF_RF4CE_ZRC_ACTION_BANKS_TX) || defined(EMBER_SCRIPTED_TEST)
HIDDEN const EmAfZrcBitmask localActionBanksSupportedTx = {EMBER_AF_RF4CE_ZRC_ACTION_BANKS_TX};
#endif // EMBER_AF_RF4CE_ZRC_ACTION_BANKS_TX

#if defined(EMBER_AF_RF4CE_ZRC_ACTION_CODES_RX) || defined(EMBER_SCRIPTED_TEST)
HIDDEN const EmAfZrcArrayedBitmask localActionCodesSupportedRx[] = EMBER_AF_RF4CE_ZRC_ACTION_CODES_RX;
#endif // EMBER_AF_RF4CE_ZRC_ACTION_CODES_RX

#if defined(EMBER_AF_RF4CE_ZRC_ACTION_CODES_TX) || defined(EMBER_SCRIPTED_TEST)
HIDDEN const EmAfZrcArrayedBitmask localActionCodesSupportedTx[] = EMBER_AF_RF4CE_ZRC_ACTION_CODES_TX;
#endif // EMBER_AF_RF4CE_ZRC_ACTION_CODES_TX

#if defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_LOCAL_IRDB_VENDOR_ATTRIBUTE_SUPPORT) || defined(EMBER_SCRIPTED_TEST)
HIDDEN const uint16_t localIRDBVendorSupport[] = EMBER_AF_RF4CE_ZRC_IRDB_VENDOR_IDS;
#endif // EMBER_AF_PLUGIN_RF4CE_ZRC20_LOCAL_IRDB_VENDOR_ATTRIBUTE_SUPPORT

#if defined(EMBER_AF_RF4CE_ZRC_ACTION_BANKS_TX) || defined(EMBER_SCRIPTED_TEST)
// We define the action banks and codes RX for the remote node only if the local
// node is an action originator.
HIDDEN EmAfZrcBitmask remoteActionBanksSupportedRx;
HIDDEN EmAfZrcArrayedBitmask remoteActionCodesSupportedRx[EMBER_AF_PLUGIN_RF4CE_ZRC20_REMOTE_ACTION_CODES_TABLE_SIZE];
#endif // EMBER_AF_RF4CE_ZRC_ACTION_BANKS_TX

#if defined(EMBER_AF_RF4CE_ZRC_ACTION_BANKS_RX) || defined(EMBER_SCRIPTED_TEST)
// We define the action banks and codes TX for the remote node only if the local
// node is an action recipient.
HIDDEN EmAfZrcBitmask remoteActionBanksSupportedTx;
HIDDEN EmAfZrcArrayedBitmask remoteActionCodesSupportedTx[EMBER_AF_PLUGIN_RF4CE_ZRC20_REMOTE_ACTION_CODES_TABLE_SIZE];
#endif // EMBER_AF_RF4CE_ZRC_ACTION_BANKS_RX

#if defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_REMOTE_IRDB_VENDOR_ATTRIBUTES_SUPPORT) || defined(EMBER_SCRIPTED_TEST)
HIDDEN uint16_t remoteIRDBVendorSupport[EMBER_AF_PLUGIN_RF4CE_ZRC20_REMOTE_IRDB_VENDORS_SUPPORTED_TABLE_SIZE];
#endif // EMBER_AF_PLUGIN_RF4CE_ZRC20_REMOTE_IRDB_VENDOR_ATTRIBUTES_SUPPORT

#if (defined(EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_ORIGINATOR) || defined (EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_RECIPIENT) || defined(EMBER_SCRIPTED_TEST))
// This is used to avoid multiple calls to the
// emberAfPluginRf4ceZrc20GetHomeAutomationAttributeCallback() callback.
static EmberAfRf4ceZrcHomeAutomationAttribute tempHaAttribute;
#endif // EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_RECIPIENT

//------------------------------------------------------------------------------
// Local macros.

#define attributeHasRemoteSetAccess(attrId)                                    \
    ((getAttributeDescriptor(attrId)->bitmask                                  \
      & ZRC_ATTRIBUTE_HAS_REMOTE_SET_ACCESS_BIT) > 0)

#define attributeHasRemoteGetAccess(attrId)                                    \
    ((getAttributeDescriptor(attrId)->bitmask                                  \
      & ZRC_ATTRIBUTE_HAS_REMOTE_GET_ACCESS_BIT) > 0)

#define attributeHasRemotePullAccess(attrId)                                   \
    ((getAttributeDescriptor(attrId)->bitmask                                  \
      & ZRC_ATTRIBUTE_HAS_REMOTE_PULL_ACCESS_BIT) > 0)

#define attributeHasRemotePushAccess(attrId)                                   \
    ((getAttributeDescriptor(attrId)->bitmask                                  \
      & ZRC_ATTRIBUTE_HAS_REMOTE_PUSH_ACCESS_BIT) > 0)

// -----------------------------------------------------------------------------
// Init function.

// Initialize the local ZRC 2.0 attributes and set up the pointers for the
// remote attributes.
void emAfRf4ceZrc20AttributesInit(void)
{
  uint8_t i;

  // ZRC version
  emAfRf4ceZrcLocalNodeAttributes.zrcProfileVersion =
      APL_ZRC_PROFILE_VERSION_DEFAULT;
  // Action banks version
  emAfRf4ceZrcLocalNodeAttributes.zrcActionBanksVersion =
      EMBER_AF_PLUGIN_RF4CE_ZRC20_ACTION_BANKS_VERSION;

#if defined(EMBER_AF_RF4CE_ZRC_ACTION_BANKS_RX) || defined(EMBER_SCRIPTED_TEST)
  // Action banks supported RX
  emAfRf4ceZrcLocalNodeAttributes.actionBanksSupportedRx =
      (EmAfZrcBitmask*)&localActionBanksSupportedRx;
#endif // EMBER_AF_RF4CE_ZRC_ACTION_BANKS_RX

#if defined(EMBER_AF_RF4CE_ZRC_ACTION_BANKS_TX) || defined(EMBER_SCRIPTED_TEST)
  // Action banks supported TX
  emAfRf4ceZrcLocalNodeAttributes.actionBanksSupportedTx =
      (EmAfZrcBitmask*)&localActionBanksSupportedTx;
#endif // EMBER_AF_RF4CE_ZRC_ACTION_BANKS_TX

#if defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_LOCAL_IRDB_VENDOR_ATTRIBUTE_SUPPORT) || defined(EMBER_SCRIPTED_TEST)
  // Supported vendor specific IRDB
  emAfRf4ceZrcLocalNodeAttributes.IRDBVendorSupport =
      (uint16_t*)localIRDBVendorSupport;
#endif // EMBER_AF_PLUGIN_RF4CE_ZRC20_LOCAL_IRDB_VENDOR_ATTRIBUTE_SUPPORT

#if defined(EMBER_AF_RF4CE_ZRC_ACTION_CODES_RX) || defined(EMBER_SCRIPTED_TEST)
  // Action codes supported RX
  emAfRf4ceZrcLocalNodeAttributes.actionCodesSupportedRx =
      (EmAfZrcArrayedBitmask*)localActionCodesSupportedRx;
#endif // EMBER_AF_RF4CE_ZRC_ACTION_CODES_RX

#if defined(EMBER_AF_RF4CE_ZRC_ACTION_CODES_TX) || defined(EMBER_SCRIPTED_TEST)
  // Action codes supported TX
  emAfRf4ceZrcLocalNodeAttributes.actionCodesSupportedTx =
      (EmAfZrcArrayedBitmask*)localActionCodesSupportedTx;
#endif // EMBER_AF_RF4CE_ZRC_ACTION_CODES_TX

  // Setting up pointers for the remote attributes

#if defined(EMBER_AF_RF4CE_ZRC_ACTION_BANKS_TX) || defined(EMBER_SCRIPTED_TEST)
  emAfRf4ceZrcRemoteNodeAttributes.actionBanksSupportedRx =
      &remoteActionBanksSupportedRx;
  emAfRf4ceZrcRemoteNodeAttributes.actionCodesSupportedRx =
      remoteActionCodesSupportedRx;
#endif // EMBER_AF_RF4CE_ZRC_ACTION_BANKS_TX

#if defined(EMBER_AF_RF4CE_ZRC_ACTION_BANKS_RX) || defined(EMBER_SCRIPTED_TEST)
  emAfRf4ceZrcRemoteNodeAttributes.actionBanksSupportedTx =
      &remoteActionBanksSupportedTx;
  emAfRf4ceZrcRemoteNodeAttributes.actionCodesSupportedTx =
      remoteActionCodesSupportedTx;
#endif // EMBER_AF_RF4CE_ZRC_ACTION_BANKS_RX

  // Initialize all the remote action codes entries to 'not in use'.
  for (i=0; i<EMBER_AF_PLUGIN_RF4CE_ZRC20_REMOTE_ACTION_CODES_TABLE_SIZE; i++) {
#if defined(EMBER_AF_RF4CE_ZRC_ACTION_BANKS_TX) || defined(EMBER_SCRIPTED_TEST)
    remoteActionCodesSupportedRx[i].inUse = false;
#endif // EMBER_AF_RF4CE_ZRC_ACTION_BANKS_TX

#if defined(EMBER_AF_RF4CE_ZRC_ACTION_BANKS_RX) || defined(EMBER_SCRIPTED_TEST)
    remoteActionCodesSupportedTx[i].inUse = false;
#endif // EMBER_AF_RF4CE_ZRC_ACTION_BANKS_RX
  }

#if defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_REMOTE_IRDB_VENDOR_ATTRIBUTES_SUPPORT) || defined(EMBER_SCRIPTED_TEST)
  for (i=0; i<EMBER_AF_PLUGIN_RF4CE_ZRC20_REMOTE_ACTION_CODES_TABLE_SIZE; i++) {
    remoteIRDBVendorSupport[i] = EMBER_RF4CE_NULL_VENDOR_ID;
  }
  emAfRf4ceZrcRemoteNodeAttributes.IRDBVendorSupport = remoteIRDBVendorSupport;
#endif // EMBER_AF_PLUGIN_RF4CE_ZRC20_REMOTE_IRDB_VENDOR_ATTRIBUTES_SUPPORT
}

// ------------------------------------------------------------------------------
// Incoming attributes-related commands handlers.

void emAfRf4ceZrcIncomingGetAttributes(void)
{
  bool processCommand = false;

#if (defined(EMBER_AF_RF4CE_ZRC_IS_RECIPIENT) || defined(EMBER_SCRIPTED_TEST))
  // See if the binding code wants to accept this GetAttributes().
  processCommand = emAfRf4ceZrcIncomingGetAttributesRecipientCallback();
#endif // EMBER_AF_RF4CE_ZRC_IS_RECIPIENT

  if (processCommand) {
    handleIncomingGetOrPullAttributesCommand(true);
  }
}

void emAfRf4ceZrcIncomingGetAttributesResponse(void)
{
  bool processCommand = false;

#if (defined(EMBER_AF_RF4CE_ZRC_IS_ORIGINATOR) || defined(EMBER_SCRIPTED_TEST))
  // See if the binding code wants to accept this GetAttributesResponse().
  processCommand = emAfRf4ceZrcIncomingGetAttributesResponseOriginatorCallback();
#endif // EMBER_AF_RF4CE_ZRC_IS_ORIGINATOR

  if (processCommand) {
    handleIncomingGetOrPullAttributesResponseCommand(true);
  }
}

void emAfRf4ceZrcIncomingPushAttributes(void)
{
  bool processCommand = false;

#if (defined(EMBER_AF_RF4CE_ZRC_IS_RECIPIENT) || defined(EMBER_SCRIPTED_TEST))
  // See if the binding code wants to accept this PushAttributes().
  processCommand = emAfRf4ceZrcIncomingPushAttributesRecipientCallback();
#endif // EMBER_AF_RF4CE_ZRC_IS_RECIPIENT

#if defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_IS_ACTION_MAPPING_SERVER) || defined(EMBER_SCRIPTED_TEST)
  // See if the Action Mapping Server code wants to accept this PushAttributes().
  emAfRf4ceGdpResetFetchAttributeFinger();
  processCommand = (processCommand
                    || emAfRf4ceZrcIncomingPushAttributesActionMappingServerCallback());
#endif

#if defined(EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_RECIPIENT) || defined(EMBER_SCRIPTED_TEST)
  // See if the Home Automation Announcement recipient code wants to accept this PushAttributes().
  emAfRf4ceGdpResetFetchAttributeFinger();
  processCommand = (processCommand
                    || emAfRf4ceZrcIncomingPushAttributesHomeAutomationRecipientCallback());
#endif // EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_RECIPIENT

  if (processCommand) {
    handleIncomingSetOrPushAttributesCommand(false);
  }
}

void emAfRf4ceZrcIncomingSetAttributes(void)
{
}

void emAfRf4ceZrcIncomingPullAttributes(void)
{
  bool processCommand = false;

#if defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_IS_ACTION_MAPPING_SERVER) || defined(EMBER_SCRIPTED_TEST)
  // See if the Action Mapping Server code wants to accept this PullAttributes().
  processCommand = emAfRf4ceZrcIncomingPullAttributesActionMappingServerCallback();
#endif

#if defined(EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_RECIPIENT) || defined(EMBER_SCRIPTED_TEST)
  emAfRf4ceGdpResetFetchAttributeFinger();
  // See if the Home Automation recipient code wants to accept this PullAttributes().
  processCommand = (processCommand
                    || emAfRf4ceZrcIncomingPullAttributesHomeAutomationRecipientCallback());
#endif

  if (processCommand) {
    handleIncomingGetOrPullAttributesCommand(false);
  }
}

void emAfRf4ceZrcIncomingPullAttributesResponse(void)
{
  bool processCommand = false;

#if defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_IS_ACTION_MAPPING_CLIENT) || defined(EMBER_SCRIPTED_TEST)
  // See if the Action Mapping Client Code wants to accept this
  // PullAttributesResponse().
  processCommand = emAfRf4ceZrcIncomingPullAttributesResponseActionMappingClientCallback();
#endif // EMBER_AF_PLUGIN_RF4CE_ZRC20_IS_ACTION_MAPPING_CLIENT

#if defined(EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_ORIGINATOR) || defined(EMBER_SCRIPTED_TEST)
  // See if the Home Automation recipient code wants to accept this PullAttributes().
  emAfRf4ceGdpResetFetchAttributeFinger();
  processCommand = (processCommand
                    || emAfRf4ceZrcIncomingPullAttributesResponseHomeAutomationOriginatorCallback());
#endif

  if (processCommand) {
    handleIncomingGetOrPullAttributesResponseCommand(false);
  }
}

//------------------------------------------------------------------------------
// Internal APIs.

// A pairing index value of 0xFF means "local attributes".
void emAfRf4ceZrcReadOrWriteAttribute(uint8_t pairingIndex,
                                      uint8_t attrId,
                                      uint16_t entryIdOrValueLength,
                                      bool isRead,
                                      uint8_t *val)
{
  bool localAttributes = (pairingIndex == 0xFF);
  EmAfRf4ceZrcAttributes *attributes = (localAttributes)
                                       ? &emAfRf4ceZrcLocalNodeAttributes
                                       : &emAfRf4ceZrcRemoteNodeAttributes;

  if (isRead) {
    switch (attrId) {
    case EMBER_AF_RF4CE_ZRC_ATTRIBUTE_VERSION:
      val[0] = LOW_BYTE(attributes->zrcProfileVersion);
      val[1] = HIGH_BYTE(attributes->zrcProfileVersion);
      break;
    case EMBER_AF_RF4CE_ZRC_ATTRIBUTE_CAPABILITIES:
    {
      uint32_t cababilities = ((pairingIndex == 0xFF)
                             ? emAfRf4ceZrcGetLocalNodeCapabilities()
                             : emAfRf4ceZrcGetRemoteNodeCapabilities(pairingIndex));
      val[0] = BYTE_0(cababilities);
      val[1] = BYTE_1(cababilities);
      val[2] = BYTE_2(cababilities);
      val[3] = BYTE_3(cababilities);
    }
      break;
    case EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_BANKS_SUPPORTED_RX:
      MEMCOPY(val, attributes->actionBanksSupportedRx->contents, ZRC_BITMASK_SIZE);
      break;
    case EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_BANKS_SUPPORTED_TX:
      MEMCOPY(val, attributes->actionBanksSupportedTx->contents, ZRC_BITMASK_SIZE);
      break;
#if (defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_LOCAL_IRDB_VENDOR_ATTRIBUTE_SUPPORT)       \
     || defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_REMOTE_IRDB_VENDOR_ATTRIBUTES_SUPPORT)  \
     || defined(EMBER_SCRIPTED_TEST))
    case EMBER_AF_RF4CE_ZRC_ATTRIBUTE_IRDB_VENDOR_SUPPORT:
    {
      // Can't just perform a MEMCOPY, endianness matters.
      uint8_t i;
      uint8_t entriesNum = (localAttributes
                          ? EMBER_AF_RF4CE_ZRC_IRDB_VENDOR_ID_COUNT
                          : EMBER_AF_PLUGIN_RF4CE_ZRC20_REMOTE_IRDB_VENDORS_SUPPORTED_TABLE_SIZE);

      // Non-valid entries are set to EMBER_RF4CE_NULL_VENDOR_ID.
      for(i=0; (i<entriesNum
                && attributes->IRDBVendorSupport[i] != EMBER_RF4CE_NULL_VENDOR_ID);
          i++) {
        val[2*i] = LOW_BYTE(attributes->IRDBVendorSupport[i]);
        val[2*i+1] = HIGH_BYTE(attributes->IRDBVendorSupport[i]);
      }
    }
      break;
#endif // EMBER_AF_PLUGIN_RF4CE_ZRC20_LOCAL_IRDB_VENDOR_ATTRIBUTE_SUPPORT || EMBER_AF_PLUGIN_RF4CE_ZRC20_REMOTE_IRDB_VENDOR_ATTRIBUTES_SUPPORT
    case EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_BANKS_VERSION:
      val[0] = LOW_BYTE(attributes->zrcActionBanksVersion);
      val[1] = HIGH_BYTE(attributes->zrcActionBanksVersion);
      break;
    case EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_CODES_SUPPORTED_RX:
#if (defined (EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_RECIPIENT) || defined(EMBER_SCRIPTED_TEST))
      // RX action codes related to HA action banks should be all 0xFF if the
      // node is an HA actions recipient.
      if (entryIdOrValueLength >= EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_INSTANCE_0
          && entryIdOrValueLength <= EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_INSTANCE_31) {
        MEMSET(val, 0xFF, ZRC_BITMASK_SIZE);
        break;
      }
      // Fall-through here
#endif // EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_RECIPIENT
    case EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_CODES_SUPPORTED_TX:
    {
      uint8_t *attrPtr = emAfRf4ceZrcGetActionCodesAttributePointer(attrId,
                                                                  entryIdOrValueLength,
                                                                  pairingIndex);
      assert(attrPtr != NULL);
      MEMCOPY(val, attrPtr, ZRC_BITMASK_SIZE);
      break;
    }
#if defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_ACTION_MAPPING_SUPPORT) || defined(EMBER_SCRIPTED_TEST)
    case EMBER_AF_RF4CE_ZRC_ATTRIBUTE_MAPPABLE_ACTIONS:
    {
      EmberAfRf4ceZrcMappableAction mappableAction;
      assert (emberAfPluginRf4ceZrc20GetMappableActionCallback(pairingIndex,
                                                               entryIdOrValueLength,
                                                               &mappableAction)
              == EMBER_SUCCESS);

      val[MAPPABLE_ACTION_ACTION_DEVICE_TYPE_OFFSET] = mappableAction.actionDeviceType;
      val[MAPPABLE_ACTION_ACTION_BANK_OFFSET] = mappableAction.actionBank;
      val[MAPPABLE_ACTION_ACTION_CODE_OFFSET] = mappableAction.actionCode;
    }
      break;
    case EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_MAPPINGS:
    {
      EmberAfRf4ceZrcActionMapping actionMapping;
      assert (emberAfPluginRf4ceZrc20GetActionMappingCallback(pairingIndex,
                                                              entryIdOrValueLength,
                                                              &actionMapping)
              == EMBER_SUCCESS);

      *val++ = actionMapping.mappingFlags;
      if (READBITS(actionMapping.mappingFlags,
                   (EMBER_AF_RF4CE_ZRC_ACTION_MAPPING_MAPPING_FLAGS_RF_SPECIFIED_BIT
                     | EMBER_AF_RF4CE_ZRC_ACTION_MAPPING_MAPPING_FLAGS_USE_DEFAULT_BIT))
          == EMBER_AF_RF4CE_ZRC_ACTION_MAPPING_MAPPING_FLAGS_RF_SPECIFIED_BIT) {
        *val++ = actionMapping.rfConfig;
        *val++ = actionMapping.rf4ceTxOptions;
        *val++ = actionMapping.actionDataLength;
        assert(actionMapping.actionData != NULL);
        MEMCOPY(val, actionMapping.actionData, actionMapping.actionDataLength);
        val += actionMapping.actionDataLength;
      }

      if (READBITS(actionMapping.mappingFlags,
                   (EMBER_AF_RF4CE_ZRC_ACTION_MAPPING_MAPPING_FLAGS_IR_SPECIFIED_BIT
                     | EMBER_AF_RF4CE_ZRC_ACTION_MAPPING_MAPPING_FLAGS_USE_DEFAULT_BIT))
          == EMBER_AF_RF4CE_ZRC_ACTION_MAPPING_MAPPING_FLAGS_IR_SPECIFIED_BIT) {
        *val++ = actionMapping.irConfig;
        if (actionMapping.irConfig
            & ACTION_MAPPING_IR_CONFIG_VENDOR_SPECIFIC_BIT) {
          *val++ = LOW_BYTE(actionMapping.irVendorId);
          *val++ = HIGH_BYTE(actionMapping.irVendorId);
        }
        *val++ = actionMapping.irCodeLength;
        assert(actionMapping.irCode != NULL);
        MEMCOPY(val, actionMapping.irCode, actionMapping.irCodeLength);
      }
    }
      break;
#endif // EMBER_AF_PLUGIN_RF4CE_ZRC20_ACTION_MAPPING_SUPPORT

#if (defined(EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_ORIGINATOR)             \
     || defined (EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_RECIPIENT)          \
     || defined(EMBER_SCRIPTED_TEST))
    case EMBER_AF_RF4CE_ZRC_ATTRIBUTE_HOME_AUTOMATION:
      // This attribute can only be pulled and is stored at the application.
      // This plugin can only access this attribute via callback. As result,
      // we will get here only after having called (and it returns 'success')
      // emAfRf4ceZrcIsArrayedAttributeSupported() in the pullAttributes()
      // handler function. emAfRf4ceZrcIsArrayedAttributeSupported() saves the
      // contents in the static object tempHaAttribute. So we just copy it
      // here.
      assert(tempHaAttribute.contentsLength < (MAX_ZRC_ATTRIBUTE_SIZE - 1));
      *val++ = HA_ATTRIBUTE_STATUS_VALUE_AVAILABLE_FLAG;
      MEMCOPY(val, tempHaAttribute.contents, tempHaAttribute.contentsLength);
      break;
    case EMBER_AF_RF4CE_ZRC_ATTRIBUTE_HOME_AUTOMATION_SUPPORTED:
      emberAfPluginRf4ceZrc20GetHomeAutomationSupportedCallback(pairingIndex,
                                                                entryIdOrValueLength,
                                                                (EmberAfRf4ceZrcHomeAutomationSupported*)val);
      break;
#endif // EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_ORIGINATOR || EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_RECIPIENT
    }
  } else { // !isRead
    switch (attrId) {
    case EMBER_AF_RF4CE_ZRC_ATTRIBUTE_VERSION:
      attributes->zrcProfileVersion = HIGH_LOW_TO_INT(val[1], val[0]);
      break;
    case EMBER_AF_RF4CE_ZRC_ATTRIBUTE_CAPABILITIES:
      // We only allow setting remote nodes capabilities.
      if (pairingIndex < 0xFF) {
        emAfRf4ceZrcSetRemoteNodeCapabilities(pairingIndex,
                                              emberFetchLowHighInt32u(val));
      }
      break;
    case EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_BANKS_SUPPORTED_RX:
      MEMCOPY(attributes->actionBanksSupportedRx->contents, val, ZRC_BITMASK_SIZE);
      break;
    case EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_BANKS_SUPPORTED_TX:
      MEMCOPY(attributes->actionBanksSupportedTx->contents, val, ZRC_BITMASK_SIZE);
      break;
#if defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_REMOTE_IRDB_VENDOR_ATTRIBUTES_SUPPORT) || defined(EMBER_SCRIPTED_TEST)
    case EMBER_AF_RF4CE_ZRC_ATTRIBUTE_IRDB_VENDOR_SUPPORT:
    {
      // Only remote IRDB vendor support attributes can written.
      uint8_t i;
      for (i=0; i < EMBER_AF_PLUGIN_RF4CE_ZRC20_REMOTE_IRDB_VENDORS_SUPPORTED_TABLE_SIZE; i++) {
        uint16_t vendorId;
        if (i < entryIdOrValueLength / 2) {
          vendorId = HIGH_LOW_TO_INT(val[2*i+1], val[2*i]);
        } else {
          // The rest of the entries are set to NULL_VENDOR_ID.
          vendorId = EMBER_RF4CE_NULL_VENDOR_ID;
        }

        attributes->IRDBVendorSupport[i] = vendorId;

#if defined(EMBER_SCRIPTED_TEST)
        simpleScriptCheck("", "Remote IRDB attribute, entry set", "ii",
                          i,
                          vendorId);
#endif // EMBER_SCRIPTED_TEST
      }
    }
      break;
#endif // EMBER_AF_PLUGIN_RF4CE_ZRC20_REMOTE_IRDB_VENDOR_ATTRIBUTES_SUPPORT
    case EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_BANKS_VERSION:
      attributes->zrcActionBanksVersion = HIGH_LOW_TO_INT(val[1], val[0]);
      break;
    case EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_CODES_SUPPORTED_RX:
    case EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_CODES_SUPPORTED_TX:
    {
      uint8_t *attrPtr = emAfRf4ceZrcGetActionCodesAttributePointer(attrId,
                                                                  entryIdOrValueLength,
                                                                  pairingIndex);
      assert(attrPtr != NULL);
      MEMCOPY(attrPtr, val, ZRC_BITMASK_SIZE);
      break;
    }

#if defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_ACTION_MAPPING_SUPPORT) || defined(EMBER_SCRIPTED_TEST)
    case EMBER_AF_RF4CE_ZRC_ATTRIBUTE_MAPPABLE_ACTIONS:
    {
      EmberAfRf4ceZrcMappableAction mappableAction;
      mappableAction.actionDeviceType = val[0];
      mappableAction.actionBank = val[1];
      mappableAction.actionCode = val[2];
      emberAfPluginRf4ceZrc20IncomingMappableActionCallback(emberAfRf4ceGetPairingIndex(),
                                                            entryIdOrValueLength,
                                                            &mappableAction);
    }
      break;
    case EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_MAPPINGS:
    {
      EmberAfRf4ceZrcActionMapping actionMapping;
      uint8_t *finger = val;
      actionMapping.actionData = NULL;
      actionMapping.irCode = NULL;

      // Mapping flags
      actionMapping.mappingFlags = *finger++;

      // RF descriptor
      if (READBITS(actionMapping.mappingFlags,
                   (EMBER_AF_RF4CE_ZRC_ACTION_MAPPING_MAPPING_FLAGS_RF_SPECIFIED_BIT
                     | EMBER_AF_RF4CE_ZRC_ACTION_MAPPING_MAPPING_FLAGS_USE_DEFAULT_BIT))
          == EMBER_AF_RF4CE_ZRC_ACTION_MAPPING_MAPPING_FLAGS_RF_SPECIFIED_BIT) {
        actionMapping.rfConfig = *finger++;
        actionMapping.rf4ceTxOptions = *finger++;
        actionMapping.actionDataLength = *finger++;
        actionMapping.actionData = finger;
        finger += actionMapping.actionDataLength;
      }

      // IR descriptor
      if (READBITS(actionMapping.mappingFlags,
                   (EMBER_AF_RF4CE_ZRC_ACTION_MAPPING_MAPPING_FLAGS_IR_SPECIFIED_BIT
                     | EMBER_AF_RF4CE_ZRC_ACTION_MAPPING_MAPPING_FLAGS_USE_DEFAULT_BIT))
          == EMBER_AF_RF4CE_ZRC_ACTION_MAPPING_MAPPING_FLAGS_IR_SPECIFIED_BIT) {
        actionMapping.irConfig = *finger++;
        if (actionMapping.irConfig
            & ACTION_MAPPING_IR_CONFIG_VENDOR_SPECIFIC_BIT) {
          actionMapping.irVendorId = HIGH_LOW_TO_INT(finger[1], finger[0]);
          finger += 2;
        }
        actionMapping.irCodeLength = *finger++;
        actionMapping.irCode = finger;
      }

      emberAfPluginRf4ceZrc20IncomingActionMappingCallback(emberAfRf4ceGetPairingIndex(),
                                                           entryIdOrValueLength,
                                                           &actionMapping);
    }
      break;
#endif // EMBER_AF_PLUGIN_RF4CE_ZRC20_ACTION_MAPPING_SUPPORT

#if (defined(EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_ORIGINATOR)             \
     || defined (EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_RECIPIENT)          \
     || defined(EMBER_SCRIPTED_TEST))

    // EMBER_AF_RF4CE_ZRC_ATTRIBUTE_HOME_AUTOMATION attributes sets are handled
    // in the HA action code.

    case EMBER_AF_RF4CE_ZRC_ATTRIBUTE_HOME_AUTOMATION_SUPPORTED:
      emberAfPluginRf4ceZrc20IncomingHomeAutomationSupportedCallback(emberAfRf4ceGetPairingIndex(),
                                                                     entryIdOrValueLength,
                                                                     (EmberAfRf4ceZrcHomeAutomationSupported*)val);
      break;
#endif // EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_ORIGINATOR || EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_RECIPIENT
    }
  }
}

bool emAfRf4ceZrcIsArrayedAttributeSupported(uint8_t attrId,
                                                uint16_t entryId,
                                                uint8_t pairingIndex)
{
  if (attrId == EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_CODES_SUPPORTED_RX
      || attrId == EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_CODES_SUPPORTED_TX) {
    return (emAfRf4ceZrcGetActionCodesAttributePointer(attrId,
                                                       entryId,
                                                       pairingIndex) != NULL);
  } else if (attrId == EMBER_AF_RF4CE_ZRC_ATTRIBUTE_MAPPABLE_ACTIONS) {
    // Mappable actions support get and push actions. If the pairing index is
    // 0xFF, means that this is get, otherwise is a push.
    // For pushes we assume the application has always room for a new entry.
    return true;
  } else if (attrId == EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_MAPPINGS) {
    // Action mappings can only be pulled. We return true if the application
    // has a corresponding mappable action.
    return (entryId <
            emberAfPluginRf4ceZrc20GetMappableActionCountCallback(pairingIndex));
  }
#if defined(EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_RECIPIENT) || defined(EMBER_SCRIPTED_TEST)
  else if (attrId == EMBER_AF_RF4CE_ZRC_ATTRIBUTE_HOME_AUTOMATION) {
    return (emberAfPluginRf4ceZrc20GetHomeAutomationAttributeCallback(pairingIndex,
                                                                      LOW_BYTE(entryId),
                                                                      HIGH_BYTE(entryId),
                                                                      &tempHaAttribute)
            == EMBER_AF_RF4CE_GDP_ATTRIBUTE_STATUS_SUCCESS);
  }
#endif // EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_RECIPIENT
  else if (attrId == EMBER_AF_RF4CE_ZRC_ATTRIBUTE_HOME_AUTOMATION_SUPPORTED) {
    // HA supported support get and push actions. If the pairing index is
    // 0xFF, means that this is get, otherwise is a push.
    // For pushes we assume the application has always room for a new entry.
    return true;
  }

  return false;
}

uint8_t *emAfRf4ceZrcGetActionCodesAttributePointer(uint8_t attrId,
                                                  uint16_t entryId,
                                                  uint8_t pairingIndex)
{
  bool isLocalAttribute = (pairingIndex == 0xFF);
  EmAfRf4ceZrcAttributes *attributes = (isLocalAttribute
                                        ? &emAfRf4ceZrcLocalNodeAttributes
                                        : &emAfRf4ceZrcRemoteNodeAttributes);
  EmAfZrcArrayedBitmask *actionCodesSupported;
  uint8_t tableSize, i;

  if (attrId == EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_CODES_SUPPORTED_RX) {
    actionCodesSupported = attributes->actionCodesSupportedRx;
    tableSize = (isLocalAttribute)
#if defined(EMBER_AF_RF4CE_ZRC_ACTION_CODES_RX)
                ? EMBER_AF_RF4CE_ZRC_ACTION_CODES_RX_COUNT
#else
                ? 0
#endif // EMBER_AF_RF4CE_ZRC_ACTION_CODES_RX
                : EMBER_AF_PLUGIN_RF4CE_ZRC20_REMOTE_ACTION_CODES_TABLE_SIZE;

  } else {
    actionCodesSupported = attributes->actionCodesSupportedTx;
    tableSize = (isLocalAttribute)
#if defined(EMBER_AF_RF4CE_ZRC_ACTION_CODES_TX)
                ? EMBER_AF_RF4CE_ZRC_ACTION_CODES_TX_COUNT
#else
                ? 0
#endif // EMBER_AF_RF4CE_ZRC_ACTION_CODES_TX_COUNT
                : EMBER_AF_PLUGIN_RF4CE_ZRC20_REMOTE_ACTION_CODES_TABLE_SIZE;
  }

  // If we find an entry in use that corresponds to the passed entry ID we
  // return that. That applies to both local and remote.
  for(i=0; i<tableSize; i++) {
    if (actionCodesSupported[i].inUse
        && actionCodesSupported[i].entryId == entryId) {
      return actionCodesSupported[i].contents;
    }
  }

  // If we are looking for a remote node's action code and we didn't find any
  // entry in use corresponding to the passed entry ID, we return an entry not
  // in use (if any), so that we can write it.
  for(i=0; i<tableSize; i++) {
    if (!isLocalAttribute && !actionCodesSupported[i].inUse) {
      return actionCodesSupported[i].contents;
    }
  }

  return NULL;
}

//------------------------------------------------------------------------------
// Static tables and functions.

// We also maintain a table that stores misc. information about the supported
// attributes.
static const EmAfRf4ceZrcAttributeDescriptor attributesInfo[ZRC_ATTRIBUTES_COUNT] = {

    {EMBER_AF_RF4CE_ZRC_ATTRIBUTE_VERSION,
     APL_ZRC_PROFILE_VERSION_SIZE,
     (ZRC_ATTRIBUTE_HAS_REMOTE_GET_ACCESS_BIT
      | ZRC_ATTRIBUTE_HAS_REMOTE_PUSH_ACCESS_BIT
      | ZRC_ATTRIBUTE_LOCAL_NODE_SUPPORTED
      | ZRC_ATTRIBUTE_REMOTE_NODE_SUPPORTED)},

    {EMBER_AF_RF4CE_ZRC_ATTRIBUTE_CAPABILITIES,
     APL_ZRC_PROFILE_CAPABILITIES_SIZE,
     (ZRC_ATTRIBUTE_HAS_REMOTE_GET_ACCESS_BIT
      | ZRC_ATTRIBUTE_HAS_REMOTE_PUSH_ACCESS_BIT
      | ZRC_ATTRIBUTE_LOCAL_NODE_SUPPORTED
      | ZRC_ATTRIBUTE_REMOTE_NODE_SUPPORTED)},

    {EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_REPEAT_TRIGGER_INTERVAL,
     APL_ACTION_REPEAT_TRIGGER_INTERVAL_SIZE,
     0},

    {EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_REPEAT_WAIT_TIME,
     APL_ACTION_REPEAT_WAIT_TIME_SIZE,
     0},

    {EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_BANKS_SUPPORTED_RX,
     ZRC_BITMASK_SIZE,
     (ZRC_ATTRIBUTE_HAS_REMOTE_PUSH_ACCESS_BIT
      | ZRC_ATTRIBUTE_HAS_REMOTE_GET_ACCESS_BIT
#if defined(EMBER_AF_RF4CE_ZRC_ACTION_BANKS_RX) || defined(EMBER_SCRIPTED_TEST)
      | ZRC_ATTRIBUTE_LOCAL_NODE_SUPPORTED
#endif

#if defined(EMBER_AF_RF4CE_ZRC_ACTION_BANKS_TX) || defined(EMBER_SCRIPTED_TEST)
      | ZRC_ATTRIBUTE_REMOTE_NODE_SUPPORTED
#endif
      )},

    {EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_BANKS_SUPPORTED_TX,
     ZRC_BITMASK_SIZE,
     (ZRC_ATTRIBUTE_HAS_REMOTE_PUSH_ACCESS_BIT
      | ZRC_ATTRIBUTE_HAS_REMOTE_GET_ACCESS_BIT
#if defined(EMBER_AF_RF4CE_ZRC_ACTION_BANKS_TX) || defined(EMBER_SCRIPTED_TEST)
      | ZRC_ATTRIBUTE_LOCAL_NODE_SUPPORTED
#endif

#if defined(EMBER_AF_RF4CE_ZRC_ACTION_BANKS_RX) || defined(EMBER_SCRIPTED_TEST)
      | ZRC_ATTRIBUTE_REMOTE_NODE_SUPPORTED
#endif
      )},

#if defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_LOCAL_IRDB_VENDOR_ATTRIBUTE_SUPPORT)      \
    || defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_REMOTE_IRDB_VENDOR_ATTRIBUTES_SUPPORT) \
    || defined(EMBER_SCRIPTED_TEST)
    {EMBER_AF_RF4CE_ZRC_ATTRIBUTE_IRDB_VENDOR_SUPPORT,
     0xFF, // variable
     (ZRC_ATTRIBUTE_HAS_REMOTE_GET_ACCESS_BIT
      | ZRC_ATTRIBUTE_HAS_REMOTE_PUSH_ACCESS_BIT
#if defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_LOCAL_IRDB_VENDOR_ATTRIBUTE_SUPPORT) || defined(EMBER_SCRIPTED_TEST)
      | ZRC_ATTRIBUTE_LOCAL_NODE_SUPPORTED
#endif

#if defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_REMOTE_IRDB_VENDOR_ATTRIBUTES_SUPPORT) || defined(EMBER_SCRIPTED_TEST)
      | ZRC_ATTRIBUTE_REMOTE_NODE_SUPPORTED
#endif
     )},
#endif // EMBER_AF_PLUGIN_RF4CE_ZRC20_LOCAL_IRDB_VENDOR_ATTRIBUTE_SUPPORT || EMBER_AF_PLUGIN_RF4CE_ZRC20_REMOTE_IRDB_VENDOR_ATTRIBUTES_SUPPORT

    {EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_BANKS_VERSION,
     APL_ZRC_ACTION_BANKS_VERSION_SIZE,
     (ZRC_ATTRIBUTE_HAS_REMOTE_GET_ACCESS_BIT
      | ZRC_ATTRIBUTE_HAS_REMOTE_PUSH_ACCESS_BIT
      | ZRC_ATTRIBUTE_LOCAL_NODE_SUPPORTED
      | ZRC_ATTRIBUTE_REMOTE_NODE_SUPPORTED)},

    {EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_CODES_SUPPORTED_RX,
     ZRC_BITMASK_SIZE,
     (ZRC_ATTRIBUTE_HAS_REMOTE_PUSH_ACCESS_BIT
      | ZRC_ATTRIBUTE_HAS_REMOTE_GET_ACCESS_BIT
#if defined(EMBER_AF_RF4CE_ZRC_ACTION_BANKS_RX) || defined(EMBER_SCRIPTED_TEST)
      | ZRC_ATTRIBUTE_LOCAL_NODE_SUPPORTED
#endif

#if defined(EMBER_AF_RF4CE_ZRC_ACTION_BANKS_TX) || defined(EMBER_SCRIPTED_TEST)
      | ZRC_ATTRIBUTE_REMOTE_NODE_SUPPORTED
#endif
      )},

    {EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_CODES_SUPPORTED_TX,
     ZRC_BITMASK_SIZE,
     (ZRC_ATTRIBUTE_HAS_REMOTE_PUSH_ACCESS_BIT
      | ZRC_ATTRIBUTE_HAS_REMOTE_GET_ACCESS_BIT
#if defined(EMBER_AF_RF4CE_ZRC_ACTION_BANKS_TX) || defined(EMBER_SCRIPTED_TEST)
      | ZRC_ATTRIBUTE_LOCAL_NODE_SUPPORTED
#endif

#if defined(EMBER_AF_RF4CE_ZRC_ACTION_BANKS_RX) || defined(EMBER_SCRIPTED_TEST)
      | ZRC_ATTRIBUTE_REMOTE_NODE_SUPPORTED
#endif
      )},

#if (defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_IS_ACTION_MAPPING_CLIENT)             \
     || defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_IS_ACTION_MAPPING_SERVER)          \
     || defined(EMBER_SCRIPTED_TEST))
    {EMBER_AF_RF4CE_ZRC_ATTRIBUTE_MAPPABLE_ACTIONS,
     0xFF, // variable
     (ZRC_ATTRIBUTE_HAS_REMOTE_GET_ACCESS_BIT
      | ZRC_ATTRIBUTE_HAS_REMOTE_PUSH_ACCESS_BIT
#if defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_IS_ACTION_MAPPING_CLIENT) || defined(EMBER_SCRIPTED_TEST)
      | ZRC_ATTRIBUTE_LOCAL_NODE_SUPPORTED
#endif

#if defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_IS_ACTION_MAPPING_SERVER) || defined(EMBER_SCRIPTED_TEST)
      | ZRC_ATTRIBUTE_REMOTE_NODE_SUPPORTED
#endif
     )},

    {EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_MAPPINGS,
     0xFF, // variable
     (ZRC_ATTRIBUTE_HAS_REMOTE_PULL_ACCESS_BIT
#if defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_IS_ACTION_MAPPING_CLIENT) || defined(EMBER_SCRIPTED_TEST)
      | ZRC_ATTRIBUTE_LOCAL_NODE_SUPPORTED
#endif

#if defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_IS_ACTION_MAPPING_SERVER) || defined(EMBER_SCRIPTED_TEST)
      | ZRC_ATTRIBUTE_REMOTE_NODE_SUPPORTED
#endif
      )},
#endif // EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_ORIGINATOR || EMBER_AF_PLUGIN_RF4CE_ZRC20_IS_ACTION_MAPPING_SERVER


#if (defined(EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_ORIGINATOR)             \
     || defined (EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_RECIPIENT)          \
     || defined(EMBER_SCRIPTED_TEST))
    {EMBER_AF_RF4CE_ZRC_ATTRIBUTE_HOME_AUTOMATION,
     0xFF, // variable
     (ZRC_ATTRIBUTE_HAS_REMOTE_PULL_ACCESS_BIT
      | ZRC_ATTRIBUTE_IS_TWO_DIMENSIONAL_ARRAYED
#if defined(EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_ORIGINATOR) || defined(EMBER_SCRIPTED_TEST)
      | ZRC_ATTRIBUTE_LOCAL_NODE_SUPPORTED
#endif
#if defined(EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_RECIPIENT) || defined(EMBER_SCRIPTED_TEST)
      | ZRC_ATTRIBUTE_REMOTE_NODE_SUPPORTED
#endif
      )},

    {EMBER_AF_RF4CE_ZRC_ATTRIBUTE_HOME_AUTOMATION_SUPPORTED,
     ZRC_BITMASK_SIZE,
     (ZRC_ATTRIBUTE_HAS_REMOTE_GET_ACCESS_BIT
      | ZRC_ATTRIBUTE_HAS_REMOTE_PUSH_ACCESS_BIT
#if defined(EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_ORIGINATOR) || defined(EMBER_SCRIPTED_TEST)
      | ZRC_ATTRIBUTE_LOCAL_NODE_SUPPORTED
#endif

#if defined(EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_RECIPIENT) || defined(EMBER_SCRIPTED_TEST)
      | ZRC_ATTRIBUTE_REMOTE_NODE_SUPPORTED
#endif
      )},
#endif // EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_ORIGINATOR || EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_RECIPIENT
};

static const EmAfRf4ceZrcAttributeDescriptor *getAttributeDescriptor(uint8_t attrId)
{
  uint8_t i;

  for(i=0; i<ZRC_ATTRIBUTES_COUNT; i++) {
    if (attrId == attributesInfo[i].id) {
      return &(attributesInfo[i]);
    }
  }

  return NULL;
}

static bool isAttributeSupported(uint8_t attributeId,
                                    bool isLocal)
{
  EmAfRf4ceZrcAttributeDescriptor *descriptor =
      (EmAfRf4ceZrcAttributeDescriptor*)getAttributeDescriptor(attributeId);
  return (descriptor != NULL
          && ((isLocal)
              ? (descriptor->bitmask & ZRC_ATTRIBUTE_LOCAL_NODE_SUPPORTED)
              : (descriptor->bitmask & ZRC_ATTRIBUTE_REMOTE_NODE_SUPPORTED)));
}

static uint16_t getAttributeSize(uint8_t attributeId,
                               uint8_t pairingIndex,
                               uint16_t entryId)
{
  switch(attributeId) {

#if (defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_LOCAL_IRDB_VENDOR_ATTRIBUTE_SUPPORT)      \
     || defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_REMOTE_IRDB_VENDOR_ATTRIBUTES_SUPPORT) \
     || defined(EMBER_SCRIPTED_TEST))
  case EMBER_AF_RF4CE_ZRC_ATTRIBUTE_IRDB_VENDOR_SUPPORT:

  // Local attribute
  if (pairingIndex == 0xFF) {
    return EMBER_AF_RF4CE_ZRC_IRDB_VENDOR_ID_COUNT*2;
  }

#if defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_REMOTE_IRDB_VENDOR_ATTRIBUTES_SUPPORT) || defined(EMBER_SCRIPTED_TEST)
  else {
    uint16_t retVal = 0;
    uint8_t i;
    for (i=0; i<EMBER_AF_PLUGIN_RF4CE_ZRC20_REMOTE_IRDB_VENDORS_SUPPORTED_TABLE_SIZE; i++) {
      if (emAfRf4ceZrcRemoteNodeAttributes.IRDBVendorSupport[i]
          != EMBER_RF4CE_NULL_VENDOR_ID) {
        retVal += 2;
      }
    }
    return retVal;
  }
#endif // EMBER_AF_PLUGIN_RF4CE_ZRC20_REMOTE_IRDB_VENDOR_ATTRIBUTES_SUPPORT

#endif // EMBER_AF_PLUGIN_RF4CE_ZRC20_LOCAL_IRDB_VENDOR_ATTRIBUTE_SUPPORT || EMBER_AF_PLUGIN_RF4CE_ZRC20_REMOTE_IRDB_VENDOR_ATTRIBUTES_SUPPORT

#if defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_ACTION_MAPPING_SUPPORT) || defined(EMBER_SCRIPTED_TEST)
  case EMBER_AF_RF4CE_ZRC_ATTRIBUTE_MAPPABLE_ACTIONS:
    return APL_MAPPABLE_ACTIONS_SIZE;
  case EMBER_AF_RF4CE_ZRC_ATTRIBUTE_ACTION_MAPPINGS:
  {
    EmberAfRf4ceZrcActionMapping actionMapping;
    uint16_t retVal = 1; // mapping flags

    if (emberAfPluginRf4ceZrc20GetActionMappingCallback(pairingIndex,
                                                        entryId,
                                                        &actionMapping) != EMBER_SUCCESS) {
      return 0;
    }

    if (READBITS(actionMapping.mappingFlags,
                 (EMBER_AF_RF4CE_ZRC_ACTION_MAPPING_MAPPING_FLAGS_RF_SPECIFIED_BIT
                   | EMBER_AF_RF4CE_ZRC_ACTION_MAPPING_MAPPING_FLAGS_USE_DEFAULT_BIT))
        == EMBER_AF_RF4CE_ZRC_ACTION_MAPPING_MAPPING_FLAGS_RF_SPECIFIED_BIT) {
      // rfConfig (1) + rf4ceTxOptions (1) + actionDataLength (1) + actionData (variable)
      retVal += (3 + actionMapping.actionDataLength);
    }

    if (READBITS(actionMapping.mappingFlags,
                 (EMBER_AF_RF4CE_ZRC_ACTION_MAPPING_MAPPING_FLAGS_IR_SPECIFIED_BIT
                   | EMBER_AF_RF4CE_ZRC_ACTION_MAPPING_MAPPING_FLAGS_USE_DEFAULT_BIT))
        == EMBER_AF_RF4CE_ZRC_ACTION_MAPPING_MAPPING_FLAGS_IR_SPECIFIED_BIT) {
      // irVendorId (2)
      if (actionMapping.irConfig & ACTION_MAPPING_IR_CONFIG_VENDOR_SPECIFIC_BIT) {
        retVal += 2;
      }

      // irConfig (1) + irCodeLength (1) + irCode (variable)
      retVal += 2 + actionMapping.irCodeLength;
    }

    return retVal;
  }
#endif // EMBER_AF_PLUGIN_RF4CE_ZRC20_ACTION_MAPPING_SUPPORT

#if (defined(EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_ORIGINATOR)             \
     || defined (EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_RECIPIENT)          \
     || defined(EMBER_SCRIPTED_TEST))
  case EMBER_AF_RF4CE_ZRC_ATTRIBUTE_HOME_AUTOMATION:
    // This attribute can only be pulled, and are stored at the application.
    // This plugin can only access this attribute via callback. As result,
    // we will get here only after having called (and it returns 'success')
    // emAfRf4ceZrcIsArrayedAttributeSupported() in the pullAttributes()
    // handler function. emAfRf4ceZrcIsArrayedAttributeSupported() saves the
    // contents in the static object tempHaAttribute. So we just copy it
    // here.

    // HA attribute status (1) + HA attribute value (variable)
    return tempHaAttribute.contentsLength + 1;
#endif // EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_ORIGINATOR || EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_RECIPIENT

  default:
    return getAttributeDescriptor(attributeId)->size;
  }
}

static void handleIncomingGetOrPullAttributesCommand(bool isGet)
{
  EmberAfRf4ceGdpAttributeIdentificationRecord idRecord;
  EmberAfRf4ceGdpAttributeStatusRecord statusRecord;

  emAfRf4ceGdpResetFetchAttributeFinger();

  // Set the outgoing message to "GetAttributeResponse" or
  // "PullAttributeResponse".
  emAfRf4ceGdpStartAttributesCommand((isGet)
                                     ? EMBER_AF_RF4CE_GDP_COMMAND_GET_ATTRIBUTES_RESPONSE
                                     : EMBER_AF_RF4CE_GDP_COMMAND_PULL_ATTRIBUTES_RESPONSE);

  while(emAfRf4ceGdpFetchAttributeIdentificationRecord(&idRecord)) {
    EmberAfRf4ceGdpAttributeId attributeId = idRecord.attributeId;
    bool isArrayed = IS_ARRAY_ATTRIBUTE(attributeId);
    uint8_t attributeVal[MAX_ZRC_ATTRIBUTE_SIZE];

    // The GetAttributesResponse command frame shall contain the same number
    // of attribute status records as attribute identifiers included in the
    // command frame.
    statusRecord.attributeId = attributeId;
    statusRecord.value = (uint8_t*)attributeVal;
    statusRecord.entryId = idRecord.entryId;

    // The recipient shall check that the attribute identifier in the
    // attribute identification record corresponds to a supported attribute.
    // If the attribute is not supported, in its response the recipient shall
    // include the attribute identifier, shall set the attribute status field
    // of the corresponding attribute status record to indicated unsupported
    // attribute and shall not include the attribute length and attribute
    // value fields. The recipient shall then move on to the next attribute
    // identifier.
    if (!isAttributeSupported(attributeId, isGet)) {
      statusRecord.status =
          EMBER_AF_RF4CE_GDP_ATTRIBUTE_STATUS_UNSUPPORTED_ATTRIBUTE;
    // Table 9 indicates for each attribute its remote access rights.
    } else if ((isGet && !attributeHasRemoteGetAccess(attributeId))
               || (!isGet && !attributeHasRemotePullAccess(attributeId))) {
      statusRecord.status =
          EMBER_AF_RF4CE_GDP_ATTRIBUTE_STATUS_ILLEGAL_REQUEST;
    } else { // supported attribute with read access right.
      // If the indices in the entry identifier fall outside the supported
      // range for this attribute, in its response the recipient shall
      // indicate an invalid entry.
      if (isArrayed
          && !emAfRf4ceZrcIsArrayedAttributeSupported(attributeId,
                                                      idRecord.entryId,
                                                      ((isGet)
                                                       ? 0xFF
                                                       : emberAfRf4ceGetPairingIndex()))) {
        statusRecord.status = EMBER_AF_RF4CE_GDP_ATTRIBUTE_STATUS_INVALID_ENTRY;
      } else {
        statusRecord.status = EMBER_AF_RF4CE_GDP_ATTRIBUTE_STATUS_SUCCESS;

        statusRecord.valueLength = getAttributeSize(attributeId,
                                                    isGet ? 0xFF : emberAfRf4ceGetPairingIndex(),
                                                    idRecord.entryId);

        if (isGet) {
          emAfRf4ceZrcReadLocalAttribute(attributeId,
                                         statusRecord.entryId,
                                         attributeVal);
        } else {
          emAfRf4ceZrcReadRemoteAttribute(emberAfRf4ceGetPairingIndex(),
                                          attributeId,
                                          statusRecord.entryId,
                                          attributeVal);
        }
      }
    }

    emAfRf4ceGdpAppendAttributeStatusRecord(&statusRecord);
  }

  emAfRf4ceGdpSendAttributesCommand(emberAfRf4ceGetPairingIndex(),
                                    EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0,
                                    EMBER_RF4CE_NULL_VENDOR_ID); // TODO: vendorId
}

static void handleIncomingGetOrPullAttributesResponseCommand(bool isGet)
{
  EmberAfRf4ceGdpAttributeStatusRecord statusRecord;

  emAfRf4ceGdpResetFetchAttributeFinger();

  while(emAfRf4ceGdpFetchAttributeStatusRecord(&statusRecord)) {
    EmberAfRf4ceGdpAttributeId attributeId = statusRecord.attributeId;
    bool isArrayed = IS_ARRAY_ATTRIBUTE(attributeId);

    if (isAttributeSupported(attributeId, !isGet)
        && statusRecord.status == EMBER_AF_RF4CE_GDP_ATTRIBUTE_STATUS_SUCCESS) {
      if (isGet) {
        emAfRf4ceZrcWriteRemoteAttribute(emberAfRf4ceGetPairingIndex(),
                                         attributeId,
                                         (isArrayed) ? statusRecord.entryId : statusRecord.valueLength,
                                         statusRecord.value);
      } else {
        emAfRf4ceZrcWriteLocalAttribute(attributeId,
                                        (isArrayed) ? statusRecord.entryId : statusRecord.valueLength,
                                        statusRecord.value);
      }
    }
  }
}

static void handleIncomingSetOrPushAttributesCommand(bool isSet)
{
  EmberAfRf4ceGdpAttributeRecord record;
 EmberAfRf4ceGdpResponseCode responseCode =
     EMBER_AF_RF4CE_GDP_RESPONSE_CODE_SUCCESSFUL;

 emAfRf4ceGdpResetFetchAttributeFinger();

 while(emAfRf4ceGdpFetchAttributeRecord(&record)) {
   EmberAfRf4ceGdpAttributeId attributeId = record.attributeId;
   bool isArrayed = IS_ARRAY_ATTRIBUTE(attributeId);

   if (!isAttributeSupported(attributeId, isSet)) {
     // If the attribute is not supported, the recipient shall set the
     // response code of the GenericResponse command frame to indicate an
     // invalid parameter and ignore the rest of the frame.
     responseCode = EMBER_AF_RF4CE_GDP_RESPONSE_CODE_INVALID_PARAMETER;
     break;
   // Table 9 indicates for each attribute its remote access rights.
   } else if ((isSet && !attributeHasRemoteSetAccess(attributeId))
              || (!isSet && !attributeHasRemotePushAccess(attributeId))
              || (isArrayed
                  && !emAfRf4ceZrcIsArrayedAttributeSupported(attributeId,
                                                              record.entryId,
                                                              ((isSet)
                                                               ? 0xFF
                                                               : emberAfRf4ceGetPairingIndex())))) {
     // If the attribute record field corresponds to an arrayed attribute,
     // the recipient shall check that the entry identifier in the attribute
     // record addresses an entry that exists in the array. If the indices in
     // the entry identifier fall outside the supported range for the
     // attribute, the recipient shall set the response code of the
     // GenericResponse command frame to indicate an invalid entry and ignore
     // the rest of the frame.
     responseCode = EMBER_AF_RF4CE_GDP_RESPONSE_CODE_UNSUPPORTED_REQUEST;
     break;
   } else {
     // Update the remote attribute value.
     if (isSet) {
       emAfRf4ceZrcWriteLocalAttribute(attributeId,
                                       (isArrayed) ? record.entryId : record.valueLength,
                                       record.value);
     } else {
       emAfRf4ceZrcWriteRemoteAttribute(emberAfRf4ceGetPairingIndex(),
                                        attributeId,
                                        (isArrayed) ? record.entryId : record.valueLength,
                                        record.value);
     }
   }
 }

 emberAfRf4ceGdpGenericResponse(emberAfRf4ceGetPairingIndex(),
                                EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0,
                                EMBER_RF4CE_NULL_VENDOR_ID, // TODO
                                responseCode);
}

