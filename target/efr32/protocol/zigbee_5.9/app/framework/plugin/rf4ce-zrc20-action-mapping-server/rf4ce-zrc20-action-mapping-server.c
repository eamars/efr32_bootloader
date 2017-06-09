/* Copyright 2014 Silicon Laboratories, Inc. */

#include "af.h"
#include "app/framework/plugin/rf4ce-profile/rf4ce-profile.h"
#include "app/framework/plugin/rf4ce-gdp/rf4ce-gdp.h"
#include "app/framework/plugin/rf4ce-gdp/rf4ce-gdp-internal.h"
#include "app/framework/plugin/rf4ce-zrc20/rf4ce-zrc20.h"
#include "app/framework/plugin/rf4ce-zrc20/rf4ce-zrc20-internal.h"
#include "rf4ce-zrc20-action-mapping-server.h"

#if defined(EMBER_SCRIPTED_TEST)
#include "../rf4ce-zrc20/rf4ce-zrc20-test.h"
#include "core/scripted-stub.h"
#endif

// Workaround for MCUDT-9965.
#ifndef UNUSED
  #define UNUSED
#endif

// The used heap space is simply how far the tail has drifted from the start of
// the heap.  The free space is everything from the tail to the end.
#define USED_HEAP_SPACE() (tail - actionMappingsServerHeap)
#define FREE_HEAP_SPACE() (sizeof(actionMappingsServerHeap) - USED_HEAP_SPACE())

// This structure maps between client side mappable actions/action mappings and
// server side mappable actions/action mappings.
typedef struct {
  uint8_t pairingIndex;
  // This is the index of the mappable action and action mapping tables on the
  // client.
  uint16_t clientEntryIndex;
  // This is the index of the mappable action and action mapping tables on the
  // server.
  uint16_t serverIndex;
} ActionRemapStruct;

// These tables are stored in RAM for simplicity.
static ActionRemapStruct actionRemapTable[EMBER_AF_PLUGIN_RF4CE_ZRC20_ACTION_MAPPING_SERVER_ACTION_REMAP_TABLE_SIZE];
static uint8_t actionRemapStatusTable[EMBER_AF_PLUGIN_RF4CE_ZRC20_ACTION_MAPPING_SERVER_ACTION_REMAP_TABLE_SIZE/8+1];
#ifdef EMBER_AF_PLUGIN_RF4CE_GDP_IS_POLL_SERVER
static uint8_t actionRemapNegotiateRequestTable[EMBER_RF4CE_PAIRING_TABLE_SIZE/8+1];
#endif // EMBER_AF_PLUGIN_RF4CE_GDP_IS_POLL_SERVER
static EmberAfRf4ceZrcMappableAction mappableActionsServerTable[EMBER_AF_PLUGIN_RF4CE_ZRC20_ACTION_MAPPING_SERVER_MAPPABLE_ACTIONS_TABLE_SIZE];
static EmberAfRf4ceZrcActionMapping actionMappingsServerTable[EMBER_AF_PLUGIN_RF4CE_ZRC20_ACTION_MAPPING_SERVER_MAPPABLE_ACTIONS_TABLE_SIZE];
static uint8_t actionMappingsServerHeap[EMBER_AF_PLUGIN_RF4CE_ZRC20_ACTION_MAPPING_SERVER_ACTION_MAPPINGS_HEAP_SIZE];
static uint8_t *tail = actionMappingsServerHeap;

static void removeOldEntry(uint16_t index);
static uint32_t calculateHeapUsage(const EmberAfRf4ceZrcActionMapping *entry);
static uint8_t *findHead(const EmberAfRf4ceZrcActionMapping *entry);
static uint8_t *copyActionData(bool hasDescriptor,
                             const EmberAfRf4ceZrcActionMapping *entry);
static uint8_t *copyIrCode(bool hasDescriptor,
                         const EmberAfRf4ceZrcActionMapping *entry);

static void setMappingDirty(uint16_t serverIndex);
static void clearMappingDirty(uint8_t pairingIndex, uint16_t clientIndex);
static void requestSelectiveAMUpdatePerPairing(uint8_t pairingIndex);
#ifdef EMBER_AF_PLUGIN_RF4CE_GDP_IS_POLL_SERVER
static void heartbeatCallback(uint8_t pairingIndex,
                              EmberAfRf4ceGdpHeartbeatTrigger trigger);
#endif // EMBER_AF_PLUGIN_RF4CE_GDP_IS_POLL_SERVER

//------------------------------------------------------------------------------
// ZRC 2.0 implemented callbacks

