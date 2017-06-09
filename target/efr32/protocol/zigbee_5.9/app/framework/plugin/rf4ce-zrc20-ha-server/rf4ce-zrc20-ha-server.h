// Copyright 2014 Silicon Laboratories, Inc.

#ifndef __RF4CE_ZRC20_HA_SERVER_H__
#define __RF4CE_ZRC20_HA_SERVER_H__

#include "app/framework/plugin/rf4ce-gdp/rf4ce-gdp-types.h"

/**
 * @addtogroup rf4ce-zrc20-ha-server
 *
 * The ZigBee Remote Control 2.0 (ZRC2.0) Home Automation (HA) Server plugin
 * implements the optional HA interoperability feature of the ZRC2.0 profile
 * for HA action recipients.  HA interoperability allows RF4CE devices to
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
 * Users will want to take note of the callback
 * emberAfPluginZrc20HaServerHaActionSentCallback (see @ref callback for
 * more information regarding callbacks). This callback conveys to the
 * application that the HA server has sent an HA action to a ZCL network.
 * 
 * Furthermore, customers can use this plugin to manage an application's store
 * of HA Attributes and Logical Device information. There are functions
 * to interface with the database of these items, as well as definitions 
 * of constants for an application to use.
 *
 * This plugin utilizes the action functionality provided by the @ref
 * rf4ce-zrc20 plugin.  Support for the optional HA action originator feature
 * is provided by the @ref rf4ce-zrc20-ha-client plugin.
 * @{
 */

// EMBER_AF_MAXIMUM_SEND_PAYLOAD_LENGTH is set in config.h
#define EMBER_AF_PLUGIN_RF4CE_ZRC20_HA_SERVER_ZCL_BUFFER_SIZE EMBER_AF_MAXIMUM_SEND_PAYLOAD_LENGTH

#define ZRC_HA_SERVER_NUM_OF_HA_INSTANCES   32

#define ZRC_ACTION_ID_HIGH_NIBBLE_MASK                              0xF0
#define ZRC_ACTION_ID_LOW_NIBBLE_MASK                               0x0F


/* ZRC HA Attribute IDs and size.
 * Size contains HA attribute status and HA attribute value. */
