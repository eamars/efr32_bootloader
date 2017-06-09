//
// test-harness-z3-zll.c
//
// August 3, 2015
// Refactored November 23, 2015
//
// ZigBee 3.0 touchlink test harness functionality
//

// To pull the ZLL types in, af-types.h required EMBER_AF_PLUGIN_ZLL_COMMISSIONING
// to be #define'd. In unit tests, we don't get this. So fake it.
#ifdef EMBER_SCRIPTED_TEST
  #define EMBER_AF_PLUGIN_ZLL_COMMISSIONING
#endif

#include "app/framework/include/af.h"

#include "test-harness-z3-core.h"
#include "test-harness-z3-zll.h"

#include "app/framework/plugin/zll-commissioning/zll-commissioning.h"

// -----------------------------------------------------------------------------
// Types

typedef struct {
  uint32_t transactionId;
  EmberAfProfileId profileId;
  uint16_t keyBitmask;
  bool setReservedInformationBits;
  uint8_t networkUpdateId;
  uint8_t numberSubDevices;
  EmberEUI64 destinationEui64;
} ScanResponseData;

// -----------------------------------------------------------------------------
// Globals

// We need this extern'd for unit tests since the zll-commissioning.h extern
// might not be compiled in.
#if defined(EMBER_SCRIPTED_TEST)
extern uint32_t emAfZllSecondaryChannelMask;
#endif

static EmberZllNetwork zllNetwork = { {0,}, {0,0,0}, };
#define initZllNetwork(network)                                             \
  do { MEMMOVE(&zllNetwork, network, sizeof(EmberZllNetwork)); } while (0);
#define deinitZllNetwork()                                                  \
  do { zllNetwork.securityAlgorithm.transactionId = 0; } while (0);
#define zllNetworkIsInit()                                                  \
  (zllNetwork.securityAlgorithm.transactionId != 0)

#define STATE_PRIMARY_CHANNELS (0x01)
#define STATE_SCANNING         (0x02)
static uint8_t state;

#define NEGATIVE_BEHAVIOR_IMMEDIATE_RESPONSE (BIT32(0))
#define NEGATIVE_BEHAVIOR_DELAY_RESPONSE     (BIT32(1))
#define NEGATIVE_BEHAVIOR_SUPRESS_RESPONSE   (BIT32(2))
#define NEGATIVE_BEHAVIOR_POSITIVE_RESPONSE  (BIT32(3))
#define NEGATIVE_BEHAVIOR_BAD_TRANSACTION_ID (BIT32(4))
static uint32_t negativeBehaviorMask = 0;
static uint8_t negativeBehaviorCommandId = 0xFF;

static ScanResponseData globalScanResponseData = {0,};

EmberEventControl emberAfPluginTestHarnessZ3ZllStuffEventControl;
#define ZLL_STUFF_EVENT_CONTROL_ACTION_SCAN_RESPONSE (0x00)
#define ZLL_STUFF_EVENT_CONTROL_ACTION_NULL          (0xFF)
static uint8_t zllStuffEventControlAction;

#define RESPONSE_ID 0xACACACAC

#define ENDPOINT       0x01
#define DEVICE_ID      0x1234
#define DEVICE_VERSION 0x01
#define GROUP_COUNT    0x00

// -----------------------------------------------------------------------------
// Util

static EmberStatus startScan(void)
{
  EmberNodeType nodeType;

  nodeType = (((emAfPluginTestHarnessZ3DeviceMode
                == EM_AF_PLUGIN_TEST_HARNESS_Z3_DEVICE_MODE_ZR_NOT_ADDRESS_ASSIGNABLE)
               || (emAfPluginTestHarnessZ3DeviceMode
                   == EM_AF_PLUGIN_TEST_HARNESS_Z3_DEVICE_MODE_ZR_ADDRESS_ASSIGNABLE))
              ? EMBER_ROUTER
              : EMBER_END_DEVICE);

  if ((emAfPluginTestHarnessZ3DeviceMode
       == EM_AF_PLUGIN_TEST_HARNESS_Z3_DEVICE_MODE_ZR_NOT_ADDRESS_ASSIGNABLE)
      || (emAfPluginTestHarnessZ3DeviceMode
          == EM_AF_PLUGIN_TEST_HARNESS_Z3_DEVICE_MODE_ZED_NOT_ADDRESS_ASSIGNABLE)) {
    EmberTokTypeStackZllData token;
    emberZllGetTokenStackZllData(&token);
    token.bitmask &= ~EMBER_ZLL_STATE_ADDRESS_ASSIGNMENT_CAPABLE;
    emberZllSetTokenStackZllData(&token);
  }

  return emberZllStartScan(emAfZllPrimaryChannelMask | emAfZllSecondaryChannelMask,
                           0, // default power
                           nodeType);
}

