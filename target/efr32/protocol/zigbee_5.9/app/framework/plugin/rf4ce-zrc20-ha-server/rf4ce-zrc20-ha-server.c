// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "app/framework/plugin/rf4ce-profile/rf4ce-profile.h"
#include "app/framework/plugin/rf4ce-zrc20/rf4ce-zrc20.h"
#include "app/framework/plugin/rf4ce-zrc20/rf4ce-zrc20-internal.h"
#include "rf4ce-zrc20-ha-server.h"


static const HaAttributesInfo aplHaAttributesInfoTable[] =
       {{ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE0_ID, ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE_SIZE},
        {ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE1_ID, ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE_SIZE},
        {ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE2_ID, ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE_SIZE},
        {ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE3_ID, ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE_SIZE},
        {ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE4_ID, ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE_SIZE},
        {ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE5_ID, ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE_SIZE},
        {ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE6_ID, ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE_SIZE},
        {ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE7_ID, ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE_SIZE},
        {ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE8_ID, ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE_SIZE},
        {ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE9_ID, ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE_SIZE},
        {ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE10_ID,ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE_SIZE},
        {ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE11_ID,ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE_SIZE},
        {ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE12_ID,ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE_SIZE},
        {ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE13_ID,ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE_SIZE},
        {ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE14_ID,ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE_SIZE},
        {ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE15_ID,ZRC_HA_SCENE_STORE_LOCAL_SCENE_RESPONSE_SIZE},
        {ZRC_HA_SCENE_LOCAL_SCENE_VALID0_ID, ZRC_HA_SCENE_LOCAL_SCENE_VALID_SIZE},
        {ZRC_HA_SCENE_LOCAL_SCENE_VALID1_ID, ZRC_HA_SCENE_LOCAL_SCENE_VALID_SIZE},
        {ZRC_HA_SCENE_LOCAL_SCENE_VALID2_ID, ZRC_HA_SCENE_LOCAL_SCENE_VALID_SIZE},
        {ZRC_HA_SCENE_LOCAL_SCENE_VALID3_ID, ZRC_HA_SCENE_LOCAL_SCENE_VALID_SIZE},
        {ZRC_HA_SCENE_LOCAL_SCENE_VALID4_ID, ZRC_HA_SCENE_LOCAL_SCENE_VALID_SIZE},
        {ZRC_HA_SCENE_LOCAL_SCENE_VALID5_ID, ZRC_HA_SCENE_LOCAL_SCENE_VALID_SIZE},
        {ZRC_HA_SCENE_LOCAL_SCENE_VALID6_ID, ZRC_HA_SCENE_LOCAL_SCENE_VALID_SIZE},
        {ZRC_HA_SCENE_LOCAL_SCENE_VALID7_ID, ZRC_HA_SCENE_LOCAL_SCENE_VALID_SIZE},
        {ZRC_HA_SCENE_LOCAL_SCENE_VALID8_ID, ZRC_HA_SCENE_LOCAL_SCENE_VALID_SIZE},
        {ZRC_HA_SCENE_LOCAL_SCENE_VALID9_ID, ZRC_HA_SCENE_LOCAL_SCENE_VALID_SIZE},
        {ZRC_HA_SCENE_LOCAL_SCENE_VALID10_ID,ZRC_HA_SCENE_LOCAL_SCENE_VALID_SIZE},
        {ZRC_HA_SCENE_LOCAL_SCENE_VALID11_ID,ZRC_HA_SCENE_LOCAL_SCENE_VALID_SIZE},
        {ZRC_HA_SCENE_LOCAL_SCENE_VALID12_ID,ZRC_HA_SCENE_LOCAL_SCENE_VALID_SIZE},
        {ZRC_HA_SCENE_LOCAL_SCENE_VALID13_ID,ZRC_HA_SCENE_LOCAL_SCENE_VALID_SIZE},
        {ZRC_HA_SCENE_LOCAL_SCENE_VALID14_ID,ZRC_HA_SCENE_LOCAL_SCENE_VALID_SIZE},
        {ZRC_HA_SCENE_LOCAL_SCENE_VALID15_ID,ZRC_HA_SCENE_LOCAL_SCENE_VALID_SIZE},
        {ZRC_HA_ON_OFF_ON_OFF_ID, ZRC_HA_ON_OFF_ON_OFF_SIZE},
        {ZRC_HA_LEVEL_CONTROL_CURRENT_LEVEL_ID, ZRC_HA_LEVEL_CONTROL_CURRENT_LEVEL_SIZE},
        {ZRC_HA_DOOR_LOCK_LOCK_DOOR_RESPONSE_ID, ZRC_HA_DOOR_LOCK_LOCK_DOOR_RESPONSE_SIZE},
        {ZRC_HA_DOOR_LOCK_UNLOCK_DOOR_RESPONSE_ID, ZRC_HA_DOOR_LOCK_UNLOCK_DOOR_RESPONSE_SIZE},
        {ZRC_HA_DOOR_LOCK_TOGGLE_RESPONSE_ID, ZRC_HA_DOOR_LOCK_TOGGLE_RESPONSE_SIZE},
        {ZRC_HA_DOOR_LOCK_UNLOCK_WITH_TIMEOUT_RESPONSE_ID, ZRC_HA_DOOR_LOCK_UNLOCK_WITH_TIMEOUT_RESPONSE_SIZE},
        {ZRC_HA_WINDOW_COVERING_CURRENT_POSITION_LIFT_PERCENTAGE_ID, ZRC_HA_WINDOW_COVERING_CURRENT_POSITION_LIFT_PERCENTAGE_SIZE},
        {ZRC_HA_WINDOW_COVERING_CURRENT_POSITION_TILT_PERCENTAGE_ID, ZRC_HA_WINDOW_COVERING_CURRENT_POSITION_TILT_PERCENTAGE_SIZE},
        {ZRC_HA_THERMOSTAT_LOCAL_TEMPERATURE_ID, ZRC_HA_THERMOSTAT_LOCAL_TEMPERATURE_SIZE},
        {ZRC_HA_THERMOSTAT_OCCUPIED_COOLING_SETPOINT_ID, ZRC_HA_THERMOSTAT_OCCUPIED_COOLING_SETPOINT_SIZE},
        {ZRC_HA_THERMOSTAT_OCCUPIED_HEATING_SETPOINT_ID, ZRC_HA_THERMOSTAT_OCCUPIED_HEATING_SETPOINT_SIZE},
        {ZRC_HA_COLOR_CONTROL_CURRENT_HUE_ID, ZRC_HA_COLOR_CONTROL_CURRENT_HUE_SIZE},
        {ZRC_HA_COLOR_CONTOL_CURRENT_SATURATION_ID, ZRC_HA_COLOR_CONTOL_CURRENT_SATURATION_SIZE},
        {ZRC_HA_COLOR_CONTOL_CURRENT_X_ID, ZRC_HA_COLOR_CONTOL_CURRENT_X_SIZE},
        {ZRC_HA_COLOR_CONTOL_CURRENT_Y_ID, ZRC_HA_COLOR_CONTOL_CURRENT_Y_SIZE},
        {ZRC_HA_COLOR_CONTOL_COLOR_MODE_ID, ZRC_HA_COLOR_CONTOL_COLOR_MODE_SIZE},
        {ZRC_HA_IAS_ACE_ARM_RESPONSE_ID, ZRC_HA_IAS_ACE_ARM_RESPONSE_SIZE}};

