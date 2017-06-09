// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "app/framework/plugin/rf4ce-profile/rf4ce-profile.h"
#include "rf4ce-zrc20.h"
#include "rf4ce-zrc20-internal.h"
#include "rf4ce-zrc20-attributes.h"

#if defined(EMBER_SCRIPTED_TEST)
#include "rf4ce-zrc20-test.h"
#endif

EmberEventControl emberAfPluginRf4ceZrc20IncomingEventControl;

#if (defined(EMBER_AF_RF4CE_ZRC_IS_ACTIONS_RECIPIENT) || defined(EMBER_SCRIPTED_TEST))

// The scheduling code below depends on the wait time fitting in an uint16_t.
// The valid range for the wait time is well within this constraint, so this
// check is really only here in case the user tries something funny.
#if MAX_INT16U_VALUE < EMBER_AF_PLUGIN_RF4CE_ZRC20_ACTION_REPEAT_WAIT_TIME_MS
  #error Invalid value for action repeat wait time.
#endif

static EmberAfRf4ceZrcActionRecord records[EMBER_AF_PLUGIN_RF4CE_ZRC20_MAX_INCOMING_ACTION_RECORDS];

static void start(EmberAfRf4ceZrcActionRecord *record);
static bool repeat(EmberAfRf4ceZrcActionRecord *record);
static void repeat10(uint8_t pairingIndex, uint16_t nowMs);
static void stop(uint8_t index);
static void callback(EmberAfRf4ceZrcActionRecord *record);
static void sow(uint8_t pairingIndex);
static void reap(uint8_t pairingIndex);
static void reschedule(void);

void emAfRf4ceZrc20InitRecipient(void)
{
  uint8_t i;
  for (i = 0; i < COUNTOF(records); i++) {
    records[i].pairingIndex = 0xFF;
  }
}

void emberAfPluginRf4ceZrc20IncomingEventHandler(void)
{
  uint16_t nowMs = halCommonGetInt16uMillisecondTick();
  uint8_t i;
  for (i = 0; i < COUNTOF(records); i++) {
    if (records[i].pairingIndex != 0xFF
        && (EMBER_AF_PLUGIN_RF4CE_ZRC20_ACTION_REPEAT_WAIT_TIME_MS
            <= elapsedTimeInt16u(records[i].timeMs, nowMs))) {
      stop(i);
    }
  }
  reschedule();
}