#if !defined(EMBER_SCRIPTED_TEST)

void emberAfPluginRf4ceZrc20ActionMappingServerInitCallback(void)
{
#ifdef EMBER_AF_PLUGIN_RF4CE_GDP_IS_POLL_SERVER
  assert(emberAfRf4ceGdpSubscribeToHeartbeat(heartbeatCallback)
         == EMBER_SUCCESS);
#endif // EMBER_AF_PLUGIN_RF4CE_GDP_IS_POLL_SERVER
  /* Restore all action mappings to default. */
  emberAfRf4ceZrc20ActionMappingServerRestoreDefaultAllActions();
}

void emberAfPluginRf4ceZrc20IncomingMappableActionCallback(uint8_t pairingIndex,
                                                           uint16_t entryIndex,
                                                           EmberAfRf4ceZrcMappableAction *mappableAction)
{
  uint16_t i = 0;

  if (mappableAction->actionDeviceType != EMBER_AF_RF4CE_DEVICE_TYPE_WILDCARD) {
    // Mappable action found.
    if (EMBER_SUCCESS == emAfRf4ceZrc20ActionMappingServerLookUpMappableAction(mappableAction,
                                                                               &i)) {
      // Add it or update it.
      if (EMBER_SUCCESS != emAfRf4ceZrc20ActionMappingServerUpdateOrAddMapping(pairingIndex,
                                                                               entryIndex,
                                                                               i)) {
        emberAfCorePrintln("Mapping table full! Couldn't add mapping.");
      }
    }
  } else {
    // Invalidate action mapping.
    // Found mapping.
    if (EMBER_SUCCESS == emAfRf4ceZrc20ActionMappingServerGetMapping(pairingIndex,
                                                                     entryIndex,
                                                                     &i)) {
      emAfRf4ceZrc20ActionMappingServerRemoveMapping(pairingIndex,
                                                     entryIndex);
    }
  }
}

EmberStatus emberAfPluginRf4ceZrc20GetMappableActionCallback(uint8_t pairingIndex,
                                                             uint16_t entryIndex,
                                                             EmberAfRf4ceZrcMappableAction *mappableAction)
{
  uint16_t serverIndex;
  EmberStatus status;

  // Look up mapping.
  if (EMBER_SUCCESS ==
        (status = emAfRf4ceZrc20ActionMappingServerGetMapping(pairingIndex,
                                                              entryIndex,
                                                              &serverIndex))) {
    status = emAfRf4ceZrc20ActionMappingServerGetMappableAction(serverIndex,
                                                                mappableAction);
  }

  return status;
}

uint16_t emberAfPluginRf4ceZrc20GetMappableActionCountCallback(uint8_t pairingIndex)
{
  // This plugin uses the same set of mappable actions for all pairings, so the
  // specific pairing index is ignored.
  return emberAfRf4ceZrc20ActionMappingServerGetMappableActionCount();
}

EmberStatus emberAfPluginRf4ceZrc20GetActionMappingCallback(uint8_t pairingIndex,
                                                            uint16_t entryIndex,
                                                            EmberAfRf4ceZrcActionMapping *actionMapping)
{
  uint16_t serverIndex;
  EmberAfRf4ceZrcActionMapping tmpActionMapping = {
    .mappingFlags = EMBER_AF_RF4CE_ZRC_ACTION_MAPPING_MAPPING_FLAGS_USE_DEFAULT_BIT,
    .rfConfig = 0x00,
    .rf4ceTxOptions = 0x00,
    .actionDataLength = 0x00,
    .actionData = NULL,
    .irConfig = 0x00,
    .irVendorId = 0x00,
    .irCodeLength = 0x00,
    .irCode = NULL
  };

  // When the client fetches action mapping from the server, the dirty flag has
  // to be cleared.
  clearMappingDirty(pairingIndex, entryIndex);

  // Didn't find entry.
  if (EMBER_SUCCESS != emAfRf4ceZrc20ActionMappingServerGetMapping(pairingIndex,
                                                                   entryIndex,
                                                                   &serverIndex)) {
    MEMCOPY(actionMapping,
            &tmpActionMapping,
            sizeof(EmberAfRf4ceZrcActionMapping));

    return EMBER_SUCCESS;
  }

  return emAfRf4ceZrc20ActionMappingServerGetActionMapping(serverIndex,
                                                           actionMapping);
}

#endif // !EMBER_SCRIPTED_TEST

//------------------------------------------------------------------------------



// Public API

