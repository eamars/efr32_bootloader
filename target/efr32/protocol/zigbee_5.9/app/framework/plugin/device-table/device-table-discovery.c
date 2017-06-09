// Copyright 2016 Silicon Laboratories, Inc.                                *80*

#include PLATFORM_HEADER
#ifdef EZSP_HOST
  // Includes needed for ember related functions for the EZSP host
  #include "stack/include/error.h"
  #include "stack/include/ember-types.h"
  #include "app/util/ezsp/ezsp-protocol.h"
  #include "app/util/ezsp/ezsp.h"
  #include "app/util/ezsp/serial-interface.h"
  #include "app/util/zigbee-framework/zigbee-device-common.h"
#else
  #include "stack/include/ember.h"
#endif

#include "hal/hal.h"
#include "app/util/serial/command-interpreter2.h"
#include "af.h"
#include "stack/include/event.h"
#include "app/util/source-route-host.h"
#include <stdlib.h>
#include "app/framework/plugin/device-table/device-table.h"
#include "app/framework/plugin/device-table/device-table-internal.h"

// Device Discovery declaractions
#define COMMAND_DELAY_QS  16
#define PJOIN_BROADCAST_PERIOD 1

// Number of times to attempt device discovery messages
#define NEW_DEVICE_RETRY_ATTEMPTS 7
#define MAX_RELAY_COUNT 10

#define APS_OPTION_DISCOVER EMBER_APS_OPTION_RETRY

// ZDO offsets
#define END_DEVICE_ANNOUNCE_IEEE_OFFSET 3
#define NETWORK_ADDRESS_RESPONSE_IEEE_OFFSET 2
#define IEEE_ADDRESS_RESPONSE_IEEE_OFFSET 2
#define IEEE_ADDRESS_RESPONSE_NODE_ID_OFFSET 10

// For the endpoint descriptor response, the endpoint is at location 5, and the
// device ID is two bytes at location 8.
#define ENDPOINT_DESCRIPTOR_ENDPOINT_LOCATION 5
#define ENDPOINT_DESCRIPTOR_DEVICE_ID_LOCATION 8

static uint8_t newDeviceEventState = 0x00;
static uint16_t currentDevice;
static uint16_t currentEndpoint;

static uint8_t currentRetryCount;
static uint16_t currentRetryNodeId;
static uint8_t permitJoinBroadcastCounter = (PJOIN_BROADCAST_PERIOD-1);

#define newDeviceEventControl emberAfPluginDeviceTableNewDeviceEventControl

static void resetRetry(EmberNodeId nodeId);
EmberEventControl newDeviceEventControl;

enum {
  DEVICE_DISCOVERY_STATE_IDLE = 0x00,
  DEVICE_DISCOVERY_STATE_ENDPOINTS_RETRY = 0x01,
  DEVICE_DISCOVERY_STATE_ENDPOINTS_WAITING = 0x02,
  DEVICE_DISCOVERY_STATE_ENDPOINTS_RECEIVED = 0x03,
  DEVICE_DISCOVERY_STATE_SIMPLE_RETRY = 0x04,
  DEVICE_DISCOVERY_STATE_SIMPLE_WAITING = 0x05,
  DEVICE_DISCOVERY_STATE_SIMPLE_RECEIVED = 0x06,
  DEVICE_DISCOVERY_STATE_ATTRIBUTES_RETRY = 0x07,
  DEVICE_DISCOVERY_STATE_ATTRIBUTES_WAITING = 0x08,
  DEVICE_DISCOVERY_STATE_ATTRIBUTES_RECEIVED = 0x09,
  DEVICE_DISCOVERY_STATE_REPORT_CONFIG_RETRY = 0x0a,
  DEVICE_DISCOVERY_STATE_REPORT_CONFIG_WAITING = 0x0b,
  DEVICE_DISCOVERY_STATE_REPORT_CONFIG_ACK_RECEIVED = 0x0c
};

