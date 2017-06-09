// Copyright 2014 Silicon Laboratories, Inc.

#ifndef __RF4CE_ZRC20_ACTION_MAPPING_CLIENT_H__
#define __RF4CE_ZRC20_ACTION_MAPPING_CLIENT_H__

/**
 * @addtogroup rf4ce-zrc20-action-mapping-client
 *
 * The ZigBee Remote Control 2.0 (ZRC2.0) Action Mapping Client plugin
 * implements the optional action mapping feature of the ZRC2.0 profile for
 * clients.  Action mapping provides a standard mechanism for remapping actions
 * and is generally used to allow a remote to control legacy IR devices or to
 * perform simulataneous IR and RF functions.  This plugin manages the storage
 * and retrieval of these mappings for action originators.
 *
 * This plugin provides information to the @ref rf4ce-zrc20 plugin about which
 * mappable actions this device supports.  A mappable action is simply an
 * action on the local device that may be remapped by the server to an IR code,
 * some other RF action, or to some IR-RF combination.  During binding, the
 * mappable actions supported by this device are exchanged with the server.
 * The server uses this information to set up the appropriate remappings.  The
 * mappable actions are configurable in AppBuilder.
 *
 * When the server provides action mappings, either during the binding
 * procedure or when the application manually queries the server for them, the
 * @ref rf4ce-zrc20 plugin receives the mappings and passes them to this plugin
 * via ::emberAfPluginRf4ceZrc20IncomingActionMappingCallback.  This plugin, in
 * turn, will store the data in RAM.  The amount of storage space dedicated to
 * storing the mappings is configuratable via the plugin options.
 *
 * Prior to starting an action, the application should call
 * ::emberAfRf4ceZrc20ActionMappingClientGetActionMapping to determine if the
 * action has been remapped.  If the action has been remapped and an IR code
 * has been specified, the application should transmit it using application-
 * specific means.  If an RF mapping has been specified, the application should
 * pass it to ::emberAfRf4ceZrc20ActionStart instead of the original action.
 *
 * Action mappings are generally managed by the server, but the application is
 * provided some control as well.  Specific mappings can be manipulated by
 * calling ::emberAfRf4ceZrc20ActionMappingClientSetActionMapping.  An action
 * may be set to the default behavior by calling
 * ::emberAfRf4ceZrc20ActionMappingClientClearActionMapping.  Resetting all
 * actions to the defaults can be accomplished on a per-pairing basis by
 * calling ::emberAfRf4ceZrc20ActionMappingClientClearActionMappingsPerPairing
 * or for all pairings by calling
 * ::emberAfRf4ceZrc20ActionMappingClientClearAllActionMappings.  It may be
 * desirable to reset action mappings when the device unpairs from a specific
 * server or is reset to factory new.
 *
 * Support for the optional action mapping feature for servers is provided by
 * the @ref rf4ce-zrc20-action-mapping-server plugin.
 * @{
 */

/** @brief  Clear all action mappings on the client.
 *
 */
void emberAfRf4ceZrc20ActionMappingClientClearAllActionMappings(void);

/** @brief  Clear action mappings per pairing on the client.
 *
 * @param  pairingIndex  The pairing index the clear action mapping command
 * should be sent to.
 *
 * @return  An ::EmberStatus value indicating whether the clear action mappings
 * per pairing command was successfully sent out or the reason of failure.
 */
EmberStatus emberAfRf4ceZrc20ActionMappingClientClearActionMappingsPerPairing(uint8_t pairingIndex);

/** @brief  Clear action mapping that belongs to a mappable action.
 *
 * @param  pairingIndex  The pairing index the clear action mapping command
 * should be sent to.
 *
 * @param  actionDeviceType  The action device type of the mappable action.
 *
 * @param  actionBank  The action bank of the mappable action.
 *
 * @param  actionCode  The action code of the mappable action.
 *
 * @return  An ::EmberStatus value indicating whether the clear action mapping
 * command was successfully sent out or the reason of failure.
 */
EmberStatus emberAfRf4ceZrc20ActionMappingClientClearActionMapping(uint8_t pairingIndex,
                                                                   EmberAfRf4ceDeviceType actionDeviceType,
                                                                   EmberAfRf4ceZrcActionBank actionBank,
                                                                   EmberAfRf4ceZrcActionCode actionCode);

/** @brief  Get action mapping corresponding to a mappable action.
 *
 * @param  pairingIndex  The pairing index the get action mapping command
 * should be sent to.
 *
 * @param  actionDeviceType  The action device type of the mappable action.
 *
 * @param  actionBank  The action bank of the mappable action.
 *
 * @param  actionCode  The action code of the mappable action.
 *
 * @param  actionMapping  The action mapping structure describing the action
 * mapping.
 *
 * @return  An ::EmberStatus value indicating whether the get action mapping
 * command was successfully sent out or the reason of failure.
 */
