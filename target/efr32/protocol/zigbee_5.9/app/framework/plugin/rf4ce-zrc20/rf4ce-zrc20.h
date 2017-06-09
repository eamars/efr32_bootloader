// Copyright 2014 Silicon Laboratories, Inc.

#ifndef __RF4CE_ZRC20_H__
#define __RF4CE_ZRC20_H__

#include "rf4ce-zrc20-types.h"

/**
 * @addtogroup rf4ce-zrc20
 *
 * The ZigBee Remote Control 2.0 (ZRC2.0) plugin implements the ZRC2.0 profile.
 * Compared to versions 1.0 and 1.1 of the profile, ZRC2.0 describes a more
 * complex discovery, pairing, configuration, and validation procedure for
 * binding devices and a more robust mechanism for controlling devices via
 * action commands.  The ZRC2.0 profile is based on the Generic Device Profile
 * (GDP).  In conjunction with the @ref rf4ce-gdp plugin, this plugin manages
 * these procedures for both originators and recipients.  Note that this plugin
 * implements version 2.0 of the ZRC profile.  Version 2.0 devices are fully
 * interoperable with version 1.0 and 1.1 devices.  (ZRC1.0 was also known as
 * the Consumer Electronics Remote Control (CERC) profile.)  As required by the
 * specification, this plugin sends ZRC1.1-formatted messages to ZRC1.x devices
 * and ZRC2.0-formatted messages to ZRC2.0 devices.  It receives ZRC1.0-,
 * ZRC1.1-, and ZRC2.0-formatted messages.
 *
 * This plugin supports originators and recipients.  Originators send action
 * messages to recipients after binding with them.  Originators are most
 * typically controllers but may also be targets.  Recipients must be targets.
 * Both originators and recipients must start general network operations before
 * beginning ZRC-specific operations.  Network operations should be started by
 * calling ::emberAfRf4ceStart in the @ref rf4ce-profile plugin.
 *
 * Once network operations have started, ZRC2.0 originators can initiate the
 * binding procedure by calling ::emberAfRf4ceZrc20Bind with the requested
 * search device type.  This causes the GDP plugin to perform discovery for
 * matching targets in range.  Potential targets are ranked according to an
 * algorithm described in the GDP specification.  If one or more potential
 * targets are identified, the GDP plugin will attempt to pair with the
 * highest-ranked target.  Once the temporary pairing is established, both the
 * GDP and ZRC profiles will perform their respective configuration procedures.
 * Finally, the validation procedure begins.  Validation is implementation
 * specific, but may be as simple as a button press on the target or a more
 * involved challenge-response mechanism between target and originator.  Only
 * if validation is successful are the originator and recipient considered
 * bound.  If any step fails, the GDP plugin will restart the binding procedure
 * using the target that has the next-highest rank.  At the conclusion of the
 * binding procedure, the GDP plugin will call
 * ::emberAfPluginRf4ceGdpBindingCompleteCallback to indicate whether binding
 * completed successfully with a target and, if so, which pairing index has
 * been assigned to that target.
 *
 * The GDP plugin and this plugin will manage discovery, pairing, and
 * configuration for recipients on behalf of the device.  If configuration
 * completes successfully, the GDP plugin will call
 * ::emberAfPluginRf4ceGdpStartValidationCallback so that the device can begin
 * the implementation-specific validation procedure.  Once the application
 * determines the validation status of the originator, it should call
 * ::emberAfRf4ceGdpSetValidationStatus.  Alternatively, the recipient may call
 * ::emberAfRf4ceGdpPushButton to set the push-button stimulus flag.  Once set,
 * the GDP plugin will automatically validate binding attempts.  Push-button
 * mode lasts for a fixed duration as described in the GDP specification and
 * may be initiated at any time before or during binding, including during the
 * validation procedure.
 *
 * Following a successful binding, action originators may send action messages
 * to action recipients by calling ::emberAfRf4ceZrc20ActionStart.  In
 * response, the plugin will transmit a start or atomic message to the
 * indicated target with specific action parameters.  If the action is atomic,
 * the plugin performs no additional processing.  Otherwise, the plugin will
 * repeatedly transmit repeat messages at fixed intervals until
 * ::emberAfRf4ceZrc20ActionStop is called.  The interval at which repeat
 * messages are transmitted is configurable in the plugin options.
 *
 * For action recipients, the plugin will keep track of incoming action
 * messages.  Each time a start or atomic command is received, the plugin will
 * call ::emberAfPluginRf4ceZrc20ActionCallback with its information.  For non-
 * atomic actions, the plugin will then wait for a fixed duration for a
 * corresponding repeat action.  If a repeat action is received within the
 * timeout, the plugin will reset its timer and wait for the next message.  If
 * a repeat message is not received within the timeout, the plugin will call
 * ::emberAfPluginRf4ceZrc20ActionCallback with an indication that the action
 * has stopped.  The timeout for receiving repeat messages is configurable in
 * the plugin options.  Note that the plugin will not call
 * ::emberAfPluginRf4ceZrc20ActionCallback for repeat messages that follow a
 * start message.
 *
 * The plugin is capable of keeping track of a fixed number of simultaneous
 * incoming and outgoing actions.  The limits are configurable in the plugin
 * options.
 *
 * This plugin manages the state of the receiver by calling
 * ::emberAfRf4ceRxEnable using ::EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0 as
 * the profile id.  If the application also wishes to manage the receiver, it
 * should do so using ::EMBER_AF_RF4CE_PROFILE_WILDCARD as the profile id or by
 * calling ::emberAfRf4ceSetPowerSavingParameters.
 *
 * This plugin utilizes the discovery, pairing, sending and receiving, and
 * power-saving functionality provided by the @ref rf4ce-profile plugin.  It
 * also utilizes the binding functionality provided by the @ref rf4ce-gdp
 * plugin.  Support for the optional Action Mapping feature in ZRC2.0 is
 * provided by the @ref rf4ce-zrc20-action-mapping-client and @ref
 * rf4ce-zrc20-action-mapping-server plugins.  Support for the optional Home
 * Automation (HA) interoperability feature is provided by the @ref
 * rf4ce-zrc20-ha-client and @ref rf4ce-zrc20-ha-server plugins.
 * @{
 */

