// *******************************************************************
// * util.h
// *
// *
// * Copyright 2007 by Ember Corporation. All rights reserved.              *80*
// *******************************************************************

#ifndef __AF_UTIL_H__
#define __AF_UTIL_H__

// This controls the type of response. Normally The library sends an automatic
// response (if appropriate) on the same PAN. The reply can be disabled by
// calling emberAfSetNoReplyForNextMessage.
#define ZCL_UTIL_RESP_NORMAL   0
#define ZCL_UTIL_RESP_NONE     1
#define ZCL_UTIL_RESP_INTERPAN 2

// Cluster name structure
typedef struct {
  uint16_t id;
  PGM_P name;
} EmberAfClusterName;

extern PGM EmberAfClusterName zclClusterNames[];

#define ZCL_NULL_CLUSTER_ID 0xFFFF

#include "../include/af.h"

// Override APS retry: 0 - don't touch, 1 - always set, 2 - always unset
#define EMBER_AF_RETRY_OVERRIDE_NONE 0
#define EMBER_AF_RETRY_OVERRIDE_SET 1
#define EMBER_AF_RETRY_OVERRIDE_UNSET 2

// EMBER_AF_MAXIMUM_SEND_PAYLOAD_LENGTH is defined in config.h
#define EMBER_AF_RESPONSE_BUFFER_LEN EMBER_AF_MAXIMUM_SEND_PAYLOAD_LENGTH

void emberAfInit(void);
void emberAfTick(void);
uint16_t emberAfFindClusterNameIndex(uint16_t cluster);
void emberAfStackDown(void);

void emberAfDecodeAndPrintCluster(uint16_t cluster);


bool emberAfProcessMessage(EmberApsFrame *apsFrame,
                              EmberIncomingMessageType type,
                              uint8_t *message,
                              uint16_t msgLen,
                              EmberNodeId source,
                              InterPanHeader *interPanHeader);

bool emberAfProcessMessageIntoZclCmd(EmberApsFrame* apsFrame,
                                        EmberIncomingMessageType type,
                                        uint8_t* message,
                                        uint16_t messageLength,
                                        EmberNodeId source,
                                        InterPanHeader* interPanHeader,
                                        EmberAfClusterCommand* returnCmd);


/**
 * Retrieves the difference between the two passed values.
 * This function assumes that the two values have the same endianness.
 * On platforms that support 64-bit primitive types, this function will work
 * on data sizes up to 8 bytes. Otherwise, it will only work on data sizes of
 * up to 4 bytes.
 */
EmberAfDifferenceType emberAfGetDifference(uint8_t* pData,
                                           EmberAfDifferenceType value,
                                           uint8_t dataSize);

/**
 * Retrieves an uint32_t from the given Zigbee payload. The integer retrieved 
 * may be cast into an integer of the appropriate size depending on the 
 * number of bytes requested from the message. In Zigbee, all integers are
 * passed over the air in LSB form. LSB to MSB conversion is 
 * done within this function automatically before the integer is returned.
 * 
 * Obviously (due to return value) this function can only handle 
 * the retrieval of integers between 1 and 4 bytes in length.
 * 
 */
uint32_t emberAfGetInt(const uint8_t* message, uint16_t currentIndex, uint16_t msgLen, uint8_t bytes);

void emberAfClearResponseData(void);
uint8_t  * emberAfPutInt8uInResp(uint8_t value);
uint16_t * emberAfPutInt16uInResp(uint16_t value);
uint32_t * emberAfPutInt32uInResp(uint32_t value);
uint32_t * emberAfPutInt24uInResp(uint32_t value);
uint8_t * emberAfPutBlockInResp(const uint8_t* data, uint16_t length);
uint8_t * emberAfPutStringInResp(const uint8_t *buffer);
uint8_t * emberAfPutDateInResp(EmberAfDate *value);

bool emberAfIsThisMyEui64(EmberEUI64 eui64);

extern uint8_t emberAfApsRetryOverride;


// If the variable has not been set, APS_TEST_SECURITY_DEFAULT will 
// eventually return false.
enum {
  APS_TEST_SECURITY_ENABLED = 0,
  APS_TEST_SECURITY_DISABLED = 1,
  APS_TEST_SECURITY_DEFAULT = 2,
};
extern uint8_t emAfTestApsSecurityOverride;


#ifdef EZSP_HOST
// the EM260 host application is expected to provide these functions if using
// a cluster that needs it.
EmberNodeId emberGetSender(void);
EmberStatus emberGetSenderEui64(EmberEUI64 senderEui64);
#endif //EZSP_HOST

// DEPRECATED.
extern uint8_t emberAfIncomingZclSequenceNumber;

// the caller to the library can set a flag to say do not respond to the
// next ZCL message passed in to the library. Passing true means disable
// the reply for the next ZCL message. Setting to false re-enables the
// reply (in the case where the app disables it and then doesnt send a 
// message that gets parsed).
void emberAfSetNoReplyForNextMessage(bool set);

// this function determines if APS Link key should be used to secure 
// the message. It is based on the clusterId and specified in the SE
// app profile.  If the message is outgoing then the 
bool emberAfDetermineIfLinkSecurityIsRequired(uint8_t commandId,
                                                 bool incoming,
                                                 bool broadcast,
                                                 EmberAfProfileId profileId,
                                                 EmberAfClusterId clusterId,
                                                 EmberNodeId remoteNodeId);

#define isThisDataTypeSentLittleEndianOTA(dataType) \
                    (!(emberAfIsThisDataTypeAStringType(dataType)))

bool emAfProcessGlobalCommand(EmberAfClusterCommand *cmd);
bool emAfProcessClusterSpecificCommand(EmberAfClusterCommand *cmd);

extern uint8_t emberAfResponseType;

uint16_t emberAfStrnlen(const uint8_t* string, uint16_t maxLength);

/* @brief Append characters to a ZCL string.
 * 
 * Appending characters to an existing ZCL string. If it is an invalid string
 * (length byte equals 0xFF), no characters will be appended.
 *
 * @param zclString - pointer to the zcl string
 * @param zclStringMaxLen - length of zcl string (does not include zcl length byte)
 * @param src - pointer to plain text characters
 * @param srcLen - length of plain text characters
 *
 * @return number of characters appended
 *
 */
uint8_t emberAfAppendCharacters(uint8_t * zclStringDst,
                              uint8_t zclStringMaxLen,
                              const uint8_t * appendingChars, 
                              uint8_t appendingCharsLen);

extern uint8_t emAfExtendedPanId[];

#endif // __AF_UTIL_H__
