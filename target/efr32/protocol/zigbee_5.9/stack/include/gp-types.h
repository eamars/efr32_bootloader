/**
 * @file gp-types.h
 * @brief Zigbee Green Power types and defines.
 * See @ref greenpower for documentation.
 *
 * <!--Copyright 2015 Silicon Laboratories, Inc.                         *80*-->
 */

/**
 * @addtogroup gp_types
 *
 * See gp-types.h for source code.
 * @{
 */

#ifndef __GP_TYPES_H__
#define __GP_TYPES_H__

/**
 * @name GP Types
 */
//@{
/** 32-bit source identifier. */
typedef uint32_t EmberGpSourceId;
/** 32-bit security frame counter */
typedef uint32_t EmberGpSecurityFrameCounter;
typedef uint32_t EmberGpMic;

/** @brief Options to use when sending a message. */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberGpSecurityLevel
#else
typedef uint8_t EmberGpSecurityLevel;
enum
#endif
{
  /** None  */
  EMBER_GP_SECURITY_LEVEL_NONE = 0x00,
  /** reserved  */
  EMBER_GP_SECURITY_LEVEL_RESERVED = 0x01,
  /** 4 Byte Frame Counter + 4 Byte MIC */
  EMBER_GP_SECURITY_LEVEL_FC_MIC = 0x02,
  /** 4 Byte Frame Counter + 4 Byte MIC + encryption */
  EMBER_GP_SECURITY_LEVEL_FC_MIC_ENCRYPTED = 0x03,
};

/** @brief Options to use when sending a message. */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberGpKeyType
#else
typedef uint8_t EmberGpKeyType;
enum
#endif
{
  /** None  */
  EMBER_GP_SECURITY_KEY_NONE = 0x00,
  /** reserved  */
  EMBER_GP_SECURITY_KEY_NWK = 0x01,
  EMBER_GP_SECURITY_KEY_GPD_GROUP = 0x02,
  EMBER_GP_SECURITY_KEY_NWK_DERIVED = 0x03,
  EMBER_GP_SECURITY_KEY_GPD_OOB = 0x04,
  EMBER_GP_SECURITY_KEY_GPD_DERIVED = 0x07,
};

/** @brief Options to use when sending a message. */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberGpApplicationId
#else
typedef uint8_t EmberGpApplicationId;
enum
#endif
{
  /** Source identifier. */
  EMBER_GP_APPLICATION_SOURCE_ID = 0x00,
  /** IEEE address. */
  EMBER_GP_APPLICATION_IEEE_ADDRESS = 0x02,
};

/**
 * @brief GP proxy table entry status.
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberGpProxyTableEntryStatus
#else
typedef uint8_t EmberGpProxyTableEntryStatus;
enum
#endif
{
  /**
   * The  pairing table entry is in use.
   */
  EMBER_GP_PROXY_TABLE_ENTRY_STATUS_ACTIVE = 0x01,
  /**
   * The proxy table entry is not in use.
   */
  EMBER_GP_PROXY_TABLE_ENTRY_STATUS_UNUSED = 0xFF,
};

/** @brief Address for sending and receiving a message. */
typedef struct {
  union {
    /** The IEEE address is used when the application identifier is
     *  ::EMBER_GP_APPLICATION_IEEE_ADDRESS.
     */
    EmberEUI64 gpdIeeeAddress;
    /** The 32-bit source identifier is used when the application identifier is
     *  ::EMBER_GP_APPLICATION_SOURCE_ID.
     *
     */
    EmberGpSourceId sourceId;
  } id;
  /** Application identifier of the GPD. */
  EmberGpApplicationId applicationId;
  uint8_t endpoint;
} EmberGpAddress;

typedef enum {
  EMBER_GP_SINK_TYPE_FULL_UNICAST,
  EMBER_GP_SINK_TYPE_D_GROUPCAST,
  EMBER_GP_SINK_TYPE_GROUPCAST,
  EMBER_GP_SINK_TYPE_LW_UNICAST,

  EMBER_GP_SINK_TYPE_UNUSED = 0xFF
} EmberGpSinkType;

typedef struct {
  EmberEUI64 sinkEUI;
  EmberNodeId sinkNodeId;
} EmberGpSinkAddress;

