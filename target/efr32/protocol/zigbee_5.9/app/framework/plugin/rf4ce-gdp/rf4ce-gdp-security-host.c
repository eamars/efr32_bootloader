// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "rf4ce-gdp.h"
#include "rf4ce-gdp-internal.h"

#if defined(EMBER_SCRIPTED_TEST)
#include "rf4ce-gdp-test.h"
#endif // EMBER_SCRIPTED_TEST

// TODO: for now we use halCommonGetRandom(). The host application should
// provide a better way of generating random numbers for these security-related
// use cases.
bool emAfRf4ceGdpSecurityGetRandomString(EmberAfRf4ceGdpRand *rn)
{
  uint8_t i;

  for (i=0; i<EMBER_AF_RF4CE_GDP_RAND_SIZE / 2; i++) {
    uint16_t randNum = halCommonGetRandom();
    rn->contents[2*i] = LOW_BYTE(randNum);
    rn->contents[2*i+1] = HIGH_BYTE(randNum);
  }

  return true;
}
