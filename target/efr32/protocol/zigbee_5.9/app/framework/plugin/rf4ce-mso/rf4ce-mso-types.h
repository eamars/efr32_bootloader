// Copyright 2014 Silicon Laboratories, Inc.

#ifndef __RF4CE_MSO_TYPES_H__
#define __RF4CE_MSO_TYPES_H__

/**
 * @brief RF4CE MSO binding states.
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberAfRf4ceMsoBindingState
#else
typedef uint8_t EmberAfRf4ceMsoBindingState;
enum
#endif
{
  EMBER_AF_RF4CE_MSO_BINDING_STATE_NOT_BOUND = 0,
  EMBER_AF_RF4CE_MSO_BINDING_STATE_BINDING   = 1,
  EMBER_AF_RF4CE_MSO_BINDING_STATE_BOUND     = 2,
};

/**
 * @brief RF4CE MSO validation states.
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberAfRf4ceMsoValidationState
#else
typedef uint8_t EmberAfRf4ceMsoValidationState;
enum
#endif
{
  EMBER_AF_RF4CE_MSO_VALIDATION_STATE_NOT_VALIDATED        = 0,
  EMBER_AF_RF4CE_MSO_VALIDATION_STATE_REJECTED             = 1,
  EMBER_AF_RF4CE_MSO_VALIDATION_STATE_VALIDATING           = 2,
  EMBER_AF_RF4CE_MSO_VALIDATION_STATE_REVALIDATING         = 3,
  EMBER_AF_RF4CE_MSO_VALIDATION_STATE_VALIDATED            = 4,
};

/**
 * @brief RF4CE MSO command codes.
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberAfRf4ceMsoCommandCode
#else
typedef uint8_t EmberAfRf4ceMsoCommandCode;
enum
#endif
{
  EMBER_AF_RF4CE_MSO_COMMAND_USER_CONTROL_PRESSED      = 0x01,
  EMBER_AF_RF4CE_MSO_COMMAND_USER_CONTROL_REPEATED     = 0x02,
  EMBER_AF_RF4CE_MSO_COMMAND_USER_CONTROL_RELEASED     = 0x03,
  EMBER_AF_RF4CE_MSO_COMMAND_CHECK_VALIDATION_REQUEST  = 0x20,
  EMBER_AF_RF4CE_MSO_COMMAND_CHECK_VALIDATION_RESPONSE = 0x21,
  EMBER_AF_RF4CE_MSO_COMMAND_SET_ATTRIBUTE_REQUEST     = 0x22,
  EMBER_AF_RF4CE_MSO_COMMAND_SET_ATTRIBUTE_RESPONSE    = 0x23,
  EMBER_AF_RF4CE_MSO_COMMAND_GET_ATTRIBUTE_REQUEST     = 0x24,
  EMBER_AF_RF4CE_MSO_COMMAND_GET_ATTRIBUTE_RESPONSE    = 0x25,
};

/**
 * @brief RF4CE MSO key codes.
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberAfRf4ceMsoKeyCode
#else
typedef uint8_t EmberAfRf4ceMsoKeyCode;
enum
#endif
{
  EMBER_AF_RF4CE_MSO_KEY_CODE_OK                                        = 0x00,
  EMBER_AF_RF4CE_MSO_KEY_CODE_UP_ARROW                                  = 0x01,
  EMBER_AF_RF4CE_MSO_KEY_CODE_DOWN_ARROW                                = 0x02,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LEFT_ARROW                                = 0x03,
  EMBER_AF_RF4CE_MSO_KEY_CODE_RIGHT_ARROW                               = 0x04,
  EMBER_AF_RF4CE_MSO_KEY_CODE_MENU                                      = 0x09,
  EMBER_AF_RF4CE_MSO_KEY_CODE_DVR                                       = 0x0B,
  EMBER_AF_RF4CE_MSO_KEY_CODE_FAV                                       = 0x0C,
  EMBER_AF_RF4CE_MSO_KEY_CODE_EXIT                                      = 0x0D,
  EMBER_AF_RF4CE_MSO_KEY_CODE_HOME                                      = 0x10,
  EMBER_AF_RF4CE_MSO_KEY_CODE_DIGIT_0                                   = 0x20,
  EMBER_AF_RF4CE_MSO_KEY_CODE_DIGIT_1                                   = 0x21,
  EMBER_AF_RF4CE_MSO_KEY_CODE_DIGIT_2                                   = 0x22,
  EMBER_AF_RF4CE_MSO_KEY_CODE_DIGIT_3                                   = 0x23,
  EMBER_AF_RF4CE_MSO_KEY_CODE_DIGIT_4                                   = 0x24,
  EMBER_AF_RF4CE_MSO_KEY_CODE_DIGIT_5                                   = 0x25,
  EMBER_AF_RF4CE_MSO_KEY_CODE_DIGIT_6                                   = 0x26,
  EMBER_AF_RF4CE_MSO_KEY_CODE_DIGIT_7                                   = 0x27,
  EMBER_AF_RF4CE_MSO_KEY_CODE_DIGIT_8                                   = 0x28,
  EMBER_AF_RF4CE_MSO_KEY_CODE_DIGIT_9                                   = 0x29,
  EMBER_AF_RF4CE_MSO_KEY_CODE_FULL_STOP                                 = 0x2A,
  EMBER_AF_RF4CE_MSO_KEY_CODE_RETURN                                    = 0x2B,
  EMBER_AF_RF4CE_MSO_KEY_CODE_CHANNEL_UP                                = 0x30,
  EMBER_AF_RF4CE_MSO_KEY_CODE_CHANNEL_DOWN                              = 0x31,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LAST                                      = 0x32,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LANG                                      = 0x33,
  EMBER_AF_RF4CE_MSO_KEY_CODE_INPUT_SELECT                              = 0x34,
  EMBER_AF_RF4CE_MSO_KEY_CODE_INFO                                      = 0x35,
  EMBER_AF_RF4CE_MSO_KEY_CODE_HELP                                      = 0x36,
  EMBER_AF_RF4CE_MSO_KEY_CODE_PAGE_UP                                   = 0x37,
  EMBER_AF_RF4CE_MSO_KEY_CODE_PAGE_DOWN                                 = 0x38,
  EMBER_AF_RF4CE_MSO_KEY_CODE_MOTION                                    = 0x3B,
  EMBER_AF_RF4CE_MSO_KEY_CODE_SEARCH                                    = 0x3C,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LIVE                                      = 0x3D,
  EMBER_AF_RF4CE_MSO_KEY_CODE_HD_ZOOM                                   = 0x3E,
  EMBER_AF_RF4CE_MSO_KEY_CODE_SHARE                                     = 0x3F,
  EMBER_AF_RF4CE_MSO_KEY_CODE_TV_POWER                                  = 0x40,
  EMBER_AF_RF4CE_MSO_KEY_CODE_VOLUME_UP                                 = 0x41,
  EMBER_AF_RF4CE_MSO_KEY_CODE_VOLUME_DOWN                               = 0x42,
  EMBER_AF_RF4CE_MSO_KEY_CODE_MUTE                                      = 0x43,
  EMBER_AF_RF4CE_MSO_KEY_CODE_PLAY                                      = 0x44,
  EMBER_AF_RF4CE_MSO_KEY_CODE_STOP                                      = 0x45,
  EMBER_AF_RF4CE_MSO_KEY_CODE_PAUSE                                     = 0x46,
  EMBER_AF_RF4CE_MSO_KEY_CODE_RECORD                                    = 0x47,
  EMBER_AF_RF4CE_MSO_KEY_CODE_REWIND                                    = 0x48,
  EMBER_AF_RF4CE_MSO_KEY_CODE_FAST_FORWARD                              = 0x49,
  EMBER_AF_RF4CE_MSO_KEY_CODE_30_SECOND_SKIP_AHEAD                      = 0x4B,
  EMBER_AF_RF4CE_MSO_KEY_CODE_REPLAY                                    = 0x4C,
  EMBER_AF_RF4CE_MSO_KEY_CODE_SWAP                                      = 0x51,
  EMBER_AF_RF4CE_MSO_KEY_CODE_ON_DEMAND                                 = 0x52,
  EMBER_AF_RF4CE_MSO_KEY_CODE_GUIDE                                     = 0x53,
  EMBER_AF_RF4CE_MSO_KEY_CODE_PUSH_TO_TALK                              = 0x57,
  EMBER_AF_RF4CE_MSO_KEY_CODE_PIP_ON_OFF                                = 0x58,
  EMBER_AF_RF4CE_MSO_KEY_CODE_PIP_MOVE                                  = 0x59,
  EMBER_AF_RF4CE_MSO_KEY_CODE_PIP_CHANNEL_UP                            = 0x5A,
  EMBER_AF_RF4CE_MSO_KEY_CODE_PIP_CHANNEL_DOWN                          = 0x5B,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LOCK                                      = 0x5C,
  EMBER_AF_RF4CE_MSO_KEY_CODE_DAY_UP                                    = 0x5D,
  EMBER_AF_RF4CE_MSO_KEY_CODE_DAY_DOWN                                  = 0x5E,
  EMBER_AF_RF4CE_MSO_KEY_CODE_PLAY_PAUSE                                = 0x61,
  EMBER_AF_RF4CE_MSO_KEY_CODE_STOP_VIDEO                                = 0x64,
  EMBER_AF_RF4CE_MSO_KEY_CODE_MUTE_MIC                                  = 0x65,
  EMBER_AF_RF4CE_MSO_KEY_CODE_POWER_TOGGLE                              = 0x6B,
  EMBER_AF_RF4CE_MSO_KEY_CODE_POWER_OFF                                 = 0x6C,
  EMBER_AF_RF4CE_MSO_KEY_CODE_POWER_ON                                  = 0x6D,
  EMBER_AF_RF4CE_MSO_KEY_CODE_OCAP_B                                    = 0x71,
  EMBER_AF_RF4CE_MSO_KEY_CODE_BLUE_SQUARE                               = 0x71,
  EMBER_AF_RF4CE_MSO_KEY_CODE_OCAP_C                                    = 0x72,
  EMBER_AF_RF4CE_MSO_KEY_CODE_RED_CIRCLE                                = 0x72,
  EMBER_AF_RF4CE_MSO_KEY_CODE_OCAP_D                                    = 0x73,
  EMBER_AF_RF4CE_MSO_KEY_CODE_GREEN_DIAMOND                             = 0x73,
  EMBER_AF_RF4CE_MSO_KEY_CODE_OCAP_A                                    = 0x74,
  EMBER_AF_RF4CE_MSO_KEY_CODE_YELLOW_TRIANGLE                           = 0x74,
  EMBER_AF_RF4CE_MSO_KEY_CODE_PROFILE                                   = 0xA0,
  EMBER_AF_RF4CE_MSO_KEY_CODE_CALL                                      = 0xA1,
  EMBER_AF_RF4CE_MSO_KEY_CODE_HOLD                                      = 0xA2,
  EMBER_AF_RF4CE_MSO_KEY_CODE_END                                       = 0xA3,
  EMBER_AF_RF4CE_MSO_KEY_CODE_VIEWS                                     = 0xA4,
  EMBER_AF_RF4CE_MSO_KEY_CODE_SELF_VIEW                                 = 0xA5,
  EMBER_AF_RF4CE_MSO_KEY_CODE_ZOOM_IN                                   = 0xA6,
  EMBER_AF_RF4CE_MSO_KEY_CODE_ZOOM_OUT                                  = 0xA7,
  EMBER_AF_RF4CE_MSO_KEY_CODE_BACKSPACE                                 = 0xA8,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LOCK_UNLOCK                               = 0xA9,
  EMBER_AF_RF4CE_MSO_KEY_CODE_CAPS                                      = 0xAA,
  EMBER_AF_RF4CE_MSO_KEY_CODE_ALT                                       = 0xAB,
  EMBER_AF_RF4CE_MSO_KEY_CODE_SPACE                                     = 0xAC,
  EMBER_AF_RF4CE_MSO_KEY_CODE_WWW_DOT                                   = 0xAD,
  EMBER_AF_RF4CE_MSO_KEY_CODE_DOT_COM                                   = 0xAE,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_CAPITAL_LETTER_A                    = 0xB0,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_CAPITAL_LETTER_B                    = 0xB1,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_CAPITAL_LETTER_C                    = 0xB2,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_CAPITAL_LETTER_D                    = 0xB3,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_CAPITAL_LETTER_E                    = 0xB4,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_CAPITAL_LETTER_F                    = 0xB5,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_CAPITAL_LETTER_G                    = 0xB6,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_CAPITAL_LETTER_H                    = 0xB7,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_CAPITAL_LETTER_I                    = 0xB8,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_CAPITAL_LETTER_J                    = 0xB9,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_CAPITAL_LETTER_K                    = 0xBA,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_CAPITAL_LETTER_L                    = 0xBB,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_CAPITAL_LETTER_M                    = 0xBC,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_CAPITAL_LETTER_N                    = 0xBD,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_CAPITAL_LETTER_O                    = 0xBE,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_CAPITAL_LETTER_P                    = 0xBF,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_CAPITAL_LETTER_Q                    = 0xC0,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_CAPITAL_LETTER_R                    = 0xC1,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_CAPITAL_LETTER_S                    = 0xC2,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_CAPITAL_LETTER_T                    = 0xC3,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_CAPITAL_LETTER_U                    = 0xC4,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_CAPITAL_LETTER_V                    = 0xC5,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_CAPITAL_LETTER_W                    = 0xC6,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_CAPITAL_LETTER_X                    = 0xC7,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_CAPITAL_LETTER_Y                    = 0xC8,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_CAPITAL_LETTER_Z                    = 0xC9,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_SMALL_LETTER_A                      = 0xCA,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_SMALL_LETTER_B                      = 0xCB,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_SMALL_LETTER_C                      = 0xCC,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_SMALL_LETTER_D                      = 0xCD,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_SMALL_LETTER_E                      = 0xCE,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_SMALL_LETTER_F                      = 0xCF,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_SMALL_LETTER_G                      = 0xD0,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_SMALL_LETTER_H                      = 0xD1,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_SMALL_LETTER_I                      = 0xD2,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_SMALL_LETTER_J                      = 0xD3,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_SMALL_LETTER_K                      = 0xD4,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_SMALL_LETTER_L                      = 0xD5,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_SMALL_LETTER_M                      = 0xD6,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_SMALL_LETTER_N                      = 0xD7,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_SMALL_LETTER_O                      = 0xD8,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_SMALL_LETTER_P                      = 0xD9,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_SMALL_LETTER_Q                      = 0xDA,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_SMALL_LETTER_R                      = 0xDB,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_SMALL_LETTER_S                      = 0xDC,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_SMALL_LETTER_T                      = 0xDD,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_SMALL_LETTER_U                      = 0xDE,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_SMALL_LETTER_V                      = 0xDF,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_SMALL_LETTER_W                      = 0xE0,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_SMALL_LETTER_X                      = 0xE1,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_SMALL_LETTER_Y                      = 0xE2,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LATIN_SMALL_LETTER_Z                      = 0xE3,
  EMBER_AF_RF4CE_MSO_KEY_CODE_QUESTION_MARK                             = 0xE4,
  EMBER_AF_RF4CE_MSO_KEY_CODE_EXCLAMATION_MARK                          = 0xE5,
  EMBER_AF_RF4CE_MSO_KEY_CODE_NUMBER_SIGN                               = 0xE6,
  EMBER_AF_RF4CE_MSO_KEY_CODE_DOLLAR_SIGN                               = 0xE7,
  EMBER_AF_RF4CE_MSO_KEY_CODE_PERCENT_SIGN                              = 0xE8,
  EMBER_AF_RF4CE_MSO_KEY_CODE_AMPERSAND                                 = 0xE9,
  EMBER_AF_RF4CE_MSO_KEY_CODE_ASTERISK                                  = 0xEA,
  EMBER_AF_RF4CE_MSO_KEY_CODE_LEFT_PARENTHESIS                          = 0xEB,
  EMBER_AF_RF4CE_MSO_KEY_CODE_RIGHT_PARENTHESIS                         = 0xEC,
  EMBER_AF_RF4CE_MSO_KEY_CODE_PLUS_SIGN                                 = 0xED,
  EMBER_AF_RF4CE_MSO_KEY_CODE_MINUS_SIGN                                = 0xEE,
  EMBER_AF_RF4CE_MSO_KEY_CODE_EQUALS_SIGN                               = 0xEF,
  EMBER_AF_RF4CE_MSO_KEY_CODE_SLASH                                     = 0xF0,
  EMBER_AF_RF4CE_MSO_KEY_CODE_UNDERSCORE                                = 0xF1,
  EMBER_AF_RF4CE_MSO_KEY_CODE_QUOTATION_MARK                            = 0xF2,
  EMBER_AF_RF4CE_MSO_KEY_CODE_COLON                                     = 0xF3,
  EMBER_AF_RF4CE_MSO_KEY_CODE_SEMICOLON                                 = 0xF4,
  EMBER_AF_RF4CE_MSO_KEY_CODE_AT_SIGN                                   = 0xF5,
  EMBER_AF_RF4CE_MSO_KEY_CODE_APOSTROPHE                                = 0xF6,
  EMBER_AF_RF4CE_MSO_KEY_CODE_COMMA                                     = 0xF7,
};

/**
 * @brief RF4CE MSO check validation statuses.
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberAfRf4ceMsoCheckValidationControl
#else
typedef uint8_t EmberAfRf4ceMsoCheckValidationControl;
enum
#endif
{
  EMBER_AF_RF4CE_MSO_CHECK_VALIDATION_CONTROL_REQUEST_AUTOMATIC_VALIDATION = BIT(0)
};

/**
 * @brief RF4CE MSO check validation statuses.
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberAfRf4ceMsoCheckValidationStatus
#else
typedef uint8_t EmberAfRf4ceMsoCheckValidationStatus;
enum
#endif
{
  /** The validation is successful. */
  EMBER_AF_RF4CE_MSO_CHECK_VALIDATION_STATUS_SUCCESS    = 0x00,
  /** The validation is still in progress. */
  EMBER_AF_RF4CE_MSO_CHECK_VALIDATION_STATUS_PENDING    = 0xC0,
  /** The validation timed out, and the binding procedure SHOULD continue with
      other devices in the list. */
  EMBER_AF_RF4CE_MSO_CHECK_VALIDATION_STATUS_TIMEOUT    = 0xC1,
  /** The validation was terminated at the target side, as more than one
      controller tried to pair. */
  EMBER_AF_RF4CE_MSO_CHECK_VALIDATION_STATUS_COLLISION  = 0xC2,
  /** The validation failed, and the binding procedure SHOULD continue with
      other devices in the list. */
  EMBER_AF_RF4CE_MSO_CHECK_VALIDATION_STATUS_FAILURE    = 0xC3,
  /** The validation is aborted, and the binding procedure SHOULD continue with
      other devices in the list. */
  EMBER_AF_RF4CE_MSO_CHECK_VALIDATION_STATUS_ABORT      = 0xC4,
  /** The validation is aborted, and the binding procedure SHOULD NOT continue
      with other devices in the list. */
  EMBER_AF_RF4CE_MSO_CHECK_VALIDATION_STATUS_FULL_ABORT = 0xC5,
};

