// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "rf4ce-mso.h"
#include "rf4ce-mso-internal.h"
#include "rf4ce-mso-attributes.h"

#ifdef EMBER_AF_PLUGIN_RF4CE_MSO_IS_RECIPIENT

static EmberAfRf4ceMsoUserControlRecord records[EMBER_AF_PLUGIN_RF4CE_MSO_MAX_USER_CONTROL_RECORDS];

static void press1xOrRepeat11(const EmberAfRf4ceMsoUserControlRecord *record);
static void repeat10(uint8_t pairingIndex, uint16_t nowMs);
static void stop(uint8_t index);
static void rescheduleUserControlEvent(void);

typedef struct {
  uint16_t responseMs;
  EmberAfRf4ceMsoCommandCode commandCode;
} Request;

static Request checkValidationRequests[EMBER_RF4CE_PAIRING_TABLE_SIZE];
static uint8_t nextCheckValidation = NULL_PAIRING_INDEX;

typedef struct {
  EmberAfRf4ceMsoAttributeId attributeId;
  uint8_t index;
  union {
    EmberAfRf4ceStatus status; // request
    uint8_t valueLen;            // response
  } requestResponseData;
} SetGetAttribute;

static Request setGetAttributeRequests[EMBER_RF4CE_PAIRING_TABLE_SIZE];
static SetGetAttribute setGetAttributeData[EMBER_RF4CE_PAIRING_TABLE_SIZE];
static uint8_t nextSetGetAttribute = NULL_PAIRING_INDEX;

static void rescheduleResponseEvent(const Request *requests,
                                    uint8_t *nextIndex,
                                    EmberEventControl *control);
#define rescheduleCheckValidationEvent() \
  rescheduleResponseEvent(checkValidationRequests,    \
                          &nextCheckValidation,       \
                          &emberAfPluginRf4ceMsoCheckValidationEventControl)
#define rescheduleSetGetAttributeEvent() \
  rescheduleResponseEvent(setGetAttributeRequests,    \
                          &nextSetGetAttribute,       \
                          &emberAfPluginRf4ceMsoSetGetAttributeEventControl)

void emAfRf4ceMsoInitCommands(void)
{
  uint8_t i;
  for (i = 0; i < COUNTOF(records); i++) {
    records[i].pairingIndex = 0xFF;
  }
}

EmberStatus emberAfRf4ceMsoUserControlPress(uint8_t pairingIndex,
                                            EmberAfRf4ceMsoKeyCode commandCode,
                                            const uint8_t *commandPayload,
                                            uint8_t commandPayloadLength,
                                            bool atomic)
{
  return EMBER_INVALID_CALL;
}

EmberStatus emberAfRf4ceMsoUserControlRelease(uint8_t pairingIndex,
                                              EmberAfRf4ceMsoKeyCode rcCommandCode)
{
  return EMBER_INVALID_CALL;
}

EmberStatus emberAfRf4ceMsoSetAttributeRequest(uint8_t pairingIndex,
                                               EmberAfRf4ceMsoAttributeId attributeId,
                                               uint8_t index,
                                               uint8_t valueLen,
                                               const uint8_t *value)
{
  return EMBER_INVALID_CALL;
}

EmberStatus emberAfRf4ceMsoGetAttributeRequest(uint8_t pairingIndex,
                                               EmberAfRf4ceMsoAttributeId attributeId,
                                               uint8_t index,
                                               uint8_t valueLen)
{
  return EMBER_INVALID_CALL;
}

void emberAfPluginRf4ceMsoUserControlEventHandler(void)
{
  uint16_t nowMs = halCommonGetInt16uMillisecondTick();
  uint8_t i;
  for (i = 0; i < COUNTOF(records); i++) {
    if (records[i].pairingIndex != 0xFF
        && (EMBER_AF_PLUGIN_RF4CE_MSO_KEY_REPEAT_WAIT_TIME_MS
            <= elapsedTimeInt16u(records[i].timeMs, nowMs))) {
      stop(i);
    }
  }
  rescheduleUserControlEvent();
}

