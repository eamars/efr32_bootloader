// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "rf4ce-gdp.h"
#include "rf4ce-gdp-internal.h"
#include "rf4ce-gdp-attributes.h"

static uint8_t bindStatusTable[EMBER_RF4CE_PAIRING_TABLE_SIZE];
static uint8_t pollConfigurationTable[EMBER_RF4CE_PAIRING_TABLE_SIZE][APL_GDP_POLL_CONFIGURATION_SIZE];

//------------------------------------------------------------------------------
// Token-related internal APIs.

uint8_t emAfRf4ceGdpGetPairingBindStatus(uint8_t pairingIndex)
{
  return bindStatusTable[pairingIndex];
}

void emAfRf4ceGdpSetPairingBindStatus(uint8_t pairingIndex, uint8_t status)
{
  bindStatusTable[pairingIndex] = status;
}

void emAfRf4ceGdpGetPollConfigurationAttribute(uint8_t pairingIndex,
                                               uint8_t *pollConfiguration)
{
  MEMCOPY(pollConfiguration,
          pollConfigurationTable[pairingIndex],
          APL_GDP_POLL_CONFIGURATION_SIZE);
}

void emAfRf4ceGdpSetPollConfigurationAttribute(uint8_t pairingIndex,
                                               const uint8_t *pollConfiguration)
{
  MEMCOPY(pollConfigurationTable[pairingIndex],
          pollConfiguration,
          APL_GDP_POLL_CONFIGURATION_SIZE);
}
