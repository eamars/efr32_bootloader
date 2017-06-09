// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "app/framework/plugin/rf4ce-mso/rf4ce-mso.h"
#include "rf4ce-mso-ir-rf-database-originator.h"

// If the Use Default bit is set, the RF Pressed/Repeated/Released Specified
// and IR Specified bits and their corresponding descriptors are ignored.
#define SET_DEFAULT(entry)                                                \
  ((entry)->flags = EMBER_AF_RF4CE_MSO_IR_RF_DATABASE_FLAG_USE_DEFAULT)

// The used heap space is simply how far the tail has drifted from the start of
// the heap.  The free space is everything from the tail to the end.
#define USED_HEAP_SPACE() (tail - heap)
#define FREE_HEAP_SPACE() (sizeof(heap) - USED_HEAP_SPACE())

static const EmberAfRf4ceMsoKeyCode keyCodes[]
  = EMBER_AF_RF4CE_MSO_IR_RF_DATABASE_ORIGINATOR_KEY_CODES;
static EmberAfRf4ceMsoIrRfDatabaseEntry database[EMBER_AF_RF4CE_MSO_IR_RF_DATABASE_ORIGINATOR_KEY_CODE_COUNT];

static uint8_t heap[EMBER_AF_PLUGIN_RF4CE_MSO_IR_RF_DATABASE_ORIGINATOR_HEAP_SIZE];
static uint8_t *tail = heap;

static uint8_t getEntryIndex(EmberAfRf4ceMsoKeyCode keyCode);

static uint32_t calculateHeapUsage(const EmberAfRf4ceMsoIrRfDatabaseEntry *entry);
static uint8_t *findHead(const EmberAfRf4ceMsoIrRfDatabaseEntry *entry);
static uint8_t *copyRfPayload(bool hasDescriptor,
                            const EmberAfRf4ceMsoIrRfDatabaseRfDescriptor *rfDescriptor);
static uint8_t *copyIrCode(bool hasDescriptor,
                         const EmberAfRf4ceMsoIrRfDatabaseIrDescriptor *irDescriptor);
static EmberStatus setEntry(uint8_t index,
                            const EmberAfRf4ceMsoIrRfDatabaseEntry *entry);

void emberAfPluginRf4ceMsoIrRfDatabaseOriginatorInitCallback(void)
{
  emberAfRf4ceMsoIrRfDatabaseOriginatorClearAll();
}

void emberAfPluginRf4ceMsoIncomingIrRfDatabaseAttributeCallback(uint8_t pairingIndex,
                                                                uint8_t entryIndex,
                                                                uint8_t valueLength,
                                                                const uint8_t *value)
{
  EmberAfRf4ceMsoIrRfDatabaseEntry entry;
  const uint8_t *finger;
  uint8_t index;

  // The entry index is the key code.  If we don't support the key code, we
  // ignore this entry.
  index = getEntryIndex((EmberAfRf4ceMsoKeyCode)entryIndex);
  if (index == 0xFF) {
    return;
  }

  // TODO: Check length of incoming data.

  finger = value;
  entry.flags = *finger++;

  // TODO: Extract RF descriptor parsing to a function.
  if (emberAfRf4ceMsoIrRfDatabaseEntryHasRfPressedDescriptor(&entry)) {
    entry.rfPressedDescriptor.rfConfig = *finger++;
    entry.rfPressedDescriptor.txOptions = *finger++;
    entry.rfPressedDescriptor.payloadLength = *finger++;
    entry.rfPressedDescriptor.payload = finger;
    finger += entry.rfPressedDescriptor.payloadLength;
  }
  if (emberAfRf4ceMsoIrRfDatabaseEntryHasRfRepeatedDescriptor(&entry)) {
    entry.rfRepeatedDescriptor.rfConfig = *finger++;
    entry.rfRepeatedDescriptor.txOptions = *finger++;
    entry.rfRepeatedDescriptor.payloadLength = *finger++;
    entry.rfRepeatedDescriptor.payload = finger;
    finger += entry.rfRepeatedDescriptor.payloadLength;
  }
  if (emberAfRf4ceMsoIrRfDatabaseEntryHasRfReleasedDescriptor(&entry)) {
    entry.rfReleasedDescriptor.rfConfig = *finger++;
    entry.rfReleasedDescriptor.txOptions = *finger++;
    entry.rfReleasedDescriptor.payloadLength = *finger++;
    entry.rfReleasedDescriptor.payload = finger;
    finger += entry.rfReleasedDescriptor.payloadLength;
  }
  if (emberAfRf4ceMsoIrRfDatabaseEntryHasIrDescriptor(&entry)) {
    entry.irDescriptor.irConfig = *finger++;
    entry.irDescriptor.irCodeLength = *finger++;
    entry.irDescriptor.irCode = finger;
    finger += entry.irDescriptor.irCodeLength;
  }

  setEntry(index, &entry);
}

