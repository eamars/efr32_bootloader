// Copyright 2013 Silicon Laboratories, Inc.

#include "app/framework/include/af.h"
#include "app/framework/util/af-main.h"

void emberAfPluginNcpConfigurationNcpInitCallback(bool memoryAllocation)
{
  if (memoryAllocation) {
    emberAfSetEzspConfigValue(EZSP_CONFIG_BINDING_TABLE_SIZE,
                              EMBER_BINDING_TABLE_SIZE,
                              "binding table size");
    emberAfSetEzspConfigValue(EZSP_CONFIG_KEY_TABLE_SIZE,
                              EMBER_KEY_TABLE_SIZE,
                              "key table size");
    emberAfSetEzspConfigValue(EZSP_CONFIG_MAX_END_DEVICE_CHILDREN,
                              EMBER_MAX_END_DEVICE_CHILDREN,
                              "max end device children");
    emberAfSetEzspConfigValue(EZSP_CONFIG_RF4CE_PAIRING_TABLE_SIZE,
                              EMBER_RF4CE_PAIRING_TABLE_SIZE,
                              "rf4ce pairing table size");
    emberAfSetEzspConfigValue(EZSP_CONFIG_RF4CE_PENDING_OUTGOING_PACKET_TABLE_SIZE,
                              EMBER_RF4CE_PENDING_OUTGOING_PACKET_TABLE_SIZE,
                              "rf4ce pending outgoing packet table size");
  } else {
    emberAfSetEzspConfigValue(EZSP_CONFIG_END_DEVICE_POLL_TIMEOUT,
                              EMBER_END_DEVICE_POLL_TIMEOUT,
                              "end device poll timeout");
    emberAfSetEzspConfigValue(EZSP_CONFIG_END_DEVICE_POLL_TIMEOUT_SHIFT,
                              EMBER_END_DEVICE_POLL_TIMEOUT_SHIFT,
                              "end device poll timeout shift");
    emberAfSetEzspConfigValue(EZSP_CONFIG_ZLL_GROUP_ADDRESSES,
                              EMBER_ZLL_GROUP_ADDRESSES,
                              "zll group addresses");
    emberAfSetEzspConfigValue(EZSP_CONFIG_ZLL_RSSI_THRESHOLD,
                              EMBER_ZLL_RSSI_THRESHOLD,
                              "zll rssi threshold");
    emberAfSetEzspConfigValue(EZSP_CONFIG_TRANSIENT_KEY_TIMEOUT_S,
                              EMBER_TRANSIENT_KEY_TIMEOUT_S,
                              "transient key timeout");
  }
}
