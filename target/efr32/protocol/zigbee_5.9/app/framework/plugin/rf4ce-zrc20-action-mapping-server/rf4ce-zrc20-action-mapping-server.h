// Copyright 2014 Silicon Laboratories, Inc.

#ifndef __RF4CE_ZRC20_ACTION_MAPPING_SERVER_H__
#define __RF4CE_ZRC20_ACTION_MAPPING_SERVER_H__

/**
 * @addtogroup rf4ce-zrc20-action-mapping-server
 *
 * The ZigBee Remote Control 2.0 (ZRC2.0) Action Mapping Server plugin
 * implements the optional action mapping feature of the ZRC2.0 profile for
 * servers.  Action mapping provides a standard mechanism for remapping actions
 * and is generally used to allow a remote to control legacy IR devices or to
 * perform simulataneous IR and RF functions.  This plugin manages the storage
 * and retrieval of these mappings for action recipients.
 *
 * This plugin provides information to the @ref rf4ce-zrc20 plugin about action
 * mappings for clients.  An action mapping remaps an action on the client to
 * an IR code, some other RF action, or to some IR-RF combination.  During
 * binding, the mappable actions supported by the client are exchanged with
 * this device.  The @ref rf4ce-zrc20 plugin receives the actions and passes
 * them to this plugin via
 * ::emberAfPluginRf4ceZrc20IncomingMappableActionCallback.  This plugin, in
 * turn, will store the data in RAM.  The amount of storage space dedicated to
 * storing mappable actions and action mappings is configuratable via the
 * plugin options.
 *
 * To create a mapping, the application should call
 * ::emberAfRf4ceZrc20ActionMappingServerRemapAction with the action to remap
 * and the new mapping.  Once created, the mapping will apply to all clients
 * for that particular action.  Clients will be notified of action mappings
 * during the binding procedure or if they manually query the server for them
 * after binding.  How the remappings are determined is application specific,
 * but may involve an interactive menu that a user must navigate to identify
 * the appropriate mappings.  If a mapping no longer applies, perhaps because a
 * connected peripheral such as a television has changed, the mapping may be
 * removed and the action restored to its default by calling
 * ::emberAfRf4ceZrc20ActionMappingServerRestoreDefaultAction.  To reset all
 * actions to the default, for example due to a reset to factory new,
 * ::emberAfRf4ceZrc20ActionMappingServerRestoreDefaultAllActions may be
 * called.
 *
 * Support for the optional action mapping feature for clients is provided by
 * the @ref rf4ce-zrc20-action-mapping-client plugin.
 * @{
 */

/** @brief  Remap mappable action on the server.
 *
 * @param  mappableAction  The mappable action structure describing the mappable
 * action to remap.
 *
 * @param  actionMapping  The action mapping structure describing the action
 * mapping to be written into the action mapping table.
 *
 * @return  An ::EmberStatus value indicating whether the remap action mapping
 * command was successfully sent out or the reason of failure.
 */
EmberStatus emberAfRf4ceZrc20ActionMappingServerRemapAction(EmberAfRf4ceZrcMappableAction* mappableAction,
                                                            EmberAfRf4ceZrcActionMapping* actionMapping);

/** @brief  Restore default action of the mappable action on the server.
 *
 * @param  mappableAction  The mappable action structure describing the mappable
 * action which action is restored to default.
 *
 * @return  An ::EmberStatus value indicating whether the restore default action
 * command was successfully sent out or the reason of failure.
 */
EmberStatus emberAfRf4ceZrc20ActionMappingServerRestoreDefaultAction(EmberAfRf4ceZrcMappableAction* mappableAction);

/** @brief  Restore all actions to default on the server.
 *
 */
void emberAfRf4ceZrc20ActionMappingServerRestoreDefaultAllActions(void);

/** @brief  Get the number of mappable actions on the server.
 *
 * @return  Mappable Action count
 */
uint16_t emberAfRf4ceZrc20ActionMappingServerGetMappableActionCount(void);

#ifndef DOXYGEN_SHOULD_SKIP_THIS

/* @brief  Clear entries in mapping table that point at serverEntryIndex.
 *
 * @param  serverIndex  The index of the mappable action on the server.
 *
 * @return  An ::EmberStatus value indicating whether the clear mapping command
 * was successfully sent out or the reason of failure.
 */
void emAfRf4ceZrc20ActionMappingServerClearMapping(uint16_t serverIndex);

/* @brief  Clear all entries in mapping table.
 *
 */
void emAfRf4ceZrc20ActionMappingServerClearAllMappings(void);

/* @brief  Add entry in mapping table.
 *
 * @param  pairingIndex  The pairing index the add mapping command should be
 * sent to.
 *
 * @param  entryIndex  The entry index of the mappable action on the client.
 *
 * @param  serverIndex  The index of the mappable action on the server.
 *
 * @return  An ::EmberStatus value indicating whether the add mapping command
 * was successfully sent out or the reason of failure.
 */
