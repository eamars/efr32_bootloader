/*
 * File: buffer-management.c
 * Description: Buffer allocation and management routines.
 * Author(s): Richard Kelsey
 *
 * Copyright 2009 by Ember Corporation. All rights reserved.                *80*
 */

// Questions:
//  - How much RAM does the pointer compression actually save?

// Provides two links in order to allow queues of header+payload.

// Issues:
//  Buffered output
//    Add a layer on top of the FIFO that allows the FIFO code
//    to pull more data when needed.  This requires a lock of
//    some kind because the FIFO operates as an ISR.  If the
//    lock is set the FIFO sets a flag indicating that data should
//    be pushed when the lock is released.  This allows the
//    existing buffered output code to be left as-is, and only
//    requires minor changes to the FIFO code.  Post-compaction
//    cleanup needs to check the flags and push data as needed.

#include <string.h>      // for memmove()
#include "core/ember-stack.h"
#include "hal/hal.h"
#include "buffer-malloc.h"
#include "plugin/serial/serial.h"

HIDDEN uint16_t *emHeapBase;
HIDDEN uint16_t *emHeapLimit;
HIDDEN uint16_t *heapPointer;             // points to next free word
HIDDEN Buffer phyToMacQueue = NULL_BUFFER;

//#define DEBUG(...) fprintf(stderr, __VA_ARGS__)
#undef DEBUG
#define DEBUG(...)

uint16_t emSaveHeapState(void)
{
  return heapPointer - emHeapBase;
}

void emRestoreHeapState(uint16_t state)
{
  heapPointer = emHeapBase + state;
}  

#define PHY_BUFFER_FREELIST_SIZE 4

static Buffer phyFreelist[PHY_BUFFER_FREELIST_SIZE];
static uint8_t phyFreelistCount;
static uint8_t phyFreelistQueuedCount;

//----------------------------------------------------------------

#ifdef EMBER_TEST

#include <stdlib.h>      // for malloc() and free()

// These are used when running under Unix to keep track of which
// Buffer values are legitimate.  They parallel the heap array:
// emMallocedContents[x] is true iff emHeapBase + x is the beginning of
// a buffer.  emNewMallocedContents[] is used during temporarily during
// the GC.
static uint16_t **emMallocedContents;
static uint16_t **emNewMallocedContents;

#define bufferTest(x) do { x } while (0)

#define NOT_NULL ((uint16_t *) 16)

// During the final phase of the GC we are partly using the old heap
// and partly using the new heap, so the emMallocedContents[] checking doesn't
// work properly.  This disables the checking and is set to true during
// that final GC phase.
static bool inGcCleanup;

// If this is true we use malloc() to allocated the contents of each buffer.
// The buffer management code runs as normal, but emGetBufferPointer()
// returns the malloc'ed contents rather than a pointer into the heap.
// This allows Valgrind to detect attempts to read or write outside the
// buffer's actual contents.
bool emBuffersUseMalloc = false;

static uint16_t *testMalloc(size_t size)
{
  if (emBuffersUseMalloc) {
    return (uint16_t *) malloc(size);
  } else {
    return NOT_NULL;
  }
}

static void testFree(void *thing)
{
  if (emBuffersUseMalloc) {
    free(thing);
  }
}

// These are used in conjuction with tool/simulator/child/child-main.c to
// find for allocations that don't check for a NULL_BUFFER return.  See
// child-main.c for how these are used.
uint32_t allocationCount = 0;             // incremented for each new allocation
uint32_t allocationToFail = 0;            // return NULL_BUFFER for this call

// To allow simulated nodes to modify the heap size at run time.
static uint16_t *newHeapBase;
static uint16_t *newHeapLimit;

#else
#define bufferTest(x) do {} while (0)
#define emBuffersUseMalloc false
#endif

void emResizeHeap(uint32_t newSize)
{
  bufferTest(
  uint8_t *newHeap = (uint8_t *) malloc(newSize);
  newHeapBase = (uint16_t *) newHeap;
  newHeapLimit = (uint16_t *) (newHeap + newSize);
  );
}

Buffer emMallocFreeList = NULL_BUFFER;

void emInitializeBuffers(void)
{
  #if defined(CORTEXM3) || defined(EMBER_STACK_COBRA)
    // Get the heap base and limit for this chip. The base is assumed to be
    // inclusive and the limit is assumed to be exclusive.  We also assume
    // 4-byte alignemnt on the heap base.
    emHeapBase = (uint16_t *)halInternalGetHeapBottom();
    emHeapLimit = (uint16_t *)halInternalGetHeapTop();
  #elif EMBER_TEST
    // Allow the simulator to set the heap size if it wants to.
    if (emHeapBase == NULL) {
      emHeapBase = (uint16_t *) ((((unsigned long) heapMemory) + 3) & -4);
      emHeapLimit = heapMemory + (heapMemorySize / 2);
    }
  #else
    // Align the heap base on 4-byte boundary.
    emHeapBase = (uint16_t *) ((((unsigned long) heapMemory) + 3) & -4);
    emHeapLimit = heapMemory + (heapMemorySize / 2);
  #endif
  heapPointer = emHeapBase;
  assert(emHeapBase != NULL && emBufferBytesRemaining() > 100);
  bufferTest(size_t size = (emHeapLimit - emHeapBase) * sizeof(uint16_t *);
             emMallocedContents = (uint16_t **) malloc(size);
             emNewMallocedContents = (uint16_t **) malloc(size);
             MEMSET(emMallocedContents,
                    0,
                    (emHeapLimit - emHeapBase) * sizeof(uint16_t *));
             inGcCleanup = false;);
}

uint16_t emBufferBytesUsed(void)
{
  assert(heapPointer >= emHeapBase);
  return ((heapPointer - emHeapBase) << 1);
}

uint16_t emBufferBytesRemaining(void)
{
  assert(heapPointer <= emHeapLimit);
  return ((emHeapLimit - heapPointer) << 1);
}

bool emPointsIntoHeap(void *pointer)
{
  return (emHeapBase <= ((uint16_t *) pointer)
          && ((uint16_t *) pointer) < heapPointer);
}

static bool isHeapPointer(uint16_t *thing)
{
  return (thing == NULL
          || emPointsIntoHeap(thing));
}

// Hack for testing the code that deals with allocation by the receive
// ISR during compaction.
#ifdef EMBER_TEST
static void doNothing(void) {}
void (*emCompactionInterruptProc)(void) = doNothing;
#endif // EMBER_TEST

// Biasing by emHeapBase allows us to store pointers in 16 bits.
// Biasing by emHeapBase + 1 allows use to use 0 as NULL_BUFFER,
// which avoids the need to initialize roots.
static uint16_t compressPointer(uint16_t *pointer)
{
  if (pointer == NULL)
    return NULL_BUFFER;
  else
    return (uint16_t) (((pointer - emHeapBase) + 1) & 0xFFFF);
}

