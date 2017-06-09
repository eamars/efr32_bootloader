// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"

#if defined(EMBER_SCRIPTED_TEST)
#include "rf4ce-zrc20-test.h"
#endif // EMBER_SCRIPTED_TEST

#include "app/framework/plugin/rf4ce-profile/rf4ce-profile.h"
#include "rf4ce-zrc20.h"
#include "rf4ce-zrc20-internal.h"
#include "rf4ce-zrc20-attributes.h"

static uint16_t zrcFlags[EMBER_RF4CE_PAIRING_TABLE_SIZE];

uint16_t emAfRf4ceZrcGetRemoteNodeFlags(uint8_t pairingIndex)
{
  return zrcFlags[pairingIndex];
}

void emAfRf4ceZrcSetRemoteNodeFlags(uint8_t pairingIndex,
                                           uint16_t flags)
{
  zrcFlags[pairingIndex] = flags;
}