#define ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE0_ID                     0x00
#define ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE1_ID                     0x01
#define ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE2_ID                     0x02
#define ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE3_ID                     0x03
#define ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE4_ID                     0x04
#define ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE5_ID                     0x05
#define ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE6_ID                     0x06
#define ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE7_ID                     0x07
#define ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE8_ID                     0x08
#define ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE9_ID                     0x09
#define ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE10_ID                    0x0A
#define ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE11_ID                    0x0B
#define ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE12_ID                    0x0C
#define ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE13_ID                    0x0D
#define ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE14_ID                    0x0E
#define ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE15_ID                    0x0F
#define ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE_ID_SIZE                 1
#define ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE_SIZE                    1
#define ZRC_HA_SCENE_LOCAL_SCENE_VALID0_ID                              0x10
#define ZRC_HA_SCENE_LOCAL_SCENE_VALID1_ID                              0x11
#define ZRC_HA_SCENE_LOCAL_SCENE_VALID2_ID                              0x12
#define ZRC_HA_SCENE_LOCAL_SCENE_VALID3_ID                              0x13
#define ZRC_HA_SCENE_LOCAL_SCENE_VALID4_ID                              0x14
#define ZRC_HA_SCENE_LOCAL_SCENE_VALID5_ID                              0x15
#define ZRC_HA_SCENE_LOCAL_SCENE_VALID6_ID                              0x16
#define ZRC_HA_SCENE_LOCAL_SCENE_VALID7_ID                              0x17
#define ZRC_HA_SCENE_LOCAL_SCENE_VALID8_ID                              0x18
#define ZRC_HA_SCENE_LOCAL_SCENE_VALID9_ID                              0x19
#define ZRC_HA_SCENE_LOCAL_SCENE_VALID10_ID                             0x1A
#define ZRC_HA_SCENE_LOCAL_SCENE_VALID11_ID                             0x1B
#define ZRC_HA_SCENE_LOCAL_SCENE_VALID12_ID                             0x1C
#define ZRC_HA_SCENE_LOCAL_SCENE_VALID13_ID                             0x1D
#define ZRC_HA_SCENE_LOCAL_SCENE_VALID14_ID                             0x1E
#define ZRC_HA_SCENE_LOCAL_SCENE_VALID15_ID                             0x1F
#define ZRC_HA_SCENE_LOCAL_SCENE_VALID_ID_SIZE                          1
#define ZRC_HA_SCENE_LOCAL_SCENE_VALID_SIZE                             1
#define ZRC_HA_ON_OFF_ON_OFF_ID                                         0x20
#define ZRC_HA_ON_OFF_ON_OFF_ID_SIZE                                    1
#define ZRC_HA_ON_OFF_ON_OFF_SIZE                                       1
#define ZRC_HA_LEVEL_CONTROL_CURRENT_LEVEL_ID                           0x30
#define ZRC_HA_LEVEL_CONTROL_CURRENT_LEVEL_ID_SIZE                      1
#define ZRC_HA_LEVEL_CONTROL_CURRENT_LEVEL_SIZE                         1
#define ZRC_HA_DOOR_LOCK_LOCK_DOOR_RESPONSE_ID                          0x40
#define ZRC_HA_DOOR_LOCK_LOCK_DOOR_RESPONSE_ID_SIZE                     1
#define ZRC_HA_DOOR_LOCK_LOCK_DOOR_RESPONSE_SIZE                        1
#define ZRC_HA_DOOR_LOCK_UNLOCK_DOOR_RESPONSE_ID                        0x41
#define ZRC_HA_DOOR_LOCK_UNLOCK_DOOR_RESPONSE_ID_SIZE                   1
#define ZRC_HA_DOOR_LOCK_UNLOCK_DOOR_RESPONSE_SIZE                      1
#define ZRC_HA_DOOR_LOCK_TOGGLE_RESPONSE_ID                             0x42
#define ZRC_HA_DOOR_LOCK_TOGGLE_RESPONSE_ID_SIZE                        1
#define ZRC_HA_DOOR_LOCK_TOGGLE_RESPONSE_SIZE                           1
#define ZRC_HA_DOOR_LOCK_UNLOCK_WITH_TIMEOUT_RESPONSE_ID                0x43
#define ZRC_HA_DOOR_LOCK_UNLOCK_WITH_TIMEOUT_RESPONSE_ID_SIZE           1
#define ZRC_HA_DOOR_LOCK_UNLOCK_WITH_TIMEOUT_RESPONSE_SIZE              1
#define ZRC_HA_WINDOW_COVERING_CURRENT_POSITION_LIFT_PERCENTAGE_ID      0x50
#define ZRC_HA_WINDOW_COVERING_CURRENT_POSITION_LIFT_PERCENTAGE_ID_SIZE 1
#define ZRC_HA_WINDOW_COVERING_CURRENT_POSITION_LIFT_PERCENTAGE_SIZE    1
#define ZRC_HA_WINDOW_COVERING_CURRENT_POSITION_TILT_PERCENTAGE_ID      0x51
#define ZRC_HA_WINDOW_COVERING_CURRENT_POSITION_TILT_PERCENTAGE_ID_SIZE 1
#define ZRC_HA_WINDOW_COVERING_CURRENT_POSITION_TILT_PERCENTAGE_SIZE    1
#define ZRC_HA_THERMOSTAT_LOCAL_TEMPERATURE_ID                          0x60
#define ZRC_HA_THERMOSTAT_LOCAL_TEMPERATURE_ID_SIZE                     1
#define ZRC_HA_THERMOSTAT_LOCAL_TEMPERATURE_SIZE                        2
#define ZRC_HA_THERMOSTAT_OCCUPIED_COOLING_SETPOINT_ID                  0x61
#define ZRC_HA_THERMOSTAT_OCCUPIED_COOLING_SETPOINT_ID_SIZE             1
#define ZRC_HA_THERMOSTAT_OCCUPIED_COOLING_SETPOINT_SIZE                2
#define ZRC_HA_THERMOSTAT_OCCUPIED_HEATING_SETPOINT_ID                  0x62
#define ZRC_HA_THERMOSTAT_OCCUPIED_HEATING_SETPOINT_ID_SIZE             1
#define ZRC_HA_THERMOSTAT_OCCUPIED_HEATING_SETPOINT_SIZE                2
#define ZRC_HA_COLOR_CONTROL_CURRENT_HUE_ID                             0x70
#define ZRC_HA_COLOR_CONTROL_CURRENT_HUE_ID_SIZE                        1
#define ZRC_HA_COLOR_CONTROL_CURRENT_HUE_SIZE                           1
#define ZRC_HA_COLOR_CONTOL_CURRENT_SATURATION_ID                       0x71
#define ZRC_HA_COLOR_CONTOL_CURRENT_SATURATION_ID_SIZE                  1
#define ZRC_HA_COLOR_CONTOL_CURRENT_SATURATION_SIZE                     1
#define ZRC_HA_COLOR_CONTOL_CURRENT_X_ID                                0x72
#define ZRC_HA_COLOR_CONTOL_CURRENT_X_ID_SIZE                           1
#define ZRC_HA_COLOR_CONTOL_CURRENT_X_SIZE                              2
#define ZRC_HA_COLOR_CONTOL_CURRENT_Y_ID                                0x73
#define ZRC_HA_COLOR_CONTOL_CURRENT_Y_ID_SIZE                           1
#define ZRC_HA_COLOR_CONTOL_CURRENT_Y_SIZE                              2
#define ZRC_HA_COLOR_CONTOL_COLOR_MODE_ID                               0x74
#define ZRC_HA_COLOR_CONTOL_COLOR_MODE_ID_SIZE                          1
#define ZRC_HA_COLOR_CONTOL_COLOR_MODE_SIZE                             1
#define ZRC_HA_IAS_ACE_ARM_RESPONSE_ID                                  0xC4
#define ZRC_HA_IAS_ACE_ARM_RESPONSE_ID_SIZE                             1
#define ZRC_HA_IAS_ACE_ARM_RESPONSE_SIZE                                1