static uint16_t *expandPointerNoCheck(uint16_t buffer)
{
  if (buffer == NULL_BUFFER)
    return NULL;
  else {
    return emHeapBase + (buffer - 1);
  }
}

#ifdef EMBER_TEST

static uint16_t *expandPointer(uint16_t buffer)
{
  if (buffer == NULL_BUFFER)
    return NULL;
  else {
    assert(inGcCleanup || emMallocedContents[buffer - 1] != NULL);
    return emHeapBase + (buffer - 1);
  }
}

#else

#define expandPointer expandPointerNoCheck

#endif

//----------------------------------------------------------------

#define NEW_LOCATION_INDEX 0    // Must be first because first value cannot
                                // be 0xFFFF, which is used to mark unused
                                // memory.
#define SIZE_INDEX         1
#define LINK0_INDEX        2
#define LINK1_INDEX        3
#define NUM_LINKS          2

#define OVERHEAD_IN_WORDS 4
#define OVERHEAD_IN_BYTES (WORDS_TO_BYTES(OVERHEAD_IN_WORDS))

// Indirect buffers contain a pointer to memory elsewhere, rather
// than containing the storage themselves.  This allows buffers to
// refer to data in read-only memory, such as certificates.

#define INDIRECT_BUFFER_LENGTH_INDEX  4
#define INDIRECT_BUFFER_POINTER_INDEX 5
#define INDIRECT_BUFFER_OBJ_REF_INDEX 7
#define INDIRECT_BUFFER_DATA_SIZE_IN_BYTES \
 (WORDS_TO_BYTES(1) + 2*sizeof(uint8_t *))

// The compactor uses two flags, LIVE and TRACED.  LIVE is set if a
// buffer is reachable from a root.  TRACED is set if the buffer is
// live and its links have had their LIVE flags set.
//
// These flags are at the start of the size word.
#define LIVE_BIT         0x8000
#define TRACED_BIT       0x4000
#define PHY_FREELIST_BIT 0x2000
#define SIZE_MASK        0x1FFF

// This flag is at the start of the new location mask.
#define INDIRECT_BIT            0x8000
#define NEW_LOCATION_MASK       0x7FFF

#define sizeWord(thing)    ((thing)[SIZE_INDEX])

#define BOOLEAN(x) ((x) ? true : false)

#define getDataSize(thing)    (sizeWord(thing) & SIZE_MASK)
#define clearFlags(thing)     (sizeWord(thing) &= ~(LIVE_BIT | TRACED_BIT))
#define setLive(thing)        (sizeWord(thing) |= LIVE_BIT)
#define isLive(thing)         BOOLEAN((sizeWord(thing) & LIVE_BIT))
#define setTraced(thing)      (sizeWord(thing) |= TRACED_BIT)
#define clearTraced(thing)    (sizeWord(thing) &= ~TRACED_BIT)
#define isTraced(thing)       BOOLEAN((sizeWord(thing) & TRACED_BIT))

#define setPhyFreelistBit(thing)   (sizeWord(thing) |= PHY_FREELIST_BIT)
#define clearPhyFreelistBit(thing) (sizeWord(thing) &= ~PHY_FREELIST_BIT)
#define isOnPhyFreelist(thing)     BOOLEAN((sizeWord(thing) & PHY_FREELIST_BIT))

static uint16_t newLocation(uint16_t *bufferPointer)
{
  return bufferPointer[NEW_LOCATION_INDEX] & NEW_LOCATION_MASK;
}

static void setNewLocation(uint16_t *bufferPointer,
                           uint16_t theNewLocation)
{
  bufferPointer[NEW_LOCATION_INDEX] =
    (bufferPointer[NEW_LOCATION_INDEX] & INDIRECT_BIT)
    | theNewLocation;
}

static bool isIndirect(uint16_t *bufferPointer)
{
  return BOOLEAN(bufferPointer[NEW_LOCATION_INDEX] & INDIRECT_BIT);
}

#ifdef EMBER_TEST
bool bufferIsOnPhyFreelist(Buffer buffer)
{
  return isOnPhyFreelist(expandPointer(buffer));
}
#endif

// This returns an even number so that the buffers (and their contents) are
// aligned on four-byte boundaries, assuming the emHeapBase is on a four-byte
// boundary.
#define BYTES_TO_WORDS(b) ((((b) + 3) >> 2) << 1)
#define WORDS_TO_BYTES(w) ((w) << 1)

static uint16_t getSizeInWords(uint16_t *thing)
{
  return OVERHEAD_IN_WORDS + BYTES_TO_WORDS(getDataSize(thing));
}

// We use 0xFFFF to mark dead space created when a buffer is
// shortened by a word or two.  For larger shortenings we can
// insert a full object.

#define UNUSED_MEMORY 0xFFFF

static uint16_t *nextBuffer(uint16_t *bufferPointer)
{
  uint16_t *next = bufferPointer + getSizeInWords(bufferPointer);
  while (next < heapPointer
         && *next == UNUSED_MEMORY) {
    next += 1;
  }
  return next;
}

#define getSizeInBytes(bufferPointer) (getSizeInWords(bufferPointer) << 1)

static const char *currentUser = "unclaimed";

void emBufferUsage(const char *tag)
{
  currentUser = tag;
}

void emEndBufferUsage(void)
{
  currentUser = "unclaimed";
}

typedef struct {
  const char *user;
  uint16_t count;
  uint16_t size;
} UserData;

static UserData *userData;
static uint16_t userDataCount;

static void noteUse(uint16_t *object)
{
  if (userDataCount > 0
      && ! isLive(object)) {
    uint16_t i;
    assert(strcmp(currentUser, "unclaimed"));
    for (i = 0; ; i++) {
      assert(i < userDataCount);
      if (userData[i].user == NULL) {
        userData[i].user = currentUser;
        break;
      } else if (strcmp(userData[i].user, currentUser) == 0) {
        break;
      }
    }
    userData[i].count += 1;
    userData[i].size += getSizeInBytes(object);
  }
}

#ifdef EMBER_SCRIPTED_TEST
  #define reportPrint(...)
#else
  #ifdef EMBER_TEST
    #define reportPrint(p, f1, f2, ...) \
    if (port == 0xFF) { fprintf(stderr, f1, __VA_ARGS__); } \
    else { emberSerialPrintfLine(p, f2, __VA_ARGS__); }
  #else
    #define reportPrint(p, f1, f2, ...) \
    emberSerialPrintfLine(p, f2, __VA_ARGS__);
  #endif
#endif

static void reportUsage(uint8_t port)
{
  uint16_t count = 0;
  uint16_t size = 0;
  uint16_t i;
  for (i = 0; i < userDataCount && userData[i].user != NULL; i++) {
    uint16_t oh = userData[i].count * OVERHEAD_IN_BYTES;
    uint16_t sz = userData[i].size - oh;
    count += userData[i].count;
    size += sz;
    reportPrint(port, " %4d %d+%d=%d %s\n", " %d %d+%d=%d %s",
                userData[i].count,
                oh, sz, oh + sz,
                userData[i].user);
  }
  uint16_t total = count * OVERHEAD_IN_BYTES + size;
  reportPrint(port, " %4d %d+%d=%d total\n", " %d %d+%d=%d total",
              count, count * OVERHEAD_IN_BYTES, size, total);
  assert(total == WORDS_TO_BYTES(heapPointer - emHeapBase));
}

