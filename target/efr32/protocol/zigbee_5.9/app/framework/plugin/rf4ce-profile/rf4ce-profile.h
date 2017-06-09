// Copyright 2014 Silicon Laboratories, Inc.

#ifndef __RF4CE_PROFILE_H__
#define __RF4CE_PROFILE_H__

#include "rf4ce-profile-types.h"

/**
 * @addtogroup rf4ce-profile
 *
 * The RF4CE Profile Support plugin provides the necessary foundation of APIs
 * to interface with an RF4CE-capable device.
 *
 * The functionality contained in this plugin provides basic RF4CE networking
 * features like discovery, pairing, security, profile and device support,
 *  and transmission. In order to commence operations in an RF4CE network, one
 * must call ::emberAfRf4ceStart. This is the starting point for beginning
 * any RF4CE activity, over any profile. In the same sense, RF4CE network
 * operations can be stopped with a call to ::emberAfRf4ceStop.
 *
 * After network operations have been brought up as discussed above, one can
 * easily configure their device using APIs in this plugin, regardless of
 * what profile they are operating on. See
 * ::emberAfRf4ceSetPowerSavingParameters, ::emberAfRf4ceRxEnable,
 * ::emberAfRf4ceSetDiscoveryLqiThreshold, and ::emberAfRf4ceSetApplicationInfo
 * for examples of ways to do this.
 *
 * Once an RF4CE network has been started as described above, one can use the
 * ::emberAfDiscovery and ::emberAfRf4cePair APIs to initiate discovery and
 * pairing processes, respectively. This functionality is complemented by
 * various other discovery and pairing helper functions that aid in configuring
 * the mechanisms. For example, a device can call
 * ::emberAfRf4ceEnableAutoDiscoveryResponse to have the plugin handle
 * discovery request messages. There are also convenience macros to read
 * different information regarding the pairing table entries.
 *
 * Note that in this plugin, these functions are purposely very generic.
 * Please see @ref rf4ce-gdp, @ref rf4ce-zrc11, @ref rf4ce-zrc20, and @ref
 * rf4ce-mso for specific implementations of RF4CE profiles.
 *
 * @{
 */

/** @brief Determine if the current network is the RF4CE network.
 *
 * @return true if the current network is the RF4CE network or false otherwise.
 */
bool emberAfRf4ceIsCurrentNetwork(void);

/** @brief Set the current network to the RF4CE network.
 *
 * This function is a convenience wrapper for ::emberAfPushNetworkIndex.  Like
 * the other push APIs, every call to this API must be paired with a subsequent
 * call to ::emberAfPopNetworkIndex.  Note that is it not necessary to call
 * function before any of the emberAfRf4ce functions or in any
 * emberAfPluginRf4ce callbacks.  This function is intended primarily for
 * internal use, but is made available to the application in case it is useful
 * for application-specific behavior not generally supported by the plugin.
 *
 * @return An ::EmberStatus value that indicates the success or failure of the
 * command.
 */
EmberStatus emberAfRf4cePushNetworkIndex(void);

/** @brief Start the RF4CE network operations.
 *
 * The function is a convenience wrapper for ::emberRf4ceStart and
 * ::ezspRf4ceStart.  It will start the network operations according to the
 * plugin configuration.
 *
 * Note that this function automatically switches to the RF4CE network index
 * before calling any stack APIs and switches back to the previous network
 * index when it is finished.  Therefore, it is not necessary to push and pop
 * the RF4CE network index before and after calling this function.
 *
 * @return An ::EmberStatus value that indicates the success or failure of the
 * command.
 */
EmberStatus emberAfRf4ceStart(void);

/** @brief Set the power-saving parameters.
 *
 * Setting the duty cycle to zero disables power saving and will force the
 * receiver to be kept on.  It is equivalent to using ::emberAfRf4ceRxEnable
 * to set the wildcard profile to enabled.  Setting the duty cycle to non-
 * zero and the active period to zero will allow the receiver to be disabled
 * when all of the profiles are inacitve.  It is equivalent to setting the
 * wildcard profile to disabled.  Otherwise, when all profiles are inactive,
 * the receiver will duty cycle, will the receiver turned on for activePeriodMs
 * within each dutyCycleMs period.
 *
 * Note that this function automatically switches to the RF4CE network index
 * before calling any stack APIs and switches back to the previous network
 * index when it is finished.  Therefore, it is not necessary to push and pop
 * the RF4CE network index before and after calling this function.
 *
 * @return An ::EmberStatus value that indicates the success or failure of the
 * command.
 */
