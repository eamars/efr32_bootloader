//
// test-harness-z3-mac.c
//
// August 3, 2015
// Refactored November 23, 2015
//
// ZigBee 3.0 mac layer test harness functionality
//

#include "app/framework/include/af.h"

#include "test-harness-z3-core.h"

// -----------------------------------------------------------------------------
// Beacon CLI Commands

// plugin test-harness z3 beacon beacon-req
void emAfPluginTestHarnessZ3BeaconBeaconReqCommand(void)
{
  EmberNetworkParameters networkParameters;
  EmberStatus status;
#ifdef EZSP_HOST
  EmberNodeType nodeType;
  status = ezspGetNetworkParameters(&nodeType, &networkParameters);
#else
  status = emberGetNetworkParameters(&networkParameters);
#endif

  if (status == EMBER_SUCCESS) {
    status = emberStartScan(EMBER_ACTIVE_SCAN,
                            BIT32(networkParameters.radioChannel),
                            2); // scan duration, whatever
  } else {
    // We probably are not on a network, so try to use the network-steering
    // channels.
    extern uint32_t emAfPluginNetworkSteeringPrimaryChannelMask;
    status = emberStartScan(EMBER_ACTIVE_SCAN,
                            emAfPluginNetworkSteeringPrimaryChannelMask,
                            2); // scan duration, whatever
  }

  emberAfCorePrintln("%p: %p: 0x%X",
                     TEST_HARNESS_Z3_PRINT_NAME,
                     "Beacon request",
                     status);
}

// plugin test-harness z3 beacon beacons-config <options:4>
void emAfPluginTestHarnessZ3BeaconBeaconsConfigCommand(void)
{
  EmberStatus status = EMBER_INVALID_CALL;

#ifndef EZSP_HOST

  uint32_t options = emAfPluginTestHarnessZ3GetSignificantBit(0);
  uint8_t packet[32];
  uint8_t *finger = packet;
  EmberMessageBuffer message;
  EmberNodeType nodeType;
  EmberNetworkParameters networkParameters;

  // Currently unused.
  (void)options;

  status = emberAfGetNetworkParameters(&nodeType, &networkParameters);
  if (status != EMBER_SUCCESS) {
    goto done;
  }
  
  // 802.15.4 header
  // frame control
  // beacon, no security, no frame pending
  // no ack required, no intra pan, no dst addr mode, short src dst addr mode
  *finger++ = 0x00;
  *finger++ = 0x80;
  // sequence (filled in by stack API)
  *finger++ = 0x00;
  // source pan id
  *finger++ = LOW_BYTE(emberAfGetPanId());
  *finger++ = HIGH_BYTE(emberAfGetPanId());
  // source address
  *finger++ = LOW_BYTE(emberAfGetNodeId());
  *finger++ = HIGH_BYTE(emberAfGetNodeId());

  // 802.15.4 beacon
  // superframe specification
  // beacon order n/a/, superframe order n/a/
  // final CAP slot n/a, no battery ext, yes pan coord, yes permit join
  *finger++ = 0xFF;
  *finger++ = 0xCF;
  // guaranteed time slots n/a/
  *finger++ = 0x00;
  // pending addresses n/a/
  *finger++ = 0x00;

  // zigbee beacon
  // protocol id zigbee pro
  *finger++ = 0x00;
  // zigbee pro version
  *finger++ = 0x22;
  // yes router capacity, no depth, yes end device capacity
  *finger++ = 0x84;
  // ext pan id
  MEMMOVE(finger, networkParameters.extendedPanId, EXTENDED_PAN_ID_SIZE);
  finger += EXTENDED_PAN_ID_SIZE;
  // tx offset
  *finger++ = 0xFF;
  *finger++ = 0xFF;
  *finger++ = 0xFF;
  // network update id
  *finger++ = networkParameters.nwkUpdateId;

  message = emberFillLinkedBuffers(packet, finger - &packet[0]);

  status = (message == EMBER_NULL_MESSAGE_BUFFER
            ? EMBER_NO_BUFFERS
            : emberSendRawMessage(message));

  emberReleaseMessageBuffer(message);

#else
  goto done; // get rid of warnings
#endif /* EZSP_HOST */

 done:
  emberAfCorePrintln("%p: %p: 0x%X",
                     TEST_HARNESS_Z3_PRINT_NAME,
                     "Beacon",
                     status);
}
