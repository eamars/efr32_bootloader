// Copyright 2014 Silicon Laboratories, Inc.

#ifndef __RF4CE_GDP_H__
#define __RF4CE_GDP_H__

#include "rf4ce-gdp-types.h"

/**
 * @addtogroup rf4ce-gdp
 *
 * The Generic Device Profile (GDP) plugin provides APIs to service the different
 * RF4CE profiles.
 *
 * The plugin offers APIs to add in application level customization to the
 * RF4CE GDP commissioning mechanism. One can initiate this
 * commissioning process with ::emberAfRf4ceGdpBind or
 * ::emberAfRf4ceGdpProxyBind. The application can use
 * ::emberAfRf4ceGdpConfigurationComplete to learn the result of
 * this binding operation.
 *
 * One can configure basic security operations of an RF4CE network using this
 * plugin. The plugin offers options Enhanced security, Standard shared secret,
 * and Vendor-specific shared secrets, so that the user can dictate the level
 * of security at which they would like this plugin to operate. During the
 * commissioning process, or any time after, the application can use
 * ::emberAfRf4ceGdpInitiateKeyExchange to force a key exchange to take place.
 *
 * This plugin also gives the application the ability to interact with RF4CE
 * attributes. There are APIs provided to perform the necessary GDP attribute
 * operations over the air.
 *
 * @{
 */

// Controllers and targets can be originators in GDP, but only targets can be
// recipients.
/** @brief Set if the RF4CE Profile plugin was configured as a controller or
 * target.
 */
#define EMBER_AF_PLUGIN_RF4CE_GDP_IS_ORIGINATOR
#ifdef EMBER_AF_RF4CE_NODE_TYPE_TARGET
  /** @brief Set if the RF4CE Profile plugin was configured as a target. */
  #define EMBER_AF_PLUGIN_RF4CE_GDP_IS_RECIPIENT
#endif

#define POLL_CLIENT 1
#define POLL_SERVER 2

#if EMBER_AF_PLUGIN_RF4CE_GDP_POLL_SUPPORT == POLL_CLIENT
  /** @brief Set if the device is configured as a poll client. */
  #define EMBER_AF_PLUGIN_RF4CE_GDP_IS_POLL_CLIENT
#elif EMBER_AF_PLUGIN_RF4CE_GDP_POLL_SUPPORT == POLL_SERVER
  /** @brief Set if the device is configured as a poll server. */
  #define EMBER_AF_PLUGIN_RF4CE_GDP_IS_POLL_SERVER
#endif

#define IDENTIFICATION_CLIENT 1
#define IDENTIFICATION_SERVER 2

#if EMBER_AF_PLUGIN_RF4CE_GDP_IDENTIFICATION_SUPPORT == IDENTIFICATION_CLIENT
  /** @brief Set if the device is configured as an identification client. */
  #define EMBER_AF_PLUGIN_RF4CE_GDP_IS_IDENTIFICATION_CLIENT
#elif EMBER_AF_PLUGIN_RF4CE_GDP_IDENTIFICATION_SUPPORT == IDENTIFICATION_SERVER
  /** @brief Set if the device is configured as an identification server. */
  #define EMBER_AF_PLUGIN_RF4CE_GDP_IS_IDENTIFICATION_SERVER
#endif

/** @brief Accesses the actual random byte string of the ::EmberAfRf4ceGdpRand
 * structure.
 *
 * @param rand A pointer to an ::EmberAfRf4ceGdpRand structure.
 *
 * @return uint8_t* Returns a pointer to the first byte of the random byte string.
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
uint8_t *emberAfRf4ceGdpRandContents(EmberAfRf4ceGdpRand *rand);
#else
#define emberAfRf4ceGdpRandContents(rand) ((rand)->contents)
#endif

/** @brief Accesses the actual tag value of the ::EmberAfRf4ceGdpTag structure.
 *
 * @param tag A pointer to an ::EmberAfRf4ceGdpTag structure.
 *
 * @return uint8_t* Returns a pointer to the first byte of the tag value.
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
uint8_t *emberAfRf4ceGdpTagContents(EmberAfRf4ceGdpTag *tag);
#else
#define emberAfRf4ceGdpTagContents(tag) ((tag)->contents)
#endif

/** @brief  Initiates the binding process.
 *
 * @param  profileIdList    The list of profile IDs supported by the node.
 *
 * @param  profileIdListLength   The size of the profile ID list.
 *
 * @param  searchDevType  The device type the node will be matching against
 * during the preliminary discovery process.
 *
 * @return  An ::EmberStatus value indicating whether the binding process was
 * successfully initiated or the reason of failure.
 */