EmberStatus emAfRf4ceZrc20ActionMappingServerUpdateOrAddMapping(uint8_t pairingIndex,
                                                                uint16_t entryIndex,
                                                                uint16_t serverIndex);

/* @brief  Remove entry from mapping table.
 *
 * @param  pairingIndex  The pairing index the remove mapping command should be
 * sent to.
 *
 * @param  entryIndex  The entry index of the mappable action on the client.
 *
 * @return  An ::EmberStatus value indicating whether the remove mapping command
 * was successfully sent out or the reason of failure.
 */
EmberStatus emAfRf4ceZrc20ActionMappingServerRemoveMapping(uint8_t pairingIndex,
                                                           uint16_t entryIndex);

/* @brief  Get entry from mapping table.
 *
 * @param  pairingIndex  The pairing index the get mapping command should be
 * sent to.
 *
 * @param  entryIndex  The entry index of the mappable action on the client.
 *
 * @param  serverIndex  Pointer to the index of the mappable action on the
 * server.
 *
 * @return  An ::EmberStatus value indicating whether the get mapping command
 * was successfully sent out or the reason of failure.
 */
EmberStatus emAfRf4ceZrc20ActionMappingServerGetMapping(uint8_t pairingIndex,
                                                        uint16_t entryIndex,
                                                        uint16_t *serverIndex);

/* @brief  Clear mappable action at index of the table on the server.
 *
 * @param  index  The index the clear mappable action command should be sent to.
 *
 * @return  An ::EmberStatus value indicating whether the clear mappable action
 * command was successfully sent out or the reason of failure.
 */
EmberStatus emAfRf4ceZrc20ActionMappingServerClearMappableAction(uint16_t index);

/* @brief  Clear all mappable actions on the server.
 *
 */
void emAfRf4ceZrc20ActionMappingServerClearAllMappableActions(void);

/* @brief  Set mappable action on the server.
 *
 * @param  index  The index of the mappable action to set.
 *
 * @param  mappableAction  The mappable action structure describing the mappable
 * action to write to index location in the table.
 *
 * @return  An ::EmberStatus value indicating whether the set mappable action
 * command was successfully sent out or the reason of failure.
 */
EmberStatus emAfRf4ceZrc20ActionMappingServerSetMappableAction(uint16_t index,
                                                               EmberAfRf4ceZrcMappableAction *mappableAction);

/* @brief  Get mappable action on the server.
 *
 * @param  index  The index of the mappable action to get.
 *
 * @param  mappableAction  The mappable action structure describing the mappable
 * action to read from index location in the table.
 *
 * @return  An ::EmberStatus value indicating whether the get mappable action
 * command was successfully sent out or the reason of failure.
 */
EmberStatus emAfRf4ceZrc20ActionMappingServerGetMappableAction(uint16_t index,
                                                               EmberAfRf4ceZrcMappableAction *mappableAction);

/* @brief  Look up mappable action on the server.
 *
 * @param  mappableAction  Pointer to the mappable action structure describing
 * the mappable action to look up.
 *
 * @param  index  The index of the mappable action found.
 *
 * @return  An ::EmberStatus value indicating whether the look up mappable
 * action command was successfully sent out or the reason of failure.
 */
EmberStatus emAfRf4ceZrc20ActionMappingServerLookUpMappableAction(EmberAfRf4ceZrcMappableAction *mappableAction,
                                                                  uint16_t* index);

/* @brief  Clear action mapping on the server.
 *
 * @param  index  The index of the action mapping to clear.
 *
 * @return  An ::EmberStatus value indicating whether the clear action mapping
 * command was successfully sent out or the reason of failure.
 */
EmberStatus emAfRf4ceZrc20ActionMappingServerClearActionMapping(uint16_t index);

/* @brief  Clear all action mappings on the server.
 *
 */
void emAfRf4ceZrc20ActionMappingServerClearAllActionMappings(void);

/* @brief  Set action mapping on the server.
 *
 * @param  index  The index of the action mapping to set.
 *
 * @param  actionMapping  The action mapping structure describing the action
 * mapping to write to index location in the table.
 *
 * @return  An ::EmberStatus value indicating whether the set action mapping
 * command was successfully sent out or the reason of failure.
 */
EmberStatus emAfRf4ceZrc20ActionMappingServerSetActionMapping(uint16_t index,
                                                              EmberAfRf4ceZrcActionMapping *actionMapping);

/* @brief  Get action mapping on the server.
 *
 * @param  index  The index of the action mapping to get.
 *
 * @param  actionMapping  The action mapping structure describing the action
 * mapping to read from index location in the table.
 *
 * @return  An ::EmberStatus value indicating whether the get action mapping
 * command was successfully sent out or the reason of failure.
 */
EmberStatus emAfRf4ceZrc20ActionMappingServerGetActionMapping(uint16_t index,
                                                              EmberAfRf4ceZrcActionMapping *actionMapping);

#endif // DOXYGEN_SHOULD_SKIP_THIS

#endif // __RF4CE_ZRC20_ACTION_MAPPING_SERVER_H__

// END addtogroup