/*
* @brief RF4CE MSO binding statuses.
*/
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberAfRf4ceMsoBindingStatus
#else
typedef uint8_t EmberAfRf4ceMsoBindingStatus;
enum
#endif
{
  EMBER_AF_RF4CE_MSO_BINDING_STATUS_SUCCESS                 = 0x00,
  EMBER_AF_RF4CE_MSO_BINDING_STATUS_NO_VALID_RESPONSE       = 0x01,
  EMBER_AF_RF4CE_MSO_BINDING_STATUS_NO_VALID_CANDIDATE      = 0x02,
  EMBER_AF_RF4CE_MSO_BINDING_STATUS_DUPLICATE_CLASS_ABORT   = 0x03,
  EMBER_AF_RF4CE_MSO_BINDING_STATUS_PAIRING_FAILED          = 0x04,
  EMBER_AF_RF4CE_MSO_BINDING_STATUS_VALIDATION_TIMEOUT      = EMBER_AF_RF4CE_MSO_CHECK_VALIDATION_STATUS_TIMEOUT,
  EMBER_AF_RF4CE_MSO_BINDING_STATUS_VALIDATION_COLLISION    = EMBER_AF_RF4CE_MSO_CHECK_VALIDATION_STATUS_COLLISION,
  EMBER_AF_RF4CE_MSO_BINDING_STATUS_VALIDATION_FAILURE      = EMBER_AF_RF4CE_MSO_CHECK_VALIDATION_STATUS_FAILURE,
  EMBER_AF_RF4CE_MSO_BINDING_STATUS_VALIDATION_ABORT        = EMBER_AF_RF4CE_MSO_CHECK_VALIDATION_STATUS_ABORT,
  EMBER_AF_RF4CE_MSO_BINDING_STATUS_VALIDATION_FULL_ABORT   = EMBER_AF_RF4CE_MSO_CHECK_VALIDATION_STATUS_FULL_ABORT,
};