void emAfRf4ceZrc20IncomingMessageRecipient(uint8_t pairingIndex,
                                            uint16_t vendorId,
                                            EmberAfRf4ceZrcCommandCode commandCode,
                                            const uint8_t *message,
                                            uint8_t messageLength)
{
  uint16_t nowMs = halCommonGetInt16uMillisecondTick();

  switch (commandCode) {
  case EMBER_AF_RF4CE_ZRC_COMMAND_USER_CONTROL_PRESSED:
  case EMBER_AF_RF4CE_ZRC_COMMAND_USER_CONTROL_REPEATED:
    // User Control Pressed is the same in ZRC 1.0 and 1.1.  It has a one-byte
    // RC command code and an n-byte RC command payload.  User Control Repeated
    // has no fields in 1.0, but is formatted the same as a press in 1.1.  If
    // we receive a stray 1.1 repeat, it must be treated as if it were a press.
    // A stray 1.0 repeat is tricky because it has no information about what is
    // being repeated.
    if (USER_CONTROL_PRESSED_LENGTH <= messageLength) {
      EmberAfRf4ceZrcActionRecord record;
      record.pairingIndex = pairingIndex;
      record.actionType
        = (commandCode == EMBER_AF_RF4CE_ZRC_COMMAND_USER_CONTROL_PRESSED
           ? EMBER_AF_RF4CE_ZRC_ACTION_TYPE_START
           : EMBER_AF_RF4CE_ZRC_ACTION_TYPE_REPEAT);
      record.modifierBits = EMBER_AF_RF4CE_ZRC_MODIFIER_BIT_NONE;
      record.actionPayloadLength
        = (messageLength - USER_CONTROL_PRESSED_RC_COMMAND_PAYLOAD_OFFSET);
      record.actionBank = EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC;
      record.actionCode = message[USER_CONTROL_PRESSED_RC_COMMAND_CODE_OFFSET];
      record.actionVendorId = EMBER_RF4CE_NULL_VENDOR_ID;
      record.actionPayload
        = (record.actionPayloadLength == 0
           ? NULL
           : (message + USER_CONTROL_PRESSED_RC_COMMAND_PAYLOAD_OFFSET));
      record.timeMs = nowMs;
      if (!repeat(&record)) {
        start(&record);
      }
    } else if (commandCode
               == EMBER_AF_RF4CE_ZRC_COMMAND_USER_CONTROL_REPEATED) {
      repeat10(pairingIndex, nowMs);
    }
    break;

  case EMBER_AF_RF4CE_ZRC_COMMAND_USER_CONTROL_RELEASED:
    {
      // User Control Released commands contains no payload in 1.0 and a one-
      // byte RC command code in 1.1.  If this is a 1.1 command (i.e., it has
      // an RC command code), we release any actions with the same action code
      // from this originator.  If this is a 1.0 command (i.e., it does not
      // have an RC command code), we can't match the release to a specific
      // action, so we just release all actions from this originator.  The
      // action vendor id is not used for ZRC 1.x, so it is not checked.
      bool hasRcCommandCode = (USER_CONTROL_RELEASED_1_1_LENGTH
                                  <= messageLength);
      EmberAfRf4ceZrcActionCode actionCode = (hasRcCommandCode
                                              ? message[USER_CONTROL_RELEASED_1_1_RC_COMMAND_CODE_OFFSET]
                                              : 0xFF);
      uint8_t i;
      for (i = 0; i < COUNTOF(records); i++) {
        if (records[i].pairingIndex == pairingIndex
            && records[i].actionBank == EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC
            && (!hasRcCommandCode
                || records[i].actionCode == actionCode)) {
          stop(i);
        }
      }
      break;
    }

  case EMBER_AF_RF4CE_ZRC_COMMAND_ACTIONS:
    {
      // First, mark all the actions from this originator for removal.  Then,
      // go through all the actions in this message and update any that we were
      // keeping track of.  This will unmark those actions.  Finally, remove
      // all actions that are still marked from our table.  When that is done,
      // go through the actions in the message again and add new ones to our
      // table.  The first pass cleans up released or timed-out actions and
      // frees up space in the table for any new actions, which are added in
      // the second pass.
      uint8_t i;
      for (i = 0; i < 2; i++) {
        EmberStatus status = EMBER_SUCCESS;
        uint8_t offset = ZRC_PAYLOAD_OFFSET;

        (i == 0 ? sow : reap)(pairingIndex);

        // Every action record has a one-byte action control, one-byte action
        // payload length, one-byte action bank, and one-byte action code.  A
        // record may also have a two-byte action vendor id and an n-byte
        // action payload.
        while (offset + 1 + 1 + 1 + 1 <= messageLength) {
          EmberAfRf4ceZrcActionRecord record;
          uint8_t actionControl = message[offset++];
          record.pairingIndex = pairingIndex;
          record.actionType = actionControl & ACTION_TYPE_MASK;
          record.modifierBits = actionControl & MODIFIER_BITS_MASK;
          record.actionPayloadLength = message[offset++];
          record.actionBank = message[offset++];
          record.actionCode = message[offset++];
          record.timeMs = nowMs;

          // The two-byte action vendor id is only included in the message if
          // the action bank is in the "explicit" range.  Otherwise, it is
          // either ours, theirs, or not set at all.
          if (EMBER_AF_RF4CE_ZRC_ACTION_BANK_VENDOR_SPECIFIC_EXPLICIT
              <= record.actionBank) {
            if (offset + 2 <= messageLength) {
              record.actionVendorId = emberFetchLowHighInt16u((uint8_t *)(message + offset));
              offset += 2;
            } else {
              emberAfDebugPrintln("%p: %cX: %p %p",
                                  "ZRC",
                                  'R',
                                  "Missing",
                                  "action vendor id");
              break;
            }
          } else if (EMBER_AF_RF4CE_ZRC_ACTION_BANK_VENDOR_SPECIFIC_IMPLICIT_RECIPIENT
                     <= record.actionBank) {
            record.actionVendorId = emberAfRf4ceVendorId();
          } else if (EMBER_AF_RF4CE_ZRC_ACTION_BANK_VENDOR_SPECIFIC_IMPLICIT_SOURCE
                     <= record.actionBank) {
            EmberRf4cePairingTableEntry entry;
            status = emberAfRf4ceGetPairingTableEntry(pairingIndex, &entry);
            record.actionVendorId = entry.destVendorId;
          } else {
            record.actionVendorId = EMBER_RF4CE_NULL_VENDOR_ID;
          }

          // If the action payload length was non-zero, the payload must exist
          // in the message.
          if (offset + record.actionPayloadLength <= messageLength) {
            record.actionPayload = (record.actionPayloadLength == 0
                                    ? NULL
                                    : message + offset);
            offset += record.actionPayloadLength;
          } else {
            emberAfDebugPrintln("%p: %cX: %p %p",
                                "ZRC",
                                'R',
                                "Invalid",
                                "action payload length");
            break;
          }

          if (status == EMBER_SUCCESS) {
            // On the first pass, we only try to unmark repeat actions.  On the
            // second pass, we actually start actions.
            if (i == 0) { // sowing
              if (record.actionType == EMBER_AF_RF4CE_ZRC_ACTION_TYPE_START
                  || record.actionType == EMBER_AF_RF4CE_ZRC_ACTION_TYPE_REPEAT) {
                repeat(&record);
              }
            } else if (i == 1) { // reaping
              if (record.actionType == EMBER_AF_RF4CE_ZRC_ACTION_TYPE_START
                  || (record.actionType == EMBER_AF_RF4CE_ZRC_ACTION_TYPE_REPEAT
                      && !repeat(&record))) {
                start(&record);
              } else if (record.actionType
                         == EMBER_AF_RF4CE_ZRC_ACTION_TYPE_ATOMIC) {
                callback(&record);
              }
            }
          }
        }
      }
      break;
    }

  default:
    break;
  }

  reschedule();
}

