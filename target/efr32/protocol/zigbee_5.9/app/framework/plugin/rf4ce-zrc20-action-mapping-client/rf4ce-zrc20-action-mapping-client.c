/* Copyright 2014 Silicon Laboratories, Inc. */

#include "af.h"
#include "app/framework/plugin/rf4ce-profile/rf4ce-profile.h"
#include "app/framework/plugin/rf4ce-zrc20/rf4ce-zrc20.h"
#include "rf4ce-zrc20-action-mapping-client.h"

#if defined(EMBER_SCRIPTED_TEST)
#include "../rf4ce-zrc20/rf4ce-zrc20-test.h"
#endif

//------------------------------------------------------------------------------
// ZRC 2.0 implemented callbacks

#if !defined(EMBER_SCRIPTED_TEST)

void emberAfPluginRf4ceZrc20ActionMappingClientInitCallback(void)
{
  emberAfRf4ceZrc20ActionMappingClientClearAllActionMappings();
}

uint16_t emberAfPluginRf4ceZrc20GetMappableActionCountCallback(uint8_t pairingIndex)
{
  // This plugin uses the same set of mappable actions for all pairings, so the
  // specific pairing index is ignored.
  return emAfPluginRf4ceZrc20ActionMappingClientGetMappableActionCount();
}

EmberStatus emberAfPluginRf4ceZrc20GetMappableActionCallback(uint8_t pairingIndex,
                                                             uint16_t entryIndex,
                                                             EmberAfRf4ceZrcMappableAction *mappableAction)
{
  // This plugin uses the same set of mappable actions for all pairings, so the
  // specific pairing index is ignored.
  return emAfPluginRf4ceZrc20ActionMappingClientGetMappableAction(entryIndex,
                                                                  mappableAction);
}

void emberAfPluginRf4ceZrc20IncomingActionMappingCallback(uint8_t pairingIndex,
                                                          uint16_t entryIndex,
                                                          EmberAfRf4ceZrcActionMapping *actionMapping)
{
  emAfRf4ceZrc20ActionMappingClientSetActionMapping(pairingIndex,
                                                    entryIndex,
                                                    actionMapping);
}

#endif // !EMBER_SCRIPTED_TEST

//------------------------------------------------------------------------------


// If the Use Default bit is set, the RF Specified and IR Specified bits and
// their corresponding descriptors are ignored.
#define SET_DEFAULT(entry)                                                \
  ((entry)->mappingFlags = EMBER_AF_RF4CE_ZRC_ACTION_MAPPING_MAPPING_FLAGS_USE_DEFAULT_BIT)

// The used heap space is simply how far the tail has drifted from the start of
// the heap.  The free space is everything from the tail to the end.
#define USED_HEAP_SPACE() (tail - actionMappingsClientHeap)
#define FREE_HEAP_SPACE() (sizeof(actionMappingsClientHeap) - USED_HEAP_SPACE())

static const EmberAfRf4ceZrcMappableAction mappableActionsClientTable[EMBER_AF_RF4CE_ZRC_MAPPABLE_ACTION_COUNT] =
        EMBER_AF_RF4CE_ZRC_MAPPABLE_ACTIONS;
static EmberAfRf4ceZrcActionMapping actionMappingsClientTable[EMBER_RF4CE_PAIRING_TABLE_SIZE][EMBER_AF_RF4CE_ZRC_MAPPABLE_ACTION_COUNT];
static uint8_t actionMappingsClientHeap[EMBER_AF_PLUGIN_RF4CE_ZRC20_ACTION_MAPPING_CLIENT_ACTION_MAPPING_HEAP_SIZE];
static uint8_t *tail = actionMappingsClientHeap;

static void removeOldEntry(uint8_t pairingIndex, uint16_t index);
static uint32_t calculateHeapUsage(const EmberAfRf4ceZrcActionMapping *entry);
static uint8_t *findHead(const EmberAfRf4ceZrcActionMapping *entry);
static uint8_t *copyActionData(bool hasDescriptor,
                             const EmberAfRf4ceZrcActionMapping *entry);
static uint8_t *copyIrCode(bool hasDescriptor,
                         const EmberAfRf4ceZrcActionMapping *entry);


// Public functions.

void emberAfRf4ceZrc20ActionMappingClientClearAllActionMappings(void)
{
  // Reset each entry back to the default and then wipe the heap.
  uint8_t i, j;
  for (i = 0; i < EMBER_RF4CE_PAIRING_TABLE_SIZE; i++) {
    for (j = 0; j < EMBER_AF_RF4CE_ZRC_MAPPABLE_ACTION_COUNT; j++) {
      SET_DEFAULT(&actionMappingsClientTable[i][j]);
    }
  }
  MEMSET(actionMappingsClientHeap, 0, sizeof(actionMappingsClientHeap));
  tail = actionMappingsClientHeap;
}