/**
 * @brief RF4CE MSO attribute ids.
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberAfRf4ceMsoAttributeId
#else
typedef uint8_t EmberAfRf4ceMsoAttributeId;
enum
#endif
{
  EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_PERIPHERAL_IDS           = 0x00,
  EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_RF_STATISTICS            = 0x01,
  EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_VERSIONING               = 0x02,
  EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_BATTERY_STATUS           = 0x03,
  EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_SHORT_RF_RETRY_PERIOD    = 0x04,
  EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_IR_RF_DATABASE           = 0xDB,
  EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_VALIDATION_CONFIGURATION = 0xDC,
  EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_GENERAL_PURPOSE          = 0xFF,
};

/**
 * @brief This data structure contains the MSO user control record.
 */
typedef struct {
  uint8_t pairingIndex;
  EmberAfRf4ceMsoCommandCode commandCode;
  EmberAfRf4ceMsoKeyCode rcCommandCode;
  const uint8_t *rcCommandPayload;
  uint8_t rcCommandPayloadLength;
  uint16_t timeMs;
} EmberAfRf4ceMsoUserControlRecord;

/**
 * @brief RF4CE MSO IR-RF database entry flags.
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberAfRf4ceMsoIrRfDatabaseFlags
#else
typedef uint8_t EmberAfRf4ceMsoIrRfDatabaseFlags;
enum
#endif
{
  /** No flags. */
  EMBER_AF_RF4CE_MSO_IR_RF_DATABASE_FLAG_NONE                  = 0x00,
  /** Indicates that an RF pressed descriptor is included in this attribute,
   *  and that an RF message should be generated when this key is pressed.  If
   *  Use Default is set, this field should be ignored (treated as if it was
   *  zero).
   */
  EMBER_AF_RF4CE_MSO_IR_RF_DATABASE_FLAG_RF_PRESSED_SPECIFIED  = 0x01,
  /** Indicates that an RF repeated descriptor is included in this attribute,
   *  and that an RF message should be generated when this key is kept pressed.
   *  If Use Default is set, this field should be ignored (treated as if it was
   *  zero).
   */
  EMBER_AF_RF4CE_MSO_IR_RF_DATABASE_FLAG_RF_REPEATED_SPECIFIED = 0x02,
  /** Indicates that an RF released descriptor is included in this attribute,
   *  and that an RF message should be generated when this key is released.  If
   *  Use Default is set, this field should be ignored (treated as if it was
   *  zero).
   */
  EMBER_AF_RF4CE_MSO_IR_RF_DATABASE_FLAG_RF_RELEASED_SPECIFIED = 0x04,
  /** Indicates that an IR descriptor is included in this attribute, and that
   *  an IR message should be generated when this key is pressed and kept
   *  pressed.  If Use Default is set, this field should be ignored (treated as
   *  if it was zero).
   */
  EMBER_AF_RF4CE_MSO_IR_RF_DATABASE_FLAG_IR_SPECIFIED          = 0x08,
  /** Represents the device type of the IR descriptor if the IR Specified flag
   *  is set.
   */
  EMBER_AF_RF4CE_MSO_IR_RF_DATABASE_FLAG_DEVICE_TYPE_MASK      = 0x48,
  /** Indicates that the default (known by the RC) RF and IR codes should be
   *  used.
   */
  EMBER_AF_RF4CE_MSO_IR_RF_DATABASE_FLAG_USE_DEFAULT           = 0x40,
  /** Indicates that the codes are permanent and can be used for all further
   *  presses of this key.
   */
  EMBER_AF_RF4CE_MSO_IR_RF_DATABASE_FLAG_PERMANENT             = 0x80,
};