#define STATUS_STRINGS_ARRAY_LENGTH 16
static PGM_P PGM statusStrings[] =
{
  "STANDARD_SECURITY_SECURED_REJOIN",
  "STANDARD_SECURITY_UNSECURED_JOIN",
  "DEVICE_LEFT",
  "STANDARD_SECURITY_UNSECURED_REJOIN",
  "HIGH_SECURITY_SECURED_REJOIN",
  "HIGH_SECURITY_UNSECURED_JOIN",
  "Reserved 6",
  "HIGH_SECURITY_UNSECURED_REJOIN"
  "Reserved 8",
  "Reserved 9",
  "Reserved 10",
  "Reserved 11",
  "Reserved 12",
  "Reserved 13",
  "Reserved 14",
  "Reserved 15"
};

// Forward declarations
static void newEndpointDiscovered(EmberAfPluginDeviceTableEntry *p_entry);

// Bookkeeping callbacks
void emAfPluginDeviceTableDeviceLeftCallback(EmberEUI64 newNodeEui64);

// --------------------------------
// Devices discovery section of code
static void kickOffDeviceDiscover(uint16_t index)
{
  EmberAfPluginDeviceTableEntry *deviceTable = emberAfDeviceTablePointer();

  // need to fix this and put the device state into the queue.
  assert(newDeviceEventState == DEVICE_DISCOVERY_STATE_IDLE);

  // Make sure the index is valid
  assert(deviceTable[index].nodeId != EMBER_AF_PLUGIN_DEVICE_TABLE_NULL_NODE_ID
         && index < EMBER_AF_PLUGIN_DEVICE_TABLE_DEVICE_TABLE_SIZE);

  emberEventControlSetActive(newDeviceEventControl);

  currentDevice = index;
  deviceTable[index].retries = 0;
  currentEndpoint = 0;
}

static void haveNewDevice(uint16_t index)
{
  if (newDeviceEventState == DEVICE_DISCOVERY_STATE_IDLE) {
    emberAfCorePrintln("successful kickoff %x", index);
    kickOffDeviceDiscover(index);
  } else {
    emberAfCorePrintln("state not idle %x", index);
  }
}

static uint16_t findUndiscoveredDevice(void)
{
  uint16_t i;
  EmberAfPluginDeviceTableEntry *deviceTable = emberAfDeviceTablePointer();

  for (i = 0; i < EMBER_AF_PLUGIN_DEVICE_TABLE_DEVICE_TABLE_SIZE; i++) {
    if (deviceTable[i].state <EMBER_AF_PLUGIN_DEVICE_TABLE_STATE_JOINED
        && deviceTable[i].nodeId != EMBER_AF_PLUGIN_DEVICE_TABLE_NULL_NODE_ID) {
      emberAfCorePrintln("Undiscovered kickoff %2x %x",
                         deviceTable[i].nodeId, deviceTable[i].state);
      return i;
    }
  }

  return EMBER_AF_PLUGIN_DEVICE_TABLE_NULL_INDEX;
}

static void checkDeviceQueue(void)
{
  uint16_t index = findUndiscoveredDevice();
  EmberAfPluginDeviceTableEntry *deviceTable = emberAfDeviceTablePointer();

  if (index == EMBER_AF_PLUGIN_DEVICE_TABLE_NULL_INDEX) {
    // All devices are discovered
    emberAfCorePrintln("QUEUE:  All Joined Devices Discovered");
    currentDevice = EMBER_AF_PLUGIN_DEVICE_TABLE_NULL_INDEX;
  } else {
    resetRetry(emberAfDeviceTableGetNodeIdFromIndex(index));
    // We have to start over from here, as the newDeviceEventState has been
    // set to DEVICE_DISCOVERY_STATE_IDLE
    deviceTable[index].state = EMBER_AF_PLUGIN_DEVICE_TABLE_STATE_JUST_JOINED;
    kickOffDeviceDiscover(index);
  }
}


static void resetRetry(EmberNodeId nodeId)
{
  currentRetryCount = 0;
  currentRetryNodeId = nodeId;
}