void emPrintBuffers(uint8_t port, const BufferMarker *markers)
{
  UserData data[20];
  userData = data;
  userDataCount = COUNTOF(data);
  emReclaimUnusedBuffers(markers);
  reportUsage(port);
}

#ifdef EMBER_TEST

// Consistency check.  The main purpose of this is to detect buffer overruns,
// so we check the next buffer, not this one.  That way an error gets signalled
// for the offending buffer and not for its innocent victim.

static bool nextBufferIsOkay(uint16_t *bufferPointer)
{
  uint16_t i;

  bufferPointer = nextBuffer(bufferPointer);

  if (heapPointer <= bufferPointer) {
    return true;
  }

  if (! ((newLocation(bufferPointer) == 0
          || newLocation(bufferPointer) == compressPointer(bufferPointer))
         && ((getDataSize(bufferPointer)
              < ((uint8_t *) heapPointer - (uint8_t *) emHeapBase))
             || isIndirect(bufferPointer)))) {
    return false;
  }

  for (i = 0; i < NUM_LINKS; i++) {
    if (! isHeapPointer(expandPointer(bufferPointer[LINK0_INDEX + i]))) {
      return false;
    }
  }

  return true;
}

//----------------------------------------------------------------
// Keeping track of who uses how much of the heap.

#define bufferUsage(x) do { x } while (0)

// The way to use this is run a simulator trace under GDB.  Print out
// gcCount at the time of interest.  Then set printUsageGcCount to that
// value and restart the trace.  The buffer usage at the previous buffer
// reclamation will be printed out, as will every subsequent allocation.

static uint32_t gcCount = 0;
static uint32_t printUsageGcCount = -1;

bool emTraceBufferUsage = false;
UserData simUserData[1000];

#if 0

#include <execinfo.h>
#include <dlfcn.h>

static void printBacktrace(void)
{
  void *callstack[6];
  int frames = backtrace(callstack, 6);
  int i;
  for (i = 2; i < frames; i++) {
    Dl_info info;
    assert(dladdr(callstack[i], &info) != 0);
    if (i == 2) {
      assert(strcmp(info.dli_sname, "emReallyAllocateBuffer") == 0);
    } else {
      fprintf(stderr, " > %s", info.dli_sname);
    }
  }
  fprintf(stderr, "\n");
}

#endif  // if 0

static void noteAllocation(uint16_t bytes)
{
  if (emTraceBufferUsage) {
    fprintf(stderr, "allocating %d+%d=%d bytes\n",
            OVERHEAD_IN_BYTES, bytes, OVERHEAD_IN_BYTES + bytes);
    //printBacktrace();
  }
}

#else   // ifdef(EMBER_TEST)
#define bufferUsage(x) do {} while (0)
#endif  // ifdef(EMBER_TEST)

//----------------------------------------------------------------

// The stack/done/isTraced mechanism would be unnecessary if buffers had
// only one link.  We want two to allow queues of header+payload.
//
// It is unlikely that any actual heap will fill up the untraced
// stack.  Doing so requires a long chain through the second link
// field with each buffer having both first and second links.

#define UNTRACED_STACK_SIZE 16

enum {
  NOT_TRACING = 0,
  MARK_LIVE,
  UPDATE
};

static uint8_t tracePhase;

// For roots we walk down the link0 and link1 fields before doing the
// full tracing.  This greatly reduces the number of times we have to
// walk through the heap for the typical case where a root contains a
// list or queue of buffers.

void emMarkBuffer(Buffer *root)
{
  if (tracePhase == MARK_LIVE) {
    uint16_t *object = expandPointer(*root);
    uint8_t linkIndex;
    if (object != NULL) {
      for (linkIndex = LINK0_INDEX;
           linkIndex < LINK0_INDEX + NUM_LINKS;
           linkIndex++) {
        uint16_t *linkedObject = object;
        do {
          bufferTest(assert(nextBufferIsOkay(linkedObject)););
          noteUse(linkedObject);
          setLive(linkedObject);
          linkedObject = expandPointer(linkedObject[linkIndex]);
        } while (linkedObject != NULL
                 && ! isLive(linkedObject));
      }
    }
  } else if (tracePhase == UPDATE) {
    uint16_t *object;
    ATOMIC(
           object = expandPointerNoCheck(*root);
           if (object != NULL) {
             assert(! isOnPhyFreelist(object));
             *root = newLocation(object);
           }
           );
  }
}

void emReclaimUnusedBuffers(const BufferMarker *markers)
{
#ifdef EMBER_STACK_COBRA
  // No need for this function on Cobra, so there's no need for a scratchpad
  uint8_t scratchpad[1];
#else
  uint8_t scratchpad[1200];
#endif // C8051_COBRA
  emReclaimUnusedBuffersAndAmalgamate(markers,
                                      scratchpad,
                                      sizeof(scratchpad));
}

//------------------------------------------------------------------------------
// Buffer amalgamation.

// Utility - copy the buffer from its old location to its new location.

static void moveToNewLocation(uint16_t *old, uint16_t *new)
{
  uint16_t size = WORDS_TO_BYTES(getSizeInWords(old));
  clearFlags(old);
  DEBUG("[copy %04X -> %04X]\n",
        compressPointer(old), compressPointer(new));
  if (isOnPhyFreelist(old)) {
    ATOMIC(
           uint8_t i;
           Buffer oldBuffer = compressPointer(old);
           Buffer newBuffer = compressPointer(new);
           MEMMOVE(new, old, size);
           for (i = 0; i < phyFreelistCount; i++) {
             if (phyFreelist[i] == oldBuffer) {
               phyFreelist[i] = newBuffer;
             }
           }
           );
  } else {
    MEMMOVE(new, old, size);
  }
}

// Advance finger over any buffers that are no longer live.

static uint16_t *skipOverNotLive(uint16_t *finger, uint16_t *heapEnd)
{
  while (finger < heapEnd
         && ! isLive(finger)) {
    finger = nextBuffer(finger);
  }
  return finger;
}

static Buffer *amalgamateQueue;

static bool noPayloadLinks(Buffer *queue)
{
  Buffer finger = emBufferQueueHead(queue);
  while (finger != NULL_BUFFER) {
    if (emGetPayloadLink(finger) != NULL_BUFFER
        || isIndirect(expandPointer(finger))) {
      return false;
    }
    finger = emBufferQueueNext(queue, finger);
  }
  return true;
}

bool emMarkAmalgamateQueue(Buffer *queue)
{
  if (tracePhase == MARK_LIVE
      && amalgamateQueue == NULL                // first come, first served
      && 1 < emBufferQueueLength(queue)
      && noPayloadLinks(queue)) {
    amalgamateQueue = queue;
    return true;
  }
  emMarkBuffer(queue);
  return false;
}