#ifdef EMBER_AF_RF4CE_ZRC_IS_ACTIONS_ORIGINATOR
  /** @brief Set if the device is configured as a ZRC originator. */
  #define EMBER_AF_RF4CE_ZRC_IS_ORIGINATOR
#endif

#ifdef EMBER_AF_RF4CE_ZRC_IS_ACTIONS_RECIPIENT
  /** @brief Set if the device is configured as a ZRC recipient. */
  #define EMBER_AF_RF4CE_ZRC_IS_RECIPIENT
#endif

#define ACTION_MAPPING_CLIENT 1
#define ACTION_MAPPING_SERVER 2

#if EMBER_AF_PLUGIN_RF4CE_ZRC20_ACTION_MAPPING_SUPPORT == ACTION_MAPPING_CLIENT
  /** @brief Set if the device is configured as an action mapping client. */
  #define EMBER_AF_PLUGIN_RF4CE_ZRC20_IS_ACTION_MAPPING_CLIENT

#if defined(EMBER_AF_RF4CE_ZRC_IRDB_VENDOR_IDS)
#define EMBER_AF_PLUGIN_RF4CE_ZRC20_LOCAL_IRDB_VENDOR_ATTRIBUTE_SUPPORT
#endif

#elif EMBER_AF_PLUGIN_RF4CE_ZRC20_ACTION_MAPPING_SUPPORT == ACTION_MAPPING_SERVER
  /** @brief Set if the device is configured as an action mapping server. */
  #define EMBER_AF_PLUGIN_RF4CE_ZRC20_IS_ACTION_MAPPING_SERVER
  /** @brief An action mapping server always supports remote vendor-specific
   * IRDB attributes. */
  #define EMBER_AF_PLUGIN_RF4CE_ZRC20_REMOTE_IRDB_VENDOR_ATTRIBUTES_SUPPORT
#endif

EmberStatus emberAfRf4ceZrc20Bind(EmberAfRf4ceDeviceType searchDevType);

EmberStatus emberAfRf4ceZrc20ProxyBind(EmberPanId panId,
                                       EmberEUI64 ieeeAddr);

/**
 * @brief Adds the passed action record to the internal list of outstanding
 * action records. If the passed action record is already in the internal record
 * list, the action type for the matching record is set back to 'start' and a
 * new actions command is sent out right away. This API ensures that the passed
 * record fits in the next outgoing actions command.
 *
 * @param pairingIndex      The index of the pairing the passed action record is
 *                          destined to.
 *
 * @param actionBank        The action bank field of the action record.
 *
 * @param actionCode        The action code field of the action record.
 *
 * @param actionModifier    The modifier bits of the action record.
 *
 * @param actionVendorId    The vendor ID to be included in the action record. A
 *                          value of ::EMBER_RF4CE_NULL_VENDOR_ID won't be
 *                          included in the over-the-air action record.
 *
 * @param actionData        A pointer to the actionData field of the action
 *                          record. Notice that this memory area is not copied,
 *                          therefore it should refer some global memory area.
 *                          A NULL pointer causes this field to not be included
 *                          in the over-the-air action record.
 *
 * @param actionDataLength  The length in bytes of the actionData field.
 *
 * @return   An ::EmberStatus value of ::EMBER_SUCCESS if there is a free entry
 *           in the table of outstanding action records and the passed action
 *           record fits in the next outgoing actions command for the passed
 *           pairing index. Otherwise it returns an ::EmberStatus value of
 *           ::EMBER_INVALID_CALL.
 */