EmberStatus emberAfRf4ceMsoIrRfDatabaseOriginatorGet(EmberAfRf4ceMsoKeyCode keyCode,
                                                     EmberAfRf4ceMsoIrRfDatabaseEntry *entry)
{
  uint8_t index = getEntryIndex(keyCode);
  if (index == 0xFF) {
    return EMBER_NOT_FOUND;
  }
  MEMCOPY(entry, &database[index], sizeof(EmberAfRf4ceMsoIrRfDatabaseEntry));
  return EMBER_SUCCESS;
}

EmberStatus emberAfRf4ceMsoIrRfDatabaseOriginatorSet(EmberAfRf4ceMsoKeyCode keyCode,
                                                     const EmberAfRf4ceMsoIrRfDatabaseEntry *entry)
{
  uint8_t index = getEntryIndex(keyCode);
  if (index == 0xFF) {
    return EMBER_NOT_FOUND;
  }
  return setEntry(index, entry);
}

EmberStatus emberAfRf4ceMsoIrRfDatabaseOriginatorClear(EmberAfRf4ceMsoKeyCode keyCode)
{
  // Clearing an entry is the same as replacing it with a default.
  EmberAfRf4ceMsoIrRfDatabaseEntry entry;
  uint8_t index = getEntryIndex(keyCode);
  if (index == 0xFF) {
    return EMBER_NOT_FOUND;
  }
  SET_DEFAULT(&entry);
  return setEntry(index, &entry);
}

void emberAfRf4ceMsoIrRfDatabaseOriginatorClearAll(void)
{
  // Reset each entry back to the default and then wipe the heap.
  uint8_t i;
  for (i = 0; i < COUNTOF(database); i++) {
    SET_DEFAULT(&database[i]);
  }
  MEMSET(heap, 0, sizeof(heap));
  tail = heap;
}

static uint8_t getEntryIndex(EmberAfRf4ceMsoKeyCode keyCode)
{
  // The index of the key code in the key codes table is the same as the index
  // of the entry in the database table.
  uint8_t i;
  for (i = 0; i < COUNTOF(keyCodes); i++) {
    if (keyCode == keyCodes[i]) {
      return i;
    }
  }
  return 0xFF;
}

static uint32_t calculateHeapUsage(const EmberAfRf4ceMsoIrRfDatabaseEntry *entry)
{
  // The heap usage of an entry is the sum of the RF payload lengths and the
  // IR code length.  The heap usage is zero if the default is used.
  uint32_t bytes = 0;
  if (emberAfRf4ceMsoIrRfDatabaseEntryHasRfPressedDescriptor(entry)) {
    bytes += entry->rfPressedDescriptor.payloadLength;
  }
  if (emberAfRf4ceMsoIrRfDatabaseEntryHasRfRepeatedDescriptor(entry)) {
    bytes += entry->rfRepeatedDescriptor.payloadLength;
  }
  if (emberAfRf4ceMsoIrRfDatabaseEntryHasRfReleasedDescriptor(entry)) {
    bytes += entry->rfReleasedDescriptor.payloadLength;
  }
  if (emberAfRf4ceMsoIrRfDatabaseEntryHasIrDescriptor(entry)) {
    bytes += entry->irDescriptor.irCodeLength;
  }
  return bytes;
}

static uint8_t *findHead(const EmberAfRf4ceMsoIrRfDatabaseEntry *entry)
{
  // The head of an entry in the heap is the location of the first RF payload
  // or IR code.  The head is NULL if the default is used.
  if (emberAfRf4ceMsoIrRfDatabaseEntryHasRfPressedDescriptor(entry)) {
    return (uint8_t *)entry->rfPressedDescriptor.payload;
  } else if (emberAfRf4ceMsoIrRfDatabaseEntryHasRfRepeatedDescriptor(entry)) {
    return (uint8_t *)entry->rfRepeatedDescriptor.payload;
  } else if (emberAfRf4ceMsoIrRfDatabaseEntryHasRfReleasedDescriptor(entry)) {
    return (uint8_t *)entry->rfReleasedDescriptor.payload;
  } else if (emberAfRf4ceMsoIrRfDatabaseEntryHasIrDescriptor(entry)) {
    return (uint8_t *)entry->irDescriptor.irCode;
  } else {
    return NULL;
  }
}

static uint8_t *copyRfPayload(bool hasDescriptor,
                            const EmberAfRf4ceMsoIrRfDatabaseRfDescriptor *rfDescriptor)
{
  // If the RF descriptor exists, the payload is copied to the heap and the
  // tail is moved forward.  Otherwise, the payload pointer points nowhere.  If
  // we ever end up off the end of the heap, then something has gone very
  /// wrong.
  uint8_t *payload = NULL;
  if (hasDescriptor) {
    payload = tail;
    MEMCOPY(tail, rfDescriptor->payload, rfDescriptor->payloadLength);
    tail += rfDescriptor->payloadLength;
    assert(tail <= heap + sizeof(heap));
  }
  return payload;
}