static uint8_t aplHaAttributesStatusTable[EMBER_RF4CE_PAIRING_TABLE_SIZE][ZRC_HA_SERVER_NUM_OF_HA_INSTANCES][ZRC_HA_ATTRIBUTE_STATUS_TABLE_SIZE/8+1];
static uint8_t aplHaAttributesValueTable[EMBER_RF4CE_PAIRING_TABLE_SIZE][ZRC_HA_SERVER_NUM_OF_HA_INSTANCES][ZRC_HA_ATTRIBUTE_TABLE_SIZE];


static EmberStatus ConvertHaActionToZclCommand(EmberAfRf4ceZrcActionCode actionCode,
                                               const uint8_t* actionPayload,
                                               uint8_t actionPayloadLength);


void emberAfPluginRf4ceZrc20HaServerClearAllHaAttributes(void)
{
  MEMSET(aplHaAttributesStatusTable,
         0x00,
         sizeof(aplHaAttributesStatusTable));

  MEMSET(aplHaAttributesValueTable,
         0x00,
         sizeof(aplHaAttributesValueTable));
}

EmberAfRf4ceGdpAttributeStatus emberAfPluginRf4ceZrc20HaServerGetHaAttribute(uint8_t pairingIndex,
                                                                             uint8_t haInstanceId,
                                                                             uint8_t haAttributeId,
                                                                             EmberAfRf4ceZrcHomeAutomationAttribute *haAttribute)
{
  uint8_t i, index = 0;

  // Ensure pairing index is valid.
  if (pairingIndex >= EMBER_RF4CE_PAIRING_TABLE_SIZE) {
    return EMBER_AF_RF4CE_GDP_ATTRIBUTE_STATUS_ILLEGAL_REQUEST;
  }

  // Ensure Instance ID is valid.
  if (haInstanceId >= ZRC_HA_SERVER_NUM_OF_HA_INSTANCES) {
      return EMBER_AF_RF4CE_GDP_ATTRIBUTE_STATUS_UNSUPPORTED_ATTRIBUTE;
  }

  haAttribute->instanceId = haInstanceId;
  haAttribute->attributeId = haAttributeId;

  // Look for attribute and calculate the index of its corresponding value.
  for (i=0; i<COUNTOF(aplHaAttributesInfoTable); i++) {
    if (haAttributeId == aplHaAttributesInfoTable[i].id) {
      // Get pointer to the value of the attribute.
      haAttribute->contents
        = &aplHaAttributesValueTable[pairingIndex][haInstanceId][index];
      haAttribute->contentsLength = aplHaAttributesInfoTable[i].length;
      // Attribute is being pulled, we clear the 'changed' flag.
//      aplHaAttributesValueTable[pairingIndex][haInstanceId][index+HA_ATTRIBUTE_STATUS_OFFSET]
//        &= ~HA_ATTRIBUTE_STATUS_CHANGED_FLAG;
      CLEARBIT(aplHaAttributesStatusTable[pairingIndex][haInstanceId][i/8],
               i & 0x07);

      return EMBER_AF_RF4CE_GDP_ATTRIBUTE_STATUS_SUCCESS;
    }

    index += aplHaAttributesInfoTable[i].length;
  }

  // Didn't find attribute ID in table.
  return EMBER_AF_RF4CE_GDP_ATTRIBUTE_STATUS_ILLEGAL_REQUEST;
}

