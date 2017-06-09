// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "rf4ce-gdp.h"
#include "rf4ce-gdp-internal.h"

#if defined(EMBER_SCRIPTED_TEST)
#include "rf4ce-gdp-test.h"
#endif // EMBER_SCRIPTED_TEST

static EmberKeyData pairingKeyTable[EMBER_RF4CE_PAIRING_TABLE_SIZE];

void emAfRf4ceGdpGetPairingKey(uint8_t pairingIndex, EmberKeyData *key)
{
  MEMMOVE(key, &pairingKeyTable[pairingIndex], sizeof(EmberKeyData));
}

void emAfRf4ceGdpSetPairingKey(uint8_t pairingIndex, EmberKeyData *key)
{
  MEMMOVE(&pairingKeyTable[pairingIndex], key, sizeof(EmberKeyData));
}
