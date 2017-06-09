//
// test-harness-z3-core.c
//
// August 3, 2015
// Refactored November 23, 2015
//
// ZigBee 3.0 core test harness functionality
//

#include "app/framework/include/af.h"

#include "app/framework/util/attribute-storage.h" // emberAfResetAttributes()
#include "app/framework/util/af-main.h" // emAfCliVersionCommand()
#include "app/framework/security/security-config.h" // key definitions

#include "app/util/zigbee-framework/zigbee-device-common.h"

#ifdef EMBER_AF_API_ZLL_PROFILE
  #include EMBER_AF_API_ZLL_PROFILE // emberAfZllResetToFactoryNew()
#elif defined(EMBER_SCRIPTED_TEST)
  #include "../zll-commissioning/zll-commissioning.h"
#endif

#ifdef EMBER_AF_API_NETWORK_STEERING
  #include EMBER_AF_API_NETWORK_STEERING
#elif defined(EMBER_SCRIPTED_TEST)
  #include "../network-steering/network-steering.h"
#endif

#include "stack/include/packet-buffer.h"
#include "stack/include/network-formation.h"

#include "test-harness-z3-core.h"
#include "test-harness-z3-nwk.h"
#include "test-harness-z3-zll.h"
#include "test-harness-z3-zdo.h"

// -----------------------------------------------------------------------------
// Constants

#define BEACON_REQUEST_COMMAND (0x07)
#define BEACON_ASSOCIATION_PERMIT_BIT (0x80)
#define BEACON_EXTENDED_PAN_ID_INDEX (7)

// -----------------------------------------------------------------------------
// Globals

EmAfPluginTestHarnessZ3DeviceMode emAfPluginTestHarnessZ3DeviceMode;
EmberEventControl emberAfPluginTestHarnessZ3OpenNetworkEventControl;

// -----------------------------------------------------------------------------
// Util

uint32_t emAfPluginTestHarnessZ3GetSignificantBit(uint8_t commandIndex)
{
  uint32_t mask = emberUnsignedCommandArgument(commandIndex);
  uint32_t significantBit = (1UL << 31);

  while (!(significantBit & mask) && significantBit) {
    significantBit >>= 1;
  }

  return significantBit;
}

#if !defined(EZSP_HOST)
static uint8_t getApsFrameControl(EmberMessageBuffer commandBuffer,
                                  uint8_t commandIndex)
{
  uint8_t fc = 0xFF;

#ifndef EZSP_HOST
  uint8_t fcIndex = commandIndex;

  fcIndex -= (EUI64_SIZE           // source address
              + sizeof(uint32_t)   // frame counter
              + sizeof(uint8_t));  // frame control

  fcIndex -= (sizeof(uint8_t)     // aps counter
              + sizeof(uint8_t)); // fc!

  fc = emberGetLinkedBuffersByte(commandBuffer, fcIndex);
#endif /* EZSP_HOST */

  return fc;
}
#endif

// -----------------------------------------------------------------------------
// State

#define PRINTING_MASK_BEACONS BIT(0)
#define PRINTING_MASK_ZDO     BIT(1)
#define PRINTING_MASK_NWK     BIT(2)
#define PRINTING_MASK_APS     BIT(3)
#define PRINTING_MASK_ZCL     BIT(4)
static uint8_t printingMask = PRINTING_MASK_ZCL; // always print zcl commands

static EmberPanId networkCreatorPanId = 0xFFFF;

#if !defined(EZSP_HOST)
static uint8_t commandData[128];
#endif

// -----------------------------------------------------------------------------
// Stack Callbacks

