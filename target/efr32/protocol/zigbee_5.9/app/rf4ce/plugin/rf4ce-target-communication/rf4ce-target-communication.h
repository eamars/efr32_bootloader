// Copyright 2014 Silicon Laboratories, Inc.
//-----------------------------------------------------------------------------
/**
 * @addtogroup full-featured-target
 *
 * The full featured target implements communication of action codes and audio
 * from the controller to the host computer and action mappings and controller
 * identification from the host computer to the controller.
 * This plugin implements the communication protocol between the RF4CE target
 * and the host computer and implements the functions that are required for
 * both the communication with the controller and host.
 *
 * One important feature of the communication protocol between the target
 * and host is that it has built in framing that defines a unique start
 * and stop for each package that is transferred. This ensures that the
 * communication between the target and host will get into synch if data
 * is lost. And it does not require and use a request - acknowledge scheme
 * that may be too slow for transferring live audio data.
 *
 * This plugin implements the two top layers of the OSI model, the
 * Application and Presentation layers. And it uses the already available
 * serial function calls that implements transport over the Physical layer.
 *
 * Attrobutes of the Presentation layer:
 * Each message begins with the byte 0xC0 and ends with the byte 0xC1.
 * In between the framing bytes are the payload and the checksum.
 * The last byte of a message (without the framing) contains an 8-bit checksum.
 * The checksum is calculated over the entire message, without framing and
 * without including the checksum byte. The checksum is obtained by XORing
 * one and one byte.
 * To make sure that 0xC0 and 0xC1 are not contained within the payload an
 * escape sequence is used. The value 0x7E is used to XOR the following byte
 * with the value 0x20. This is used for the following three combinations:
 * 0x7E 0x5E is used for 0x7E
 * 0x7E 0xE0 is used for 0xC0
 * 0x7E 0xE1 is used for 0xC1
 *
 * Attributes of the Application layer:
 * The application layer consists of an Application Header and Application Data.
 * The Application Header is mandatory. Some messages may consist of an
 * application header only. The Application Data is the payload of the
 * application layer message.
 * @{
 */

//****************************************************************************
enum
{
  EMBER_AF_TARGET_COMMUNICATION_BINDINFO_INIT    = 0,
  EMBER_AF_TARGET_COMMUNICATION_BINDINFO_SUCCESS = 1,
  EMBER_AF_TARGET_COMMUNICATION_BINDINFO_FAILURE = 2,
  EMBER_AF_TARGET_COMMUNICATION_BINDINFO_ATTEMPT = 3,
  EMBER_AF_TARGET_COMMUNICATION_BINDINFO_PROXY   = 4
};

//****************************************************************************
/**
 * @brief The initialization function.
 *
 * @param port The host communication port.
 **/
void    emberAfTargetCommunicationInit(uint8_t port);

/**
 * @brief Send an action to the host computer.
 *
 * @param port The host communication port.
 * @param type The action type.
 * @param modifier The action modifier.
 * @param bank The action bank.
 * @param code The action code.
 * @param vendor The action vendor.
 **/
void    emberAfTargetCommunicationHostActionTx(uint8_t port,
                                               uint8_t type,
                                               uint8_t modifier,
                                               uint8_t bank,
                                               uint8_t code,
                                               uint16_t vendor);

/**
 * @brief Send audio data to the host computer.
 *
 * @param port The host communication port.
 * @param pBuf Pointer to the buffer containing audion data.
 * @param uLen The length of the audion data in bytes.
 **/
void    emberAfTargetCommunicationHostAudioTx(uint8_t port,
                                              const uint8_t *pBuf,
                                              uint8_t uLen);

/**
 * @brief Send information about binding to the host computer.
 *
 * @param port The host communication port.
 * @param info The binding information.
 **/
void emberAfTargetCommunicationHostBindInfoTx(uint8_t port,
                                              uint8_t info);
/**
 * @brief Send an identification message to the controller.
 *
 * @param pairingIndex The pairing index to send the identification to.
 * @param flags The zrc identification flags for the identification.
 * @param seconds The length of the identification.
 **/
void    emberAfTargetCommunicationControllerIdentify(uint8_t pairingIndex,
                                                     uint8_t flags,
                                                     uint8_t seconds);

/**
 * @brief Send a notification message to the controller
 *
 * @param pairingIndex The pairing index to send the notification to.
 **/
void    emberAfTargetCommunicationControllerNotify(uint8_t uPairingIndex);

/**
 * @brief Set an action mapping in the mapping server.
 *
 * @param deviceType The device type.
 * @param bank The bank.
 * @param action The action code.
 * @param mappingFlags Mapping flags.
 * @param rfDat A pointer to the rf action mapping.
 * @param rfLen The length of the rf action mapping.
 * @param irDat A pointer to the ir action mapping.
 * @param irLen The length of the ir action mapping.
 **/
void    emberAfTargetCommunicationControllerRemapAction(uint8_t deviceType,
                                                        uint8_t bank,
                                                        uint8_t action,
                                                        uint8_t mappingFlags,
                                                        uint8_t *rfDat,
                                                        uint8_t rfLen,
                                                        uint8_t *irDat,
                                                        uint8_t irLen);

/** @brief Host Binding Request
 *
 * This function is called by the RF4CE Target Communication plugin when it
 * receives a request from the host to accept incoming binding requests.
 */
void emberAfPluginRf4ceTargetCommunicationHostBindingRequestCallback(void);

//************************************
// Test functions
#ifndef DOXYGEN_SHOULD_SKIP_THIS
void    emAfTargetCommunicationTestAppEncodingAndDecoding(void);
void    emAfTargetCommunicationTestPreEncodingAndDecoding(void);
void    emAfTargetCommunicationTestUsbTransferSpeed(uint8_t port);
void    emAfTargetCommunicationTestPrSecond(void);
#endif

//************************************