EmberAfRf4ceGdpAttributeStatus emberAfPluginRf4ceZrc20HaServerSetHaAttribute(uint8_t pairingIndex,
                                                                             uint8_t haInstanceId,
                                                                             uint8_t haAttributeId,
                                                                             EmberAfRf4ceZrcHomeAutomationAttribute *haAttribute)
{
  uint8_t i, index = 0;

  // Ensure pairing index is valid.
  if (pairingIndex >= EMBER_RF4CE_PAIRING_TABLE_SIZE) {
    return EMBER_AF_RF4CE_GDP_ATTRIBUTE_STATUS_ILLEGAL_REQUEST;
  }

  // Ensure Instance ID is valid.
  if (haInstanceId >= ZRC_HA_SERVER_NUM_OF_HA_INSTANCES) {
      return EMBER_AF_RF4CE_GDP_ATTRIBUTE_STATUS_UNSUPPORTED_ATTRIBUTE;
  }

  // Look for attribute and calculate the index of its corresponding value.
  for (i=0; i<COUNTOF(aplHaAttributesInfoTable); i++) {
    if (haAttributeId == aplHaAttributesInfoTable[i].id) {
      // Compare new value with current and update only if changed.
      if (MEMCOMPARE(haAttribute->contents,
                     &aplHaAttributesValueTable[pairingIndex][haInstanceId][index],
                     aplHaAttributesInfoTable[i].length)) {
        // Set value of the attribute.
        MEMCOPY(&aplHaAttributesValueTable[pairingIndex][haInstanceId][index],
                haAttribute->contents,
                aplHaAttributesInfoTable[i].length);
        // Attribute is being modified, we set the 'changed' flag.
//        aplHaAttributesValueTable[pairingIndex][haInstanceId][index + HA_ATTRIBUTE_STATUS_OFFSET]
//          |= HA_ATTRIBUTE_STATUS_CHANGED_FLAG;
        SETBIT(aplHaAttributesStatusTable[pairingIndex][haInstanceId][i/8],
               i & 0x07);
      }

        return EMBER_AF_RF4CE_GDP_ATTRIBUTE_STATUS_SUCCESS;
    }

    index += aplHaAttributesInfoTable[i].length;
  }

  // Didn't find attribute ID in table.
  return EMBER_AF_RF4CE_GDP_ATTRIBUTE_STATUS_ILLEGAL_REQUEST;
}

