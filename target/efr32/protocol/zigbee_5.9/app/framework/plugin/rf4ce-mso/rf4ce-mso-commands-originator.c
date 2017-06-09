// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "rf4ce-mso.h"
#include "rf4ce-mso-internal.h"
#ifdef EMBER_SCRIPTED_TEST
#include "rf4ce-mso-test.h"
#endif // EMBER_SCRIPTED_TEST
#include "rf4ce-mso-attributes.h"

#ifdef EMBER_AF_PLUGIN_RF4CE_MSO_IS_ORIGINATOR

// TODO: Add support for multiple attribute sets/gets.

typedef struct {
  uint8_t pairingIndex;
  EmberAfRf4ceMsoCommandCode commandCode;
  EmberAfRf4ceMsoKeyCode rcCommandCode;
  uint8_t rcCommandPayload[EMBER_AF_PLUGIN_RF4CE_MSO_MAXIMUM_RC_COMMAND_PAYLOAD_LENGTH];
  uint8_t rcCommandPayloadLength;
  uint16_t timeMs;
  uint8_t numberOfTransmissions;
} UserControlRecord;
static UserControlRecord records[EMBER_AF_PLUGIN_RF4CE_MSO_MAX_USER_CONTROL_RECORDS];

typedef struct {
  uint8_t pairingIndex;
  EmberAfRf4ceMsoAttributeId attributeId;
  uint8_t index;
  uint8_t valueLen;
  bool set;
  bool idle;
} SetGetAttributeState;
static SetGetAttributeState attribute = {NULL_PAIRING_INDEX};

//------------------------------------------------------------------------------
// Forward declarations.

static EmberStatus setGetAttributeRequest(uint8_t pairingIndex,
                                          EmberAfRf4ceMsoAttributeId attributeId,
                                          uint8_t index,
                                          uint8_t valueLen,
                                          const uint8_t *value);

static EmberStatus sendAndReschedule(UserControlRecord *record);
static void rescheduleUserControlEvent(void);

//------------------------------------------------------------------------------
// Public APIs.

void emAfRf4ceMsoInitCommands(void)
{
  // A pairing index of 0xFF means that the record is not in use.  We need to
  // set all record this way at startup to free them for use.
  uint8_t i;
  for (i = 0; i < COUNTOF(records); i++) {
    records[i].pairingIndex = 0xFF;
  }
}

EmberStatus emberAfRf4ceMsoUserControlPress(uint8_t pairingIndex,
                                            EmberAfRf4ceMsoKeyCode rcCommandCode,
                                            const uint8_t *rcCommandPayload,
                                            uint8_t rcCommandPayloadLength,
                                            bool atomic)
{
  uint8_t i, index = 0xFF;

  // We have a limited amount of space for payload in our table and also in our
  // outgoing buffer.  These are configurable via #defines if they aren't large
  // enough.
  if ((EMBER_AF_PLUGIN_RF4CE_MSO_MAXIMUM_RC_COMMAND_PAYLOAD_LENGTH
       < rcCommandPayloadLength)
      || (EMBER_AF_PLUGIN_RF4CE_MSO_MAXIMUM_PAYLOAD_LENGTH
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
    record.commandCode = EMBER_AF_RF4CE_MSO_COMMAND_USER_CONTROL_PRESSED;
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
        = EMBER_AF_RF4CE_MSO_COMMAND_USER_CONTROL_PRESSED;
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

EmberStatus emberAfRf4ceMsoUserControlRelease(uint8_t pairingIndex,
                                              EmberAfRf4ceMsoKeyCode rcCommandCode)
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
      records[i].commandCode = EMBER_AF_RF4CE_MSO_COMMAND_USER_CONTROL_RELEASED;
      sendAndReschedule(&records[i]);
      return EMBER_SUCCESS;
    }
  }
  return EMBER_INVALID_CALL;
}

EmberStatus emberAfRf4ceMsoSetAttributeRequest(uint8_t pairingIndex,
                                               EmberAfRf4ceMsoAttributeId attributeId,
                                               uint8_t index,
                                               uint8_t valueLen,
                                               const uint8_t *value)
{
  return setGetAttributeRequest(pairingIndex,
                                attributeId,
                                index,
                                valueLen,
                                value);
}