#define ZRC_HA_ATTRIBUTE_STATUS_TABLE_SIZE                                     \
  (ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE_ID_SIZE * 16 +                      \
   ZRC_HA_SCENE_LOCAL_SCENE_VALID_ID_SIZE * 16 +                               \
   ZRC_HA_ON_OFF_ON_OFF_ID_SIZE +                                              \
   ZRC_HA_LEVEL_CONTROL_CURRENT_LEVEL_ID_SIZE +                                \
   ZRC_HA_DOOR_LOCK_LOCK_DOOR_RESPONSE_ID_SIZE +                               \
   ZRC_HA_DOOR_LOCK_UNLOCK_DOOR_RESPONSE_ID_SIZE +                             \
   ZRC_HA_DOOR_LOCK_TOGGLE_RESPONSE_ID_SIZE +                                  \
   ZRC_HA_DOOR_LOCK_UNLOCK_WITH_TIMEOUT_RESPONSE_ID_SIZE +                     \
   ZRC_HA_WINDOW_COVERING_CURRENT_POSITION_LIFT_PERCENTAGE_ID_SIZE +           \
   ZRC_HA_WINDOW_COVERING_CURRENT_POSITION_TILT_PERCENTAGE_ID_SIZE +           \
   ZRC_HA_THERMOSTAT_LOCAL_TEMPERATURE_ID_SIZE +                               \
   ZRC_HA_THERMOSTAT_OCCUPIED_COOLING_SETPOINT_ID_SIZE +                       \
   ZRC_HA_THERMOSTAT_OCCUPIED_HEATING_SETPOINT_ID_SIZE +                       \
   ZRC_HA_COLOR_CONTROL_CURRENT_HUE_ID_SIZE +                                  \
   ZRC_HA_COLOR_CONTOL_CURRENT_SATURATION_ID_SIZE +                            \
   ZRC_HA_COLOR_CONTOL_CURRENT_X_ID_SIZE +                                     \
   ZRC_HA_COLOR_CONTOL_CURRENT_Y_ID_SIZE +                                     \
   ZRC_HA_COLOR_CONTOL_COLOR_MODE_ID_SIZE +                                    \
   ZRC_HA_IAS_ACE_ARM_RESPONSE_ID_SIZE)