EmberStatus emberAfRf4ceSetPowerSavingParameters(uint32_t dutyCycleMs,
                                                 uint32_t activePeriodMs);

/** @brief Enable or disable the receiver for the given profile.
 *
 * Each profile can individually indicate whether it needs the receiver on or
 * if the receiver can be disabled to conserve power.  If at least one profile
 * requires the receiver to be on (e.g., when it expects to receive a message),
 * the plugin will enable the receiver.  Otherwise, the device will revert back
 * to the previously specified power-saving parameters.
 *
 * The profile ::EMBER_AF_RF4CE_PROFILE_WILDCARD may be used to set the default
 * state for the application.  Enabling the receiver for the wildcard profile
 * will force the receiver to be kept on.  Disabling the receiver for the
 * wildcard profile will permit the receiver to be turned off when all of the
 * other profiles are inactive.
 *
 * Note that this function automatically switches to the RF4CE network index
 * before calling any stack APIs and switches back to the previous network
 * index when it is finished.  Therefore, it is not necessary to push and pop
 * the RF4CE network index before and after calling this function.
 *
 * @return An ::EmberStatus value that indicates the success or failure of the
 * command.
 */
EmberStatus emberAfRf4ceRxEnable(EmberAfRf4ceProfileId profileId,
                                 bool enable);

/** @brief Set the frequency agility parameters.
 *
 * The function is a convenience wrapper for
 * ::emberRf4ceSetFrequencyAgilityParameters and
 * ::ezspRf4ceSetFrequencyAgilityParameters.
 *
 * Note that this function automatically switches to the RF4CE network index
 * before calling any stack APIs and switches back to the previous network
 * index when it is finished.  Therefore, it is not necessary to push and pop
 * the RF4CE network index before and after calling this function.
 *
 * @return An ::EmberStatus value that indicates the success or failure of the
 * command.
 */
EmberStatus emberAfRf4ceSetFrequencyAgilityParameters(uint8_t rssiWindowSize,
                                                      uint8_t channelChangeReads,
                                                      int8_t rssiThreshold,
                                                      uint16_t readIntervalS,
                                                      uint8_t readDuration);

/** @brief Set the discovery LQI threshold.
 *
 * The function is a convenience wrapper for
 * ::emberRf4ceSetDiscoveryLqiThreshold and
 * ::ezspSetValue(EZSP_VALUE_RF4CE_DISCOVERY_LQI_THRESHOLD, ...).
 *
 * Note that this function automatically switches to the RF4CE network index
 * before calling any stack APIs and switches back to the previous network
 * index when it is finished.  Therefore, it is not necessary to push and pop
 * the RF4CE network index before and after calling this function.
 *
 * @return An ::EmberStatus value that indicates the success or failure of the
 * command.
 */
EmberStatus emberAfRf4ceSetDiscoveryLqiThreshold(uint8_t threshold);


/** @brief Get the device RF4CE base channel.
 *
 * The function is a convenience wrapper for
 * ::emberRf4ceGetBaseChannel and
 * ::ezspGetValue(EZSP_VALUE_RF4CE_BASE_CHANNEL, ...).
 *
 * Note that this function automatically switches to the RF4CE network index
 * before calling any stack APIs and switches back to the previous network
 * index when it is finished.  Therefore, it is not necessary to push and pop
 * the RF4CE network index before and after calling this function.
 *
 * @return The device RF4CE base channel.
 */
uint8_t emberAfRf4ceGetBaseChannel(void);