EmberStatus emberAfRf4ceZrc20ActionMappingClientClearActionMappingsPerPairing(uint8_t pairingIndex)
{
  uint16_t i;

  // Pairing index out of range.
  if (pairingIndex >= EMBER_RF4CE_PAIRING_TABLE_SIZE) {
    return EMBER_INDEX_OUT_OF_RANGE;
  }

  // Clear all action mappings belonging to the pairing index.
  for (i=0; i<EMBER_AF_RF4CE_ZRC_MAPPABLE_ACTION_COUNT; i++) {
    emAfRf4ceZrc20ActionMappingClientClearActionMapping(pairingIndex, i);
  }

  return EMBER_SUCCESS;
}

EmberStatus emberAfRf4ceZrc20ActionMappingClientClearActionMapping(uint8_t pairingIndex,
                                                                   EmberAfRf4ceDeviceType actionDeviceType,
                                                                   EmberAfRf4ceZrcActionBank actionBank,
                                                                   EmberAfRf4ceZrcActionCode actionCode)
{
  uint16_t i;

  for (i = 0; i < EMBER_AF_RF4CE_ZRC_MAPPABLE_ACTION_COUNT; i++) {
    if ((mappableActionsClientTable[i].actionDeviceType == actionDeviceType)
        && (mappableActionsClientTable[i].actionBank == actionBank)
        && (mappableActionsClientTable[i].actionCode == actionCode)) {
      return emAfRf4ceZrc20ActionMappingClientClearActionMapping(pairingIndex,
                                                                 i);
    }
  }

  // Did not find matching entry.
  return EMBER_NOT_FOUND;
}

EmberStatus emberAfRf4ceZrc20ActionMappingClientGetActionMapping(uint8_t pairingIndex,
                                                                 EmberAfRf4ceDeviceType actionDeviceType,
                                                                 EmberAfRf4ceZrcActionBank actionBank,
                                                                 EmberAfRf4ceZrcActionCode actionCode,
                                                                 EmberAfRf4ceZrcActionMapping* actionMapping)
{
  uint16_t i;

  // Pairing index out of range.
  if (pairingIndex >= EMBER_RF4CE_PAIRING_TABLE_SIZE) {
    return EMBER_INDEX_OUT_OF_RANGE;
  }

  for (i = 0; i < EMBER_AF_RF4CE_ZRC_MAPPABLE_ACTION_COUNT; i++) {
    if ((mappableActionsClientTable[i].actionDeviceType == actionDeviceType)
        && (mappableActionsClientTable[i].actionBank == actionBank)
        && (mappableActionsClientTable[i].actionCode == actionCode)) {
      // Found matching entry. Get its action mapping.
      MEMCOPY((uint8_t*) actionMapping,
              (uint8_t*) &actionMappingsClientTable[pairingIndex][i],
              sizeof(EmberAfRf4ceZrcActionMapping));
      return EMBER_SUCCESS;
    }
  }

  // Did not find matching entry.
  return EMBER_NOT_FOUND;
}

EmberStatus emberAfRf4ceZrc20ActionMappingClientSetActionMapping(uint8_t pairingIndex,
                                                                 EmberAfRf4ceDeviceType actionDeviceType,
                                                                 EmberAfRf4ceZrcActionBank actionBank,
                                                                 EmberAfRf4ceZrcActionCode actionCode,
                                                                 EmberAfRf4ceZrcActionMapping* actionMapping)
{
  uint16_t i;

  for (i = 0; i < EMBER_AF_RF4CE_ZRC_MAPPABLE_ACTION_COUNT; i++) {
    if ((mappableActionsClientTable[i].actionDeviceType == actionDeviceType)
        && (mappableActionsClientTable[i].actionBank == actionBank)
        && (mappableActionsClientTable[i].actionCode == actionCode)) {
      return emAfRf4ceZrc20ActionMappingClientSetActionMapping(pairingIndex,
                                                               i,
                                                               actionMapping);
    }
  }

  // Did not find matching entry.
  return EMBER_NOT_FOUND;
}


// Private functions.

EmberStatus emAfRf4ceZrc20ActionMappingClientClearActionMapping(uint8_t pairingIndex,
                                                                uint16_t entryIndex)
{
  // Pairing index out of range.
  if (pairingIndex >= EMBER_RF4CE_PAIRING_TABLE_SIZE) {
    return EMBER_INDEX_OUT_OF_RANGE;
  }

  // Entry index out of range.
  if (entryIndex >= EMBER_AF_RF4CE_ZRC_MAPPABLE_ACTION_COUNT) {
    return EMBER_INDEX_OUT_OF_RANGE;
  }

  // remove the RF and IR descriptors of the action mapping entry if they exist.
  removeOldEntry(pairingIndex, entryIndex);
  // The basic structure of the new entry is copied as is to the database.  The
  // variable-sized portion of each descriptor is copied into the heap and then
  // the pointer to that data from the database is adjusted to point into the
  // heap.
  MEMSET(&actionMappingsClientTable[pairingIndex][entryIndex],
         0,
         sizeof(EmberAfRf4ceZrcActionMapping));
  SET_DEFAULT(&actionMappingsClientTable[pairingIndex][entryIndex]);

  return EMBER_SUCCESS;
}

