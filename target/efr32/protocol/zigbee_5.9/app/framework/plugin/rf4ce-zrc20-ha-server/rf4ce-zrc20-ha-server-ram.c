/* Copyright 2014 Silicon Laboratories, Inc. */

#include "af.h"
#include "rf4ce-zrc20-ha-server.h"

static DestStruct logicalDevices[EMBER_AF_PLUGIN_RF4CE_ZRC20_HA_SERVER_LOGICAL_DEVICES_TABLE_SIZE];
static uint8_t instanceToLogicalDevice[EMBER_RF4CE_PAIRING_TABLE_SIZE][ZRC_HA_SERVER_NUM_OF_HA_INSTANCES];


EmberStatus emberAfPluginRf4ceZrc20HaMappingToLogicalDeviceAdd(uint8_t pairingIndex,
                                                               uint8_t haInstanceId,
                                                               uint8_t destIndex)
{
    /* Pairing index out of range. */
    if (pairingIndex >= EMBER_RF4CE_PAIRING_TABLE_SIZE)
    {
        return EMBER_INDEX_OUT_OF_RANGE;
    }

    /* Entry index out of range. */
    if (haInstanceId >= ZRC_HA_SERVER_NUM_OF_HA_INSTANCES)
    {
        return EMBER_INDEX_OUT_OF_RANGE;
    }

    if (destIndex >= EMBER_AF_PLUGIN_RF4CE_ZRC20_HA_SERVER_LOGICAL_DEVICES_TABLE_SIZE)
    {
        return EMBER_INDEX_OUT_OF_RANGE;
    }

    /* Map logical device to instance. */
    instanceToLogicalDevice[pairingIndex][haInstanceId] = destIndex;

    return EMBER_SUCCESS;
}

EmberStatus emberAfPluginRf4ceZrc20HaMappingToLogicalDeviceRemove(uint8_t destIndex)
{
    uint8_t i, j;
    uint8_t* tmpPoi;

    if (destIndex >= EMBER_AF_PLUGIN_RF4CE_ZRC20_HA_SERVER_LOGICAL_DEVICES_TABLE_SIZE)
    {
        return EMBER_INDEX_OUT_OF_RANGE;
    }

    for (i=0; i<EMBER_RF4CE_PAIRING_TABLE_SIZE; i++)
    {
        tmpPoi = &instanceToLogicalDevice[i][0];
        for (j=0; j<ZRC_HA_SERVER_NUM_OF_HA_INSTANCES; j++)
        {
            if (destIndex == tmpPoi[j])
            {
                tmpPoi[j] = 0xFF;
            }
        }
    }

    return EMBER_SUCCESS;
}

EmberStatus emberAfPluginRf4ceZrc20HaMappingToLogicalDeviceGet(uint8_t pairingIndex,
                                                               uint8_t haInstanceId,
                                                               uint8_t* destIndex)
{
    /* Pairing index out of range. */
    if (pairingIndex >= EMBER_RF4CE_PAIRING_TABLE_SIZE)
    {
        return EMBER_INDEX_OUT_OF_RANGE;
    }

    /* Entry index out of range. */
    if (haInstanceId >= ZRC_HA_SERVER_NUM_OF_HA_INSTANCES)
    {
        return EMBER_INDEX_OUT_OF_RANGE;
    }

    *destIndex = instanceToLogicalDevice[pairingIndex][haInstanceId];

    return EMBER_SUCCESS;
}

EmberStatus emberAfPluginRf4ceZrc20HaLogicalDeviceIndexLookUp(DestStruct* dest,
                                                              uint8_t* index)
{
    uint8_t i;
    DestStruct tmpDest;

    for (i=0; i<EMBER_AF_PLUGIN_RF4CE_ZRC20_HA_SERVER_LOGICAL_DEVICES_TABLE_SIZE; i++)
    {
        tmpDest = logicalDevices[i];
        if (tmpDest.type == dest->type
            && tmpDest.indexOrDestination == dest->indexOrDestination
            && tmpDest.sourceEndpoint == dest->sourceEndpoint
            && tmpDest.destinationEndpoint == dest->destinationEndpoint)
        {
            *index = i;
            return EMBER_SUCCESS;
        }
    }

    /* Did not find matching entry. */
    return EMBER_NOT_FOUND;
}


void emAfRf4ceZrc20ClearLogicalDevicesTable(void)
{
    MEMSET((uint8_t*)logicalDevices, 0xFF, sizeof(logicalDevices));
}

EmberStatus emAfRf4ceZrc20AddLogicalDeviceDestination(DestStruct* dest,
                                                      uint8_t* index)
{
    uint8_t i;

    for (i=0; i<EMBER_AF_PLUGIN_RF4CE_ZRC20_HA_SERVER_LOGICAL_DEVICES_TABLE_SIZE; i++)
    {
        /* Empty entry. */
        if (logicalDevices[i].type == 0xFF)
        {
            MEMCOPY((uint8_t*)&logicalDevices[i], (uint8_t*)dest, sizeof(DestStruct));
            *index = i;
            return EMBER_SUCCESS;
        }
    }

    /* Table is full. */
    return EMBER_TABLE_FULL;
}

EmberStatus emAfRf4ceZrc20RemoveLogicalDeviceDestination(uint8_t destIndex)
{
    if (destIndex >= EMBER_AF_PLUGIN_RF4CE_ZRC20_HA_SERVER_LOGICAL_DEVICES_TABLE_SIZE)
    {
        return EMBER_INDEX_OUT_OF_RANGE;
    }

    MEMSET((uint8_t*)&logicalDevices[destIndex], 0xFF, sizeof(DestStruct));
    return EMBER_SUCCESS;
}

uint8_t GetLogicalDeviceDestination(uint8_t i, DestStruct* dest)
{
    if (i >= EMBER_AF_PLUGIN_RF4CE_ZRC20_HA_SERVER_LOGICAL_DEVICES_TABLE_SIZE)
    {
        return 0xFF;
    }

    MEMCOPY((uint8_t*)dest, (uint8_t*)&logicalDevices[i], sizeof(DestStruct));
    return i;
}

void emAfRf4ceZrc20ClearInstanceToLogicalDeviceTable(void)
{
    MEMSET(instanceToLogicalDevice, 0xFF, sizeof(instanceToLogicalDevice));
}

void DestLookup(uint8_t pairingIndex,
                uint8_t haInstanceId,
                DestStruct* dest)
{
    uint8_t destIndex = instanceToLogicalDevice[pairingIndex][haInstanceId];

    MEMCOPY((uint8_t*)dest,
            (uint8_t*)&logicalDevices[destIndex],
            sizeof(DestStruct));
}


