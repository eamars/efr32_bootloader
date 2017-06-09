// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "rf4ce-gdp.h"
#include "rf4ce-gdp-internal.h"

#if defined(EMBER_SCRIPTED_TEST)
#include "rf4ce-gdp-test.h"
#endif // EMBER_SCRIPTED_TEST

extern bool emRadioGetRandomNumbers(uint16_t *rn, uint8_t count);

bool emAfRf4ceGdpSecurityGetRandomString(EmberAfRf4ceGdpRand *rn)
{
  // Because of alignment issues with some platforms, we can't just cast the
  // passed array to (uint16_t*).
  uint16_t tempRand[EMBER_AF_RF4CE_GDP_RAND_SIZE];
  uint8_t i;

  if (!emRadioGetRandomNumbers(tempRand, EMBER_AF_RF4CE_GDP_RAND_SIZE / 2)) {
    return false;
  }

  for (i=0; i<EMBER_AF_RF4CE_GDP_RAND_SIZE / 2; i++) {
    rn->contents[2*i] = LOW_BYTE(tempRand[i]);
    rn->contents[2*i+1] = HIGH_BYTE(tempRand[i]);
  }

  return true;
}