static uint8_t zigbeeInformation(void)
{
  uint8_t byte = 0x00;

  switch (emAfPluginTestHarnessZ3DeviceMode) {
  case EM_AF_PLUGIN_TEST_HARNESS_Z3_DEVICE_MODE_ZR_NOT_ADDRESS_ASSIGNABLE:
  case EM_AF_PLUGIN_TEST_HARNESS_Z3_DEVICE_MODE_ZR_ADDRESS_ASSIGNABLE:
    byte |= 0x01;
    // fallthrough
  case EM_AF_PLUGIN_TEST_HARNESS_Z3_DEVICE_MODE_ZED_NOT_ADDRESS_ASSIGNABLE:
  case EM_AF_PLUGIN_TEST_HARNESS_Z3_DEVICE_MODE_ZED_ADDRESS_ASSIGNABLE:
    byte |= 0x04;
    break;
  case EM_AF_PLUGIN_TEST_HARNESS_Z3_DEVICE_MODE_SLEEPY_ZED_NOT_ADDRESS_ASSIGNABLE:
  case EM_AF_PLUGIN_TEST_HARNESS_Z3_DEVICE_MODE_SLEEPY_ZED_ADDRESS_ASSIGNABLE:
  default:
    ; // nothing
  }

  if (globalScanResponseData.setReservedInformationBits) {
    byte |= (0xF0 | 0x08);
  }

  return byte;
}

static uint8_t zllInformation(void)
{
  uint8_t byte;
  EmberTokTypeStackZllData token;

  emberZllGetTokenStackZllData(&token);
  byte = BYTE_0(token.bitmask);

  switch (emAfPluginTestHarnessZ3DeviceMode) {
  case EM_AF_PLUGIN_TEST_HARNESS_Z3_DEVICE_MODE_ZR_NOT_ADDRESS_ASSIGNABLE:
  case EM_AF_PLUGIN_TEST_HARNESS_Z3_DEVICE_MODE_ZED_NOT_ADDRESS_ASSIGNABLE:
  case EM_AF_PLUGIN_TEST_HARNESS_Z3_DEVICE_MODE_SLEEPY_ZED_NOT_ADDRESS_ASSIGNABLE:
    byte &= ~EMBER_ZLL_STATE_ADDRESS_ASSIGNMENT_CAPABLE;
    break;
  case EM_AF_PLUGIN_TEST_HARNESS_Z3_DEVICE_MODE_ZR_ADDRESS_ASSIGNABLE:
  case EM_AF_PLUGIN_TEST_HARNESS_Z3_DEVICE_MODE_ZED_ADDRESS_ASSIGNABLE:
  case EM_AF_PLUGIN_TEST_HARNESS_Z3_DEVICE_MODE_SLEEPY_ZED_ADDRESS_ASSIGNABLE:
  default:
    byte |= EMBER_ZLL_STATE_ADDRESS_ASSIGNMENT_CAPABLE;
    break;
  }

  if (globalScanResponseData.setReservedInformationBits) {
    byte |= (0x40 | 0x20 | 0x10);
  }

  return byte;
}