static void advanceDeviceDiscoveryRetry(EmberNodeId nodeId)
{
  if (nodeId != currentRetryNodeId) {
    resetRetry(nodeId);
    return;
  }
  currentRetryCount++;

  if (currentRetryCount > NEW_DEVICE_RETRY_ATTEMPTS) {
    // Reset retry
    resetRetry(EMBER_AF_PLUGIN_DEVICE_TABLE_NULL_NODE_ID);

    // Idle the device state and check for new devices.
    emberEventControlSetInactive(newDeviceEventControl);
    newDeviceEventState = DEVICE_DISCOVERY_STATE_IDLE;
    checkDeviceQueue();
  }
}

static void setCurrentSourceRoute(uint16_t nodeId)
{
  uint16_t relayList[ZA_MAX_HOPS];
  uint8_t relayCount;

  // Note:  We will send a discovery if we fail to get an APS ACK.  So we do
  // not need to handle source route failure here.  In fact, source route
  // failure here doesn't mean there will be a transmit failure.  For example,
  // the device could be a child of the gateway.
  if (emberFindSourceRoute(nodeId, &relayCount, relayList)) {
    if (relayCount < MAX_RELAY_COUNT) {
      ezspSetSourceRoute(nodeId, relayCount, relayList);
    }
  }
}