EmberStatus emberAfRf4ceZrc20ActionMappingClientGetActionMapping(uint8_t pairingIndex,
                                                                 EmberAfRf4ceDeviceType actionDeviceType,
                                                                 EmberAfRf4ceZrcActionBank actionBank,
                                                                 EmberAfRf4ceZrcActionCode actionCode,
                                                                 EmberAfRf4ceZrcActionMapping* actionMapping);

#ifdef DOXYGEN_SHOULD_SKIP_THIS
  /** @brief  Look up action mapping that belongs to a mappable action.
   *
   * @param  pairingIndex  The pairing index the look up action mapping command
   * should be sent to.
   *
   * @param  actionDeviceType  The action device type of the mappable action.
   *
   * @param  actionBank  The action bank of the mappable action.
   *
   * @param  actionCode  The action code of the mappable action.
   *
   * @param  actionMapping  The action mapping structure describing the action
   * mapping.
   *
   * @return  An ::EmberStatus value indicating whether the look up action
   * mapping command was successfully sent out or the reason of failure.
   *
   * @deprecated This function is deprecated and will be removed in a future
   * release.  Customers should use
   * ::emberAfRf4ceZrc20ActionMappingClientGetActionMapping instead.
   */
  EmberStatus emberAfRf4ceZrc20ActionMappingClientLookUpActionMapping(uint8_t pairingIndex,
                                                                      EmberAfRf4ceDeviceType actionDeviceType,
                                                                      EmberAfRf4ceZrcActionBank actionBank,
                                                                      EmberAfRf4ceZrcActionCode actionCode,
                                                                      EmberAfRf4ceZrcActionMapping* actionMapping);
#else
  #define emberAfRf4ceZrc20ActionMappingClientLookUpActionMapping \
    emberAfRf4ceZrc20ActionMappingClientGetActionMapping
#endif

/** @brief  Set action mapping corresponding to a mappable action.
 *
 * @param  pairingIndex  The pairing index the set action mapping command
 * should be sent to.
 *
 * @param  actionDeviceType  The action device type of the mappable action.
 *
 * @param  actionBank  The action bank of the mappable action.
 *
 * @param  actionCode  The action code of the mappable action.
 *
 * @param  actionMapping  The action mapping structure describing the action
 * mapping.
 *
 * @return  An ::EmberStatus value indicating whether the set action mapping
 * command was successfully sent out or the reason of failure.
 */
EmberStatus emberAfRf4ceZrc20ActionMappingClientSetActionMapping(uint8_t pairingIndex,
                                                                 EmberAfRf4ceDeviceType actionDeviceType,
                                                                 EmberAfRf4ceZrcActionBank actionBank,
                                                                 EmberAfRf4ceZrcActionCode actionCode,
                                                                 EmberAfRf4ceZrcActionMapping* actionMapping);

#ifndef DOXYGEN_SHOULD_SKIP_THIS

/* @brief  Set selected action mapping.
 *
 * @param  pairingIndex  The pairing index the set action mapping command
 * should be sent to.
 *
 * @param  entryIndex  The entry index of the action mapping to be set.
 *
 * @param  actionMapping  The action mapping structure describing the action
 * mapping.
 *
 * @return  An ::EmberStatus value indicating whether the set attributes command
 * was successfully sent out or the reason of failure.
 */
EmberStatus emAfRf4ceZrc20ActionMappingClientSetActionMapping(uint8_t pairingIndex,
                                                              uint16_t entryIndex,
                                                              EmberAfRf4ceZrcActionMapping* actionMapping);

/* @brief  Clear selected action mapping on the client.
 *
 * @param  pairingIndex  The pairing index the clear action mapping command
 * should be sent to.
 *
 * @param  entryIndex  The entry index of the action mapping to be cleared.
 *
 * @return  An ::EmberStatus value indicating whether the clear action mapping
 * command was successfully sent out or the reason of failure.
 */
EmberStatus emAfRf4ceZrc20ActionMappingClientClearActionMapping(uint8_t pairingIndex,
                                                                uint16_t entryIndex);

/* @brief  Get the number of mappable actions on the client.
 *
 * @return  Mappable Action count
 */
uint16_t emAfPluginRf4ceZrc20ActionMappingClientGetMappableActionCount(void);

/* @brief  Get mappable action at entryIndex.
 *
 * @param  entryIndex  The entry index of the mappable action to get.
 *
 * @param  mappableAction  The mappable action structure describing the mappable
 * action.
 *
 * @return  An ::EmberStatus value indicating whether the get mappable action
 * command was successfully sent out or the reason of failure.
 */
EmberStatus emAfPluginRf4ceZrc20ActionMappingClientGetMappableAction(uint16_t entryIndex,
                                                                     EmberAfRf4ceZrcMappableAction *mappableAction);

#endif // DOXYGEN_SHOULD_SKIP_THIS

#endif // __RF4CE_ZRC20_ACTION_MAPPING_CLIENT_H__

// END addtogroup
