// Copyright 2014 Silicon Laboratories, Inc.

#ifndef __RF4CE_ZRC11_H__
#define __RF4CE_ZRC11_H__

#include "rf4ce-zrc11-types.h"

/**
 * @addtogroup rf4ce-zrc11
 *
 * The ZigBee Remote Control 1.1 (ZRC1.1) plugin implements the ZRC1.1 profile.
 * The profile describes a simple discovery and pairing procedure for joining
 * devices and a simple user control procedure for controlling devices via
 * HDMI-CEC UI commands.  The plugin manages these procedures for both
 * originators and recipients.  Note that this plugin implements version 1.1 of
 * the ZRC profile.  Version 1.1 devices are fully interoperable with version
 * 1.0 devices.  (ZRC1.0 was also known as the Consumer Electronics Remote
 * Control (CERC) profile.)  As required by the specification, this plugin
 * sends ZRC1.1-formatted messages and receives both ZRC1.0- and ZRC1.1-
 * formatted messages.
 *
 * This plugin supports originators and recipients.  Originators send user
 * control messages to recipients after pairing with them.  Originators are
 * most typically controllers but may also be targets.  Recipients must be
 * targets.  Both originators and recipients must start general network
 * operations before beginning ZRC-specific operations.  Network operations
 * should be started by calling ::emberAfRf4ceStart in the @ref rf4ce-profile
 * plugin.
 *
 * Once network operations have started, ZRC1.1 originators can initiate the
 * discovery and pairing procedure by calling ::emberAfRf4ceZrc11Discovery with
 * the appropriate discovery parameters.  The plugin will first perform
 * discovery for matching targets in range.  If exactly one target is
 * identified, the plugin will then continue to the pairing procedure.  At the
 * conclusion of the discovery and pairing procedure, the plugin will call
 * ::emberAfPluginRf4ceZrc11PairingCompleteCallback to indicate whether pairing
 * completed successfully with a target and, if so, which pairing index has
 * been assigned to that target.
 *
 * Recipients can initiate the discovery and pairing procedure by calling
 * ::emberAfRf4ceZrc11EnableAutoDiscoveryResponse.  This will put the node into
 * auto discovery mode for a fixed duration of time.  During this time, the
 * stack will automatically respond to discovery requests from originators that
 * match the configuration of this device.  If an originator discovers this
 * device, the plugin will then wait for a pair request from the same
 * originator.  When the process completes, the plugin will call
 * ::emberAfPluginRf4ceZrc11PairingCompleteCallback to indicate whether pairing
 * completed successfully with an originator and, if so, which pairing index
 * has been assigned to that originator.
 *
 * Following a successful discovery and pairing, originators may send user
 * control messages to targets by calling ::emberAfRf4ceZrc11UserControlPress.
 * In response, the plugin will transmit a user control press message to the
 * indicated target with the HDMI-CEC command code and payload.  If the press
 * is atomic, the plugin performs no additional processing.  Otherwise, the
 * plugin will repeatedly transmit user control repeat messages at fixed
 * intervals until ::emberAfRf4ceZrc11UserControlRelease is called.  The
 * interval at which user control repeat messages are transmitted is
 * configurable in the plugin options.
 *
 * For targets, the plugin will keep track of incoming user control messages.
 * Each time a user control press command is received, the plugin will call
 * ::emberAfPluginRf4ceZrc11UserControlCallback with its HDMI-CEC command code
 * and payload.  The plugin will then wait for a fixed duration for a
 * corresponding user control repeat or release messages.  If a user control
 * repeat message is received within the timeout, the plugin will reset its
 * timer and wait for the next message.  If a user control repeat message is
 * not received within the timeout or if a user control release is received,
 * the plugin will call ::emberAfPluginRf4ceZrc11UserControlCallback with an
 * indication that the user control has stopped.  The timeout for receiving
 * repeat messages is configurable in the plugin options.  Note that the plugin
 * will not call ::emberAfPluginRf4ceZrc11UserControlCallback for repeat
 * messages that follow a press message.
 *
 * The plugin is capable of keeping track of a fixed number of simultaneous
 * incoming and outgoing user control messages.  The limits are configurable in
 * the plugin options.
 *
 * Originators and recipients may each perform command discovery to determine
 * which user control codes are supported by the paired node.  This is done by
 * calling ::emberAfRf4ceZrc11CommandDiscoveryRequest.  Once command discovery
 * completes, the plugin calls
 * ::emberAfPluginRf4ceZrc11CommandDiscoveryResponseCallback with the results.
 * If this node receives a command discovery request, it will automatically
 * respond with the list of HDMI-CEC command codes that the device supports.
 * The supported command codes are configurable in AppBuilder.
 *
 * This plugin manages the state of the receiver by calling
 * ::emberAfRf4ceRxEnable using ::EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_1_1 as
 * the profile id.  If the application also wishes to manage the receiver, it
 * should do so using ::EMBER_AF_RF4CE_PROFILE_WILDCARD as the profile id or by
 * calling ::emberAfRf4ceSetPowerSavingParameters.
 *
 * This plugin utilizes the discovery, pairing, sending and receiving, and
 * power-saving functionality provided by the @ref rf4ce-profile plugin.
 * @{
 */

