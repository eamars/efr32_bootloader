/**
 * @file cstartup-common.c
 * @brief Alternative implementation of helper functions for crt0
 * @author Ran Bao
 * @date May, 2017
 */

#include PLATFORM_HEADER
#include "hal/micro/cortexm3/diagnostic.h"
#include "hal/micro/cortexm3/efm32/mpu.h"
#include "hal/micro/micro.h"
#include "hal/micro/cortexm3/memmap.h"
#include "hal/micro/cortexm3/cstartup-common.h"
#include "hal/micro/cortexm3/internal-storage.h"

#include "stack/include/ember-types.h"
#include "hal/micro/bootloader-interface.h"
#include "em_device.h"
#include "em_rmu.h"

// TODO: comments in this file relate to the em3xx instead of efr32. Should
// be cleaned up.

// Pull in the SOFTWARE_VERSION and EMBER_BUILD_NUMBER from the stack
#include "stack/config/config.h"

// Define the CUSTOMER_APPLICATION_VERSION if it wasn't set
#ifndef CUSTOMER_APPLICATION_VERSION
#define CUSTOMER_APPLICATION_VERSION 0
#endif
// Define the CUSTOMER_BOOTLOADER_VERSION if it wasn't set
#ifndef CUSTOMER_BOOTLOADER_VERSION
#define CUSTOMER_BOOTLOADER_VERSION 0
#endif


#ifndef CSTACK_SIZE
#ifdef RTOS
// The RTOS will handle the actual CSTACK sizing per-task, but we must
    // still allocate some space for startup and exceptions.
    #define CSTACK_SIZE (128)  // *4 = 512 bytes
#else
#if (! defined(EMBER_STACK_IP))
// Pro Stack

// Right now we define the stack size to be for the worst case scenario,
// ECC.  The ECC 163k1 library  and the ECC 283k1 Library both use the stack
// for calculations. Empirically I have seen it use as much as 1900 bytes
// for the 'key bit generate' operation.
// So we add a 25% buffer: 1900 * 1.25 = 2375
#define CSTACK_SIZE  (600)  // *4 = 2400 bytes

#else
// IP Stack
      #define CSTACK_SIZE (950) // *4 = 3800 bytes
#endif // !EMBER_STACK_IP
#endif
#endif
VAR_AT_SEGMENT(NO_STRIPPING uint32_t cstackMemory[CSTACK_SIZE], __CSTACK__);

#ifndef HTOL_EM3XX
// Create an array to hold space for the guard region. Do not actually use this
// array in code as we will move the guard region around programatically. This
// is only here so that the linker takes into account the size of the guard
// region when configuring the RAM.
ALIGNMENT(HEAP_GUARD_REGION_SIZE_BYTES)
VAR_AT_SEGMENT(NO_STRIPPING uint8_t guardRegionPlaceHolder[HEAP_GUARD_REGION_SIZE_BYTES], __GUARD_REGION__);
#endif

// Reset cause and crash info live in a special RAM segment that is
// not modified during startup.  This segment is overlayed on top of the
// bottom of the cstack.
VAR_AT_SEGMENT(NO_STRIPPING HalResetInfoType halResetInfo, __RESETINFO__);

// If space is needed in the flash for data storage like for the local storage
// bootloader then create an array here to hold a place for this data.
#if INTERNAL_STORAGE_SIZE_B > 0
// Define the storage region as an uninitialized array in the
  // __INTERNAL_STORAGE__ region which the linker knows how to place.
  VAR_AT_SEGMENT(NO_STRIPPING uint8_t internalStorage[INTERNAL_STORAGE_SIZE_B], __INTERNAL_STORAGE__);
#endif

// halInternalClassifyReset() records the cause of the last reset and any
// assert information here. If the last reset was not due to an assert,
// the saved assert filename and line number will be NULL and 0 respectively.
static uint16_t savedResetCause;
static HalAssertInfoType savedAssertInfo;

void halInternalClassifyReset(void)
{
  // Table used to convert from RESET_EVENT register bits to reset types
  static const uint16_t resetEventTable[] = {
      RESET_POWERON_HV,                  // bit  0: PORST
      RESET_UNKNOWN_UNKNOWN,             // bit  1: RESERVED
      RESET_BROWNOUT_AVDD,               // bit  2: AVDDBOD
      RESET_BROWNOUT_DVDD,               // bit  3: DVDDBOD
      RESET_BROWNOUT_DEC,                // bit  4: DECBOD
      RESET_UNKNOWN_UNKNOWN,             // bit  5: RESERVED
      RESET_UNKNOWN_UNKNOWN,             // bit  6: RESERVED
      RESET_UNKNOWN_UNKNOWN,             // bit  7: RESERVED
      RESET_EXTERNAL_PIN,                // bit  8: EXTRST
      RESET_FATAL_LOCKUP,                // bit  9: LOCKUPRST
      RESET_SOFTWARE,                    // bit 10: SYSREQRST
      RESET_WATCHDOG_EXPIRED,            // bit 11: WDOGRST
      RESET_UNKNOWN_UNKNOWN,             // bit 12: RESERVED
      RESET_UNKNOWN_UNKNOWN,             // bit 13: RESERVED
      RESET_UNKNOWN_UNKNOWN,             // bit 14: RESERVED
      RESET_UNKNOWN_UNKNOWN,             // bit 15: RESERVED
      RESET_SOFTWARE_EM4,                // bit 16: EM4RST
  };

  uint32_t resetEvent = RMU_ResetCauseGet();
  RMU_ResetCauseClear();
  uint16_t cause = RESET_UNKNOWN;
  uint16_t i;

  for (i = 0; i < sizeof(resetEventTable)/sizeof(resetEventTable[0]); i++) {
    if (resetEvent & (1 << i)) {
      cause = resetEventTable[i];
      break;
    }
  }

  if (cause == RESET_SOFTWARE) {
    if((halResetInfo.crash.resetSignature == RESET_VALID_SIGNATURE) &&
       (RESET_BASE_TYPE(halResetInfo.crash.resetReason) < NUM_RESET_BASE_TYPES)) {
      // The extended reset cause is recovered from RAM
      // This can be trusted because the hardware reset event was software
      //  and additionally because the signature is valid
      savedResetCause = halResetInfo.crash.resetReason;
    } else {
      savedResetCause = RESET_SOFTWARE_UNKNOWN;
    }
    // mark the signature as invalid
    halResetInfo.crash.resetSignature = RESET_INVALID_SIGNATURE;
  } else if (    (cause == RESET_BOOTLOADER_DEEPSLEEP)
                 && (halResetInfo.crash.resetSignature == RESET_VALID_SIGNATURE)
                 && (halResetInfo.crash.resetReason == RESET_BOOTLOADER_DEEPSLEEP)) {
    // Save the crash info for bootloader deep sleep (even though it's not used
    // yet) and invalidate the resetSignature.
    halResetInfo.crash.resetSignature = RESET_INVALID_SIGNATURE;
    savedResetCause = halResetInfo.crash.resetReason;
  } else {
    savedResetCause = cause;
  }
  // If the last reset was due to an assert, save the assert info.
  if (savedResetCause == RESET_CRASH_ASSERT) {
    savedAssertInfo = halResetInfo.crash.data.assertInfo;
  }
}

uint8_t halGetResetInfo(void)
{
  return RESET_BASE_TYPE(savedResetCause);
}

uint16_t halGetExtendedResetInfo(void)
{
  return savedResetCause;
}

const HalAssertInfoType *halGetAssertInfo(void)
{
  return &savedAssertInfo;
}


