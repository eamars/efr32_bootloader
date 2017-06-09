// Copyright 2014 Silicon Laboratories, Inc.

// For each pairing entry we maintain two bytes of flags. The first byte
// stores the first byte of the ZRC capabilities (bytes 1-3 are reserved).
// The second byte is left for future use as well.
#define CREATOR_PLUGIN_RF4CE_ZRC20_FLAGS          0x8732

#ifdef DEFINETOKENS
DEFINE_INDEXED_TOKEN(PLUGIN_RF4CE_ZRC20_FLAGS,
                     uint16_t,
                     EMBER_RF4CE_PAIRING_TABLE_SIZE,
                     0x0000)
#endif