#define ZRC_HA_ATTRIBUTE_TABLE_SIZE                                            \
    (ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE_SIZE * 16 +                       \
     ZRC_HA_SCENE_LOCAL_SCENE_VALID_SIZE * 16 +                                \
     ZRC_HA_ON_OFF_ON_OFF_SIZE +                                               \
     ZRC_HA_LEVEL_CONTROL_CURRENT_LEVEL_SIZE +                                 \
     ZRC_HA_DOOR_LOCK_LOCK_DOOR_RESPONSE_SIZE +                                \
     ZRC_HA_DOOR_LOCK_UNLOCK_DOOR_RESPONSE_SIZE +                              \
     ZRC_HA_DOOR_LOCK_TOGGLE_RESPONSE_SIZE +                                   \
     ZRC_HA_DOOR_LOCK_UNLOCK_WITH_TIMEOUT_RESPONSE_SIZE +                      \
     ZRC_HA_WINDOW_COVERING_CURRENT_POSITION_LIFT_PERCENTAGE_SIZE +            \
     ZRC_HA_WINDOW_COVERING_CURRENT_POSITION_TILT_PERCENTAGE_SIZE +            \
     ZRC_HA_THERMOSTAT_LOCAL_TEMPERATURE_SIZE +                                \
     ZRC_HA_THERMOSTAT_OCCUPIED_COOLING_SETPOINT_SIZE +                        \
     ZRC_HA_THERMOSTAT_OCCUPIED_HEATING_SETPOINT_SIZE +                        \
     ZRC_HA_COLOR_CONTROL_CURRENT_HUE_SIZE +                                   \
     ZRC_HA_COLOR_CONTOL_CURRENT_SATURATION_SIZE +                             \
     ZRC_HA_COLOR_CONTOL_CURRENT_X_SIZE +                                      \
     ZRC_HA_COLOR_CONTOL_CURRENT_Y_SIZE +                                      \
     ZRC_HA_COLOR_CONTOL_COLOR_MODE_SIZE +                                     \
     ZRC_HA_IAS_ACE_ARM_RESPONSE_SIZE)

#define HA_ATTRIBUTE_STATUS_LENGTH                1
//#define HA_ATTRIBUTE_STATUS_OFFSET                0
//#define HA_ATTRIBUTE_VALUE_OFFSET                 1

// The status byte for each HA attribute is defined by the following.
#define HA_ATTRIBUTE_STATUS_CHANGED_FLAG          0x01


typedef struct
{
    EmberOutgoingMessageType type;
    uint16_t indexOrDestination;
    uint8_t sourceEndpoint;
    uint8_t destinationEndpoint;
} DestStruct;

typedef struct
{
    uint8_t id;
    uint8_t length;
} HaAttributesInfo;

//typedef struct
//{
//  uint8_t stat[ZRC_HA_ATTRIBUTE_STATUS_TABLE_SIZE/8+1];
//} HaAttributesStat;

//typedef struct
//{
//  uint8_t val[ZRC_HA_ATTRIBUTE_TABLE_SIZE];
//} HaAttributesVal;



/** @brief  Clear all HA attributes on the server.
 *
 */
void emberAfPluginRf4ceZrc20HaServerClearAllHaAttributes(void);