void emberAfPluginDeviceTableNewDeviceEventHandler(void)
{
  EmberAfPluginDeviceTableEntry *deviceTable = emberAfDeviceTablePointer();
  EmberStatus status;
  uint16_t newEndpoint;

  emberAfCorePrintln("NEW DEVICE STATE:  %d %x %2x %x",
                     currentDevice,
                     newDeviceEventState,
                     deviceTable[currentDevice].nodeId,
                     deviceTable[currentDevice].state);

  emberEventControlSetInactive(newDeviceEventControl);

  if (currentDevice >= EMBER_AF_PLUGIN_DEVICE_TABLE_DEVICE_TABLE_SIZE) {
    return;
  }

  if (deviceTable[currentDevice].nodeId == EMBER_AF_PLUGIN_DEVICE_TABLE_NULL_NODE_ID) {
    newDeviceEventState = DEVICE_DISCOVERY_STATE_IDLE;
    checkDeviceQueue();
    return;
  }

  if (deviceTable[currentDevice].state == EMBER_AF_PLUGIN_DEVICE_TABLE_STATE_JOINED) {
    emberEventControlSetInactive(newDeviceEventControl);
    newDeviceEventState = DEVICE_DISCOVERY_STATE_IDLE;
    checkDeviceQueue();
    return;
  }

  advanceDeviceDiscoveryRetry(deviceTable[currentDevice].nodeId);

  switch (newDeviceEventState) {
  case DEVICE_DISCOVERY_STATE_IDLE:
    emberEventControlSetActive(newDeviceEventControl);
    newDeviceEventState = DEVICE_DISCOVERY_STATE_ENDPOINTS_RETRY;
    break;
  case DEVICE_DISCOVERY_STATE_ENDPOINTS_RETRY:
    // send out active endpoints request.
    setCurrentSourceRoute(deviceTable[currentDevice].nodeId);

    status = emberActiveEndpointsRequest(deviceTable[currentDevice].nodeId,
                                         APS_OPTION_DISCOVER);
    newDeviceEventState = DEVICE_DISCOVERY_STATE_ENDPOINTS_WAITING;

    if (status == EMBER_SUCCESS) {
      emberEventControlSetDelayQS(newDeviceEventControl, COMMAND_DELAY_QS);
    } else {
      emberEventControlSetActive(newDeviceEventControl);
    }

    break;

  case DEVICE_DISCOVERY_STATE_ENDPOINTS_WAITING:
    // kick off discovery
    newDeviceEventState = DEVICE_DISCOVERY_STATE_ENDPOINTS_RETRY;
    emberEventControlSetDelayQS(newDeviceEventControl, COMMAND_DELAY_QS);
    emberAfCorePrintln("ROUTE REPAIR:  discover 1");
    emberAfPluginDeviceTableInitiateRouteRepair(deviceTable[currentDevice].nodeId);
    break;

  case DEVICE_DISCOVERY_STATE_ENDPOINTS_RECEIVED:
    // We just received the endpoints response.  We need to check if what we
    // received was successful.  This is the same code as timing out waiting
    // for the message.  Fall through to the retry state.
  case DEVICE_DISCOVERY_STATE_SIMPLE_RETRY:
    // Check to see if the endpoints request was successful.  If not, then
    // send out the message again.
    if (deviceTable[currentDevice].state
        == EMBER_AF_PLUGIN_DEVICE_TABLE_STATE_HAVE_ACTIVE) {
      // Send endpoint descriptor request
      emberSimpleDescriptorRequest(deviceTable[currentDevice].nodeId,
                                   deviceTable[currentDevice].endpoint,
                                   EMBER_AF_DEFAULT_APS_OPTIONS);
      newDeviceEventState = DEVICE_DISCOVERY_STATE_SIMPLE_WAITING;
      emberEventControlSetDelayQS(newDeviceEventControl, COMMAND_DELAY_QS);
    } else {
      setCurrentSourceRoute(deviceTable[currentDevice].nodeId);

      status = emberActiveEndpointsRequest(deviceTable[currentDevice].nodeId,
                                           APS_OPTION_DISCOVER);

      if (status == EMBER_SUCCESS) {
        emberEventControlSetDelayQS(newDeviceEventControl, COMMAND_DELAY_QS);
      } else {
        emberEventControlSetActive(newDeviceEventControl);
      }
    }

    break;

  case DEVICE_DISCOVERY_STATE_SIMPLE_WAITING:
    // Kick off discovery
    newDeviceEventState = DEVICE_DISCOVERY_STATE_SIMPLE_RETRY;
    emberEventControlSetDelayQS(newDeviceEventControl, COMMAND_DELAY_QS);
    emberAfCorePrintln("ROUTE REPAIR:  discover 2");
    emberAfPluginDeviceTableInitiateRouteRepair(deviceTable[currentDevice].nodeId);
    break;

  case DEVICE_DISCOVERY_STATE_SIMPLE_RECEIVED:
    // Check to see if we received a simple descriptor.  TBD:  need to add
    // support for multiple endpoints.
    if (deviceTable[currentDevice].state
        >= EMBER_AF_PLUGIN_DEVICE_TABLE_STATE_HAVE_EP_DESC) {
      // We are done
      emberEventControlSetInactive(newDeviceEventControl);
      deviceTable[currentDevice].state =
        EMBER_AF_PLUGIN_DEVICE_TABLE_STATE_JOINED;
      newDeviceEventState = DEVICE_DISCOVERY_STATE_IDLE;

      // New device is set, time to make the callback to indicate a new device
      // has joined.
      emberAfPluginDeviceTableNewDeviceCallback(deviceTable[currentDevice].eui64);

      // Check to see if there is another endpoint on this device.
      newEndpoint = emAfDeviceTableFindNextEndpoint(currentDevice);
      if (newEndpoint != EMBER_AF_PLUGIN_DEVICE_TABLE_NULL_INDEX) {
        currentDevice = newEndpoint;
        emberEventControlSetActive(newDeviceEventControl);
      }

      // Save on Node Joined
      emAfDeviceTableSave();
      checkDeviceQueue();
    } else {
      // Re-send endpoint descriptor messages;
      setCurrentSourceRoute(deviceTable[currentDevice].nodeId);
      status = emberActiveEndpointsRequest(deviceTable[currentDevice].nodeId,
                                           APS_OPTION_DISCOVER);

      emberEventControlSetDelayQS(newDeviceEventControl, COMMAND_DELAY_QS);
    }

  default:
    break;
  }

}