void emberAfPluginRf4ceMsoCheckValidationEventHandler(void)
{
  // Send the Check Validation Response to the pairing that's up next.  This is
  // a one-and-done thing.  If we fail to queue here or if the message fails to
  // send, we do not try again and instead rely on the originator asking again.
  // Once we send (or try to send), we clear this guy, and reschedule.
  if (nextCheckValidation != NULL_PAIRING_INDEX) {
    emAfRf4ceMsoBufferLength = 0;
    emAfRf4ceMsoBuffer[emAfRf4ceMsoBufferLength++]
      = EMBER_AF_RF4CE_MSO_COMMAND_CHECK_VALIDATION_RESPONSE;
    emAfRf4ceMsoBuffer[emAfRf4ceMsoBufferLength++]
      = emAfRf4ceMsoGetValidationStatus(nextCheckValidation);
    emAfRf4ceMsoSend(nextCheckValidation);
    checkValidationRequests[nextCheckValidation].commandCode = 0;
  }
  rescheduleCheckValidationEvent();
}

void emberAfPluginRf4ceMsoSetGetAttributeEventHandler(void)
{
  // Send the Set Attribute Response or Get Attribute Response to the pairing
  // that's up next.  Like the Check Validation Response, we try once and rely
  // on MAC retries and/or the originator asking again.
  if (nextSetGetAttribute != NULL_PAIRING_INDEX) {
    emAfRf4ceMsoBufferLength = 0;
    emAfRf4ceMsoBuffer[emAfRf4ceMsoBufferLength++]
      = setGetAttributeRequests[nextSetGetAttribute].commandCode;
    emAfRf4ceMsoBuffer[emAfRf4ceMsoBufferLength++]
      = setGetAttributeData[nextSetGetAttribute].attributeId;
    emAfRf4ceMsoBuffer[emAfRf4ceMsoBufferLength++]
      = setGetAttributeData[nextSetGetAttribute].index;
    if (setGetAttributeRequests[nextSetGetAttribute].commandCode
        == EMBER_AF_RF4CE_MSO_COMMAND_SET_ATTRIBUTE_RESPONSE) {
      emAfRf4ceMsoBuffer[emAfRf4ceMsoBufferLength++]
        = setGetAttributeData[nextSetGetAttribute].requestResponseData.status;
    } else {
      // We have not yet asked the attribute code for the attribute data, so we
      // do that here.  The format of the response is status|value length|value,
      // so need to use some fingers to keep placeholders for the data we don't
      // have yet.
      EmberAfRf4ceStatus *status;
      uint8_t *valueLen;
      uint8_t *value;
      status = emAfRf4ceMsoBuffer + emAfRf4ceMsoBufferLength;
      emAfRf4ceMsoBufferLength++;
      valueLen = emAfRf4ceMsoBuffer + emAfRf4ceMsoBufferLength;
      emAfRf4ceMsoBufferLength++;
      value = emAfRf4ceMsoBuffer + emAfRf4ceMsoBufferLength;
      *valueLen = (EMBER_AF_PLUGIN_RF4CE_MSO_MAXIMUM_PAYLOAD_LENGTH
                   - emAfRf4ceMsoBufferLength);
      *status
        = emAfRf4ceMsoGetAttributeRequestCallback(nextSetGetAttribute,
                                                  setGetAttributeData[nextSetGetAttribute].attributeId,
                                                  setGetAttributeData[nextSetGetAttribute].index,
                                                  valueLen,
                                                  value);
      if (*status != EMBER_AF_RF4CE_STATUS_SUCCESS) {
        *valueLen = 0;
      }
      emAfRf4ceMsoBufferLength += *valueLen;
    }
    emAfRf4ceMsoSend(nextSetGetAttribute);
    setGetAttributeRequests[nextSetGetAttribute].commandCode = 0;
  }
  rescheduleSetGetAttributeEvent();
}

void emAfRf4ceMsoMessageSent(uint8_t pairingIndex,
                             EmberAfRf4ceMsoCommandCode commandCode,
                             const uint8_t *message,
                             uint8_t messageLength,
                             EmberStatus status)
{
  // TODO: Handle success and failure of outgoing messages.
}