/** @brief Start the discovery procedure.
 *
 * The function is a convenience wrapper for ::emberRf4ceDiscovery and
 * ::ezspRf4ceDiscovery.  The plugin will call the DiscoveryResponse callback
 * for any profile id that was requested if a response is received from a node
 * supporting that profile.  For any given response, discovery will continue if
 * at least one matching profile requests it.  Otherwise, the plugin instructs
 * the stack to end discovery at the conclusion of the current tria.  When
 * discovery completes, the plugin will call the DiscoveryComplete callback for
 * each profile id that was requested.
 *
 * Note that this function automatically switches to the RF4CE network index
 * before calling any stack APIs and switches back to the previous network
 * index when it is finished.  Therefore, it is not necessary to push and pop
 * the RF4CE network index before and after calling this function.
 *
 * @return An ::EmberStatus value that indicates the success or failure of the
 * command.
 */
EmberStatus emberAfRf4ceDiscovery(EmberPanId panId,
                                  EmberNodeId nodeId,
                                  uint8_t searchDevType,
                                  uint16_t discDurationMs,
                                  uint8_t maxDiscRepetitions,
                                  uint8_t discProfileIdListLength,
                                  uint8_t *discProfileIdList);

/** @brief Enable auto discovery mode.
 *
 * The function is a convenience wrapper for
 * ::emberRf4ceEnableAutoDiscoveryResponse and
 * ::ezspRf4ceEnableAutoDiscoveryResponse.  When auto discovery completes, the
 * plugin will call the AutoDiscoveryResponseComplete callback for each profile
 * id that was requested.
 *
 * Note that this function automatically switches to the RF4CE network index
 * before calling any stack APIs and switches back to the previous network
 * index when it is finished.  Therefore, it is not necessary to push and pop
 * the RF4CE network index before and after calling this function.
 *
 * @return An ::EmberStatus value that indicates the success or failure of the
 * command.
 */
EmberStatus emberAfRf4ceEnableAutoDiscoveryResponse(uint16_t durationMs,
                                                    uint8_t discProfileIdListLength,
                                                    uint8_t *discProfileIdList);

/** @brief Start the pairing procedure.
 *
 * The function is a convenience wrapper for ::emberRf4cePair and
 * ::ezspRf4cePair.  When pairing completes, if a callback was specified in the
 * API call (that is, callback was a non-NULL function pointer), the passed
 * callback shall be called, otherwise the plugin will call the PairComplete
 * callback for each profile id.
 *
 * Note that this function automatically switches to the RF4CE network index
 * before calling any stack APIs and switches back to the previous network
 * index when it is finished.  Therefore, it is not necessary to push and pop
 * the RF4CE network index before and after calling this function.
 *
 * @return An ::EmberStatus value that indicates the success or failure of the
 * command.
 */
EmberStatus emberAfRf4cePair(uint8_t channel,
                             EmberPanId panId,
                             EmberEUI64 ieeeAddr,
                             uint8_t keyExchangeTransferCount,
                             EmberAfRf4cePairCompleteCallback pairCompleteCallback);

/** @brief Get the pairing index of the incoming or sent message.
 *
 * The function returns the pairing index of the sender of the current incoming
 * message or the destination of the current outgoing message.  This function
 * can only be called in the context of ::emberRf4ceIncomingMessageHandler,
 * ::ezspRf4ceIncomingMessageHandler, ::emberRf4ceMessageSentHandler, or
 * ::ezspRf4ceMessageSentHandler.
 *
 * Note that this function automatically switches to the RF4CE network index
 * before calling any stack APIs and switches back to the previous network
 * index when it is finished.  Therefore, it is not necessary to push and pop
 * the RF4CE network index before and after calling this function.
 *
 * @return The pairing index of the incoming or outgoing message or 0xFF if
 * called from outside the context of ::emberRf4ceIncomingMessageHandler,
 * ::ezspRf4ceIncomingMessageHandler, ::emberRf4ceMessageSentHandler, or
 * ::ezspRf4ceMessageSentHandler.
 */
uint8_t emberAfRf4ceGetPairingIndex(void);

/** @brief Set the pairing table entry at a particular index.
 *
 * The function is a convenience wrapper for ::emberRf4ceSetPairingTableEntry
 * and ::ezspRf4ceSetPairingTableEntry.
 *
 * Note that this function automatically switches to the RF4CE network index
 * before calling any stack APIs and switches back to the previous network
 * index when it is finished.  Therefore, it is not necessary to push and pop
 * the RF4CE network index before and after calling this function.
 *
 * @return An ::EmberStatus value that indicates the success or failure of the
 * command.
 */
