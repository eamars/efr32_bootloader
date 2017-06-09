// Copyright 2014 Silicon Laboratories, Inc.

#ifndef __RF4CE_ZRC20_HA_CLIENT_H__
#define __RF4CE_ZRC20_HA_CLIENT_H__

/**
 * @addtogroup rf4ce-zrc20-ha-client
 *
 * The ZigBee Remote Control 2.0 (ZRC2.0) Home Automation (HA) Client plugin
 * implements the optional HA interoperability feature of the ZRC2.0 profile
 * for HA action originators.  HA interoperability allows RF4CE devices to
 * control HA devices operating on a ZigBee PRO network within the same
 * premises.
 *
 * HA action originators are most typically controllers but may also be
 * targets.  HA action recipients must be targets.  HA originators transmit HA
 * commands to an HA recipient over the RF4CE network as ZRC2.0 actions.  The
 * recipient then translates these actions to ZigBee Cluster Library (ZCL)
 * messages and retransmits them to the appropriate HA device on the PRO
 * network.  The originator directs all actions to abstract HA instances.  Each
 * HA action recipient is responsible for mapping HA instances to actual
 * physical devices.
 *
 * After binding with an HA action recipient, the originator can construct an
 * HA command by calling one of the \em emberAfZrcHaFill APIs.  The message can
 * then be sent to the recipient by calling ::emberAfRf4ceZrcHaSend.  For
 * example, the application can toggle the light bulb identified as HA instance
 * 0 on its first pairing by calling
 * ::emberAfRf4ceZrcHaFillCommandOnOffClusterToggle followed by
 * ::emberAfRf4ceZrcHaSend(0, EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_INSTANCE_0).
 *
 * This plugin also provides information to the @ref rf4ce-zrc20 plugin about
 * which HA attributes this device is interested in.  During binding, this
 * information is exchanged with the HA action recipient.  The recipient will
 * use this information to send notifications when those attributes change on
 * the HA network.  The supported attributes are configurable in AppBuilder.
 *
 * This plugin utilizes the action functionality provided by the @ref
 * rf4ce-zrc20 plugin.  Support for the optional HA action recipient feature is
 * provided by the @ref rf4ce-zrc20-ha-server plugin.
 * @{
 */

typedef enum {
  EMBER_ZCL_THERMOSTAT_SETPOINT_RAISE_LOWER_MODE_HEAT = 0x00,
  EMBER_ZCL_THERMOSTAT_SETPOINT_RAISE_LOWER_MODE_COOL = 0x01,
  EMBER_ZCL_THERMOSTAT_SETPOINT_RAISE_LOWER_MODE_BOTH = 0x02
} EmberAfThermostatSetpointRaiseLowerMode;


uint16_t emAfPluginRf4ceZrc20HaFillExternalBuffer(PGM_P format, ...);

