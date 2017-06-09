// Copyright 2016 Silicon Laboratories, Inc.                                *80*

#ifndef __COMMAND_RELAY_H
#define __COMMAND_RELAY_H

typedef struct
{
  EmberEUI64 eui64;
  uint8_t endpoint;
} EmberAfPluginCommandRelayDeviceEndpoint;

typedef struct {
  EmberAfPluginCommandRelayDeviceEndpoint inDeviceEndpoint;
  EmberAfPluginCommandRelayDeviceEndpoint outDeviceEndpoint;
} EmberAfPluginCommandRelayEntry;

/** @brief Pointer to the relay table structure
  *
  * For some MQTT messages, it is more convenient to use the relay table data
  * structure directly.  Use this API to get a pointer to the relay table
  * structure.
  *
  * @out  Pointer to the address table structure
  */
EmberAfPluginCommandRelayEntry* emberAfPluginCommandRelayTablePointer(void);

#endif //__COMMAND_RELAY_H
