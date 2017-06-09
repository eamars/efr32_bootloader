// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "app/framework/plugin/rf4ce-profile/rf4ce-profile.h"
#include "app/framework/plugin/rf4ce-gdp/rf4ce-gdp.h"
#include "app/framework/plugin/rf4ce-gdp/rf4ce-gdp-internal.h"
#include "rf4ce-zrc20.h"
#include "rf4ce-zrc20-internal.h"

#if defined(EMBER_SCRIPTED_TEST)
#include "rf4ce-zrc20-test.h"
#endif

//------------------------------------------------------------------------------
// Static variables and forward declarations

#if (defined(EMBER_AF_RF4CE_ZRC_IS_ACTIONS_ORIGINATOR) || defined(EMBER_SCRIPTED_TEST))

static EmberAfRf4ceZrcActionRecord recordsTable[EMBER_AF_PLUGIN_RF4CE_ZRC20_MAX_OUTGOING_ACTION_RECORDS];

static void recordsTableInit(void);
static void outgoingEventHandler(void);
static bool addOrUpdateActionRecord(uint8_t pairingIndex,
                                       EmberAfRf4ceZrcActionBank actionBank,
                                       EmberAfRf4ceZrcActionCode actionCode,
                                       EmberAfRf4ceZrcModifierBit actionModifiers,
                                       uint16_t actionVendorId,
                                       const uint8_t *actionData,
                                       uint8_t actionDataLength,
                                       bool atomic);
static EmberStatus sendActionsCommand(uint8_t pairingIndex);
static EmberStatus sendZrc11UserControlCommand(uint8_t recordIndex);
static bool checkActionsCommandSize(uint8_t pairingIndex);
static void updateRecordAfterSend(uint8_t recordIndex);
static void rescheduleOutgoingEvent(void);
static EmberStatus getActionsCommandTxOptions(uint8_t pairingIndex,
                                              EmberRf4ceTxOption *txOptions);
#endif // EMBER_AF_RF4CE_ZRC_IS_ACTIONS_ORIGINATOR

//------------------------------------------------------------------------------
// Events and event handlers.

EmberEventControl emberAfPluginRf4ceZrc20OutgoingEventControl;

void emberAfPluginRf4ceZrc20OutgoingEventHandler(void)
{
#if (defined(EMBER_AF_RF4CE_ZRC_IS_ACTIONS_ORIGINATOR) || defined(EMBER_SCRIPTED_TEST))
  outgoingEventHandler();
#endif // EMBER_AF_RF4CE_ZRC_IS_ACTIONS_ORIGINATOR
}

#if (defined(EMBER_AF_RF4CE_ZRC_IS_ACTIONS_ORIGINATOR) || defined(EMBER_SCRIPTED_TEST))

//------------------------------------------------------------------------------
// Internal APIs and callbacks

void emAfRf4ceZrc20InitOriginator(void)
{
  recordsTableInit();
}

//------------------------------------------------------------------------------
// Public APIs

