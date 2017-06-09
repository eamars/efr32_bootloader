// Copyright 2014 Silicon Laboratories, Inc.

#define ZRC_BITMASK_SIZE                                    32

// Fixed size ZRC attributes size (scalar)
#define APL_ZRC_PROFILE_VERSION_SIZE                        2
#define APL_ZRC_PROFILE_CAPABILITIES_SIZE                   4
#define APL_ACTION_REPEAT_TRIGGER_INTERVAL_SIZE             1
#define APL_ACTION_REPEAT_WAIT_TIME_SIZE                    2
#define APL_ZRC_ACTION_BANKS_VERSION_SIZE                   2

// Fixed size ZRC attributes size (arrayed)
#define APL_MAPPABLE_ACTIONS_SIZE                           3

// Default values for those that are defined.
#define APL_ZRC_PROFILE_VERSION_DEFAULT                     0x0200
#define APL_ACTION_REPEAT_TRIGGER_INTERVAL_DEFAULT_MS       (APLC_MAX_ACTION_REPEAT_TRIGGER_INTERVAL_MS / 2)
#define APL_ACTION_REPEAT_WAIT_TIME_DEFAULT_MS              (APLC_MAX_ACTION_REPEAT_TRIGGER_INTERVAL_MS * 2)
#define APL_ZRC_ACTION_BANKS_VERSION_DEFAULT                0x0100

#define MAX_ZRC_ATTRIBUTE_SIZE       EMBER_AF_RF4CE_MAXIMUM_RF4CE_PAYLOAD_LENGTH

#if (defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_LOCAL_IRDB_VENDOR_ATTRIBUTE_SUPPORT)  \
     || defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_REMOTE_IRDB_VENDOR_ATTRIBUTES_SUPPORT) \
     || defined(EMBER_SCRIPTED_TEST))
#define IRBD_SUPPORT_ATTRIBUTES_COUNT   1
#else
#define IRBD_SUPPORT_ATTRIBUTES_COUNT   0
#endif // EMBER_AF_PLUGIN_RF4CE_ZRC20_LOCAL_IRDB_VENDOR_ATTRIBUTE_SUPPORT || EMBER_AF_PLUGIN_RF4CE_ZRC20_REMOTE_IRDB_VENDOR_ATTRIBUTES_SUPPORT

#if defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_IS_ACTION_MAPPING_CLIENT) || defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_IS_ACTION_MAPPING_SERVER) || defined(EMBER_SCRIPTED_TEST)
#define ACTION_MAPPING_ATTRIBUTES_COUNT   2
#else
#define ACTION_MAPPING_ATTRIBUTES_COUNT   0
#endif // EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_ORIGINATOR || EMBER_AF_PLUGIN_RF4CE_ZRC20_IS_ACTION_MAPPING_SERVER

#if defined(EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_ORIGINATOR) || defined (EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_RECIPIENT) || defined(EMBER_SCRIPTED_TEST)
#define HA_ATTRIBUTES_COUNT   2
#else
#define HA_ATTRIBUTES_COUNT   0
#endif // EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_ORIGINATOR || EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_RECIPIENT

#define ZRC_ATTRIBUTES_COUNT    (9 + IRBD_SUPPORT_ATTRIBUTES_COUNT             \
                                 + ACTION_MAPPING_ATTRIBUTES_COUNT             \
                                 + HA_ATTRIBUTES_COUNT)

// Add here all the other mappable action and action mapping defines as needed.
#define MAPPABLE_ACTION_ACTION_DEVICE_TYPE_OFFSET                        0
#define MAPPABLE_ACTION_ACTION_BANK_OFFSET                               1
#define MAPPABLE_ACTION_ACTION_CODE_OFFSET                               2

#define ACTION_MAPPING_FLAGS_OFFSET                                      0
#define ACTION_MAPPING_IR_CONFIG_VENDOR_SPECIFIC_BIT                     0x01

#define HA_ATTRIBUTE_STATUS_OFFSET                                       0
#define HA_ATTRIBUTE_STATUS_VALUE_AVAILABLE_FLAG                         0x01
#define HA_ATTRIBUTE_VALUE_OFFSET                                        1

// Attribute descriptor bitmask field definitions.
#define ZRC_ATTRIBUTE_HAS_REMOTE_GET_ACCESS_BIT                       0x01
#define ZRC_ATTRIBUTE_HAS_REMOTE_SET_ACCESS_BIT                       0x02
#define ZRC_ATTRIBUTE_HAS_REMOTE_PUSH_ACCESS_BIT                      0x04
#define ZRC_ATTRIBUTE_HAS_REMOTE_PULL_ACCESS_BIT                      0x08
#define ZRC_ATTRIBUTE_IS_TWO_DIMENSIONAL_ARRAYED                      0x10
#define ZRC_ATTRIBUTE_LOCAL_NODE_SUPPORTED                            0x20
#define ZRC_ATTRIBUTE_REMOTE_NODE_SUPPORTED                           0x40

