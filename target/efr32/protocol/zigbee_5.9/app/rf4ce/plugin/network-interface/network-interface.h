// Copyright 2014 Silicon Laboratories, Inc.

/** @brief Get the EUI64 (IEEE address) of the local node.
 *
 * The function is a convenience wrapper for ::emberGetEui64 and
 * ::ezspGetEui64.
 *
 * @param resultLocation A pointer to an ::EmberEUI64 value into which the
 * local EUI64 will be copied.
 */
void emberAfGetEui64(EmberEUI64 returnEui64);

/** @brief Get the type and network parameters of the local node.
 *
 * The function is a convenience wrapper for ::emberGetNetworkParameters,
 * ::emberGetNodeType, and ::ezspGetNetworkParameters.
 *
 * @param nodeType A pointer to an ::EmberNodeType value into which the current
 * node type will be copied.
 * @param parameters A pointer to an ::EmberNetworkParameters value into which
 * the current network parameters will be copied.
 *
 * @return An ::EmberStatus value indicating the success or failure of the
 * command.
 */
EmberStatus emberAfGetNetworkParameters(EmberNodeType *nodeType,
                                        EmberNetworkParameters *parameters);

/** @brief Get the 16-bit network id of the local node.
 *
 * The function is a convenience wrapper for ::emberGetNodeId and
 * ::ezspGetNodeId.
 *
 * @return The network id.
 */
EmberNodeId emberAfGetNodeId(void);

/** @brief Get the type of the local node.
 *
 * The function is a convenience wrapper for ::emberGetNodeType and
 * ::ezspGetNetworkParameters.
 *
 * @param nodeType A pointer to an ::EmberNodeType value into which the current
 * node type will be copied.
 *
 * @return An ::EmberStatus value indicating the success or failure of the
 * command.
 */
EmberStatus emberAfGetNodeType(EmberNodeType *nodeType);

/** @brief Get the 16-bit PAN id of the local node.
 *
 * The function is a convenience wrapper for ::emberGetPanId and
 * ::ezspGetNetworkParameters.
 *
 * @return The PAN id.
 */
EmberPanId emberAfGetPanId(void);

/** @brief Resume network operation after a reset.
 *
 * The function is a convenience wrapper for ::emberNetworkInit and
 * ::ezspNetworkInit.
 *
 * @return An ::EmberStatus value that indicates the success or failure of the
 * command.
 */
EmberStatus emberAfNetworkInit(void);