// Uses the global variable globalScanResponseData.
static EmberStatus sendScanResponse(void)
{
  EmberNodeType nodeType;
  EmberNetworkParameters networkParams;
  EmberStatus status;

  uint8_t payload[39]; // scan response payload is always 39 bytes
  uint8_t *finger = payload;

  status = emberAfGetNetworkParameters(&nodeType, &networkParams);
  if (status != EMBER_SUCCESS) {
    return status;
  }

  // ZCL.
  *finger++ = 0x19; // FC (cluster-specific, server to client)
  *finger++ = 0xAC; // sequence
  *finger++ = 0x01; // command ID (scan response)

  // ZLL.
  emberAfCopyInt32u(finger, 0, globalScanResponseData.transactionId);
  finger += sizeof(globalScanResponseData.transactionId);
  *finger++ = 0x00; // RSSI correction - whatever
  *finger++ = zigbeeInformation();
  *finger++ = zllInformation();
  emberAfCopyInt16u(finger, 0, globalScanResponseData.keyBitmask);
  finger += sizeof(globalScanResponseData.keyBitmask);
  emberAfCopyInt32u(finger, 0, RESPONSE_ID);
  finger += sizeof(RESPONSE_ID);
  MEMMOVE(finger, networkParams.extendedPanId, EXTENDED_PAN_ID_SIZE);
  finger += EXTENDED_PAN_ID_SIZE;
  *finger++ = globalScanResponseData.networkUpdateId;
  *finger++ = networkParams.radioChannel;
  emberAfCopyInt16u(finger, 0, networkParams.panId);
  finger += sizeof(networkParams.panId);
  *finger++ = globalScanResponseData.numberSubDevices;
  *finger++ = 0x01; // endpoint
  emberAfCopyInt16u(finger, 0, globalScanResponseData.profileId);
  finger += sizeof(globalScanResponseData.profileId);
  emberAfCopyInt16u(finger, 0, DEVICE_ID);
  finger += sizeof(DEVICE_ID);
  *finger++ = DEVICE_VERSION;
  *finger++ = GROUP_COUNT;

  status = emberAfSendInterPan(0xFFFF, // pan ID
                               globalScanResponseData.destinationEui64,
                               emberLookupNodeIdByEui64(globalScanResponseData.destinationEui64),
                               0xFFFF, // multicast id - whatever
                               ZCL_ZLL_COMMISSIONING_CLUSTER_ID,
                               globalScanResponseData.profileId,
                               sizeof(payload),
                               payload);

  return status;
}

static uint8_t networkUpdateId(void)
{
  EmberNodeType nodeType;
  EmberNetworkParameters networkParams;

  emberAfGetNetworkParameters(&nodeType, &networkParams);

  return networkParams.nwkUpdateId;
}

// -----------------------------------------------------------------------------
// Touchlink CLI Commands

// plugin test-harness z3 touchlink scan-request-process <linkInitiator:1>
// <unused:1> <options:4>
void emAfPluginTestHarnessZ3TouchlinkScanRequestProcessCommand(void)
{
  EmberStatus status = EMBER_INVALID_CALL;

#ifndef EZSP_HOST
  uint8_t linkInitiator = (uint8_t)emberUnsignedCommandArgument(0);
  uint32_t options      = emAfPluginTestHarnessZ3GetSignificantBit(2);

  // We currently don't have a way to use the link initiator option.
  (void)linkInitiator;

  if (options & BIT32(1)) {
    // Use current channel for scanning.
    emAfZllPrimaryChannelMask = BIT32(emberAfGetRadioChannel());
    emAfZllSecondaryChannelMask = 0;
  } else if (options & BIT32(4)) {
    // Only use secondary channels for scanning.
    emAfZllPrimaryChannelMask = 0;
    emAfZllSecondaryChannelMask = EMBER_ZLL_SECONDARY_CHANNEL_MASK;
  }

  if (options & BIT32(3)) {
    // ignore scan-requests
    emberZllSetPolicy(EMBER_ZLL_POLICY_DISABLED);
    status = EMBER_SUCCESS;
  } else {
    emberZllSetPolicy(EMBER_ZLL_POLICY_ENABLED);
    emberAfZllSetInitialSecurityState();
    state = (STATE_PRIMARY_CHANNELS | STATE_SCANNING);
    status = startScan();
  }
#endif /* EZSP_HOST */

  emberAfCorePrintln("%p: %p: 0x%X",
                     TEST_HARNESS_Z3_PRINT_NAME,
                     "Scan request process",
                     status);
}

// plugin test-harness z3 touchlink start-as-router
void emAfPluginTestHarnessZ3TouchlinkStartAsRouterCommand(void)
{
  EmberPanId panId = (EmberPanId)emberUnsignedCommandArgument(0);
  uint32_t options = emAfPluginTestHarnessZ3GetSignificantBit(1);
  EmberStatus status = EMBER_INVALID_CALL;

  // This options bitmask is currently unused.
  (void)options;

  if (zllNetworkIsInit()) {
    status = emAfZllFormNetwork(zllNetwork.zigbeeNetwork.channel, 0, panId);
  }

  emberAfCorePrintln("%p: %p: 0x%X",
                    TEST_HARNESS_Z3_PRINT_NAME,
                    "Start as router",
                    status);
}

// plugin test-harness z3 touchlink is-scanning
void emAfPluginTestHarnessZ3TouchlinkIsScanningCommand(void)
{
  emberAfCorePrintln("scanning:%p",
                     (state & STATE_SCANNING ? "true" : "false"));
}