EmberStatus emberAfRf4ceZrc20ActionStart(uint8_t pairingIndex,
                                         EmberAfRf4ceZrcActionBank actionBank,
                                         EmberAfRf4ceZrcActionCode actionCode,
                                         EmberAfRf4ceZrcModifierBit actionModifiers,
                                         uint16_t actionVendorId,
                                         const uint8_t *actionData,
                                         uint8_t actionDataLength,
                                         bool atomic)
{
  uint8_t version = emAfRf4ceZrc20GetPeerZrcVersion(pairingIndex);

  // Pairing index is invalid or peer node does not support ZRC 1.1 nor ZRC 2.0
  if (version == ZRC_VERSION_NONE) {
    return EMBER_INVALID_CALL;
  }

  // Destination is ZRC 2.0
  if (version == ZRC_VERSION_2_0) {
    debugScriptCheck("Destination is ZRC 2.0");

    // When a second trigger is initiated by the Action Originator a new actions
    // command shall be immediately transmitted to the Actions Recipient,
    // containing all current action records. The aplActionRepeatTriggerInterval
    // shall be reset when an actions command is transmitted.
    if (!addOrUpdateActionRecord(pairingIndex,
                                 actionBank,
                                 actionCode,
                                 actionModifiers,
                                 actionVendorId,
                                 actionData,
                                 actionDataLength,
                                 atomic)) {
      return EMBER_INVALID_CALL;
    }
  } else { // Destination is ZRC 1.1
    debugScriptCheck("Destination is ZRC 1.1");

    // According to the ZRC 1.1 specs, the only commands supported are those
    // in the HDMI CEC action bank.
    if (actionBank != EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC
        || !addOrUpdateActionRecord(pairingIndex,
                                    EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC,
                                    actionCode,
                                    0x00,
                                    EMBER_RF4CE_NULL_VENDOR_ID,
                                    actionData,
                                    actionDataLength,
                                    atomic)) {
      return EMBER_INVALID_CALL;
    }
  }

  emberAfPluginRf4ceZrc20OutgoingEventHandler();

  return EMBER_SUCCESS;
}

EmberStatus emberAfRf4ceZrc20ActionStop(uint8_t pairingIndex,
                                        EmberAfRf4ceZrcActionBank actionBank,
                                        EmberAfRf4ceZrcActionCode actionCode,
                                        EmberAfRf4ceZrcModifierBit actionModifiers,
                                        uint16_t actionVendorId)
{
  uint8_t version = emAfRf4ceZrc20GetPeerZrcVersion(pairingIndex);
  uint8_t i;

  // On a Key Up trigger on the Actions Originator a new actions command shall
  // be immediately transmitted to the Action Recipient, containing all current
  // action records. If no action record exists, only an actions command shall
  // still be transmitted without any action records indicating that no triggers
  // are currently occurring.
  for(i=0; i<COUNTOF(recordsTable); i++) {
    if (recordsTable[i].pairingIndex == pairingIndex
        && recordsTable[i].actionBank == actionBank
        && recordsTable[i].actionCode == actionCode
        && recordsTable[i].actionVendorId == ((version == ZRC_VERSION_2_0)
                                              ? actionVendorId
                                              : EMBER_RF4CE_NULL_VENDOR_ID)) {
      recordsTable[i].actionType = EMBER_AF_RF4CE_ZRC_ACTION_TYPE_STOP;
      emberAfPluginRf4ceZrc20OutgoingEventHandler();
      return EMBER_SUCCESS;
    }
  }

  return EMBER_INVALID_CALL;
}

//------------------------------------------------------------------------------
// Static functions

static void recordsTableInit(void)
{
  uint8_t i;
  for(i=0; i<COUNTOF(recordsTable); i++) {
    recordsTable[i].pairingIndex = 0xFF;
  }
}

static void outgoingEventHandler(void)
{
  uint16_t nowMs = halCommonGetInt16uMillisecondTick();
  uint8_t pairingIndex = 0xFF;
  uint8_t recordIndex;

  emberEventControlSetInactive(emberAfPluginRf4ceZrc20OutgoingEventControl);

  for(recordIndex=0; recordIndex<COUNTOF(recordsTable); recordIndex++) {
    // Is the current record active and due for transmission? (all non-repeat
    // actions are sent right away).
    if (recordsTable[recordIndex].pairingIndex < 0xFF
        && (recordsTable[recordIndex].actionType != EMBER_AF_RF4CE_ZRC_ACTION_TYPE_REPEAT
            || timeGTorEqualInt16u(nowMs, recordsTable[recordIndex].timeMs))) {
      pairingIndex = recordsTable[recordIndex].pairingIndex;
      break;
    }
  }

  // When the event expires, there should always be at least a record due for
  // transmission.
  assert(pairingIndex < 0xFF);

  // Peer node supports ZRC 2.0
  if (emAfRf4ceZrc20GetPeerZrcVersion(pairingIndex) == ZRC_VERSION_2_0) {
    uint8_t i;

    sendActionsCommand(pairingIndex);

    // Update the actionType of all the transmitted records.
    for(i=0; i<COUNTOF(recordsTable); i++) {
      if (recordsTable[i].pairingIndex == pairingIndex) {
        updateRecordAfterSend(i);
      }
    }
  } else { // Peer node supports ZRC 1.1
    sendZrc11UserControlCommand(recordIndex);
    updateRecordAfterSend(recordIndex);
  }

  // Reschedule the event.
  rescheduleOutgoingEvent();
}

