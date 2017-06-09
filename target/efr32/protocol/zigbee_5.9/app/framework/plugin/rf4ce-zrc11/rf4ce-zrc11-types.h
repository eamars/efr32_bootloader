// Copyright 2014 Silicon Laboratories, Inc.

#ifndef __RF4CE_ZRC11_TYPES_H__
#define __RF4CE_ZRC11_TYPES_H__

/**
 * @brief RF4CE ZRC 1.1 command codes.
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberAfRf4ceZrcCommandCode
#else
typedef uint8_t EmberAfRf4ceZrcCommandCode;
enum
#endif
{
  EMBER_AF_RF4CE_ZRC_COMMAND_USER_CONTROL_PRESSED       = 0x01,
  EMBER_AF_RF4CE_ZRC_COMMAND_USER_CONTROL_REPEATED      = 0x02,
  EMBER_AF_RF4CE_ZRC_COMMAND_USER_CONTROL_RELEASED      = 0x03,
  EMBER_AF_RF4CE_ZRC_COMMAND_COMMAND_DISCOVERY_REQUEST  = 0x04,
  EMBER_AF_RF4CE_ZRC_COMMAND_COMMAND_DISCOVERY_RESPONSE = 0x05,
};

/**
 * @brief RF4CE user control codes from HDMI 1.3a CEC operand [UI Command].
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberAfRf4ceUserControlCode
#else
typedef uint8_t EmberAfRf4ceUserControlCode;
enum
#endif
{
  EMBER_AF_RF4CE_USER_CONTROL_CODE_SELECT                      = 0x00,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_UP                          = 0x01,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_DOWN                        = 0x02,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_LEFT                        = 0x03,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_RIGHT                       = 0x04,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_RIGHT_UP                    = 0x05,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_RIGHT_DOWN                  = 0x06,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_LEFT_UP                     = 0x07,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_LEFT_DOWN                   = 0x08,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_ROOT_MENU                   = 0x09,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_SETUP_MENU                  = 0x0A,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_CONTENTS_MENU               = 0x0B,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_FAVORITE_MENU               = 0x0C,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_EXIT                        = 0x0D,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_0                           = 0x20,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_1                           = 0x21,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_2                           = 0x22,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_3                           = 0x23,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_4                           = 0x24,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_5                           = 0x25,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_6                           = 0x26,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_7                           = 0x27,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_8                           = 0x28,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_9                           = 0x29,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_DOT                         = 0x2A,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_ENTER                       = 0x2B,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_CLEAR                       = 0x2C,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_NEXT_FAVORITE               = 0x2F,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_CHANNEL_UP                  = 0x30,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_CHANNEL_DOWN                = 0x31,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_PREVIOUS_CHANNEL            = 0x32,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_SOUND_SELECT                = 0x33,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_INPUT_SELECT                = 0x34,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_DISPLAY_INFORMATION         = 0x35,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_HELP                        = 0x36,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_PAGE_UP                     = 0x37,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_PAGE_DOWN                   = 0x38,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_POWER                       = 0x40,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_VOLUME_UP                   = 0x41,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_VOLUME_DOWN                 = 0x42,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_MUTE                        = 0x43,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_PLAY                        = 0x44,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_STOP                        = 0x45,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_PAUSE                       = 0x46,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_RECORD                      = 0x47,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_REWIND                      = 0x48,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_FAST_FORWARD                = 0x49,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_EJECT                       = 0x4A,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_FORWARD                     = 0x4B,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_BACKWARD                    = 0x4C,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_STOP_RECORD                 = 0x4D,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_PAUSE_RECORD                = 0x4E,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_ANGLE                       = 0x50,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_SUB_PICTURE                 = 0x51,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_VIDEO_ON_DEMAND             = 0x52,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_ELECTRONIC_PROGRAM_GUIDE    = 0x53,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_TIMER_PROGRAMMING           = 0x54,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_INITIAL_CONFIGURATION       = 0x55,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_PLAY_FUNCTION               = 0x60, // Play Mode - 1 byte
  EMBER_AF_RF4CE_USER_CONTROL_CODE_PAUSE_PLAY_FUNCTION         = 0x61,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_RECORD_FUNCTION             = 0x62,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_PAUSE_RECORD_FUNCTION       = 0x63,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_STOP_FUNCTION               = 0x64,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_MUTE_FUNCTION               = 0x65,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_RESTORE_VOLUME_FUNCTION     = 0x66,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_TUNE_FUNCTION               = 0x67, // Channel Identifier - 4 bytes
  EMBER_AF_RF4CE_USER_CONTROL_CODE_SELECT_MEDIA_FUNCTION       = 0x68, // UI Function Media - 1 byte
  EMBER_AF_RF4CE_USER_CONTROL_CODE_SELECT_A_V_INPUT_FUNCTION   = 0x69, // UI Function Select A/V Input - 1 byte
  EMBER_AF_RF4CE_USER_CONTROL_CODE_SELECT_AUDIO_INPUT_FUNCTION = 0x6A, // UI Function Select Audio Input - 1 byte
  EMBER_AF_RF4CE_USER_CONTROL_CODE_POWER_TOGGLE_FUNCTION       = 0x6B,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_POWER_OFF_FUNCTION          = 0x6C,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_POWER_ON_FUNCTION           = 0x6D,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_F1_BLUE                     = 0x71,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_F2_RED                      = 0x72,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_F3_GREEN                    = 0x73,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_F4_YELLOW                   = 0x74,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_F5                          = 0x75,
  EMBER_AF_RF4CE_USER_CONTROL_CODE_DATA                        = 0x76,
};

/**
 * @brief This data structure contains the ZRC 1.x user control record.
 */
typedef struct {
  uint8_t pairingIndex;
  EmberAfRf4ceZrcCommandCode commandCode;
  EmberAfRf4ceUserControlCode rcCommandCode;
  const uint8_t *rcCommandPayload;
  uint8_t rcCommandPayloadLength;
  uint16_t timeMs;
} EmberAfRf4ceZrcUserControlRecord;

/**
 * @brief Size of the ZRC 1.x command discovery data in bytes (32).
 */
#define EMBER_AF_RF4CE_ZRC_COMMANDS_SUPPORTED_SIZE 32

/**
 * @brief This data structure contains the ZRC 1.x command discovery data.
 */
typedef struct {
  /** This is the command discovery data. */
  uint8_t contents[EMBER_AF_RF4CE_ZRC_COMMANDS_SUPPORTED_SIZE];
} EmberAfRf4ceZrcCommandsSupported;

#endif // __RF4CE_ZRC11_TYPES_H__