EmberStatus emberAfRf4ceSetPairingTableEntry(uint8_t pairingIndex,
                                             EmberRf4cePairingTableEntry *entry);

/** @brief Get the pairing table entry at a particular index.
 *
 * The function is a convenience wrapper for ::emberRf4ceGetPairingTableEntry
 * and ::ezspRf4ceGetPairingTableEntry.
 *
 * Note that this function automatically switches to the RF4CE network index
 * before calling any stack APIs and switches back to the previous network
 * index when it is finished.  Therefore, it is not necessary to push and pop
 * the RF4CE network index before and after calling this function.
 *
 * @return An ::EmberStatus value that indicates the success or failure of the
 * command.
 */
EmberStatus emberAfRf4ceGetPairingTableEntry(uint8_t pairingIndex,
                                             EmberRf4cePairingTableEntry *entry);

/** @brief Set the node application information.
 *
 * The function is a convenience wrapper for ::emberRf4ceSetApplicationInfo
 * and ::ezspRf4ceSetApplicationInfo.
 *
 * Note that this function automatically switches to the RF4CE network index
 * before calling any stack APIs and switches back to the previous network
 * index when it is finished.  Therefore, it is not necessary to push and pop
 * the RF4CE network index before and after calling this function.
 *
 * @return An ::EmberStatus value that indicates the success or failure of the
 * command.
 */
EmberStatus emberAfRf4ceSetApplicationInfo(EmberRf4ceApplicationInfo *appInfo);

/** @brief Get the node application information.
 *
 * The function is a convenience wrapper for ::emberRf4ceGetApplicationInfo
 * and ::ezspRf4ceGetApplicationInfo.
 *
 * Note that this function automatically switches to the RF4CE network index
 * before calling any stack APIs and switches back to the previous network
 * index when it is finished.  Therefore, it is not necessary to push and pop
 * the RF4CE network index before and after calling this function.
 *
 * @return An ::EmberStatus value that indicates the success or failure of the
 * command.
 */
EmberStatus emberAfRf4ceGetApplicationInfo(EmberRf4ceApplicationInfo *appInfo);

/** @brief Update the link key of a pairing table entry.
 *
 * The function is a convenience wrapper for ::emberRf4ceKeyUpdate and
 * ::ezspRf4ceKeyUpdate.
 *
 * Note that this function automatically switches to the RF4CE network index
 * before calling any stack APIs and switches back to the previous network
 * index when it is finished.  Therefore, it is not necessary to push and pop
 * the RF4CE network index before and after calling this function.
 *
 * @return An ::EmberStatus value that indicates the success or failure of the
 * command.
 */
EmberStatus emberAfRf4ceKeyUpdate(uint8_t pairingIndex, EmberKeyData *key);

/** @brief Send a message to a pairing index.
 *
 * The function is a convenience wrapper for ::emberRf4ceSend and
 * ::ezspRf4ceSend.  If the pairing index is
 * ::EMBER_RF4CE_PAIRING_TABLE_BROADCAST_INDEX, the plugin will broadcast the
 * message.  Otherwise, the plugin will send a unicast.  For unicasts, the
 * plugin automatically enables security if the remote node is security capable
 * and if a link key exists to the node.  If the local node is a target and the
 * remote node supports channel normalization, the plugin will automatically
 * set the channel designator option in the outgoing message.  The plugin
 * always requests acknowledgements for unicast messages.
 *
 * The plugin will allocate a new message tag for the message and, if
 * messageTag is not NULL, store it in the address pointed to by messageTag.
 * See ::EMBER_AF_RF4CE_MESSAGE_TAG_MASK for more information about the message
 * tags allocated by the plugin.
 *
 * Note that this function automatically switches to the RF4CE network index
 * before calling any stack APIs and switches back to the previous network
 * index when it is finished.  Therefore, it is not necessary to push and pop
 * the RF4CE network index before and after calling this function.
 *
 * @return An ::EmberStatus value that indicates the success or failure of the
 * command.
 */
EmberStatus emberAfRf4ceSend(uint8_t pairingIndex,
                             uint8_t profileId,
                             uint8_t *message,
                             uint8_t messageLength,
                             uint8_t *messageTag);