EmberStatus emberAfRf4ceGdpBind(uint8_t *profileIdList,
                                uint8_t profileIdListLength,
                                uint8_t searchDevType);
/** @brief  Initiates the proxy binding process.
 *
 * @param  panId  The pan ID of the recipient.
 *
 * @param  ieeeAddr   The IEEE address of the recipient.
 *
 * @param  profileIdList   The list of profile IDs supported by the node.
 *
 * @param  profileIdListLength   The size of the profile ID list.
 *
 * @return  An ::EmberStatus value indicating whether the proxy binding process
 * was successfully initiated or the reason of failure.
 */
EmberStatus emberAfRf4ceGdpProxyBind(EmberPanId panId,
                                     EmberEUI64 ieeeAddr,
                                     uint8_t *profileIdList,
                                     uint8_t profileIdListLength);

/** @brief  It notifies the GDP profile that a profile-specific configuration
 * procedure has completed.
 *
 * @param  success  Indicates whether the profile-specific configuration
 * procedure completed successfully or not.
 */
void emberAfRf4ceGdpConfigurationProcedureComplete(bool success);

/** @brief It sets or clears the push button stimulus pending flag at the GDP
 * recipient.
 *
 * @param  The push button stimulus pending flag.
 */
void emberAfRf4ceGdpPushButton(bool setPending);

/** @brief  It sets the validation status at the GDP recipient.
 *
 * @param status  The validation status.
 */
void emberAfRf4ceGdpSetValidationStatus(EmberAfRf4ceGdpCheckValidationStatus status);

/** @brief  It kicks off the extended key exchange procedure at the GDP
 * originator or recipient.
 *
 * @param   The pairing index for which the key exchange procedure should be
 * executed.
 */
EmberStatus emberAfRf4ceGdpInitiateKeyExchange(uint8_t pairingIndex);

/** @brief  It polls the poll server by sending out a GDP heartbeat command to
 * the poll server.
 *
 * @param  pairingIndex   The pairing index of the poll server.
 *
 * @param  vendorId   The vendor ID to be included in the heartbeat command.
 *
 * @param  trigger   The heartbeat trigger to be included in the heartbeat
 * command.
 *
 * @return  Indicates whether the heartbeat command was successfully sent out
 * or the reason of failure.
 */
EmberStatus emberAfRf4ceGdpPoll(uint8_t pairingIndex,
                                uint16_t vendorId,
                                EmberAfRf4ceGdpHeartbeatTrigger trigger);

/** @brief Submit a message sent indication to the GDP plugin for any
 * applicable GDP-specific processing.
 *
 * The application should submit all message sent indications for profiles that
 * use GDP to this plugin before performing any profile-specific processing.
 * Profile-specific processing should only occur if this API returns false.
 *
 * @return true if the message sent indication was for a GDP command and has
 * been handled by the GDP plugin and should therefore not be processed further
 * by the application or false if the message sent indication was not for a GDP
 * command and the application should perform profile-specific processing.
 */
bool emberAfRf4ceGdpMessageSent(uint8_t pairingIndex,
                                   uint8_t profileId,
                                   uint16_t vendorId,
                                   const uint8_t *message,
                                   uint8_t messageLength,
                                   EmberStatus status);

/** @brief Submit an incoming message to the GDP plugin for any applicable GDP-
 * specific processing.
 *
 * The application should submit all incoming messages for profiles that use
 * GDP to this plugin before performing any profile-specific processing.
 * Profile-specific processing should only occur if this API returns false.
 *
 * @return true if the incoming message was a GDP command and has been handled
 * by the GDP plugin and should therefore not be processed further by the
 * application or false if the incoming message was not a GDP command and the
 * application should perform profile-specific processing.
 */