#define emberAfRf4ceZrcHaFillCommandScenesClusterStoreLocalScene(scene)        \
            emAfPluginRf4ceZrc20HaFillExternalBuffer("u",                   \
                                                     EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_ACTION_CODE_SCENES_STORE_LOCAL_SCENE_ ## scene)

#define emberAfRf4ceZrcHaFillCommandScenesClusterRecallLocalScene(scene)       \
            emAfPluginRf4ceZrc20HaFillExternalBuffer("u",                   \
                                                     EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_ACTION_CODE_SCENES_RECALL_LOCAL_SCENE_ ## scene)


/* Public API. */


/** @brief  Send HA command to the server.
 *
 * @param  pairingIndex  The pairing index the send HA command should be sent
 * to.
 *
 * @param haInstanceId  The HA instance ID the send HA command should be sent
 * to.
 *
 * @return  An ::EmberStatus value indicating whether the send HA command was
 * successfully sent out or the reason of failure.
 */
EmberStatus emberAfRf4ceZrcHaSend(uint8_t pairingIndex,
                                  uint8_t haInstanceId);


/** @name Scenes Commands */
// @{
/** @brief Store local scene 0
 *
 * Cluster: Scenes, Provides an interface for storing local scene 0.
 * Command: StoreLocalScene0
 */
#define emberAfRf4ceZrcHaFillCommandScenesClusterStoreLocalScene0()            \
            emberAfRf4ceZrcHaFillCommandScenesClusterStoreLocalScene(0)


/** @brief Store local scene 1
 *
 * Cluster: Scenes, Provides an interface for storing local scene 1.
 * Command: StoreLocalScene1
 */
#define emberAfRf4ceZrcHaFillCommandScenesClusterStoreLocalScene1()            \
            emberAfRf4ceZrcHaFillCommandScenesClusterStoreLocalScene(1)


/** @brief Store local scene 2
 *
 * Cluster: Scenes, Provides an interface for storing local scene 2.
 * Command: StoreLocalScene2
 */
#define emberAfRf4ceZrcHaFillCommandScenesClusterStoreLocalScene2()            \
            emberAfRf4ceZrcHaFillCommandScenesClusterStoreLocalScene(2)


/** @brief Store local scene 3
 *
 * Cluster: Scenes, Provides an interface for storing local scene 3.
 * Command: StoreLocalScene3
 */
#define emberAfRf4ceZrcHaFillCommandScenesClusterStoreLocalScene3()            \
            emberAfRf4ceZrcHaFillCommandScenesClusterStoreLocalScene(3)


/** @brief Store local scene 4
 *
 * Cluster: Scenes, Provides an interface for storing local scene 4.
 * Command: StoreLocalScene4
 */
#define emberAfRf4ceZrcHaFillCommandScenesClusterStoreLocalScene4()            \
            emberAfRf4ceZrcHaFillCommandScenesClusterStoreLocalScene(4)


/** @brief Store local scene 5
 *
 * Cluster: Scenes, Provides an interface for storing local scene 5.
 * Command: StoreLocalScene5
 */
#define emberAfRf4ceZrcHaFillCommandScenesClusterStoreLocalScene5()            \
            emberAfRf4ceZrcHaFillCommandScenesClusterStoreLocalScene(5)


/** @brief Store local scene 6
 *
 * Cluster: Scenes, Provides an interface for storing local scene 6.
 * Command: StoreLocalScene6
 */
#define emberAfRf4ceZrcHaFillCommandScenesClusterStoreLocalScene6()            \
            emberAfRf4ceZrcHaFillCommandScenesClusterStoreLocalScene(6)


/** @brief Store local scene 7
 *
 * Cluster: Scenes, Provides an interface for storing local scene 7.
 * Command: StoreLocalScene7
 */
#define emberAfRf4ceZrcHaFillCommandScenesClusterStoreLocalScene7()            \
            emberAfRf4ceZrcHaFillCommandScenesClusterStoreLocalScene(7)


/** @brief Store local scene 8
 *
 * Cluster: Scenes, Provides an interface for storing local scene 8.
 * Command: StoreLocalScene8
 */
#define emberAfRf4ceZrcHaFillCommandScenesClusterStoreLocalScene8()            \
            emberAfRf4ceZrcHaFillCommandScenesClusterStoreLocalScene(8)


/** @brief Store local scene 9
 *
 * Cluster: Scenes, Provides an interface for storing local scene 9.
 * Command: StoreLocalScene9
 */
#define emberAfRf4ceZrcHaFillCommandScenesClusterStoreLocalScene9()            \
            emberAfRf4ceZrcHaFillCommandScenesClusterStoreLocalScene(9)


/** @brief Store local scene 10
 *
 * Cluster: Scenes, Provides an interface for storing local scene 10.
 * Command: StoreLocalScene10
 */
#define emberAfRf4ceZrcHaFillCommandScenesClusterStoreLocalScene10()           \
            emberAfRf4ceZrcHaFillCommandScenesClusterStoreLocalScene(10)


/** @brief Store local scene 11
 *
 * Cluster: Scenes, Provides an interface for storing local scene 11.
 * Command: StoreLocalScene11
 */
#define emberAfRf4ceZrcHaFillCommandScenesClusterStoreLocalScene11()           \
            emberAfRf4ceZrcHaFillCommandScenesClusterStoreLocalScene(11)


/** @brief Store local scene 12
 *
 * Cluster: Scenes, Provides an interface for storing local scene 12.
 * Command: StoreLocalScene12
 */
#define emberAfRf4ceZrcHaFillCommandScenesClusterStoreLocalScene12()           \
            emberAfRf4ceZrcHaFillCommandScenesClusterStoreLocalScene(12)


/** @brief Store local scene 13
 *
 * Cluster: Scenes, Provides an interface for storing local scene 13.
 * Command: StoreLocalScene13
 */
#define emberAfRf4ceZrcHaFillCommandScenesClusterStoreLocalScene13()           \
            emberAfRf4ceZrcHaFillCommandScenesClusterStoreLocalScene(13)


/** @brief Store local scene 14
 *
 * Cluster: Scenes, Provides an interface for storing local scene 14.
 * Command: StoreLocalScene14
 */
#define emberAfRf4ceZrcHaFillCommandScenesClusterStoreLocalScene14()           \
            emberAfRf4ceZrcHaFillCommandScenesClusterStoreLocalScene(14)


/** @brief Store local scene 15
 *
 * Cluster: Scenes, Provides an interface for storing local scene 15.
 * Command: StoreLocalScene15
 */
#define emberAfRf4ceZrcHaFillCommandScenesClusterStoreLocalScene15()           \
            emberAfRf4ceZrcHaFillCommandScenesClusterStoreLocalScene(15)


/** @brief Recall local scene 0
 *
 * Cluster: Scenes, Provides an interface for recalling local scene 0.
 * Command: RecallLocalScene0
 */
#define emberAfRf4ceZrcHaFillCommandScenesClusterRecallLocalScene0()           \
            emberAfRf4ceZrcHaFillCommandScenesClusterRecallLocalScene(0)


/** @brief Recall local scene 1
 *
 * Cluster: Scenes, Provides an interface for recalling local scene 1.
 * Command: RecallLocalScene1
 */
#define emberAfRf4ceZrcHaFillCommandScenesClusterRecallLocalScene1()           \
            emberAfRf4ceZrcHaFillCommandScenesClusterRecallLocalScene(1)


/** @brief Recall local scene 2
 *
 * Cluster: Scenes, Provides an interface for recalling local scene 2.
 * Command: RecallLocalScene2
 */
#define emberAfRf4ceZrcHaFillCommandScenesClusterRecallLocalScene2()           \
            emberAfRf4ceZrcHaFillCommandScenesClusterRecallLocalScene(2)


/** @brief Recall local scene 3
 *
 * Cluster: Scenes, Provides an interface for recalling local scene 3.
 * Command: RecallLocalScene3
 */
#define emberAfRf4ceZrcHaFillCommandScenesClusterRecallLocalScene3()           \
            emberAfRf4ceZrcHaFillCommandScenesClusterRecallLocalScene(3)


/** @brief Recall local scene 4
 *
 * Cluster: Scenes, Provides an interface for recalling local scene 4.
 * Command: RecallLocalScene4
 */
#define emberAfRf4ceZrcHaFillCommandScenesClusterRecallLocalScene4()           \
            emberAfRf4ceZrcHaFillCommandScenesClusterRecallLocalScene(4)


/** @brief Recall local scene 5
 *
 * Cluster: Scenes, Provides an interface for recalling local scene 5.
 * Command: RecallLocalScene5
 */
#define emberAfRf4ceZrcHaFillCommandScenesClusterRecallLocalScene5()           \
            emberAfRf4ceZrcHaFillCommandScenesClusterRecallLocalScene(5)


/** @brief Recall local scene 6
 *
 * Cluster: Scenes, Provides an interface for recalling local scene 6.
 * Command: RecallLocalScene6
 */
#define emberAfRf4ceZrcHaFillCommandScenesClusterRecallLocalScene6()           \
            emberAfRf4ceZrcHaFillCommandScenesClusterRecallLocalScene(6)


/** @brief Recall local scene 7
 *
 * Cluster: Scenes, Provides an interface for recalling local scene 7.
 * Command: RecallLocalScene7
 */
#define emberAfRf4ceZrcHaFillCommandScenesClusterRecallLocalScene7()           \
            emberAfRf4ceZrcHaFillCommandScenesClusterRecallLocalScene(7)


/** @brief Recall local scene 8
 *
 * Cluster: Scenes, Provides an interface for recalling local scene 8.
 * Command: RecallLocalScene8
 */
#define emberAfRf4ceZrcHaFillCommandScenesClusterRecallLocalScene8()           \
            emberAfRf4ceZrcHaFillCommandScenesClusterRecallLocalScene(8)


/** @brief Recall local scene 9
 *
 * Cluster: Scenes, Provides an interface for recalling local scene 9.
 * Command: RecallLocalScene9
 */
#define emberAfRf4ceZrcHaFillCommandScenesClusterRecallLocalScene9()           \
            emberAfRf4ceZrcHaFillCommandScenesClusterRecallLocalScene(9)


/** @brief Recall local scene 10
 *
 * Cluster: Scenes, Provides an interface for recalling local scene 10.
 * Command: RecallLocalScene10
 */
#define emberAfRf4ceZrcHaFillCommandScenesClusterRecallLocalScene10()          \
            emberAfRf4ceZrcHaFillCommandScenesClusterRecallLocalScene(10)


/** @brief Recall local scene 11
 *
 * Cluster: Scenes, Provides an interface for recalling local scene 11.
 * Command: RecallLocalScene11
 */
#define emberAfRf4ceZrcHaFillCommandScenesClusterRecallLocalScene11()          \
            emberAfRf4ceZrcHaFillCommandScenesClusterRecallLocalScene(11)


/** @brief Recall local scene 12
 *
 * Cluster: Scenes, Provides an interface for recalling local scene 12.
 * Command: RecallLocalScene12
 */
#define emberAfRf4ceZrcHaFillCommandScenesClusterRecallLocalScene12()          \
            emberAfRf4ceZrcHaFillCommandScenesClusterRecallLocalScene(12)


/** @brief Recall local scene 13
 *
 * Cluster: Scenes, Provides an interface for recalling local scene 13.
 * Command: RecallLocalScene13
 */
#define emberAfRf4ceZrcHaFillCommandScenesClusterRecallLocalScene13()          \
            emberAfRf4ceZrcHaFillCommandScenesClusterRecallLocalScene(13)


/** @brief Recall local scene 14
 *
 * Cluster: Scenes, Provides an interface for recalling local scene 14.
 * Command: RecallLocalScene14
 */
#define emberAfRf4ceZrcHaFillCommandScenesClusterRecallLocalScene14()          \
            emberAfRf4ceZrcHaFillCommandScenesClusterRecallLocalScene(14)


/** @brief Recall local scene 15
 *
 * Cluster: Scenes, Provides an interface for recalling local scene 15.
 * Command: RecallLocalScene15
 */
#define emberAfRf4ceZrcHaFillCommandScenesClusterRecallLocalScene15()          \
            emberAfRf4ceZrcHaFillCommandScenesClusterRecallLocalScene(15)
/** @} END Scenes Commands */


/** @name On/off Commands */
// @{
/** @brief Command description for Off
 *
 * Cluster: On/off, Attributes and commands for switching devices between 'On' and 'Off' states.
 * Command: Off
 */
#define emberAfRf4ceZrcHaFillCommandOnOffClusterOff()                          \
            emAfPluginRf4ceZrc20HaFillExternalBuffer("u",                   \
                                                     EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_ACTION_CODE_ON_OFF_OFF)


/** @brief Command description for On
 *
 * Cluster: On/off, Attributes and commands for switching devices between 'On' and 'Off' states.
 * Command: On
 */
#define emberAfRf4ceZrcHaFillCommandOnOffClusterOn()                           \
            emAfPluginRf4ceZrc20HaFillExternalBuffer("u",                   \
                                                     EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_ACTION_CODE_ON_OFF_ON)


/** @brief Command description for Toggle
 *
 * Cluster: On/off, Attributes and commands for switching devices between 'On' and 'Off' states.
 * Command: Toggle
 */
#define emberAfRf4ceZrcHaFillCommandOnOffClusterToggle()                       \
            emAfPluginRf4ceZrc20HaFillExternalBuffer("u",                   \
                                                     EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_ACTION_CODE_ON_OFF_TOGGLE)
/** @} END On/off Commands */


/** @name Level Control Commands */
// @{
/** @brief Command description for Move To Level
 *
 * Cluster: Level Control, Attributes and commands for controlling devices that can be set to a level between fully 'On' and fully 'Off.'
 * Command: MoveToLevel
 * @param level uint8_t
 * @param transitionTime uint16_t
 */
#define emberAfRf4ceZrcHaFillCommandLevelControlClusterMoveToLevel(level,           \
                                                                   transitionTime)  \
            emAfPluginRf4ceZrc20HaFillExternalBuffer("uuv",                      \
                                                     EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_ACTION_CODE_LEVEL_CONTROL_MOVE_TO_LEVEL, \
                                                     level,                                                                                  \
                                                     transitionTime)


/** @brief Command description for Move
 *
 * Cluster: Level Control, Attributes and commands for controlling devices that can be set to a level between fully 'On' and fully 'Off.'
 * Command: Move
 * @param moveMode uint8_t
 * @param rate uint8_t
 */
#define emberAfRf4ceZrcHaFillCommandLevelControlClusterMove(moveMode,          \
                                                            rate)              \
            emAfPluginRf4ceZrc20HaFillExternalBuffer("uuu",                 \
                                                     EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_ACTION_CODE_LEVEL_CONTROL_MOVE,  \
                                                     moveMode,                                                                       \
                                                     rate)


