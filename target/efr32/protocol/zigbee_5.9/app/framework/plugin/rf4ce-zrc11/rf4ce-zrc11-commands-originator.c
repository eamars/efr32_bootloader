// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "app/framework/plugin/rf4ce-profile/rf4ce-profile.h"
#include "rf4ce-zrc11.h"
#include "rf4ce-zrc11-internal.h"

EmberEventControl emberAfPluginRf4ceZrc11OutgoingUserControlEventControl;

#ifdef EMBER_AF_RF4CE_ZRC_IS_ORIGINATOR

typedef struct {
  uint8_t pairingIndex;
  EmberAfRf4ceZrcCommandCode commandCode;
  EmberAfRf4ceUserControlCode rcCommandCode;
  uint8_t rcCommandPayload[EMBER_AF_PLUGIN_RF4CE_ZRC11_MAXIMUM_RC_COMMAND_PAYLOAD_LENGTH];
  uint8_t rcCommandPayloadLength;
  uint16_t timeMs;
} UserControlRecord;
static UserControlRecord records[EMBER_AF_PLUGIN_RF4CE_ZRC11_MAX_OUTGOING_USER_CONTROL_RECORDS];

static EmberStatus sendAndReschedule(UserControlRecord *record);
static void rescheduleUserControlEvent(void);

void emAfRf4ceZrc11InitOriginator(void)
{
  // A pairing index of 0xFF means that the record is not in use.  We need to
  // set all record this way at startup to free them for use.
  uint8_t i;
  for (i = 0; i < COUNTOF(records); i++) {
    records[i].pairingIndex = 0xFF;
  }
}

EmberStatus emberAfRf4ceZrc11UserControlPress(uint8_t pairingIndex,
                                              EmberAfRf4ceUserControlCode rcCommandCode,
                                              const uint8_t *rcCommandPayload,
                                              uint8_t rcCommandPayloadLength,
                                              bool atomic)
{
  uint8_t i, index = 0xFF;

  // We have a limited amount of space for payload in our table and also in our
  // outgoing buffer.  These are configurable via #defines if they aren't large
  // enough.
  if ((EMBER_AF_PLUGIN_RF4CE_ZRC11_MAXIMUM_RC_COMMAND_PAYLOAD_LENGTH
       < rcCommandPayloadLength)
      || (EMBER_AF_PLUGIN_RF4CE_ZRC11_MAXIMUM_PAYLOAD_LENGTH
          < USER_CONTROL_PRESSED_LENGTH + rcCommandPayloadLength)) {
    return EMBER_MESSAGE_TOO_LONG;
  }

  // Look for a matching record or a free slot in the table.  If we find a
  // match, we will reuse the record on the assumption that a button can't be
  // doubly pressed.
  for (i = 0; i < COUNTOF(records); i++) {
    if (records[i].pairingIndex == pairingIndex
        && records[i].rcCommandCode == rcCommandCode) {
      index = i;
      break;
    } else if (records[i].pairingIndex == 0xFF && index == 0xFF) {
      index = i;
    }
  }

  if (atomic) {
    // Atomic (i.e., non repeatable keys) do not require a slot in the table,
    // so a new, temporary record is created and sent.  We don't need to set
    // the time of the record because times are only used for repeats.
    UserControlRecord record;
    record.pairingIndex = pairingIndex;
    record.commandCode = EMBER_AF_RF4CE_ZRC_COMMAND_USER_CONTROL_PRESSED;
    record.rcCommandCode = rcCommandCode;
    MEMCOPY(record.rcCommandPayload, rcCommandPayload, rcCommandPayloadLength);
    record.rcCommandPayloadLength = rcCommandPayloadLength;

    // Although we don't need a slot in the table, we do care if there was a
    // matching record in the table already.  If so, we want to remove it so
    // that it stops repeating and doesn't cause a release later.  (This logic
    // will free already-free records, but there's no harm in that.)  When we
    // send this command below, we'll also reschedule based on the new state of
    // the table.
    if (index != 0xFF) {
      records[index].pairingIndex = 0xFF;
    }

    // We return the actual status of the send because we will not repeat this
    // user control.
    return sendAndReschedule(&record);
  } else {
    // Repeatable keys require a slot in the table so we can send repeats and
    // releases in the future.  Therefore, if we had no match and also no free
    // slots, we have to give up.  Otherwise, we build the record and send it
    // immediately.  Sending in this function lets the application call
    // UserControlPress and UserControlRelease back-to-back and have it
    // actually work.  As above, the time for the record is not set here.  In
    // this case, sending will automatically reset the time (and reschedule the
    // event), so there's no reason to set it.  Note also that we return
    // SUCCESS even if the send fails.  We'll automatically repeat the user
    // control, so having one go missing, even if it's the first one, isn't a
    // problem.
    if (index == 0xFF) {
      return EMBER_TABLE_FULL;
    } else {
      records[index].pairingIndex = pairingIndex;
      records[index].commandCode
        = EMBER_AF_RF4CE_ZRC_COMMAND_USER_CONTROL_PRESSED;
      records[index].rcCommandCode = rcCommandCode;
      MEMCOPY(records[index].rcCommandPayload,
              rcCommandPayload,
              rcCommandPayloadLength);
      records[index].rcCommandPayloadLength = rcCommandPayloadLength;
      sendAndReschedule(&records[index]);
      return EMBER_SUCCESS;
    }
  }
}