EmberStatus emberAfRf4ceZrc20ActionMappingServerRemapAction(EmberAfRf4ceZrcMappableAction* mappableAction,
                                                            EmberAfRf4ceZrcActionMapping* actionMapping)
{
  uint16_t i;
  uint8_t j;
  EmberRf4cePairingTableEntry entry;
  EmberAfRf4ceZrcMappableAction tmpMappableAction;
  EmberStatus status;

  // Check if mappable action is already in the table. If it is, update its
  // action mapping.
  for (i=0; i<EMBER_AF_PLUGIN_RF4CE_ZRC20_ACTION_MAPPING_SERVER_MAPPABLE_ACTIONS_TABLE_SIZE; i++) {
    emAfRf4ceZrc20ActionMappingServerGetMappableAction(i, &tmpMappableAction);
    // Mappable action found.
    if (0x00 == MEMCOMPARE(mappableAction,
                           &tmpMappableAction,
                           sizeof(EmberAfRf4ceZrcMappableAction))) {
      // Set its action mapping.
      if (EMBER_SUCCESS !=
          (status = emAfRf4ceZrc20ActionMappingServerSetActionMapping(i,
                                                                      actionMapping))) {
        // Error.
        return status;
      }
      // Set dirty flags of remap table entries corresponding to the updated
      // action mapping
      setMappingDirty(i);
#ifdef EMBER_AF_PLUGIN_RF4CE_GDP_IS_POLL_SERVER
      uint8_t status = emAfRf4ceGdpGetPairingBindStatus(actionRemapTable[i].pairingIndex);
      // If we are a poll server and the client is also a poll client, we have to
      // wait for a heartbeat before we can send the notification command.
      // Otherwise, we can just send the message right away.
      bool pollClient = READBITS(status, PAIRING_ENTRY_POLLING_ACTIVE_BIT);
      if (pollClient) {
        return EMBER_SUCCESS;
      }
#endif // EMBER_AF_PLUGIN_RF4CE_GDP_IS_POLL_SERVER
      // Loop through pairing table.
      for (i=0; i<EMBER_RF4CE_PAIRING_TABLE_SIZE; i++) {
        // Exit on any error.
        if (EMBER_SUCCESS !=
            (status = emberAfRf4ceGetPairingTableEntry(i, &entry))) {
          return status;
        }
        // Send notification to active peer only
        if (emberAfRf4cePairingTableEntryIsActive(&entry)) {
          requestSelectiveAMUpdatePerPairing(i);
        }
      }
      return EMBER_SUCCESS;
    }
  }

  // Try to find an empty slot for the mappable action and action mapping.
  for (i=0; i<EMBER_AF_PLUGIN_RF4CE_ZRC20_ACTION_MAPPING_SERVER_MAPPABLE_ACTIONS_TABLE_SIZE; i++) {
    emAfRf4ceZrc20ActionMappingServerGetMappableAction(i, &tmpMappableAction);
    // Found empty slot.
    if (tmpMappableAction.actionDeviceType == EMBER_AF_RF4CE_DEVICE_TYPE_RESERVED) {
      // Set mappable action. Return on error.
      if (EMBER_SUCCESS !=
          (status = emAfRf4ceZrc20ActionMappingServerSetMappableAction(i,
                                                                       mappableAction))) {
        return status;
      }
      // Set action mapping. Return on error.
      if (EMBER_SUCCESS !=
          (status = emAfRf4ceZrc20ActionMappingServerSetActionMapping(i,
                                                                      actionMapping))) {
        return status;
      }
      // If we are GDP poll server and the client is GDP poll client we should
      // send out notification when the heartbeat comes in from the paired
      // controller. Otherwise we send out negotiation notification to all
      // paired controllers immediately.
      // Loop over the pairing table.
      for (j=0; j<EMBER_RF4CE_PAIRING_TABLE_SIZE; j++) {
        // Exit on any error.
        if (EMBER_SUCCESS !=
            (status = emberAfRf4ceGetPairingTableEntry(j, &entry))) {
          return status;
        }
        // Send notification to active peer only.
        if (emberAfRf4cePairingTableEntryIsActive(&entry)) {
#ifdef EMBER_AF_PLUGIN_RF4CE_GDP_IS_POLL_SERVER
          uint8_t st = emAfRf4ceGdpGetPairingBindStatus(j);
          // If we are a poll server and the client is also a poll client, we have to
          // wait for a heartbeat before we can send the notification command.
          // Otherwise, we can just send the message right away.
          bool pollClient = READBITS(st, PAIRING_ENTRY_POLLING_ACTIVE_BIT);
          // Client needs to be GDP poll client in order to send us heartbeats.
          if (pollClient) {
            SETBIT(actionRemapNegotiateRequestTable[j/8], j & 0x07);
            continue;
          }
#endif // EMBER_AF_PLUGIN_RF4CE_GDP_IS_POLL_SERVER
          // Request action mapping negotiation (full update).
          if (EMBER_SUCCESS !=
              (status = emberAfRf4ceGdpClientNotification(j,
                                                          EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0,
                                                          EMBER_RF4CE_NULL_VENDOR_ID,
                                                          CLIENT_NOTIFICATION_SUBTYPE_REQUEST_ACTION_MAPPING_NEGOTIATION,
                                                          NULL,
                                                          CLIENT_NOTIFICATION_REQUEST_ACTION_MAPPING_NEGOTIATION_PAYLOAD_LENGTH))) {
            return status;
          }
        }
      }
      return EMBER_SUCCESS;
    }
  }
  return EMBER_TABLE_FULL;
}

