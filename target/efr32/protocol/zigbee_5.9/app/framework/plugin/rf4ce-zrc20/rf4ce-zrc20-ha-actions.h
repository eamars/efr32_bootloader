// Copyright 2014 Silicon Laboratories, Inc.


// We delay the announcement procedure in case the GDP plugin is busy doing
// something or the ZRC state machine is not in its initial state.
#define HA_ACTIONS_ANNOUNCEMENT_PROCEDURE_DELAY_MSEC     500

// Action bank 0x80-0x9F
#define HA_SUPPORTED_MAX_INSTANCE_ID_INDEX  31

void emAfRf4ceZrcHAActionsBindingCompleteCallback(uint8_t pairingIndex);

void emAfRf4ceZrcStartHomeAutomationAnnouncementClient(uint8_t pairingIndex);