void emberIncomingCommandHandler(EmberZigbeeCommandType commandType,
                                 EmberMessageBuffer commandBuffer,
                                 uint8_t indexOfCommand,
                                 void *data)
{
#if !defined(EZSP_HOST)
  EmberStatus status;
  uint8_t commandId, commandDataLength;

  // FIXME: protect against bad lengths.
  commandDataLength = emberMessageBufferLength(commandBuffer) - indexOfCommand;
  if (indexOfCommand + commandDataLength
      > emberMessageBufferLength(commandBuffer)) {
    return;
  }

  emberCopyFromLinkedBuffers(commandBuffer,
                             indexOfCommand,
                             commandData,
                             commandDataLength);
  commandId = commandData[0];
  switch (commandType) {
  case EMBER_ZIGBEE_COMMAND_TYPE_NWK:
    if (printingMask & PRINTING_MASK_NWK) {
      // FIXME: talk to Simon about sequence and security bytes.
      emberAfCorePrint("nwk:rx seq AC sec 28 cmd %X payload[",
                       commandId);
      emberAfCorePrintBuffer(commandData+1, commandDataLength-1, true); // spaces?
      emberAfCorePrintln("]");
    }
    if (commandId == NWK_LEAVE_COMMAND
        && emAfPluginTestHarnessZ3IgnoreLeaveCommands) {
      // Ignore the leave by turning off the request bit in the options.
      uint8_t newOptions
        = emberGetLinkedBuffersByte(commandBuffer, indexOfCommand + 1) & ~BIT(6);
      emberSetLinkedBuffersByte(commandBuffer,
                                indexOfCommand + 1,
                                newOptions);
    }
    break;
  case EMBER_ZIGBEE_COMMAND_TYPE_APS:
    if (printingMask & PRINTING_MASK_APS) {
      uint8_t fc = getApsFrameControl(commandBuffer, indexOfCommand);
      emberAfCorePrint("aps:rx seq AC fc %X cmd %X payload[",
                       fc,
                       commandId);
      emberAfCorePrintBuffer(commandData+1, commandDataLength-1, true); // spaces?
      emberAfCorePrintln("]");
    }
    break;
  case EMBER_ZIGBEE_COMMAND_TYPE_BEACON:
    if (printingMask & PRINTING_MASK_BEACONS) {
      // Cheat since we know where the pan id lives.
      uint16_t panId
        = emberAfGetInt16u(emberMessageBufferContents(commandBuffer), 9, 11);
      emberAfCorePrint("beacon:rx 0x%2X, AP 0x%p, EP ",
                       panId,
                       ((commandData[1] & BEACON_ASSOCIATION_PERMIT_BIT)
                        ? "1"
                        : "0"));
      emberAfCorePrintBuffer(commandData + BEACON_EXTENDED_PAN_ID_INDEX,
                             EXTENDED_PAN_ID_SIZE,
                             true); // spaces?
      emberAfCorePrintln("");
    }
    break;
  case EMBER_ZIGBEE_COMMAND_TYPE_MAC:
    if (printingMask & PRINTING_MASK_BEACONS) {
      if (commandId == BEACON_REQUEST_COMMAND) {
        emberAfCorePrintln("beacon-req:rx");
      }
    }
    break;
  case EMBER_ZIGBEE_COMMAND_TYPE_ZDO: {
    EmberApsFrame *apsFrame = (EmberApsFrame *)data;
    if (printingMask & PRINTING_MASK_ZDO) {
      emberAfCorePrint("zdo:t%4X:%p seq %X cmd %2X payload[",
                       emberAfGetCurrentTime(),
                       "rx",
                       commandData[0],
                       apsFrame->clusterId);
      emberAfCorePrintBuffer(commandData + ZDO_MESSAGE_OVERHEAD,
                             commandDataLength - ZDO_MESSAGE_OVERHEAD,
                             true); // spaces?
      emberAfCorePrintln("]");
    }
    status = emAfPluginTestHarnessZ3ZdoCommandResponseHandler(commandBuffer,
                                                              indexOfCommand,
                                                              apsFrame);
    if (status != EMBER_INVALID_CALL) {
      emberAfCorePrintln("%p: %p: cluster: 0x%02X status: 0x%X",
                         TEST_HARNESS_Z3_PRINT_NAME,
                         "ZDO negative command",
                         apsFrame->clusterId | 0x8000,
                         status);
    }
  }
    break;
  case EMBER_ZIGBEE_COMMAND_TYPE_ZCL:
    if (printingMask & PRINTING_MASK_ZCL) {
      emberAfCorePrint("t%4x:rx len %d, ep %X, clus 0x1000 (ZLL Commissioning) FC %X seq %X cmd %X payload[",
                       emberAfGetCurrentTime(),
                       commandDataLength,
                       1,               // fake endpoint 1 for zll 
                       commandData[0],  // frame control
                       commandData[1],  // sequence
                       commandData[2]); // command
      emberAfCorePrintBuffer(commandData + 3,
                             commandDataLength - 3,
                             true); // spaces?
      emberAfCorePrintln("]");
      emAfPluginTestHarnessZ3ZllCommandCallback(commandBuffer,
                                                indexOfCommand,
                                                data); // source eui64
    }
    break;
  default:
    emberAfCorePrintln("Error: unrecognized EmberZigbeeCommandType.");
  }
#endif // EZSP_HOST
}

