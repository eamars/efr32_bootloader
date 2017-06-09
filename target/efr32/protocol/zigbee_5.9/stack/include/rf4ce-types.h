/**
 * @file rf4ce-types.h
 * @brief Zigbee RF4CE types and defines.
 * See @ref rf4ce for documentation.
 *
 * <!--Copyright 2013 Silicon Laboratories, Inc.                         *80*-->
 */

/**
 * @addtogroup rf4ce_types
 *
 * See rf4ce-types.h for source code.
 * @{
 */

#ifndef __RF4CE_TYPES_H__
#define __RF4CE_TYPES_H__

/**
 * @name RF4CE Types
 */
//@{

/**
 * @brief Zigbee RF4CE boradcast network address.
 */
#define EMBER_RF4CE_BROADCAST_ADDRESS           0xFFFF

/**
 * @brief Zigbee RF4CE boradcast PAN ID.
 */
#define EMBER_RF4CE_BROADCAST_PAN_ID            0xFFFF

/**
 * @brief Bitmask to scan all the Zigbee RF4CE channels.
 */
#define EMBER_ALL_ZIGBEE_RF4CE_CHANNELS_MASK    0x02108000UL

/*
 * @brief A distinguished vendor id that is used to indicate the absence of a
 * vendor-specific command.
 */
#define EMBER_RF4CE_NULL_VENDOR_ID 0x0000

/**
 * @brief The length of the vendor string stored in the ::EmberRf4ceVendorInfo
 *   struct.
 */
#define EMBER_RF4CE_VENDOR_STRING_LENGTH                      7

/**
 * @brief The length of the application user string stored in the
 *  ::EmberRf4ceApplicationInfo struct.
 */
#define EMBER_RF4CE_APPLICATION_USER_STRING_LENGTH            15

/**
 * @brief The length of the application device type list stored in the
 *  ::EmberRf4ceApplicationInfo struct.
 */
#define EMBER_RF4CE_APPLICATION_DEVICE_TYPE_LIST_MAX_LENGTH   3

/**
 * @brief The length of the application profile ID list stored in the
 *  ::EmberRf4ceApplicationInfo struct.
 */
#define EMBER_RF4CE_APPLICATION_PROFILE_ID_LIST_MAX_LENGTH    7

/**
 * @brief A distinguished pairing table index used to indicate a broadcast
 * message.
 */
#define EMBER_RF4CE_PAIRING_TABLE_BROADCAST_INDEX 0xFF

/**
 * @brief Minimum active period duration in milliseconds.
 */
#define EMBER_RF4CE_MIN_ACTIVE_PERIOD_MS 17

/**
 * @brief Maximum duty cycle duration in milliseconds.
 */
#define EMBER_RF4CE_MAX_DUTY_CYCLE_MS 1000

/**
 * @brief Options to use when sending a message.
 *
 * These are defined in the RF4CE specs (see Table 2, section 3.1.1.1.1).
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberRf4ceTxOption
#else
typedef uint8_t EmberRf4ceTxOption;
enum
#endif
{
  /**
   * No options.
   */
  EMBER_RF4CE_TX_OPTIONS_NONE                             = 0x00,
  /**
   * Broadcast or unicast transmission (transmission mode).
   */
  EMBER_RF4CE_TX_OPTIONS_BROADCAST_BIT                    = 0x01,
  /**
   * Use destination IEEE address or destination network address (destination
   *  addressing mode).
   */
  EMBER_RF4CE_TX_OPTIONS_USE_IEEE_ADDRESS_BIT             = 0x02,
  /**
   * MAC acknowledged transmission (acknowledgment mode).
   */
  EMBER_RF4CE_TX_OPTIONS_ACK_REQUESTED_BIT                = 0x04,
  /**
   * Transmit with or without security (security mode).
   */
  EMBER_RF4CE_TX_OPTIONS_SECURITY_ENABLED_BIT             = 0x08,
  /**
   * Use single or multiple channel operation (channel agility mode).
   */
  EMBER_RF4CE_TX_OPTIONS_SINGLE_CHANNEL_BIT               = 0x10,
  /**
   * Specify channel designator or not (channel normalization mode).
   */
  EMBER_RF4CE_TX_OPTIONS_CHANNEL_DESIGNATOR_BIT           = 0x20,
  /**
   * Data is vendor specific or non-vendor specific (payload mode).
   */
  EMBER_RF4CE_TX_OPTIONS_VENDOR_SPECIFIC_BIT              = 0x40
};