EmberStatus emberAfPluginRf4ceZrc20HaLogicalDeviceDestinationAdd(DestStruct* dest,
                                                                 uint8_t* index)
{
  EmberStatus status;

  // Entry not found in table, add it.
  if (EMBER_SUCCESS !=
          (status = emberAfPluginRf4ceZrc20HaLogicalDeviceIndexLookUp(dest, index))) {
    // Add it if there is free space.
    status = emAfRf4ceZrc20AddLogicalDeviceDestination(dest, index);
  }

  return status;
}

EmberStatus emberAfPluginRf4ceZrc20HaLogicalDeviceDestinationRemove(DestStruct* dest,
                                                                    uint8_t* index)
{
  EmberStatus status;

  // Entry found in table.
  if (EMBER_SUCCESS ==
          (status = emberAfPluginRf4ceZrc20HaLogicalDeviceIndexLookUp(dest,
                                                                      index))) {
    // Remove it.
    status = emAfRf4ceZrc20RemoveLogicalDeviceDestination(*index);
  }

  return status;
}

uint8_t emberAfPluginRf4ceZrc20HaLogicalDeviceDestinationTableSize(void)
{
    return EMBER_AF_PLUGIN_RF4CE_ZRC20_HA_SERVER_LOGICAL_DEVICES_TABLE_SIZE;
}

EmberStatus emberAfPluginRf4ceZrc20HaLogicalDeviceDestinationGet(uint8_t pairingIndex,
                                                                 uint8_t haInstanceId,
                                                                 DestStruct* dest)
{
    /* Pairing index out of range. */
    if (pairingIndex >= EMBER_RF4CE_PAIRING_TABLE_SIZE)
    {
        return EMBER_INDEX_OUT_OF_RANGE;
    }

    /* Entry index out of range. */
    if (haInstanceId >= ZRC_HA_SERVER_NUM_OF_HA_INSTANCES)
    {
        return EMBER_INDEX_OUT_OF_RANGE;
    }

    DestLookup(pairingIndex,
               haInstanceId,
               dest);

    return EMBER_SUCCESS;
}


void emberAfPluginRf4ceZrc20HaLogicalDeviceAndInstanceToLogicalDeviceMappingClear(void)
{
  emAfRf4ceZrc20ClearLogicalDevicesTable();
  emAfRf4ceZrc20ClearInstanceToLogicalDeviceTable();
}

/**
 * Add logical device and map it to instance.
 */
EmberStatus emberAfPluginRf4ceZrc20HaLogicalDeviceAndInstanceToLogicalDeviceMappingAdd(uint8_t pairingIndex,
                                                                                       uint8_t haInstanceId,
                                                                                       DestStruct* dest)
{
  uint8_t i = 0;
  EmberStatus status;

  /* Pairing index out of range. */
  if (pairingIndex >= EMBER_RF4CE_PAIRING_TABLE_SIZE)
  {
      return EMBER_INDEX_OUT_OF_RANGE;
  }

  /* Entry index out of range. */
  if (haInstanceId >= ZRC_HA_SERVER_NUM_OF_HA_INSTANCES)
  {
      return EMBER_INDEX_OUT_OF_RANGE;
  }

  // Table full. Can't add logical device.
  if (EMBER_SUCCESS !=
          (status = emberAfPluginRf4ceZrc20HaLogicalDeviceDestinationAdd(dest,
                                                                         &i))) {
    return status;
  }

  // Map logical device to instance.
  return emberAfPluginRf4ceZrc20HaMappingToLogicalDeviceAdd(pairingIndex,
                                                            haInstanceId,
                                                            i);
}

/**
 * Remove logical device and all instances mapped to it.
 */
