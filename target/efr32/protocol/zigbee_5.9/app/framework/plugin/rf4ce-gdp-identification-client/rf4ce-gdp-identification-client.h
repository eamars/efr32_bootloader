// Copyright 2014 Silicon Laboratories, Inc.

#ifndef __RF4CE_GDP_IDENTIFCATION_CLIENT_H__
#define __RF4CE_GDP_IDENTIFCATION_CLIENT_H__

/**
 * @addtogroup rf4ce-gdp-identification-client
 *
 * This plugin provides an implementation of the Generic Device
 * Profile (GDP) Identification Client. It offers an excellent
 * example of the role of an identification client under GDP using
 * the Silicon Labs development kit. However, if a customer moves their
 * application to different hardware, they will need to create their
 * own implementation that makes use of the new hardware. See UG10310
 * for more information regarding the role of the GDP Identification Client.
 * 
 * In this Ember implementation of the GDP Identification Client, indicators
 * on the Silicon Labs development board will react to identification
 * commands from the server, notifying the user that it is in identifying
 * mode. These notifications come in the form of flashing LEDs, a buzzer
 * sounding, or printing status messages.
 *
 * The application can notify the plugin that a user interaction
 * has been detected using the API
 * ::emberAfRf4ceGdpIdentificationClientDetectedUserInteraction.
 * This will trigger the plugin to perform the necessary actions
 * in accordance with the GDP standard. A user can also incite call upon
 * this event by issuing the following command over the command line
 * interface: <em>plugin rf4ce-gdp-identification-client user-interaction</em>.
 *
 * Please see @ref rf4ce-gdp-identification-server for an implementation of
 * the RF4CE GDP Identification Server.
 *
 * @{
 */

/**
 * @brief Notify the plugin that a user interaction has been detected. Note
 * that this command can also be called from the command line interface
 * using the command
 * <em>plugin rf4ce-gdp-identification-client user-interaction</em>.
 */
void emberAfRf4ceGdpIdentificationClientDetectedUserInteraction(void);

#endif /* __RF4CE_GDP_IDENTIFCATION_CLIENT_H__ */

// END addtogroup