EmberStatus emberAfRf4ceMsoGetAttributeRequest(uint8_t pairingIndex,
                                               EmberAfRf4ceMsoAttributeId attributeId,
                                               uint8_t index,
                                               uint8_t valueLen)
{
  return setGetAttributeRequest(pairingIndex,
                                attributeId,
                                index,
                                valueLen,
                                NULL);
}

//------------------------------------------------------------------------------
// Event handlers.

void emberAfPluginRf4ceMsoUserControlEventHandler(void)
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

void emberAfPluginRf4ceMsoSetGetAttributeEventHandler(void)
{
  if (attribute.idle) {
    // If the transmission was successful, the controller SHALL wait
    // aplcResponseIdleTime with its receiver disabled.  Next it enables its
    // receiver and waits aplResponseWaitTime symbols for the corresponding
    // set/get attribute response command frame to arrive.
    emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_MSO, true);
    attribute.idle = false;
    emberEventControlSetDelayMS(emberAfPluginRf4ceMsoSetGetAttributeEventControl,
                                EMBER_AF_PLUGIN_RF4CE_MSO_RESPONSE_WAIT_TIME_MS);
  } else {
    // Timed out waiting for the response, turn the radio back off.
    emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_MSO, false);

    attribute.pairingIndex = NULL_PAIRING_INDEX;
    emberEventControlSetInactive(emberAfPluginRf4ceMsoSetGetAttributeEventControl);
    if (attribute.set) {
      emAfRf4ceMsoSetAttributeResponseCallback(attribute.attributeId,
                                               attribute.index,
                                               EMBER_AF_RF4CE_STATUS_NO_RESPONSE);
    } else {
      emAfRf4ceMsoGetAttributeResponseCallback(attribute.attributeId,
                                               attribute.index,
                                               EMBER_AF_RF4CE_STATUS_NO_RESPONSE,
                                               0,
                                               NULL);
    }
  }
}

void emAfRf4ceMsoMessageSent(uint8_t pairingIndex,
                             EmberAfRf4ceMsoCommandCode commandCode,
                             const uint8_t *message,
                             uint8_t messageLength,
                             EmberStatus status)
{
  switch(commandCode) {
  case EMBER_AF_RF4CE_MSO_COMMAND_CHECK_VALIDATION_REQUEST:
    emAfPluginRf4ceMsoCheckValidationRequestSentCallback(status, pairingIndex);
    break;
  default:
    // TODO: add other commands here if needed
    break;
  }
}

//------------------------------------------------------------------------------
// Internal APIs.

void emAfRf4ceMsoIncomingMessage(uint8_t pairingIndex,
                                 EmberAfRf4ceMsoCommandCode commandCode,
                                 const uint8_t *message,
                                 uint8_t messageLength)
{
  switch (commandCode) {
  case EMBER_AF_RF4CE_MSO_COMMAND_CHECK_VALIDATION_RESPONSE:
    {
      EmberAfRf4ceMsoCheckValidationStatus checkValidationStatus
        = message[CHECK_VALIDATION_RESPONSE_CHECK_VALIDATION_STATUS_OFFSET];
      emAfPluginRf4ceMsoIncomingCheckValidationResponseCallback(checkValidationStatus,
                                                                pairingIndex);
      break;
    }

  case EMBER_AF_RF4CE_MSO_COMMAND_SET_ATTRIBUTE_RESPONSE:
    if (attribute.pairingIndex == pairingIndex
        && attribute.attributeId == message[SET_ATTRIBUTE_RESPONSE_ATTRIBUTE_ID_OFFSET]
        && (!emAfRf4ceMsoAttributeIsArrayed(attribute.attributeId)
            || attribute.index == message[SET_ATTRIBUTE_RESPONSE_INDEX_OFFSET])
        && attribute.set
        && !attribute.idle) {
      attribute.pairingIndex = NULL_PAIRING_INDEX;
      emberEventControlSetInactive(emberAfPluginRf4ceMsoSetGetAttributeEventControl);
      // Turn the radio back off.
      emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_MSO, false);
      emAfRf4ceMsoSetAttributeResponseCallback(attribute.attributeId,
                                               message[SET_ATTRIBUTE_RESPONSE_INDEX_OFFSET],
                                               message[SET_ATTRIBUTE_RESPONSE_STATUS_OFFSET]);
    }
    break;

  case EMBER_AF_RF4CE_MSO_COMMAND_GET_ATTRIBUTE_RESPONSE:
    if (attribute.pairingIndex == pairingIndex
        && attribute.attributeId == message[GET_ATTRIBUTE_RESPONSE_ATTRIBUTE_ID_OFFSET]
        && (!emAfRf4ceMsoAttributeIsArrayed(attribute.attributeId)
            || attribute.index == message[GET_ATTRIBUTE_RESPONSE_INDEX_OFFSET])
        //&& attribute.valueLen == message[GET_ATTRIBUTE_RESPONSE_VALUE_LENGTH_OFFSET]
        && !attribute.set
        && !attribute.idle) {
      attribute.pairingIndex = NULL_PAIRING_INDEX;
      emberEventControlSetInactive(emberAfPluginRf4ceMsoSetGetAttributeEventControl);
      // Turn the radio back off.
      emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_MSO, false);
      emAfRf4ceMsoGetAttributeResponseCallback(attribute.attributeId,
                                               message[GET_ATTRIBUTE_RESPONSE_INDEX_OFFSET],
                                               message[GET_ATTRIBUTE_RESPONSE_STATUS_OFFSET],
                                               message[GET_ATTRIBUTE_RESPONSE_VALUE_LENGTH_OFFSET],
                                               message + GET_ATTRIBUTE_RESPONSE_VALUE_OFFSET);
    }
    break;
  }
}