EmberStatus emberAfPluginRf4ceZrc20HaLogicalDeviceAndInstanceToLogicalDeviceMappingRemove(DestStruct* dest)
{
  uint8_t i = 0;
  EmberStatus status;

  // Didn't find logical device.
  if (EMBER_SUCCESS !=
          (status = emberAfPluginRf4ceZrc20HaLogicalDeviceDestinationRemove(dest,
                                                                            &i))) {
    return status;
  }

  emberAfPluginRf4ceZrc20HaMappingToLogicalDeviceRemove(i);

  return EMBER_SUCCESS;
}


EmberStatus emAfRf4ceZrc20ParseHaActionAndForwardToZclNetwork(const EmberAfRf4ceZrcActionRecord *record)
{
  DestStruct dest;
  EmberStatus status;

  // Convert HA action to ZCL command.
  if (EMBER_SUCCESS !=
          (status = ConvertHaActionToZclCommand(record->actionCode,
                                                record->actionPayload,
                                                record->actionPayloadLength))) {
    return status;
  }

  // Look up destination corresponding to pairingIndex and haInstanceId.
  if (EMBER_SUCCESS !=
          (status = emberAfPluginRf4ceZrc20HaLogicalDeviceDestinationGet(record->pairingIndex,
                                                                         record->actionBank-EMBER_AF_RF4CE_ZRC_ACTION_BANK_HOME_AUTOMATION_INSTANCE_0,
                                                                         &dest))) {
    return status;
  }

  // Send frame.
  if (dest.type == EMBER_OUTGOING_MULTICAST) {
    // Local scene number on the ZRC side shall be mapped to group ID and
    // scene ID pair on the ZCL side.
    status = emberAfSendCommandMulticastWithCallback((EmberMulticastId)(record->actionCode & ZRC_ACTION_ID_HIGH_NIBBLE_MASK),
                                                     emberAfPluginRf4ceZrc20HaServerHaActionSentCallback);
  }
  else if (dest.type == EMBER_OUTGOING_BROADCAST) {
    status = emberAfSendCommandBroadcastWithCallback(dest.indexOrDestination,
                                                     emberAfPluginRf4ceZrc20HaServerHaActionSentCallback);
  }
  else { // This is done under the hood for binding.
    if (dest.type != EMBER_OUTGOING_VIA_BINDING) {
      // Construct endpoints of the APS frame.
      emberAfSetCommandEndpoints(dest.sourceEndpoint, dest.destinationEndpoint);
    }
    status = emberAfSendCommandUnicastWithCallback(dest.type,
                                                   dest.indexOrDestination,
                                                   emberAfPluginRf4ceZrc20HaServerHaActionSentCallback);
  }

  return status;
}

EmberStatus ConvertHaActionToZclCommand(EmberAfRf4ceZrcActionCode actionCode,
                                        const uint8_t* actionPayload,
                                        uint8_t actionPayloadLength)
{
  EmberAfClusterId clusterId;
  uint8_t commandId;

  // Map ZRC HA command ID to ZCL Cluster ID & Command ID.
  switch (actionCode & ZRC_ACTION_ID_HIGH_NIBBLE_MASK) {
    // TODO: local scene number on the ZRC side shall be mapped to group ID
    // and scene ID pair on the ZCL side.
  case 0x00:
  case 0x10:
      clusterId = ZCL_SCENES_CLUSTER_ID;
      break;

  case 0x20:
      clusterId = ZCL_ON_OFF_CLUSTER_ID;
      break;

  case 0x30:
      clusterId = ZCL_LEVEL_CONTROL_CLUSTER_ID;
      break;

  case 0x40:
      clusterId = ZCL_DOOR_LOCK_CLUSTER_ID;
      break;

  case 0x50:
      clusterId = ZCL_WINDOW_COVERING_CLUSTER_ID;
      break;

  case 0x60:
      clusterId = ZCL_THERMOSTAT_CLUSTER_ID;
      break;

  case 0x70:
      clusterId = ZCL_COLOR_CONTROL_CLUSTER_ID;
      break;

  case 0xC0:
      clusterId = ZCL_IAS_ACE_CLUSTER_ID;
      break;

  // TODO: implement generic actions.
  // With generic actions the HA Actions Originator can use a single HA
  // instance to control different devices on the HA network.
  // In this case, the HA Actions Recipient may maintain a list of
  // devices/groups that can be controlled by the HA Actions Originator.
  // How this list is populated by the HA Actions Recipient can be
  // implementation specific.
//  case 0xF0:
//  clusterId = 0;
//     break;

  default:
      return EMBER_BAD_ARGUMENT;
      break;
  }

  commandId = actionCode & ZRC_ACTION_ID_LOW_NIBBLE_MASK;

  emberAfFillExternalBuffer((ZCL_CLUSTER_SPECIFIC_COMMAND
                          | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER),
                            clusterId,
                            commandId,
                            "b",
                            actionPayload,
                            actionPayloadLength);

  return EMBER_SUCCESS;
}


