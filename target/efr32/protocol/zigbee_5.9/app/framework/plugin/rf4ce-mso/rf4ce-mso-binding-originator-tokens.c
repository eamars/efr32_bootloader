// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "rf4ce-mso.h"
#include "rf4ce-mso-internal.h"

#ifdef EMBER_AF_PLUGIN_RF4CE_MSO_IS_ORIGINATOR

// The token stores the pairing index of the current active bind, if any.

uint8_t emAfRf4ceMsoGetActiveBindPairingIndex(void)
{
  uint8_t activeBindPairingIndex;
  halCommonGetToken(&activeBindPairingIndex,
                    TOKEN_PLUGIN_RF4CE_MSO_ACTIVE_BIND_PAIRING_INDEX);
  return activeBindPairingIndex;
}

void emAfRf4ceMsoSetActiveBindPairingIndex(uint8_t pairingIndex)
{
  halCommonSetToken(TOKEN_PLUGIN_RF4CE_MSO_ACTIVE_BIND_PAIRING_INDEX,
                    &pairingIndex);
}

#endif