/**
 * @brief RF4CE MSO IR-RF database device types.
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberAfRf4ceMsoIrRfDatabaseDeviceType
#else
typedef uint8_t EmberAfRf4ceMsoIrRfDatabaseDeviceType;
enum
#endif
{
  /** The device type of the IR descriptor is for a TV. */
  EMBER_AF_RF4CE_MSO_IR_RF_DATABASE_DEVICE_TYPE_TV  = 0x00,
  /** The device type of the IR descriptor is for ab AVR. */
  EMBER_AF_RF4CE_MSO_IR_RF_DATABASE_DEVICE_TYPE_AVR = 0x20,
};

/**
 * @brief RF4CE MSO IR-RF database RF config.
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberAfRf4ceMsoIrRfDatabaseRfConfig
#else
typedef uint8_t EmberAfRf4ceMsoIrRfDatabaseRfConfig;
enum
#endif
{
  /** Indicates the minimum number of transmissions for this code.  For
   *  acknowledged RF transmissions, this field is set to '1'.
   */
  EMBER_AF_RF4CE_MSO_IR_RF_DATABASE_RF_CONFIG_MINIMUM_NUMBER_OF_TRANSMISSIONS_MASK = 0x0F,
  /** Indicates if the code should continue being transmitted after the minimum
   *  number of transmissions have taken place, when the key is kept pressed.
   *  (This field only applies to RF Repeated frames.)
   */
  EMBER_AF_RF4CE_MSO_IR_RF_DATABASE_RF_CONFIG_KEEP_TRANSMITTING_UNTIL_KEY_RELEASE  = 0x10,
  /** Indicates if the RF4CE retry period for UAM messages should be shorted
   *  for this code to increase responsiveness of the system.
   */
  EMBER_AF_RF4CE_MSO_IR_RF_DATABASE_RF_CONFIG_SHORT_RETRY                          = 0x20,
};