/** @brief Command description for Step
 *
 * Cluster: Level Control, Attributes and commands for controlling devices that can be set to a level between fully 'On' and fully 'Off.'
 * Command: Step
 * @param stepMode uint8_t
 * @param stepSize uint8_t
 * @param transitionTime uint16_t
 */
#define emberAfRf4ceZrcHaFillCommandLevelControlClusterStep(stepMode,          \
                                                            stepSize,          \
                                                            transitionTime)    \
            emAfPluginRf4ceZrc20HaFillExternalBuffer("uuuv",                \
                                                     EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_ACTION_CODE_LEVEL_CONTROL_STEP,  \
                                                     stepMode,                                                                       \
                                                     stepSize,                                                                       \
                                                     transitionTime)


/** @brief Command description for Stop
 *
 * Cluster: Level Control, Attributes and commands for controlling devices that can be set to a level between fully 'On' and fully 'Off.'
 * Command: Stop
 */
#define emberAfRf4ceZrcHaFillCommandLevelControlClusterStop()                  \
            emAfPluginRf4ceZrc20HaFillExternalBuffer("u",                   \
                                                     EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_ACTION_CODE_LEVEL_CONTROL_STOP)


/** @brief Command description for Move To Level With On/Off
 *
 * Cluster: Level Control, Attributes and commands for controlling devices that can be set to a level between fully 'On' and fully 'Off.'
 * Command: Move To Level With On/Off
 * @param level uint8_t
 * @param transitionTime uint16_t
 */
