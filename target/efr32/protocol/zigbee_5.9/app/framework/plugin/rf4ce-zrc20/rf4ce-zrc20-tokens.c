// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"

#if defined(EMBER_SCRIPTED_TEST)
#include "rf4ce-zrc20-test.h"
#endif // EMBER_SCRIPTED_TEST

#include "app/framework/plugin/rf4ce-profile/rf4ce-profile.h"
#include "rf4ce-zrc20.h"
#include "rf4ce-zrc20-internal.h"
#include "rf4ce-zrc20-attributes.h"

uint16_t emAfRf4ceZrcGetRemoteNodeFlags(uint8_t pairingIndex)
{
  uint16_t flags;
  halCommonGetIndexedToken(&flags,
                           TOKEN_PLUGIN_RF4CE_ZRC20_FLAGS,
                           pairingIndex);
  return flags;
}

void emAfRf4ceZrcSetRemoteNodeFlags(uint8_t pairingIndex,
                                           uint16_t flags)
{
  halCommonSetIndexedToken(TOKEN_PLUGIN_RF4CE_ZRC20_FLAGS,
                           pairingIndex,
                           &flags);
}
