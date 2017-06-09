// Copyright 2014 Silicon Laboratories, Inc.

#ifndef __RF4CE_MSO_H__
#define __RF4CE_MSO_H__

#include "rf4ce-mso-types.h"

/**
 * @addtogroup rf4ce-mso
 *
 * The RF4CE Multiple System Operators (MSO) plugin implements the MSO profile.
 * The profile uses the simple user control procedure from the ZigBee Remote
 * Control (ZRC) profile with a more robust discovery, pairing, and validating
 * procedure for binding devices.  The plugin manages these procedures for both
 * originators and recipients.
 *
 * This plugin supports originators and recipients.  Originators send action
 * messages to recipients after binding with them.  Unlike ZRC, originators
 * must be controllers.  Recipients must be targets, as in ZRC.  Both
 * controllers and targets must start general network operations before
 * beginning MSO-specific operations.  Network operations should be started by
 * calling ::emberAfRf4ceStart in the @ref rf4ce-profile plugin.
 *
 * Once network operations have started, controllers can initiate the binding
 * procedure by calling ::emberAfRf4ceMsoBind.  The plugin will perform
 * discovery for matching targets in range.  Potential targets are ranked
 * according to an algorithm described in the MSO specification.  If one or
 * more potential targets are identified, the plugin will attempt to pair with
 * the highest-ranked target.  Once the temporary pairing is established, the
 * validation procedure begins.  Validation is implementation-specific, but may
 * be as simple as a button press on the target or a more involved challenge-
 * response mechanism between target and originator.  Only if validation is
 * successful are the controller and target considered bound.  If any step
 * fails, the plugin will restart the binding procedure using the target that
 * has the next-highest rank.  At the conclusion of the binding procedure, the
 * plugin will call ::emberAfPluginRf4ceMsoBindingCompleteCallback to indicate
 * whether binding completed successfully with a target and, if so, which
 * pairing index has been assigned to that target.
 *
 * The plugin manages discovery and pairing for targets on behalf of the
 * device.  If pairing completes successfully, the plugin will call
 * ::emberAfPluginRf4ceMsoStartValidationCallback so that the device can begin
 * the implementation-specific validation procedure.  During validation, the
 * application must periodically call ::emberAfRf4ceMsoWatchdogKick to prevent
 * a validation timeout.  Once the application determines the controller has
 * validated successfully, it should call ::emberAfRf4ceMsoValidate.  If the
 * controller fails validation, the application may instead call
 * ::emberAfRf4ceMsoTerminateValidation or ::emberAfRf4ceMsoAbortValidation.
 * Once binding completes, the plugin will call
 * ::emberAfPluginRf4ceMsoBindingCompleteCallback to indicate whether binding
 * completed successfully with a controller and, if so, which pairing index has
 * been assigned to that controller.
 *
 * Following a successful discovery and pairing, controllers may send user
 * control messages to targets by calling ::emberAfRf4ceMsoUserControlPress.
 * In response, the plugin will transmit a user control press message to the
 * indicated target with the HDMI-CEC command code and payload.  If the press
 * is atomic, the plugin performs no additional processing.  Otherwise, the
 * plugin will repeatedly transmit user control repeat messages at fixed
 * intervals until ::emberAfRf4ceMsoUserControlRelease is called.  The interval
 * at which user control repeat messages are transmitted is configurable in the
 * plugin options.
 *
 * For targets, the plugin will keep track of incoming user control messages.
 * Each time a user control press command is received, the plugin will call
 * ::emberAfPluginRf4ceMsoUserControlCallback with its HDMI-CEC command code
 * and payload.  The plugin will then wait for a fixed duration for a
 * corresponding user control repeat or release messages.  If a user control
 * repeat message is received within the timeout, the plugin will reset its
 * timer and wait for the next message.  If a user control repeat message is
 * not received within the timeout or if a user control release is received,
 * the plugin will call ::emberAfPluginRf4ceMsoUserControlCallback with an
 * indication that the user control has stopped.  The timeout for receiving
 * repeat messages is configurable in the plugin options.  Note that the plugin
 * will not call ::emberAfPluginRf4ceMsoUserControlCallback for repeat messages
 * that follow a press message.
 *
 * The plugin is capable of keeping track of a fixed number of simultaneous
 * incoming and outgoing user control messages.  The limits are configurable in
 * the plugin options.
 *
 * This plugin manages the state of the receiver by calling
 * ::emberAfRf4ceRxEnable using ::EMBER_AF_RF4CE_PROFILE_MSO as the profile id.
 * If the application also wishes to manage the receiver, it should do so using
 * ::EMBER_AF_RF4CE_PROFILE_WILDCARD as the profile id or by calling
 * ::emberAfRf4ceSetPowerSavingParameters.
 *
 * This plugin utilizes the discovery, pairing, sending and receiving, and
 * power-saving functionality provided by the @ref rf4ce-profile plugin.
 * Support for the optional IR-RF database feature is provided by the @ref
 * rf4ce-mso-ir-rf-database-originator and @ref
 * rf4ce-mso-ir-rf-database-recipient plugins.
 * @{
 */