#define emberAfRf4ceZrcHaFillCommandLevelControlClusterMoveToLevelWithOnOff(level,          \
                                                                            transitionTime) \
            emAfPluginRf4ceZrc20HaFillExternalBuffer("uuv",                              \
                                                     EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_ACTION_CODE_LEVEL_CONTROL_MOVE_TO_LEVEL_WITH_ON_OFF, \
                                                     level,                                                                                              \
                                                     transitionTime)


/** @brief Command description for Move With On/Off
 *
 * Cluster: Level Control, Attributes and commands for controlling devices that can be set to a level between fully 'On' and fully 'Off.'
 * Command: Move With On/Off
 * @param moveMode uint8_t
 * @param rate uint8_t
 */
#define emberAfRf4ceZrcHaFillCommandLevelControlClusterMoveWithOnOff(moveMode, \
                                                                     rate)     \
            emAfPluginRf4ceZrc20HaFillExternalBuffer("uuu",                 \
                                                     EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_ACTION_CODE_LEVEL_CONTROL_MOVE_WITH_ON_OFF,  \
                                                     moveMode,                                                                                   \
                                                     rate)


/** @brief Command description for Step With On/Off
 *
 * Cluster: Level Control, Attributes and commands for controlling devices that can be set to a level between fully 'On' and fully 'Off.'
 * Command: Step With On/Off
 * @param stepMode uint8_t
 * @param stepSize uint8_t
 * @param transitionTime uint16_t
 */