/** @brief  Get selected HA attribute on the server.
 *
 * @param  pairingIndex  The pairing index the get HA attribute command should
 * be sent to.
 *
 * @param  haInstanceId  The instance ID of the HA attribute to get.
 *
 * @param  haAttributeId  The attribute ID of the HA attribute to get.
 *
 * @param  haAttribute  The HA attribute structure describing the HA attribute.
 *
 * @return  An ::EmberAfRf4ceGdpAttributeStatus value indicating whether the get
 * HA attribute command was successfully sent out or the reason of failure.
 */
EmberAfRf4ceGdpAttributeStatus emberAfPluginRf4ceZrc20HaServerGetHaAttribute(uint8_t pairingIndex,
                                                                             uint8_t haInstanceId,
                                                                             uint8_t haAttributeId,
                                                                             EmberAfRf4ceZrcHomeAutomationAttribute *haAttribute);


/** @brief  Set selected HA attribute on the server.
 *
 * @param  pairingIndex  The pairing index the set HA attribute command should
 * be sent to.
 *
 * @param  haInstanceId  The instance ID of the HA attribute to set.
 *
 * @param  haAttributeId  The attribute ID of the HA attribute to set.
 *
 * @param  haAttribute  The HA attribute structure describing the HA attribute.
 *
 * @return  An ::EmberAfRf4ceGdpAttributeStatus value indicating whether the set
 * HA attribute command was successfully sent out or the reason of failure.
 */
EmberAfRf4ceGdpAttributeStatus emberAfPluginRf4ceZrc20HaServerSetHaAttribute(uint8_t pairingIndex,
                                                                             uint8_t haInstanceId,
                                                                             uint8_t haAttributeId,
                                                                             EmberAfRf4ceZrcHomeAutomationAttribute *haAttribute);


/** @brief  Add mapping to HA logical device on the server.
 *
 * @param  pairingIndex  The pairing index to which the HA logical device is
 * mapped.
 *
 * @param  haInstanceId  The instance ID to which the HA logical device is
 * mapped.
 *
 * @param  destIndex  The index of the HA logical device in the logical devices
 * table.
 *
 * @return  An ::EmberStatus value indicating whether adding mapping to the HA
 * logical device was successfully sent out or the reason of failure.
 */
EmberStatus emberAfPluginRf4ceZrc20HaMappingToLogicalDeviceAdd(uint8_t pairingIndex,
                                                               uint8_t haInstanceId,
                                                               uint8_t destIndex);


/** @brief  Remove mapping to HA logical device on the server.
 *
 * @param  destIndex  The index of the HA logical device in the logical devices
 * table.
 *
 * @return  An ::EmberStatus value indicating whether removing mapping to the HA
 * logical device was successfully sent out or the reason of failure.
 */
EmberStatus emberAfPluginRf4ceZrc20HaMappingToLogicalDeviceRemove(uint8_t destIndex);


/** @brief  Get mapping to HA logical device on the server.
 *
 * @param  pairingIndex  The pairing index to which the HA logical device is
 * mapped.
 *
 * @param  haInstanceId  The instance ID to which the HA logical device is
 * mapped.
 *
 * @param  destIndex  The index of the HA logical device in the logical devices
 * table.
 *
 * @return  An ::EmberStatus value indicating whether getting mapping to the HA
 * logical device was successfully sent out or the reason of failure.
 */
EmberStatus emberAfPluginRf4ceZrc20HaMappingToLogicalDeviceGet(uint8_t pairingIndex,
                                                               uint8_t haInstanceId,
                                                               uint8_t* destIndex);


/** @brief  Add HA logical device to the logical devices table on the server.
 *
 * @param  dest  The destination structure describing the logical device to add.
 *
 * @param  index  The index of the logical device table to which the HA logical
 * device was added.
 *
 * @return  An ::EmberStatus value indicating whether adding logical device
 * was successfully sent out or the reason of failure.
 */
EmberStatus emberAfPluginRf4ceZrc20HaLogicalDeviceDestinationAdd(DestStruct* dest,
                                                                 uint8_t* index);


/** @brief  Remove HA logical device from the logical devices table on the
 * server.
 *
 * @param  dest  The destination structure describing the logical device to
 * remove.
 *
 * @param  index  The index of the logical device table from which the HA
 * logical device was removed.
 *
 * @return  An ::EmberStatus value indicating whether removing logical device
 * was successfully sent out or the reason of failure.
 */