// plugin test-harness z3 touchlink device-information-request
// <startIndex:1> <options:4>
void emAfPluginTestHarnessZ3TouchlinkDeviceInformationRequestCommand(void)
{
  uint8_t startIndex = (uint8_t)emberUnsignedCommandArgument(0);
  uint32_t options   = emAfPluginTestHarnessZ3GetSignificantBit(1);
  EmberStatus status = EMBER_INVALID_CALL;

  uint32_t interpanTransactionId = zllNetwork.securityAlgorithm.transactionId;
  if (options & BIT(0)) {
    interpanTransactionId --;
  }

  if (zllNetworkIsInit()) {
    emberAfFillExternalBuffer(EM_AF_PLUGIN_TEST_HARNESS_Z3_ZLL_CLIENT_TO_SERVER_FRAME_CONTROL,
                              ZCL_ZLL_COMMISSIONING_CLUSTER_ID,
                              ZCL_DEVICE_INFORMATION_REQUEST_COMMAND_ID,
                              "wu",
                              interpanTransactionId,
                              startIndex);
    status = emberAfSendCommandInterPan(0xFFFF,                // destination pan id
                                        zllNetwork.eui64,
                                        EMBER_NULL_NODE_ID,    // node id - ignored
                                        0x0000,                // group id - ignored
                                        EMBER_ZLL_PROFILE_ID);
  }

  emberAfCorePrintln("%p: %p: 0x%X",
                     TEST_HARNESS_Z3_PRINT_NAME,
                     "Device information request",
                     status);
}

// plugin test-harness z3 touchlink identify-request <duration:2> <options:4>
void emAfPluginTestHarnessZ3TouchlinkIdentifyRequestCommand(void)
{
  uint16_t duration = (uint16_t)emberUnsignedCommandArgument(0);
  uint32_t options  = emAfPluginTestHarnessZ3GetSignificantBit(1);
  EmberStatus status = EMBER_INVALID_CALL;

  uint32_t interpanTransactionId = zllNetwork.securityAlgorithm.transactionId;
  if (options & BIT(0)) {
    interpanTransactionId --;
  }

  if (zllNetworkIsInit()) {
    emberAfFillExternalBuffer(EM_AF_PLUGIN_TEST_HARNESS_Z3_ZLL_CLIENT_TO_SERVER_FRAME_CONTROL,
                              ZCL_ZLL_COMMISSIONING_CLUSTER_ID,
                              ZCL_IDENTIFY_REQUEST_COMMAND_ID,
                              "wv",
                              interpanTransactionId,
                              duration);
    status = emberAfSendCommandInterPan(0xFFFF,                // destination pan id
                                        zllNetwork.eui64,
                                        EMBER_NULL_NODE_ID,    // node id - ignored
                                        0x0000,                // group id - ignored
                                        EMBER_ZLL_PROFILE_ID);
  }

  emberAfCorePrintln("%p: %p: 0x%X",
                     TEST_HARNESS_Z3_PRINT_NAME,
                     "Identify request",
                     status);
}

// plugin test-harness z3 touchlink reset-to-factory-new-request <options:4>
void emAfPluginTestHarnessZ3TouchlinkRTFNRequestCommand(void)
{
  uint32_t options  = emAfPluginTestHarnessZ3GetSignificantBit(0);
  EmberStatus status = EMBER_INVALID_CALL;

  // We currently do not have a way to use the option bitmask.
  (void)options;

  if (zllNetworkIsInit()) {
    emberAfFillExternalBuffer(EM_AF_PLUGIN_TEST_HARNESS_Z3_ZLL_CLIENT_TO_SERVER_FRAME_CONTROL,
                              ZCL_ZLL_COMMISSIONING_CLUSTER_ID,
                              ZCL_RESET_TO_FACTORY_NEW_REQUEST_COMMAND_ID,
                              "w",
                              zllNetwork.securityAlgorithm.transactionId);
    status = emberAfSendCommandInterPan(0xFFFF,                // destination pan id
                                        zllNetwork.eui64,
                                        EMBER_NULL_NODE_ID,    // node id - ignored
                                        0x0000,                // group id - ignored
                                        EMBER_ZLL_PROFILE_ID);
  }

  emberAfCorePrintln("%p: %p: 0x%X",
                     TEST_HARNESS_Z3_PRINT_NAME,
                     "Reset to factory new request",
                     status);
}

