// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "rf4ce-gdp-identification-server.h"

#ifdef EMBER_AF_LEGACY_CLI
  #error The RF4CE GDP Identification Server plugin is not compatible with the legacy CLI.
#endif

#if defined(EMBER_AF_GENERATE_CLI) || defined(EMBER_AF_API_COMMAND_INTERPRETER2)

// plugin rf4ce-gdp-identification-server identify <pairing index:1> <flags:1> <time:2>
void emberAfRf4ceGdpIdentificationServerIdentifyCommand(void)
{
  uint8_t pairingIndex = (uint8_t)emberUnsignedCommandArgument(0);
  EmberAfRf4ceGdpClientNotificationIdentifyFlags flags = (EmberAfRf4ceGdpClientNotificationIdentifyFlags)emberUnsignedCommandArgument(1);
  uint16_t timeS = (uint16_t)emberUnsignedCommandArgument(2);
  EmberStatus status = emberAfRf4ceGdpIdentificationServerIdentify(pairingIndex,
                                                                   flags,
                                                                   timeS);
  emberAfAppPrintln("%p 0x%x", "identify", status);
}

#endif