/** @brief Send a vendor-specific message to a pairing index.
 *
 * The function is a convenience wrapper for ::emberRf4ceSend and
 * ::ezspRf4ceSend.  If the pairing index is
 * ::EMBER_RF4CE_PAIRING_TABLE_BROADCAST_INDEX, the plugin will broadcast the
 * message.  Otherwise, the plugin will send a unicast.  For unicasts, the
 * plugin automatically enables security if the remote node is security capable
 * and if a link key exists to the node.  If the local node is a target and the
 * remote node supports channel normalization, the plugin will automatically
 * set the channel designator option in the outgoing message.  The plugin
 * always requests acknowledgements for unicast messages.
 *
 * The plugin will allocate a new message tag for the message and, if
 * messageTag is not NULL, store it in the address pointed to by messageTag.
 * See ::EMBER_AF_RF4CE_MESSAGE_TAG_MASK for more information about the message
 * tags allocated by the plugin.
 *
 * Note that this function automatically switches to the RF4CE network index
 * before calling any stack APIs and switches back to the previous network
 * index when it is finished.  Therefore, it is not necessary to push and pop
 * the RF4CE network index before and after calling this function.
 *
 * @return An ::EmberStatus value that indicates the success or failure of the
 * command.
 */
EmberStatus emberAfRf4ceSendVendorSpecific(uint8_t pairingIndex,
                                           uint8_t profileId,
                                           uint16_t vendorId,
                                           uint8_t *message,
                                           uint8_t messageLength,
                                           uint8_t *messageTag);

/** @brief Send a message to a pairing index specifying the transmission options
 *  as described in the RF4CE specifications.
 *
 * The function is a convenience wrapper for ::emberRf4ceSend and
 * ::ezspRf4ceSend.  If the pairing index is
 * ::EMBER_RF4CE_PAIRING_TABLE_BROADCAST_INDEX, the plugin will broadcast the
 * message.  Otherwise, the plugin will send a unicast.
 *
 * The plugin will allocate a new message tag for the message and, if
 * messageTag is not NULL, store it in the address pointed to by messageTag.
 * See ::EMBER_AF_RF4CE_MESSAGE_TAG_MASK for more information about the message
 * tags allocated by the plugin.
 *
 * Note that this function automatically switches to the RF4CE network index
 * before calling any stack APIs and switches back to the previous network
 * index when it is finished.  Therefore, it is not necessary to push and pop
 * the RF4CE network index before and after calling this function.
 *
 * @return An ::EmberStatus value that indicates the success or failure of the
 * command.
 */
EmberStatus emberAfRf4ceSendExtended(uint8_t pairingIndex,
                                     uint8_t profileId,
                                     uint16_t vendorId,
                                     EmberRf4ceTxOption txOptions,
                                     uint8_t *message,
                                     uint8_t messageLength,
                                     uint8_t *messageTag);

/** @brief Get the default TX options for the pairing index.
 *
 * Note that this function automatically switches to the RF4CE network index
 * before calling any stack APIs and switches back to the previous network
 * index when it is finished.  Therefore, it is not necessary to push and pop
 * the RF4CE network index before and after calling this function.
 *
 * @param pairingIndex The pairing index to check.
 * @param txOptions A pointer to the ::EmberRf4ceTxOption to be populated.
 *
 * @return An ::EmberStatus value that indicates the success of failure or the
 * command.
 */
EmberStatus emberAfRf4ceGetDefaultTxOptions(uint8_t pairingIndex,
                                            EmberRf4ceTxOption *txOptions);

/** @brief Remove a pairing.
 *
 * The function is a convenience wrapper for ::emberRf4ceUnpair and
 * ::ezspRf4ceUnpair.  When unpairing completes, the plugin will call
 * UnpairComplete callback for each profile id.
 *
 * Note that this function automatically switches to the RF4CE network index
 * before calling any stack APIs and switches back to the previous network
 * index when it is finished.  Therefore, it is not necessary to push and pop
 * the RF4CE network index before and after calling this function.
 *
 * @return An ::EmberStatus value that indicates the success or failure of the
 * command.
 */
