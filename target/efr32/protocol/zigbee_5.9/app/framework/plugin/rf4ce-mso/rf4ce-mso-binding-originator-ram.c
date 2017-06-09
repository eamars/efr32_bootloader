// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "rf4ce-mso.h"
#include "rf4ce-mso-internal.h"

#ifdef EMBER_AF_PLUGIN_RF4CE_MSO_IS_ORIGINATOR

// This variable stores the pairing index of the current active bind, if any.
static uint8_t activeBindPairingIndex = 0xFF;

uint8_t emAfRf4ceMsoGetActiveBindPairingIndex(void)
{
  return activeBindPairingIndex;
}

void emAfRf4ceMsoSetActiveBindPairingIndex(uint8_t pairingIndex)
{
  activeBindPairingIndex = pairingIndex;
}

#endif