EmberStatus emberAfRf4ceZrc20ActionMappingServerRestoreDefaultAction(EmberAfRf4ceZrcMappableAction* mappableAction)
{
  uint16_t i;
  EmberAfRf4ceZrcMappableAction tmpMappableAction;

  for (i=0; i<EMBER_AF_PLUGIN_RF4CE_ZRC20_ACTION_MAPPING_SERVER_MAPPABLE_ACTIONS_TABLE_SIZE; i++) {
    emAfRf4ceZrc20ActionMappingServerGetMappableAction(i, &tmpMappableAction);
    // Found mappable action.
    if (0x00 == MEMCOMPARE(mappableAction,
                           &tmpMappableAction,
                           sizeof(EmberAfRf4ceZrcMappableAction))) {
      // Clear mappable action.
      emAfRf4ceZrc20ActionMappingServerClearMappableAction(i);
      // Clear action mapping.
      emAfRf4ceZrc20ActionMappingServerClearActionMapping(i);
      // Set dirty flags of remap table entries corresponding to the updated
      // action mapping
      setMappingDirty(i);
      // Remove entries from remap table that point to cleared action mapping.
      emAfRf4ceZrc20ActionMappingServerClearMapping(i);
      return EMBER_SUCCESS;
    }
  }

  // Did not find matching entry.
  return EMBER_NOT_FOUND;
}

void emberAfRf4ceZrc20ActionMappingServerRestoreDefaultAllActions(void)
{
  emAfRf4ceZrc20ActionMappingServerClearAllMappings();
  emAfRf4ceZrc20ActionMappingServerClearAllMappableActions();
  emAfRf4ceZrc20ActionMappingServerClearAllActionMappings();
}

uint16_t emberAfRf4ceZrc20ActionMappingServerGetMappableActionCount(void)
{
    return EMBER_AF_PLUGIN_RF4CE_ZRC20_ACTION_MAPPING_SERVER_MAPPABLE_ACTIONS_TABLE_SIZE;
}




// Internal API.

void emAfRf4ceZrc20ActionMappingServerClearMapping(uint16_t serverIndex)
{
  uint16_t i;

  for (i=0; i<EMBER_AF_PLUGIN_RF4CE_ZRC20_ACTION_MAPPING_SERVER_ACTION_REMAP_TABLE_SIZE; i++) {
    // Found mapping to selected index.
    if (actionRemapTable[i].serverIndex == serverIndex) {
      // Clear entry.
      actionRemapTable[i].serverIndex = 0xFFFF;
    }
  }
}

void emAfRf4ceZrc20ActionMappingServerClearAllMappings(void)
{
  MEMSET(actionRemapTable, 0xFF, sizeof(actionRemapTable));
  MEMSET(actionRemapStatusTable, 0, sizeof(actionRemapStatusTable));
#ifdef EMBER_AF_PLUGIN_RF4CE_GDP_IS_POLL_SERVER
  MEMSET(actionRemapNegotiateRequestTable, 0, sizeof(actionRemapNegotiateRequestTable));
#endif // EMBER_AF_PLUGIN_RF4CE_GDP_IS_POLL_SERVER
}