// -----------------------------------------------------------------------------
// Printing

void emAfPluginTestHarnessZ3PrintingCommand(void)
{
  bool enabled = (emberStringCommandArgument(-1, NULL)[0] == 'e');
  uint8_t mask;

  switch (emberStringCommandArgument(-2, NULL)[0]) {
  case 'b':
    mask = PRINTING_MASK_BEACONS;
    break;
  case 'z':
    mask = PRINTING_MASK_ZDO;
    break;
  case 'n':
    mask = PRINTING_MASK_NWK;
    break;
  case 'a':
    mask = PRINTING_MASK_APS;
    break;
  default:
    mask = 0xFF;
  }

  if (enabled) {
    SETBITS(printingMask, mask);
  } else {
    CLEARBITS(printingMask, mask);
  }

  emberAfCorePrintln("%puccessfully %p printing.",
                     (mask == 0xFF ? "Uns" : "S"),
                     (enabled ? "enabled" : "disabled"));
}

// -----------------------------------------------------------------------------
// Framework Callbacks

EmberNodeType emberAfPluginNetworkSteeringGetNodeTypeCallback(EmberAfPluginNetworkSteeringJoiningState state)
{
  switch (emAfPluginTestHarnessZ3DeviceMode) {
  case EM_AF_PLUGIN_TEST_HARNESS_Z3_DEVICE_MODE_ZR_NOT_ADDRESS_ASSIGNABLE:
  case EM_AF_PLUGIN_TEST_HARNESS_Z3_DEVICE_MODE_ZR_ADDRESS_ASSIGNABLE:
    return EMBER_ROUTER;
  case EM_AF_PLUGIN_TEST_HARNESS_Z3_DEVICE_MODE_SLEEPY_ZED_NOT_ADDRESS_ASSIGNABLE:
  case EM_AF_PLUGIN_TEST_HARNESS_Z3_DEVICE_MODE_SLEEPY_ZED_ADDRESS_ASSIGNABLE:
    return EMBER_SLEEPY_END_DEVICE;
  case EM_AF_PLUGIN_TEST_HARNESS_Z3_DEVICE_MODE_ZED_NOT_ADDRESS_ASSIGNABLE:
  case EM_AF_PLUGIN_TEST_HARNESS_Z3_DEVICE_MODE_ZED_ADDRESS_ASSIGNABLE:
  default:
    return EMBER_END_DEVICE;
  }
}

void emberAfPluginTestHarnessZ3StackStatusCallback(EmberStatus status)
{
  if (status == EMBER_NETWORK_UP
      && emberAfGetNodeId() == EMBER_TRUST_CENTER_NODE_ID) {
    // Force the network open for joining if we are the trust center.
    // This is terrible security, but this plugin is for a terrible test
    // harness app, so I feel like it lines up.
    emberEventControlSetActive(emberAfPluginTestHarnessZ3OpenNetworkEventControl);
  }
}

