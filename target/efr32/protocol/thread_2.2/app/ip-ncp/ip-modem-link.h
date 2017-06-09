// File: ip-modem-link.h
//
// Description: Connect the IP modem to a communications layer for 
// sending and receiving management and packet data.
//
// Copyright 2014 by Silicon Laboratories. All rights reserved.            *80*

#ifndef __IP_MODEM_LINK_H__
#define __IP_MODEM_LINK_H__

#include "stack/include/ember-types.h"

///////////////////////////////////////////////////////////////////////////////
// Custom types used by the IP modem link layer
///////////////////////////////////////////////////////////////////////////////

// The type of management command being sent to/from the host
typedef enum {
  MANAGEMENT_IDLE,              // internal state, not sent in messages
  MANAGEMENT_COMMAND,           // a command
  MANAGEMENT_NOTIFY,            // error and warning messages sent by the NCP
  // The NCP sends a RESPONSE_DONE or RESPONSE_ERROR when it finishes
  // processing a command.
  MANAGEMENT_RESPONSE_DONE,     // end of responses to most recent command
  MANAGEMENT_RESPONSE_ERROR,    // If a command was not parsed successfully,
                                // an error message is sent as a notification,
                                // followed by an empty RESPONSE_ERROR message.
} ManagementType;


///////////////////////////////////////////////////////////////////////////////
// Functions implemented by the lower level layer
///////////////////////////////////////////////////////////////////////////////

/**
 * Initialize the communication link used by the IP modem. This function should
 * be implemented for the appliacation specific communication mechanism. If
 * it is ommitted then a default initialization that does nothing will be used.
 */
void ipModemLinkInit(void);

/**
 * Reset the IP modem communications link. This should clear any state held
 * in the link and ensure that it's ready to work again.
 */
void ipModemLinkReset(void);

/**
 * This function should block until an incoming message is available for the
 * IP modem link or the timeout expires.
 * @param timeoutMs The timeout in miliseconds.
 * @return Returns true if a message is available and false if the timeout
 * expired.
 */
bool ipModemLinkWaitForIncoming(uint32_t timeoutMs);

/**
 * Attempt to exit the wait state entered by the ipModemLinkWaitForIncoming()
 * command. This can be used to stop the IP modem main loop from blocking
 * after an asynchronous event like an interrupt.
 * NOTE: Code in this function must be safe to call from within interrupt
 * context.
 */
void ipModemLinkAbortWaitForIncoming(void);

/**
 * This function should check for incoming messages for the IP modem, read them
 * out and call the appropriate processing functions. This must be implemented
 * for the specific link and should not block waiting for new messages.
 */
void ipModemLinkProcessIncoming(void);

/**
 * This funtion will send an IP packet received by the node to the host
 * application or thread for further processing.
 * @param secured If true then this packet is to be secured on output
 * @param len The length in bytes of the data packet.
 */
void ipModemLinkIpPacketReceived(SerialLinkMessageType type, Buffer b);

/**
 * This function will take a management command and send it out over the
 * current link. The type is given and should be included so that it
 * can be decoded on the other side of the link.
 * @param managementType The type of management command being sent.
 * @param data A pointer to the management command data.
 * @param dataLength The length of the management command being sent.
 * @return Returns True if the management command could be sent or false
 * if we were unable to send it for any reason.
 */
bool ipModemLinkSendManagementCommand(ManagementType managementType,
                                      const uint8_t *data,
                                      uint8_t len);

/**
 * This function is only used in environments where both the host and stack
 * code run on the same chip (e.g. RTOS). In this situation this function
 * will be called to send a management command from the host to the virtual
 * NCP thread. On platforms that don't support this return false.
 * @param managementType The type of management command being sent.
 * @param data A pointer to the management command data.
 * @param dataLength The length of the management command being sent.
 * @return Returns True if the management command could be sent or false
 * if we were unable to send it for any reason.
 */
bool ipModemLinkSendManagementCommandHost(ManagementType managementType,
                                          const uint8_t *data,
                                          uint8_t len);

/**
 * If this link is using any Ember buffers it should mark them here to prevent
 * garbage collection. If you are not using Ember buffers then this function
 * should just be stubbed out.
 */
void ipModemLinkMarkBuffers(void);

/**
 * This optional routine allows the ip modem to use memory allocated from the
 * ip modem link instead of Ember buffers. You must return a pointer to the
 * memory allocated as well as set the pointer to be passed to the
 * ipModemLinkMemoryFree() function when garbage collecting. This function must
 * be thread safe.
 * @param size The number of bytes to allocate.
 * @param freePtr The pointer that should be passed to ipModemLinkMemoryFree()
 * when releasing this memory. If NULL, no callback will happen.
 * @return The address of the data pointer to use or NULL if this routine
 * is not implemented and we should just use Ember buffers.
 */
void* ipModemLinkMemoryAllocate(uint32_t size, void **freePtr);

/**
 * This routine is called when freeing memory that was allocated with the 
 * ipModemLinkMemoryAllocate() function. It will be passed the freePtr that
 * was specified by the allocate routine. This function must be thread safe.
 * @param ptr The value of the pointer that was set in freePtr when the
 * ipModemLinkMemoryAllocate() function was called.
 */
void ipModemLinkMemoryFree(void *ptr);

/*
 * This function is called when the chip is powering down to clean up any
 * transactions over the link. This function should return true if there are
 * more messages that you're waiting to transmit over the link and false
 * otherwise.
 * @return true if you have more messages to send out over the communications
 * link.
 */
bool ipModemLinkPreparingForPowerDown(void);

/**
 * This function is called when there is an error sending management commands
 * over the IP modem link. In RTOS builds it can be called by either the host
 * or VNCP threads as indicated by the hostToNcp parameter.
 * @param data A pointer to the data that we could not send.
 * @param len The length of the data that we could not send in bytes.
 * @param hostToNcp True if this message was from the host to VNCP in RTOS
 * builds and false otherwise.
 */
void ipModemLinkManagementErrorHandler(const uint8_t *data,
                                       uint8_t len,
                                       bool hostToNcp);

///////////////////////////////////////////////////////////////////////////////
// Functions that are implemented in this layer
///////////////////////////////////////////////////////////////////////////////

/*
 * Call this function to send a raw IPv6 packet to the networking stack.
 * @param secured Whether this packet should be secured at the MAC layer.
 * @param b A buffer containing the raw IPv6 packet.
 * @return true if we were able to process the packet and false if something
 * went wrong and the packet should be retried again later. 
 */
bool ipModemLinkIpPacketHandler(SerialLinkMessageType type, Buffer b);

/*
 * Call this function to handle a management packet that was sent over the
 * IP modem link.
 * @param managementType The type of management command received.
 * @param data A pointer to the data for this management packet.
 * @param len The length of the management command to process.
 * @return true if we were able to process the packet and false if something
 * went wrong.
 */
bool ipModemLinkManagementPacketHandler(ManagementType managementType,
                                           uint8_t *data,
                                           uint8_t len);

void ipModemLinkUartTransmit(uint8_t type, PacketHeader header);

#endif //__IP_MODEM_LINK_H__