EmberStatus emAfRf4ceZrc20ActionMappingServerUpdateOrAddMapping(uint8_t pairingIndex,
                                                                uint16_t entryIndex,
                                                                uint16_t serverIndex)
{
  uint16_t i;

  // Update mapping if already in table.
  for (i=0; i<EMBER_AF_PLUGIN_RF4CE_ZRC20_ACTION_MAPPING_SERVER_ACTION_REMAP_TABLE_SIZE; i++) {
    if (actionRemapTable[i].pairingIndex == pairingIndex
        && actionRemapTable[i].clientEntryIndex == entryIndex) {
      actionRemapTable[i].serverIndex = serverIndex;
      return EMBER_SUCCESS;
    }
  }

  // Add mapping if not in table yet.
  for (i=0; i<EMBER_AF_PLUGIN_RF4CE_ZRC20_ACTION_MAPPING_SERVER_ACTION_REMAP_TABLE_SIZE; i++) {
    if (actionRemapTable[i].serverIndex == 0xFFFF) {
      actionRemapTable[i].pairingIndex = pairingIndex;
      actionRemapTable[i].clientEntryIndex = entryIndex;
      actionRemapTable[i].serverIndex = serverIndex;
      return EMBER_SUCCESS;
    }
  }

  // Table full.
  return EMBER_TABLE_FULL;
}

EmberStatus emAfRf4ceZrc20ActionMappingServerRemoveMapping(uint8_t pairingIndex,
                                                           uint16_t entryIndex)
{
  uint16_t i;

  for (i=0; i<EMBER_AF_PLUGIN_RF4CE_ZRC20_ACTION_MAPPING_SERVER_ACTION_REMAP_TABLE_SIZE; i++) {
    if (actionRemapTable[i].pairingIndex == pairingIndex
        && actionRemapTable[i].clientEntryIndex == entryIndex) {
      actionRemapTable[i].serverIndex = 0xFFFF;
      return EMBER_SUCCESS;
    }
  }

  // Did not find matching entry.
  return EMBER_NOT_FOUND;
}

EmberStatus emAfRf4ceZrc20ActionMappingServerGetMapping(uint8_t pairingIndex,
                                                        uint16_t entryIndex,
                                                        uint16_t *serverIndex)
{
  uint16_t i;

  for (i=0; i<EMBER_AF_PLUGIN_RF4CE_ZRC20_ACTION_MAPPING_SERVER_ACTION_REMAP_TABLE_SIZE; i++) {
    if (actionRemapTable[i].pairingIndex == pairingIndex
        && actionRemapTable[i].clientEntryIndex == entryIndex
        && actionRemapTable[i].serverIndex != 0xFFFF) {
      *serverIndex = actionRemapTable[i].serverIndex;
      return EMBER_SUCCESS;
    }
  }

  // Did not find matching entry.
  return EMBER_NOT_FOUND;
}

EmberStatus emAfRf4ceZrc20ActionMappingServerClearMappableAction(uint16_t index)
{
  // Index out of range.
  if (index >= EMBER_AF_PLUGIN_RF4CE_ZRC20_ACTION_MAPPING_SERVER_MAPPABLE_ACTIONS_TABLE_SIZE) {
    return EMBER_INDEX_OUT_OF_RANGE;
  }

  MEMSET(&mappableActionsServerTable[index],
         0x00,
         sizeof(EmberAfRf4ceZrcMappableAction));

  return EMBER_SUCCESS;
}

void emAfRf4ceZrc20ActionMappingServerClearAllMappableActions(void)
{
  MEMSET(mappableActionsServerTable,
         0x00,
         sizeof(mappableActionsServerTable));
}

EmberStatus emAfRf4ceZrc20ActionMappingServerSetMappableAction(uint16_t index,
                                                               EmberAfRf4ceZrcMappableAction *mappableAction)
{
  // Index out of range.
  if (index >= EMBER_AF_PLUGIN_RF4CE_ZRC20_ACTION_MAPPING_SERVER_MAPPABLE_ACTIONS_TABLE_SIZE) {
    return EMBER_INDEX_OUT_OF_RANGE;
  }

  MEMCOPY(&mappableActionsServerTable[index],
          mappableAction,
          sizeof(EmberAfRf4ceZrcMappableAction));

  return EMBER_SUCCESS;
}

EmberStatus emAfRf4ceZrc20ActionMappingServerGetMappableAction(uint16_t index,
                                                               EmberAfRf4ceZrcMappableAction *mappableAction)
{
  // Index out of range.
  if (index >= EMBER_AF_PLUGIN_RF4CE_ZRC20_ACTION_MAPPING_SERVER_MAPPABLE_ACTIONS_TABLE_SIZE) {
    return EMBER_INDEX_OUT_OF_RANGE;
  }

  MEMCOPY(mappableAction,
          &mappableActionsServerTable[index],
          sizeof(EmberAfRf4ceZrcMappableAction));

  return EMBER_SUCCESS;
}

