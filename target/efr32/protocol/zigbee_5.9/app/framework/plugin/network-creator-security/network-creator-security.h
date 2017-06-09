// Copyright 2015 Silicon Laboratories, Inc.

// -----------------------------------------------------------------------------
// Constants

#define EMBER_AF_PLUGIN_NETWORK_CREATOR_SECURITY_PLUGIN_NAME "NWK Creator Security"

// -----------------------------------------------------------------------------
// API

/** @brief Start
 *
 * This API initializes the security needed for forming and then operating on
 * a network. The centralizedNetwork parameter allows the caller to specify
 * whether or not the network that they plan to form will use centralized or
 * distributed security.
 *
 * @param centralizedNetwork Whether or not the network that the caller plans
 * to form will use centralized or distributed security.
 *
 * @return Status of the commencement of the network creator process.
 */
EmberStatus emberAfPluginNetworkCreatorSecurityStart(bool centralizedNetwork);

/** @brief Open Network
 *
 * This API will open a network for joining. It broadcasts a permit join to
 * the network, as well as adds a transient link key of ZigBeeAlliance09
 * if this device is a trust center.
 *
 * @return An ::EmberStatus value describing the success or failure of the
 * network opening procedure. If this node is not currently on a network, then
 * this will return ::EMBER_ERR_FATAL.
 */
EmberStatus emberAfPluginNetworkCreatorSecurityOpenNetwork(void);

/** @brief Close Network
 *
 * This API will close the network for joining. It broadcasts a permit join
 * to the network with time 0, as well as clears any transient link keys in
 * the stack.
 *
 * @return An ::EmberStatus value describing closing the network. If this node
 * is not currently on a network, then this will return ::EMBER_ERR_FATAL. This
 * API will also return an error code based on the success or failure of the
 * broadcasted permit join.
 */
EmberStatus emberAfPluginNetworkCreatorSecurityCloseNetwork(void);
