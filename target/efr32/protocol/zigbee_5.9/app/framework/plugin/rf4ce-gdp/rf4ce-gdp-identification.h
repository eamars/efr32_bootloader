// Copyright 2014 Silicon Laboratories, Inc.

// We delay the procedure in case the enhanced security procedure or the polling
// negotiation procedure take place, which should happen after binding has
// completed.
#define IDENTIFICATION_PROCEDURE_DELAY_MSEC     500

// If the identification procedure fails, we wait a longer delay before trying
// again.
#define IDENTIFICATION_PROCEDURE_AFTER_FAILURE_DELAY_SEC     60

// This defines how many times the identification procedure is started again
// after it failed.
#define IDENTIFICATION_PROCEDURE_CLIENT_MAX_RETIRES          3


extern void emAfRf4ceGdpIdentificationNotifyBindingComplete(uint8_t pairingIndex);

extern void emAfRf4ceGdpIncomingIdentifyCallback(EmberAfRf4ceGdpClientNotificationIdentifyFlags flags,
                                                 uint16_t timeS);

