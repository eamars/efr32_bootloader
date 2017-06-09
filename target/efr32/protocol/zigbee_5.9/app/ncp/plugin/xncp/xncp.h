//
// xncp.h
//
// Author(s): Maurizio Nanni, maurizio.nanni@ember.com
//
// Programmable NCP code.
//
// Copyright 2015 Silicon Laboratories, Inc.
//

#ifndef __XNCP_H__

#include PLATFORM_HEADER

/**
 * @addtogroup xncp
 *
 * The XNCP functionality provides a way for an NCP application to send
 * and receive custom EZSP frames to and from a HOST.  This gives users the
 * ability to develop their own serial protocols between a HOST and the NCP.
 *
 * An NCP application can use the API ::emberAfPluginXncpSendCustomEzspMessage
 * to send custom EZSP messages to the HOST.  The message will be sent
 * to the HOST in an asynchronus manner, but the application can use the
 * ::EmberStatus return byte from the API to tell if their message was
 * successfully scheduled.
 *
 * An NCP application wishing to receive and handle custom EZSP frames from
 * a HOST should make use of the callbacks provided by this module.  The
 * two most important callbacks to an NCP application will most likely be
 * ::emberAfPluginXncpGetXncpInformation and
 * ::emberAfPluginXncpIncomingCustomFrameCallback.  Users will want to
 * implement the former of these two callbacks to declare the manufacturer ID
 * and version of their NCP application.  The second callback will need to be
 * implemented in order for the NCP application to process custom EZSP frames
 * coming from the HOST.  Using this callback, the message can be processed and
 * the response can be written.  Upon return of this function, the custom
 * response will be sent back to the HOST.
 *
 * @{
 */

/** @brief Send Custom EZSP Message
 *
 * This function will send a custom EZSP message payload of length to the HOST.
 *
 * @param length The length of the custom EZSP message.  Ver.: always
 * @param payload The custom EZSP message itself.  Ver.: always
 *
 * @return An ::EmberStatus value describing the result of sending the custom
 * EZSP frame to the HOST.
 */
EmberStatus emberAfPluginXncpSendCustomEzspMessage(uint8_t length, uint8_t *payload);

#endif /* __XNCP_H__ */

// @} END addtogroup