static uint8_t *copyIrCode(bool hasDescriptor,
                         const EmberAfRf4ceMsoIrRfDatabaseIrDescriptor *irDescriptor)
{
  // If the IR descriptor exists, the code is copied to the heap and the tail
  // is moved forward.  Otherwise, the code pointer points nowhere.  If we ever
  // end up off the end of the heap, then something has gone very wrong.
  uint8_t *irCode = NULL;
  if (hasDescriptor) {
    irCode = tail;
    MEMCOPY(tail, irDescriptor->irCode, irDescriptor->irCodeLength);
    tail += irDescriptor->irCodeLength;
    assert(tail <= heap + sizeof(heap));
  }
  return irCode;
}

static EmberStatus setEntry(uint8_t index,
                            const EmberAfRf4ceMsoIrRfDatabaseEntry *entry)
{
  // When a new entry comes in, we always remove the old entry, even though the
  // new entry might not fit.  The rationale is that the old entry is being
  // replaced because it is no longer valid for the peripheral devices in the
  // system.  Reverting to a default entry seems better than keeping the old,
  // invalid entry.
  uint32_t reclaimableBytes = calculateHeapUsage(&database[index]);
  if (reclaimableBytes != 0) {
    uint8_t *head;
    uint8_t i;

    // If the space we think we are using is ever less than what we think we
    // can reclaim, then something has gone very wrong.
    assert(reclaimableBytes <= USED_HEAP_SPACE());

    // The RF payloads and the IR code are all stored contiguously in the heap,
    // although some of them may be empty.  We just need to find whichever one
    // is first.  If we expect to have one but can't find it or we do find it
    // but its not actually within the heap area, then something has gone very
    // wrong.
    head = findHead(&database[index]);
    assert(head != NULL);
    assert(heap <= head);
    assert(head + reclaimableBytes <= tail);

    // Rewind the tail pointer and all the RF payload and IR code pointers for
    // entries that follow the old one.  This makes them point to where they
    // should after the heap is adjusted.  This means the pointers are NOT
    // valid until the heap is adjusted.
    tail -= reclaimableBytes;
    for (i = 0; i < COUNTOF(database); i++) {
      if (head < database[i].rfPressedDescriptor.payload) {
        database[i].rfPressedDescriptor.payload -= reclaimableBytes;
      }
      if (head < database[i].rfRepeatedDescriptor.payload) {
        database[i].rfRepeatedDescriptor.payload -= reclaimableBytes;
      }
      if (head < database[i].rfReleasedDescriptor.payload) {
        database[i].rfReleasedDescriptor.payload -= reclaimableBytes;
      }
      if (head < database[i].irDescriptor.irCode) {
        database[i].irDescriptor.irCode -= reclaimableBytes;
      }
    }

    // Move the stuff after the old entry so it immediately follows the stuff
    // preceding the old entry.  The old entry is now gone and the tail, RF
    // payload, and IR pointers are all valid again.
    MEMMOVE(head, head + reclaimableBytes, tail - head);

    // Wipe the stuff following the new tail.
    MEMSET(tail, 0, reclaimableBytes);
  }

  // The free space is the unused space and includes what we reclaimed from the
  // old entry.  If we don't have enough room, we drop the new entry and leave
  // a default in its place.
  if (FREE_HEAP_SPACE() < calculateHeapUsage(entry)) {
    SET_DEFAULT(&database[index]);
    return EMBER_TABLE_FULL;
  }

  // The basic structure of the new entry is copied as is to the database.  The
  // variable-sized portion of each descriptor is copied into the heap and then
  // the pointer to that data from the database is adjusted to point into the
  // heap.
  MEMCOPY(&database[index], entry, sizeof(EmberAfRf4ceMsoIrRfDatabaseEntry));
  database[index].rfPressedDescriptor.payload
    = copyRfPayload(emberAfRf4ceMsoIrRfDatabaseEntryHasRfPressedDescriptor(entry),
                    &entry->rfPressedDescriptor);
  database[index].rfRepeatedDescriptor.payload
    = copyRfPayload(emberAfRf4ceMsoIrRfDatabaseEntryHasRfRepeatedDescriptor(entry),
                    &entry->rfRepeatedDescriptor);
  database[index].rfReleasedDescriptor.payload
    = copyRfPayload(emberAfRf4ceMsoIrRfDatabaseEntryHasRfReleasedDescriptor(entry),
                    &entry->rfReleasedDescriptor);
  database[index].irDescriptor.irCode
    = copyIrCode(emberAfRf4ceMsoIrRfDatabaseEntryHasIrDescriptor(entry),
                 &entry->irDescriptor);

  return EMBER_SUCCESS;
}