typedef struct {
  uint16_t groupID;
  uint16_t alias;
} EmberGpSinkGroup;

typedef struct {
  EmberGpSinkType type;

  union {
    EmberGpSinkAddress unicast;
    EmberGpSinkGroup groupcast;
  } target;

} EmberGpSinkListEntry;

//minimum required by the spec is 2
#define GP_SINK_LIST_ENTRIES 2

/** @brief The internal representation of a proxy table entry.
 */
typedef struct {
  /**
   * Internal status of the proxy table entry
   */
  EmberGpProxyTableEntryStatus status;
  /**
   * The tunneling options (this contains both options and extendedOptions from the spec)
   */
  uint32_t options;
  /**
   * The addressing info of the GPD
   */
  EmberGpAddress gpd;
  /**
   * The assigned alias for the GPD
   */
  EmberNodeId assignedAlias;
  /**
   * The security options field
   */
  uint8_t securityOptions;
  /**
   * The SFC of the GPD
   */
  EmberGpSecurityFrameCounter gpdSecurityFrameCounter;
  /**
   * The key
   */
  EmberKeyData gpdKey;
  /**
   * The list of sinks  (hardcoded to 2 which is the spec minimum)  (maybe we should indirect this?)
   */
  EmberGpSinkListEntry sinkList[GP_SINK_LIST_ENTRIES];
  /**
   * The groupcast radius
   */
  uint8_t groupcastRadius;
  /**
   * The search counter
   */
  uint8_t searchCounter;
} EmberGpProxyTableEntry;

/**
 * @brief GP sink table entry status.
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberGpSinkTableEntryStatus
#else
typedef uint8_t EmberGpSinkTableEntryStatus;
enum
#endif
{
  /**
   * The  pairing table entry is in use.
   */
  EMBER_GP_SINK_TABLE_ENTRY_STATUS_ACTIVE = 0x01,
  /**
   * The proxy table entry is not in use.
   */
  EMBER_GP_SINK_TABLE_ENTRY_STATUS_UNUSED = 0xFF,
};

/** @brief Options to use when sending a message. */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberCGpTxOption
#else
typedef uint8_t EmberCGpTxOption;
enum
#endif
{
  /** No options. */
  EMBER_CGP_TX_OPTION_NONE = 0x00,
  /** Use CSMA/CA. */
  EMBER_CGP_TX_OPTION_USE_CSMA_CA = 0x01,
  /** Use MAC ACK. */
  EMBER_CGP_TX_OPTION_USE_MAC_ACK = 0x02,
  /** Reserved. */
  EMBER_CGP_TX_OPTION_RESERVED = 0xFC,
};

/** @brief Addressing modes for sending and receiving a message. */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberCGpAddressMode
#else
typedef uint8_t EmberCGpAddressMode;
enum
#endif
{
  /** No address (PAN identifier and address omitted). */
  EMBER_CGP_ADDRESS_MODE_NONE = 0x00,
  /** Reserved. */
  EMBER_CGP_ADDRESS_MODE_RESERVED = 0x01,
  /** 16-bit short address. */
  EMBER_CGP_ADDRESS_MODE_SHORT = 0x02,
  /** 64-bit extended address. */
  EMBER_CGP_ADDRESS_MODE_EXTENDED = 0x03,
};

/** @brief Address for sending and receiving a message. */
typedef struct {
  /** The address. */
  union {
    /** The 16-bit short address is used when the mode is
     *  ::EMBER_CGP_ADDRESS_MODE_SHORT.
     */
    EmberNodeId shortId;
    /** The 64-bit extended address is used when the mode is
     *  ::EMBER_CGP_ADDRESS_MODE_EXTENDED.
     */
    EmberEUI64 extendedId;
  } address;
  /** The PAN identifier is used when the mode is not
   *  ::EMBER_CGP_ADDRESS_MODE_NONE.
   */
  EmberPanId panId;
  /** The addressing mode. */
  EmberCGpAddressMode mode;
} EmberCGpAddress;