#define emberAfRf4ceZrcHaFillCommandLevelControlClusterStepWithOnOff(stepMode,      \
                                                                     stepSize,      \
                                                                     transitionTime)\
            emAfPluginRf4ceZrc20HaFillExternalBuffer("uuuv",                     \
                                                     EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_ACTION_CODE_LEVEL_CONTROL_STEP_WITH_ON_OFF,  \
                                                     stepMode,                                                                                   \
                                                     stepSize,                                                                                   \
                                                     transitionTime)


/** @brief Command description for Stop With On/Off
 *
 * Cluster: Level Control, Attributes and commands for controlling devices that can be set to a level between fully 'On' and fully 'Off.'
 * Command: Stop With On/Off
 */
#define emberAfRf4ceZrcHaFillCommandLevelControlClusterStopWithOnOff()         \
            emAfPluginRf4ceZrc20HaFillExternalBuffer("u",                   \
                                                     EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_ACTION_CODE_LEVEL_CONTROL_STOP_WITH_ON_OFF)
/** @} END Level Control Commands */


/** @name Door Lock Commands */
// @{
/** @brief Locks the door
 *
 * Cluster: Door Lock, Provides an interface into a generic way to secure a door.
 * Command: LockDoor
 * @param pinRfidCode uint8_t*
 */
#define emberAfRf4ceZrcHaFillCommandDoorLockClusterLockDoor(pinRfidCode)       \
            emAfPluginRf4ceZrc20HaFillExternalBuffer("us",                  \
                                                     EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_ACTION_CODE_DOOR_LOCK_LOCK_DOOR, \
                                                     pinRfidCode)


/** @brief Unlocks the door
 *
 * Cluster: Door Lock, Provides an interface into a generic way to secure a door.
 * Command: UnlockDoor
 * @param pinRfidCode uint8_t*
 */
#define emberAfRf4ceZrcHaFillCommandDoorLockClusterUnlockDoor(pinRfidCode)     \
            emAfPluginRf4ceZrc20HaFillExternalBuffer("us",                  \
                                                     EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_ACTION_CODE_DOOR_LOCK_UNLOCK_DOOR,   \
                                                     pinRfidCode)


/** @brief Toggles the door lock from its current state to the opposite state locked or unlocked.
 *
 * Cluster: Door Lock, Provides an interface into a generic way to secure a door.
 * Command: Toggle
 * @param pinRfidCode uint8_t*
 */
#define emberAfRf4ceZrcHaFillCommandDoorLockClusterToggle(pinRfidCode)         \
            emAfPluginRf4ceZrc20HaFillExternalBuffer("us",                  \
                                                     EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_ACTION_CODE_DOOR_LOCK_TOGGLE,    \
                                                     pinRfidCode)


/** @brief Unlock the door with a timeout. When the timeout expires, the door will automatically re-lock.
 *
 * Cluster: Door Lock, Provides an interface into a generic way to secure a door.
 * Command: UnlockWithTimeout
 * @param timeoutInSeconds uint16_t
 * @param pinRfidCode uint8_t*
 */
#define emberAfRf4ceZrcHaFillCommandDoorLockClusterUnlockWithTimeout(timeoutInSeconds,  \
                                                                     pinRfidCode)       \
            emAfPluginRf4ceZrc20HaFillExternalBuffer("uvs",                          \
                                                     EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_ACTION_CODE_DOOR_LOCK_TOGGLE,    \
                                                     timeoutInSeconds,                                                               \
                                                     pinRfidCode)
/** @} END Door Lock Commands */


/** @name Window Covering Commands */
// @{
/** @brief Moves window covering to InstalledOpenLimit - Lift and InstalledOpenLimit - Tilt
 *
 * Cluster: Window Covering, Provides an interface for controlling and adjusting automatic window coverings.
 * Command: WindowCoveringUpOpen
 */
#define emberAfRf4ceZrcHaFillCommandWindowCoveringClusterUpOpen()              \
            emAfPluginRf4ceZrc20HaFillExternalBuffer("u",                   \
                                                     EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_ACTION_CODE_WINDOW_COVERING_UP_OPEN)


/** @brief Moves window covering to InstalledClosedLimit - Lift and InstalledCloseLimit - Tilt
 *
 * Cluster: Window Covering, Provides an interface for controlling and adjusting automatic window coverings.
 * Command: WindowCoveringDownClose
 */