typedef struct {
  uint8_t id;
  uint8_t size;
  uint8_t bitmask;
} EmAfRf4ceZrcAttributeDescriptor;

typedef struct {
  uint8_t contents[ZRC_BITMASK_SIZE];
} EmAfZrcBitmask;

typedef struct {
  bool inUse;
  uint8_t entryId;
  uint8_t contents[ZRC_BITMASK_SIZE];
} EmAfZrcArrayedBitmask;

typedef struct {
  // Scalar
  uint16_t zrcProfileVersion;
  uint16_t zrcActionBanksVersion;
  EmAfZrcBitmask *actionBanksSupportedRx;
  EmAfZrcBitmask *actionBanksSupportedTx;
#if (defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_LOCAL_IRDB_VENDOR_ATTRIBUTE_SUPPORT)  \
     || defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_REMOTE_IRDB_VENDOR_ATTRIBUTES_SUPPORT) \
     || defined(EMBER_SCRIPTED_TEST))
  uint16_t *IRDBVendorSupport;
#endif

  // Arrayed
  EmAfZrcArrayedBitmask *actionCodesSupportedRx;
  EmAfZrcArrayedBitmask *actionCodesSupportedTx;

  // Mappable actions, action mappings and HA attributes are stored separately.
} EmAfRf4ceZrcAttributes;

#define ZRC20_CAPABILITIES_NON_RESERVED_BITS_BITMASK      0x000000FF

// Pre-processing build up of the local node ZRC capabilities.
#if defined(EMBER_AF_RF4CE_ZRC_ACTION_BANKS_RX)
#define LOCAL_NODE_SUPPORTS_ACTIONS_RECIPIENT_BIT  EMBER_AF_RF4CE_ZRC_CAPABILITY_SUPPORT_ACTIONS_RECIPIENT
#else
#define LOCAL_NODE_SUPPORTS_ACTIONS_RECIPIENT_BIT  0
#endif // EMBER_AF_RF4CE_ZRC_ACTION_BANKS_RX

#if defined(EMBER_AF_RF4CE_ZRC_ACTION_BANKS_TX)
#define LOCAL_NODE_SUPPORTS_ACTIONS_ORIGINATOR_BIT EMBER_AF_RF4CE_ZRC_CAPABILITY_SUPPORT_ACTIONS_ORIGINATOR
#else
#define LOCAL_NODE_SUPPORTS_ACTIONS_ORIGINATOR_BIT 0
#endif // EMBER_AF_RF4CE_ZRC_ACTION_BANKS_RX

#if defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_IS_ACTION_MAPPING_CLIENT)
#define LOCAL_NODE_IS_AM_CLIENT_BIT EMBER_AF_RF4CE_ZRC_CAPABILITY_SUPPORT_ACTION_MAPPING_CLIENT
#if defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_LOCAL_IRDB_VENDOR_ATTRIBUTE_SUPPORT)
#define LOCAL_NODE_SUPPORTS_VENDOR_SPECIFIC_IRDB_BIT EMBER_AF_RF4CE_ZRC_CAPABILITY_SUPPORT_VENDOR_SPECIFIC_IRDB_FORMATS
#else
#define LOCAL_NODE_SUPPORTS_VENDOR_SPECIFIC_IRDB_BIT 0
#endif // EMBER_AF_PLUGIN_RF4CE_ZRC20_LOCAL_IRDB_VENDOR_ATTRIBUTE_SUPPORT
#else
#define LOCAL_NODE_IS_AM_CLIENT_BIT 0
#define LOCAL_NODE_SUPPORTS_VENDOR_SPECIFIC_IRDB_BIT 0
#endif // EMBER_AF_PLUGIN_RF4CE_ZRC20_IS_ACTION_MAPPING_CLIENT

#if defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_IS_ACTION_MAPPING_SERVER)
#define LOCAL_NODE_IS_AM_SERVER_BIT EMBER_AF_RF4CE_ZRC_CAPABILITY_SUPPORT_ACTION_MAPPING_SERVER
#else
#define LOCAL_NODE_IS_AM_SERVER_BIT 0
#endif // EMBER_AF_PLUGIN_RF4CE_ZRC20_IS_ACTION_MAPPING_SERVER

#if defined(EMBER_AF_PLUGIN_RF4CE_ZRC20_INFORM_ABOUT_SUPPORTED_ACTIONS)
#define LOCAL_NODE_INFORM_BIT EMBER_AF_RF4CE_ZRC_CAPABILITY_INFORM_ABOUT_SUPPORTED_ACTIONS
#else
#define LOCAL_NODE_INFORM_BIT 0
#endif // EMBER_AF_PLUGIN_RF4CE_ZRC20_INFORM_ABOUT_SUPPORTED_ACTIONS

#if defined(EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_ORIGINATOR)
#define LOCAL_NODE_IS_HA_ORIGINATOR_BIT EMBER_AF_RF4CE_ZRC_CAPABILITY_SUPPORT_HA_ACTIONS_ORIGINATOR
#else
#define LOCAL_NODE_IS_HA_ORIGINATOR_BIT 0
#endif // EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_ORIGINATOR