EmberStatus emberAfRf4ceUnpair(uint8_t pairingIndex);

/** @brief Stop the RF4CE network operations.
 *
 * The function is a convenience wrapper for ::emberRf4ceStop and
 * ::ezspRf4ceStop.
 *
 * Note that this function automatically switches to the RF4CE network index
 * before calling any stack APIs and switches back to the previous network
 * index when it is finished.  Therefore, it is not necessary to push and pop
 * the RF4CE network index before and after calling this function.
 *
 * @return An ::EmberStatus value that indicates the success or failure of the
 * command.
 */
EmberStatus emberAfRf4ceStop(void);

/** @brief Returns the maximum RF4CE network layer payload.
 *
 * The function is a convenience wrapper for ::emberRf4ceGetMaxPayload and
 * ::ezspRf4ceGetMaxPayload.
 *
 * Note that this function automatically switches to the RF4CE network index
 * before calling any stack APIs and switches back to the previous network
 * index when it is finished.  Therefore, it is not necessary to push and pop
 * the RF4CE network index before and after calling this function.
 *
 * @return The maximum allowed length in bytes of the RF4CE network layer
 * payload according to the passed pairing index and TX options.
 */
uint8_t emberAfRf4ceGetMaxPayload(uint8_t pairingIndex,
                                EmberRf4ceTxOption txOptions);

#ifdef DOXYGEN_SHOULD_SKIP_THIS
  /** @brief Get the length of the device type list from the application
   * capabilities bitmask.
   *
   * Implemented as a macro for efficiency.
   *
   * @return The length of the device type list.
   */
  uint8_t emberAfRf4ceDeviceTypeListLength(EmberRf4ceApplicationCapabilities capabilities);
#else
  #define emberAfRf4ceDeviceTypeListLength(capabilities)                         \
    (((capabilities) & EMBER_RF4CE_APP_CAPABILITIES_SUPPORTED_DEVICE_TYPES_MASK) \
     >> EMBER_RF4CE_APP_CAPABILITIES_SUPPORTED_DEVICE_TYPES_OFFSET)
#endif

#ifdef DOXYGEN_SHOULD_SKIP_THIS
  /** @brief Get the length of the profile id list from the application
   * capabilities bitmask.
   *
   * Implemented as a macro for efficiency.
   *
   * @return The length of the profile id list.
   */
  uint8_t emberAfRf4ceProfileIdListLength(EmberRf4ceApplicationCapabilities capabilities);
#else
  #define emberAfRf4ceProfileIdListLength(capabilities)                      \
    (((capabilities) & EMBER_RF4CE_APP_CAPABILITIES_SUPPORTED_PROFILES_MASK) \
     >> EMBER_RF4CE_APP_CAPABILITIES_SUPPORTED_PROFILES_OFFSET)
#endif

#ifdef DOXYGEN_SHOULD_SKIP_THIS
  /** @brief Get the vendor if of the local application.
   *
   * Implemented as a macro for efficiency.
   *
   * @return The vendor id.
   */
  uint16_t emberAfRf4ceVendorId(void);
#else
  #define emberAfRf4ceVendorId() (emAfRf4ceVendorInfo.vendorId + 0)
#endif

/** @brief Determine if the application implements a particular device type.
 *
 * If the device type is ::EMBER_AF_RF4CE_DEVICE_TYPE_WILDCARD, this function
 * always returns true.
 *
 * @return true if the application implements the device type or false
 * otherwise.
 */
bool emberAfRf4ceIsDeviceTypeSupported(const EmberRf4ceApplicationInfo *appInfo,
                                          EmberAfRf4ceDeviceType deviceType);

#ifdef DOXYGEN_SHOULD_SKIP_THIS
  /** @brief Determine if the local application implements a particular device
   * type.
   *
   * If the device type is ::EMBER_AF_RF4CE_DEVICE_TYPE_WILDCARD, this function
   * always returns true.
   *
   * Implemented as a macro for efficiency.
   *
   * @return true if the application implements the device type or false
   * otherwise.
   */
   bool emberAfRf4ceIsDeviceTypeSupportedLocally(EmberAfRf4ceDeviceType deviceType);