bool emberAfRf4ceGdpIncomingMessage(uint8_t pairingIndex,
                                       uint8_t profileId,
                                       uint16_t vendorId,
                                       EmberRf4ceTxOption secured,
                                       const uint8_t *message,
                                       uint8_t messageLength);

/** @brief Allows the application to retrieve the RF4CE TX options used by
 * GDP for a specific command, pairing index and vendor ID.
 *
 * @return An ::EmberStatus indicating success or the reason of failure.
 */
EmberStatus emberAfRf4ceGdpGetCommandTxOptions(EmberAfRf4ceGdpCommandCode commandCode,
                                               uint8_t pairingIndex,
                                               uint16_t vendorId,
                                               EmberRf4ceTxOption *txOptions);

/** @brief  It sends out a GDP generic response command.
 *
 * @param  paringIndex   The pairing index the generic response  command should
 * be sent to.
 *
 * @param  profileId    The profile ID to be included in the generic response
 * command.
 *
 * @param  vendorId   The vendor ID to be included in the generic response
 * command.
 *
 * @param  responseCode   The response code to be included in the generic
 * response command.
 *
 * @return  An ::EmberStatus value indicating whether the generic response
 * command was successfully sent out or the reason of failure.
 */
EmberStatus emberAfRf4ceGdpGenericResponse(uint8_t pairingIndex,
                                           uint8_t profileId,
                                           uint16_t vendorId,
                                           EmberAfRf4ceGdpResponseCode responseCode);

/** @brief  It sends out a GDP configuration complete command.
 *
 * @param  paringIndex   The pairing index the configuration complete command
 * should be sent to.
 *
 * @param  profileId    The profile ID to be included in the configuration
 * complete command.
 *
 * @param  vendorId   The vendor ID to be included in the configuration
 * complete command.
 *
 * @param  status  The status code to be included in the configuration
 * complete command.
 *
 * @return  An ::EmberStatus value indicating whether the configuration complete
 * command was successfully sent out or the reason of failure.
 */
EmberStatus emberAfRf4ceGdpConfigurationComplete(uint8_t pairingIndex,
                                                 uint8_t profileId,
                                                 uint16_t vendorId,
                                                 EmberAfRf4ceGdpStatus status);

/** @brief  It sends out a GDP hearbeat command.
 *
 * @param  paringIndex   The pairing index the hearbeat command should be sent
 * to.
 *
 * @param  vendorId   The vendor ID to be included in the hearbeat command.
 *
 * @param  trigger  The trigger code to be included in the hearbeat command.
 *
 * @return  An ::EmberStatus value indicating whether the hearbeat command was
 * successfully sent out or the reason of failure.
 */
EmberStatus emberAfRf4ceGdpHeartbeat(uint8_t pairingIndex,
                                     uint16_t vendorId,
                                     EmberAfRf4ceGdpHeartbeatTrigger trigger);

/** @brief  It sends out a GDP get attributes command.
 *
 * @param  paringIndex   The pairing index the get attributes command should be
 * sent to.
 *
 * @param  profileId    The profile ID to be included in the get attributes
 * command.
 *
 * @param  vendorId   The vendor ID to be included in the get attributes
 * command.
 *
 * @param  records  A list of ::EmberAfRf4ceGdpAttributeIdentificationRecord
 * to be included in the get attributes command.
 *
 * @param  recordsLength  The size of the attribute identification records list.
 *
 * @return  An ::EmberStatus value indicating whether the get attributes command
 * was successfully sent out or the reason of failure.
 */
EmberStatus emberAfRf4ceGdpGetAttributes(uint8_t pairingIndex,
                                         uint8_t profileId,
                                         uint16_t vendorId,
                                         const EmberAfRf4ceGdpAttributeIdentificationRecord *records,
                                         uint8_t recordsLength);

/** @brief  It sends out a GDP get attributes response command.
 *
 * @param  paringIndex   The pairing index the get attributes response command
 * should be sent to.
 *
 * @param  profileId    The profile ID to be included in the get attributes
 * response command.
 *
 * @param  vendorId   The vendor ID to be included in the get attributes
 * response command.
 *
 * @param  records  A list of ::EmberAfRf4ceGdpAttributeStatusRecord
 * to be included in the get attributes response command.
 *
 * @param  recordsLength  The size of the attribute status records list.
 *
 * @return  An ::EmberStatus value indicating whether the get attributes
 * response command was successfully sent out or the reason of failure.
 */
