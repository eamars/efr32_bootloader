// File: command-functions.h
// 
// *** Generated file. Do not edit! ***
// 
// Description: Functions for sending every EM260 frame and returning the result
// to the Host.
// 
// Copyright 2007 by Ember Corporation. All rights reserved.                *80*


//------------------------------------------------------------------------------
// Configuration Frames
//------------------------------------------------------------------------------

uint8_t ezspVersion(
      uint8_t desiredProtocolVersion,
      uint8_t *stackType,
      uint16_t *stackVersion)
{
  uint8_t protocolVersion;
  startCommand(EZSP_VERSION);
  appendInt8u(desiredProtocolVersion);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    protocolVersion = fetchInt8u();
    *stackType = fetchInt8u();
    *stackVersion = fetchInt16u();
    return protocolVersion;
  }
  return 255;
}

EzspStatus ezspGetConfigurationValue(
      EzspConfigId configId,
      uint16_t *value)
{
  uint8_t status;
  startCommand(EZSP_GET_CONFIGURATION_VALUE);
  appendInt8u(configId);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    *value = fetchInt16u();
    return status;
  }
  return sendStatus;
}

EzspStatus ezspSetConfigurationValue(
      EzspConfigId configId,
      uint16_t value)
{
  uint8_t status;
  startCommand(EZSP_SET_CONFIGURATION_VALUE);
  appendInt8u(configId);
  appendInt16u(value);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EzspStatus ezspAddEndpoint(
      uint8_t endpoint,
      uint16_t profileId,
      uint16_t deviceId,
      uint8_t deviceVersion,
      uint8_t inputClusterCount,
      uint8_t outputClusterCount,
      uint16_t *inputClusterList,
      uint16_t *outputClusterList)
{
  uint8_t status;
  startCommand(EZSP_ADD_ENDPOINT);
  appendInt8u(endpoint);
  appendInt16u(profileId);
  appendInt16u(deviceId);
  appendInt8u(deviceVersion);
  appendInt8u(inputClusterCount);
  appendInt8u(outputClusterCount);
  appendInt16uArray(inputClusterCount, inputClusterList);
  appendInt16uArray(outputClusterCount, outputClusterList);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EzspStatus ezspSetPolicy(
      EzspPolicyId policyId,
      EzspDecisionId decisionId)
{
  uint8_t status;
  startCommand(EZSP_SET_POLICY);
  appendInt8u(policyId);
  appendInt8u(decisionId);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EzspStatus ezspGetPolicy(
      EzspPolicyId policyId,
      EzspDecisionId *decisionId)
{
  uint8_t status;
  startCommand(EZSP_GET_POLICY);
  appendInt8u(policyId);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    *decisionId = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EzspStatus ezspGetValue(
      EzspValueId valueId,
      uint8_t *valueLength,
      uint8_t *value)
{
  uint8_t status;
  startCommand(EZSP_GET_VALUE);
  appendInt8u(valueId);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    *valueLength = fetchInt8u();
    fetchInt8uArray(*valueLength, value);
    return status;
  }
  return sendStatus;
}

EzspStatus ezspGetExtendedValue(
      EzspExtendedValueId valueId,
      uint32_t characteristics,
      uint8_t *valueLength,
      uint8_t *value)
{
  uint8_t status;
  startCommand(EZSP_GET_EXTENDED_VALUE);
  appendInt8u(valueId);
  appendInt32u(characteristics);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    *valueLength = fetchInt8u();
    fetchInt8uArray(*valueLength, value);
    return status;
  }
  return sendStatus;
}

EzspStatus ezspSetValue(
      EzspValueId valueId,
      uint8_t valueLength,
      uint8_t *value)
{
  uint8_t status;
  startCommand(EZSP_SET_VALUE);
  appendInt8u(valueId);
  appendInt8u(valueLength);
  appendInt8uArray(valueLength, value);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EzspStatus ezspSetGpioCurrentConfiguration(
      uint8_t portPin,
      uint8_t cfg,
      uint8_t out)
{
  uint8_t status;
  startCommand(EZSP_SET_GPIO_CURRENT_CONFIGURATION);
  appendInt8u(portPin);
  appendInt8u(cfg);
  appendInt8u(out);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EzspStatus ezspSetGpioPowerUpDownConfiguration(
      uint8_t portPin,
      uint8_t puCfg,
      uint8_t puOut,
      uint8_t pdCfg,
      uint8_t pdOut)
{
  uint8_t status;
  startCommand(EZSP_SET_GPIO_POWER_UP_DOWN_CONFIGURATION);
  appendInt8u(portPin);
  appendInt8u(puCfg);
  appendInt8u(puOut);
  appendInt8u(pdCfg);
  appendInt8u(pdOut);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

void ezspSetGpioRadioPowerMask(
      uint32_t mask)
{
  startCommand(EZSP_SET_GPIO_RADIO_POWER_MASK);
  appendInt32u(mask);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    EZSP_ASH_TRACE("%s(): sendCommand() error: 0x%x", __func__, sendStatus);
  }
}

void ezspSetCtune(
      uint16_t ctune)
{
  startCommand(EZSP_SET_CTUNE);
  appendInt16u(ctune);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    EZSP_ASH_TRACE("%s(): sendCommand() error: 0x%x", __func__, sendStatus);
  }
}

uint16_t ezspGetCtune(void)
{
  uint16_t ctune;
  startCommand(EZSP_GET_CTUNE);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    ctune = fetchInt16u();
    return ctune;
  }
  return 255;
}

void ezspSetChannelMap(
      uint8_t page,
      uint8_t channel)
{
  startCommand(EZSP_SET_CHANNEL_MAP);
  appendInt8u(page);
  appendInt8u(channel);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    EZSP_ASH_TRACE("%s(): sendCommand() error: 0x%x", __func__, sendStatus);
  }
}

//------------------------------------------------------------------------------
// Utilities Frames
//------------------------------------------------------------------------------

void ezspNop(void)
{
  startCommand(EZSP_NOP);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    EZSP_ASH_TRACE("%s(): sendCommand() error: 0x%x", __func__, sendStatus);
  }
}

uint8_t ezspEcho(
      uint8_t dataLength,
      uint8_t *data,
      uint8_t *echo)
{
  uint8_t echoLength;
  startCommand(EZSP_ECHO);
  appendInt8u(dataLength);
  appendInt8uArray(dataLength, data);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    echoLength = fetchInt8u();
    fetchInt8uArray(echoLength, echo);
    return echoLength;
  }
  return 0;
}

void ezspCallback(void)
{
  startCommand(EZSP_CALLBACK);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    callbackDispatch();
  }
}

EmberStatus ezspSetToken(
      uint8_t tokenId,
      uint8_t *tokenData)
{
  uint8_t status;
  startCommand(EZSP_SET_TOKEN);
  appendInt8u(tokenId);
  appendInt8uArray(8, tokenData);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspGetToken(
      uint8_t tokenId,
      uint8_t *tokenData)
{
  uint8_t status;
  startCommand(EZSP_GET_TOKEN);
  appendInt8u(tokenId);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    fetchInt8uArray(8, tokenData);
    return status;
  }
  return sendStatus;
}

uint8_t ezspGetMfgToken(
      EzspMfgTokenId tokenId,
      uint8_t *tokenData)
{
  uint8_t tokenDataLength;
  startCommand(EZSP_GET_MFG_TOKEN);
  appendInt8u(tokenId);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    tokenDataLength = fetchInt8u();
    fetchInt8uArray(tokenDataLength, tokenData);
    return tokenDataLength;
  }
  return 255;
}

EmberStatus ezspSetMfgToken(
      EzspMfgTokenId tokenId,
      uint8_t tokenDataLength,
      uint8_t *tokenData)
{
  uint8_t status;
  startCommand(EZSP_SET_MFG_TOKEN);
  appendInt8u(tokenId);
  appendInt8u(tokenDataLength);
  appendInt8uArray(tokenDataLength, tokenData);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspGetRandomNumber(
      uint16_t *value)
{
  uint8_t status;
  startCommand(EZSP_GET_RANDOM_NUMBER);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    *value = fetchInt16u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspSetTimer(
      uint8_t timerId,
      uint16_t time,
      EmberEventUnits units,
      bool repeat)
{
  uint8_t status;
  startCommand(EZSP_SET_TIMER);
  appendInt8u(timerId);
  appendInt16u(time);
  appendInt8u(units);
  appendInt8u(repeat);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

uint16_t ezspGetTimer(
      uint8_t timerId,
      EmberEventUnits *units,
      bool *repeat)
{
  uint16_t time;
  startCommand(EZSP_GET_TIMER);
  appendInt8u(timerId);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    time = fetchInt16u();
    *units = fetchInt8u();
    *repeat = fetchInt8u();
    return time;
  }
  return 255;
}

EmberStatus ezspDebugWrite(
      bool binaryMessage,
      uint8_t messageLength,
      uint8_t *messageContents)
{
  uint8_t status;
  startCommand(EZSP_DEBUG_WRITE);
  appendInt8u(binaryMessage);
  appendInt8u(messageLength);
  appendInt8uArray(messageLength, messageContents);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

void ezspReadAndClearCounters(
      uint16_t *values)
{
  startCommand(EZSP_READ_AND_CLEAR_COUNTERS);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    fetchInt16uArray(EMBER_COUNTER_TYPE_COUNT, values);
  }
}

void ezspReadCounters(
      uint16_t *values)
{
  startCommand(EZSP_READ_COUNTERS);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    fetchInt16uArray(EMBER_COUNTER_TYPE_COUNT, values);
  }
}

void ezspDelayTest(
      uint16_t delay)
{
  startCommand(EZSP_DELAY_TEST);
  appendInt16u(delay);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    EZSP_ASH_TRACE("%s(): sendCommand() error: 0x%x", __func__, sendStatus);
  }
}