// For each instance ID we look if there exist at least an attribute that
// changed since the client pulled it. If so, we send a notification 'HA pull'
// command.
static void heartbeatCallback(uint8_t pairingIndex,
                              EmberAfRf4ceGdpHeartbeatTrigger trigger)
{
  uint8_t instanceId;

  for(instanceId=0; instanceId<ZRC_HA_SERVER_NUM_OF_HA_INSTANCES; instanceId++) {
    bool attributeChanged = false;
    uint8_t payload[CLIENT_NOTIFICATION_REQUEST_HA_PULL_PAYLOAD_LENGTH];
    uint8_t index = 0;
    uint8_t i;

    MEMSET(payload, 0x00, CLIENT_NOTIFICATION_REQUEST_HA_PULL_PAYLOAD_LENGTH);

    payload[CLIENT_NOTIFICATION_REQUEST_HA_PULL_HA_INSTANCE_ID_OFFSET] = instanceId;

    for (i=0; i<COUNTOF(aplHaAttributesInfoTable); i++) {
//      if (aplHaAttributesValueTable[pairingIndex][instanceId][index + HA_ATTRIBUTE_STATUS_OFFSET]
//          & HA_ATTRIBUTE_STATUS_CHANGED_FLAG) {
      if (READBIT(aplHaAttributesStatusTable[pairingIndex][instanceId][i/8],
                  i & 0x07)) {
        uint8_t byte = (aplHaAttributesInfoTable[i].id / 8)
            + CLIENT_NOTIFICATION_REQUEST_HA_PULL_HA_ATTRIBUTE_DIRTY_FLAGS_OFFSET;
        uint8_t bit = (aplHaAttributesInfoTable[i].id & 0x07);
        SETBIT(payload[byte], bit);
        attributeChanged = true;
      }

      index += aplHaAttributesInfoTable[i].length;
    }

    if (attributeChanged) {
      emberAfRf4ceGdpClientNotification(pairingIndex,
                                        EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0,
                                        EMBER_RF4CE_NULL_VENDOR_ID,
                                        CLIENT_NOTIFICATION_SUBTYPE_REQUEST_HA_PULL,
                                        payload,
                                        CLIENT_NOTIFICATION_REQUEST_HA_PULL_PAYLOAD_LENGTH);
    }
  }
}

void emberAfPluginRf4ceZrc20HaServerInitCallback(void)
{
  assert(emberAfRf4ceGdpSubscribeToHeartbeat(heartbeatCallback)
         == EMBER_SUCCESS);
}

EmberAfRf4ceGdpAttributeStatus emberAfPluginRf4ceZrc20GetHomeAutomationAttributeCallback(uint8_t pairingIndex,
                                                                                         uint8_t haInstanceId,
                                                                                         uint8_t haAttributeId,
                                                                                         EmberAfRf4ceZrcHomeAutomationAttribute *haAttribute)
{
    return emberAfPluginRf4ceZrc20HaServerGetHaAttribute(pairingIndex,
                                                         haInstanceId,
                                                         haAttributeId,
                                                         haAttribute);
}

void emberAfPluginRf4ceZrc20HaActionCallback(const EmberAfRf4ceZrcActionRecord *record)
{
  if (record->actionType == EMBER_AF_RF4CE_ZRC_ACTION_TYPE_START
      || record->actionType == EMBER_AF_RF4CE_ZRC_ACTION_TYPE_REPEAT
      || record->actionType == EMBER_AF_RF4CE_ZRC_ACTION_TYPE_ATOMIC) {

    /* TODO: should we use the return value? */

    // Create ZCL message from HA action and send it to logical device.
    emAfRf4ceZrc20ParseHaActionAndForwardToZclNetwork(record);
  }
}