EmberStatus emberAfRf4ceGdpGetAttributesResponse(uint8_t pairingIndex,
                                                 uint8_t profileId,
                                                 uint16_t vendorId,
                                                 const EmberAfRf4ceGdpAttributeStatusRecord *records,
                                                 uint8_t recordsLength);

/** @brief  It sends out a GDP push attributes command.
 *
 * @param  paringIndex   The pairing index the push attributes command should be
 * sent to.
 *
 * @param  profileId    The profile ID to be included in the push attributes
 * command.
 *
 * @param  vendorId   The vendor ID to be included in the push attributes
 * command.
 *
 * @param  records  A list of ::EmberAfRf4ceGdpAttributeRecord
 * to be included in the push attributes command.
 *
 * @param  recordsLength  The size of the attribute records list.
 *
 * @return  An ::EmberStatus value indicating whether the push attributes
 * command was successfully sent out or the reason of failure.
 */
EmberStatus emberAfRf4ceGdpPushAttributes(uint8_t pairingIndex,
                                          uint8_t profileId,
                                          uint16_t vendorId,
                                          const EmberAfRf4ceGdpAttributeRecord *records,
                                          uint8_t recordsLength);

/** @brief  It sends out a GDP set attributes command.
 *
 * @param  paringIndex   The pairing index the set attributes command should be
 * sent to.
 *
 * @param  profileId    The profile ID to be included in the set attributes
 * command.
 *
 * @param  vendorId   The vendor ID to be included in the set attributes
 * command.
 *
 * @param  records  A list of ::EmberAfRf4ceGdpAttributeRecord
 * to be included in the set attributes command.
 *
 * @param  recordsLength  The size of the attribute records list.
 *
 * @return  An ::EmberStatus value indicating whether the set attributes
 * command was successfully sent out or the reason of failure.
 */
EmberStatus emberAfRf4ceGdpSetAttributes(uint8_t pairingIndex,
                                         uint8_t profileId,
                                         uint16_t vendorId,
                                         const EmberAfRf4ceGdpAttributeRecord *records,
                                         uint8_t recordsLength);

/** @brief  It sends out a GDP pull attributes command.
 *
 * @param  paringIndex   The pairing index the pull attributes command should be
 * sent to.
 *
 * @param  profileId    The profile ID to be included in the pull attributes
 * command.
 *
 * @param  vendorId   The vendor ID to be included in the pull attributes
 * command.
 *
 * @param  records  A list of ::EmberAfRf4ceGdpAttributeIdentificationRecord
 * to be included in the pull attributes command.
 *
 * @param  recordsLength  The size of the attribute identification records list.
 *
 * @return  An ::EmberStatus value indicating whether the pull attributes
 * command was successfully sent out or the reason of failure.
 */
EmberStatus emberAfRf4ceGdpPullAttributes(uint8_t pairingIndex,
                                          uint8_t profileId,
                                          uint16_t vendorId,
                                          const EmberAfRf4ceGdpAttributeIdentificationRecord *records,
                                          uint8_t recordsLength);

/** @brief  It sends out a GDP pull attributes response command.
 *
 * @param  paringIndex   The pairing index the pull attributes response command
 * should be sent to.
 *
 * @param  profileId    The profile ID to be included in the pull attributes
 * response command.
 *
 * @param  vendorId   The vendor ID to be included in the pull attributes
 * response command.
 *
 * @param  records  A list of ::EmberAfRf4ceGdpAttributeStatusRecord
 * to be included in the pull attributes response command.
 *
 * @param  recordsLength  The size of the attribute status records list.
 *
 * @return  An ::EmberStatus value indicating whether the pull attributes
 * response command was successfully sent out or the reason of failure.
 */
EmberStatus emberAfRf4ceGdpPullAttributesResponse(uint8_t pairingIndex,
                                                  uint8_t profileId,
                                                  uint16_t vendorId,
                                                  const EmberAfRf4ceGdpAttributeStatusRecord *records,
                                                  uint8_t recordsLength);