static void newDeviceParseEndpointDescriptorResponse(EmberNodeId nodeId,
                                                     uint8_t* message,
                                                     uint16_t length)
{
  uint8_t endpoint = message[ENDPOINT_DESCRIPTOR_ENDPOINT_LOCATION];
  uint16_t index = emAfDeviceTableFindIndexNodeIdEndpoint(nodeId, endpoint);
  EmberAfPluginDeviceTableEntry *pEntry;

  if (index == EMBER_AF_PLUGIN_DEVICE_TABLE_NULL_INDEX) {
    emberAfCorePrintln("Error:  unexpected endpoint descriptor");
    return;
  }

  pEntry = emberAfDeviceTableFindDeviceTableEntry(index);

  pEntry->deviceId =
    emberFetchLowHighInt16u(message+ENDPOINT_DESCRIPTOR_DEVICE_ID_LOCATION);

  emberAfCorePrintln("simple descriptor node ID: 0x%2x deviceId 0x%2x",
                     nodeId,
                     pEntry->deviceId);

  if (pEntry->state < EMBER_AF_PLUGIN_DEVICE_TABLE_STATE_HAVE_EP_DESC) {
    pEntry->state = EMBER_AF_PLUGIN_DEVICE_TABLE_STATE_HAVE_EP_DESC;
    newDeviceEventState = DEVICE_DISCOVERY_STATE_SIMPLE_RECEIVED;
    newEndpointDiscovered(pEntry);
    emberEventControlSetActive(newDeviceEventControl);
  }
}

static void newDeviceParseActiveEndpointsResponse(EmberNodeId emberNodeId,
                                                  EmberApsFrame* apsFrame,
                                                  uint8_t* message,
                                                  uint16_t length)
{
  uint16_t index = emberAfDeviceTableGetIndexFromNodeId(emberNodeId);
  EmberAfPluginDeviceTableEntry *deviceTable = emberAfDeviceTablePointer();
  uint16_t i;

  if (index == EMBER_AF_PLUGIN_DEVICE_TABLE_NULL_INDEX) {
    emberAfCorePrintln("Error:  No Valid Address Table Entry");
    return;
  }

  // Make sure I have not used the redundant endpoint response
  if (deviceTable[index].state < EMBER_AF_PLUGIN_DEVICE_TABLE_STATE_HAVE_ACTIVE) {
    deviceTable[index].state = EMBER_AF_PLUGIN_DEVICE_TABLE_STATE_HAVE_ACTIVE;
    newDeviceEventState = DEVICE_DISCOVERY_STATE_ENDPOINTS_RECEIVED;
    emberEventControlSetActive(newDeviceEventControl);

    deviceTable[index].endpoint = message[5];

    for (i = 1; i < message[4]; i++) {
      if (emAfDeviceTableAddNewEndpoint(index, message[5+i])
          != EMBER_ZCL_STATUS_SUCCESS) {
        emberAfCorePrintln("Error:  device table full");
      }
    }
  }

  return;
}

void emberAfDeviceTableNewDeviceJoinHandler(EmberNodeId newNodeId,
                                            EmberEUI64 newNodeEui64)
{
  uint16_t index = emberAfDeviceTableGetFirstIndexFromEui64(newNodeEui64);
  EmberAfPluginDeviceTableEntry *deviceTable = emberAfDeviceTablePointer();

  emberAfPrintBigEndianEui64(newNodeEui64);
  emberAfCorePrint("NewDeviceJoinHandler: ");
  emberAfPrintBigEndianEui64(newNodeEui64);
  emberAfCorePrintln(" %2x", newNodeId);

  if (index == EMBER_AF_PLUGIN_DEVICE_TABLE_NULL_INDEX) {
    index = emAfDeviceTableFindFreeDeviceTableIndex();
    if (index == EMBER_AF_PLUGIN_DEVICE_TABLE_NULL_INDEX) {
      // Error case... no more room in the index table
      emberAfCorePrintln("Error:  Address Table Full");
      return;
    }

    deviceTable[index].nodeId = newNodeId;
    MEMCOPY(deviceTable[index].eui64, newNodeEui64, EUI64_SIZE);

    deviceTable[index].state = EMBER_AF_PLUGIN_DEVICE_TABLE_STATE_JUST_JOINED;
    deviceTable[index].endpoint = DEVICE_TABLE_UNKNOWN_ENDPOINT;

    haveNewDevice(index);
  } else {
    // Is this a new node ID?
    if (newNodeId != deviceTable[index].nodeId) {
      emberAfCorePrintln("Node ID Change:  was %2x, is %2x\r\n",
                         deviceTable[index].nodeId,
                         newNodeId);

      emAfDeviceTableUpdateNodeId(deviceTable[index].nodeId, newNodeId);

      // Test code for failure to see leave request.
      uint16_t endpointIndex =
                 emberAfDeviceTableGetEndpointFromNodeIdAndEndpoint(
                   deviceTable[index].nodeId,
                   deviceTable[index].endpoint);

      if (endpointIndex == 0xffff) {
        return;
      }

      // New device is set, time to make the callback to indicate a new device
      // has joined.
      emberAfPluginDeviceTableRejoinDeviceCallback(deviceTable[index].eui64);
      // Need to save when the node ID changes.
      emAfDeviceTableSave();
    }
  }
}