EmberLibraryStatus ezspGetLibraryStatus(
      uint8_t libraryId)
{
  uint8_t status;
  startCommand(EZSP_GET_LIBRARY_STATUS);
  appendInt8u(libraryId);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspGetXncpInfo(
      uint16_t *manufacturerId,
      uint16_t *versionNumber)
{
  uint8_t status;
  startCommand(EZSP_GET_XNCP_INFO);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    *manufacturerId = fetchInt16u();
    *versionNumber = fetchInt16u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspCustomFrame(
      uint8_t payloadLength,
      uint8_t *payload,
      uint8_t *replyLength,
      uint8_t *reply)
{
  uint8_t status;
  startCommand(EZSP_CUSTOM_FRAME);
  appendInt8u(payloadLength);
  appendInt8uArray(payloadLength, payload);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    *replyLength = fetchInt8u();
    fetchInt8uArray(*replyLength, reply);
    return status;
  }
  return sendStatus;
}

void ezspGetEui64(
      EmberEUI64 eui64)
{
  startCommand(EZSP_GET_EUI64);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    fetchInt8uArray(8, eui64);
  }
}

EmberNodeId ezspGetNodeId(void)
{
  uint16_t nodeId;
  startCommand(EZSP_GET_NODE_ID);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    nodeId = fetchInt16u();
    return nodeId;
  }
  return 0xFFFE;
}

EmberStatus ezspNetworkInit(void)
{
  uint8_t status;
  startCommand(EZSP_NETWORK_INIT);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

//------------------------------------------------------------------------------
// Networking Frames
//------------------------------------------------------------------------------

void ezspSetManufacturerCode(
      uint16_t code)
{
  startCommand(EZSP_SET_MANUFACTURER_CODE);
  appendInt16u(code);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    EZSP_ASH_TRACE("%s(): sendCommand() error: 0x%x", __func__, sendStatus);
  }
}

void ezspSetPowerDescriptor(
      uint16_t descriptor)
{
  startCommand(EZSP_SET_POWER_DESCRIPTOR);
  appendInt16u(descriptor);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    EZSP_ASH_TRACE("%s(): sendCommand() error: 0x%x", __func__, sendStatus);
  }
}