//------------------------------------------------------------------------------
// Static functions.

static EmberStatus setGetAttributeRequest(uint8_t pairingIndex,
                                          EmberAfRf4ceMsoAttributeId attributeId,
                                          uint8_t index,
                                          uint8_t valueLen,
                                          const uint8_t *value)
{
  EmberStatus status = EMBER_INVALID_CALL;
  if (attribute.pairingIndex == NULL_PAIRING_INDEX) {
    emAfRf4ceMsoBufferLength = 0;
    emAfRf4ceMsoBuffer[emAfRf4ceMsoBufferLength++]
      = (value != NULL
         ? EMBER_AF_RF4CE_MSO_COMMAND_SET_ATTRIBUTE_REQUEST
         : EMBER_AF_RF4CE_MSO_COMMAND_GET_ATTRIBUTE_REQUEST);
    emAfRf4ceMsoBuffer[emAfRf4ceMsoBufferLength++] = attributeId;
    emAfRf4ceMsoBuffer[emAfRf4ceMsoBufferLength++] = index;
    emAfRf4ceMsoBuffer[emAfRf4ceMsoBufferLength++] = valueLen;
    if (value != NULL) {
      MEMMOVE(emAfRf4ceMsoBuffer + emAfRf4ceMsoBufferLength, value, valueLen);
      emAfRf4ceMsoBufferLength += valueLen;
    }
    status = emAfRf4ceMsoSend(pairingIndex);
    if (status == EMBER_SUCCESS) {
      attribute.pairingIndex = pairingIndex;
      attribute.attributeId = attributeId;
      attribute.index = index;
      attribute.valueLen = valueLen;
      attribute.set = (value != NULL);
      attribute.idle = true;
      emberEventControlSetDelayMS(emberAfPluginRf4ceMsoSetGetAttributeEventControl,
                                  APLC_RESPONSE_IDLE_TIME_MS);
      // If the transmission was successful, the controller SHALL wait
      // aplcResponseIdleTime with its receiver disabled.  Next it enables its
      // receiver and waits aplResponseWaitTime symbols for the corresponding
      // set/get attribute response command frame to arrive.
      emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_MSO, false);
    }
  }
  return status;
}