EmberStatus emAfRf4ceZrc20ActionMappingClientSetActionMapping(uint8_t pairingIndex,
                                                              uint16_t entryIndex,
                                                              EmberAfRf4ceZrcActionMapping* actionMapping)
{
  // Pairing index out of range.
  if (pairingIndex >= EMBER_RF4CE_PAIRING_TABLE_SIZE) {
    return EMBER_INDEX_OUT_OF_RANGE;
  }

  // Entry index out of range.
  if (entryIndex >= EMBER_AF_RF4CE_ZRC_MAPPABLE_ACTION_COUNT) {
    return EMBER_INDEX_OUT_OF_RANGE;
  }

  // we always remove the old entry, even though the new entry might not fit
  removeOldEntry(pairingIndex, entryIndex);
  // The free space is the unused space and includes what we reclaimed from the
  // old entry.  If we don't have enough room, we drop the new entry and leave
  // a default in its place.
  if (FREE_HEAP_SPACE() < calculateHeapUsage(actionMapping)) {
    SET_DEFAULT(&actionMappingsClientTable[pairingIndex][entryIndex]);
    return EMBER_TABLE_FULL;
  }

  // The basic structure of the new entry is copied as is to the database.  The
  // variable-sized portion of each descriptor is copied into the heap and then
  // the pointer to that data from the database is adjusted to point into the
  // heap.
  MEMCOPY(&actionMappingsClientTable[pairingIndex][entryIndex],
          actionMapping,
          sizeof(EmberAfRf4ceZrcActionMapping));
  actionMappingsClientTable[pairingIndex][entryIndex].actionData
    = copyActionData(emberAfRf4ceZrc20ActionMappingEntryHasRfDescriptor(actionMapping),
                    actionMapping);
  actionMappingsClientTable[pairingIndex][entryIndex].irCode
    = copyIrCode(emberAfRf4ceZrc20ActionMappingEntryHasIrDescriptor(actionMapping),
                 actionMapping);

  return EMBER_SUCCESS;
}

uint16_t emAfPluginRf4ceZrc20ActionMappingClientGetMappableActionCount(void)
{
    return EMBER_AF_RF4CE_ZRC_MAPPABLE_ACTION_COUNT;
}

EmberStatus emAfPluginRf4ceZrc20ActionMappingClientGetMappableAction(uint16_t entryIndex,
                                                                     EmberAfRf4ceZrcMappableAction* mappableAction)
{
    /* Index out of range. */
    if (entryIndex >= EMBER_AF_RF4CE_ZRC_MAPPABLE_ACTION_COUNT)
    {
        return EMBER_INDEX_OUT_OF_RANGE;
    }

    MEMCOPY((uint8_t*)mappableAction,
            (uint8_t*)&mappableActionsClientTable[entryIndex],
            sizeof(EmberAfRf4ceZrcMappableAction));

    return EMBER_SUCCESS;
}


static void removeOldEntry(uint8_t pairingIndex, uint16_t index)
{
  // When a new entry comes in, we always remove the old entry, even though the
  // new entry might not fit.  The rationale is that the old entry is being
  // replaced because it is no longer valid for the peripheral devices in the
  // system.  Reverting to a default entry seems better than keeping the old,
  // invalid entry.
  uint32_t reclaimableBytes
    = calculateHeapUsage(&actionMappingsClientTable[pairingIndex][index]);
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
    head = findHead(&actionMappingsClientTable[pairingIndex][index]);
    assert(head != NULL);
    assert(actionMappingsClientHeap <= head);
    assert(head + reclaimableBytes <= tail);

    // Rewind the tail pointer and all the action data and IR code pointers for
    // entries that follow the old one.  This makes them point to where they
    // should after the heap is adjusted.  This means the pointers are NOT
    // valid until the heap is adjusted.
    tail -= reclaimableBytes;
    for (i = 0; i < COUNTOF(actionMappingsClientTable[pairingIndex]); i++) {
      if (head < actionMappingsClientTable[pairingIndex][i].actionData) {
        actionMappingsClientTable[pairingIndex][i].actionData -= reclaimableBytes;
      }
      if (head < actionMappingsClientTable[pairingIndex][i].irCode) {
          actionMappingsClientTable[pairingIndex][i].irCode -= reclaimableBytes;
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
    assert(tail <= actionMappingsClientHeap + sizeof(actionMappingsClientHeap));
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
    assert(tail <= actionMappingsClientHeap + sizeof(actionMappingsClientHeap));
  }
  return irCode;
}