/**
 * @brief RF4CE MSO IR-RF database RF descriptor.
 */
typedef struct {
  EmberAfRf4ceMsoIrRfDatabaseRfConfig rfConfig;
  EmberRf4ceTxOption txOptions;
  uint8_t payloadLength;
  const uint8_t *payload;
} EmberAfRf4ceMsoIrRfDatabaseRfDescriptor;

/**
 * @brief RF4CE MSO IR-RF database IR config.
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberAfRf4ceMsoIrRfDatabaseIrConfig
#else
typedef uint8_t EmberAfRf4ceMsoIrRfDatabaseIrConfig;
enum
#endif
{
  /** Indicates the minimum number of transmissions for the IR repeat frames.
   *  Only valid when Tweak Database is set to '1', otherwise follow behavior
   *  as defined by database.  Special case: when Tweak Database is set to '1',
   *  setting this field to 0xF enforces the use of the value from the
   *  database.
   */
  EMBER_AF_RF4CE_MSO_IR_RF_DATABASE_IR_CONFIG_MINIMUM_NUMBER_OF_TRANSMISSIONS_MASK = 0x0F,
  /** Indicates if the repeat frames should continue being transmitted after
   *  the minimum number of transmissions as defined by the database have been
   *  performed, in case the key remains pressed.  Only valid when Tweak
   *  Database is set to '1', otherwise follow behavior as defined by database.
   */
  EMBER_AF_RF4CE_MSO_IR_RF_DATABASE_IR_CONFIG_KEEP_TRANSMITTING_UNTIL_KEY_RELEASE  = 0x10,
  /** Indicates that database behavior should be tweaked, using the "Minimum
   *  number of transmissions" and "Keep Transmitting Until Key Release"
   * fields.
   */
  EMBER_AF_RF4CE_MSO_IR_RF_DATABASE_IR_CONFIG_SHORT_RETRY                          = 0x40,
};

/**
 * @brief RF4CE MSO IR-RF database IR descriptor.
 */
typedef struct {
  EmberAfRf4ceMsoIrRfDatabaseIrConfig irConfig;
  uint8_t irCodeLength;
  const uint8_t *irCode;
} EmberAfRf4ceMsoIrRfDatabaseIrDescriptor;

/**
 * @brief RF4CE MSO IR-RF database entry.
 */
typedef struct {
  EmberAfRf4ceMsoIrRfDatabaseFlags flags;
  EmberAfRf4ceMsoIrRfDatabaseRfDescriptor rfPressedDescriptor;
  EmberAfRf4ceMsoIrRfDatabaseRfDescriptor rfRepeatedDescriptor;
  EmberAfRf4ceMsoIrRfDatabaseRfDescriptor rfReleasedDescriptor;
  EmberAfRf4ceMsoIrRfDatabaseIrDescriptor irDescriptor;
} EmberAfRf4ceMsoIrRfDatabaseEntry;

#endif // __RF4CE_MSO_TYPES_H__