static EmberStatus sendAndReschedule(UserControlRecord *record)
{
  EmberStatus status;
  EmberAfRf4ceMsoIrRfDatabaseEntry entry;
  const EmberAfRf4ceMsoIrRfDatabaseRfDescriptor *rfDescriptor = NULL;

  // No matter if we use the IR-RF database or not, every command starts with
  // the command code.
  emAfRf4ceMsoBufferLength = 0;
  emAfRf4ceMsoBuffer[emAfRf4ceMsoBufferLength++] = record->commandCode;

  // If either the IR-RF database entry for this key code exists and isn't the
  // default, we use the RF descriptor from the database.  Otherwise, we use
  // the information that came from the application.
  status
    = emberAfPluginRf4ceMsoGetIrRfDatabaseEntryCallback(record->rcCommandCode,
                                                        &entry);
  if (status == EMBER_SUCCESS
      && !emberAfRf4ceMsoIrRfDatabaseEntryUseDefault(&entry)) {
    if (record->commandCode == EMBER_AF_RF4CE_MSO_COMMAND_USER_CONTROL_PRESSED
        && emberAfRf4ceMsoIrRfDatabaseEntryHasRfPressedDescriptor(&entry)) {
      rfDescriptor = &entry.rfPressedDescriptor;
    } else if (record->commandCode == EMBER_AF_RF4CE_MSO_COMMAND_USER_CONTROL_REPEATED
               && emberAfRf4ceMsoIrRfDatabaseEntryHasRfRepeatedDescriptor(&entry)) {
      rfDescriptor = &entry.rfRepeatedDescriptor;
    } else if (record->commandCode == EMBER_AF_RF4CE_MSO_COMMAND_USER_CONTROL_RELEASED
               && emberAfRf4ceMsoIrRfDatabaseEntryHasRfReleasedDescriptor(&entry)) {
      rfDescriptor = &entry.rfReleasedDescriptor;
    }

    // If the RF descriptor does not exist, it means nothing is sent via RF for
    // this particular command for this particular key code.  In that case, we
    // can just drop this record and reschedule.  Note that status is already
    // SUCCESS here, so it doesn't have to be set before kicking out.  If the
    // descriptor does exist, we use it and send.
    if (rfDescriptor == NULL) {
      record->pairingIndex = 0xFF;
      goto kickout;
    } else {
      MEMCOPY(emAfRf4ceMsoBuffer + emAfRf4ceMsoBufferLength,
              rfDescriptor->payload,
              rfDescriptor->payloadLength);
      emAfRf4ceMsoBufferLength += rfDescriptor->payloadLength;
      status = emAfRf4ceMsoSendExtended(record->pairingIndex,
                                        rfDescriptor->txOptions);
    }
  } else {
    emAfRf4ceMsoBuffer[emAfRf4ceMsoBufferLength++] = record->rcCommandCode;
    MEMCOPY(emAfRf4ceMsoBuffer + emAfRf4ceMsoBufferLength,
            record->rcCommandPayload,
            record->rcCommandPayloadLength);
    emAfRf4ceMsoBufferLength += record->rcCommandPayloadLength;
    status = emAfRf4ceMsoSend(record->pairingIndex);
  }

  // If this was a release, we are done and clear the record.  For presses and
  // repeats, if we satisfied the minimum transmission requirements, we queue
  // up a release for next time.  Otherwise, a press becomes a repeat (and a
  // repeat stays a repeat).  The next time to send this record depends on the
  // key repeat interval.  Once we adjust this record, we reschedule.
  if (record->commandCode == EMBER_AF_RF4CE_MSO_COMMAND_USER_CONTROL_PRESSED) {
    record->numberOfTransmissions = 0;
  }
  record->numberOfTransmissions++;
  if (record->commandCode == EMBER_AF_RF4CE_MSO_COMMAND_USER_CONTROL_RELEASED) {
    record->pairingIndex = 0xFF;
  } else {
    if (rfDescriptor != NULL
        && (emberAfRf4ceMsoIrRfDatabaseEntryGetMinimumNumberOfTransmissions(rfDescriptor)
            <= record->numberOfTransmissions)
        && !emberAfRf4ceMsoIrRfDatabaseEntryShouldTransmitUntilRelease(rfDescriptor)) {
      record->commandCode = EMBER_AF_RF4CE_MSO_COMMAND_USER_CONTROL_RELEASED;
    } else {
      record->commandCode = EMBER_AF_RF4CE_MSO_COMMAND_USER_CONTROL_REPEATED;
    }
    record->timeMs = (halCommonGetInt16uMillisecondTick()
                      + EMBER_AF_PLUGIN_RF4CE_MSO_KEY_REPEAT_INTERVAL_MS);
  }

kickout:
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
    emberEventControlSetInactive(emberAfPluginRf4ceMsoUserControlEventControl);
  } else if (delayMs == 0) {
    emberEventControlSetActive(emberAfPluginRf4ceMsoUserControlEventControl);
  } else {
    emberEventControlSetDelayMS(emberAfPluginRf4ceMsoUserControlEventControl,
                                delayMs);
  }
}

#endif // EMBER_AF_PLUGIN_RF4CE_MSO_IS_ORIGINATOR