#else
  #define emberAfRf4ceIsDeviceTypeSupportedLocally(deviceType) \
    emberAfRf4ceIsDeviceTypeSupported(&emAfRf4ceApplicationInfo, deviceType)
#endif

/** @brief Determine if the application implements a particular profile.
 *
 * @return true if the application implements the profile or false otherwise.
 */
bool emberAfRf4ceIsProfileSupported(const EmberRf4ceApplicationInfo *appInfo,
                                       EmberAfRf4ceProfileId profileId);

#ifdef DOXYGEN_SHOULD_SKIP_THIS
  /** @brief Determine if the local application implements a particular
   * profile.
   *
   * Implemented as a macro for efficiency.
   *
   * @return true if the application implements the profile or false otherwise.
   */
   bool emberAfRf4ceIsProfileSupportedLocally(EmberAfRf4ceProfileId profileId);
#else
  #define emberAfRf4ceIsProfileSupportedLocally(profileId) \
    emberAfRf4ceIsProfileSupported(&emAfRf4ceApplicationInfo, profileId)
#endif

#ifdef DOXYGEN_SHOULD_SKIP_THIS
  /** @brief Determine if the pairing table entry is unused.
   *
   * Implemented as a macro for efficiency.
   *
   * @return true if the pairing table entry is unused or false otherwise.
   */
  bool emberAfRf4cePairingTableEntryIsUnused(const EmberRf4cePairingTableEntry *pairingTableEntry);
#else
  #define emberAfRf4cePairingTableEntryIsUnused(pairingTableEntry) \
    (READBITS((pairingTableEntry)->info,                           \
              EMBER_RF4CE_PAIRING_TABLE_ENTRY_INFO_STATUS_MASK)    \
     == EMBER_RF4CE_PAIRING_TABLE_ENTRY_STATUS_UNUSED)
#endif

#ifdef DOXYGEN_SHOULD_SKIP_THIS
 /** @brief Determine if the pairing table entry is provisional.
  *
  * Implemented as a macro for efficiency.
  *
  * @return true if the pairing table entry is provisional or false otherwise.
  */
  bool emberAfRf4cePairingTableEntryIsProvisional(const EmberRf4cePairingTableEntry *pairingTableEntry);
#else
  #define emberAfRf4cePairingTableEntryIsProvisional(pairingTableEntry) \
    (READBITS((pairingTableEntry)->info,                                \
              EMBER_RF4CE_PAIRING_TABLE_ENTRY_INFO_STATUS_MASK)         \
     == EMBER_RF4CE_PAIRING_TABLE_ENTRY_STATUS_PROVISIONAL)
#endif

#ifdef DOXYGEN_SHOULD_SKIP_THIS
 /** @brief Determine if the pairing table entry is active.
  *
  * Implemented as a macro for efficiency.
  *
  * @return true if the pairing table entry is active or false otherwise.
  */
  bool emberAfRf4cePairingTableEntryIsActive(const EmberRf4cePairingTableEntry *pairingTableEntry);
#else
  #define emberAfRf4cePairingTableEntryIsActive(pairingTableEntry) \
    (READBITS((pairingTableEntry)->info,                           \
              EMBER_RF4CE_PAIRING_TABLE_ENTRY_INFO_STATUS_MASK)    \
     == EMBER_RF4CE_PAIRING_TABLE_ENTRY_STATUS_ACTIVE)
#endif

#ifdef DOXYGEN_SHOULD_SKIP_THIS
 /** @brief Determine if the pairing table entry has a link key.
  *
  * Implemented as a macro for efficiency.
  *
  * @return true if the pairing table entry has a link key or false otherwise.
  */
  bool emberAfRf4cePairingTableEntryHasLinkKey(const EmberRf4cePairingTableEntry *pairingTableEntry);
#else
  #define emberAfRf4cePairingTableEntryHasLinkKey(pairingTableEntry) \
    (READBITS((pairingTableEntry)->info,                             \
              EMBER_RF4CE_PAIRING_TABLE_ENTRY_INFO_HAS_LINK_KEY_BIT) \
     == EMBER_RF4CE_PAIRING_TABLE_ENTRY_INFO_HAS_LINK_KEY_BIT)
#endif