// plugin test-harness z3 touchlink network-start-request <dstAddress:2>
// <freeAddrBegin:2> <freeAddrEnd:2> <groupIdBegin:2> <groupIdEnd:2> <options:4>
// plugin test-harness z3 touchlink network-join-router-request <dstAddress:2>
// <freeAddrBegin:2> <freeAddrEnd:2> <groupIdBegin:2> <groupIdEnd:2> <options:4>
// plugin test-harness z3 touchlink network-join-end-device-request <dstAddress:2>
// <freeAddrBegin:2> <freeAddrEnd:2> <groupIdBegin:2> <groupIdEnd:2> <options:4>
void emAfPluginTestHarnessZ3TouchlinkNetworkCommand(void)
{
  EmberNodeId nodeId              = (EmberNodeId)emberUnsignedCommandArgument(0);
  EmberNodeId freeAddrBegin       = (EmberNodeId)emberUnsignedCommandArgument(1);
  EmberNodeId freeAddrEnd         = (EmberNodeId)emberUnsignedCommandArgument(2);
  EmberMulticastId freeGroupBegin = (EmberMulticastId)emberUnsignedCommandArgument(3);
  EmberMulticastId freeGroupEnd   = (EmberMulticastId)emberUnsignedCommandArgument(4);
  uint32_t options                = emAfPluginTestHarnessZ3GetSignificantBit(5);

  uint8_t command = 0;
  EmberTokTypeStackZllData token;
  EmberStatus status = EMBER_INVALID_CALL;
  EmberZllNetwork mangledNetwork;

  // We currently do not have a way to use the nodeId.
  (void)nodeId;

  if (!zllNetworkIsInit()) {
    goto done;
  }

  switch (emberStringCommandArgument(-1, NULL)[13]) {
  case '-':
    command = ZCL_NETWORK_START_REQUEST_COMMAND_ID;
    break;
  case 'r':
    command = ZCL_NETWORK_JOIN_ROUTER_REQUEST_COMMAND_ID;
    break;
  case 'e':
    command = ZCL_NETWORK_JOIN_END_DEVICE_REQUEST_COMMAND_ID;
    break;
  default:
    status = EMBER_BAD_ARGUMENT;
    goto done;
  }

  MEMMOVE(&mangledNetwork, &zllNetwork, sizeof(EmberZllNetwork));

  emberZllGetTokenStackZllData(&token);
  token.freeNodeIdMin  = freeAddrBegin;
  token.freeNodeIdMax  = freeAddrEnd;
  token.freeGroupIdMin = freeGroupBegin;  
  token.freeGroupIdMax = freeGroupEnd;
  if (command == ZCL_NETWORK_START_REQUEST_COMMAND_ID) {
    // Say that we are FN so that we will indeed send a network start.
    token.bitmask |= EMBER_ZLL_STATE_FACTORY_NEW;
  } else {
    // Say that we are not FN so we will send a network join.
    token.bitmask &= ~EMBER_ZLL_STATE_FACTORY_NEW;
    // Mangle the target device type so that we get the command we want.
    mangledNetwork.nodeType
      = (command == ZCL_NETWORK_JOIN_ROUTER_REQUEST_COMMAND_ID
         ? EMBER_ROUTER
         : EMBER_END_DEVICE);
  }
  emberZllSetTokenStackZllData(&token);

  if (command == ZCL_NETWORK_START_REQUEST_COMMAND_ID) {
    if (options & BIT32(0)) {
      mangledNetwork.securityAlgorithm.transactionId --;
    }
  } else { // command == network join
    switch (options) {
    case BIT32(0):
      // TODO: no encryption.
      break;
    case BIT32(1):
      mangledNetwork.securityAlgorithm.bitmask = EMBER_ZLL_KEY_MASK_DEVELOPMENT;
      break;
    case BIT32(2):
      // TODO: broadcast.
      break;
    case BIT32(3):
      mangledNetwork.securityAlgorithm.transactionId --;
      break;
    default:
      ; // whatever
    }
  }

  emberAfZllSetInitialSecurityState();
  status = emberZllJoinTarget(&mangledNetwork);

 done:
  emberAfCorePrintln("%p: %p: 0x%X",
                     TEST_HARNESS_Z3_PRINT_NAME,
                     (command == ZCL_NETWORK_START_REQUEST_COMMAND_ID
                      ? "Network start request"
                      : (command == ZCL_NETWORK_JOIN_ROUTER_REQUEST_COMMAND_ID
                         ? "Network join router request"
                         : "Network join end device request")),
                     status);
}