static void updateRecordAfterSend(uint8_t recordIndex)
{
  // A start action becomes a repeat action
  if (recordsTable[recordIndex].actionType == EMBER_AF_RF4CE_ZRC_ACTION_TYPE_START) {
    recordsTable[recordIndex].actionType = EMBER_AF_RF4CE_ZRC_ACTION_TYPE_REPEAT;
  // A record with an atomic action destined to a 1.1 recipient becomes a stop action.
  } else if (recordsTable[recordIndex].actionType == EMBER_AF_RF4CE_ZRC_ACTION_TYPE_ATOMIC
             && emAfRf4ceZrc20GetPeerZrcVersion(recordsTable[recordIndex].pairingIndex) == ZRC_VERSION_1_1) {
    recordsTable[recordIndex].actionType = EMBER_AF_RF4CE_ZRC_ACTION_TYPE_STOP;
  // A record with a stop or atomic actions gets removed from the records
  // table.
  } else if (recordsTable[recordIndex].actionType == EMBER_AF_RF4CE_ZRC_ACTION_TYPE_STOP
             || recordsTable[recordIndex].actionType == EMBER_AF_RF4CE_ZRC_ACTION_TYPE_ATOMIC) {
    recordsTable[recordIndex].pairingIndex = 0xFF;
  }

  // Update the next expiration time for all these records (updating removed
  // records does not cause any problem, so we don't add any check for it).
  recordsTable[recordIndex].timeMs =
      halCommonGetInt16uMillisecondTick()
      + EMBER_AF_PLUGIN_RF4CE_ZRC20_ACTION_REPEAT_TRIGGER_INTERVAL_MS;
}