#define emberAfRf4ceZrcHaFillCommandWindowCoveringClusterDownClose()           \
            emAfPluginRf4ceZrc20HaFillExternalBuffer("u",                   \
                                                     EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_ACTION_CODE_WINDOW_COVERING_DOWN_CLOSE)


/** @brief Stop any adjusting of window covering
 *
 * Cluster: Window Covering, Provides an interface for controlling and adjusting automatic window coverings.
 * Command: WindowCoveringStop
 */
#define emberAfRf4ceZrcHaFillCommandWindowCoveringClusterStop()                \
            emAfPluginRf4ceZrc20HaFillExternalBuffer("u",                   \
                                                     EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_ACTION_CODE_WINDOW_COVERING_STOP)


/** @brief Goto lift value specified
 *
 * Cluster: Window Covering, Provides an interface for controlling and adjusting automatic window coverings.
 * Command: WindowCoveringGoToLiftValue
 * @param liftValue uint16_t
 */
#define emberAfRf4ceZrcHaFillCommandWindowCoveringClusterGoToLiftValue(liftValue)   \
            emAfPluginRf4ceZrc20HaFillExternalBuffer("uv",                       \
                                                     EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_ACTION_CODE_WINDOW_COVERING_GO_TO_LIFT_VALUE,    \
                                                     liftValue)


/** @brief Goto lift percentage specified
 *
 * Cluster: Window Covering, Provides an interface for controlling and adjusting automatic window coverings.
 * Command: WindowCoveringGoToLiftPercentage
 * @param percentageLiftValue uint8_t
 */
#define emberAfRf4ceZrcHaFillCommandWindowCoveringClusterGoToLiftPercentage(percentageLiftValue)    \
            emAfPluginRf4ceZrc20HaFillExternalBuffer("uu",                                       \
                                                     EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_ACTION_CODE_WINDOW_COVERING_GO_TO_LIFT_PERCENTAGE,   \
                                                     percentageLiftValue)


/** @brief Goto tilt value specified
 *
 * Cluster: Window Covering, Provides an interface for controlling and adjusting automatic window coverings.
 * Command: WindowCoveringGoToTiltValue
 * @param tiltValue uint16_t
 */
#define emberAfRf4ceZrcHaFillCommandWindowCoveringClusterGoToTiltValue(tiltValue)   \
            emAfPluginRf4ceZrc20HaFillExternalBuffer("uv",                       \
                                                     EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_ACTION_CODE_WINDOW_COVERING_GO_TO_TILT_VALUE,    \
                                                     tiltValue)


/** @brief Goto tilt percentage specified
 *
 * Cluster: Window Covering, Provides an interface for controlling and adjusting automatic window coverings.
 * Command: WindowCoveringGoToTiltPercentage
 * @param percentageTiltValue uint8_t
 */
#define emberAfRf4ceZrcHaFillCommandWindowCoveringClusterGoToTiltPercentage(percentageTiltValue)    \
            emAfPluginRf4ceZrc20HaFillExternalBuffer("uu",                                       \
                                                     EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_ACTION_CODE_WINDOW_COVERING_GO_TO_TILT_PERCENTAGE,   \
                                                     percentageTiltValue)
/** @} END Window Covering Commands */


/** @name Thermostat Commands */
// @{
/** @brief Command description for SetpointRaiseLower
 *
 * Cluster: Thermostat, An interface for configuring and controlling the functionality of a thermostat.
 * Command: SetpointRaiseLower
 * @param mode uint8_t
 * @param amount int8_t
 */
#define emberAfRf4ceZrcHaFillCommandThermostatClusterSetpointRaiseLower(mode,  \
                                                                        amount)\
            emAfPluginRf4ceZrc20HaFillExternalBuffer("uuu",                 \
                                                     EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_ACTION_CODE_THERMOSTAT_SETPOINT_RAISE_LOWER, \
                                                     mode,                                                                                       \
                                                     amount)
/** @} END Thermostat Commands */


/** @name Color Control Commands */
// @{
/** @brief Move to specified hue.
 *
 * Cluster: Color Control, Attributes and commands for controlling the color properties of a color-capable light.
 * Command: MoveToHue
 * @param hue uint8_t
 * @param direction uint8_t
 * @param transitionTime uint16_t
 */
#define emberAfRf4ceZrcHaFillCommandColorControlClusterMoveToHue(hue,           \
                                                                 direction,     \
                                                                 transitionTime)\
            emAfPluginRf4ceZrc20HaFillExternalBuffer("uuuv",                 \
                                                     EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_ACTION_CODE_COLOR_CONTROL_MOVE_TO_HUE,   \
                                                     hue,                                                                                    \
                                                     direction,                                                                              \
                                                     transitionTime)


/** @brief Move hue up or down at specified rate.
 *
 * Cluster: Color Control, Attributes and commands for controlling the color properties of a color-capable light.
 * Command: MoveHue
 * @param moveMode uint8_t
 * @param rate uint8_t
 */