// plugin test-harness z3 touchlink network-update-request <options:4>
void emAfPluginTestHarnessZ3TouchlinkNetworkUpdateRequestCommand(void)
{
  uint32_t options = emAfPluginTestHarnessZ3GetSignificantBit(0);

  EmberStatus status = EMBER_INVALID_CALL;
  EmberNodeType nodeType;
  EmberNetworkParameters networkParameters;
  uint8_t mangledNwkUpdateId;

  emberAfGetNetworkParameters(&nodeType, &networkParameters);

  mangledNwkUpdateId = networkParameters.nwkUpdateId;
  if (options == BIT32(0)) {
    mangledNwkUpdateId += 5;
  }

  if (zllNetworkIsInit()) {
    emberAfFillExternalBuffer(EM_AF_PLUGIN_TEST_HARNESS_Z3_ZLL_CLIENT_TO_SERVER_FRAME_CONTROL,
                              ZCL_ZLL_COMMISSIONING_CLUSTER_ID,
                              ZCL_NETWORK_UPDATE_REQUEST_COMMAND_ID,
                              "w8uuvv",
                              zllNetwork.securityAlgorithm.transactionId,
                              networkParameters.extendedPanId,
                              mangledNwkUpdateId,
                              networkParameters.radioChannel,
                              networkParameters.panId,
                              emberAfGetNodeId());
    status = emberAfSendCommandInterPan(0xFFFF, // destination pan id
                                        zllNetwork.eui64,
                                        EMBER_NULL_NODE_ID, // node id - ignored
                                        0x0000,             // group id - ignored
                                        EMBER_ZLL_PROFILE_ID);
  }

  emberAfCorePrintln("%p: %p: 0x%X",
                     TEST_HARNESS_Z3_PRINT_NAME,
                     "Network update request",
                     status);
}

// plugin test-harness z3 touchlink get-group-identifiers-request
// <startIndex:1>
void emAfPluginTestHarnessZ3TouchlinkGetGroupIdentifiersRequestCommand(void)
{
  uint8_t startIndex = emberUnsignedCommandArgument(0);
  EmberStatus status = EMBER_INVALID_CALL;
  EmberNodeId destination;
  
  if (zllNetworkIsInit()) {
    if ((destination = zllNetwork.nodeId) == EMBER_NULL_NODE_ID) {
      destination = emberLookupNodeIdByEui64(zllNetwork.eui64);
    }
    emberAfFillCommandZllCommissioningClusterGetGroupIdentifiersRequest(startIndex);
    emberAfSetCommandEndpoints(1, 1); // endpoints?
    status = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, destination);
  }
                                     
  emberAfCorePrintln("%p: %p: 0x%X",
                     TEST_HARNESS_Z3_PRINT_NAME,
                     "Get group identifiers request",
                     status);
}

// plugin test-harness z3 touchlink get-endpoint-list-request
// <startIndex:1>
void emAfPluginTestHarnessZ3TouchlinkGetEndpointListRequestCommand(void)
{
  uint8_t startIndex = emberUnsignedCommandArgument(0);
  EmberStatus status = EMBER_INVALID_CALL;
  EmberNodeId destination;
  
  if (zllNetworkIsInit()) {
    if ((destination = zllNetwork.nodeId) == EMBER_NULL_NODE_ID) {
      destination = emberLookupNodeIdByEui64(zllNetwork.eui64);
    }
    emberAfFillCommandZllCommissioningClusterGetEndpointListRequest(startIndex);
    emberAfSetCommandEndpoints(1, 1); // endpoints?
    status = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, destination);
  }
                                     
  emberAfCorePrintln("%p: %p: 0x%X",
                     TEST_HARNESS_Z3_PRINT_NAME,
                     "Get endpoint list request",
                     status);
}