// Returns true if a new record was added or an existing one was updated.
// false otherwise. Notice that this function also checks whether the record
// that is being added would make the action command exceed the total size.
// In this case, the record won't be added (and this function shall return
// false).
static bool addOrUpdateActionRecord(uint8_t pairingIndex,
                                       EmberAfRf4ceZrcActionBank actionBank,
                                       EmberAfRf4ceZrcActionCode actionCode,
                                       EmberAfRf4ceZrcModifierBit actionModifiers,
                                       uint16_t actionVendorId,
                                       const uint8_t *actionData,
                                       uint8_t actionDataLength,
                                       bool atomic)
{
  uint8_t entryIndex = 0xFF;
  uint8_t freeEntryIndex = 0xFF;
  uint16_t nowMs = halCommonGetInt16uMillisecondTick();
  uint8_t zrcVersion = emAfRf4ceZrc20GetPeerZrcVersion(pairingIndex);
  uint8_t i;

  for(i=0; i<COUNTOF(recordsTable); i++) {
    if (freeEntryIndex == 0xFF && recordsTable[i].pairingIndex == 0xFF) {
      freeEntryIndex = i;
    }

    // TODO: should we check the action modifier here?
    if (recordsTable[i].pairingIndex == pairingIndex
        && recordsTable[i].actionBank == actionBank
        && recordsTable[i].actionCode == actionCode
        && recordsTable[i].actionVendorId == actionVendorId) {
      entryIndex = i;
    }
  }

  // Add a new record or update an old record.
  if ((entryIndex == 0xFF && freeEntryIndex < 0xFF) || entryIndex < 0xFF) {

    // Record not found, adding it to the records table.
    if (entryIndex == 0xFF) {
      entryIndex = freeEntryIndex;
    }

    recordsTable[entryIndex].pairingIndex = pairingIndex;
    recordsTable[entryIndex].actionBank = actionBank;
    recordsTable[entryIndex].actionCode = actionCode;
    recordsTable[entryIndex].actionVendorId = actionVendorId;
    recordsTable[entryIndex].modifierBits = actionModifiers;
    recordsTable[entryIndex].actionPayload = actionData;
    recordsTable[entryIndex].actionPayloadLength = actionDataLength;

    if (zrcVersion == ZRC_VERSION_2_0) {
      // If this is a new or updated record and we exceed the maximum payload
      // available, we remove the record (and return false below).
      // We also call the reschedule() routine to catch the case where an
      // updated record no longer fit and gets deleted.
      if (!checkActionsCommandSize(pairingIndex)) {
        recordsTable[entryIndex].pairingIndex = 0xFF;
        entryIndex = 0xFF;
        rescheduleOutgoingEvent();
      }
    } else { // ZRC 1.1
      if (actionDataLength > ZRC11_MAX_USER_CONTROL_COMMAND_PAYLOAD_LENGTH) {
        recordsTable[entryIndex].pairingIndex = 0xFF;
      }
    }
  }

  // This updates an existing record or sets these fields for a new record.
  if (entryIndex < 0xFF) {
    recordsTable[entryIndex].timeMs = nowMs;
    recordsTable[entryIndex].actionType =
        ((atomic)
         ? EMBER_AF_RF4CE_ZRC_ACTION_TYPE_ATOMIC
         : EMBER_AF_RF4CE_ZRC_ACTION_TYPE_START);

    return true;
  }

  return false;
}

// This function returns the length of the record corresponding to the passed
// index.
static uint8_t getActionRecordLength(uint8_t recordIndex)
{
  return (ACTION_RECORD_ACTION_CONTROL_LENGTH
          + ACTION_RECORD_ACTION_PAYLOAD_LENGTH_LENGTH
          + ACTION_RECORD_ACTION_BANK_LENGTH
          + ACTION_RECORD_ACTION_CODE_LENGTH
          + ((recordsTable[recordIndex].actionVendorId
              == EMBER_RF4CE_NULL_VENDOR_ID)
             ? 0
             : ACTION_RECORD_ACTION_VENDOR_LENGTH)
          + recordsTable[recordIndex].actionPayloadLength);
}

static bool checkActionsCommandSize(uint8_t pairingIndex)
{
  // Frame control
  uint8_t length = 1;
  uint8_t i;
  EmberRf4ceTxOption txOptions;

  if (getActionsCommandTxOptions(pairingIndex, &txOptions) != EMBER_SUCCESS) {
    return false;
  }

  for (i = 0; i < COUNTOF(recordsTable); i++) {
    if (recordsTable[i].pairingIndex == pairingIndex) {
      length += getActionRecordLength(i);
    }
  }

  return (length <= emberAfRf4ceGetMaxPayload(pairingIndex, txOptions));
}

// This function writes the record corresponding to the passed index starting
// from the passed pointer. It returns the length of the record.
static uint8_t addActionRecordToActionsCommand(uint8_t recordIndex, uint8_t *ptr)
{
  // Action control (action type + modifier bits)
  *ptr++ = ((recordsTable[recordIndex].actionType
             << ACTION_RECORD_ACTION_CONTROL_ACTION_TYPE_OFFSET)
            | (recordsTable[recordIndex].modifierBits
               << ACTION_RECORD_ACTION_CONTROL_MODIFIER_BITS_OFFSET));

  // Action payload length
  *ptr++ = recordsTable[recordIndex].actionPayloadLength;

  // Action bank
  *ptr++ = recordsTable[recordIndex].actionBank;

  // Action code
  *ptr++ = recordsTable[recordIndex].actionCode;

  // Action vendor ID
  if (recordsTable[recordIndex].actionVendorId != EMBER_RF4CE_NULL_VENDOR_ID) {
    *ptr++ = LOW_BYTE(recordsTable[recordIndex].actionVendorId);
    *ptr++ = HIGH_BYTE(recordsTable[recordIndex].actionVendorId);
  }

  if (recordsTable[recordIndex].actionPayloadLength > 0) {
    MEMMOVE(ptr,
            recordsTable[recordIndex].actionPayload,
            recordsTable[recordIndex].actionPayloadLength);
  }

  return getActionRecordLength(recordIndex);
}

