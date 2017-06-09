// Copyright 2014 Silicon Laboratories, Inc.


// We delay the negotiation procedure in case the GDP plugin is busy doing
// something or the ZRC state machine is not in its initial state.
#define ACTION_MAPPING_NEGOTIATION_PROCEDURE_DELAY_MSEC     500


// Attribute ID (1 byte) + entry ID (2 bytes) + Attribute length (1 byte)
// + Attribute value (3 bytes)
#define MAPPABLE_ACTION_RECORD_SIZE                         7

void emAfRf4ceZrcActionMappingBindingCompleteCallback(uint8_t pairingIndex);

void emAfRf4ceZrcActionMappingStartNegotiationClient(uint8_t pairingIndex);