// plugin test-harness z3 touchlink scan-response-config <options:4>
void emAfPluginTestHarnessZ3TouchlinkScanResponseConfigCommand(void)
{
  uint32_t options = emAfPluginTestHarnessZ3GetSignificantBit(0);

  globalScanResponseData.keyBitmask = EMBER_ZLL_KEY_MASK_CERTIFICATION;
  globalScanResponseData.profileId = EMBER_ZLL_PROFILE_ID;
  globalScanResponseData.setReservedInformationBits = false;
  globalScanResponseData.networkUpdateId = networkUpdateId();
  globalScanResponseData.numberSubDevices = 1;

  switch (options) {
  case BIT32(0):
    negativeBehaviorMask |= NEGATIVE_BEHAVIOR_BAD_TRANSACTION_ID;
    break;
  case BIT32(1):
    negativeBehaviorMask |= NEGATIVE_BEHAVIOR_POSITIVE_RESPONSE;
    break;
  case BIT32(2):
    negativeBehaviorMask |= NEGATIVE_BEHAVIOR_DELAY_RESPONSE;
    break;
  case BIT32(3):
    negativeBehaviorMask |= NEGATIVE_BEHAVIOR_IMMEDIATE_RESPONSE;
    globalScanResponseData.keyBitmask = 0x0000;
    break;
  case BIT32(4):
    negativeBehaviorMask |= NEGATIVE_BEHAVIOR_IMMEDIATE_RESPONSE;
    globalScanResponseData.keyBitmask = 0x0001;
    break;
  case BIT32(5):
    negativeBehaviorMask |= NEGATIVE_BEHAVIOR_IMMEDIATE_RESPONSE;
    globalScanResponseData.setReservedInformationBits = true;
    break;
  case BIT32(6):
    negativeBehaviorMask |= NEGATIVE_BEHAVIOR_IMMEDIATE_RESPONSE;
    globalScanResponseData.networkUpdateId = 0x00;
    break;
  case BIT32(7):
    negativeBehaviorMask |= NEGATIVE_BEHAVIOR_IMMEDIATE_RESPONSE;
    globalScanResponseData.networkUpdateId = 0x50;
    break;
  case BIT32(8):
    negativeBehaviorMask |= NEGATIVE_BEHAVIOR_SUPRESS_RESPONSE;
    break;
  case BIT32(9):
    negativeBehaviorMask |= NEGATIVE_BEHAVIOR_IMMEDIATE_RESPONSE;
    globalScanResponseData.numberSubDevices = 10;
    break;
  case BIT32(10):
    negativeBehaviorMask |= NEGATIVE_BEHAVIOR_IMMEDIATE_RESPONSE;
    globalScanResponseData.networkUpdateId = 0x5A;
    break;
  case BIT32(11):
    negativeBehaviorMask |= NEGATIVE_BEHAVIOR_IMMEDIATE_RESPONSE;
    globalScanResponseData.keyBitmask = 0x0020;
    break;
  default:
    ; // whatever
  }

  negativeBehaviorCommandId = ZCL_SCAN_REQUEST_COMMAND_ID;

  emberAfCorePrintln("%p: %p: 0x%X",
                     TEST_HARNESS_Z3_PRINT_NAME,
                     "Scan response config",
                     EMBER_SUCCESS);
}

// plugin test-harness z3 touchlink network-start-response-config <options:4>
void emAfPluginTestHarnessZ3TouchlinkNetworkStartResponseConfig(void)
{
  uint32_t options = emAfPluginTestHarnessZ3GetSignificantBit(0);

  switch (options) {
  case BIT32(0):
    negativeBehaviorMask |= NEGATIVE_BEHAVIOR_SUPRESS_RESPONSE;
    break;
  default:
    ; // whatever
  }

  negativeBehaviorCommandId = ZCL_NETWORK_START_REQUEST_COMMAND_ID;

  emberAfCorePrintln("%p: %p: 0x%X",
                     TEST_HARNESS_Z3_PRINT_NAME,
                     "Network start response config",
                     EMBER_SUCCESS);
}

// plugin test-harness z3 touchlink device-information-response-config
// <options:4>
void emAfPluginTestHarnessZ3TouchlinkDeviceInformationResponseConfigCommand(void)
{
  uint32_t options = emAfPluginTestHarnessZ3GetSignificantBit(0);

  if (options & BIT32(0)) {
    negativeBehaviorMask |= NEGATIVE_BEHAVIOR_SUPRESS_RESPONSE;
  }

  negativeBehaviorCommandId = ZCL_DEVICE_INFORMATION_REQUEST_COMMAND_ID;

  emberAfCorePrintln("%p: %p: 0x%X",
                     TEST_HARNESS_Z3_PRINT_NAME,
                     "Device information response config",
                     EMBER_SUCCESS);
}

// -----------------------------------------------------------------------------
// Framework callbacks

void emAfPluginTestHarnessZ3ZllNetworkFoundCallback(const EmberZllNetwork *networkInfo)
{
  initZllNetwork(networkInfo);
}