// Only controllers can be originators in MSO and only targets can be
// recipients.
#ifdef EMBER_AF_RF4CE_NODE_TYPE_CONTROLLER
  /** @brief Set if the RF4CE Profile plugin was configured as a controller. */
  #define EMBER_AF_PLUGIN_RF4CE_MSO_IS_ORIGINATOR
#else
  /** @brief Set if the RF4CE Profile plugin was configured as a target. */
  #define EMBER_AF_PLUGIN_RF4CE_MSO_IS_RECIPIENT
#endif

/** @brief Initiate the binding procedure.
 *
 * The plugin begins the binding procedure by searching for and ranking
 * potential targets with which to pair.  If one or more targets is identified,
 * the plugin will create a temporary pairing with the highest-ranked target.
 * Once the temporary pairing completes, the application should perform the
 * required validation procedure.  During this time, the plugin will
 * periodically query the target for the validation status.  If validation
 * completes successfully, the plugin will notify the application by calling
 * ::emberAfPluginRf4ceMsoBindingCompleteCallback.  If the temporary pairing
 * fails or if validation fails, the plugin will attempt to bind to the target
 * with the next-highest rank.  If the plugin fails to bind with any target,
 * it will call ::emberAfPluginRf4ceMsoBindingCompleteCallback with an error.
 *
 * This function is only applicable to target devices.
 *
 * @return An ::EmberStatus value that indicates the success or failure of the
 * command.
 */
EmberStatus emberAfRf4ceMsoBind(void);

/** @brief Kick the watchdog.
 *
 * If the plugin was configured with a non-zero initial watchdog time, this
 * function must be called during validation to prevent the watchdog timer from
 * expiring and triggering a TIMEOUT failure.  The first call should occur
 * within the initial watchdog time of the call to
 * ::emberAfPluginRf4ceMsoStartValidationCallback.  To avoid further TIMEOUTs,
 * each call to this function must either be followed either by a subsequent
 * call within the provided timeout or by a call to
 * ::emberAfRf4ceMsoValidate, ::emberAfRf4ceMsoTerminateValidation,
 * or ::emberAfRf4ceMsoAbortValidation.
 *
 * This function is only applicable to target devices.
 *
 * @param validationWatchdogTimeMs The time in milliseconds to reset the
 * watchdog timer or zero to disable the watchdog timer.
 *
 * @return An ::EmberStatus value that indicates the success or failure of the
 * command.
 */
EmberStatus emberAfRf4ceMsoWatchdogKick(uint16_t validationWatchdogTimeMs);

/** @brief Validate a controller.
 *
 * This function can be called to indicate the validation procedure completed
 * successfully.  It should be called if the controller has performed the
 * required validation procedure satisfactorily.
 *
 * This function is only applicable to target devices.
 *
 * @return An ::EmberStatus value that indicates the success or failure of the
 * command.
 */
EmberStatus emberAfRf4ceMsoValidate(void);

/** @brief Terminate the validation procedure.
 *
 * This function can be called to terminate the validation procedure.  It
 * should be called if the controller fails validation.
 *
 * This function is only applicable to target devices.
 *
 * @return An ::EmberStatus value that indicates the success or failure of the
 * command.
 */
EmberStatus emberAfRf4ceMsoTerminateValidation(void);

/** @brief Abort the validation procedure.
 *
 * This function can be called to abort the validation procedure.  It should be
 * called in response to a controller sending the Abort or FullAbort keys
 * during validation.
 *
 * @param fullAbort true if the controller should not attempt to validate with
 * other controllers or false if the controller should attempt to validate with
 * other controller.
 *
 * This function is only applicable to target devices.
 *
 * @return An ::EmberStatus value that indicates the success or failure of the
 * command.
 */
EmberStatus emberAfRf4ceMsoAbortValidation(bool fullAbort);

/** @brief Send indications of user control presses and repeats to a pairing
 * index.
 *
 * This function can be called when a user control has been pressed and an
 * indication of this should be sent to a remote node.  If the user control
 * should be repeated, the plugin will automatically send user control repeat
 * messages at fixed intervals according to the plugin configuration.  Every
 * call to this function for a repeatable user control should be followed by a
 * call to ::emberAfRf4ceMsoUserControlRelease.
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
 * This function is only applicable to controller devices.
 *
 * @return An ::EmberStatus value that indicates the success or failure of the
 * command.
 */
