// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "rf4ce-mso.h"
#include "rf4ce-mso-internal.h"

#define DEFINETYPES
#include "rf4ce-mso-tokens.h"
#undef DEFINETYPES

#ifdef EMBER_AF_PLUGIN_RF4CE_MSO_IS_RECIPIENT

static tokTypePluginRf4ceMsoValidation table[EMBER_RF4CE_PAIRING_TABLE_SIZE];

void emAfRf4ceMsoInitializeValidationStates(void)
{
  // When using RAM storage, all pairings have to start out as not validated.
  // This means that every originator has to rebind following a reboot of the
  // recipient.
  uint8_t i;
  for (i = 0; i < EMBER_RF4CE_PAIRING_TABLE_SIZE; i++) {
    emAfRf4ceMsoSetValidation(i,
                              EMBER_AF_RF4CE_MSO_VALIDATION_STATE_NOT_VALIDATED,
                              EMBER_AF_RF4CE_MSO_CHECK_VALIDATION_STATUS_FAILURE);
  }
}

EmberAfRf4ceMsoValidationState emAfRf4ceMsoGetValidationState(uint8_t pairingIndex)
{
  return table[pairingIndex].state;
}

EmberAfRf4ceMsoCheckValidationStatus emAfRf4ceMsoGetValidationStatus(uint8_t pairingIndex)
{
  return table[pairingIndex].status;
}

void emAfRf4ceMsoSetValidation(uint8_t pairingIndex,
                               EmberAfRf4ceMsoValidationState state,
                               EmberAfRf4ceMsoCheckValidationStatus status)
{
  table[pairingIndex].state = state;
  table[pairingIndex].status = status;
}

#endif
