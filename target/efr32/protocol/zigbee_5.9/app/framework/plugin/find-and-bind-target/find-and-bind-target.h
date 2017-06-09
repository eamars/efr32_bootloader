// Copyright 2016 Silicon Laboratories, Inc.

#ifndef __FIND_AND_BIND_TARGET_H__
#define __FIND_AND_BIND_TARGET_H__

// -----------------------------------------------------------------------------
// Constants

#define EMBER_AF_PLUGIN_FIND_AND_BIND_TARGET_PLUGIN_NAME "Find and Bind Target"

#ifndef EMBER_AF_PLUGIN_FIND_AND_BIND_COMMISSIONING_TIME
  #define  EMBER_AF_PLUGIN_FIND_AND_BIND_COMMISSIONING_TIME (180) /* seconds */
#endif

// -----------------------------------------------------------------------------
// API

/** @brief Start target finding and binding operations
 *
 * A call to this function will commence the target finding and
 * binding operations. Specifically, the target will attempt to start
 * identifying on the endpoint that is passed as a parameter.
 *
 * @param endpoint The endpoint on which to begin target operations.
 *
 * @returns An ::EmberAfStatus value describing the success of the
 * commencement of the target operations.
 */
EmberAfStatus emberAfPluginFindAndBindTargetStart(uint8_t endpoint);

#endif /* __FIND_AND_BIND_TARGET_H__ */