static void start(EmberAfRf4ceZrcActionRecord *record)
{
  uint8_t i;
  for (i = 0; i < COUNTOF(records); i++) {
    if (records[i].pairingIndex == 0xFF) {
      MEMMOVE(&records[i], record, sizeof(EmberAfRf4ceZrcActionRecord));
      callback(record);
      return;
    }
  }
}

static bool repeat(EmberAfRf4ceZrcActionRecord *record)
{
  uint8_t i;
  for (i = 0; i < COUNTOF(records); i++) {
    if (records[i].pairingIndex      == record->pairingIndex
        && (records[i].actionType    == EMBER_AF_RF4CE_ZRC_ACTION_TYPE_START
            || records[i].actionType == EMBER_AF_RF4CE_ZRC_ACTION_TYPE_REPEAT)
        && records[i].actionBank     == record->actionBank
        && records[i].actionCode     == record->actionCode
        && records[i].actionVendorId == record->actionVendorId) {
      CLEARBITS(records[i].modifierBits, MODIFIER_BITS_SPECIAL_MARK);
      records[i].timeMs = record->timeMs;
      return true;
    }
  }
  return false;
}

static void repeat10(uint8_t pairingIndex, uint16_t nowMs)
{
  // This is a 1.0 repeat, but we don't explicitly know which action it applies
  // to.  So, we assume that it applies to the action that was received most
  // recently.  We find that one and repeat it.  Every other action is stopped.
  // The action vendor id is not used for ZRC 1.x, so it is not checked.
  uint8_t i, index = 0xFF;
  for (i = 0; i < COUNTOF(records); i++) {
    if (records[i].pairingIndex      == pairingIndex
        && (records[i].actionType    == EMBER_AF_RF4CE_ZRC_ACTION_TYPE_START
            || records[i].actionType == EMBER_AF_RF4CE_ZRC_ACTION_TYPE_REPEAT)
        && records[i].actionBank     == EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC
        && (index == 0xFF
            || timeGTorEqualInt16u(records[i].timeMs, records[index].timeMs))) {
      index = i;
    }
  }
  for (i = 0; i < COUNTOF(records); i++) {
    if (records[i].pairingIndex      == pairingIndex
        && (records[i].actionType    == EMBER_AF_RF4CE_ZRC_ACTION_TYPE_START
            || records[i].actionType == EMBER_AF_RF4CE_ZRC_ACTION_TYPE_REPEAT)
        && records[i].actionBank     == EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC) {
      if (i == index) {
        records[i].timeMs = nowMs;
      } else {
        stop(i);
      }
    }
  }
}