// If amalgamateQueue contains 2 or more buffers, the buffer at the head of
// the queue (amalgamateHead) is merged with the second buffer in the queue
// (amalgamateNext). The contents of amalgamateNext are temporarily moved to a
// scratchpad while the heap is being compacted. amalgamateHead then increases
// in size to provide a new home for the contents on the scratchpad.
//
// Usually amalgamateHead will come first on the heap because it was allocated
// before amalgamateNext. Therefore to make amalgamateHead bigger, any buffers
// between amalgamateHead and amalgamateNext must be moved forward on the heap
// during compaction. Normally buffers only move down on the heap during
// compaction.
//
// Before: Q1 A B Q2 C X D
// After:  Q1+Q2 A B C D
//
// Once the compaction is complete, the contents of amalgamateNext are moved
// from the scratchpad to the space at the end of amalgamateHead.
//
// Limitations:
// - The scratchpad must be large enough to hold all of amalgamateNext.
// - There must be no other references to the buffers in amalgamateQueue.

void emReclaimUnusedBuffersAndAmalgamate(const BufferMarker *markers,
                                         uint8_t* scratchpad,
                                         uint16_t scratchpadSize)
{
  uint16_t *finger;
  uint16_t *heap;
  uint16_t *heapEnd;
  uint16_t *liveHeapEnd;
  Buffer phyToMacQueueTemp;
  bool done;

  emMallocFreeList = NULL_BUFFER;
  amalgamateQueue = NULL;

  ATOMIC(
    tracePhase = MARK_LIVE;
    phyToMacQueueTemp = phyToMacQueue;
    phyToMacQueue = NULL_BUFFER;
    heapEnd = heapPointer;
  );

  bufferUsage(gcCount += 1;
              if (gcCount == printUsageGcCount) {
                emTraceBufferUsage = true;
                userData = simUserData;
                userDataCount = COUNTOF(simUserData);
              });
  MEMSET(userData, 0, userDataCount * sizeof(UserData));

  // trace the roots, including the PHY->MAC queue

  bufferUsage(emBufferUsage("PHY->MAC queue"););
  emMarkBuffer(&phyToMacQueueTemp);

  bufferUsage(emBufferUsage("PHY freelist"););
   
  uint8_t phyFreelistCountTemp;
  uint8_t i;
  // The receive ISR can change these two values, but not their total.
  ATOMIC(phyFreelistCountTemp = phyFreelistCount + phyFreelistQueuedCount;)
  for (i = 0; i < phyFreelistCountTemp; i++) {
    emMarkBuffer(phyFreelist + i);
  }
  
  bufferUsage(emBufferUsage("marker array"););
  if (markers != NULL) {
    uint8_t i;
    for (i = 0; markers[i] != NULL; i++) {
      markers[i]();
    }
  }

  // trace the links
  bufferUsage(emBufferUsage("tracing"););
  do {
    uint16_t *stack[UNTRACED_STACK_SIZE]; // live buffers that need to be traced
    uint8_t top = 0;                      // top of the stack

    finger = emHeapBase;                // walk the heap finding live buffers
    done = true;                        // set to false if an untraced buffer
                                        //   could not be put on the stack

    while(finger < heapEnd
          || 0 < top) {

      uint16_t *next;

      if (0 < top) {
        top -= 1;
        next = stack[top];
      } else {
        next = finger;
        finger = nextBuffer(finger);
      }

      if (isLive(next)
          && ! isTraced(next)) {
        setTraced(next);
        uint8_t i;

        for (i = LINK0_INDEX; i <= LINK1_INDEX; i++) {
          uint16_t *link = expandPointer(next[i]);

          if (link != NULL
              && ! isLive(link)) {
            setLive(link);
            noteUse(link);
            if (top < UNTRACED_STACK_SIZE) {
              stack[top] = link;
              top += 1;
            } else {
              done = false;
            }
          }
        }
      }
    }
  } while (! done);

  // decide what to amalgamate

  if (amalgamateQueue != NULL) {
    // Disable amalgamation if any buffers in the queue were already marked.
    // Because queues are circular, we only need to check one.
    if (isLive(expandPointer(*amalgamateQueue))) {
      amalgamateQueue = NULL;
    } else {
      emMarkBuffer(amalgamateQueue);
    }
  }

  Buffer amalgamateHead = NULL_BUFFER;
  uint16_t amalgamateHeadLength = 0;
  uint16_t amalgamateNextLength = 0;

  if (amalgamateQueue != NULL) {
    amalgamateHead = emGetQueueLink(*amalgamateQueue);
    Buffer amalgamateNext = emGetQueueLink(amalgamateHead);
    amalgamateHeadLength = emGetBufferLength(amalgamateHead);
    amalgamateNextLength = emGetBufferLength(amalgamateNext);
    if (amalgamateNextLength <= scratchpadSize) {
      MEMMOVE(scratchpad,
              emGetBufferPointer(amalgamateNext),
              emGetBufferLength(amalgamateNext));
      clearFlags(expandPointer(amalgamateNext));
      emBufferQueueRemove(amalgamateQueue, amalgamateNext);
    } else {
      amalgamateHead = NULL_BUFFER;
    }
    DEBUG("head = %04X (%u)\n", amalgamateHead, amalgamateHeadLength);
    DEBUG("next = %04X (%u)\n", amalgamateNext, amalgamateNextLength);
  }

  // set new locations

  heap = emHeapBase;

  bufferTest(MEMSET(emNewMallocedContents,
                    0,
                    (emHeapLimit - emHeapBase) * sizeof(uint16_t *)););

  liveHeapEnd = emHeapBase;

  for (finger = emHeapBase; finger < heapEnd; finger = nextBuffer(finger)) {
    uint16_t sizeInWords = getSizeInWords(finger);
    DEBUG("sizeInWords for %04X is %u\n", compressPointer(finger), sizeInWords);
    if (compressPointer(finger) == amalgamateHead) {
      sizeInWords = (OVERHEAD_IN_WORDS
                     + BYTES_TO_WORDS(amalgamateHeadLength
                                      + amalgamateNextLength));
      DEBUG("increasing sizeInWords to %u\n", sizeInWords);
    }
    if (isLive(finger)) {
      setNewLocation(finger, compressPointer(heap));
      bufferTest(emNewMallocedContents[heap - emHeapBase]
                 = (emBuffersUseMalloc
                    ? emMallocedContents[finger - emHeapBase]
                    : NOT_NULL););
      DEBUG("[%04X -> %04X]\n", compressPointer(finger), compressPointer(heap));
      heap += sizeInWords;
      liveHeapEnd = finger + sizeInWords;
    } else {
      bufferTest(testFree(emMallocedContents[finger - emHeapBase]);
                 emMallocedContents[finger - emHeapBase] = NULL;);
      if(isIndirect(finger)) {
        void* objectRef = *((void **) (finger + INDIRECT_BUFFER_OBJ_REF_INDEX));
        if(objectRef != NULL) {
          emberFreeMemoryForPacketHandler(objectRef);
        }
      }
    }
  }

  // When copying, don't scan past end of live objects.  Amalgamation may
  // copy live objects forward, overwriting dead objects at the end of the heap.
  heapEnd = liveHeapEnd;

#ifdef EMBER_TEST
  // This only works in simulation.  On real hardware an incoming message ISR
  // that allocates a buffer will break this.
  if (newHeapBase != NULL) {
    uint16_t liveSize = liveHeapEnd - emHeapBase;
    MEMCOPY(newHeapBase, emHeapBase, liveSize * sizeof(uint16_t));
    heap = newHeapBase + (heap - emHeapBase);
    heapEnd = newHeapBase + liveSize;
    emHeapBase = newHeapBase;
    emHeapLimit = newHeapLimit;
    newHeapBase = NULL;
    newHeapLimit = NULL;
    size_t size = (emHeapLimit - emHeapBase) * sizeof(uint16_t *);
    emMallocedContents = (uint16_t **) realloc(emMallocedContents, size);
    emNewMallocedContents = (uint16_t **) realloc(emNewMallocedContents, size);
  }
#endif

  // update the roots

  tracePhase = UPDATE;
  emMarkBuffer(&phyToMacQueueTemp);
  emMarkBuffer(&amalgamateHead);

  if (markers != NULL) {
    uint8_t i;
    for (i = 0; markers[i] != NULL; i++) {
      markers[i]();
    }
  }

  // update the links

  for (finger = emHeapBase; finger < heapEnd; finger = nextBuffer(finger)) {
    uint8_t i;
    for (i = LINK0_INDEX; i < LINK0_INDEX + NUM_LINKS; i++) {
      uint16_t *old = expandPointerNoCheck(finger[i]);
      if (old != NULL) {
        finger[i] = newLocation(old);
      }
    }
  }

  // do the actual move

  for (finger = emHeapBase; finger < heapEnd; ) {
    uint16_t *next = nextBuffer(finger);
    if (isLive(finger)) {
      uint16_t *dest = expandPointerNoCheck(newLocation(finger));
      if (dest == finger) {
        DEBUG("[copy %04X -> %04X]\n",
              compressPointer(finger),
              compressPointer(finger));
        clearFlags(finger);
      } else if (dest < finger) {
        moveToNewLocation(finger, dest);
      } else {
        // We are moving buffers forward, which means they must get moved in
        // in reverse order.  Scan forward to find the end of the block of
        // buffers that move forward.

        uint16_t *forwardBlockStart = finger;
        while (next < heapEnd
               && next < expandPointerNoCheck(newLocation(next))) {
          finger = next;
          next = skipOverNotLive(nextBuffer(next), heapEnd);
        }
        DEBUG("[scan forward to %04X -> %04X]\n",
              compressPointer(finger), compressPointer(next));

        // At this point next == nextBuffer(finger), next moves
        // backward, and finger moves forward.  After the forward
        // moves are done, the outer loop will continue with next.

        // Do the forward moves, starting with finger and working backwards
        // to forwardBlockStart.
        uint16_t *lastMoved;
        do {
          moveToNewLocation(finger,
                            expandPointerNoCheck(newLocation(finger)));
          lastMoved = finger;
          // Scan forward to find the buffer before lastMoved.
          uint16_t *scanNext = forwardBlockStart;
          while (scanNext < lastMoved) {
            finger = scanNext;
            scanNext = skipOverNotLive(nextBuffer(scanNext), heapEnd);
          }
          DEBUG("[scan forward2 to %04X -> %04X]\n",
                compressPointer(finger), compressPointer(scanNext));
          // Quit when we have worked back to the start of the block.
        } while (lastMoved != forwardBlockStart);
      }
    }
    finger = next;
  }

  // The receive ISR may have been adding to the PHY_TO_MAC_QUEUE while
  // we were busy.  This moves any new buffers to the new heap base and
  // puts them on phyToMacQueue.  If PHY_TO_MAC_QUEUE is empty we are done.

//  fprintf(stderr, "[start]\n");

  // During this next phase we are operating partly in the old heap
  // and partly in the new heap, so the flag checking doesn't work.
  bufferTest(inGcCleanup = true;);

  do {
#ifdef EMBER_TEST
    (*emCompactionInterruptProc)();
#endif // EMBER_TEST
    ATOMIC(
      for ( ; phyFreelistQueuedCount; phyFreelistQueuedCount--) {
        uint8_t index = phyFreelistCount + phyFreelistQueuedCount - 1;
        Buffer b = phyFreelist[index];
        phyFreelist[index] = NULL_BUFFER;
        clearPhyFreelistBit(expandPointerNoCheck(b));
        emBufferQueueAdd(&phyToMacQueueTemp, b);
      }
      phyFreelistQueuedCount = 0;
      finger = expandPointer(emBufferQueueRemoveHead(&phyToMacQueue));
      if (finger == NULL) {
//        fprintf(stderr, "[done]\n");
        phyToMacQueue = phyToMacQueueTemp;
        heapPointer = heap;
        tracePhase = NOT_TRACING;
      }
      )
    if (finger != NULL) {
      uint16_t size = getSizeInWords(finger);
      uint16_t newTail = compressPointer(heap);
      bufferTest(emNewMallocedContents[heap - emHeapBase]
                 = (emBuffersUseMalloc
                    ? emMallocedContents[finger - emHeapBase]
                    : NOT_NULL););
      MEMMOVE(heap, finger, WORDS_TO_BYTES(size));
      // fprintf(stderr, "[ISR copy %04X -> %04X]\n",
      //         compressPointer(finger), compressPointer(heap));
      emBufferQueueAdd(&phyToMacQueueTemp, newTail);
      heap += size;
    }
  } while (finger != NULL);

  bufferTest(MEMMOVE(emMallocedContents,
                     emNewMallocedContents,
                     (emHeapLimit - emHeapBase) * sizeof(uint16_t *));
             inGcCleanup = false;);

  // finish the amalgamation

  if (amalgamateHead != NULL_BUFFER) {
    Buffer b = amalgamateHead;
    DEBUG("head = %04X\n", b);
    finger = expandPointer(b);
    uint16_t newSize = amalgamateHeadLength + amalgamateNextLength;
    bufferTest(uint16_t *newMalloc = testMalloc(newSize);
               if (emBuffersUseMalloc) {
                 MEMMOVE(newMalloc,
                         emMallocedContents[finger - emHeapBase],
                         emGetBufferLength(b));
               }
               testFree(emMallocedContents[finger - emHeapBase]);
               emMallocedContents[finger - emHeapBase] = newMalloc;);
    DEBUG("[adjusting size of %04X from %u to %u]\n",
          compressPointer(finger), sizeWord(finger), newSize);
    sizeWord(finger) = newSize;
    uint8_t *contents = emGetBufferPointer(b);
    // Buffer is old head, but bigger. Old next is on the scratchpad. Move
    // scratchpad to end of buffer.
    MEMMOVE(contents + amalgamateHeadLength,
            scratchpad,
            amalgamateNextLength);
  }

  bufferUsage(if (emTraceBufferUsage) { reportUsage(0xFF); });
}