#define emberAfRf4ceZrcHaFillCommandColorControlClusterMoveHue(moveMode,       \
                                                               rate)           \
            emAfPluginRf4ceZrc20HaFillExternalBuffer("uuu",                 \
                                                     EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_ACTION_CODE_COLOR_CONTROL_MOVE_HUE,  \
                                                     moveMode,                                                                           \
                                                     rate)


/** @brief Step hue up or down by specified size at specified rate.
 *
 * Cluster: Color Control, Attributes and commands for controlling the color properties of a color-capable light.
 * Command: StepHue
 * @param stepMode uint8_t
 * @param stepSize uint8_t
 * @param transitionTime uint8_t
 */
#define emberAfRf4ceZrcHaFillCommandColorControlClusterStepHue(stepMode,       \
                                                               stepSize,       \
                                                               transitionTime) \
            emAfPluginRf4ceZrc20HaFillExternalBuffer("uuuu",                \
                                                     EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_ACTION_CODE_COLOR_CONTROL_STEP_HUE,  \
                                                     stepMode,                                                                           \
                                                     stepSize,                                                                           \
                                                     transitionTime)


/** @brief Move to specified saturation.
 *
 * Cluster: Color Control, Attributes and commands for controlling the color properties of a color-capable light.
 * Command: MoveToSaturation
 * @param saturation uint8_t
 * @param transitionTime uint16_t
 */
#define emberAfRf4ceZrcHaFillCommandColorControlClusterMoveToSaturation(saturation,     \
                                                                        transitionTime) \
            emAfPluginRf4ceZrc20HaFillExternalBuffer("uuv",                          \
                                                     EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_ACTION_CODE_COLOR_CONTROL_MOVE_TO_SATURATION,    \
                                                     saturation,                                                                                     \
                                                     transitionTime)


/** @brief Move saturation up or down at specified rate.
 *
 * Cluster: Color Control, Attributes and commands for controlling the color properties of a color-capable light.
 * Command: MoveSaturation
 * @param moveMode uint8_t
 * @param rate uint8_t
 */
#define emberAfRf4ceZrcHaFillCommandColorControlClusterMoveSaturation(moveMode,\
                                                                      rate)    \
            emAfPluginRf4ceZrc20HaFillExternalBuffer("uuu",                 \
                                                     EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_ACTION_CODE_COLOR_CONTROL_MOVE_SATURATION,   \
                                                     moveMode,                                                                                   \
                                                     rate)


/** @brief Step saturation up or down by specified size at specified rate.
 *
 * Cluster: Color Control, Attributes and commands for controlling the color properties of a color-capable light.
 * Command: StepSaturation
 * @param stepMode uint8_t
 * @param stepSize uint8_t
 * @param transitionTime uint8_t
 */
#define emberAfRf4ceZrcHaFillCommandColorControlClusterStepSaturation(stepMode,         \
                                                                      stepSize,         \
                                                                      transitionTime)   \
            emAfPluginRf4ceZrc20HaFillExternalBuffer("uuuu",                         \
                                                     EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_ACTION_CODE_COLOR_CONTROL_STEP_SATURATION,   \
                                                     stepMode,                                                                                   \
                                                     stepSize,                                                                                   \
                                                     transitionTime)


/** @brief Move to hue and saturation.
 *
 * Cluster: Color Control, Attributes and commands for controlling the color properties of a color-capable light.
 * Command: MoveToHueAndSaturation
 * @param hue uint8_t
 * @param saturation uint8_t
 * @param transitionTime uint16_t
 */
#define emberAfRf4ceZrcHaFillCommandColorControlClusterMoveToHueAndSaturation(hue,              \
                                                                              saturation,       \
                                                                              transitionTime)   \
            emAfPluginRf4ceZrc20HaFillExternalBuffer("uuuv",                                 \
                                                     EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_ACTION_CODE_COLOR_CONTROL_MOVE_TO_HUE_AND_SATURATION,    \
                                                     hue,                                                                                                    \
                                                     saturation,                                                                                             \
                                                     transitionTime)


/** @brief Move to specified color.
 *
 * Cluster: Color Control, Attributes and commands for controlling the color properties of a color-capable light.
 * Command: MoveToColor
 * @param colorX uint16_t
 * @param colorY uint16_t
 * @param transitionTime uint16_t
 */
#define emberAfRf4ceZrcHaFillCommandColorControlClusterMoveToColor(colorX,          \
                                                                   colorY,          \
                                                                   transitionTime)  \
            emAfPluginRf4ceZrc20HaFillExternalBuffer("uvvv",                     \
                                                     EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_ACTION_CODE_COLOR_CONTROL_MOVE_TO_COLOR, \
                                                     colorX,                                                                                 \
                                                     colorY,                                                                                 \
                                                     transitionTime)


/** @brief Moves the color.
 *
 * Cluster: Color Control, Attributes and commands for controlling the color properties of a color-capable light.
 * Command: MoveColor
 * @param rateX int16_t
 * @param rateY int16_t
 */
#define emberAfRf4ceZrcHaFillCommandColorControlClusterMoveColor(rateX,        \
                                                                 rateY)        \
            emAfPluginRf4ceZrc20HaFillExternalBuffer("uvv",                 \
                                                     EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_ACTION_CODE_COLOR_CONTROL_MOVE_COLOR,    \
                                                     rateX,                                                                                  \
                                                     rateY)


