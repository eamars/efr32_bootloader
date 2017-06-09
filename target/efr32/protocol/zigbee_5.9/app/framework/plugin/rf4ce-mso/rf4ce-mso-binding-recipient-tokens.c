// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "rf4ce-mso.h"
#include "rf4ce-mso-internal.h"

#ifdef EMBER_AF_PLUGIN_RF4CE_MSO_IS_RECIPIENT

void emAfRf4ceMsoInitializeValidationStates(void)
{
  // Validation states are saved in tokens, so there is nothing to initialize.
}

EmberAfRf4ceMsoValidationState emAfRf4ceMsoGetValidationState(uint8_t pairingIndex)
{
  tokTypePluginRf4ceMsoValidation data;
  halCommonGetIndexedToken(&data,
                           TOKEN_PLUGIN_RF4CE_MSO_VALIDATION_TABLE,
                           pairingIndex);
  return data.state;
}

EmberAfRf4ceMsoCheckValidationStatus emAfRf4ceMsoGetValidationStatus(uint8_t pairingIndex)
{
  tokTypePluginRf4ceMsoValidation data;
  halCommonGetIndexedToken(&data,
                           TOKEN_PLUGIN_RF4CE_MSO_VALIDATION_TABLE,
                           pairingIndex);
  return data.status;
}

void emAfRf4ceMsoSetValidation(uint8_t pairingIndex,
                               EmberAfRf4ceMsoValidationState state,
                               EmberAfRf4ceMsoCheckValidationStatus status)
{
  tokTypePluginRf4ceMsoValidation data = {state, status};
  halCommonSetIndexedToken(TOKEN_PLUGIN_RF4CE_MSO_VALIDATION_TABLE,
                           pairingIndex,
                           &data);
}

#endif