void emberAfPluginTestHarnessZ3OpenNetworkEventHandler(void)
{
  emberEventControlSetInactive(emberAfPluginTestHarnessZ3OpenNetworkEventControl);

  if (emberAfGetNodeId() == EMBER_TRUST_CENTER_NODE_ID) {
    EmberEUI64 wildcardEui64 = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,};
    EmberKeyData centralizedKey = ZIGBEE_PROFILE_INTEROPERABILITY_LINK_KEY;
    EmberStatus status;
    uint16_t transientKeyTimeoutS;

    // Make sure the trust center lets everyone on the network and
    // allows key requests.
#ifdef EZSP_HOST
    status = ezspAddTransientLinkKey(wildcardEui64, &centralizedKey);
    ezspSetPolicy(EZSP_TC_KEY_REQUEST_POLICY, EZSP_GENERATE_NEW_TC_LINK_KEY);
    ezspGetConfigurationValue(EZSP_CONFIG_TRANSIENT_KEY_TIMEOUT_S,
                              &transientKeyTimeoutS);
#else
    status = emberAddTransientLinkKey(wildcardEui64, &centralizedKey);
    emberTrustCenterLinkKeyRequestPolicy = EMBER_GENERATE_NEW_TC_LINK_KEY;
    transientKeyTimeoutS = emberTransientKeyTimeoutS;
#endif

    // Make sure we reopen the network before the transient key disappears.
    // Add in a timing slop of a second to prevent any race conditions.
    emberEventControlSetDelayMS(emberAfPluginTestHarnessZ3OpenNetworkEventControl,
                                ((transientKeyTimeoutS * MILLISECOND_TICKS_PER_SECOND)
                                 - MILLISECOND_TICKS_PER_SECOND));
  }
}

// -----------------------------------------------------------------------------
// Core CLI commands

// plugin test-harness z3 reset
void emAfPluginTestHarnessZ3ResetCommand(void)
{
  emberAfZllResetToFactoryNew();

  emberAfCorePrintln("%p: %p: 0x%X",
                     TEST_HARNESS_Z3_PRINT_NAME,
                     "Reset",
                     EMBER_SUCCESS);
}

// plugin test-harness z3 set-device-mode <mode:1>
void emAfPluginTestHarnessZ3SetDeviceModeCommand(void)
{
  EmberStatus status = EMBER_BAD_ARGUMENT;
  EmAfPluginTestHarnessZ3DeviceMode mode
    = (EmAfPluginTestHarnessZ3DeviceMode)emberUnsignedCommandArgument(0);

  if (mode <= EM_AF_PLUGIN_TEST_HARNESS_Z3_DEVICE_MODE_MAX) {
    emAfPluginTestHarnessZ3DeviceMode = mode;
    status = EMBER_SUCCESS;
  }

  emberAfCorePrintln("%p: %p: 0x%X",
                     TEST_HARNESS_Z3_PRINT_NAME,
                     "Set device mode",
                     status);
}

// plugin test-harness z3 set-short-address
void emAfPluginTestHarnessZ3SetShortAddressCommand(void)
{
  emberAfCorePrintln("%p: %p: 0x%X",
                     TEST_HARNESS_Z3_PRINT_NAME,
                     "Set short address",
                     EMBER_INVALID_CALL);
}

// plugin test-harness z3 legacy-profile-id
void emAfPluginTestHarnessZ3LegacyProfileCommand(void)
{
  bool enable = (emberStringCommandArgument(-1, NULL)[0] == 'e');
  (void)enable;
}

// plugin test-harness z3 set-pan-id
void emAfPluginTestHarnessSetNetworkCreatorPanId(void)
{
  networkCreatorPanId = (EmberPanId)emberUnsignedCommandArgument(0);

  emberAfCorePrintln("Network Creator PAN ID = 0x%2X", networkCreatorPanId);
}

EmberPanId emberAfPluginNetworkCreatorGetPanIdCallback(void)
{
  return (networkCreatorPanId == 0xFFFF
          ? halCommonGetRandom()
          : networkCreatorPanId);
}

// plugin test-harness z3 platform
void emAfPluginTestHarnessZ3PlatformCommand(void)
{
  emberAfCorePrintln("Platform: Silicon Labs");
  emberAfCorePrint("EmberZNet ");
  emAfCliVersionCommand();
}

// plugin test-harness z3 install-code clear
// plugin test-harness z3 install-code set <code>
void emAfPluginTestHarnessZ3InstallCodeClearOrSetCommand(void)
{
  bool doClear = (emberStringCommandArgument(-1, NULL)[0] == 'c');

  emberAfCorePrintln("%p: %p %p: 0x%X",
                     TEST_HARNESS_Z3_PRINT_NAME,
                     "Install code",
                     (doClear ? "clear" : "set"),
                     EMBER_INVALID_CALL);
}