/** @brief Steps the lighting to a specific color.
 *
 * Cluster: Color Control, Attributes and commands for controlling the color properties of a color-capable light.
 * Command: StepColor
 * @param stepX int16_t
 * @param stepY int16_t
 * @param transitionTime uint16_t
 */
#define emberAfRf4ceZrcHaFillCommandColorControlClusterStepColor(stepX,         \
                                                                 stepY,         \
                                                                 transitionTime)\
            emAfPluginRf4ceZrc20HaFillExternalBuffer("uvvv",                 \
                                                     EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_ACTION_CODE_COLOR_CONTROL_STEP_COLOR,   \
                                                     stepX,                                                                                 \
                                                     stepY,                                                                                 \
                                                     transitionTime)


/** @brief Moves the lighting to a specific color temperature.
 *
 * Cluster: Color Control, Attributes and commands for controlling the color properties of a color-capable light.
 * Command: MoveToColorTemperature
 * @param colorTemperature uint16_t
 * @param transitionTime uint16_t
 */
#define emberAfRf4ceZrcHaFillCommandColorControlClusterMoveToColorTemperature(colorTemperature, \
                                                                              transitionTime)   \
            emAfPluginRf4ceZrc20HaFillExternalBuffer("uvv",                                  \
                                                     EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_ACTION_CODE_COLOR_CONTROL_MOVE_TO_COLOR_TEMPERATURE, \
                                                     colorTemperature,                                                                                   \
                                                     transitionTime)
/** @} END Color Control Commands */


/** @name IAS ACE Commands */
// @{
/** @brief Command description for Arm
 *
 * Cluster: IAS ACE, Attributes and commands for IAS Ancillary Control Equipment.
 * Command: Arm
 * @param armMode uint8_t
 */
#define emberAfRf4ceZrcHaFillCommandIASACEClusterArm(armMode)                  \
            emAfPluginRf4ceZrc20HaFillExternalBuffer("uu",                  \
                                                     EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_ACTION_CODE_IAS_ACE_ARM, \
                                                     armMode)


/** @brief Command description for Bypass
 *
 * Cluster: IAS ACE, Attributes and commands for IAS Ancillary Control Equipment.
 * Command: Bypass
 * @param numberOfZones uint8_t
 * @param zoneIds uint8_t*
 * @param zoneIdsLen uint16_t
 */
#define emberAfRf4ceZrcHaFillCommandIASACEClusterBypass(numberOfZones,         \
                                                        zoneIds,               \
                                                        zoneIdsLen)            \
            emAfPluginRf4ceZrc20HaFillExternalBuffer("uub",                 \
                                                     EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_ACTION_CODE_IAS_ACE_BYPASS,  \
                                                     numberOfZones,                                                              \
                                                     zoneIds,                                                                    \
                                                     zoneIdsLen)


/** @brief Command description for Emergency
 *
 * Cluster: IAS ACE, Attributes and commands for IAS Ancillary Control Equipment.
 * Command: Emergency
 */
#define emberAfRf4ceZrcHaFillCommandIASACEClusterEmergency()                   \
            emAfPluginRf4ceZrc20HaFillExternalBuffer("u",                   \
                                                     EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_ACTION_CODE_IAS_ACE_EMERGENCY)


/** @brief Command description for Fire
 *
 * Cluster: IAS ACE, Attributes and commands for IAS Ancillary Control Equipment.
 * Command: Fire
 */
#define emberAfRf4ceZrcHaFillCommandIASACEClusterFire()                        \
            emAfPluginRf4ceZrc20HaFillExternalBuffer("u",                   \
                                                     EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_ACTION_CODE_IAS_ACE_FIRE)


/** @brief Command description for Panic
 *
 * Cluster: IAS ACE, Attributes and commands for IAS Ancillary Control Equipment.
 * Command: Panic
 */
#define emberAfRf4ceZrcHaFillCommandIASACEClusterPanic()                       \
            emAfPluginRf4ceZrc20HaFillExternalBuffer("u",                   \
                                                     EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_ACTION_CODE_IAS_ACE_PANIC)
/** @} END IAS ACE Commands */


/** @brief Previous destination group.
 *
 * Command: PreviousDestinationGroup
 */
#define emberAfRf4ceZrcHaFillCommandPreviousDestinationGroup()                 \
            emAfPluginRf4ceZrc20HaFillExternalBuffer("u",                   \
                                                     EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_ACTION_CODE_PREVIOUS_DESTINATION_GROUP)


/** @brief Next destination group.
 *
 * Command: NextDestinationGroup
 */
#define emberAfRf4ceZrcHaFillCommandNextDestinationGroup()                     \
            emAfPluginRf4ceZrc20HaFillExternalBuffer("u",                   \
                                                     EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_ACTION_CODE_NEXT_DESTINATION_GROUP)

#endif // __RF4CE_ZRC20_HA_CLIENT_H__

// END addtogroup