static void newDeviceLeftHandler(EmberEUI64 newNodeEui64)
{
  uint16_t index = emberAfDeviceTableGetFirstIndexFromEui64(newNodeEui64);

  if (index != EMBER_AF_PLUGIN_DEVICE_TABLE_NULL_INDEX) {
    emAfPluginDeviceTableDeviceLeftCallback(newNodeEui64);
    emAfPluginDeviceTableDeleteEntry(index);
    // Save on Node Left
    emAfDeviceTableSave();
  }
}

static EmberStatus broadcastPermitJoin(uint8_t duration)
{
  permitJoinBroadcastCounter++;
  EmberStatus status;

  if (permitJoinBroadcastCounter == PJOIN_BROADCAST_PERIOD) {
    uint8_t data[3] = { 0,   // sequence number (filled in later)
                        0,   // duration (filled in below)
                        0 }; // TC significance (not used)
    permitJoinBroadcastCounter = 0;

    data[1] = duration;
    status = emberSendZigDevRequest(EMBER_BROADCAST_ADDRESS,
                                    PERMIT_JOINING_REQUEST,
                                    0,   // APS options
                                    data,
                                    3);  // length
  } else {
    status = 0;
  }

  return status;
}

/** @brief Trust Center Join
 *
 * This callback is called from within the application framework's
 * implementation of emberTrustCenterJoinHandler or
 * ezspTrustCenterJoinHandler. This callback provides the same arguments
 * passed to the TrustCenterJoinHandler. For more information about the
 * TrustCenterJoinHandler please see documentation included in
 * stack/include/trust-center.h.
 *
 * @param newNodeId   Ver.: always
 * @param newNodeEui64   Ver.: always
 * @param parentOfNewNode   Ver.: always
 * @param status   Ver.: always
 * @param decision   Ver.: always
 */
void emberAfTrustCenterJoinCallback(EmberNodeId newNodeId,
                                    EmberEUI64 newNodeEui64,
                                    EmberNodeId parentOfNewNode,
                                    EmberDeviceUpdate status,
                                    EmberJoinDecision decision)
{
  uint8_t i;

  emberAfCorePrint("\r\nTC Join Callback %2x ",
                   newNodeId);
  for (i = 0; i < 8; i++) {
    emberAfCorePrint("%x",
                     newNodeEui64[7-i]);
  }
  if (status < STATUS_STRINGS_ARRAY_LENGTH) {
    emberAfCorePrintln(" %s", statusStrings[status]);
  } else {
    emberAfCorePrintln(" %d", status);
  }

  switch (status) {
  case EMBER_STANDARD_SECURITY_UNSECURED_JOIN:
    // Broadcast permit joining to new router as it joins.
    broadcastPermitJoin(254);
    emberAfCorePrintln("new device line %d", __LINE__);
    emberAfDeviceTableNewDeviceJoinHandler(newNodeId, newNodeEui64);
    break;
  case EMBER_DEVICE_LEFT:
    newDeviceLeftHandler(newNodeEui64);
    break;
  default:
    // If the device is in the left sent state, we want to send another
    // left message.
    if (emAfDeviceTableShouldDeviceLeave(newNodeId)) {
      return;
    } else {
      emberAfCorePrintln("new device line %d", __LINE__);
      emberAfDeviceTableNewDeviceJoinHandler(newNodeId, newNodeEui64);
    }
    break;
  }

  // If a new device did an unsecure join, we need to turn on permit joining, as
  // there may be more coming
  if (status == EMBER_STANDARD_SECURITY_UNSECURED_JOIN) {
    // Broadcast permit joining to new router as it joins.
    broadcastPermitJoin(254);
  }

}