void emAfPluginTestHarnessZ3ZllScanCompleteCallback(EmberStatus status)
{
  if (!zllNetworkIsInit() && (state & STATE_PRIMARY_CHANNELS)) {
    state &= ~STATE_PRIMARY_CHANNELS;
    startScan();
  } else {
    state &= ~STATE_SCANNING;
    if (status != EMBER_SUCCESS) {
      deinitZllNetwork();
    } else {
      extern EmberStatus emSetLogicalAndRadioChannel(uint8_t channel);
      emSetLogicalAndRadioChannel(zllNetwork.zigbeeNetwork.channel);
    }
  }
}

void emAfPluginTestHarnessZ3ZllCommandCallback(EmberMessageBuffer command,
                                               uint8_t startIndex,
                                               EmberEUI64 sourceEui64)
{
  // The start index points at the beginning of the ZCL frame.
  // FRAME_CONTROL:1 | SEQUENCE:1 | COMMAND_ID:1 | start of transaction...
  uint32_t transactionIdIndex = startIndex + 3;
  uint8_t commandId = emberGetLinkedBuffersByte(command, startIndex + 2);
  uint32_t realTransactionId
    = emberGetLinkedBuffersLowHighInt32u(command, transactionIdIndex);

  // Ignore any negative behavior if the command doesn't match up.
  if (commandId != negativeBehaviorCommandId) {
    return;
  }

  globalScanResponseData.transactionId = realTransactionId;
  MEMMOVE(globalScanResponseData.destinationEui64, sourceEui64, EUI64_SIZE);

  if (negativeBehaviorMask
      & (NEGATIVE_BEHAVIOR_SUPRESS_RESPONSE
         | NEGATIVE_BEHAVIOR_DELAY_RESPONSE
         | NEGATIVE_BEHAVIOR_IMMEDIATE_RESPONSE)) {
    if (!(negativeBehaviorMask & NEGATIVE_BEHAVIOR_POSITIVE_RESPONSE)) {
      // Overwrite the transaction ID to be 0. This will make us not respond
      // to the command.
      emberSetLinkedBuffersLowHighInt32u(command, transactionIdIndex, 0);
    }

    if (negativeBehaviorMask & NEGATIVE_BEHAVIOR_DELAY_RESPONSE) {
      // Delay 8 seconds, since this is the transaction ID lifetime.
      emberEventControlSetDelayMS(emberAfPluginTestHarnessZ3ZllStuffEventControl,
                                  MILLISECOND_TICKS_PER_SECOND * 8);
      zllStuffEventControlAction = ZLL_STUFF_EVENT_CONTROL_ACTION_SCAN_RESPONSE;
    } else if (negativeBehaviorMask & NEGATIVE_BEHAVIOR_IMMEDIATE_RESPONSE) {
      // Send our own response immediately.
      emberEventControlSetActive(emberAfPluginTestHarnessZ3ZllStuffEventControl);
      zllStuffEventControlAction = ZLL_STUFF_EVENT_CONTROL_ACTION_SCAN_RESPONSE;
    }
  } else if (negativeBehaviorMask & NEGATIVE_BEHAVIOR_BAD_TRANSACTION_ID) {
    emberSetLinkedBuffersLowHighInt32u(command,
                                       transactionIdIndex,
                                       realTransactionId - 1);
  }

  if (!emberEventControlGetActive(emberAfPluginTestHarnessZ3ZllStuffEventControl)) {
    negativeBehaviorMask = 0;
    negativeBehaviorCommandId = 0xFF;
  }
}

void emberAfPluginTestHarnessZ3ZllStuffEventHandler(void)
{
  EmberStatus status;

  emberEventControlSetInactive(emberAfPluginTestHarnessZ3ZllStuffEventControl);

  switch (zllStuffEventControlAction) {
  case ZLL_STUFF_EVENT_CONTROL_ACTION_SCAN_RESPONSE:
    status = sendScanResponse();
    negativeBehaviorMask = 0;
    negativeBehaviorCommandId = 0xFF;
    break;
  default:
    status = EMBER_BAD_ARGUMENT;
  }

  emberAfCorePrintln("%p: %p (0x%X): 0x%X",
                     TEST_HARNESS_Z3_PRINT_NAME,
                     "ZLL event handler",
                     zllStuffEventControlAction,
                     status);

  zllStuffEventControlAction = ZLL_STUFF_EVENT_CONTROL_ACTION_NULL;
}