static uint16_t bufferTracker = 0;

void emResetBufferTracking(void)
{
  bufferTracker = 0;
}

uint16_t emGetTrackedBufferBytes(void)
{
  return bufferTracker;
}

bool emIsValidBuffer(Buffer buffer)
{
  uint16_t *bufferPointer = expandPointer(buffer);
  return isHeapPointer(bufferPointer);
}

//----------------------------------------------------------------
// External Interface

uint8_t *emGetBufferPointer(Buffer buffer)
{
  if (buffer == NULL_BUFFER) {
    return NULL;
  }

  assert(emIsValidBuffer(buffer));
  uint16_t *bufferPointer = expandPointer(buffer);

  if (isIndirect(bufferPointer)) {
    return *((uint8_t **) (bufferPointer + INDIRECT_BUFFER_POINTER_INDEX));

#ifdef EMBER_TEST
  } else if (emBuffersUseMalloc) {
    return (uint8_t *) emMallocedContents[bufferPointer - emHeapBase];
#endif
  } else {
    return (uint8_t *) (bufferPointer + OVERHEAD_IN_WORDS);
  }
}

uint16_t emGetBufferLength(Buffer buffer)
{
  if (buffer == NULL_BUFFER) {
    return 0;
  }

  uint16_t *bufferPointer = expandPointer(buffer);
  assert(isHeapPointer(bufferPointer));
  if (isIndirect(bufferPointer)) {
    return bufferPointer[INDIRECT_BUFFER_LENGTH_INDEX];
  } else {
    return getDataSize(bufferPointer);
  }
}