/**
 * @brief RF4CE node capabilities.
 *
 * These are defined in the RF4CE specs (see section 3.4.2.4).
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberRf4ceNodeCapabilities
#else
typedef uint8_t EmberRf4ceNodeCapabilities;
enum
#endif
{
  EMBER_RF4CE_NODE_CAPABILITIES_NONE                         = 0x00,
  /**
   * From RF4CE specs: "The node type sub-field is 1-bit in length and shall be
   * set to one if the node is a target node. Otherwise the node type sub-field
   * shall be set to zero indicating a controller node".
   */
  EMBER_RF4CE_NODE_CAPABILITIES_IS_TARGET_BIT                = 0x01,
  /**
   * From RF4CE specs: "The power source sub-field is 1-bit in length and shall
   * be set to one if the node is receiving power from the alternating current
   * mains. Otherwise, the power source sub-field shall be set to zero".
   */
  EMBER_RF4CE_NODE_CAPABILITIES_POWER_SOURCE_BIT             = 0x02,
  /**
   * From RF4CE specs: "The security capable sub-field is 1-bit in length and
   * shall be set to one if the node is capable of sending and receiving
   * cryptographically protected frames. Otherwise, the security capable
   * sub-field shall be set to zero".
   */
  EMBER_RF4CE_NODE_CAPABILITIES_SECURITY_BIT                 = 0x04,
  /**
   * From RF4CE specs: "The channel normalization sub-field is 1-bit in length
   * and shall be set to one if the node will react to a channel change request
   * through the channel designator sub-field of the frame control field.
   * Otherwise, the channel normalization sub-field shall be set to zero".
   */
  EMBER_RF4CE_NODE_CAPABILITIES_CHANNEL_NORM_BIT             = 0x08

  // Bits 4-7 are reserved
};

/**
 * @brief RF4CE application capabilities.
 *
 * These are defined in the RF4CE specs (see section 3.3.1.5, Figure 18).
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberRf4ceApplicationCapabilities
#else
typedef uint8_t EmberRf4ceApplicationCapabilities;
enum
#endif
{
  EMBER_RF4CE_APP_CAPABILITIES_NONE                            = 0x00,
  /**
   * From RF4CE specs: "The user string bit shall be set to one if a user string
   * is to be included in the frame. Otherwise, it shall be set to zero. The
   * user string field shall be included in the frame only if the user string
   * bit is set to 1".
   */
  EMBER_RF4CE_APP_CAPABILITIES_USER_STRING_BIT                 = 0x01,
  /**
   * From RF4CE specs: "The number of supported device types sub-field is 2 bits
   * in length and shall contain the number of device types supported by the
   * application".
   */
  EMBER_RF4CE_APP_CAPABILITIES_SUPPORTED_DEVICE_TYPES_MASK     = 0x06,
  /**
   * The offset of the device types subfield within the application
   * capabilities field.
   */
  EMBER_RF4CE_APP_CAPABILITIES_SUPPORTED_DEVICE_TYPES_OFFSET   = 1,

  // Bit 3 is reserved.

  /**
   * From RF4CE specs: "The number of supported profiles sub-field is 3 bits in
   * length and shall contain the number of profiles disclosed as supported by
   * the application".
   */
  EMBER_RF4CE_APP_CAPABILITIES_SUPPORTED_PROFILES_MASK         = 0x70,
  /**
   * The offset of the supported profiles subfield within the application
   * capabilities field.
   */
  EMBER_RF4CE_APP_CAPABILITIES_SUPPORTED_PROFILES_OFFSET       = 4,

  // Bit 7 is reserved.
};

/**
 * @brief Defines the vendor information block (see section 3.3.1, Figure 16).
 */
typedef struct {
  /**
   * The vendor identifier field shall contain the vendor identifier of the
   * node.
   */
  uint16_t vendorId;
  /**
   * The vendor string field shall contain the vendor string of the node.
   */
  uint8_t vendorString[EMBER_RF4CE_VENDOR_STRING_LENGTH];
} EmberRf4ceVendorInfo;

