// Copyright 2014 Silicon Laboratories, Inc.

// We delay the procedure in case the enhanced security procedure takes place,
// which should happen right after binding has completed.
#define POLLING_NEGOTIATION_PROCEDURE_DELAY_MSEC     500

// If the poll negotiation procedure fails, we wait a longer delay before trying
// again.
#define POLLING_NEGOTIATION_PROCEDURE_AFTER_FAILURE_DELAY_SEC     60

// This defines how many times the poll negotiation procedure is started again
// after it failed.
#define POLLING_NEGOTIATION_PROCEDURE_CLIENT_MAX_RETIRES          3

// The server will wait for this time for a PullAttributes() from the server
// prior to declaring the poll negotiation procedure a failure.
#define POLLING_NEGOTIATION_PROCEDURE_SERVER_PULL_TIMEOUT_MSEC    500

extern void emAfRf4ceGdpPollingStackStatusCallback(EmberStatus status);

extern void emAfRf4ceGdpPollingNotifyBindingComplete(uint8_t pairingIndex);

extern void emAfRf4ceGdpPollingIncomingCommandCallback(bool framePending);

extern void emAfRf4ceGdpGetPollConfigurationAttribute(uint8_t pairingIndex,
                                                      uint8_t *pollConfiguration);

extern void emAfRf4ceGdpSetPollConfigurationAttribute(uint8_t pairingIndex,
                                                      const uint8_t *pollConfiguration);

#define emAfRf4ceGdpIsPollTriggerValid(trigger)                                \
  (trigger <= EMBER_AF_RF4CE_GDP_HEARTBEAT_TRIGGER_POLLING_ON_OTHER_USER_ACTIVITY)