EmberStatus ezspNetworkInitExtended(
      EmberNetworkInitStruct *networkInitStruct)
{
  uint8_t status;
  startCommand(EZSP_NETWORK_INIT_EXTENDED);
  appendEmberNetworkInitStruct(networkInitStruct);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberNetworkStatus ezspNetworkState(void)
{
  uint8_t status;
  startCommand(EZSP_NETWORK_STATE);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspStartScan(
      EzspNetworkScanType scanType,
      uint32_t channelMask,
      uint8_t duration)
{
  uint8_t status;
  startCommand(EZSP_START_SCAN);
  appendInt8u(scanType);
  appendInt32u(channelMask);
  appendInt8u(duration);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspStopScan(void)
{
  uint8_t status;
  startCommand(EZSP_STOP_SCAN);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspFormNetwork(
      EmberNetworkParameters *parameters)
{
  uint8_t status;
  startCommand(EZSP_FORM_NETWORK);
  appendEmberNetworkParameters(parameters);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspJoinNetwork(
      EmberNodeType nodeType,
      EmberNetworkParameters *parameters)
{
  uint8_t status;
  startCommand(EZSP_JOIN_NETWORK);
  appendInt8u(nodeType);
  appendEmberNetworkParameters(parameters);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspLeaveNetwork(void)
{
  uint8_t status;
  startCommand(EZSP_LEAVE_NETWORK);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspFindAndRejoinNetwork(
      bool haveCurrentNetworkKey,
      uint32_t channelMask)
{
  uint8_t status;
  startCommand(EZSP_FIND_AND_REJOIN_NETWORK);
  appendInt8u(haveCurrentNetworkKey);
  appendInt32u(channelMask);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspPermitJoining(
      uint8_t duration)
{
  uint8_t status;
  startCommand(EZSP_PERMIT_JOINING);
  appendInt8u(duration);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspEnergyScanRequest(
      EmberNodeId target,
      uint32_t scanChannels,
      uint8_t scanDuration,
      uint16_t scanCount)
{
  uint8_t status;
  startCommand(EZSP_ENERGY_SCAN_REQUEST);
  appendInt16u(target);
  appendInt32u(scanChannels);
  appendInt8u(scanDuration);
  appendInt16u(scanCount);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspGetNetworkParameters(
      EmberNodeType *nodeType,
      EmberNetworkParameters *parameters)
{
  uint8_t status;
  startCommand(EZSP_GET_NETWORK_PARAMETERS);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    *nodeType = fetchInt8u();
    fetchEmberNetworkParameters(parameters);
    return status;
  }
  return sendStatus;
}

uint8_t ezspGetParentChildParameters(
      EmberEUI64 parentEui64,
      EmberNodeId *parentNodeId)
{
  uint8_t childCount;
  startCommand(EZSP_GET_PARENT_CHILD_PARAMETERS);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    childCount = fetchInt8u();
    fetchInt8uArray(8, parentEui64);
    *parentNodeId = fetchInt16u();
    return childCount;
  }
  return 255;
}

EmberStatus ezspGetChildData(
      uint8_t index,
      EmberNodeId *childId,
      EmberEUI64 childEui64,
      EmberNodeType *childType)
{
  uint8_t status;
  startCommand(EZSP_GET_CHILD_DATA);
  appendInt8u(index);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    *childId = fetchInt16u();
    fetchInt8uArray(8, childEui64);
    *childType = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspGetNeighbor(
      uint8_t index,
      EmberNeighborTableEntry *value)
{
  uint8_t status;
  startCommand(EZSP_GET_NEIGHBOR);
  appendInt8u(index);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    fetchEmberNeighborTableEntry(value);
    return status;
  }
  return sendStatus;
}

uint8_t ezspNeighborCount(void)
{
  uint8_t value;
  startCommand(EZSP_NEIGHBOR_COUNT);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    value = fetchInt8u();
    return value;
  }
  return 255;
}

EmberStatus ezspGetRouteTableEntry(
      uint8_t index,
      EmberRouteTableEntry *value)
{
  uint8_t status;
  startCommand(EZSP_GET_ROUTE_TABLE_ENTRY);
  appendInt8u(index);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    fetchEmberRouteTableEntry(value);
    return status;
  }
  return sendStatus;
}

EmberStatus ezspSetRadioPower(
      int8_t power)
{
  uint8_t status;
  startCommand(EZSP_SET_RADIO_POWER);
  appendInt8u(power);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspSetRadioChannel(
      uint8_t channel)
{
  uint8_t status;
  startCommand(EZSP_SET_RADIO_CHANNEL);
  appendInt8u(channel);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspSetConcentrator(
      bool on,
      uint16_t concentratorType,
      uint16_t minTime,
      uint16_t maxTime,
      uint8_t routeErrorThreshold,
      uint8_t deliveryFailureThreshold,
      uint8_t maxHops)
{
  uint8_t status;
  startCommand(EZSP_SET_CONCENTRATOR);
  appendInt8u(on);
  appendInt16u(concentratorType);
  appendInt16u(minTime);
  appendInt16u(maxTime);
  appendInt8u(routeErrorThreshold);
  appendInt8u(deliveryFailureThreshold);
  appendInt8u(maxHops);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

//------------------------------------------------------------------------------
// Binding Frames
//------------------------------------------------------------------------------

EmberStatus ezspClearBindingTable(void)
{
  uint8_t status;
  startCommand(EZSP_CLEAR_BINDING_TABLE);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspSetBinding(
      uint8_t index,
      EmberBindingTableEntry *value)
{
  uint8_t status;
  startCommand(EZSP_SET_BINDING);
  appendInt8u(index);
  appendEmberBindingTableEntry(value);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspGetBinding(
      uint8_t index,
      EmberBindingTableEntry *value)
{
  uint8_t status;
  startCommand(EZSP_GET_BINDING);
  appendInt8u(index);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    fetchEmberBindingTableEntry(value);
    return status;
  }
  return sendStatus;
}

EmberStatus ezspDeleteBinding(
      uint8_t index)
{
  uint8_t status;
  startCommand(EZSP_DELETE_BINDING);
  appendInt8u(index);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

bool ezspBindingIsActive(
      uint8_t index)
{
  uint8_t active;
  startCommand(EZSP_BINDING_IS_ACTIVE);
  appendInt8u(index);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    active = fetchInt8u();
    return active;
  }
  return false;
}

EmberNodeId ezspGetBindingRemoteNodeId(
      uint8_t index)
{
  uint16_t nodeId;
  startCommand(EZSP_GET_BINDING_REMOTE_NODE_ID);
  appendInt8u(index);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    nodeId = fetchInt16u();
    return nodeId;
  }
  return 0xFFFE;
}

void ezspSetBindingRemoteNodeId(
      uint8_t index,
      EmberNodeId nodeId)
{
  startCommand(EZSP_SET_BINDING_REMOTE_NODE_ID);
  appendInt8u(index);
  appendInt16u(nodeId);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    EZSP_ASH_TRACE("%s(): sendCommand() error: 0x%x", __func__, sendStatus);
  }
}

//------------------------------------------------------------------------------
// Messaging Frames
//------------------------------------------------------------------------------

uint8_t ezspMaximumPayloadLength(void)
{
  uint8_t apsLength;
  startCommand(EZSP_MAXIMUM_PAYLOAD_LENGTH);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    apsLength = fetchInt8u();
    return apsLength;
  }
  return 255;
}

EmberStatus ezspSendUnicast(
      EmberOutgoingMessageType type,
      EmberNodeId indexOrDestination,
      EmberApsFrame *apsFrame,
      uint8_t messageTag,
      uint8_t messageLength,
      uint8_t *messageContents,
      uint8_t *sequence)
{
  uint8_t status;
  startCommand(EZSP_SEND_UNICAST);
  appendInt8u(type);
  appendInt16u(indexOrDestination);
  appendEmberApsFrame(apsFrame);
  appendInt8u(messageTag);
  appendInt8u(messageLength);
  appendInt8uArray(messageLength, messageContents);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    *sequence = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspSendBroadcast(
      EmberNodeId destination,
      EmberApsFrame *apsFrame,
      uint8_t radius,
      uint8_t messageTag,
      uint8_t messageLength,
      uint8_t *messageContents,
      uint8_t *sequence)
{
  uint8_t status;
  startCommand(EZSP_SEND_BROADCAST);
  appendInt16u(destination);
  appendEmberApsFrame(apsFrame);
  appendInt8u(radius);
  appendInt8u(messageTag);
  appendInt8u(messageLength);
  appendInt8uArray(messageLength, messageContents);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    *sequence = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspProxyBroadcast(
      EmberNodeId source,
      EmberNodeId destination,
      uint8_t nwkSequence,
      EmberApsFrame *apsFrame,
      uint8_t radius,
      uint8_t messageTag,
      uint8_t messageLength,
      uint8_t *messageContents,
      uint8_t *apsSequence)
{
  uint8_t status;
  startCommand(EZSP_PROXY_BROADCAST);
  appendInt16u(source);
  appendInt16u(destination);
  appendInt8u(nwkSequence);
  appendEmberApsFrame(apsFrame);
  appendInt8u(radius);
  appendInt8u(messageTag);
  appendInt8u(messageLength);
  appendInt8uArray(messageLength, messageContents);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    *apsSequence = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspSendMulticast(
      EmberApsFrame *apsFrame,
      uint8_t hops,
      uint8_t nonmemberRadius,
      uint8_t messageTag,
      uint8_t messageLength,
      uint8_t *messageContents,
      uint8_t *sequence)
{
  uint8_t status;
  startCommand(EZSP_SEND_MULTICAST);
  appendEmberApsFrame(apsFrame);
  appendInt8u(hops);
  appendInt8u(nonmemberRadius);
  appendInt8u(messageTag);
  appendInt8u(messageLength);
  appendInt8uArray(messageLength, messageContents);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    *sequence = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspSendReply(
      EmberNodeId sender,
      EmberApsFrame *apsFrame,
      uint8_t messageLength,
      uint8_t *messageContents)
{
  uint8_t status;
  startCommand(EZSP_SEND_REPLY);
  appendInt16u(sender);
  appendEmberApsFrame(apsFrame);
  appendInt8u(messageLength);
  appendInt8uArray(messageLength, messageContents);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspSendManyToOneRouteRequest(
      uint16_t concentratorType,
      uint8_t radius)
{
  uint8_t status;
  startCommand(EZSP_SEND_MANY_TO_ONE_ROUTE_REQUEST);
  appendInt16u(concentratorType);
  appendInt8u(radius);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspPollForData(
      uint16_t interval,
      EmberEventUnits units,
      uint8_t failureLimit)
{
  uint8_t status;
  startCommand(EZSP_POLL_FOR_DATA);
  appendInt16u(interval);
  appendInt8u(units);
  appendInt8u(failureLimit);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspSetSourceRoute(
      EmberNodeId destination,
      uint8_t relayCount,
      uint16_t *relayList)
{
  uint8_t status;
  startCommand(EZSP_SET_SOURCE_ROUTE);
  appendInt16u(destination);
  appendInt8u(relayCount);
  appendInt16uArray(relayCount, relayList);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

bool ezspAddressTableEntryIsActive(
      uint8_t addressTableIndex)
{
  uint8_t active;
  startCommand(EZSP_ADDRESS_TABLE_ENTRY_IS_ACTIVE);
  appendInt8u(addressTableIndex);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    active = fetchInt8u();
    return active;
  }
  return false;
}

EmberStatus ezspSetAddressTableRemoteEui64(
      uint8_t addressTableIndex,
      EmberEUI64 eui64)
{
  uint8_t status;
  startCommand(EZSP_SET_ADDRESS_TABLE_REMOTE_EUI64);
  appendInt8u(addressTableIndex);
  appendInt8uArray(8, eui64);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

void ezspSetAddressTableRemoteNodeId(
      uint8_t addressTableIndex,
      EmberNodeId id)
{
  startCommand(EZSP_SET_ADDRESS_TABLE_REMOTE_NODE_ID);
  appendInt8u(addressTableIndex);
  appendInt16u(id);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    EZSP_ASH_TRACE("%s(): sendCommand() error: 0x%x", __func__, sendStatus);
  }
}

void ezspGetAddressTableRemoteEui64(
      uint8_t addressTableIndex,
      EmberEUI64 eui64)
{
  startCommand(EZSP_GET_ADDRESS_TABLE_REMOTE_EUI64);
  appendInt8u(addressTableIndex);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    fetchInt8uArray(8, eui64);
  }
}

EmberNodeId ezspGetAddressTableRemoteNodeId(
      uint8_t addressTableIndex)
{
  uint16_t nodeId;
  startCommand(EZSP_GET_ADDRESS_TABLE_REMOTE_NODE_ID);
  appendInt8u(addressTableIndex);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    nodeId = fetchInt16u();
    return nodeId;
  }
  return 0xFFFE;
}

void ezspSetExtendedTimeout(
      EmberEUI64 remoteEui64,
      bool extendedTimeout)
{
  startCommand(EZSP_SET_EXTENDED_TIMEOUT);
  appendInt8uArray(8, remoteEui64);
  appendInt8u(extendedTimeout);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    EZSP_ASH_TRACE("%s(): sendCommand() error: 0x%x", __func__, sendStatus);
  }
}

bool ezspGetExtendedTimeout(
      EmberEUI64 remoteEui64)
{
  uint8_t extendedTimeout;
  startCommand(EZSP_GET_EXTENDED_TIMEOUT);
  appendInt8uArray(8, remoteEui64);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    extendedTimeout = fetchInt8u();
    return extendedTimeout;
  }
  return false;
}

EmberStatus ezspReplaceAddressTableEntry(
      uint8_t addressTableIndex,
      EmberEUI64 newEui64,
      EmberNodeId newId,
      bool newExtendedTimeout,
      EmberEUI64 oldEui64,
      EmberNodeId *oldId,
      bool *oldExtendedTimeout)
{
  uint8_t status;
  startCommand(EZSP_REPLACE_ADDRESS_TABLE_ENTRY);
  appendInt8u(addressTableIndex);
  appendInt8uArray(8, newEui64);
  appendInt16u(newId);
  appendInt8u(newExtendedTimeout);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    fetchInt8uArray(8, oldEui64);
    *oldId = fetchInt16u();
    *oldExtendedTimeout = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberNodeId ezspLookupNodeIdByEui64(
      EmberEUI64 eui64)
{
  uint16_t nodeId;
  startCommand(EZSP_LOOKUP_NODE_ID_BY_EUI64);
  appendInt8uArray(8, eui64);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    nodeId = fetchInt16u();
    return nodeId;
  }
  return 0xFFFE;
}

EmberStatus ezspLookupEui64ByNodeId(
      EmberNodeId nodeId,
      EmberEUI64 eui64)
{
  uint8_t status;
  startCommand(EZSP_LOOKUP_EUI64_BY_NODE_ID);
  appendInt16u(nodeId);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    fetchInt8uArray(8, eui64);
    return status;
  }
  return sendStatus;
}

EmberStatus ezspGetMulticastTableEntry(
      uint8_t index,
      EmberMulticastTableEntry *value)
{
  uint8_t status;
  startCommand(EZSP_GET_MULTICAST_TABLE_ENTRY);
  appendInt8u(index);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    fetchEmberMulticastTableEntry(value);
    return status;
  }
  return sendStatus;
}

EmberStatus ezspSetMulticastTableEntry(
      uint8_t index,
      EmberMulticastTableEntry *value)
{
  uint8_t status;
  startCommand(EZSP_SET_MULTICAST_TABLE_ENTRY);
  appendInt8u(index);
  appendEmberMulticastTableEntry(value);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspSendRawMessage(
      uint8_t messageLength,
      uint8_t *messageContents)
{
  uint8_t status;
  startCommand(EZSP_SEND_RAW_MESSAGE);
  appendInt8u(messageLength);
  appendInt8uArray(messageLength, messageContents);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

//------------------------------------------------------------------------------
// Security Frames
//------------------------------------------------------------------------------

EmberStatus ezspSetInitialSecurityState(
      EmberInitialSecurityState *state)
{
  uint8_t success;
  startCommand(EZSP_SET_INITIAL_SECURITY_STATE);
  appendEmberInitialSecurityState(state);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    success = fetchInt8u();
    return success;
  }
  return sendStatus;
}

EmberStatus ezspGetCurrentSecurityState(
      EmberCurrentSecurityState *state)
{
  uint8_t status;
  startCommand(EZSP_GET_CURRENT_SECURITY_STATE);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    fetchEmberCurrentSecurityState(state);
    return status;
  }
  return sendStatus;
}

EmberStatus ezspGetKey(
      EmberKeyType keyType,
      EmberKeyStruct *keyStruct)
{
  uint8_t status;
  startCommand(EZSP_GET_KEY);
  appendInt8u(keyType);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    fetchEmberKeyStruct(keyStruct);
    return status;
  }
  return sendStatus;
}

EmberStatus ezspGetKeyTableEntry(
      uint8_t index,
      EmberKeyStruct *keyStruct)
{
  uint8_t status;
  startCommand(EZSP_GET_KEY_TABLE_ENTRY);
  appendInt8u(index);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    fetchEmberKeyStruct(keyStruct);
    return status;
  }
  return sendStatus;
}

EmberStatus ezspSetKeyTableEntry(
      uint8_t index,
      EmberEUI64 address,
      bool linkKey,
      EmberKeyData *keyData)
{
  uint8_t status;
  startCommand(EZSP_SET_KEY_TABLE_ENTRY);
  appendInt8u(index);
  appendInt8uArray(8, address);
  appendInt8u(linkKey);
  appendEmberKeyData(keyData);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

uint8_t ezspFindKeyTableEntry(
      EmberEUI64 address,
      bool linkKey)
{
  uint8_t index;
  startCommand(EZSP_FIND_KEY_TABLE_ENTRY);
  appendInt8uArray(8, address);
  appendInt8u(linkKey);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    index = fetchInt8u();
    return index;
  }
  return 255;
}

EmberStatus ezspAddOrUpdateKeyTableEntry(
      EmberEUI64 address,
      bool linkKey,
      EmberKeyData *keyData)
{
  uint8_t status;
  startCommand(EZSP_ADD_OR_UPDATE_KEY_TABLE_ENTRY);
  appendInt8uArray(8, address);
  appendInt8u(linkKey);
  appendEmberKeyData(keyData);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspEraseKeyTableEntry(
      uint8_t index)
{
  uint8_t status;
  startCommand(EZSP_ERASE_KEY_TABLE_ENTRY);
  appendInt8u(index);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspClearKeyTable(void)
{
  uint8_t status;
  startCommand(EZSP_CLEAR_KEY_TABLE);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspRequestLinkKey(
      EmberEUI64 partner)
{
  uint8_t status;
  startCommand(EZSP_REQUEST_LINK_KEY);
  appendInt8uArray(8, partner);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspAddTransientLinkKey(
      EmberEUI64 partner,
      EmberKeyData *transientKey)
{
  uint8_t status;
  startCommand(EZSP_ADD_TRANSIENT_LINK_KEY);
  appendInt8uArray(8, partner);
  appendEmberKeyData(transientKey);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

void ezspClearTransientLinkKeys(void)
{
  startCommand(EZSP_CLEAR_TRANSIENT_LINK_KEYS);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    EZSP_ASH_TRACE("%s(): sendCommand() error: 0x%x", __func__, sendStatus);
  }
}

//------------------------------------------------------------------------------
// Trust Center Frames
//------------------------------------------------------------------------------

EmberStatus ezspBroadcastNextNetworkKey(
      EmberKeyData *key)
{
  uint8_t status;
  startCommand(EZSP_BROADCAST_NEXT_NETWORK_KEY);
  appendEmberKeyData(key);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspBroadcastNetworkKeySwitch(void)
{
  uint8_t status;
  startCommand(EZSP_BROADCAST_NETWORK_KEY_SWITCH);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspBecomeTrustCenter(
      EmberKeyData *newNetworkKey)
{
  uint8_t status;
  startCommand(EZSP_BECOME_TRUST_CENTER);
  appendEmberKeyData(newNetworkKey);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspAesMmoHash(
      EmberAesMmoHashContext *context,
      bool finalize,
      uint8_t length,
      uint8_t *data,
      EmberAesMmoHashContext *returnContext)
{
  uint8_t status;
  startCommand(EZSP_AES_MMO_HASH);
  appendEmberAesMmoHashContext(context);
  appendInt8u(finalize);
  appendInt8u(length);
  appendInt8uArray(length, data);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    fetchEmberAesMmoHashContext(returnContext);
    return status;
  }
  return sendStatus;
}

EmberStatus ezspRemoveDevice(
      EmberNodeId destShort,
      EmberEUI64 destLong,
      EmberEUI64 targetLong)
{
  uint8_t status;
  startCommand(EZSP_REMOVE_DEVICE);
  appendInt16u(destShort);
  appendInt8uArray(8, destLong);
  appendInt8uArray(8, targetLong);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspUnicastNwkKeyUpdate(
      EmberNodeId destShort,
      EmberEUI64 destLong,
      EmberKeyData *key)
{
  uint8_t status;
  startCommand(EZSP_UNICAST_NWK_KEY_UPDATE);
  appendInt16u(destShort);
  appendInt8uArray(8, destLong);
  appendEmberKeyData(key);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

//------------------------------------------------------------------------------
// Certificate Based Key Exchange (CBKE)
//------------------------------------------------------------------------------

EmberStatus ezspGenerateCbkeKeys(void)
{
  uint8_t status;
  startCommand(EZSP_GENERATE_CBKE_KEYS);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspCalculateSmacs(
      bool amInitiator,
      EmberCertificateData *partnerCertificate,
      EmberPublicKeyData *partnerEphemeralPublicKey)
{
  uint8_t status;
  startCommand(EZSP_CALCULATE_SMACS);
  appendInt8u(amInitiator);
  appendEmberCertificateData(partnerCertificate);
  appendEmberPublicKeyData(partnerEphemeralPublicKey);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspGenerateCbkeKeys283k1(void)
{
  uint8_t status;
  startCommand(EZSP_GENERATE_CBKE_KEYS283K1);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspCalculateSmacs283k1(
      bool amInitiator,
      EmberCertificate283k1Data *partnerCertificate,
      EmberPublicKey283k1Data *partnerEphemeralPublicKey)
{
  uint8_t status;
  startCommand(EZSP_CALCULATE_SMACS283K1);
  appendInt8u(amInitiator);
  appendEmberCertificate283k1Data(partnerCertificate);
  appendEmberPublicKey283k1Data(partnerEphemeralPublicKey);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspClearTemporaryDataMaybeStoreLinkKey(
      bool storeLinkKey)
{
  uint8_t status;
  startCommand(EZSP_CLEAR_TEMPORARY_DATA_MAYBE_STORE_LINK_KEY);
  appendInt8u(storeLinkKey);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspClearTemporaryDataMaybeStoreLinkKey283k1(
      bool storeLinkKey)
{
  uint8_t status;
  startCommand(EZSP_CLEAR_TEMPORARY_DATA_MAYBE_STORE_LINK_KEY283K1);
  appendInt8u(storeLinkKey);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspGetCertificate(
      EmberCertificateData *localCert)
{
  uint8_t status;
  startCommand(EZSP_GET_CERTIFICATE);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    fetchEmberCertificateData(localCert);
    return status;
  }
  return sendStatus;
}

EmberStatus ezspGetCertificate283k1(
      EmberCertificate283k1Data *localCert)
{
  uint8_t status;
  startCommand(EZSP_GET_CERTIFICATE283K1);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    fetchEmberCertificate283k1Data(localCert);
    return status;
  }
  return sendStatus;
}

EmberStatus ezspDsaSign(
      uint8_t messageLength,
      uint8_t *messageContents)
{
  uint8_t status;
  startCommand(EZSP_DSA_SIGN);
  appendInt8u(messageLength);
  appendInt8uArray(messageLength, messageContents);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspDsaVerify(
      EmberMessageDigest *digest,
      EmberCertificateData *signerCertificate,
      EmberSignatureData *receivedSig)
{
  uint8_t status;
  startCommand(EZSP_DSA_VERIFY);
  appendEmberMessageDigest(digest);
  appendEmberCertificateData(signerCertificate);
  appendEmberSignatureData(receivedSig);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspDsaVerify283k1(
      EmberMessageDigest *digest,
      EmberCertificate283k1Data *signerCertificate,
      EmberSignature283k1Data *receivedSig)
{
  uint8_t status;
  startCommand(EZSP_DSA_VERIFY283K1);
  appendEmberMessageDigest(digest);
  appendEmberCertificate283k1Data(signerCertificate);
  appendEmberSignature283k1Data(receivedSig);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspSetPreinstalledCbkeData(
      EmberPublicKeyData *caPublic,
      EmberCertificateData *myCert,
      EmberPrivateKeyData *myKey)
{
  uint8_t status;
  startCommand(EZSP_SET_PREINSTALLED_CBKE_DATA);
  appendEmberPublicKeyData(caPublic);
  appendEmberCertificateData(myCert);
  appendEmberPrivateKeyData(myKey);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspSavePreinstalledCbkeData283k1(void)
{
  uint8_t status;
  startCommand(EZSP_SAVE_PREINSTALLED_CBKE_DATA283K1);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

//------------------------------------------------------------------------------
// Mfglib
//------------------------------------------------------------------------------

EmberStatus ezspMfglibStart(
      bool rxCallback)
{
  uint8_t status;
  startCommand(EZSP_MFGLIB_START);
  appendInt8u(rxCallback);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus mfglibEnd(void)
{
  uint8_t status;
  startCommand(EZSP_MFGLIB_END);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus mfglibStartTone(void)
{
  uint8_t status;
  startCommand(EZSP_MFGLIB_START_TONE);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus mfglibStopTone(void)
{
  uint8_t status;
  startCommand(EZSP_MFGLIB_STOP_TONE);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus mfglibStartStream(void)
{
  uint8_t status;
  startCommand(EZSP_MFGLIB_START_STREAM);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus mfglibStopStream(void)
{
  uint8_t status;
  startCommand(EZSP_MFGLIB_STOP_STREAM);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus mfglibSendPacket(
      uint8_t packetLength,
      uint8_t *packetContents)
{
  uint8_t status;
  startCommand(EZSP_MFGLIB_SEND_PACKET);
  appendInt8u(packetLength);
  appendInt8uArray(packetLength, packetContents);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus mfglibSetChannel(
      uint8_t channel)
{
  uint8_t status;
  startCommand(EZSP_MFGLIB_SET_CHANNEL);
  appendInt8u(channel);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

uint8_t mfglibGetChannel(void)
{
  uint8_t channel;
  startCommand(EZSP_MFGLIB_GET_CHANNEL);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    channel = fetchInt8u();
    return channel;
  }
  return 255;
}

EmberStatus mfglibSetPower(
      uint16_t txPowerMode,
      int8_t power)
{
  uint8_t status;
  startCommand(EZSP_MFGLIB_SET_POWER);
  appendInt16u(txPowerMode);
  appendInt8u(power);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

int8_t mfglibGetPower(void)
{
  int8_t power;
  startCommand(EZSP_MFGLIB_GET_POWER);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    power = fetchInt8u();
    return power;
  }
  return -128;
}

//------------------------------------------------------------------------------
// Bootloader
//------------------------------------------------------------------------------

EmberStatus ezspLaunchStandaloneBootloader(
      uint8_t mode)
{
  uint8_t status;
  startCommand(EZSP_LAUNCH_STANDALONE_BOOTLOADER);
  appendInt8u(mode);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspSendBootloadMessage(
      bool broadcast,
      EmberEUI64 destEui64,
      uint8_t messageLength,
      uint8_t *messageContents)
{
  uint8_t status;
  startCommand(EZSP_SEND_BOOTLOAD_MESSAGE);
  appendInt8u(broadcast);
  appendInt8uArray(8, destEui64);
  appendInt8u(messageLength);
  appendInt8uArray(messageLength, messageContents);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

uint16_t ezspGetStandaloneBootloaderVersionPlatMicroPhy(
      uint8_t *nodePlat,
      uint8_t *nodeMicro,
      uint8_t *nodePhy)
{
  uint16_t bootloader_version;
  startCommand(EZSP_GET_STANDALONE_BOOTLOADER_VERSION_PLAT_MICRO_PHY);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    bootloader_version = fetchInt16u();
    *nodePlat = fetchInt8u();
    *nodeMicro = fetchInt8u();
    *nodePhy = fetchInt8u();
    return bootloader_version;
  }
  return 255;
}

void ezspAesEncrypt(
      uint8_t *plaintext,
      uint8_t *key,
      uint8_t *ciphertext)
{
  startCommand(EZSP_AES_ENCRYPT);
  appendInt8uArray(16, plaintext);
  appendInt8uArray(16, key);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    fetchInt8uArray(16, ciphertext);
  }
}

EmberStatus ezspOverrideCurrentChannel(
      uint8_t channel)
{
  uint8_t status;
  startCommand(EZSP_OVERRIDE_CURRENT_CHANNEL);
  appendInt8u(channel);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

//------------------------------------------------------------------------------
// ZLL
//------------------------------------------------------------------------------

EmberStatus ezspZllNetworkOps(
      EmberZllNetwork *networkInfo,
      EzspZllNetworkOperation op,
      int8_t radioTxPower)
{
  uint8_t status;
  startCommand(EZSP_ZLL_NETWORK_OPS);
  appendEmberZllNetwork(networkInfo);
  appendInt8u(op);
  appendInt8u(radioTxPower);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspZllSetInitialSecurityState(
      EmberKeyData *networkKey,
      EmberZllInitialSecurityState *securityState)
{
  uint8_t status;
  startCommand(EZSP_ZLL_SET_INITIAL_SECURITY_STATE);
  appendEmberKeyData(networkKey);
  appendEmberZllInitialSecurityState(securityState);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspZllStartScan(
      uint32_t channelMask,
      int8_t radioPowerForScan,
      EmberNodeType nodeType)
{
  uint8_t status;
  startCommand(EZSP_ZLL_START_SCAN);
  appendInt32u(channelMask);
  appendInt8u(radioPowerForScan);
  appendInt8u(nodeType);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspZllSetRxOnWhenIdle(
      uint16_t durationMs)
{
  uint8_t status;
  startCommand(EZSP_ZLL_SET_RX_ON_WHEN_IDLE);
  appendInt16u(durationMs);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspSetLogicalAndRadioChannel(
      uint8_t radioChannel)
{
  uint8_t status;
  startCommand(EZSP_SET_LOGICAL_AND_RADIO_CHANNEL);
  appendInt8u(radioChannel);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

uint8_t ezspGetLogicalChannel(void)
{
  uint8_t logicalChannel;
  startCommand(EZSP_GET_LOGICAL_CHANNEL);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    logicalChannel = fetchInt8u();
    return logicalChannel;
  }
  return 255;
}

void ezspZllGetTokens(
      EmberTokTypeStackZllData *data,
      EmberTokTypeStackZllSecurity *security)
{
  startCommand(EZSP_ZLL_GET_TOKENS);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    fetchEmberTokTypeStackZllData(data);
    fetchEmberTokTypeStackZllSecurity(security);
  }
}

void ezspZllSetDataToken(
      EmberTokTypeStackZllData *data)
{
  startCommand(EZSP_ZLL_SET_DATA_TOKEN);
  appendEmberTokTypeStackZllData(data);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    EZSP_ASH_TRACE("%s(): sendCommand() error: 0x%x", __func__, sendStatus);
  }
}

void ezspZllSetNonZllNetwork(void)
{
  startCommand(EZSP_ZLL_SET_NON_ZLL_NETWORK);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    EZSP_ASH_TRACE("%s(): sendCommand() error: 0x%x", __func__, sendStatus);
  }
}

bool ezspIsZllNetwork(void)
{
  uint8_t isZllNetwork;
  startCommand(EZSP_IS_ZLL_NETWORK);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    isZllNetwork = fetchInt8u();
    return isZllNetwork;
  }
  return false;
}

//------------------------------------------------------------------------------
// RF4CE
//------------------------------------------------------------------------------

EmberStatus ezspRf4ceSetPairingTableEntry(
      uint8_t pairingIndex,
      EmberRf4cePairingTableEntry *entry)
{
  uint8_t status;
  startCommand(EZSP_RF4CE_SET_PAIRING_TABLE_ENTRY);
  appendInt8u(pairingIndex);
  appendEmberRf4cePairingTableEntry(entry);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspRf4ceGetPairingTableEntry(
      uint8_t pairingIndex,
      EmberRf4cePairingTableEntry *entry)
{
  uint8_t status;
  startCommand(EZSP_RF4CE_GET_PAIRING_TABLE_ENTRY);
  appendInt8u(pairingIndex);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    fetchEmberRf4cePairingTableEntry(entry);
    return status;
  }
  return sendStatus;
}

EmberStatus ezspRf4ceDeletePairingTableEntry(
      uint8_t pairingIndex)
{
  uint8_t status;
  startCommand(EZSP_RF4CE_DELETE_PAIRING_TABLE_ENTRY);
  appendInt8u(pairingIndex);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspRf4ceKeyUpdate(
      uint8_t pairingIndex,
      EmberKeyData *key)
{
  uint8_t status;
  startCommand(EZSP_RF4CE_KEY_UPDATE);
  appendInt8u(pairingIndex);
  appendEmberKeyData(key);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspRf4ceSend(
      uint8_t pairingIndex,
      uint8_t profileId,
      uint16_t vendorId,
      EmberRf4ceTxOption txOptions,
      uint8_t messageTag,
      uint8_t messageLength,
      uint8_t *message)
{
  uint8_t status;
  startCommand(EZSP_RF4CE_SEND);
  appendInt8u(pairingIndex);
  appendInt8u(profileId);
  appendInt16u(vendorId);
  appendInt8u(txOptions);
  appendInt8u(messageTag);
  appendInt8u(messageLength);
  appendInt8uArray(messageLength, message);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspRf4ceStart(
      EmberRf4ceNodeCapabilities capabilities,
      EmberRf4ceVendorInfo *vendorInfo,
      int8_t power)
{
  uint8_t status;
  startCommand(EZSP_RF4CE_START);
  appendInt8u(capabilities);
  appendEmberRf4ceVendorInfo(vendorInfo);
  appendInt8u(power);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspRf4ceStop(void)
{
  uint8_t status;
  startCommand(EZSP_RF4CE_STOP);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspRf4ceDiscovery(
      EmberPanId panId,
      EmberNodeId nodeId,
      uint8_t searchDevType,
      uint16_t discDuration,
      uint8_t maxDiscRepetitions,
      uint8_t discProfileIdListLength,
      uint8_t *discProfileIdList)
{
  uint8_t status;
  startCommand(EZSP_RF4CE_DISCOVERY);
  appendInt16u(panId);
  appendInt16u(nodeId);
  appendInt8u(searchDevType);
  appendInt16u(discDuration);
  appendInt8u(maxDiscRepetitions);
  appendInt8u(discProfileIdListLength);
  appendInt8uArray(discProfileIdListLength, discProfileIdList);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspRf4ceEnableAutoDiscoveryResponse(
      uint16_t duration)
{
  uint8_t status;
  startCommand(EZSP_RF4CE_ENABLE_AUTO_DISCOVERY_RESPONSE);
  appendInt16u(duration);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspRf4cePair(
      uint8_t channel,
      EmberPanId panId,
      EmberEUI64 ieeeAddr,
      uint8_t keyExchangeTransferCount)
{
  uint8_t status;
  startCommand(EZSP_RF4CE_PAIR);
  appendInt8u(channel);
  appendInt16u(panId);
  appendInt8uArray(8, ieeeAddr);
  appendInt8u(keyExchangeTransferCount);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspRf4ceUnpair(
      uint8_t pairingIndex)
{
  uint8_t status;
  startCommand(EZSP_RF4CE_UNPAIR);
  appendInt8u(pairingIndex);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspRf4ceSetPowerSavingParameters(
      uint32_t dutyCycle,
      uint32_t activePeriod)
{
  uint8_t status;
  startCommand(EZSP_RF4CE_SET_POWER_SAVING_PARAMETERS);
  appendInt32u(dutyCycle);
  appendInt32u(activePeriod);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspRf4ceSetFrequencyAgilityParameters(
      uint8_t rssiWindowSize,
      uint8_t channelChangeReads,
      int8_t rssiThreshold,
      uint16_t readInterval,
      uint8_t readDuration)
{
  uint8_t status;
  startCommand(EZSP_RF4CE_SET_FREQUENCY_AGILITY_PARAMETERS);
  appendInt8u(rssiWindowSize);
  appendInt8u(channelChangeReads);
  appendInt8u(rssiThreshold);
  appendInt16u(readInterval);
  appendInt8u(readDuration);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspRf4ceSetApplicationInfo(
      EmberRf4ceApplicationInfo *appInfo)
{
  uint8_t status;
  startCommand(EZSP_RF4CE_SET_APPLICATION_INFO);
  appendEmberRf4ceApplicationInfo(appInfo);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EmberStatus ezspRf4ceGetApplicationInfo(
      EmberRf4ceApplicationInfo *appInfo)
{
  uint8_t status;
  startCommand(EZSP_RF4CE_GET_APPLICATION_INFO);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    fetchEmberRf4ceApplicationInfo(appInfo);
    return status;
  }
  return sendStatus;
}

uint8_t ezspRf4ceGetMaxPayload(
      uint8_t pairingIndex,
      EmberRf4ceTxOption txOptions)
{
  uint8_t maxLength;
  startCommand(EZSP_RF4CE_GET_MAX_PAYLOAD);
  appendInt8u(pairingIndex);
  appendInt8u(txOptions);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    maxLength = fetchInt8u();
    return maxLength;
  }
  return 255;
}

EmberStatus ezspRf4ceGetNetworkParameters(
      EmberNodeType *nodeType,
      EmberNetworkParameters *parameters)
{
  uint8_t status;
  startCommand(EZSP_RF4CE_GET_NETWORK_PARAMETERS);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    *nodeType = fetchInt8u();
    fetchEmberNetworkParameters(parameters);
    return status;
  }
  return sendStatus;
}

//------------------------------------------------------------------------------
// Green Power
//------------------------------------------------------------------------------

bool ezspGpProxyTableProcessGpPairing(
      uint32_t options,
      EmberGpAddress *addr,
      uint8_t commMode,
      uint16_t sinkNetworkAddress,
      uint16_t sinkGroupId,
      uint16_t assignedAlias,
      uint8_t *sinkIeeeAddress,
      EmberKeyData *gpdKey)
{
  uint8_t gpPairingAdded;
  startCommand(EZSP_GP_PROXY_TABLE_PROCESS_GP_PAIRING);
  appendInt32u(options);
  appendEmberGpAddress(addr);
  appendInt8u(commMode);
  appendInt16u(sinkNetworkAddress);
  appendInt16u(sinkGroupId);
  appendInt16u(assignedAlias);
  appendInt8uArray(8, sinkIeeeAddress);
  appendEmberKeyData(gpdKey);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    gpPairingAdded = fetchInt8u();
    return gpPairingAdded;
  }
  return false;
}

EmberStatus ezspDGpSend(
      bool action,
      bool useCca,
      EmberGpAddress *addr,
      uint8_t gpdCommandId,
      uint8_t gpdAsduLength,
      uint8_t *gpdAsdu,
      uint8_t gpepHandle,
      uint16_t gpTxQueueEntryLifetimeMs)
{
  uint8_t status;
  startCommand(EZSP_D_GP_SEND);
  appendInt8u(action);
  appendInt8u(useCca);
  appendEmberGpAddress(addr);
  appendInt8u(gpdCommandId);
  appendInt8u(gpdAsduLength);
  appendInt8uArray(gpdAsduLength, gpdAsdu);
  appendInt8u(gpepHandle);
  appendInt16u(gpTxQueueEntryLifetimeMs);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

//------------------------------------------------------------------------------
// Secure EZSP
//------------------------------------------------------------------------------

EzspStatus ezspSetSecurityKey(
      EmberKeyData *key,
      SecureEzspSecurityType securityType)
{
  uint8_t status;
  startCommand(EZSP_SET_SECURITY_KEY);
  appendEmberKeyData(key);
  appendInt32u(securityType);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EzspStatus ezspSetSecurityParameters(
      SecureEzspSecurityLevel securityLevel,
      SecureEzspRandomNumber *hostRandomNumber,
      SecureEzspRandomNumber *returnNcpRandomNumber)
{
  uint8_t status;
  startCommand(EZSP_SET_SECURITY_PARAMETERS);
  appendInt8u(securityLevel);
  appendSecureEzspRandomNumber(hostRandomNumber);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    fetchSecureEzspRandomNumber(returnNcpRandomNumber);
    return status;
  }
  return sendStatus;
}

EzspStatus ezspResetToFactoryDefaults(void)
{
  uint8_t status;
  startCommand(EZSP_RESET_TO_FACTORY_DEFAULTS);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    return status;
  }
  return sendStatus;
}

EzspStatus ezspGetSecurityKeyStatus(
      SecureEzspSecurityType *returnSecurityType)
{
  uint8_t status;
  startCommand(EZSP_GET_SECURITY_KEY_STATUS);
  EzspStatus sendStatus = sendCommand();
  if (sendStatus == EZSP_SUCCESS) {
    status = fetchInt8u();
    *returnSecurityType = fetchInt32u();
    return status;
  }
  return sendStatus;
}

static void callbackDispatch(void)
{
  callbackPointerInit();
  switch (serialGetResponseByte(EZSP_EXTENDED_FRAME_ID_INDEX)) {

  case EZSP_NO_CALLBACKS: {
    ezspNoCallbacks();
    break;
  }

  case EZSP_STACK_TOKEN_CHANGED_HANDLER: {
    uint16_t tokenAddress;
    tokenAddress = fetchInt16u();
    ezspStackTokenChangedHandler(tokenAddress);
    break;
  }

  case EZSP_TIMER_HANDLER: {
    uint8_t timerId;
    timerId = fetchInt8u();
    ezspTimerHandler(timerId);
    break;
  }

  case EZSP_COUNTER_ROLLOVER_HANDLER: {
    uint8_t type;
    type = fetchInt8u();
    ezspCounterRolloverHandler(type);
    break;
  }

  case EZSP_CUSTOM_FRAME_HANDLER: {
    uint8_t payloadLength;
    uint8_t *payload;
    payloadLength = fetchInt8u();
    payload = fetchInt8uPointer(payloadLength);
    ezspCustomFrameHandler(payloadLength, payload);
    break;
  }

  case EZSP_STACK_STATUS_HANDLER: {
    uint8_t status;
    status = fetchInt8u();
    ezspStackStatusHandler(status);
    break;
  }

  case EZSP_ENERGY_SCAN_RESULT_HANDLER: {
    uint8_t channel;
    int8_t maxRssiValue;
    channel = fetchInt8u();
    maxRssiValue = fetchInt8u();
    ezspEnergyScanResultHandler(channel, maxRssiValue);
    break;
  }

  case EZSP_NETWORK_FOUND_HANDLER: {
    EmberZigbeeNetwork networkFound;
    uint8_t lastHopLqi;
    int8_t lastHopRssi;
    fetchEmberZigbeeNetwork(&networkFound);
    lastHopLqi = fetchInt8u();
    lastHopRssi = fetchInt8u();
    ezspNetworkFoundHandler(&networkFound, lastHopLqi, lastHopRssi);
    break;
  }

  case EZSP_SCAN_COMPLETE_HANDLER: {
    uint8_t channel;
    uint8_t status;
    channel = fetchInt8u();
    status = fetchInt8u();
    ezspScanCompleteHandler(channel, status);
    break;
  }

  case EZSP_CHILD_JOIN_HANDLER: {
    uint8_t index;
    uint8_t joining;
    uint16_t childId;
    uint8_t childEui64[8];
    uint8_t childType;
    index = fetchInt8u();
    joining = fetchInt8u();
    childId = fetchInt16u();
    fetchInt8uArray(8, childEui64);
    childType = fetchInt8u();
    ezspChildJoinHandler(index, joining, childId, childEui64, childType);
    break;
  }

  case EZSP_REMOTE_SET_BINDING_HANDLER: {
    EmberBindingTableEntry entry;
    uint8_t index;
    uint8_t policyDecision;
    fetchEmberBindingTableEntry(&entry);
    index = fetchInt8u();
    policyDecision = fetchInt8u();
    ezspRemoteSetBindingHandler(&entry, index, policyDecision);
    break;
  }

  case EZSP_REMOTE_DELETE_BINDING_HANDLER: {
    uint8_t index;
    uint8_t policyDecision;
    index = fetchInt8u();
    policyDecision = fetchInt8u();
    ezspRemoteDeleteBindingHandler(index, policyDecision);
    break;
  }

  case EZSP_MESSAGE_SENT_HANDLER: {
    uint8_t type;
    uint16_t indexOrDestination;
    EmberApsFrame apsFrame;
    uint8_t messageTag;
    uint8_t status;
    uint8_t messageLength;
    uint8_t *messageContents;
    type = fetchInt8u();
    indexOrDestination = fetchInt16u();
    fetchEmberApsFrame(&apsFrame);
    messageTag = fetchInt8u();
    status = fetchInt8u();
    messageLength = fetchInt8u();
    messageContents = fetchInt8uPointer(messageLength);
    ezspMessageSentHandler(type, indexOrDestination, &apsFrame, messageTag, status, messageLength, messageContents);
    break;
  }

  case EZSP_POLL_COMPLETE_HANDLER: {
    uint8_t status;
    status = fetchInt8u();
    ezspPollCompleteHandler(status);
    break;
  }

  case EZSP_POLL_HANDLER: {
    uint16_t childId;
    childId = fetchInt16u();
    ezspPollHandler(childId);
    break;
  }

  case EZSP_INCOMING_SENDER_EUI64_HANDLER: {
    uint8_t senderEui64[8];
    fetchInt8uArray(8, senderEui64);
    ezspIncomingSenderEui64Handler(senderEui64);
    break;
  }

  case EZSP_INCOMING_MESSAGE_HANDLER: {
    uint8_t type;
    EmberApsFrame apsFrame;
    uint8_t lastHopLqi;
    int8_t lastHopRssi;
    uint16_t sender;
    uint8_t bindingIndex;
    uint8_t addressIndex;
    uint8_t messageLength;
    uint8_t *messageContents;
    type = fetchInt8u();
    fetchEmberApsFrame(&apsFrame);
    lastHopLqi = fetchInt8u();
    lastHopRssi = fetchInt8u();
    sender = fetchInt16u();
    bindingIndex = fetchInt8u();
    addressIndex = fetchInt8u();
    messageLength = fetchInt8u();
    messageContents = fetchInt8uPointer(messageLength);
    ezspIncomingMessageHandler(type, &apsFrame, lastHopLqi, lastHopRssi, sender, bindingIndex, addressIndex, messageLength, messageContents);
    break;
  }

  case EZSP_INCOMING_ROUTE_RECORD_HANDLER: {
    uint16_t source;
    uint8_t sourceEui[8];
    uint8_t lastHopLqi;
    int8_t lastHopRssi;
    uint8_t relayCount;
    uint8_t *relayList;
    source = fetchInt16u();
    fetchInt8uArray(8, sourceEui);
    lastHopLqi = fetchInt8u();
    lastHopRssi = fetchInt8u();
    relayCount = fetchInt8u();
    relayList = fetchInt8uPointer(relayCount*2);
    ezspIncomingRouteRecordHandler(source, sourceEui, lastHopLqi, lastHopRssi, relayCount, relayList);
    break;
  }

  case EZSP_INCOMING_MANY_TO_ONE_ROUTE_REQUEST_HANDLER: {
    uint16_t source;
    uint8_t longId[8];
    uint8_t cost;
    source = fetchInt16u();
    fetchInt8uArray(8, longId);
    cost = fetchInt8u();
    ezspIncomingManyToOneRouteRequestHandler(source, longId, cost);
    break;
  }

  case EZSP_INCOMING_ROUTE_ERROR_HANDLER: {
    uint8_t status;
    uint16_t target;
    status = fetchInt8u();
    target = fetchInt16u();
    ezspIncomingRouteErrorHandler(status, target);
    break;
  }

  case EZSP_ID_CONFLICT_HANDLER: {
    uint16_t id;
    id = fetchInt16u();
    ezspIdConflictHandler(id);
    break;
  }

  case EZSP_MAC_PASSTHROUGH_MESSAGE_HANDLER: {
    uint8_t messageType;
    uint8_t lastHopLqi;
    int8_t lastHopRssi;
    uint8_t messageLength;
    uint8_t *messageContents;
    messageType = fetchInt8u();
    lastHopLqi = fetchInt8u();
    lastHopRssi = fetchInt8u();
    messageLength = fetchInt8u();
    messageContents = fetchInt8uPointer(messageLength);
    ezspMacPassthroughMessageHandler(messageType, lastHopLqi, lastHopRssi, messageLength, messageContents);
    break;
  }

  case EZSP_MAC_FILTER_MATCH_MESSAGE_HANDLER: {
    uint8_t filterIndexMatch;
    uint8_t legacyPassthroughType;
    uint8_t lastHopLqi;
    int8_t lastHopRssi;
    uint8_t messageLength;
    uint8_t *messageContents;
    filterIndexMatch = fetchInt8u();
    legacyPassthroughType = fetchInt8u();
    lastHopLqi = fetchInt8u();
    lastHopRssi = fetchInt8u();
    messageLength = fetchInt8u();
    messageContents = fetchInt8uPointer(messageLength);
    ezspMacFilterMatchMessageHandler(filterIndexMatch, legacyPassthroughType, lastHopLqi, lastHopRssi, messageLength, messageContents);
    break;
  }

  case EZSP_RAW_TRANSMIT_COMPLETE_HANDLER: {
    uint8_t status;
    status = fetchInt8u();
    ezspRawTransmitCompleteHandler(status);
    break;
  }

  case EZSP_SWITCH_NETWORK_KEY_HANDLER: {
    uint8_t sequenceNumber;
    sequenceNumber = fetchInt8u();
    ezspSwitchNetworkKeyHandler(sequenceNumber);
    break;
  }

  case EZSP_ZIGBEE_KEY_ESTABLISHMENT_HANDLER: {
    uint8_t partner[8];
    uint8_t status;
    fetchInt8uArray(8, partner);
    status = fetchInt8u();
    ezspZigbeeKeyEstablishmentHandler(partner, status);
    break;
  }

  case EZSP_TRUST_CENTER_JOIN_HANDLER: {
    uint16_t newNodeId;
    uint8_t newNodeEui64[8];
    uint8_t status;
    uint8_t policyDecision;
    uint16_t parentOfNewNodeId;
    newNodeId = fetchInt16u();
    fetchInt8uArray(8, newNodeEui64);
    status = fetchInt8u();
    policyDecision = fetchInt8u();
    parentOfNewNodeId = fetchInt16u();
    ezspTrustCenterJoinHandler(newNodeId, newNodeEui64, status, policyDecision, parentOfNewNodeId);
    break;
  }

  case EZSP_GENERATE_CBKE_KEYS_HANDLER: {
    uint8_t status;
    EmberPublicKeyData ephemeralPublicKey;
    status = fetchInt8u();
    fetchEmberPublicKeyData(&ephemeralPublicKey);
    ezspGenerateCbkeKeysHandler(status, &ephemeralPublicKey);
    break;
  }

  case EZSP_CALCULATE_SMACS_HANDLER: {
    uint8_t status;
    EmberSmacData initiatorSmac;
    EmberSmacData responderSmac;
    status = fetchInt8u();
    fetchEmberSmacData(&initiatorSmac);
    fetchEmberSmacData(&responderSmac);
    ezspCalculateSmacsHandler(status, &initiatorSmac, &responderSmac);
    break;
  }

  case EZSP_GENERATE_CBKE_KEYS_HANDLER283K1: {
    uint8_t status;
    EmberPublicKey283k1Data ephemeralPublicKey;
    status = fetchInt8u();
    fetchEmberPublicKey283k1Data(&ephemeralPublicKey);
    ezspGenerateCbkeKeysHandler283k1(status, &ephemeralPublicKey);
    break;
  }

  case EZSP_CALCULATE_SMACS_HANDLER283K1: {
    uint8_t status;
    EmberSmacData initiatorSmac;
    EmberSmacData responderSmac;
    status = fetchInt8u();
    fetchEmberSmacData(&initiatorSmac);
    fetchEmberSmacData(&responderSmac);
    ezspCalculateSmacsHandler283k1(status, &initiatorSmac, &responderSmac);
    break;
  }

  case EZSP_DSA_SIGN_HANDLER: {
    uint8_t status;
    uint8_t messageLength;
    uint8_t *messageContents;
    status = fetchInt8u();
    messageLength = fetchInt8u();
    messageContents = fetchInt8uPointer(messageLength);
    ezspDsaSignHandler(status, messageLength, messageContents);
    break;
  }

  case EZSP_DSA_VERIFY_HANDLER: {
    uint8_t status;
    status = fetchInt8u();
    ezspDsaVerifyHandler(status);
    break;
  }

  case EZSP_MFGLIB_RX_HANDLER: {
    uint8_t linkQuality;
    int8_t rssi;
    uint8_t packetLength;
    uint8_t *packetContents;
    linkQuality = fetchInt8u();
    rssi = fetchInt8u();
    packetLength = fetchInt8u();
    packetContents = fetchInt8uPointer(packetLength);
    ezspMfglibRxHandler(linkQuality, rssi, packetLength, packetContents);
    break;
  }

  case EZSP_INCOMING_BOOTLOAD_MESSAGE_HANDLER: {
    uint8_t longId[8];
    uint8_t lastHopLqi;
    int8_t lastHopRssi;
    uint8_t messageLength;
    uint8_t *messageContents;
    fetchInt8uArray(8, longId);
    lastHopLqi = fetchInt8u();
    lastHopRssi = fetchInt8u();
    messageLength = fetchInt8u();
    messageContents = fetchInt8uPointer(messageLength);
    ezspIncomingBootloadMessageHandler(longId, lastHopLqi, lastHopRssi, messageLength, messageContents);
    break;
  }

  case EZSP_BOOTLOAD_TRANSMIT_COMPLETE_HANDLER: {
    uint8_t status;
    uint8_t messageLength;
    uint8_t *messageContents;
    status = fetchInt8u();
    messageLength = fetchInt8u();
    messageContents = fetchInt8uPointer(messageLength);
    ezspBootloadTransmitCompleteHandler(status, messageLength, messageContents);
    break;
  }

  case EZSP_ZLL_NETWORK_FOUND_HANDLER: {
    EmberZllNetwork networkInfo;
    uint8_t isDeviceInfoNull;
    EmberZllDeviceInfoRecord deviceInfo;
    uint8_t lastHopLqi;
    int8_t lastHopRssi;
    fetchEmberZllNetwork(&networkInfo);
    isDeviceInfoNull = fetchInt8u();
    fetchEmberZllDeviceInfoRecord(&deviceInfo);
    lastHopLqi = fetchInt8u();
    lastHopRssi = fetchInt8u();
    ezspZllNetworkFoundHandler(&networkInfo, isDeviceInfoNull, &deviceInfo, lastHopLqi, lastHopRssi);
    break;
  }

  case EZSP_ZLL_SCAN_COMPLETE_HANDLER: {
    uint8_t status;
    status = fetchInt8u();
    ezspZllScanCompleteHandler(status);
    break;
  }

  case EZSP_ZLL_ADDRESS_ASSIGNMENT_HANDLER: {
    EmberZllAddressAssignment addressInfo;
    uint8_t lastHopLqi;
    int8_t lastHopRssi;
    fetchEmberZllAddressAssignment(&addressInfo);
    lastHopLqi = fetchInt8u();
    lastHopRssi = fetchInt8u();
    ezspZllAddressAssignmentHandler(&addressInfo, lastHopLqi, lastHopRssi);
    break;
  }

  case EZSP_ZLL_TOUCH_LINK_TARGET_HANDLER: {
    EmberZllNetwork networkInfo;
    fetchEmberZllNetwork(&networkInfo);
    ezspZllTouchLinkTargetHandler(&networkInfo);
    break;
  }

  case EZSP_RF4CE_INCOMING_MESSAGE_HANDLER: {
    uint8_t pairingIndex;
    uint8_t profileId;
    uint16_t vendorId;
    uint8_t txOptions;
    uint8_t messageLength;
    uint8_t *message;
    pairingIndex = fetchInt8u();
    profileId = fetchInt8u();
    vendorId = fetchInt16u();
    txOptions = fetchInt8u();
    messageLength = fetchInt8u();
    message = fetchInt8uPointer(messageLength);
    ezspRf4ceIncomingMessageHandler(pairingIndex, profileId, vendorId, txOptions, messageLength, message);
    break;
  }

  case EZSP_RF4CE_MESSAGE_SENT_HANDLER: {
    uint8_t status;
    uint8_t pairingIndex;
    uint8_t txOptions;
    uint8_t profileId;
    uint16_t vendorId;
    uint8_t messageTag;
    uint8_t messageLength;
    uint8_t *message;
    status = fetchInt8u();
    pairingIndex = fetchInt8u();
    txOptions = fetchInt8u();
    profileId = fetchInt8u();
    vendorId = fetchInt16u();
    messageTag = fetchInt8u();
    messageLength = fetchInt8u();
    message = fetchInt8uPointer(messageLength);
    ezspRf4ceMessageSentHandler(status, pairingIndex, txOptions, profileId, vendorId, messageTag, messageLength, message);
    break;
  }

  case EZSP_RF4CE_DISCOVERY_COMPLETE_HANDLER: {
    uint8_t status;
    status = fetchInt8u();
    ezspRf4ceDiscoveryCompleteHandler(status);
    break;
  }

  case EZSP_RF4CE_DISCOVERY_REQUEST_HANDLER: {
    uint8_t ieeeAddr[8];
    uint8_t nodeCapabilities;
    EmberRf4ceVendorInfo vendorInfo;
    EmberRf4ceApplicationInfo appInfo;
    uint8_t searchDevType;
    uint8_t rxLinkQuality;
    fetchInt8uArray(8, ieeeAddr);
    nodeCapabilities = fetchInt8u();
    fetchEmberRf4ceVendorInfo(&vendorInfo);
    fetchEmberRf4ceApplicationInfo(&appInfo);
    searchDevType = fetchInt8u();
    rxLinkQuality = fetchInt8u();
    ezspRf4ceDiscoveryRequestHandler(ieeeAddr, nodeCapabilities, &vendorInfo, &appInfo, searchDevType, rxLinkQuality);
    break;
  }

  case EZSP_RF4CE_DISCOVERY_RESPONSE_HANDLER: {
    uint8_t atCapacity;
    uint8_t channel;
    uint16_t panId;
    uint8_t ieeeAddr[8];
    uint8_t nodeCapabilities;
    EmberRf4ceVendorInfo vendorInfo;
    EmberRf4ceApplicationInfo appInfo;
    uint8_t rxLinkQuality;
    uint8_t discRequestLqi;
    atCapacity = fetchInt8u();
    channel = fetchInt8u();
    panId = fetchInt16u();
    fetchInt8uArray(8, ieeeAddr);
    nodeCapabilities = fetchInt8u();
    fetchEmberRf4ceVendorInfo(&vendorInfo);
    fetchEmberRf4ceApplicationInfo(&appInfo);
    rxLinkQuality = fetchInt8u();
    discRequestLqi = fetchInt8u();
    ezspRf4ceDiscoveryResponseHandler(atCapacity, channel, panId, ieeeAddr, nodeCapabilities, &vendorInfo, &appInfo, rxLinkQuality, discRequestLqi);
    break;
  }

  case EZSP_RF4CE_AUTO_DISCOVERY_RESPONSE_COMPLETE_HANDLER: {
    uint8_t status;
    uint8_t srcIeeeAddr[8];
    uint8_t nodeCapabilities;
    EmberRf4ceVendorInfo vendorInfo;
    EmberRf4ceApplicationInfo appInfo;
    uint8_t searchDevType;
    status = fetchInt8u();
    fetchInt8uArray(8, srcIeeeAddr);
    nodeCapabilities = fetchInt8u();
    fetchEmberRf4ceVendorInfo(&vendorInfo);
    fetchEmberRf4ceApplicationInfo(&appInfo);
    searchDevType = fetchInt8u();
    ezspRf4ceAutoDiscoveryResponseCompleteHandler(status, srcIeeeAddr, nodeCapabilities, &vendorInfo, &appInfo, searchDevType);
    break;
  }

  case EZSP_RF4CE_PAIR_COMPLETE_HANDLER: {
    uint8_t status;
    uint8_t pairingIndex;
    EmberRf4ceVendorInfo vendorInfo;
    EmberRf4ceApplicationInfo appInfo;
    status = fetchInt8u();
    pairingIndex = fetchInt8u();
    fetchEmberRf4ceVendorInfo(&vendorInfo);
    fetchEmberRf4ceApplicationInfo(&appInfo);
    ezspRf4cePairCompleteHandler(status, pairingIndex, &vendorInfo, &appInfo);
    break;
  }

  case EZSP_RF4CE_PAIR_REQUEST_HANDLER: {
    uint8_t status;
    uint8_t pairingIndex;
    uint8_t srcIeeeAddr[8];
    uint8_t nodeCapabilities;
    EmberRf4ceVendorInfo vendorInfo;
    EmberRf4ceApplicationInfo appInfo;
    uint8_t keyExchangeTransferCount;
    status = fetchInt8u();
    pairingIndex = fetchInt8u();
    fetchInt8uArray(8, srcIeeeAddr);
    nodeCapabilities = fetchInt8u();
    fetchEmberRf4ceVendorInfo(&vendorInfo);
    fetchEmberRf4ceApplicationInfo(&appInfo);
    keyExchangeTransferCount = fetchInt8u();
    ezspRf4cePairRequestHandler(status, pairingIndex, srcIeeeAddr, nodeCapabilities, &vendorInfo, &appInfo, keyExchangeTransferCount);
    break;
  }

  case EZSP_RF4CE_UNPAIR_HANDLER: {
    uint8_t pairingIndex;
    pairingIndex = fetchInt8u();
    ezspRf4ceUnpairHandler(pairingIndex);
    break;
  }

  case EZSP_RF4CE_UNPAIR_COMPLETE_HANDLER: {
    uint8_t pairingIndex;
    pairingIndex = fetchInt8u();
    ezspRf4ceUnpairCompleteHandler(pairingIndex);
    break;
  }

  case EZSP_D_GP_SENT_HANDLER: {
    uint8_t status;
    uint8_t gpepHandle;
    status = fetchInt8u();
    gpepHandle = fetchInt8u();
    ezspDGpSentHandler(status, gpepHandle);
    break;
  }

  case EZSP_GPEP_INCOMING_MESSAGE_HANDLER: {
    uint8_t status;
    uint8_t gpdLink;
    uint8_t sequenceNumber;
    EmberGpAddress addr;
    uint8_t gpdfSecurityLevel;
    uint8_t gpdfSecurityKeyType;
    uint8_t autoCommissioning;
    uint8_t rxAfterTx;
    uint32_t gpdSecurityFrameCounter;
    uint8_t gpdCommandId;
    uint32_t mic;
    // TODO: this is manually edited. Keep it manually edited until proper fix.
    EmberGpSinkListEntry sinkList[2];
    uint8_t gpdCommandPayloadLength;
    uint8_t *gpdCommandPayload;
    status = fetchInt8u();
    gpdLink = fetchInt8u();
    sequenceNumber = fetchInt8u();
    fetchEmberGpAddress(&addr);
    gpdfSecurityLevel = fetchInt8u();
    gpdfSecurityKeyType = fetchInt8u();
    autoCommissioning = fetchInt8u();
    rxAfterTx = fetchInt8u();
    gpdSecurityFrameCounter = fetchInt32u();
    gpdCommandId = fetchInt8u();
    mic = fetchInt32u();
    // TODO: this is manually edited. Keep it manually edited until proper fix.
    fetchEmberGpSinkList(sinkList);
    gpdCommandPayloadLength = fetchInt8u();
    gpdCommandPayload = fetchInt8uPointer(gpdCommandPayloadLength);
    // TODO: this is manually edited. Keep it manually edited until proper fix.
    ezspGpepIncomingMessageHandler(status, gpdLink, sequenceNumber, &addr, gpdfSecurityLevel, gpdfSecurityKeyType, autoCommissioning, rxAfterTx, gpdSecurityFrameCounter, gpdCommandId, mic, sinkList, gpdCommandPayloadLength, gpdCommandPayload);
    break;
  }

  default:
    ezspErrorHandler(EZSP_ERROR_INVALID_FRAME_ID);
  }

}  

