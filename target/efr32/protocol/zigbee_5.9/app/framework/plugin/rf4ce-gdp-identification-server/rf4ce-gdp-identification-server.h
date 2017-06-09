// Copyright 2014 Silicon Laboratories, Inc.

#ifndef __RF4CE_GDP_IDENTIFICATION_SERVER__
#define __RF4CE_GDP_IDENTIFICATION_SERVER__

/**
 * @addtogroup rf4ce-gdp-identification-server
 *
 * This plugin provides an implementation of the RF4CE
 * Generic Device Profile (GDP) Identification Server. It gives the user
 * the opportunity to manage the GDP identification process from the
 * server side.
 *
 * The application can command the server to send an identify command
 * to a client using the API
 * ::emberAfRf4ceGdpIdentificationServerIdentify. One can also being the
 * process over the command line interface by using the command
 * <em>plugin rf4ce-gdp-identification-server identify</em> and supplying
 * the desired arguments.
 *
 * Please see @ref rf4ce-gdp-identification-Client for an implementation of
 * the RF4CE GDP Identification Client. Unlike the RF4CE GDP Identification
 * Client plugin, this plugin is able to be used across hardware platforms.
 *
 * @{
 */

/**
 * @brief Queue up an identify command to be sent to an identification client
 * in the pairing table. Note that one can issue the command
 * <em>plugin rf4ce-gdp-identification-server identify</em>
 * over the command line interface in order to perform the same action.
 *
 * @param pairingIndex The index in the pairing table to which to send
 * the identification command.
 * @param flags The flags to send in this command. See data type
 * ::EmberAfRf4ceGdpClientNotificationIdentifyFlags for more information
 * on this parameter.
 * @param timeS The time in seconds for which the client should identify.
 *
 * @return An ::EmberStatus value indicating the success of the attempt
 * to queue up the identify command.
 */
EmberStatus emberAfRf4ceGdpIdentificationServerIdentify(uint8_t pairingIndex,
                                                        EmberAfRf4ceGdpClientNotificationIdentifyFlags flags,
                                                        uint16_t timeS);


#endif /* __RF4CE_GDP_IDENTIFICATION_SERVER__ */

// END addtogroup