EmberStatus emberAfPluginRf4ceZrc20HaLogicalDeviceDestinationRemove(DestStruct* dest,
                                                                    uint8_t* index);


/** @brief  Get the size of the HA logical devices table on the server.
 *
 * @return  Size of the HA logical devices table.
 */
uint8_t emberAfPluginRf4ceZrc20HaLogicalDeviceDestinationTableSize(void);


/** @brief  Get the destination of the HA logical device on the server.
 *
 * @param  pairingIndex  The pairing index to which the HA logical device is
 * mapped.
 *
 * @param  haInstanceId  The instance ID to which the HA logical device is
 * mapped.
 *
 * @param  dest  The destination structure describing the HA logical device to
 * get.
 *
 * @return  An ::EmberStatus value indicating whether getting HA logical device
 * destination was successfully sent out or the reason of failure.
 */
EmberStatus emberAfPluginRf4ceZrc20HaLogicalDeviceDestinationGet(uint8_t pairingIndex,
                                                                 uint8_t haInstanceId,
                                                                 DestStruct* dest);


/** @brief  Look up the index of the HA logical device on the server.
 *
 * @param  dest  The destination structure describing the HA logical device to
 * look up.
 *
 * @param  index  The index of the HA logical device in the logical devices
 * table.
 *
 * @return  An ::EmberStatus value indicating whether looking up HA logical
 * device destination was successfully sent out or the reason of failure.
 */
EmberStatus emberAfPluginRf4ceZrc20HaLogicalDeviceIndexLookUp(DestStruct* dest,
                                                              uint8_t* index);


/** @brief  Clear all HA logical devices and mappings to logical devices.
 *
 */
void emberAfPluginRf4ceZrc20HaLogicalDeviceAndInstanceToLogicalDeviceMappingClear(void);


/** @brief  Add HA logical device and map it to pairing index and HA instance
 * ID.
 *
 * @param  pairingIndex  The pairing index to which the HA logical device will
 * be mapped.
 *
 * @param  haInstanceId  The instance ID to which the HA logical device will be
 * mapped.
 *
 * @param  dest  The destination structure describing the HA logical device to
 * map.
 *
 * @return  An ::EmberStatus value indicating whether mapping the HA logical
 * device was successfully sent out or the reason of failure.
 */
EmberStatus emberAfPluginRf4ceZrc20HaLogicalDeviceAndInstanceToLogicalDeviceMappingAdd(uint8_t pairingIndex,
                                                                                       uint8_t haInstanceId,
                                                                                       DestStruct* dest);


/** @brief  Remove HA logical device and its mappings.
 *
 * @param  dest  The destination structure describing the HA logical device to
 * remove.
 *
 * @return  An ::EmberStatus value indicating whether removing the HA logical
 * device was successfully sent out or the reason of failure.
 */
EmberStatus emberAfPluginRf4ceZrc20HaLogicalDeviceAndInstanceToLogicalDeviceMappingRemove(DestStruct* dest);



EmberStatus emAfRf4ceZrc20ParseHaActionAndForwardToZclNetwork(const EmberAfRf4ceZrcActionRecord *record);
void emAfRf4ceZrc20ClearLogicalDevicesTable(void);
void emAfRf4ceZrc20ClearInstanceToLogicalDeviceTable(void);
EmberStatus emAfRf4ceZrc20AddLogicalDeviceDestination(DestStruct* dest,
                                                      uint8_t* index);
EmberStatus emAfRf4ceZrc20RemoveLogicalDeviceDestination(uint8_t destIndex);

uint8_t GetLogicalDeviceDestination(uint8_t i,
                                  DestStruct* dest);
void DestLookup(uint8_t pairingIndex,
                uint8_t haInstanceId,
                DestStruct* dest);



#endif // __RF4CE_ZRC20_HA_SERVER_H__

// END rf4ce-zrc20-ha-server
