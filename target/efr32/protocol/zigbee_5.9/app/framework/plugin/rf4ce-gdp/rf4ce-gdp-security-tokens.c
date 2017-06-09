// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "rf4ce-gdp.h"
#include "rf4ce-gdp-internal.h"

#if defined(EMBER_SCRIPTED_TEST)
#include "rf4ce-gdp-test.h"
#endif // EMBER_SCRIPTED_TEST

void emAfRf4ceGdpGetPairingKey(uint8_t pairingIndex, EmberKeyData *key)
{
  halCommonGetIndexedToken(key->contents,
                           TOKEN_PLUGIN_RF4CE_GDP_PAIRING_KEY_TABLE,
                           pairingIndex);
}

void emAfRf4ceGdpSetPairingKey(uint8_t pairingIndex, EmberKeyData *key)
{
  halCommonSetIndexedToken(TOKEN_PLUGIN_RF4CE_GDP_PAIRING_KEY_TABLE,
                           pairingIndex,
                           key->contents);
}