void emSetBufferLength(Buffer buffer, uint16_t newLength)
{
  uint16_t *bufferPointer = expandPointer(buffer);
  uint16_t oldLength;
  uint16_t *next;

  assert(isHeapPointer(bufferPointer));
  oldLength = emGetBufferLength(buffer);
  next = nextBuffer(bufferPointer);

  if (newLength == oldLength) {
    return;
  }

  assert(newLength < oldLength);

  if (isIndirect(bufferPointer)) {
    bufferPointer[INDIRECT_BUFFER_LENGTH_INDEX] = newLength;
  } else {
    // If there is enough space we turn the leftover space into a new,
    // unreferenced buffer.  If not, we fill it with the UNUSED_MEMORY
    // marker.
    uint16_t *leftoverPointer;
    uint16_t leftoverBytes;

    sizeWord(bufferPointer) = newLength;

    leftoverPointer = bufferPointer + getSizeInWords(bufferPointer);
    leftoverBytes = ((uint8_t *) next) - ((uint8_t *) leftoverPointer);

    ATOMIC(
      if (next == heapPointer) {
        heapPointer = leftoverPointer;
        leftoverBytes = 0;
      }
           )

    if (leftoverBytes < OVERHEAD_IN_BYTES) {
      MEMSET(leftoverPointer, (uint8_t)UNUSED_MEMORY, leftoverBytes);
    } else {
      MEMSET(leftoverPointer, 0, OVERHEAD_IN_BYTES);
      sizeWord(leftoverPointer) = leftoverBytes - OVERHEAD_IN_BYTES;
    }
  }
}

void emSetBufferLengthFromEnd(Buffer buffer, uint16_t newLength)
{
  uint16_t *bufferPointer = expandPointer(buffer);
  uint16_t oldLength = emGetBufferLength(buffer);
  uint16_t remove = oldLength - newLength;

  if (newLength == oldLength) {
    return;
  }
  assert(newLength < oldLength);

  if (isIndirect(bufferPointer)) {
    *((const uint8_t **) (bufferPointer + INDIRECT_BUFFER_POINTER_INDEX))
      += remove;
    bufferPointer[INDIRECT_BUFFER_LENGTH_INDEX] = newLength;
  } else {
    uint8_t *contents = emGetBufferPointer(buffer);
    MEMMOVE(contents, contents + remove, newLength);
    emSetBufferLength(buffer, newLength);
  }
}

Buffer emGetBufferLink(Buffer buffer, uint8_t i)
{
  uint16_t *bufferPointer = expandPointer(buffer);
  assert(isHeapPointer(bufferPointer));
  return (Buffer) bufferPointer[LINK0_INDEX + i];
}

void emSetBufferLink(Buffer buffer, uint8_t i, Buffer newLink)
{
  uint16_t *bufferPointer = expandPointer(buffer);
  assert(isHeapPointer(bufferPointer));
  assert(isHeapPointer(expandPointer(newLink)));
  bufferPointer[LINK0_INDEX + i] = newLink;
}

uint16_t emBufferChainByteLength(Buffer buffer, uint8_t link)
{
  uint16_t length = 0;

  while (buffer != NULL_BUFFER) {
    length += emGetBufferLength(buffer);
    buffer = emGetBufferLink(buffer, link);
  }

  return length;
}

static uint16_t *asynchronousHeapLimit = NULL;

bool emSetReservedBufferSpace(uint16_t dataSizeInBytes)
{
  uint16_t totalWords = BYTES_TO_WORDS(dataSizeInBytes);
  assert(asynchronousHeapLimit == NULL);

  if (totalWords < (uint16_t)(emHeapLimit - heapPointer)) {
    asynchronousHeapLimit = emHeapLimit - totalWords;
    return true;
  }
  return false;
}

void emEndBufferSpaceReservation(void)
{
  asynchronousHeapLimit = NULL;
}

// The receive ISR calls this from interrupt context, so we need the ATOMIC
// to protect emHeapPointer.

Buffer emReallyAllocateBuffer(uint16_t dataSizeInBytes, bool asynchronous)
{
  uint16_t sizeInWords = (OVERHEAD_IN_WORDS + BYTES_TO_WORDS(dataSizeInBytes));
  uint16_t *result = NULL;
  uint16_t *heapLimit = emHeapLimit;

  assert(dataSizeInBytes <= SIZE_MASK);

  bufferTest(
    allocationCount += 1;
    if (allocationCount == allocationToFail) {
      return NULL_BUFFER;
    }
  );

  ATOMIC(
    uint16_t available;
    if (asynchronousHeapLimit == NULL) {
      available = heapLimit - heapPointer;
    } else if (asynchronous) {
      available = asynchronousHeapLimit - heapPointer;
    } else {
      available = heapLimit - asynchronousHeapLimit;
      assert(available);        // check that heap reservation was large enough
    }

    if (sizeInWords <= available) {
      result = heapPointer;

      heapPointer += sizeInWords;

      if (asynchronousHeapLimit != NULL
          && ! asynchronous) {
        asynchronousHeapLimit += sizeInWords;
      }

      result[SIZE_INDEX] = dataSizeInBytes;
      result[LINK0_INDEX] = NULL_BUFFER;
      result[LINK1_INDEX] = NULL_BUFFER;
      // anything other than the UNUSED_MEMORY is OK
      result[NEW_LOCATION_INDEX] = (~ UNUSED_MEMORY) & 0xFFFF;

      bufferTracker += dataSizeInBytes + OVERHEAD_IN_BYTES;
    }
  )

    bufferTest(
      if (result != NULL) {
        emMallocedContents[result - emHeapBase] = testMalloc(dataSizeInBytes);
      });
  bufferUsage(noteAllocation(dataSizeInBytes););
#ifndef EMBER_SCRIPTED_TEST
  if (result == NULL) {
    emApiCounterHandler(EMBER_COUNTER_BUFFER_ALLOCATION_FAIL, 1);
  }
#endif
  return compressPointer(result);
}

