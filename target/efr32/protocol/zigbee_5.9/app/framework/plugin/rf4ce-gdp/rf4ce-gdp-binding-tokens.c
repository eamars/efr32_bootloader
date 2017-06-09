// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "rf4ce-gdp.h"
#include "rf4ce-gdp-internal.h"

#include "rf4ce-gdp-attributes.h"

//------------------------------------------------------------------------------
// Token-related internal APIs.

uint8_t emAfRf4ceGdpGetPairingBindStatus(uint8_t pairingIndex)
{
  uint8_t status;
  halCommonGetIndexedToken(&status,
                           TOKEN_PLUGIN_RF4CE_GDP_BIND_TABLE,
                           pairingIndex);
  return status;
}

void emAfRf4ceGdpSetPairingBindStatus(uint8_t pairingIndex, uint8_t status)
{
  halCommonSetIndexedToken(TOKEN_PLUGIN_RF4CE_GDP_BIND_TABLE,
                           pairingIndex,
                           &status);
}

void emAfRf4ceGdpGetPollConfigurationAttribute(uint8_t pairingIndex,
                                               uint8_t *pollConfiguration)
{
  halCommonGetIndexedToken(pollConfiguration,
                           TOKEN_PLUGIN_RF4CE_GDP_POLLING_CONFIGURATION_TABLE,
                           pairingIndex);
}

void emAfRf4ceGdpSetPollConfigurationAttribute(uint8_t pairingIndex,
                                               const uint8_t *pollConfiguration)
{
  halCommonSetIndexedToken(TOKEN_PLUGIN_RF4CE_GDP_POLLING_CONFIGURATION_TABLE,
                           pairingIndex,
                           (uint8_t*)pollConfiguration);
}