#ifdef DOXYGEN_SHOULD_SKIP_THIS
 /** @brief Determine if the local node was the pairing initiator for the
  * pairing table entry.
  *
  * Implemented as a macro for efficiency.
  *
  * @return true if the local node was the pairing initiator for the pairing
  * table entry or false otherwise.
  */
  bool emberAfRf4cePairingTableEntryIsPairingInitiator(const EmberRf4cePairingTableEntry *pairingTableEntry);
#else
  #define emberAfRf4cePairingTableEntryIsPairingInitiator(pairingTableEntry) \
    (READBITS((pairingTableEntry)->info,                                     \
              EMBER_RF4CE_PAIRING_TABLE_ENTRY_INFO_IS_PAIRING_INITIATOR_BIT) \
     == EMBER_RF4CE_PAIRING_TABLE_ENTRY_INFO_IS_PAIRING_INITIATOR_BIT)
#endif

#ifdef DOXYGEN_SHOULD_SKIP_THIS
 /** @brief Determine if the pairing table entry is security capable.
  *
  * Implemented as a macro for efficiency.
  *
  * @return true if the pairing table entry is security capable or false
  * otherwise.
  */
  bool emberAfRf4cePairingTableEntryHasSecurity(const EmberRf4cePairingTableEntry *pairingTableEntry);
#else
  #define emberAfRf4cePairingTableEntryHasSecurity(pairingTableEntry) \
    (READBITS((pairingTableEntry)->capabilities,                      \
              EMBER_RF4CE_NODE_CAPABILITIES_SECURITY_BIT)             \
     == EMBER_RF4CE_NODE_CAPABILITIES_SECURITY_BIT)
#endif

#ifdef DOXYGEN_SHOULD_SKIP_THIS
 /** @brief Determine if the pairing table entry supports channel
  * normalization.
  *
  * Implemented as a macro for efficiency.
  *
  * @return true if the pairing table entry supports channel normalization or
  * false otherwise.
  */
  bool emberAfRf4cePairingTableEntryHasChannelNormalization(const EmberRf4cePairingTableEntry *pairingTableEntry);
#else
  #define emberAfRf4cePairingTableEntryHasChannelNormalization(pairingTableEntry) \
    (READBITS((pairingTableEntry)->capabilities,                                  \
              EMBER_RF4CE_NODE_CAPABILITIES_CHANNEL_NORM_BIT)                     \
     == EMBER_RF4CE_NODE_CAPABILITIES_CHANNEL_NORM_BIT)
#endif

#ifndef DOXYGEN_SHOULD_SKIP_THIS
extern EmberRf4ceVendorInfo emAfRf4ceVendorInfo;
extern EmberRf4ceApplicationInfo emAfRf4ceApplicationInfo;
#endif

// There are a maximum of 128 bytes at the PHY layer, but there is a one-byte
// length and a two-byte CRC, leaving 125 for the PHY payload.  The minimum MAC
// header consists of a two-byte frame control, one-byte sequence number, two-
// byte destination pan id, and at least two bytes each for the source and
// destination addresses.  That leaves a maximum of 116 bytes for the MAC
// payload, although inter-pan messages or long source or destination addresses
// will reduce that.  The minimum NWK header consists of a one-byte frame
// control, four-byte frame counter, and one-byte profile id.  That leaves a
// maximum of 110 bytes for the NWK payload, although encrypted or vendor-
// specific messages will reduce that. Use the ::emberAfRf4ceGetMaxPayload()
// API to retrieve the maximum allowed payload for a certain pairing and certain
// TX options.
#define EMBER_AF_RF4CE_MAXIMUM_RF4CE_PAYLOAD_LENGTH 110

/** @brief The mask applied by ::emberAfRf4ceSend,
 * ::emberAfRf4ceSendVendorSpecific, and ::emberAfRf4ceSendExtended when
 * allocating a new message tag to an outgoing message.  Customers who call
 * ::emberRf4ceSend or ::ezspRf4ceSend directly must use message tags outside
 * this mask.
 */
#define EMBER_AF_RF4CE_MESSAGE_TAG_MASK 0x7F

#endif // __RF4CE_PROFILE_H__

// @} END addtogroup