#if defined(EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_RECIPIENT)
#define LOCAL_NODE_IS_HA_RECIPIENT_BIT EMBER_AF_RF4CE_ZRC_CAPABILITY_SUPPORT_HA_ACTIONS_RECIPIENT
#else
#define LOCAL_NODE_IS_HA_RECIPIENT_BIT 0
#endif // EMBER_AF_RF4CE_ZRC_IS_HA_ACTIONS_RECIPIENT

#define LOCAL_NODE_ZRC_CAPABILITIES                                            \
  (0x00000000                                                                  \
   | LOCAL_NODE_SUPPORTS_ACTIONS_RECIPIENT_BIT                                 \
   | LOCAL_NODE_SUPPORTS_ACTIONS_ORIGINATOR_BIT                                \
   | LOCAL_NODE_IS_AM_CLIENT_BIT                                               \
   | LOCAL_NODE_IS_AM_SERVER_BIT                                               \
   | LOCAL_NODE_SUPPORTS_VENDOR_SPECIFIC_IRDB_BIT                              \
   | LOCAL_NODE_INFORM_BIT                                                     \
   | LOCAL_NODE_IS_HA_ORIGINATOR_BIT                                           \
   | LOCAL_NODE_IS_HA_RECIPIENT_BIT)

#if defined(EMBER_SCRIPTED_TEST)
extern uint32_t localNodeZrcCapabilities;
#define emAfRf4ceZrcGetLocalNodeCapabilities() (localNodeZrcCapabilities)
#else
#define emAfRf4ceZrcGetLocalNodeCapabilities() (LOCAL_NODE_ZRC_CAPABILITIES)
#endif

#define emAfRf4ceZrcGetRemoteNodeCapabilities(pairingIndex)                    \
    ((uint32_t)((emAfRf4ceZrcGetRemoteNodeFlags(pairingIndex)                    \
               & ZRC_INTERNAL_FLAGS_CAPABILITIES_MASK)                         \
              >> ZRC_INTERNAL_FLAGS_CAPABILITIES_OFFSET))

#define emAfRf4ceZrcSetRemoteNodeCapabilities(pairingIndex, capabilities)      \
    emAfRf4ceZrcSetRemoteNodeFlags((pairingIndex),                             \
            ((emAfRf4ceZrcGetRemoteNodeFlags(pairingIndex)                     \
              & ~ZRC_INTERNAL_FLAGS_CAPABILITIES_MASK)                         \
             | ((capabilities & ZRC20_CAPABILITIES_NON_RESERVED_BITS_BITMASK)  \
                 << ZRC_INTERNAL_FLAGS_CAPABILITIES_OFFSET)))

extern EmAfRf4ceZrcAttributes emAfRf4ceZrcLocalNodeAttributes;
extern EmAfRf4ceZrcAttributes emAfRf4ceZrcRemoteNodeAttributes;

// This API returns a pointer to the requested (attribute ID, entry ID) pair
// if it exists. Otherwise it returns NULL. The pairing index is used to
// distinguish between local and remote attributes. If the pairing index is
// 0xFF, then the API will look at the local attributes, otherwise it will look
// at the remote attributes corresponding to the passed pairing index.
uint8_t *emAfRf4ceZrcGetArrayedAttributePointer(uint8_t attrId,
                                              uint16_t entryId,
                                              uint8_t pairingIndex,
                                              uint8_t *index);

void emAfRf4ceZrcReadOrWriteAttribute(uint8_t pairingIndex,
                                      uint8_t attrId,
                                      uint16_t entryIdOrValueLength,
                                      bool isRead,
                                      uint8_t *val);

#define emAfRf4ceZrcWriteLocalAttribute(attrId, entryIdOrValueLength, val)     \
    emAfRf4ceZrcReadOrWriteAttribute(0xFF,                                     \
                                     (attrId),                                 \
                                     (entryIdOrValueLength),                   \
                                     false,                                    \
                                     (uint8_t*)(val))

#define emAfRf4ceZrcReadLocalAttribute(attrId, entryId, val)                   \
    emAfRf4ceZrcReadOrWriteAttribute(0xFF,                                     \
                                     (attrId),                                 \
                                     (entryId),                                \
                                     true,                                     \
                                     (uint8_t*)(val))

#define emAfRf4ceZrcWriteRemoteAttribute(pairingIndex,                         \
                                         attrId,                               \
                                         entryIdOrValueLength,                 \
                                         val)                                  \
    emAfRf4ceZrcReadOrWriteAttribute((pairingIndex),                           \
                                     (attrId),                                 \
                                     (entryIdOrValueLength),                   \
                                     false,                                    \
                                     (uint8_t*)(val))

#define emAfRf4ceZrcReadRemoteAttribute(pairingIndex, attrId, entryId, val)    \
    emAfRf4ceZrcReadOrWriteAttribute((pairingIndex),                           \
                                     (attrId),                                 \
                                     (entryId),                                \
                                     true,                                     \
                                     (uint8_t*)(val))
