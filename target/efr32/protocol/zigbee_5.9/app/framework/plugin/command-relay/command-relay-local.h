// Copyright 2016 Silicon Laboratories, Inc.                                *80*

#ifndef __COMMAND_RELAY_LOCAL_H
#define __COMMAND_RELAY_LOCAL_H

void emAfPluginCommandRelayAddBind(EmberEUI64 inEui,
                                  uint8_t inEndpoint,
                                  EmberEUI64 outEui,
                                  uint8_t outEndpoint);

void emAfPluginCommandRelaySave(void);
void emAfPluginCommandRelayLoad(void);
void emAfPluginCommandRelayPrint(void);
void emAfPluginCommandRelayClear(void);
void emAfPluginCommandRelayRemove(EmberAfPluginCommandRelayDeviceEndpoint* inDeviceEndpoint,
                                  EmberAfPluginCommandRelayDeviceEndpoint* outDeviceEndpoint);
void emAfPluginCommandRelayRemoveDeviceByEui64(EmberEUI64 eui64);
void emAfPluginCommandRelayAdd(EmberAfPluginCommandRelayDeviceEndpoint* inDeviceEndpoint,
                               EmberAfPluginCommandRelayDeviceEndpoint* outDeviceEndpoint);

#endif //__COMMAND_RELAY_H