EmberStatus emAfRf4ceZrc20ActionMappingServerLookUpMappableAction(EmberAfRf4ceZrcMappableAction *mappableAction,
                                                                  uint16_t* index)
{
  uint16_t i;

  for (i=0; i<EMBER_AF_PLUGIN_RF4CE_ZRC20_ACTION_MAPPING_SERVER_MAPPABLE_ACTIONS_TABLE_SIZE; i++) {
      if (0x00 == MEMCOMPARE(mappableAction,
                             &mappableActionsServerTable[i],
                             sizeof(EmberAfRf4ceZrcMappableAction))) {
      // Found entry.
      *index = i;
      return EMBER_SUCCESS;
    }
  }

  // Did not find matching entry.
  return EMBER_NOT_FOUND;
}

EmberStatus emAfRf4ceZrc20ActionMappingServerClearActionMapping(uint16_t index)
{
  // Entry index out of range.
  if (index >= EMBER_AF_PLUGIN_RF4CE_ZRC20_ACTION_MAPPING_SERVER_MAPPABLE_ACTIONS_TABLE_SIZE) {
    return EMBER_INDEX_OUT_OF_RANGE;
  }

  // remove the RF and IR descriptors of the action mapping entry if they exist.
  removeOldEntry(index);
  // The basic structure of the new entry is copied as is to the database.  The
  // variable-sized portion of each descriptor is copied into the heap and then
  // the pointer to that data from the database is adjusted to point into the
  // heap.
  MEMSET(&actionMappingsServerTable[index], 0, sizeof(EmberAfRf4ceZrcActionMapping));
  SET_DEFAULT(&actionMappingsServerTable[index]);

  return EMBER_SUCCESS;
}

void emAfRf4ceZrc20ActionMappingServerClearAllActionMappings(void)
{
  uint8_t i;
  for (i = 0; i < EMBER_AF_PLUGIN_RF4CE_ZRC20_ACTION_MAPPING_SERVER_MAPPABLE_ACTIONS_TABLE_SIZE; i++) {
    SET_DEFAULT(&actionMappingsServerTable[i]);
  }

  MEMSET(actionMappingsServerHeap, 0, sizeof(actionMappingsServerHeap));
  tail = actionMappingsServerHeap;
}

EmberStatus emAfRf4ceZrc20ActionMappingServerSetActionMapping(uint16_t index,
                                                              EmberAfRf4ceZrcActionMapping *actionMapping)
{
  // Index out of range.
  if (index >= EMBER_AF_PLUGIN_RF4CE_ZRC20_ACTION_MAPPING_SERVER_MAPPABLE_ACTIONS_TABLE_SIZE) {
    return EMBER_INDEX_OUT_OF_RANGE;
  }

  // we always remove the old entry, even though the new entry might not fit
  removeOldEntry(index);
  // The free space is the unused space and includes what we reclaimed from the
  // old entry.  If we don't have enough room, we drop the new entry and leave
  // a default in its place.
  if (FREE_HEAP_SPACE() < calculateHeapUsage(actionMapping)) {
    SET_DEFAULT(&actionMappingsServerTable[index]);
    return EMBER_TABLE_FULL;
  }

  // The basic structure of the new entry is copied as is to the database.  The
  // variable-sized portion of each descriptor is copied into the heap and then
  // the pointer to that data from the database is adjusted to point into the
  // heap.
  MEMCOPY(&actionMappingsServerTable[index], actionMapping, sizeof(EmberAfRf4ceZrcActionMapping));
  actionMappingsServerTable[index].actionData
    = copyActionData(emberAfRf4ceZrc20ActionMappingEntryHasRfDescriptor(actionMapping),
                    actionMapping);
  actionMappingsServerTable[index].irCode
    = copyIrCode(emberAfRf4ceZrc20ActionMappingEntryHasIrDescriptor(actionMapping),
                 actionMapping);

  return EMBER_SUCCESS;
}

EmberStatus emAfRf4ceZrc20ActionMappingServerGetActionMapping(uint16_t index,
                                                              EmberAfRf4ceZrcActionMapping *actionMapping)
{
  // Index out of range.
  if (index >= EMBER_AF_PLUGIN_RF4CE_ZRC20_ACTION_MAPPING_SERVER_MAPPABLE_ACTIONS_TABLE_SIZE) {
    return EMBER_INDEX_OUT_OF_RANGE;
  }

  MEMCOPY(actionMapping,
          &actionMappingsServerTable[index],
          sizeof(EmberAfRf4ceZrcActionMapping));

  return EMBER_SUCCESS;
}


