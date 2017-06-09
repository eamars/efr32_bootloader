// Copyright 2014 Silicon Laboratories, Inc.

#include PLATFORM_HEADER
#include CONFIGURATION_HEADER
#include EMBER_AF_API_EMBER_TYPES
#include EMBER_AF_API_COMMAND_INTERPRETER2
#ifdef EMBER_AF_API_DEBUG_PRINT
  #include EMBER_AF_API_DEBUG_PRINT
#endif
#include EMBER_AF_API_NETWORK_INTERFACE

// The gateway code prints its own prompt, so we only have to print one if we
// aren't running as a gatway.
#ifdef EMBER_AF_API_GATEWAY
  #define PRINT_PROMPT(...)
#else
  #define PRINT_PROMPT emberAfCorePrint
#endif

extern EmberCommandEntry emberCommandTable[];

void emberAfPluginCliTickCallback(void)
{
  if (emberProcessCommandInput(APP_SERIAL)) {
    PRINT_PROMPT("%p>", EMBER_AF_DEVICE_NAME);
  }
}

void emberAfPluginCliInfoCommand(void)
{
  EmberEUI64 eui64;
  EmberNodeType nodeType;
  EmberNetworkParameters parameters;
  emberAfGetEui64(eui64);
  emberAfGetNetworkParameters(&nodeType, &parameters);
  emberAfAppPrint("node [");
  emberAfAppDebugExec(emberAfPrintBigEndianEui64(eui64));
  emberAfAppPrintln("] chan [%d] pwr [%d]",
                    parameters.radioChannel,
                    parameters.radioTxPower);
  // TODO: the node id is hiding in the network manager id.
  emberAfAppPrintln("panID [0x%2x] nodeID [0x%2x]",
                    parameters.panId,
                    parameters.nwkManagerId);
}