Buffer emAllocateIndirectBuffer(uint8_t *contents,
                                void    *freePtr,
                                uint16_t length)
{
  Buffer buffer =
    emReallyAllocateBuffer(INDIRECT_BUFFER_DATA_SIZE_IN_BYTES, false);
  uint16_t *bufferPointer = expandPointer(buffer);
  bufferPointer[INDIRECT_BUFFER_LENGTH_INDEX] = length;
  *((uint8_t **) (bufferPointer + INDIRECT_BUFFER_POINTER_INDEX))
    = contents;
  bufferPointer[NEW_LOCATION_INDEX] |= INDIRECT_BIT;
  *((void **) (bufferPointer + INDIRECT_BUFFER_OBJ_REF_INDEX)) = freePtr;
  return buffer;
}

void* emberGetObjectRefFromBuffer(Buffer b)
{
  uint16_t *bufferPointer = expandPointer(b);

  if(!isIndirect(bufferPointer)) {
    return NULL;
  }

  return *((void **) (bufferPointer + INDIRECT_BUFFER_OBJ_REF_INDEX));
}

Buffer emReallyFillBuffer(const uint8_t *contents, uint16_t length, bool async)
{
  Buffer buffer = emReallyAllocateBuffer(length, async);

  if (contents != NULL
      && buffer != NULL_BUFFER) {
    MEMMOVE(emGetBufferPointer(buffer),
            contents,
            length);
  }
  return buffer;
}

void emCopyFromLinkedBuffers(uint8_t *to, Buffer from, uint16_t count)
{
  while (0 < count) {
    uint16_t next = emGetBufferLength(from);
    if (count < next) {
      next = count;
    }
    MEMCOPY(to, emGetBufferPointer(from), next);
    to += next;
    from = emGetPayloadLink(from);
    count -= next;
  }
}

//----------------------------------------------------------------

bool emPhyToMacQueueAddIsr(uint8_t *packet, uint8_t length, uint8_t padding)
{
  // Not currently true, but we don't put anything on the freelist yet.
  // assert(length + padding == PHY_BUFFER_SIZE);
  EmberMessageBuffer rxPayload = NULL_BUFFER;
  DECLARE_INTERRUPT_STATE;
  DISABLE_INTERRUPTS();
  if (0 < phyFreelistCount) {
    phyFreelistCount -= 1;
    rxPayload = phyFreelist[phyFreelistCount];
    if (tracePhase == NOT_TRACING) {
      phyFreelist[phyFreelistCount] = NULL_BUFFER;
      clearPhyFreelistBit(expandPointer(rxPayload));
    } else {
      // We're in the middle of a GC, so leave the buffer on the freelist but 
      // mark that it needs to be moved to the queue.
      phyFreelistQueuedCount += 1;
    }
  } else {
    rxPayload = emAllocateAsyncBuffer(length + padding);
    if (rxPayload == NULL_BUFFER) {
      RESTORE_INTERRUPTS();
      return false;
    }
  }
  MEMCOPY(emGetBufferPointer(rxPayload), packet, length);
  MEMSET(emGetBufferPointer(rxPayload) + length, 0, padding); // safety
  emBufferQueueAdd(&phyToMacQueue, rxPayload);
  RESTORE_INTERRUPTS();
  return true;
}

void emPhyToMacQueueFreelistAdd(Buffer buffer)
{
  uint16_t *bufferPointer = expandPointer(buffer);
//  assert(emGetBufferLength(buffer) == PHY_BUFFER_SIZE);
  assert(! isOnPhyFreelist(bufferPointer));
  bufferPointer[LINK0_INDEX] = NULL_BUFFER;
  bufferPointer[LINK1_INDEX] = NULL_BUFFER;
  ATOMIC(
         if (phyFreelistCount < PHY_BUFFER_FREELIST_SIZE) {
           phyFreelist[phyFreelistCount] = buffer;
           phyFreelistCount += 1;
           setPhyFreelistBit(bufferPointer);
         }
  );
}

Buffer emPhyToMacQueueRemoveHead(void)
{
  Buffer result;
  ATOMIC(result = emBufferQueueRemoveHead(&phyToMacQueue);)
  return result;
}

bool emPhyToMacQueueIsEmpty(void)
{
  return phyToMacQueue == NULL_BUFFER;
}

void emEmptyPhyToMacQueue(void)
{
  phyToMacQueue = NULL_BUFFER;
}

//----------------------------------------------------------------
// Wrappers for the MessageBuffer interface.

void emHoldMessageBuffer(Buffer buffer EM_BUFFER_FILE_DECL)
{
}

void emReleaseMessageBuffer(Buffer buffer EM_BUFFER_FILE_DECL)
{
}

void emReallyCopyToLinkedBuffers(PGM_P contents,
                                 Buffer buffer,
                                 uint8_t startIndex,
                                 uint8_t length,
                                 uint8_t direction)
{
  uint8_t *bufferPointer = emGetBufferPointer(buffer) + startIndex;
  assert (startIndex + length <= emberMessageBufferLength(buffer));
  if (length == 0) {
    return;
  }

  if (direction == 0) {         // from buffer to RAM
    MEMMOVE((uint8_t *) contents, bufferPointer, length);
  } else if (direction == 1) {  // from RAM to buffer
    MEMMOVE(bufferPointer, (uint8_t *) contents, length);
  } else {                      // from PGM to buffer
    MEMPGMCOPY(bufferPointer, contents, length);
  }
}

EmberStatus emberSetLinkedBuffersLength(Buffer buffer,
                                        uint8_t newLength)
{
  emSetBufferLength(buffer, newLength);
  return EMBER_SUCCESS;
}

uint8_t *emberGetLinkedBuffersPointer(Buffer buffer, uint8_t index)
{
  assert(index < emberMessageBufferLength(buffer));
  return emGetBufferPointer(buffer) + index;
}

uint8_t emberGetLinkedBuffersByte(Buffer buffer, uint8_t index)
{
  return *(emGetBufferPointer(buffer) + index);
}