EmberStatus emberAfRf4ceZrc20ActionStart(uint8_t pairingIndex,
                                         EmberAfRf4ceZrcActionBank actionBank,
                                         EmberAfRf4ceZrcActionCode actionCode,
                                         EmberAfRf4ceZrcModifierBit actionModifier,
                                         uint16_t actionVendorId,
                                         const uint8_t *actionData,
                                         uint8_t actionDataLength,
                                         bool atomic);

/**
 * @brief Removes the passed action record from the internal list of outstanding
 * action records.
 *
 * @param pairingIndex   The index of the pairing the passed action record is
 *                       destined to.
 *
 * @param actionBank     The action bank field of the action record to be
 *                       removed.
 *
 * @param actionCode     The action code field of the action record to be
 *                       removed.
 *
 * @param actionModifier The modifier bits of the action record to be removed.
 *
 * @return   An ::EmberStatus value of ::EMBER_SUCCESS if the passed action
 *           record was successfully found and removed from the table of
 *           outstanding action records.  Otherwise it returns an ::EmberStatus
 *           value of ::EMBER_INVALID_CALL.
 */
EmberStatus emberAfRf4ceZrc20ActionStop(uint8_t pairingIndex,
                                        EmberAfRf4ceZrcActionBank actionBank,
                                        EmberAfRf4ceZrcActionCode actionCode,
                                        EmberAfRf4ceZrcModifierBit actionModifier,
                                        uint16_t actionVendorId);
/**
 * @brief Initiates the legacy ZRC 1.1 command discovery process. If this API
 * returns a successful status, the corresponding
 * emberAfRf4ceZrc20LegacyCommandDiscoveryComplete() callback will be called
 * upon receiving the discovery response command from the peer node or upon
 * timeout.
 *
 * @param pairingIndex    The index of the destination pairing.
 *
 * @return   An ::EmberStatus value of ::EMBER_SUCCESS if a ZRC 1.1 Discovery
 *           Request command was sent to the pairing corresponding to the passed
 *           pairing index. An ::EmberStatus value of ::EMBER_INVALID_CALL if
 *           the destination pairing supports ZRC 2.0 or does not support ZRC
 *           1.1.  Otherwise it returns an ::EmberStatus value indicating the
 *           TX failure reason.
 */
EmberStatus emberAfRf4ceZrc20LegacyCommandDiscovery(uint8_t pairingIndex);

/**
 * @brief Initiates the action mapping negotiation procedureat the action
 * mapping client with the action mapping server.
 *
 * @param pairingIndex    The index of the action mapping server pairing.
 *
 * @return   An ::EmberStatus value of ::EMBER_SUCCESS if the action mapping
 * negotiation procedure is successfully initiated. Otherwise it returns an
 * ::EmberStatus value of ::EMBER_INVALID_CALL.
 */
EmberStatus emberAfRf4ceZrc20StartActionMappingsNegotiation(uint8_t pairingIndex);

/**
 * @brief Initiates the Home Automation supported announcement procedure at the
 * Home Automation originator with the Home Automation recipient.
 *
 * @param pairingIndex    The index of the Home Automation recipient pairing.
 *
 * @return   An ::EmberStatus value of ::EMBER_SUCCESS if the Home Automation
 * supported announcement procedure is successfully initiated. Otherwise it
 * returns an ::EmberStatus value of ::EMBER_INVALID_CALL.
 */
EmberStatus emberAfRf4ceZrc20StartHomeAutomationSupportedAnnouncement(uint8_t pairingIndex);

/**
 * @brief Pulls a Home Automation attribute from the Home Automation recipient.
 *
 * @param pairingIndex    The index of the Home Automation recipient pairing.
 *
 * @param vendorId        The vendor ID to be included in the Pull command. If
 *                        a value of ::EMBER_RF4CE_NULL_VENDOR_ID is passed, no
 *                        vendor ID will be included in the Pull command.
 *
 * @param haInstanceId    The Home Automation instance ID.
 *
 * @param haAttribureId   The Home Automation attribute ID.
 *
 * @return   An ::EmberStatus value of ::EMBER_SUCCESS if the Home Automation
 * Pull command was successfully sent. If this is the case, the corresponding
 * callback
 * ::emberAfPluginRf4ceZrc20PulllHomeAutomationAttributeCompleteCallback()
 * will fire. Otherwise it returns an ::EmberStatus indicating the reason of
 * failure.
 */
