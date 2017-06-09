// Copyright 2016 Silicon Laboratories, Inc.                                *80*

#include "af.h"
#include "command-relay.h"
#include "command-relay-local.h"

static void parseDeviceEndpointsFromArguments(EmberAfPluginCommandRelayDeviceEndpoint *inDeviceEndpoint,
                                              EmberAfPluginCommandRelayDeviceEndpoint *outDeviceEndpoint);

void emAfPluginCommandRelayAddCommand(void)
{
  EmberAfPluginCommandRelayDeviceEndpoint inDeviceEndpoint;
  EmberAfPluginCommandRelayDeviceEndpoint outDeviceEndpoint;
  parseDeviceEndpointsFromArguments(&inDeviceEndpoint, &outDeviceEndpoint);

  emAfPluginCommandRelayAdd(&inDeviceEndpoint, &outDeviceEndpoint);
}

void emAfPluginCommandRelayRemoveCommand(void)
{
  EmberAfPluginCommandRelayDeviceEndpoint inDeviceEndpoint;
  EmberAfPluginCommandRelayDeviceEndpoint outDeviceEndpoint;
  parseDeviceEndpointsFromArguments(&inDeviceEndpoint, &outDeviceEndpoint);

  emAfPluginCommandRelayRemove(&inDeviceEndpoint, &outDeviceEndpoint);
}

void emAfPluginCommandRelayClearCommand(void)
{
  emAfPluginCommandRelayClear();
}

void emAfPluginCommandRelaySaveCommand(void)
{
  emAfPluginCommandRelaySave();
}

void emAfPluginCommandRelayLoadCommand(void)
{
  emAfPluginCommandRelayLoad();
}

void emAfPluginCommandRelayPrintCommand(void)
{
  emAfPluginCommandRelayPrint();
}

static void parseDeviceEndpointsFromArguments(EmberAfPluginCommandRelayDeviceEndpoint *inDeviceEndpoint,
                                              EmberAfPluginCommandRelayDeviceEndpoint *outDeviceEndpoint)
{
  emberCopyBigEndianEui64Argument(0, inDeviceEndpoint->eui64);
  inDeviceEndpoint->endpoint = (uint8_t)emberUnsignedCommandArgument(1);

  emberCopyBigEndianEui64Argument(2, outDeviceEndpoint->eui64);
  outDeviceEndpoint->endpoint = (uint8_t)emberUnsignedCommandArgument(3);
}