void emberSetLinkedBuffersByte(Buffer buffer, uint8_t index, uint8_t byte)
{
  *(emGetBufferPointer(buffer) + index) = byte;
}

// Count the number of buffers needed, allocate them, then copy the data over.

Buffer emberCopyLinkedBuffers(Buffer buffer)
{
  return emFillBuffer(emGetBufferPointer(buffer),
                      emGetBufferLength(buffer));
}

Buffer emFillStringBuffer(const uint8_t *contents)
{
  return (contents == NULL
          ? NULL_BUFFER
          : emFillBuffer(contents, strlen((char const *)contents) + 1));
}

#ifdef EMBER_TEST
uint8_t emFreePacketBufferCount(void)
{
  return 0;
}

void printPacketBuffers(Buffer buffer)
{
  uint8_t i;
  uint8_t j = 0;
  uint8_t length = emGetBufferLength(buffer);

  for (i = 0; i < length; i++) {
    fprintf(stderr, "%c%02X",
            (i = 0
             ? '['
             : ' '),
            emGetBufferPointer(buffer)[j]);
  }

  fprintf(stderr, "]\n");
}

void simPrintBytes(char *prefix, uint8_t *bytes, uint16_t count)
{
  uint16_t i;

  fprintf(stderr, "%s", prefix);
  for (i = 0; i < count; i++)
    fprintf(stderr, " %02X", bytes[i]);
  fprintf(stderr, "\n");
}

void simPrintBuffer(char *prefix, Buffer buffer)
{
  simPrintBytes(prefix, emGetBufferPointer(buffer), emGetBufferLength(buffer));
}

#endif // EMBER_TEST

// Miscellaneous functions

uint16_t emberGetLinkedBuffersLowHighInt16u(Buffer buffer,
                                       uint8_t index)
{
  return HIGH_LOW_TO_INT(emberGetLinkedBuffersByte(buffer, index + 1),
                         emberGetLinkedBuffersByte(buffer, index));
}

void emberSetLinkedBuffersLowHighInt16u(Buffer buffer,
                                     uint8_t index,
                                     uint16_t value)
{
  uint8_t temp[2];
  temp[0] = LOW_BYTE(value);
  temp[1] = HIGH_BYTE(value);
  emberCopyToLinkedBuffers(temp, buffer, index, 2);
}

//----------------------------------------------------------------
// Utilities used by the buffer malloc code.

// Given a pointer, return the buffer whose data pointer it is.

Buffer emBufferPointerToBuffer(uint16_t *bufferPointer)
{
#ifdef EMBER_TEST
  if (emBuffersUseMalloc) {
    if (bufferPointer == NULL) {
      return NULL_BUFFER;
    } else {
      uint16_t i;
      uint16_t limit = emHeapLimit - emHeapBase;
      for (i = 0; i < limit; i++) {
        if (emMallocedContents[i] == bufferPointer) {
          expandPointer(i + 1);
          return i + 1;
        }
      }
      return NULL_BUFFER;
    }
  }
#endif
  if (emPointsIntoHeap(bufferPointer)) {
    return compressPointer(bufferPointer - OVERHEAD_IN_WORDS);
  } else {
    return NULL_BUFFER;
  }
}

// Returns the buffer following the given one, which may in fact be
// no buffer at all.

Buffer emFollowingBuffer(Buffer buffer)
{
  uint16_t *bufferPointer = expandPointer(buffer);
  uint16_t *next = bufferPointer + getSizeInWords(bufferPointer);

// This would allow the current buffer to be reclaimed immediately.
// Having this be done as a side effect of finding the following
// buffer would likely lead to obscure bugs.
//
//  ATOMIC(
//   if (next == heapPointer) {
//      heapPointer = bufferPointer;
//      next = NULL;
//   }
//         )

  return compressPointer(next);
}

// Set the length of the first buffer so that it includes the second,
// which must immediately follow the first in the heap.

void emMergeBuffers(Buffer first, Buffer second)
{
  uint16_t *bufferPointer = expandPointer(first);
  uint16_t oldLength = getDataSize(bufferPointer);
  uint16_t addedLength = getSizeInBytes(expandPointer(second));
  uint16_t newLength = (oldLength
                      + (oldLength & 1)
                      + addedLength);
  assert(emFollowingBuffer(first) == second);
  bufferTest(testFree(emMallocedContents[bufferPointer - emHeapBase]);
             emMallocedContents[bufferPointer - emHeapBase]
               = testMalloc(newLength);
             testFree(emMallocedContents[expandPointer(second) - emHeapBase]);
             emMallocedContents[expandPointer(second) - emHeapBase]
               = NULL;);
  sizeWord(bufferPointer) = newLength;
}

// Split 'buffer' in two, leaving the first buffer with 'newLength'
// bytes.  Returns the new buffer, or NULL_BUFFER if there is not
// enough space left over to create a buffer.

Buffer emSplitBuffer(Buffer buffer, uint16_t newLength)
{
  uint16_t *bufferPointer = expandPointer(buffer);
  uint16_t oldLength = getDataSize(bufferPointer);
  uint16_t oldLengthEven = oldLength + (oldLength & 1);
  uint16_t newLengthEven = newLength + (newLength & 1);
  uint16_t leftoverBytes = oldLengthEven - newLengthEven;
  uint16_t *leftoverPointer = (bufferPointer
                             + OVERHEAD_IN_WORDS
                             + BYTES_TO_WORDS(newLength));

  assert(isHeapPointer(bufferPointer)
         && newLength <= oldLength);

  if (newLength == oldLength
      || leftoverBytes <= OVERHEAD_IN_BYTES) {
    return NULL_BUFFER;
  } else {
    sizeWord(bufferPointer) = newLength;
    MEMSET(leftoverPointer, 0, OVERHEAD_IN_BYTES);
    sizeWord(leftoverPointer) = leftoverBytes - OVERHEAD_IN_BYTES;
    bufferTest(testFree(emMallocedContents[bufferPointer - emHeapBase]);
               emMallocedContents[bufferPointer - emHeapBase]
                 = testMalloc(newLength);
               emMallocedContents[leftoverPointer - emHeapBase]
                 = testMalloc(leftoverBytes););
    return compressPointer(leftoverPointer);
  }
}

// A utility for marking a buffer via a pointer to its contents.

void emMarkBufferPointer(void **pointerLoc)
{
  Buffer buffer = emBufferPointerToBuffer(*pointerLoc);
  if (buffer != NULL_BUFFER) {
    uint16_t *bufferPointer = expandPointer(buffer);
    assert(!isIndirect(bufferPointer));
    emMarkBuffer(&buffer);
    if (! emBuffersUseMalloc) {
      // Can't use emGetBufferPointer() to update *pointerLoc because it checks
      // INDIRECT_BIT and this may not have been copied to the new location yet.
      bufferPointer = expandPointerNoCheck(buffer);
      assert(isHeapPointer(bufferPointer));
      *pointerLoc = (uint8_t *) (bufferPointer + OVERHEAD_IN_WORDS);
    }
  }
}