EmberStatus emberAfRf4ceZrc20PullHomeAutomationAttribute(uint8_t pairingIndex,
                                                         uint16_t vendorId,
                                                         uint8_t haInstanceId,
                                                         uint8_t haAttributeId);

/** @brief Accesses the actual command discovery data of the
 * ::EmberAfRf4ceZrcCommandsSupported structure.
 *
 * @param tag A pointer to an ::EmberAfRf4ceZrcCommandsSupported structure.
 *
 * @return uint8_t* Returns a pointer to the first byte of the command discovery
 * value.
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
  uint8_t *emberAfRf4ceZrcCommandsSupportedContents(EmberAfRf4ceZrcCommandsSupported *commandsSupported);
#else
  #define emberAfRf4ceZrcCommandsSupportedContents(commandsSupported) \
    ((commandsSupported)->contents)
#endif

#ifdef DOXYGEN_SHOULD_SKIP_THIS
  /** @brief Determine if the action mapping entry has an RF descriptor.
   *
   * Implemented as a macro for efficiency.
   *
   * @return true if the action mapping table entry has an RF descriptor or
   * false otherwise.
   */
  bool emberAfRf4ceZrc20ActionMappingEntryHasRfDescriptor(const EmberAfRf4ceZrcActionMapping *entry);
#else
  #define emberAfRf4ceZrc20ActionMappingEntryHasRfDescriptor(entry)                     \
    (READBITS((entry)->mappingFlags,                                                    \
              (EMBER_AF_RF4CE_ZRC_ACTION_MAPPING_MAPPING_FLAGS_RF_SPECIFIED_BIT         \
               | EMBER_AF_RF4CE_ZRC_ACTION_MAPPING_MAPPING_FLAGS_USE_DEFAULT_BIT))      \
     == EMBER_AF_RF4CE_ZRC_ACTION_MAPPING_MAPPING_FLAGS_RF_SPECIFIED_BIT)
#endif

#ifdef DOXYGEN_SHOULD_SKIP_THIS
  /** @brief Determine if the action mapping entry has an IR descriptor.
   *
   * Implemented as a macro for efficiency.
   *
   * @return true if the action mapping table entry has an IR descriptor or
   * false otherwise.
   */
  bool emberAfRf4ceZrc20ActionMappingEntryHasIrDescriptor(const EmberAfRf4ceZrcActionMapping *entry);
#else
  #define emberAfRf4ceZrc20ActionMappingEntryHasIrDescriptor(entry)                     \
    (READBITS((entry)->mappingFlags,                                                    \
              (EMBER_AF_RF4CE_ZRC_ACTION_MAPPING_MAPPING_FLAGS_IR_SPECIFIED_BIT         \
               | EMBER_AF_RF4CE_ZRC_ACTION_MAPPING_MAPPING_FLAGS_USE_DEFAULT_BIT))      \
     == EMBER_AF_RF4CE_ZRC_ACTION_MAPPING_MAPPING_FLAGS_IR_SPECIFIED_BIT)
#endif

#ifdef DOXYGEN_SHOULD_SKIP_THIS
  /** @brief Determine if the action mapping entry has an IR vendor ID.
   *
   * Implemented as a macro for efficiency.
   *
   * @return true if the action mapping table entry has an IR vendor ID or false
   * otherwise.
   */
  bool emberAfRf4ceZrc20ActionMappingEntryHasIrVendorId(const EmberAfRf4ceZrcActionMapping *entry);
#else
  #define emberAfRf4ceZrc20ActionMappingEntryHasIrVendorId(entry)       \
    (READBITS((entry)->irConfig,                                        \
              RF4CE_ZRC_ACTION_MAPPING_IR_CONFIG_VENDOR_SPECIFIC)       \
     == RF4CE_ZRC_ACTION_MAPPING_IR_CONFIG_VENDOR_SPECIFIC)
#endif

// If the Use Default bit is set, the RF Specified and IR Specified bits and
// their corresponding descriptors are ignored.
#define SET_DEFAULT(entry)                                                                      \
  ((entry)->mappingFlags = EMBER_AF_RF4CE_ZRC_ACTION_MAPPING_MAPPING_FLAGS_USE_DEFAULT_BIT)

#endif // __RF4CE_ZRC20_H__

// END addtogroup