/** @brief Defines the application information block (see section 3.3.1,
 * Figure 17).
 */
typedef struct {
  /**
   * The application capabilities field shall contain information relating to
   * the capabilities of the application of the node.
   */
  EmberRf4ceApplicationCapabilities capabilities;
  /**
   * The user string field shall contain the user specified identification
   * string.
   */
  uint8_t userString[EMBER_RF4CE_APPLICATION_USER_STRING_LENGTH];
  /**
   * The device type list field shall contain the list of device types supported
   * by the node.
   */
  uint8_t deviceTypeList[EMBER_RF4CE_APPLICATION_DEVICE_TYPE_LIST_MAX_LENGTH];
  /**
   * The profile ID list field shall contain the list of profile identifiers
   * disclosed as supported by the node.
   */
  uint8_t profileIdList[EMBER_RF4CE_APPLICATION_PROFILE_ID_LIST_MAX_LENGTH];
} EmberRf4ceApplicationInfo;

/**
 * @brief RF4CE pairing table entry status.
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberRf4cePairingEntryStatus
#else
typedef uint8_t EmberRf4cePairingEntryStatus;
enum
#endif
{
  /**
   * The pairing table entry is not in use.
   */
  EMBER_RF4CE_PAIRING_TABLE_ENTRY_STATUS_UNUSED          = 0x00,
  /**
   * The pairing table entry is marked as provisional because currently involved
   * in some pairing process.
   */
  EMBER_RF4CE_PAIRING_TABLE_ENTRY_STATUS_PROVISIONAL     = 0x01,
  /**
   * The pairing table entry is in use.
   */
  EMBER_RF4CE_PAIRING_TABLE_ENTRY_STATUS_ACTIVE          = 0x02
};

/**
 * @brief The status of the pairing table entry stored in the info byte.
 */
#define EMBER_RF4CE_PAIRING_TABLE_ENTRY_INFO_STATUS_MASK                    0x03

/**
 * @brief Bit in the info byte of a pairing table entry that indicates whether
 * the pairing entry has a link key.
 */
#define EMBER_RF4CE_PAIRING_TABLE_ENTRY_INFO_HAS_LINK_KEY_BIT               0x04

/**
 * @brief Bit in the info byte of a pairing table entry that indicates whether
 * the node initiated the pairing process.
 */
#define EMBER_RF4CE_PAIRING_TABLE_ENTRY_INFO_IS_PAIRING_INITIATOR_BIT       0x08

/** @brief The internal representation of a pairing table entry.
 */
typedef struct {
  /**
   * The link key to be used to secure this pairing link.
   */
  EmberKeyData securityLinkKey;
  /**
   * The IEEE address of the destination device.
   */
  EmberEUI64 destLongId;
  /**
   * The frame counter last received from the recipient node.
   */
  uint32_t frameCounter;
  /**
   * The network address to be assumed by the source device.
   */
  EmberNodeId sourceNodeId;
  /**
   * The PAN identifier of the destination device.
   */
  EmberPanId destPanId;
  /**
   * The network address of the destination device.
   */
  EmberNodeId destNodeId;
  /**
   * The vendor ID of the destination device.
   */
  uint16_t destVendorId;
  /**
   * The list of profiles supported by the destination device.
   */
  uint8_t destProfileIdList[EMBER_RF4CE_APPLICATION_PROFILE_ID_LIST_MAX_LENGTH];
  /**
   * The length of the list of supported profiles by the destination device.
   */
  uint8_t destProfileIdListLength;
  /**
   * Info byte (bits [3-7] are reserved for internal use).
   */
  uint8_t info;
  /**
   * The expected channel of the destination device.
   */
  uint8_t channel;
  /**
   * The node capabilities of the recipient node.
   */
  uint8_t capabilities;
  /**
   * Last MAC sequence number seen on this pairing link.
   */
  uint8_t lastSeqn;
} EmberRf4cePairingTableEntry;

//@} \\END RF4CE Types

#endif // __RF4CE_TYPES_H__

/** @} // END addtogroup
*/
