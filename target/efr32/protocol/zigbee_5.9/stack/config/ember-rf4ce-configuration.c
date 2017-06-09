/** @file ember-rf4ce-configuration.c
 * @brief User-configurable stack memory allocation.
 *
 *
 * \b Note: Application developers should \b not modify any portion
 * of this file. Doing so may lead to mysterious bugs. Allocations should be
 * adjusted only with macros in a custom CONFIGURATION_HEADER.
 *
 * <!--Copyright 2014 by Silicon Laboratories. All rights reserved.      *80*-->
 */
#include PLATFORM_HEADER
#include "stack/include/ember.h"
#include "stack/include/error.h"
#include "stack/include/ember-static-struct.h" // Required typedefs
#include "hal/micro/micro.h"

// *****************************************
// Memory Allocations & declarations
// *****************************************

extern uint8_t emAvailableMemory[];
#define align(value) (value)

//------------------------------------------------------------------------------
// API Version

const uint8_t emApiVersion
  = (EMBER_API_MAJOR_VERSION << 4) + EMBER_API_MINOR_VERSION;

//------------------------------------------------------------------------------
// Packet Buffers

uint8_t emPacketBufferCount = EMBER_PACKET_BUFFER_COUNT;
uint8_t emPacketBufferFreeCount = EMBER_PACKET_BUFFER_COUNT;

// The actual memory for buffers.
uint8_t *emPacketBufferData = &emAvailableMemory[0];
#define END_emPacketBufferData          \
  (align(EMBER_PACKET_BUFFER_COUNT * 32))

uint8_t *emMessageBufferLengths = &emAvailableMemory[END_emPacketBufferData];
#define END_emMessageBufferLengths      \
  (END_emPacketBufferData + align(EMBER_PACKET_BUFFER_COUNT))

uint8_t *emMessageBufferReferenceCounts = &emAvailableMemory[END_emMessageBufferLengths];
#define END_emMessageBufferReferenceCounts      \
  (END_emMessageBufferLengths + align(EMBER_PACKET_BUFFER_COUNT))

uint8_t *emPacketBufferLinks = &emAvailableMemory[END_emMessageBufferReferenceCounts];
#define END_emPacketBufferLinks      \
  (END_emMessageBufferReferenceCounts + align(EMBER_PACKET_BUFFER_COUNT))

uint8_t *emPacketBufferQueueLinks = &emAvailableMemory[END_emPacketBufferLinks];
#define END_emPacketBufferQueueLinks      \
  (END_emPacketBufferLinks + align(EMBER_PACKET_BUFFER_COUNT))

//------------------------------------------------------------------------------
// NWK Retry Queue

EmRetryQueueEntry *emRetryQueue = (EmRetryQueueEntry *) &emAvailableMemory[END_emPacketBufferQueueLinks];
uint8_t emRetryQueueSize = EMBER_RETRY_QUEUE_SIZE;
#define END_emRetryQueue  \
 (END_emPacketBufferQueueLinks + align(EMBER_RETRY_QUEUE_SIZE * sizeof(EmRetryQueueEntry)))

//------------------------------------------------------------------------------
// RF4CE stack tables

EmberRf4cePairingTableEntry *emRf4cePairingTable = (EmberRf4cePairingTableEntry *) &emAvailableMemory[END_emRetryQueue];
uint8_t emRf4cePairingTableSize = EMBER_RF4CE_PAIRING_TABLE_SIZE;
#define END_emRf4cePairingTable         \
  (END_emRetryQueue + align(EMBER_RF4CE_PAIRING_TABLE_SIZE * sizeof(EmberRf4cePairingTableEntry)))

EmRf4ceOutgoingPacketInfoEntry *emRf4cePendingOutgoingPacketTable = (EmRf4ceOutgoingPacketInfoEntry *) &emAvailableMemory[END_emRf4cePairingTable];
uint8_t emRf4cePendingOutgoingPacketTableSize = EMBER_RF4CE_PENDING_OUTGOING_PACKET_TABLE_SIZE;
#define END_emRf4cePendingOutgoingPacketTable     \
  (END_emRf4cePairingTable + align(EMBER_RF4CE_PENDING_OUTGOING_PACKET_TABLE_SIZE * sizeof(EmRf4ceOutgoingPacketInfoEntry)))

//------------------------------------------------------------------------------
// Memory Allocation

#define END_stackMemory  END_emRf4cePendingOutgoingPacketTable

#if defined (CORTEXM3)
  VAR_AT_SEGMENT(uint8_t emAvailableMemory[END_stackMemory], __EMHEAP__);
#elif defined(EMBER_TEST)
  uint8_t emAvailableMemory[END_stackMemory];
  const uint16_t emAvailableMemorySize = END_stackMemory;
#else
  #error "Unknown platform."
#endif

// *****************************************
// Non-dynamically configurable structures
// *****************************************
PGM uint8_t emTaskCount = EMBER_TASK_COUNT;
EmberTaskControl emTasks[EMBER_TASK_COUNT];

// *****************************************
// Stack Generic Parameters
// *****************************************

PGM uint8_t emberStackProfileId[8] = { 0, };

uint8_t emDefaultStackProfile = EMBER_STACK_PROFILE;
uint8_t emDefaultSecurityLevel = EMBER_SECURITY_LEVEL;
uint8_t emSupportedNetworks = 1;
