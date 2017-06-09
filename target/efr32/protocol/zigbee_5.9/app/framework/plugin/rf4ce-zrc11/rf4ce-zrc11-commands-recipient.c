// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "app/framework/plugin/rf4ce-profile/rf4ce-profile.h"
#include "rf4ce-zrc11.h"
#include "rf4ce-zrc11-internal.h"

EmberEventControl emberAfPluginRf4ceZrc11IncomingUserControlEventControl;

#ifdef EMBER_AF_RF4CE_ZRC_IS_RECIPIENT

static EmberAfRf4ceZrcUserControlRecord records[EMBER_AF_PLUGIN_RF4CE_ZRC11_MAX_INCOMING_USER_CONTROL_RECORDS];

static void press1xOrRepeat11(const EmberAfRf4ceZrcUserControlRecord *record);
static void repeat10(uint8_t pairingIndex, uint16_t nowMs);
static void stop(uint8_t index);
static void rescheduleUserControlEvent(void);

void emAfRf4ceZrc11InitRecipient(void)
{
  uint8_t i;
  for (i = 0; i < COUNTOF(records); i++) {
    records[i].pairingIndex = 0xFF;
  }
}

void emberAfPluginRf4ceZrc11IncomingUserControlEventHandler(void)
{
  uint16_t nowMs = halCommonGetInt16uMillisecondTick();
  uint8_t i;
  for (i = 0; i < COUNTOF(records); i++) {
    if (records[i].pairingIndex != 0xFF
        && (EMBER_AF_PLUGIN_RF4CE_ZRC11_KEY_REPEAT_WAIT_TIME_MS
            <= elapsedTimeInt16u(records[i].timeMs, nowMs))) {
      stop(i);
    }
  }
  rescheduleUserControlEvent();
}

void emAfRf4ceZrc11IncomingUserControl(uint8_t pairingIndex,
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
      EmberAfRf4ceZrcUserControlRecord record;
      record.pairingIndex = pairingIndex;
      record.commandCode = commandCode;
      record.rcCommandPayloadLength
        = (messageLength - USER_CONTROL_PRESSED_RC_COMMAND_PAYLOAD_OFFSET);
      record.rcCommandCode
        = message[USER_CONTROL_PRESSED_RC_COMMAND_CODE_OFFSET];
      record.rcCommandPayload
        = (record.rcCommandPayloadLength == 0
           ? NULL
           : (message + USER_CONTROL_PRESSED_RC_COMMAND_PAYLOAD_OFFSET));
      record.timeMs = nowMs;
      press1xOrRepeat11(&record);
    } else if (commandCode
               == EMBER_AF_RF4CE_ZRC_COMMAND_USER_CONTROL_REPEATED) {
      repeat10(pairingIndex, nowMs);
    }
    rescheduleUserControlEvent();
    break;

  case EMBER_AF_RF4CE_ZRC_COMMAND_USER_CONTROL_RELEASED:
    {
      // User Control Released commands contains no payload in 1.0 and a one-
      // byte RC command code in 1.1.  If this is a 1.1 command (i.e., it has
      // an RC command code), we release any records with the same RC command
      // code from this originator.  If this is a 1.0 command (i.e., it does
      // not have an RC command code), we can't match the release to a specific
      // record, so we just release all records from this originator.
      bool hasRcCommandCode = (USER_CONTROL_RELEASED_1_1_LENGTH
                                  <= messageLength);
      EmberAfRf4ceUserControlCode rcCommandCode
        = (hasRcCommandCode
           ? message[USER_CONTROL_RELEASED_1_1_RC_COMMAND_CODE_OFFSET]
           : 0xFF);
      uint8_t i;
      for (i = 0; i < COUNTOF(records); i++) {
        if (records[i].pairingIndex == pairingIndex
            && (!hasRcCommandCode
                || records[i].rcCommandCode == rcCommandCode)) {
          stop(i);
        }
      }
      rescheduleUserControlEvent();
      break;
    }
  }
}

