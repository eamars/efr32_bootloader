// Copyright 2014 Silicon Laboratories, Inc.

#include "rf4ce-gdp-attributes.h"

// For each pairing entry we maintain a status byte initialized to 0x00. This is
// shared between originator and recipient code (which can run both at once on a
// device).
#define CREATOR_PLUGIN_RF4CE_GDP_BIND_TABLE                      0x8730
// For each pairing entry we need to remember the original link key that was
// established during pairing.
#define CREATOR_PLUGIN_RF4CE_GDP_PAIRING_KEY_TABLE               0x8731
// We maintain the polling configuration attribute for each pairing entry.
#define CREATOR_PLUGIN_RF4CE_GDP_POLLING_CONFIGURATION_TABLE     0x8733

#ifdef DEFINETYPES
typedef uint8_t tokTypePairingKey[EMBER_ENCRYPTION_KEY_SIZE];
typedef uint8_t tokTypePollConfiguration[APL_GDP_POLL_CONFIGURATION_SIZE];
#endif // DEFINETYPES

#ifdef DEFINETOKENS
DEFINE_INDEXED_TOKEN(PLUGIN_RF4CE_GDP_BIND_TABLE,
                     uint8_t,
                     EMBER_RF4CE_PAIRING_TABLE_SIZE,
                     0x00)
DEFINE_INDEXED_TOKEN(PLUGIN_RF4CE_GDP_PAIRING_KEY_TABLE,
                     tokTypePairingKey,
                     EMBER_RF4CE_PAIRING_TABLE_SIZE,
                     {0,})
DEFINE_INDEXED_TOKEN(PLUGIN_RF4CE_GDP_POLLING_CONFIGURATION_TABLE,
                     tokTypePollConfiguration,
                     EMBER_RF4CE_PAIRING_TABLE_SIZE,
                     {0,})
#endif // DEFINETOKENS