static void removeOldEntry(uint16_t index)
{
  // When a new entry comes in, we always remove the old entry, even though the
  // new entry might not fit.  The rationale is that the old entry is being
  // replaced because it is no longer valid for the peripheral devices in the
  // system.  Reverting to a default entry seems better than keeping the old,
  // invalid entry.
  uint32_t reclaimableBytes
    = calculateHeapUsage(&actionMappingsServerTable[index]);
  if (reclaimableBytes != 0) {
    uint8_t *head;
    uint8_t i;

    // If the space we think we are using is ever less than what we think we
    // can reclaim, then something has gone very wrong.
    assert(reclaimableBytes <= USED_HEAP_SPACE());

    // The action data and the IR code are all stored contiguously in the heap,
    // although some of them may be empty.  We just need to find whichever one
    // is first.  If we expect to have one but can't find it or we do find it
    // but its not actually within the heap area, then something has gone very
    // wrong.
    head = findHead(&actionMappingsServerTable[index]);
    assert(head != NULL);
    assert(actionMappingsServerHeap <= head);
    assert(head + reclaimableBytes <= tail);

    // Rewind the tail pointer and all the action data and IR code pointers for
    // entries that follow the old one.  This makes them point to where they
    // should after the heap is adjusted.  This means the pointers are NOT
    // valid until the heap is adjusted.
    tail -= reclaimableBytes;
    for (i = 0; i < COUNTOF(actionMappingsServerTable); i++) {
      if (head < actionMappingsServerTable[i].actionData) {
        actionMappingsServerTable[i].actionData -= reclaimableBytes;
      }
      if (head < actionMappingsServerTable[i].irCode) {
          actionMappingsServerTable[i].irCode -= reclaimableBytes;
      }
    }

    // Move the stuff after the old entry so it immediately follows the stuff
    // preceding the old entry.  The old entry is now gone and the tail, action
    // data and IR code pointers are all valid again.
    MEMMOVE(head, head + reclaimableBytes, tail - head);

    // Wipe the stuff following the new tail.
    MEMSET(tail, 0, reclaimableBytes);
  }
}

static uint32_t calculateHeapUsage(const EmberAfRf4ceZrcActionMapping *entry)
{
  // The heap usage of an entry is the sum of the action data length and the
  // IR code length.  The heap usage is zero if the default is used.
  uint32_t bytes = 0;
  if (emberAfRf4ceZrc20ActionMappingEntryHasRfDescriptor(entry)) {
    bytes += entry->actionDataLength;
  }
  if (emberAfRf4ceZrc20ActionMappingEntryHasIrDescriptor(entry)) {
    bytes += entry->irCodeLength;
  }
  return bytes;
}

static uint8_t *findHead(const EmberAfRf4ceZrcActionMapping *entry)
{
  // The head of an entry in the heap is the location of the action data or IR
  // code.  The head is NULL if the default is used.
  if (emberAfRf4ceZrc20ActionMappingEntryHasRfDescriptor(entry)) {
    return (uint8_t *)entry->actionData;
  } else if (emberAfRf4ceZrc20ActionMappingEntryHasIrDescriptor(entry)) {
    return (uint8_t *)entry->irCode;
  } else {
    return NULL;
  }
}

static uint8_t *copyActionData(bool hasDescriptor,
                             const EmberAfRf4ceZrcActionMapping *entry)
{
  // If the RF is specified, action data is copied to the heap and the
  // tail is moved forward.  Otherwise, the payload pointer points nowhere.  If
  // we ever end up off the end of the heap, then something has gone very
  // wrong.
  uint8_t *payload = NULL;
  if (hasDescriptor) {
    payload = tail;
    MEMCOPY(tail, entry->actionData, entry->actionDataLength);
    tail += entry->actionDataLength;
    assert(tail <= actionMappingsServerHeap + sizeof(actionMappingsServerHeap));
  }
  return payload;
}

static uint8_t *copyIrCode(bool hasDescriptor,
                         const EmberAfRf4ceZrcActionMapping *entry)
{
  // If the IR descriptor exists, the code is copied to the heap and the tail
  // is moved forward.  Otherwise, the code pointer points nowhere.  If we ever
  // end up off the end of the heap, then something has gone very wrong.
  uint8_t *irCode = NULL;
  if (hasDescriptor) {
    irCode = tail;
    MEMCOPY(tail, entry->irCode, entry->irCodeLength);
    tail += entry->irCodeLength;
    assert(tail <= actionMappingsServerHeap + sizeof(actionMappingsServerHeap));
  }
  return irCode;
}

static void setMappingDirty(uint16_t serverIndex)
{
  uint16_t i;

  for (i=0; i<EMBER_AF_PLUGIN_RF4CE_ZRC20_ACTION_MAPPING_SERVER_ACTION_REMAP_TABLE_SIZE; i++) {
    if (actionRemapTable[i].serverIndex == serverIndex) {
        SETBIT(actionRemapStatusTable[i/8], (uint8_t)i & 0x07);
      }
  }
}