static void press1xOrRepeat11(const EmberAfRf4ceZrcUserControlRecord *record)
{
  // This is a 1.x press or a 1.1 repeat.  We process them both exactly the
  // same: if there is a press or repeat record from the same pairing index
  // with the same RC command code, we update the time of the record as if it
  // were a repeat.  If we don't find a match to repeat and we have a free
  // slot, we will add a new record as if it were a press.  We look for free
  // slots while searching for a match.  Treating a repeat as a press is
  // required by the spec.  Treating a press as a repeat is required to make
  // out-of-order messages work reasonably.  For example, the originator may
  // send press-repeat1-repeat2-release, but we may receive repeat1-press-
  // repeat2-release due to failed and retried transmissions.  If the release
  // comes out of order, things will get confused, but there's not a great way
  // to deal with that.
  uint8_t i, index = 0xFF;
  for (i = 0; i < COUNTOF(records); i++) {
    if (records[i].pairingIndex       == record->pairingIndex
        && (records[i].commandCode    == EMBER_AF_RF4CE_ZRC_COMMAND_USER_CONTROL_PRESSED
            || records[i].commandCode == EMBER_AF_RF4CE_ZRC_COMMAND_USER_CONTROL_REPEATED)
        && records[i].rcCommandCode   == record->rcCommandCode) {
      records[i].timeMs = record->timeMs;
      return;
    } else if (index == 0xFF && records[i].pairingIndex == 0xFF) {
      index = i;
    }
  }
  if (index != 0xFF) {
    MEMCOPY(&records[index], record, sizeof(EmberAfRf4ceZrcUserControlRecord));
    emberAfPluginRf4ceZrc11UserControlCallback(record);
  }
}

static void repeat10(uint8_t pairingIndex, uint16_t nowMs)
{
  // This is a 1.0 repeat, but we don't explicitly know which record it applies
  // to.  So, we assume that it applies to the record that was received most
  // recently.  We find that one and repeat it.  Every other record is stopped.
  uint8_t i, index = 0xFF;
  for (i = 0; i < COUNTOF(records); i++) {
    if (records[i].pairingIndex       == pairingIndex
        && (records[i].commandCode    == EMBER_AF_RF4CE_ZRC_COMMAND_USER_CONTROL_PRESSED
            || records[i].commandCode == EMBER_AF_RF4CE_ZRC_COMMAND_USER_CONTROL_REPEATED)
        && (index == 0xFF
            || timeGTorEqualInt16u(records[i].timeMs, records[index].timeMs))) {
      index = i;
    }
  }
  for (i = 0; i < COUNTOF(records); i++) {
    if (records[i].pairingIndex       == pairingIndex
        && (records[i].commandCode    == EMBER_AF_RF4CE_ZRC_COMMAND_USER_CONTROL_PRESSED
            || records[i].commandCode == EMBER_AF_RF4CE_ZRC_COMMAND_USER_CONTROL_REPEATED)) {
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
  records[index].commandCode = EMBER_AF_RF4CE_ZRC_COMMAND_USER_CONTROL_RELEASED;
  records[index].rcCommandPayloadLength = 0;
  records[index].rcCommandPayload = NULL;
  emberAfPluginRf4ceZrc11UserControlCallback(&records[index]);
  records[index].pairingIndex = 0xFF;
}

static void rescheduleUserControlEvent(void)
{
  uint16_t nowMs = halCommonGetInt16uMillisecondTick();
  uint16_t delayMs = MAX_INT16U_VALUE;
  uint8_t i;
  for (i = 0; i < COUNTOF(records); i++) {
    if (records[i].pairingIndex != 0xFF) {
      uint16_t elapsedMs = elapsedTimeInt16u(records[i].timeMs, nowMs);
      uint16_t remainingMs = ((EMBER_AF_PLUGIN_RF4CE_ZRC11_KEY_REPEAT_WAIT_TIME_MS
                             <= elapsedMs)
                            ? 0
                            : (EMBER_AF_PLUGIN_RF4CE_ZRC11_KEY_REPEAT_WAIT_TIME_MS
                               - elapsedMs));
      if (remainingMs < delayMs) {
        delayMs = remainingMs;
      }
    }
  }
  if (delayMs == MAX_INT16U_VALUE) {
    emberEventControlSetInactive(emberAfPluginRf4ceZrc11IncomingUserControlEventControl);
  } else if (delayMs == 0) {
    emberEventControlSetActive(emberAfPluginRf4ceZrc11IncomingUserControlEventControl);
  } else {
    emberEventControlSetDelayMS(emberAfPluginRf4ceZrc11IncomingUserControlEventControl,
                                delayMs);
  }
}

#else

void emAfRf4ceZrc11InitRecipient(void)
{
}

void emberAfPluginRf4ceZrc11IncomingUserControlEventHandler(void)
{
  emberEventControlSetInactive(emberAfPluginRf4ceZrc11IncomingUserControlEventControl);
}

void emAfRf4ceZrc11IncomingUserControl(uint8_t pairingIndex,
                                       EmberAfRf4ceZrcCommandCode commandCode,
                                       const uint8_t *message,
                                       uint8_t messageLength)
{
}

#endif
