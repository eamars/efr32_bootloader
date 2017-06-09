// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "app/framework/plugin/rf4ce-mso/rf4ce-mso.h"
#include "rf4ce-mso-ir-rf-database-recipient.h"

// If the Use Default bit is set, the RF Pressed/Repeated/Released Specified
// and IR Specified bits and their corresponding descriptors are ignored.
#define SET_DEFAULT(entry)                                                \
  ((entry)->flags = EMBER_AF_RF4CE_MSO_IR_RF_DATABASE_FLAG_USE_DEFAULT)

// The used heap space is simply how far the tail has drifted from the start of
// the heap.  The free space is everything from the tail to the end.
#define USED_HEAP_SPACE() (tail - heap)
#define FREE_HEAP_SPACE() (sizeof(heap) - USED_HEAP_SPACE())

// keyCodes[] and database[] belong together. Entry 'n' in the former is the
// keyCode of entry 'n' in database.
static EmberAfRf4ceMsoKeyCode keyCodes[EMBER_AF_PLUGIN_RF4CE_MSO_IR_RF_DATABASE_RECIPIENT_KEY_CODE_COUNT];
static EmberAfRf4ceMsoIrRfDatabaseEntry database[EMBER_AF_PLUGIN_RF4CE_MSO_IR_RF_DATABASE_RECIPIENT_KEY_CODE_COUNT];
static uint8_t heap[EMBER_AF_PLUGIN_RF4CE_MSO_IR_RF_DATABASE_RECIPIENT_HEAP_SIZE];
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


// Public API

EmberStatus emberAfRf4ceMsoIrRfDatabaseRecipientAdd(EmberAfRf4ceMsoKeyCode keyCode,
                                                    EmberAfRf4ceMsoIrRfDatabaseEntry *entry)
{
  // Adding an entry is adding a key code to keyCode table and adding its
  // database entry to the database table.
  uint8_t index = getEntryIndex(keyCode);
  if (index == 0xFF) {
    // not found, look for empty entry
    if (0xFF == (index = getEntryIndex((EmberAfRf4ceMsoKeyCode)0xFF))) {
      // not found, table full
      return EMBER_TABLE_FULL;
    }
    // Add to keyCode table
    keyCodes[index] = keyCode;
  }
  // Add or update entry
  return setEntry(index, entry);
}

EmberStatus emberAfRf4ceMsoIrRfDatabaseRecipientRemove(EmberAfRf4ceMsoKeyCode keyCode)
{
  // Removing an entry is clearing its key code from the keyCode table and
  // replacing its database entry with a default.
  EmberAfRf4ceMsoIrRfDatabaseEntry entry;
  uint8_t index = getEntryIndex(keyCode);
  if (index == 0xFF) {
    return EMBER_NOT_FOUND;
  }
  keyCodes[index] = 0xFF;
  SET_DEFAULT(&entry);
  return setEntry(index, &entry);
}

void emberAfRf4ceMsoIrRfDatabaseRecipientRemoveAll(void)
{
  // Wipe the key codes, database and the heap.
  MEMSET(keyCodes, 0xFF, sizeof(keyCodes));
  MEMSET(database, 0, sizeof(database));
  MEMSET(heap, 0, sizeof(heap));
  tail = heap;
}


void emberAfPluginRf4ceMsoIrRfDatabaseRecipientInitCallback(void)
{
  emberAfRf4ceMsoIrRfDatabaseRecipientRemoveAll();
}


EmberAfRf4ceStatus emberAfPluginRf4ceMsoGetIrRfDatabaseAttributeCallback(uint8_t pairingIndex,
                                                                         uint8_t entryIndex,
                                                                         uint8_t *valueLength,
                                                                         uint8_t* value)
{
  UNUSED_VAR(pairingIndex);
  uint8_t *finger;
  uint8_t index;

  // entryIndex is the key code.  If we don't support the key code, we return
  // invalid index.
  index = getEntryIndex((EmberAfRf4ceMsoKeyCode)entryIndex);
  if (index == 0xFF) {
    return EMBER_AF_RF4CE_STATUS_INVALID_INDEX;
  }

  finger = value;
  *finger++ = database[index].flags;

  if (emberAfRf4ceMsoIrRfDatabaseEntryHasRfPressedDescriptor(&database[index])) {
    *finger++ = database[index].rfPressedDescriptor.rfConfig;
    *finger++ = database[index].rfPressedDescriptor.txOptions;
    *finger++ = database[index].rfPressedDescriptor.payloadLength;
    MEMCOPY(finger,
            database[index].rfPressedDescriptor.payload,
            database[index].rfPressedDescriptor.payloadLength);
    finger += database[index].rfPressedDescriptor.payloadLength;
  }
  if (emberAfRf4ceMsoIrRfDatabaseEntryHasRfRepeatedDescriptor(&database[index])) {
    *finger++ = database[index].rfRepeatedDescriptor.rfConfig;
    *finger++ = database[index].rfRepeatedDescriptor.txOptions;
    *finger++ = database[index].rfRepeatedDescriptor.payloadLength;
    MEMCOPY(finger,
            database[index].rfRepeatedDescriptor.payload,
            database[index].rfRepeatedDescriptor.payloadLength);
    finger += database[index].rfRepeatedDescriptor.payloadLength;
  }
  if (emberAfRf4ceMsoIrRfDatabaseEntryHasRfReleasedDescriptor(&database[index])) {
    *finger++ = database[index].rfReleasedDescriptor.rfConfig;
    *finger++ = database[index].rfReleasedDescriptor.txOptions;
    *finger++ = database[index].rfReleasedDescriptor.payloadLength;
    MEMCOPY(finger,
            database[index].rfReleasedDescriptor.payload,
            database[index].rfReleasedDescriptor.payloadLength);
    finger += database[index].rfReleasedDescriptor.payloadLength;
  }
  if (emberAfRf4ceMsoIrRfDatabaseEntryHasIrDescriptor(&database[index])) {
    *finger++ = database[index].irDescriptor.irConfig;
    *finger++ = database[index].irDescriptor.irCodeLength;
    MEMCOPY(finger,
            database[index].irDescriptor.irCode,
            database[index].irDescriptor.irCodeLength);
    finger += database[index].irDescriptor.irCodeLength;
  }

  *valueLength = finger - value;
  return EMBER_AF_RF4CE_STATUS_SUCCESS;
}

bool emberAfPluginRf4ceMsoHaveIrRfDatabaseAttributeCallback(uint8_t pairingIndex,
                                                               uint8_t entryIndex)
{
  UNUSED_VAR(pairingIndex);

  // entryIndex is the key code.  If we don't support the key code, we return
  // false.
  uint8_t index = getEntryIndex((EmberAfRf4ceMsoKeyCode)entryIndex);
  if (index == 0xFF) {
    return false;
  }

  return true;
}


// Local functions
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


