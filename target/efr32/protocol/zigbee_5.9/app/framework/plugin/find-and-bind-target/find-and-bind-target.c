// Copyright 2016 Silicon Laboratories, Inc.

#include "app/framework/include/af.h"

#include "find-and-bind-target.h"

// -----------------------------------------------------------------------------
// Public API

EmberAfStatus emberAfPluginFindAndBindTargetStart(uint8_t endpoint)
{
  // Write the identify time.
  uint16_t identifyTime = EMBER_AF_PLUGIN_FIND_AND_BIND_COMMISSIONING_TIME;
  EmberAfStatus status = EMBER_ZCL_STATUS_UNSUPPORTED_ATTRIBUTE;

  if (emberAfContainsServer(endpoint, ZCL_IDENTIFY_CLUSTER_ID))
    status = emberAfWriteServerAttribute(endpoint,
                                         ZCL_IDENTIFY_CLUSTER_ID,
                                         ZCL_IDENTIFY_TIME_ATTRIBUTE_ID,
                                         (uint8_t *)&identifyTime,
                                         ZCL_INT16U_ATTRIBUTE_TYPE);

  emberAfCorePrintln("%p: %p: 0x%X",
                     EMBER_AF_PLUGIN_FIND_AND_BIND_TARGET_PLUGIN_NAME,
                     "Start target",
                     status);

  return status;
}