void emAfRf4ceMsoIncomingMessage(uint8_t pairingIndex,
                                 EmberAfRf4ceMsoCommandCode commandCode,
                                 const uint8_t *message,
                                 uint8_t messageLength)
{
  uint16_t nowMs = halCommonGetInt16uMillisecondTick();

  // Messages from originators that are not validated are ignored and
  // rejected originators can only send CheckValidationRequests.
  EmberAfRf4ceMsoValidationState state = emAfRf4ceMsoGetValidationState(pairingIndex);
  if (state == EMBER_AF_RF4CE_MSO_VALIDATION_STATE_NOT_VALIDATED) {
    emberAfDebugPrint("%p: %cX: %p %p",
                      "MSO",
                      'R',
                      "Unvalidated",
                      "originator");
    emberAfDebugPrintln(": 0x%x", pairingIndex);
    return;
  } else if (state == EMBER_AF_RF4CE_MSO_VALIDATION_STATE_REJECTED
             && commandCode != EMBER_AF_RF4CE_MSO_COMMAND_CHECK_VALIDATION_REQUEST) {
    emberAfDebugPrint("%p: %cX: %p %p",
                      "MSO",
                      'R',
                      "Rejected",
                      "originator");
    emberAfDebugPrintln(": 0x%x", pairingIndex);
    return;
  }

  switch (commandCode) {
  case EMBER_AF_RF4CE_MSO_COMMAND_USER_CONTROL_PRESSED:
  case EMBER_AF_RF4CE_MSO_COMMAND_USER_CONTROL_REPEATED:
    // User Control Pressed is the same in ZRC 1.0 and 1.1.  It has a one-byte
    // RC command code and an n-byte RC command payload.  User Control Repeated
    // has no fields in 1.0, but is formatted the same as a press in 1.1.  If
    // we receive a stray 1.1 repeat, it must be treated as if it were a press.
    // A stray 1.0 repeat is tricky because it has no information about what is
    // being repeated.
    if (USER_CONTROL_PRESSED_LENGTH <= messageLength) {
      EmberAfRf4ceMsoUserControlRecord record;
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
               == EMBER_AF_RF4CE_MSO_COMMAND_USER_CONTROL_REPEATED) {
      repeat10(pairingIndex, nowMs);
    }
    rescheduleUserControlEvent();
    break;

  case EMBER_AF_RF4CE_MSO_COMMAND_USER_CONTROL_RELEASED:
    {
      // User Control Released commands contains no payload in 1.0 and a one-
      // byte RC command code in 1.1.  If this is a 1.1 command (i.e., it has
      // an RC command code), we release any records with the same RC command
      // code from this originator.  If this is a 1.0 command (i.e., it does
      // not have an RC command code), we can't match the release to a specific
      // record, so we just release all records from this originator.
      bool hasRcCommandCode = (USER_CONTROL_RELEASED_1_1_LENGTH
                                  <= messageLength);
      EmberAfRf4ceMsoKeyCode rcCommandCode
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

  case EMBER_AF_RF4CE_MSO_COMMAND_CHECK_VALIDATION_REQUEST:
    // When a Check Validation Request comes in, we have to queue up a response
    // because of the required idle timeout.  If this pairing already had one
    // queued, we will forget the old one and make a new one.  We save the time
    // that our response has to go out, which is the current time plus the idle
    // time.  Then, we schedule the event.
    checkValidationRequests[pairingIndex].commandCode
      = EMBER_AF_RF4CE_MSO_COMMAND_CHECK_VALIDATION_RESPONSE;
    checkValidationRequests[pairingIndex].responseMs
      = (nowMs + APLC_RESPONSE_IDLE_TIME_MS);
    rescheduleCheckValidationEvent();
    break;

  case EMBER_AF_RF4CE_MSO_COMMAND_SET_ATTRIBUTE_REQUEST:
  case EMBER_AF_RF4CE_MSO_COMMAND_GET_ATTRIBUTE_REQUEST:
    // Only one pending Set Attribute Request or Get Attribute Request is
    // allowed per pairing index.  For a Set Attribute Request, we call the
    // callback right away with the incoming data.  For a Get Attribute
    // Request, we save the request and call the callback when it is time to
    // send the response.  Doing it this way avoids needing to store the
    // incoming or outgoing data in RAM in this plugin.
    if (setGetAttributeRequests[pairingIndex].commandCode == 0) {
      setGetAttributeRequests[pairingIndex].commandCode = commandCode + 1;
      setGetAttributeRequests[pairingIndex].responseMs
        = (nowMs + APLC_RESPONSE_IDLE_TIME_MS);

      // The attribute id and index are at the same offset for the Set Attribute
      // Request and Get Attribute Request commands.
      setGetAttributeData[pairingIndex].attributeId
        = message[SET_ATTRIBUTE_REQUEST_ATTRIBUTE_ID_OFFSET];
      setGetAttributeData[pairingIndex].index
        = message[SET_ATTRIBUTE_REQUEST_INDEX_OFFSET];

      if (commandCode == EMBER_AF_RF4CE_MSO_COMMAND_SET_ATTRIBUTE_REQUEST) {
        setGetAttributeData[pairingIndex].requestResponseData.status
          = emAfRf4ceMsoSetAttributeRequestCallback(pairingIndex,
                                                    setGetAttributeData[pairingIndex].attributeId,
                                                    setGetAttributeData[pairingIndex].index,
                                                    message[SET_ATTRIBUTE_REQUEST_VALUE_LENGTH_OFFSET],
                                                    message + SET_ATTRIBUTE_REQUEST_VALUE_OFFSET);
      } else {
        setGetAttributeData[pairingIndex].requestResponseData.valueLen
          = message[GET_ATTRIBUTE_REQUEST_VALUE_LENGTH_OFFSET];
      }
      rescheduleSetGetAttributeEvent();
    }
  }
}

static void press1xOrRepeat11(const EmberAfRf4ceMsoUserControlRecord *record)
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
        && (records[i].commandCode    == EMBER_AF_RF4CE_MSO_COMMAND_USER_CONTROL_PRESSED
            || records[i].commandCode == EMBER_AF_RF4CE_MSO_COMMAND_USER_CONTROL_REPEATED)
        && records[i].rcCommandCode   == record->rcCommandCode) {
      records[i].timeMs = record->timeMs;
      return;
    } else if (index == 0xFF && records[i].pairingIndex == 0xFF) {
      index = i;
    }
  }
  if (index != 0xFF) {
    MEMCOPY(&records[index], record, sizeof(EmberAfRf4ceMsoUserControlRecord));
    emberAfPluginRf4ceMsoUserControlCallback(record);
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
        && (records[i].commandCode    == EMBER_AF_RF4CE_MSO_COMMAND_USER_CONTROL_PRESSED
            || records[i].commandCode == EMBER_AF_RF4CE_MSO_COMMAND_USER_CONTROL_REPEATED)
        && (index == 0xFF
            || timeGTorEqualInt16u(records[i].timeMs, records[index].timeMs))) {
      index = i;
    }
  }
  for (i = 0; i < COUNTOF(records); i++) {
    if (records[i].pairingIndex       == pairingIndex
        && (records[i].commandCode    == EMBER_AF_RF4CE_MSO_COMMAND_USER_CONTROL_PRESSED
            || records[i].commandCode == EMBER_AF_RF4CE_MSO_COMMAND_USER_CONTROL_REPEATED)) {
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
  records[index].commandCode = EMBER_AF_RF4CE_MSO_COMMAND_USER_CONTROL_RELEASED;
  records[index].rcCommandPayloadLength = 0;
  records[index].rcCommandPayload = NULL;
  emberAfPluginRf4ceMsoUserControlCallback(&records[index]);
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
      uint16_t remainingMs = ((EMBER_AF_PLUGIN_RF4CE_MSO_KEY_REPEAT_WAIT_TIME_MS
                             <= elapsedMs)
                            ? 0
                            : (EMBER_AF_PLUGIN_RF4CE_MSO_KEY_REPEAT_WAIT_TIME_MS
                               - elapsedMs));
      if (remainingMs < delayMs) {
        delayMs = remainingMs;
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

static void rescheduleResponseEvent(const Request *requests,
                                    uint8_t *nextIndex,
                                    EmberEventControl *control)
{
  uint8_t i;
  *nextIndex = NULL_PAIRING_INDEX;
  for (i = 0; i < EMBER_RF4CE_PAIRING_TABLE_SIZE; i++) {
    if (requests[i].commandCode != 0
        && (*nextIndex == NULL_PAIRING_INDEX
            || timeGTorEqualInt16u(requests[*nextIndex].responseMs,
                                   requests[i].responseMs))) {
      *nextIndex = i;
    }
  }

  if (*nextIndex == NULL_PAIRING_INDEX) {
    emberEventControlSetInactive(*control);
  } else {
    uint16_t nowMs = halCommonGetInt16uMillisecondTick();
    if (timeGTorEqualInt16u(nowMs, requests[*nextIndex].responseMs)) {
      emberEventControlSetActive(*control);
    } else {
      emberEventControlSetDelayMS(*control,
                                  elapsedTimeInt16u(nowMs,
                                                    requests[*nextIndex].responseMs));
    }
  }
}

#endif // EMBER_AF_PLUGIN_RF4CE_MSO_IS_RECIPIENT
