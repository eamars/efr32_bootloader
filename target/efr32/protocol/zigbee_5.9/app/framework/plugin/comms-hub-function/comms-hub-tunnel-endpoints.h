
#ifndef _COMMS_HUB_TUNNEL_ENDPOINTS_H_
#define _COMMS_HUB_TUNNEL_ENDPOINTS_H_

#define INVALID_TUNNELING_ENDPOINT  0xFF


/**
 * @brief Tunneling Endpoint Init
 *
 * This function initializes the table of device tunnel endpoints.
 *
 */
void emberAfPluginTunnelingEndpointInit( void );


/**
 * @brief Add Tunneling Endpoint
 *
 * This function adds an address and tunnel endpoint into the table.
 *
 * @param nodeId The address of the device that should be added to the table.
 * @param endpointList The list of tunneling endpoints on the device.
 * @param numEndpoints The number of tunneling endpoints on the device.
 *
 */
void emberAfPluginAddTunnelingEndpoint( uint16_t nodeId, uint8_t *endpointList, uint8_t numEndpoints );


/**
 * @brief Get Tunneling Endpoint
 *
 * This function returns the tunneling endpoint for the specified nodeId.  If an endpoint
 * for the nodeId cannot be found, it returns INVALID_TUNNELING_ENDPOINT.
 *
 * @return The tunneling endpoint for the nodeId, or INVALID_TUNNELING_ENDPOINT.
 *
 */
uint8_t emberAfPluginGetDeviceTunnelingEndpoint( uint16_t nodeId );


#endif  // #ifndef _COMMS_HUB_TUNNEL_ENDPOINTS_H_