static EmberStatus getActionsCommandTxOptions(uint8_t pairingIndex,
                                              EmberRf4ceTxOption *txOptions)
{
  EmberRf4cePairingTableEntry destinationEntry;
  EmberStatus status =
      emberAfRf4ceGetPairingTableEntry(pairingIndex, &destinationEntry);

  if (status != EMBER_SUCCESS) {
    return status;
  }

  *txOptions = (EMBER_RF4CE_TX_OPTIONS_ACK_REQUESTED_BIT
                | EMBER_RF4CE_TX_OPTIONS_SECURITY_ENABLED_BIT);

  if (!READBITS(destinationEntry.capabilities,
      EMBER_RF4CE_NODE_CAPABILITIES_IS_TARGET_BIT)) {
    *txOptions |= EMBER_RF4CE_TX_OPTIONS_SINGLE_CHANNEL_BIT;
  }

  return EMBER_SUCCESS;
}

// This function is expected to always fit all the records for a certain pairing
// since we check the record size in the start() API.
// TODO: txOptions
// If an action mapping with 'RF specified' bit set is present, the TX options
// are specified in the 'RF descriptor' field. We will need to support this once
// we have action mappings. However, what are the TX options when no action
// mapping is specified? For now we follow what the GDP specs say in this
// regard: "Unless otherwise specified, the txOptions parameter in this
// primitive shall be set to use the UAM transmission service with security
// enabled, when the destination is an RF4CE Target device, and it shall be set
// to use the UAS transmission service with security enabled when the
// destination is an RF4CE Controller device."
static EmberStatus sendActionsCommand(uint8_t pairingIndex)
{
  EmberStatus status;
  uint8_t buffer[EMBER_AF_RF4CE_MAXIMUM_RF4CE_PAYLOAD_LENGTH];
  uint8_t bufferLength = 0;
  uint8_t i;
  EmberRf4ceTxOption txOptions;

  status = getActionsCommandTxOptions(pairingIndex, &txOptions);

  if (status != EMBER_SUCCESS) {
    return status;
  }

  // TODO: should we include the vendor ID? For now we never send it.

  // Action records
  for(i=0; i<COUNTOF(recordsTable); i++) {
    if (recordsTable[i].pairingIndex == pairingIndex
        && recordsTable[i].actionType != EMBER_AF_RF4CE_ZRC_ACTION_TYPE_STOP) {
      bufferLength += addActionRecordToActionsCommand(i, buffer + bufferLength);
    }
  }

  return emAfRf4ceGdpSendProfileSpecificCommand(pairingIndex,
                                                EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0,
                                                EMBER_RF4CE_NULL_VENDOR_ID, // TODO: should we set this?
                                                txOptions,
                                                EMBER_AF_RF4CE_ZRC_COMMAND_ACTIONS,
                                                buffer,
                                                bufferLength,
                                                NULL); // message tag - unused
}

