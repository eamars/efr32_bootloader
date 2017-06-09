// Copyright 2014 Silicon Laboratories, Inc.


#define CREATOR_PLUGIN_RF4CE_ZRC20_HA_SERVER_LOGICAL_DEVICES_TABLE              0x8735
#define CREATOR_PLUGIN_RF4CE_ZRC20_HA_SERVER_INSTANCE_TO_LOGICAL_DEVICE_TABLE   0x8736

#ifdef DEFINETYPES
#include "app/framework/plugin/rf4ce-profile/rf4ce-profile-types.h"
#include "app/framework/plugin/rf4ce-zrc20/rf4ce-zrc20-types.h"
#include "rf4ce-zrc20-ha-server.h"

typedef struct {
  uint8_t instances[ZRC_HA_SERVER_NUM_OF_HA_INSTANCES];
} InstStruct;
#endif


#ifdef DEFINETOKENS
DEFINE_INDEXED_TOKEN(PLUGIN_RF4CE_ZRC20_HA_SERVER_LOGICAL_DEVICES_TABLE,
                     DestStruct,
                     (EMBER_AF_PLUGIN_RF4CE_ZRC20_HA_SERVER_LOGICAL_DEVICES_TABLE_SIZE),
                     {0,})

DEFINE_INDEXED_TOKEN(PLUGIN_RF4CE_ZRC20_HA_SERVER_INSTANCE_TO_LOGICAL_DEVICE_TABLE,
                     InstStruct,
                     (EMBER_RF4CE_PAIRING_TABLE_SIZE),
                     {{0,}})
#endif

