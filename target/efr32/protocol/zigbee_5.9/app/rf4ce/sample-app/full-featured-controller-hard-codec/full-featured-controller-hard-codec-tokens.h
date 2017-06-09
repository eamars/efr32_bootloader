// Copyright 2014 Silicon Laboratories, Inc.

// We use an array to hold the pairing index for each device
#define CREATOR_PAIRING_DEVICE_TABLE 0x0800

#ifdef DEFINETOKENS
DEFINE_INDEXED_TOKEN(PAIRING_DEVICE_TABLE,
                     uint8_t,
                     DEVICE_COUNT,
                     NO_PAIRING)
#endif //DEFINETOKENS
