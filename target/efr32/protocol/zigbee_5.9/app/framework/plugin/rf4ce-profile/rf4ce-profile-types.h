// Copyright 2014 Silicon Laboratories, Inc.

#ifndef __RF4CE_PROFILE_TYPES_H__
#define __RF4CE_PROFILE_TYPES_H__

/**
 * @brief ZigBee RF4CE status.
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberAfRf4ceStatus
#else
typedef uint8_t EmberAfRf4ceStatus;
enum
#endif
{
  EMBER_AF_RF4CE_STATUS_SUCCESS               = 0x00,
  EMBER_AF_RF4CE_STATUS_NO_ORG_CAPACITY       = 0xB0,
  EMBER_AF_RF4CE_STATUS_NO_REC_CAPACITY       = 0xB1,
  EMBER_AF_RF4CE_STATUS_NO_PAIRING            = 0xB2,
  EMBER_AF_RF4CE_STATUS_NO_RESPONSE           = 0xB3,
  EMBER_AF_RF4CE_STATUS_NOT_PERMITTED         = 0xB4,
  EMBER_AF_RF4CE_STATUS_DUPLICATE_PAIRING     = 0xB5,
  EMBER_AF_RF4CE_STATUS_FRAME_COUNTER_EXPIRED = 0xB6,
  EMBER_AF_RF4CE_STATUS_DISCOVERY_ERROR       = 0xB7,
  EMBER_AF_RF4CE_STATUS_DISCOVERY_TIMEOUT     = 0xB8,
  EMBER_AF_RF4CE_STATUS_SECURITY_TIMEOUT      = 0xB9,
  EMBER_AF_RF4CE_STATUS_SECURITY_FAILURE      = 0xBA,
  EMBER_AF_RF4CE_STATUS_INVALID_PARAMETER     = 0xE8,
  EMBER_AF_RF4CE_STATUS_UNSUPPORTED_ATTRIBUTE = 0xF4,
  EMBER_AF_RF4CE_STATUS_INVALID_INDEX         = 0xF9,
};

/**
 * @brief ZigBee RF4CE profile identifier.
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberAfRf4ceProfileId
#else
typedef uint8_t EmberAfRf4ceProfileId;
enum
#endif
{
  /** Generic Device Profile (GDP) versions 1.0 and 2.0. */
  EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE                      = 0x00,
  /** The Consumer Electronics Remote Control (CERC) profile was renamed to the
   * ZigBee Remote Control (ZRC) profile when the specification went from
   * version 1.0 in document 09-4946-00 to version 1.1 in document 09-4946-01.
   * 1.1 is backwards compatible with 1.0.  For convenience, the profile can be
   * referred to as CERC, ZRC 1.0, or ZRC 1.1.
   */
  EMBER_AF_RF4CE_PROFILE_CONSUMER_ELECTRONICS_REMOTE_CONTROL = 0x01,
  /** A convenience alias for
   * ::EMBER_AF_RF4CE_PROFILE_CONSUMER_ELECTRONICS_REMOTE_CONTROL.
   */
  EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_1_0                  = 0x01,
  /** ZigBee Remote Control (ZRC) profile version 1.1. */
  EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_1_1                  = 0x01,
  /** ZigBee Input Device (ZID) profile version 1.0. */
  EMBER_AF_RF4CE_PROFILE_INPUT_DEVICE_1_0                    = 0x02,
  /** ZigBee Remote Control (ZRC) profile version 2.0. */
  EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0                  = 0x03,
  /** Multiple System Operators (MSO) profile. */
  EMBER_AF_RF4CE_PROFILE_MSO                                 = 0xC0,
  /** Wildcard profile. */
  EMBER_AF_RF4CE_PROFILE_WILDCARD                            = 0xFF,
};

/**
 * @brief ZigBee RF4CE vendor identifier.
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberAfRf4ceVendor
#else
typedef uint16_t EmberAfRf4ceVendor;
enum
#endif
{
  EMBER_AF_RF4CE_VENDOR_PANASONIC         = 0x0001,
  EMBER_AF_RF4CE_VENDOR_SONY              = 0x0002,
  EMBER_AF_RF4CE_VENDOR_SAMSUNG           = 0x0003,
  EMBER_AF_RF4CE_VENDOR_PHILIPS           = 0x0004,
  EMBER_AF_RF4CE_VENDOR_FREESCALE         = 0x0005,
  EMBER_AF_RF4CE_VENDOR_OKI_SEMICONDUCTOR = 0x0006,
  EMBER_AF_RF4CE_VENDOR_TEXAS_INSTRUMENTS = 0x0007,
  EMBER_AF_RF4CE_VENDOR_TEST_VENDOR_1     = 0xFFF1,
  EMBER_AF_RF4CE_VENDOR_TEST_VENDOR_2     = 0xFFF2,
  EMBER_AF_RF4CE_VENDOR_TEST_VENDOR_3     = 0xFFF3,
};

/**
 * @brief ZigBee RF4CE device type.
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberAfRf4ceDeviceType
#else
typedef uint8_t EmberAfRf4ceDeviceType;
enum
#endif
{
  EMBER_AF_RF4CE_DEVICE_TYPE_RESERVED                 = 0x00,
  EMBER_AF_RF4CE_DEVICE_TYPE_REMOTE_CONTROL           = 0x01,
  EMBER_AF_RF4CE_DEVICE_TYPE_TELEVISION               = 0x02,
  EMBER_AF_RF4CE_DEVICE_TYPE_PROJECTOR                = 0x03,
  EMBER_AF_RF4CE_DEVICE_TYPE_PLAYER                   = 0x04,
  EMBER_AF_RF4CE_DEVICE_TYPE_RECORDER                 = 0x05,
  EMBER_AF_RF4CE_DEVICE_TYPE_VIDEO_PLAYER_RECORDER    = 0x06,
  EMBER_AF_RF4CE_DEVICE_TYPE_AUDIO_PLAYER_RECORDER    = 0x07,
  EMBER_AF_RF4CE_DEVICE_TYPE_AUDIO_VIDEO_RECORDER     = 0x08,
  EMBER_AF_RF4CE_DEVICE_TYPE_SET_TOP_BOX              = 0x09,
  EMBER_AF_RF4CE_DEVICE_TYPE_HOME_THEATER_SYSTEM      = 0x0A,
  EMBER_AF_RF4CE_DEVICE_TYPE_MEDIA_CENTER_PC          = 0x0B,
  EMBER_AF_RF4CE_DEVICE_TYPE_GAME_CONSOLE             = 0x0C,
  EMBER_AF_RF4CE_DEVICE_TYPE_SATELLITE_RADIO_RECEIVER = 0x0D,
  EMBER_AF_RF4CE_DEVICE_TYPE_IR_EXTENDER              = 0x0E,
  EMBER_AF_RF4CE_DEVICE_TYPE_MONITOR                  = 0x0F,
  EMBER_AF_RF4CE_DEVICE_TYPE_GENERIC                  = 0xFE,
  EMBER_AF_RF4CE_DEVICE_TYPE_WILDCARD                 = 0xFF,
};

/**
 * @brief Function type definition of the pair complete callback.
 */
typedef void(*EmberAfRf4cePairCompleteCallback)(uint8_t,
                                                EmberStatus,
                                                const EmberRf4ceVendorInfo*,
                                                const EmberRf4ceApplicationInfo*);

#endif // __RF4CE_PROFILE_TYPES_H__
