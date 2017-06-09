// Copyright 2014 Silicon Laboratories, Inc.

#include PLATFORM_HEADER
#include CONFIGURATION_HEADER
#include EMBER_AF_API_EMBER_TYPES
#ifdef EMBER_AF_API_STACK
  #include EMBER_AF_API_STACK
#endif
#ifdef EMBER_AF_API_EZSP_PROTOCOL
  #include EMBER_AF_API_EZSP_PROTOCOL
#endif
#ifdef EMBER_AF_API_EZSP
  #include EMBER_AF_API_EZSP
#endif

#ifdef EZSP_HOST

// TODO: Cache values that change infrequently.

static EmberEUI64 eui64;

void emberAfPluginNetworkInterfaceNcpInitCallback(bool memoryAllocation)
{
  if (!memoryAllocation) {
    ezspGetEui64(eui64);
  }
}

void emberAfGetEui64(EmberEUI64 returnEui64)
{
  MEMCOPY(returnEui64, eui64, EUI64_SIZE);
}

EmberStatus emberAfGetNetworkParameters(EmberNodeType *nodeType,
                                        EmberNetworkParameters *parameters)
{
  return ezspRf4ceGetNetworkParameters(nodeType, parameters);
}

EmberNodeId emberAfGetNodeId(void)
{
  return ezspGetNodeId();
}

EmberStatus emberAfGetNodeType(EmberNodeType *nodeType)
{
  EmberNetworkParameters parameters;
  return ezspRf4ceGetNetworkParameters(nodeType, &parameters);
}

EmberPanId emberAfGetPanId(void)
{
  EmberNodeType nodeType;
  EmberNetworkParameters parameters;
  ezspRf4ceGetNetworkParameters(&nodeType, &parameters);
  return parameters.panId;
}

EmberStatus emberAfNetworkInit(void)
{
  return ezspNetworkInit();
}

#else

void emberAfPluginNetworkInterfaceNcpInitCallback(bool memoryAllocation)
{
}

void emberAfGetEui64(EmberEUI64 returnEui64)
{
  MEMCOPY(returnEui64, emberGetEui64(), EUI64_SIZE);
}

EmberStatus emberAfGetNetworkParameters(EmberNodeType *nodeType,
                                        EmberNetworkParameters *parameters)
{
  emberGetNetworkParameters(parameters);
  return emberGetNodeType(nodeType);
}

EmberNodeId emberAfGetNodeId(void)
{
  return emberGetNodeId();
}

EmberStatus emberAfGetNodeType(EmberNodeType *nodeType)
{
  return emberGetNodeType(nodeType);
}

EmberPanId emberAfGetPanId(void)
{
  return emberGetPanId();
}

EmberStatus emberAfNetworkInit(void)
{
  return emberNetworkInit();
}

#endif