#ifdef EMBER_AF_RF4CE_ZRC_IS_USER_CONTROL_ORIGINATOR
  /** @brief Set if the device is configured as a ZRC originator. */
  #define EMBER_AF_RF4CE_ZRC_IS_ORIGINATOR
#endif

#ifdef EMBER_AF_RF4CE_ZRC_IS_USER_CONTROL_RECIPIENT
  /** @brief Set if the device is configured as a ZRC recipient. */
  #define EMBER_AF_RF4CE_ZRC_IS_RECIPIENT
#endif

/** @brief Initiate the pairing procedure for an originator.
 *
 * The plugin begins the pairing procedure by searching for a potential target
 * with which to pair.  If exactly one target is found, the plugin will attempt
 * to pair with it.  If no targets or more than one target is found, the
 * pairing will fail, as required by the ZRC1.x specification.  The plugin will
 * call ::emberAfPluginRf4ceZrc11PairingCompleteCallback with the results of
 * the pairing procedure.
 *
 * @param panId The pan id to search or ::EMBER_RF4CE_BROADCAST_PAN_ID.
 * @param nodeId The node id for which to search or
 * ::EMBER_RF4CE_BROADCAST_ADDRESS.
 * @param searchDevType The device type for which to search or
 * ::EMBER_AF_RF4CE_DEVICE_TYPE_WILDCARD.
 *
 * @return An ::EmberStatus value that indicates the success or failure of the
 * command.
 */
EmberStatus emberAfRf4ceZrc11Discovery(EmberPanId panId,
                                       EmberNodeId nodeId,
                                       EmberAfRf4ceDeviceType searchDevType);

/** @brief Initiate the pairing procedure for a recipient.
 *
 * The plugin performs the binding procedure by enabling auto discovery mode.
 * The plugin will call ::emberAfPluginRf4ceZrc11PairingCompleteCallback with
 * the results of the pairing.  This function may only be called on a target
 * device.
 *
 * @return An ::EmberStatus value that indicates the success or failure of the
 * command.
 */
EmberStatus emberAfRf4ceZrc11EnableAutoDiscoveryResponse(void);

/** @brief Send indications of user control presses and repeats to a pairing
 * index.
 *
 * This function can be called when a user control has been pressed and an
 * indication of this should be sent to a target.  If the user control should
 * be repeated, the plugin will automatically send user control repeat messages
 * at fixed intervals according to the plugin configuration.  Every call to
 * this function for a repeatable user control should be followed by a call to
 * ::emberAfRf4ceZrc11UserControlRelease.
 *
 * @param pairingIndex The pairing index to which to send user control
 * messages.
 * @param rcCommandCode The RC command code of the user control.
 * @param rcCommandPayload The optional RC command payload of the user control.
 * @param rcCommandPayloadLength The length of the optional RC command payload
 * of the user control.
 * @param atomic true if the user control is atomic or false if it should
 * repeat.
 *
 * @return An ::EmberStatus value that indicates the success or failure of the
 * command.
 */
EmberStatus emberAfRf4ceZrc11UserControlPress(uint8_t pairingIndex,
                                              EmberAfRf4ceUserControlCode rcCommandCode,
                                              const uint8_t *rcCommandPayload,
                                              uint8_t rcCommandPayloadLength,
                                              bool atomic);

/** @brief Send indications of user control release to a pairing index.
 *
 * This function can be called when a user control has been released and an
 * indication of this should be sent to a remote node.
 *
 * @param pairingIndex The pairing index to which to send user control
 * messages.
 * @param rcCommandCode The RC command code of the user control.
 *
 * @return An ::EmberStatus value that indicates the success or failure of the
 * command.
 */
EmberStatus emberAfRf4ceZrc11UserControlRelease(uint8_t pairingIndex,
                                                EmberAfRf4ceUserControlCode rcCommandCode);

/** @brief Send a command discovery request to a pairing index.
 *
 * This function can be called to discover which commands are supported by a
 * remote node.  The plugin will call
 * ::emberAfPluginRf4ceZrc11CommandDiscoveryResponseCallback when the response
 * is received.
 *
 * @param pairingIndex The pairing index to which to the request.
 *
 * @return An ::EmberStatus value that indicates the success or failure of the
 * command.
 */
EmberStatus emberAfRf4ceZrc11CommandDiscoveryRequest(uint8_t pairingIndex);

/** @brief Accesses the actual command discovery data of the
 * ::EmberAfRf4ceZrcCommandsSupported structure.
 *
 * @param tag A pointer to an ::EmberAfRf4ceZrcCommandsSupported structure.
 *
 * @return Returns a pointer to the first byte of the command discovery value.
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
  uint8_t *emberAfRf4ceZrcCommandsSupportedContents(EmberAfRf4ceZrcCommandsSupported *commandsSupported);
#else
  #define emberAfRf4ceZrcCommandsSupportedContents(commandsSupported) \
    ((commandsSupported)->contents)
#endif

#endif // __RF4CE_ZRC11_H__

// @} END addtogroup
