// *******************************************************************
// * ota-server.h
// *
// *
// * Copyright 2010 by Ember Corporation. All rights reserved.              *80*
// *******************************************************************

uint8_t emAfOtaServerGetBlockSize(void);
uint8_t emAfOtaImageBlockRequestHandler(EmberAfImageBlockRequestCallbackStruct* callbackData);

bool emAfOtaPageRequestErrorHandler(void);

void emAfOtaPageRequestTick(uint8_t endpoint);

// Returns the status code to the request.
uint8_t emAfOtaPageRequestHandler(uint8_t clientEndpoint,
                                uint8_t serverEndpoint,
                                const EmberAfOtaImageId* id,
                                uint32_t offset,
                                uint8_t maxDataSize,
                                uint16_t pageSize,
                                uint16_t responseSpacing);

bool emAfOtaServerHandlingPageRequest(void);

// This will eventually be moved into a Plugin specific callbacks file.
void emberAfOtaServerSendUpgradeCommandCallback(EmberNodeId dest,
                                                uint8_t endpoint,
                                                const EmberAfOtaImageId* id);

/** @brief OTA Server Block Sent Callback
 *
 * This function will be called when a block is sent to a device.
 *
 * @param actualLength  Block length.
 * @param manufacturerId  ManufacturerId.
 * @param imageTypeId  Image Type Id.
 * @param firmwareVersion  Firmware Version
 */
void emberAfPluginOtaServerBlockSentCallback(uint8_t actualLength,
                                             uint16_t manufacturerId,
                                             uint16_t imageTypeId,
                                             uint32_t firmwareVersion);

/** @brief OTA Server Update Started Callback
 *
 * This function will be called when an update has started.
 *
 * @param manufacturerId  ManufacturerId.
 * @param imageTypeId  Image Type Id.
 * @param firmwareVersion  Firmware Version.
 * @param maxDataSize  Max data size.
 * @param offset  Offset.
 */
void emberAfPluginOtaServerUpdateStartedCallback(uint16_t manufacturerId,
                                                 uint16_t imageTypeId,
                                                 uint32_t firmwareVersion,
                                                 uint8_t maxDataSize,
                                                 uint32_t offset);

/** @brief OTA Server Update Complete Callback
 *
 * This function will be called when an OTA update has finished.
 *
 * @param manufacturerId  ManufacturerId.
 * @param imageTypeId  Image Type Id.
 * @param firmwareVersion  Firmware Version.
 * @param source  Source node id.
 * @param status  Status of update.
 */
void emberAfPluginOtaServerUpdateCompleteCallback(uint16_t manufacturerId,
                                                  uint16_t imageTypeId,
                                                  uint32_t firmwareVersion,
                                                  EmberNodeId source,
                                                  uint8_t status);

#if defined(EMBER_TEST) && !defined(EM_AF_TEST_HARNESS_CODE)
  #define EM_AF_TEST_HARNESS_CODE
#endif