EmberStatus emberAfRf4ceZrc11UserControlRelease(uint8_t pairingIndex,
                                                EmberAfRf4ceUserControlCode rcCommandCode)
{
  // Find the record to release and immediately send out the release message.
  // As with the press function, this one does not set the time.  We also do
  // not return an error if the send fails.  The release isn't repeated, but
  // the recipient will time out our press (or repeat), so a missing release
  // isn't a problem.
  uint8_t i;
  for (i = 0; i < COUNTOF(records); i++) {
    if (records[i].pairingIndex == pairingIndex
        && records[i].rcCommandCode == rcCommandCode) {
      records[i].commandCode = EMBER_AF_RF4CE_ZRC_COMMAND_USER_CONTROL_RELEASED;
      sendAndReschedule(&records[i]);
      return EMBER_SUCCESS;
    }
  }
  return EMBER_INVALID_CALL;
}

void emberAfPluginRf4ceZrc11OutgoingUserControlEventHandler(void)
{
  // We expect the user control event to fire only when there is something to
  // send.  We find it, send it, and reschedule.
  uint16_t nowMs = halCommonGetInt16uMillisecondTick();
  uint8_t i, index = 0xFF;
  for (i = 0; i < COUNTOF(records); i++) {
    if (records[i].pairingIndex != 0xFF
        && timeGTorEqualInt16u(nowMs, records[i].timeMs)) {
      index = i;
      break;
    }
  }
  assert(index != 0xFF);
  sendAndReschedule(&records[index]);
}

static EmberStatus sendAndReschedule(UserControlRecord *record)
{
  // Construct the message and send it.
  EmberStatus status;
  emAfRf4ceZrc11BufferLength = 0;
  emAfRf4ceZrc11Buffer[emAfRf4ceZrc11BufferLength++] = record->commandCode;
  emAfRf4ceZrc11Buffer[emAfRf4ceZrc11BufferLength++] = record->rcCommandCode;
  MEMCOPY(emAfRf4ceZrc11Buffer + emAfRf4ceZrc11BufferLength,
          record->rcCommandPayload,
          record->rcCommandPayloadLength);
  emAfRf4ceZrc11BufferLength += record->rcCommandPayloadLength;
  status = emAfRf4ceZrc11Send(record->pairingIndex);

  // If this was a release, we are done and clear the record.  Otherwise, a
  // press becomes a repeat (and a repeat stays a repeat).  The next time to
  // send this record depends on the key repeat interval.  Once we adjust this
  // record, we reschedule.
  if (record->commandCode == EMBER_AF_RF4CE_ZRC_COMMAND_USER_CONTROL_RELEASED) {
    record->pairingIndex = 0xFF;
  } else {
    record->commandCode = EMBER_AF_RF4CE_ZRC_COMMAND_USER_CONTROL_REPEATED;
    record->timeMs = (halCommonGetInt16uMillisecondTick()
                      + EMBER_AF_PLUGIN_RF4CE_ZRC11_KEY_REPEAT_INTERVAL_MS);
  }
  rescheduleUserControlEvent();
  return status;
}

static void rescheduleUserControlEvent(void)
{
  // Look at all the active records and figure out when to schedule the event.
  // If any record should have been sent in the past, we immediately schedule
  // the event to send it as soon as possible.  Otherwise, we find the record
  // that has to be repeated soonest and schedule based on that.  If there are
  // no active records, we are done and cancel the event.
  uint16_t nowMs = halCommonGetInt16uMillisecondTick();
  uint16_t delayMs = MAX_INT16U_VALUE;
  uint8_t i;
  for (i = 0; i < COUNTOF(records); i++) {
    if (records[i].pairingIndex != 0xFF) {
      if (timeGTorEqualInt16u(nowMs, records[i].timeMs)) {
        delayMs = 0;
        break;
      } else {
        uint16_t remainingMs = elapsedTimeInt16u(nowMs, records[i].timeMs);
        if (remainingMs < delayMs) {
          delayMs = remainingMs;
        }
      }
    }
  }
  if (delayMs == MAX_INT16U_VALUE) {
    emberEventControlSetInactive(emberAfPluginRf4ceZrc11OutgoingUserControlEventControl);
  } else if (delayMs == 0) {
    emberEventControlSetActive(emberAfPluginRf4ceZrc11OutgoingUserControlEventControl);
  } else {
    emberEventControlSetDelayMS(emberAfPluginRf4ceZrc11OutgoingUserControlEventControl,
                                delayMs);
  }
}

#else

void emAfRf4ceZrc11InitOriginator(void)
{
}

EmberStatus emberAfRf4ceZrc11UserControlPress(uint8_t pairingIndex,
                                              EmberAfRf4ceUserControlCode commandCode,
                                              const uint8_t *commandPayload,
                                              uint8_t rcCommandPayloadLength,
                                              bool atomic)
{
  return EMBER_INVALID_CALL;
}

EmberStatus emberAfRf4ceZrc11UserControlRelease(uint8_t pairingIndex,
                                                EmberAfRf4ceUserControlCode rcCommandCode)
{
  return EMBER_INVALID_CALL;
}

void emberAfPluginRf4ceZrc11OutgoingUserControlEventHandler(void)
{
  emberEventControlSetActive(emberAfPluginRf4ceZrc11OutgoingUserControlEventControl);
}

#endif