/** @brief  It sends out a GDP check validation request command.
 *
 * @param  paringIndex   The pairing index the check validation request command
 * should be sent to.
 *
 * @param  vendorId   The vendor ID to be included in the check validation
 * request command.
 *
 * @param  control  The control field to be included in the check validation
 * request command.
 *
 * @return  An ::EmberStatus value indicating whether the check validation
 * request command was successfully sent out or the reason of failure.
 */
EmberStatus emberAfRf4ceGdpCheckValidationRequest(uint8_t pairingIndex,
                                                  uint16_t vendorId,
                                                  uint8_t control);

/** @brief  It sends out a GDP check validation response command.
 *
 * @param  paringIndex   The pairing index the check validation response command
 * should be sent to.
 *
 * @param  vendorId   The vendor ID to be included in the check validation
 * response command.
 *
 * @param  status  The status code to be included in the check validation
 * response command.
 *
 * @return  An ::EmberStatus value indicating whether the check validation
 * response command was successfully sent out or the reason of failure.
 */
EmberStatus emberAfRf4ceGdpCheckValidationResponse(uint8_t pairingIndex,
                                                   uint16_t vendorId,
                                                   EmberAfRf4ceGdpCheckValidationStatus status);

/** @brief  It sends out a GDP client notification identify command.
 *
 * @param  paringIndex   The pairing index the client notification identify
 * command should be sent to.
 *
 * @param  vendorId   The vendor ID to be included in the client notification
 * identify command.
 *
 * @param  flags  The flags field to be included in the client notification
 * identify command.
 *
 * @param  timeS  The time field in seconds to be included in the client
 * notification  identify command.
 *
 * @return  An ::EmberStatus value indicating whether the client notification
 * identify command was successfully sent out or the reason of failure.
 */
EmberStatus emberAfRf4ceGdpClientNotificationIdentify(uint8_t pairingIndex,
                                                      uint16_t vendorId,
                                                      EmberAfRf4ceGdpClientNotificationIdentifyFlags flags,
                                                      uint16_t timeS);

/** @brief  It sends out a GDP client notification request poll negotiation
 * command.
 *
 * @param  paringIndex   The pairing index the client notification request poll
 * negotiation command should be sent to.
 *
 * @param  vendorId   The vendor ID to be included in the client notification
 * request poll negotiation command.
 *
 * @return  An ::EmberStatus value indicating whether the client notification
 * request poll negotiation command was successfully sent out or the reason of
 * failure.
 */
EmberStatus emberAfRf4ceGdpClientNotificationRequestPollNegotiation(uint8_t pairingIndex,
                                                                    uint16_t vendorId);

/** @brief  It sends out a generic GDP client notification command.
 *
 * @param  paringIndex   The pairing index the client notification command
 * should be sent to.
 *
 * @param  vendorId   The vendor ID to be included in the client notification
 * command.
 *
 * @param  subType   The sub-type field of the client notification command.
 *
 * @param  payload   The payload to be included in the client notification
 * command.
 *
 * @param  payloadLength   The length in bytes of the payload.
 *
 * @return  An ::EmberStatus value indicating whether the client notification
 * command was successfully sent out or the reason of failure.
 */
EmberStatus emberAfRf4ceGdpClientNotification(uint8_t pairingIndex,
                                              uint8_t profileId,
                                              uint16_t vendorId,
                                              uint8_t subtype,
                                              const uint8_t *payload,
                                              uint8_t payloadLength);

/** @brief  It allows a software module to subscribe to incoming heartbeat
 * commands.
 *
 * @param  callback   The callback to be called by the poll server upon
 * receiving a heartbeat command.
 *
 * @return  An ::EmberStatus value of ::EMBER_SUCCESS if the subscription
 * process succeeded.  An ::EmberStatus value of ::EMBER_TABLE_FULL if there is
 * no room in the subscription table. An ::EmberStatus of ::EMBER_INVALID_CALL
 * if the node is not a poll server.
 */
EmberStatus emberAfRf4ceGdpSubscribeToHeartbeat(EmberAfRf4ceGdpHeartbeatCallback callback);

#endif // __RF4CE_GDP_H__

// @} END addtogroup