static EmberStatus sendZrc11UserControlCommand(uint8_t recordIndex)
{
  uint8_t buffer[ZRC11_MAX_USER_CONTROL_COMMAND_LENGTH];
  uint8_t bufferLength = 2; // frame control (1) + RC command code (1)

  assert(recordsTable[recordIndex].pairingIndex < 0xFF);

  // ZRC 1.1 frame control
  if (recordsTable[recordIndex].actionType
      == EMBER_AF_RF4CE_ZRC_ACTION_TYPE_START
      || recordsTable[recordIndex].actionType
         == EMBER_AF_RF4CE_ZRC_ACTION_TYPE_ATOMIC) {
    buffer[0] = EMBER_AF_RF4CE_ZRC_COMMAND_USER_CONTROL_PRESSED;
  } else if (recordsTable[recordIndex].actionType
             == EMBER_AF_RF4CE_ZRC_ACTION_TYPE_REPEAT) {
    buffer[0] = EMBER_AF_RF4CE_ZRC_COMMAND_USER_CONTROL_REPEATED;
  } else {
    buffer[0] = EMBER_AF_RF4CE_ZRC_COMMAND_USER_CONTROL_RELEASED;
  }

  // ZRC 1.1 RC command code
  buffer[1] = recordsTable[recordIndex].actionCode;

  if (recordsTable[recordIndex].actionPayloadLength > 0
      && (buffer[0] == EMBER_AF_RF4CE_ZRC_COMMAND_USER_CONTROL_PRESSED
          || buffer[0] == EMBER_AF_RF4CE_ZRC_COMMAND_USER_CONTROL_REPEATED)) {
    // RC command payload
    MEMMOVE(buffer + 2,
            recordsTable[recordIndex].actionPayload,
            recordsTable[recordIndex].actionPayloadLength);
    bufferLength += recordsTable[recordIndex].actionPayloadLength;
  }

  return emberAfRf4ceSend(recordsTable[recordIndex].pairingIndex,
                          EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_1_1,
                          buffer,
                          bufferLength,
                          NULL); // message tag - unused
}

static void rescheduleOutgoingEvent(void)
{
  uint16_t nowMs = halCommonGetInt16uMillisecondTick();
  uint16_t delayMs = MAX_INT16U_VALUE;
  uint8_t i;

  for (i = 0; i < COUNTOF(recordsTable); i++) {
    if (recordsTable[i].pairingIndex != 0xFF) {
      if (timeGTorEqualInt16u(nowMs, recordsTable[i].timeMs)) {
        delayMs = 0;
        break;
      } else {
        uint16_t expirationTimeMs =
            elapsedTimeInt16u(nowMs, recordsTable[i].timeMs);
        if (expirationTimeMs < delayMs) {
          delayMs = expirationTimeMs;
        }
      }
    }
  }

  if (delayMs == MAX_INT16U_VALUE) {
    emberEventControlSetInactive(emberAfPluginRf4ceZrc20OutgoingEventControl);
  } else if (delayMs == 0) {
    emberEventControlSetActive(emberAfPluginRf4ceZrc20OutgoingEventControl);
  } else {
    emberEventControlSetDelayMS(emberAfPluginRf4ceZrc20OutgoingEventControl,
                                delayMs);
  }
}

#else

void emAfRf4ceZrc20InitOriginator(void)
{
}

EmberStatus emberAfRf4ceZrc20ActionStart(uint8_t pairingIndex,
                                         EmberAfRf4ceZrcActionBank actionBank,
                                         EmberAfRf4ceZrcActionCode actionCode,
                                         EmberAfRf4ceZrcModifierBit actionModifiers,
                                         uint16_t actionVendorId,
                                         const uint8_t *actionData,
                                         uint8_t actionDataLength,
                                         bool atomic)
{
  return EMBER_INVALID_CALL;
}

EmberStatus emberAfRf4ceZrc20ActionStop(uint8_t pairingIndex,
                                        EmberAfRf4ceZrcActionBank actionBank,
                                        EmberAfRf4ceZrcActionCode actionCode,
                                        EmberAfRf4ceZrcModifierBit actionModifiers,
                                        uint16_t actionVendorId)
{
  return EMBER_INVALID_CALL;
}

#endif // EMBER_AF_RF4CE_ZRC_IS_ACTIONS_ORIGINATOR