static void stop(uint8_t index)
{
  records[index].actionType = EMBER_AF_RF4CE_ZRC_ACTION_TYPE_STOP;
  records[index].actionPayloadLength = 0;
  records[index].actionPayload = NULL;
  callback(&records[index]);
  records[index].pairingIndex = 0xFF;
}

static void callback(EmberAfRf4ceZrcActionRecord *record)
{
  // Clear the special bits before calling the application and then restore
  // them after.
  EmberAfRf4ceZrcModifierBit modifierBits = record->modifierBits;
  CLEARBITS(record->modifierBits, MODIFIER_BITS_SPECIAL_MASK);

  // Call HA callback if incoming action is HA action.
  if (record->actionBank >= EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_INSTANCE_0
      && record->actionBank <= EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_INSTANCE_31) {
      emberAfPluginRf4ceZrc20HaActionCallback(record);
  } else {
    emberAfPluginRf4ceZrc20ActionCallback(record);
  }
  record->modifierBits = modifierBits;
}

static void sow(uint8_t pairingIndex)
{
  uint8_t i;
  for (i = 0; i < COUNTOF(records); i++) {
    if (pairingIndex == records[i].pairingIndex) {
      SETBITS(records[i].modifierBits, MODIFIER_BITS_SPECIAL_MARK);
    }
  }
}

static void reap(uint8_t pairingIndex)
{
  uint8_t i;
  for (i = 0; i < COUNTOF(records); i++) {
    if (pairingIndex == records[i].pairingIndex
        && READBITS(records[i].modifierBits, MODIFIER_BITS_SPECIAL_MARK)) {
      stop(i);
    }
  }
}

static void reschedule(void)
{
  uint16_t nowMs = halCommonGetInt16uMillisecondTick();
  uint16_t delayMs = MAX_INT16U_VALUE;
  uint8_t i;
  for (i = 0; i < COUNTOF(records); i++) {
    if (records[i].pairingIndex != 0xFF) {
      uint16_t elapsedMs = elapsedTimeInt16u(records[i].timeMs, nowMs);
      uint16_t remainingMs = ((EMBER_AF_PLUGIN_RF4CE_ZRC20_ACTION_REPEAT_WAIT_TIME_MS
                             <= elapsedMs)
                            ? 0
                            : (EMBER_AF_PLUGIN_RF4CE_ZRC20_ACTION_REPEAT_WAIT_TIME_MS
                               - elapsedMs));
      if (remainingMs < delayMs) {
        delayMs = remainingMs;
      }
    }
  }
  if (delayMs == MAX_INT16U_VALUE) {
    emberEventControlSetInactive(emberAfPluginRf4ceZrc20IncomingEventControl);
  } else if (delayMs == 0) {
    emberEventControlSetActive(emberAfPluginRf4ceZrc20IncomingEventControl);
  } else {
    emberEventControlSetDelayMS(emberAfPluginRf4ceZrc20IncomingEventControl,
                                delayMs);
  }
}

#else

void emAfRf4ceZrc20InitRecipient(void)
{
}

void emberAfPluginRf4ceZrc20IncomingEventHandler(void)
{
  emberEventControlSetInactive(emberAfPluginRf4ceZrc20IncomingEventControl);
}

void emAfRf4ceZrc20IncomingMessageRecipient(uint8_t pairingIndex,
                                            uint16_t vendorId,
                                            EmberAfRf4ceZrcCommandCode commandCode,
                                            const uint8_t *message,
                                            uint8_t messageLength)
{
}

#endif