static void UNUSED clearMappingDirty(uint8_t pairingIndex, uint16_t clientIndex)
{
  uint16_t i;

  for (i=0; i<EMBER_AF_PLUGIN_RF4CE_ZRC20_ACTION_MAPPING_SERVER_ACTION_REMAP_TABLE_SIZE; i++) {
    if (actionRemapTable[i].pairingIndex == pairingIndex
        && actionRemapTable[i].clientEntryIndex == clientIndex) {
      CLEARBIT(actionRemapStatusTable[i/8], (uint8_t)i & 0x07);
      return;
    }
  }
}

static void requestSelectiveAMUpdatePerPairing(uint8_t pairingIndex)
{
  uint16_t i;
  // TODO: use defines!
  uint8_t payload[CLIENT_NOTIFICATION_REQUEST_SELECTIVE_AM_UPDATE_INDEX_LIST_LENGTH_LENGTH+256*2];
  uint8_t indexListLen = 0;
  bool actionMappingChanged = false;

  MEMSET(payload, 0, sizeof(payload));

  // Send selective action mapping update notification
  for (i=0; i<EMBER_AF_PLUGIN_RF4CE_ZRC20_ACTION_MAPPING_SERVER_ACTION_REMAP_TABLE_SIZE; i++) {
    if (actionRemapTable[i].pairingIndex == pairingIndex
        && READBIT(actionRemapStatusTable[i/8], (uint8_t)i & 0x07)) {
      *(uint16_t*)(&payload[indexListLen*2 + CLIENT_NOTIFICATION_REQUEST_SELECTIVE_AM_UPDATE_INDEX_LIST_OFFSET])
        = actionRemapTable[i].clientEntryIndex;
      indexListLen++;
      actionMappingChanged = true;
    }
    payload[CLIENT_NOTIFICATION_REQUEST_SELECTIVE_AM_UPDATE_INDEX_LIST_LENGTH_OFFSET] = indexListLen;

    if (actionMappingChanged) {
      actionMappingChanged = false;
      emberAfRf4ceGdpClientNotification(pairingIndex,
                                        EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0,
                                        EMBER_RF4CE_NULL_VENDOR_ID,
                                        CLIENT_NOTIFICATION_SUBTYPE_REQUEST_SELECTIVE_ACTION_MAPPING_UPDATE,
                                        payload,
                                        CLIENT_NOTIFICATION_REQUEST_SELECTIVE_AM_UPDATE_INDEX_LIST_OFFSET
                                          + indexListLen*2);
    }
  }
}

#ifdef EMBER_AF_PLUGIN_RF4CE_GDP_IS_POLL_SERVER
// For each Pairing Index we look if there exist at least an action mapping that
// changed since the client pulled it. If so, we send a notification
// 'RequestSelectiveActionMappingUpdate' command.
static void heartbeatCallback(uint8_t pairingIndex,
                              EmberAfRf4ceGdpHeartbeatTrigger trigger)
{
  uint8_t status = emAfRf4ceGdpGetPairingBindStatus(pairingIndex);
  // If we are a poll server and the client is also a poll client, we have to
  // wait for a heartbeat before we can send the notification command.
  // Otherwise, we can just send the message right away.
  bool pollClient = READBITS(status, PAIRING_ENTRY_POLLING_ACTIVE_BIT);
  if (pollClient) {
    // New action mapping has been added. Send negotiate request to create
    // mapping.
    if (READBIT(actionRemapNegotiateRequestTable[pairingIndex/8], pairingIndex & 0x07)) {
      CLEARBIT(actionRemapNegotiateRequestTable[pairingIndex/8], pairingIndex & 0x07);
      emberAfRf4ceGdpClientNotification(pairingIndex,
                                        EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0,
                                        EMBER_RF4CE_NULL_VENDOR_ID,
                                        CLIENT_NOTIFICATION_SUBTYPE_REQUEST_ACTION_MAPPING_NEGOTIATION,
                                        NULL,
                                        CLIENT_NOTIFICATION_REQUEST_ACTION_MAPPING_NEGOTIATION_PAYLOAD_LENGTH);
      return;
    }
    // Existing action mapping has been updated/restored to default. Send update
    // request.
    requestSelectiveAMUpdatePerPairing(pairingIndex);
  }
}
#endif // EMBER_AF_PLUGIN_RF4CE_GDP_IS_POLL_SERVER