/*S @brief Options to use when sending a message. */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
enum EmberDGpTxOption
#else
typedef uint8_t EmberDGpTxOption;
enum
#endif
{
  /** No options. */
  EMBER_DGP_TX_OPTION_NONE = 0x00,
  /** Use gpTxQueue. */
  EMBER_DGP_TX_OPTION_USE_GP_TX_QUEUE = 0x01,
  /** Use CSMA/CA. */
  EMBER_DGP_TX_OPTION_USE_CSMA_CA = 0x02,
  /** Use MAC ACK. */
  EMBER_DGP_TX_OPTION_USE_MAC_ACK = 0x04,
  /** Data frame. */
  EMBER_DGP_TX_OPTION_FRAME_TYPE_DATA = 0x00,
  /** Maintenance frame. */
  EMBER_DGP_TX_OPTION_FRAME_TYPE_MAINTENANCE = 0x08,
  /** Reserved. */
  EMBER_DGP_TX_OPTION_RESERVED = 0xE0,
};


typedef struct {
  EmberGpSinkTableEntryStatus status;
  uint16_t options;
  EmberGpAddress gpd;
  uint8_t deviceId;
  //optional Group List
  EmberNodeId assignedAlias;
  uint8_t groupcastRadius;
  uint8_t securityOptions;
  uint32_t gpdSecurityFrameCounter;
  EmberKeyData gpdKey;
} EmberGpSinkTableEntry;

typedef struct {
  uint8_t foo;

} EmberGpProxyClusterAttributes;

typedef struct {
  bool inUse;
  bool useCca;
  EmberGpAddress addr;
  uint8_t gpdCommandId;
  EmberMessageBuffer asdu;
  uint8_t gpepHandle;
  uint16_t queueEntryLifetimeMs;

} EmberGpTxQueueEntry;

//these defines are ugly, but there are a bunch of callbacks that take the same set of arguments
//while the design is still in flux, it makes sense to only have one place to change them

#define GP_PARAMS \
    EmberStatus status,\
    uint8_t gpdLink,\
    uint8_t sequenceNumber,\
    EmberGpAddress *addr,\
    EmberGpSecurityLevel gpdfSecurityLevel,\
    EmberGpKeyType gpdfSecurityKeyType,\
    bool autoCommissioning,\
    bool rxAfterTx,\
    uint32_t gpdSecurityFrameCounter,\
    uint8_t gpdCommandId,\
    uint32_t mic,\
    EmberGpSinkListEntry *sinkList, \
    uint8_t gpdCommandPayloadLength,\
    uint8_t *gpdCommandPayload

#define GP_PROXY_TABLE_OPTIONS_IN_RANGE (BIT(10))

#define GP_ARGS status,gpdLink,sequenceNumber,addr,gpdfSecurityLevel,gpdfSecurityKeyType,autoCommissioning,rxAfterTx,gpdSecurityFrameCounter,gpdCommandId,mic,sinkList,gpdCommandPayloadLength,gpdCommandPayload

//3 consumed by device id, options, and app info byte
#define GP_COMMISSIONING_MAX_BYTES (55 - 3)

typedef struct {
  uint8_t deviceId;
  uint16_t manufacturerId;
  uint16_t modelId;
  uint8_t gpdCommands[GP_COMMISSIONING_MAX_BYTES-1];
  uint16_t serverClusters[15];
  uint16_t clientClusters[15];
} EmberGpApplicationInfo;

//@} \\END GP Types

#define GP_DMIN_B 32
#define GP_DMIN_U 5
#define GP_DMAX 100

EmberStatus emberDGpSend(bool action,
                         bool useCca,
                         EmberGpAddress *addr,
                         uint8_t gpdCommandId,
                         uint8_t gpdAsduLength,
                         uint8_t const *gpdAsdu,
                         uint8_t gpepHandle,
                         uint16_t gpTxQueueEntryLifetimeMs);

bool emberGpProxyTableProcessGpPairing(uint32_t options,
                                       EmberGpAddress* addr,
                                       uint8_t commMode,
                                       uint16_t sinkNwkAddress,
                                       uint16_t sinkGroupId,
                                       uint16_t assignedAlias,
                                       uint8_t* sinkIeeeAddress,
                                       EmberKeyData* gpdKey);


#endif // __GP_TYPES_H__

/** @} // END addtogroup
 */