EmberStatus emberAfRf4ceMsoUserControlPress(uint8_t pairingIndex,
                                            EmberAfRf4ceMsoKeyCode rcCommandCode,
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
 * This function is only applicable to controller devices.
 *
 * @return An ::EmberStatus value that indicates the success or failure of the
 * command.
 */
EmberStatus emberAfRf4ceMsoUserControlRelease(uint8_t pairingIndex,
                                              EmberAfRf4ceMsoKeyCode rcCommandCode);

/** @brief Set an attribute on a remote node.
 *
 * This function can be called to set an attribute on a remote node.  After
 * sending the request, the plugin will automatically idle for a fixed duration
 * as required by the MSO specification and then wait a configurable duration
 * for the response from the target.  The plugin will call
 * ::emberAfPluginRf4ceMsoSetAttributeResponseCallback when the response is
 * received or if a timeout occurs.
 *
 * @param pairingIndex The pairing index on which to set the attribute.
 * @param attributeId The attribute id to set..
 * @param index The index of the element for vector attributes.
 * @param valueLen The length of the value to set.
 * @param value The value to set.
 *
 * This function is only applicable to controller devices.
 *
 * @return An ::EmberStatus value that indicates the success or failure of the
 * command.
 */
EmberStatus emberAfRf4ceMsoSetAttributeRequest(uint8_t pairingIndex,
                                               EmberAfRf4ceMsoAttributeId attributeId,
                                               uint8_t index,
                                               uint8_t valueLen,
                                               const uint8_t *value);

/** @brief Get an attribute on a remote node.
 *
 * This function can be called to get an attribute on a remote node.  After
 * sending the request, the plugin will automatically idle for a fixed duration
 * as required by the MSO specification and then wait a configurable duration
 * for the response from the target.  The plugin will call
 * ::emberAfPluginRf4ceMsoGetAttributeResponseCallback when the response is
 * received or if a timeout occurs.
 *
 * @param pairingIndex The pairing index on which to get the attribute.
 * @param attributeId The attribute id to get.
 * @param index The index of the element for vector attributes.
 * @param valueLen The length of the value to get.
 *
 * This function is only applicable to controller devices.
 *
 * @return An ::EmberStatus value that indicates the success or failure of the
 * command.
 */
EmberStatus emberAfRf4ceMsoGetAttributeRequest(uint8_t pairingIndex,
                                               EmberAfRf4ceMsoAttributeId attributeId,
                                               uint8_t index,
                                               uint8_t valueLen);

#ifdef DOXYGEN_SHOULD_SKIP_THIS
  /** @brief Determine if the IR-RF database entry is the default.
   *
   * Implemented as a macro for efficiency.
   *
   * @return true if the IR-RF database table entry is the default or false
   * otherwise.
   */
  bool emberAfRf4ceMsoIrRfDatabaseEntryUseDefault(const EmberAfRf4ceMsoIrRfDatabaseEntry *entry);
#else
  #define emberAfRf4ceMsoIrRfDatabaseEntryUseDefault(entry)       \
    (READBITS((entry)->flags,                                     \
              EMBER_AF_RF4CE_MSO_IR_RF_DATABASE_FLAG_USE_DEFAULT) \
     == EMBER_AF_RF4CE_MSO_IR_RF_DATABASE_FLAG_USE_DEFAULT)
#endif

#ifdef DOXYGEN_SHOULD_SKIP_THIS
  /** @brief Determine if the IR-RF database entry has an RF pressed
   * descriptor.
   *
   * Implemented as a macro for efficiency.
   *
   * @return true if the IR-RF database table entry has an RF pressed
   * descriptor or false otherwise.
   */
  bool emberAfRf4ceMsoIrRfDatabaseEntryHasRfPressedDescriptor(const EmberAfRf4ceMsoIrRfDatabaseEntry *entry);
#else
  #define emberAfRf4ceMsoIrRfDatabaseEntryHasRfPressedDescriptor(entry)    \
    (READBITS((entry)->flags,                                              \
              (EMBER_AF_RF4CE_MSO_IR_RF_DATABASE_FLAG_RF_PRESSED_SPECIFIED \
               | EMBER_AF_RF4CE_MSO_IR_RF_DATABASE_FLAG_USE_DEFAULT))      \
     == EMBER_AF_RF4CE_MSO_IR_RF_DATABASE_FLAG_RF_PRESSED_SPECIFIED)
#endif

#ifdef DOXYGEN_SHOULD_SKIP_THIS
  /** @brief Determine if the IR-RF database entry has an RF repeated
   * descriptor.
   *
   * Implemented as a macro for efficiency.
   *
   * @return true if the IR-RF database table entry has an RF repeated
   * descriptor or false otherwise.
   */
  bool emberAfRf4ceMsoIrRfDatabaseEntryHasRfRepeatedDescriptor(const EmberAfRf4ceMsoIrRfDatabaseEntry *entry);
#else
  #define emberAfRf4ceMsoIrRfDatabaseEntryHasRfRepeatedDescriptor(entry)    \
    (READBITS((entry)->flags,                                               \
              (EMBER_AF_RF4CE_MSO_IR_RF_DATABASE_FLAG_RF_REPEATED_SPECIFIED \
               | EMBER_AF_RF4CE_MSO_IR_RF_DATABASE_FLAG_USE_DEFAULT))       \
     == EMBER_AF_RF4CE_MSO_IR_RF_DATABASE_FLAG_RF_REPEATED_SPECIFIED)
#endif

#ifdef DOXYGEN_SHOULD_SKIP_THIS
  /** @brief Determine if the IR-RF database entry has an RF released
   * descriptor.
   *
   * Implemented as a macro for efficiency.
   *
   * @return true if the IR-RF database table entry has an RF released
   * descriptor or false otherwise.
   */
  bool emberAfRf4ceMsoIrRfDatabaseEntryHasRfReleasedDescriptor(const EmberAfRf4ceMsoIrRfDatabaseEntry *entry);
#else
  #define emberAfRf4ceMsoIrRfDatabaseEntryHasRfReleasedDescriptor(entry)    \
    (READBITS((entry)->flags,                                               \
              (EMBER_AF_RF4CE_MSO_IR_RF_DATABASE_FLAG_RF_RELEASED_SPECIFIED \
               | EMBER_AF_RF4CE_MSO_IR_RF_DATABASE_FLAG_USE_DEFAULT))       \
     == EMBER_AF_RF4CE_MSO_IR_RF_DATABASE_FLAG_RF_RELEASED_SPECIFIED)
#endif

#ifdef DOXYGEN_SHOULD_SKIP_THIS
  /** @brief Get the minimum number of transmissions for a key code from its RF
   * RF descriptor.
   *
   * Implemented as a macro for efficiency.
   *
   * @return The minimum number of transmissions.
   */
  uint8_t emberAfRf4ceMsoIrRfDatabaseEntryGetMinimumNumberOfTransmissions(const EmberAfRf4ceMsoIrRfDatabaseRfDescriptor *rfDescriptor);
#else
  #define emberAfRf4ceMsoIrRfDatabaseEntryGetMinimumNumberOfTransmissions(rfDescriptor) \
    READBITS((rfDescriptor)->rfConfig,                                                  \
             EMBER_AF_RF4CE_MSO_IR_RF_DATABASE_RF_CONFIG_MINIMUM_NUMBER_OF_TRANSMISSIONS_MASK)
#endif

#ifdef DOXYGEN_SHOULD_SKIP_THIS
  /** @brief Determine if a key code should continue being transmitted after
   * the minimum number of transmissions have taken place when the key is kept
   * pressed.
   *
   * Implemented as a macro for efficiency.
   *
   * @return true if the key code should continue being transmitted after the
   * minimum number of transmissions have taken place or false otherwise.
   */
  bool emberAfRf4ceMsoIrRfDatabaseEntryShouldTransmitUntilRelease(const EmberAfRf4ceMsoIrRfDatabaseRfDescriptor *rfDescriptor);
#else
  #define emberAfRf4ceMsoIrRfDatabaseEntryShouldTransmitUntilRelease(rfDescriptor)             \
    (READBITS((rfDescriptor)->rfConfig,                                                        \
              EMBER_AF_RF4CE_MSO_IR_RF_DATABASE_RF_CONFIG_KEEP_TRANSMITTING_UNTIL_KEY_RELEASE) \
     == EMBER_AF_RF4CE_MSO_IR_RF_DATABASE_RF_CONFIG_KEEP_TRANSMITTING_UNTIL_KEY_RELEASE)
#endif

#ifdef DOXYGEN_SHOULD_SKIP_THIS
  /** @brief Determine if the IR-RF database entry has an IR descriptor.
   *
   * Implemented as a macro for efficiency.
   *
   * @return true if the IR-RF database table entry has an IR descriptor or
   * false otherwise.
   */
  bool emberAfRf4ceMsoIrRfDatabaseEntryHasIrDescriptor(const EmberAfRf4ceMsoIrRfDatabaseEntry *entry);
#else
  #define emberAfRf4ceMsoIrRfDatabaseEntryHasIrDescriptor(entry)      \
    (READBITS((entry)->flags,                                         \
              (EMBER_AF_RF4CE_MSO_IR_RF_DATABASE_FLAG_IR_SPECIFIED    \
               | EMBER_AF_RF4CE_MSO_IR_RF_DATABASE_FLAG_USE_DEFAULT)) \
     == EMBER_AF_RF4CE_MSO_IR_RF_DATABASE_FLAG_IR_SPECIFIED)
#endif

#endif // __RF4CE_MSO_H__

// END addtogroup