/** @brief Pre ZDO Message Received
 *
 * This function passes the application an incoming ZDO message and gives the
 * appictation the opportunity to handle it. By default, this callback returns
 * false indicating that the incoming ZDO message has not been handled and
 * should be handled by the Application Framework.
 *
 * @param emberNodeId   Ver.: always
 * @param apsFrame   Ver.: always
 * @param message   Ver.: always
 * @param length   Ver.: always
 */
bool emAfPluginDeviceTablePreZDOMessageReceived(EmberNodeId emberNodeId,
                                                EmberApsFrame* apsFrame,
                                                uint8_t* message,
                                                uint16_t length)
{
  EmberNodeId ieeeSourceNode;

  emberAfCorePrintln("%2x:  ", emberNodeId);
  switch (apsFrame->clusterId)
  {
  case ACTIVE_ENDPOINTS_RESPONSE:
    emberAfCorePrintln("Active Endpoints Response");
    newDeviceParseActiveEndpointsResponse(emberNodeId,apsFrame,message,length);
    return false;
    break;
  case SIMPLE_DESCRIPTOR_RESPONSE:
    emberAfCorePrintln("Simple Descriptor Response");

    newDeviceParseEndpointDescriptorResponse(emberNodeId,
                                             message,
                                             length);
    return false;
    break;
  case END_DEVICE_ANNOUNCE:
    // Any time an end device announces, we need to see if we have to update
    // the device handler.
    emberAfCorePrintln("new device line %d", __LINE__);
    emberAfDeviceTableNewDeviceJoinHandler(emberNodeId,
                                           message+END_DEVICE_ANNOUNCE_IEEE_OFFSET);
    break;
  case PERMIT_JOINING_RESPONSE:
    break;
  case LEAVE_RESPONSE:
    break;
  case BIND_RESPONSE:
    break;
  case BINDING_TABLE_RESPONSE:
    break;
  case NETWORK_ADDRESS_RESPONSE:
    emberAfCorePrintln("new device line %d", __LINE__);
    emberAfDeviceTableNewDeviceJoinHandler(emberNodeId,
                                           message+NETWORK_ADDRESS_RESPONSE_IEEE_OFFSET);
    break;
  case IEEE_ADDRESS_RESPONSE:
    emberAfCorePrintln("new device line %d", __LINE__);
    ieeeSourceNode =
      emberFetchLowHighInt16u(message+IEEE_ADDRESS_RESPONSE_NODE_ID_OFFSET);
    emberAfCorePrintln("Ieee source node %2x", ieeeSourceNode);

    emberAfDeviceTableNewDeviceJoinHandler(ieeeSourceNode,
                                           message+IEEE_ADDRESS_RESPONSE_IEEE_OFFSET);
    break;
  default:
    emberAfCorePrintln("Untracked ZDO %2x", apsFrame->clusterId);
    break;
  }

  emberAfCorePrint("%2x ", emberNodeId);

  emAfDeviceTablePrintBuffer(message, length);

  return false;
}

// We have a new endpoint.  Figure out if we need to do anything, like write
// the CIE address to it.
static void newEndpointDiscovered(EmberAfPluginDeviceTableEntry *p_entry)
{
  if (p_entry->deviceId == DEVICE_ID_IAS_ZONE) {
      // write IEEE address to CIE address location
      emAfDeviceTableSendCieAddressWrite(p_entry->nodeId, p_entry->endpoint);
  }
}
