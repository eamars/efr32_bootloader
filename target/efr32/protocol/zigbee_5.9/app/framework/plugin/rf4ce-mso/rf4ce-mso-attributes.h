// Copyright 2014 Silicon Laboratories, Inc.

#ifndef _RF4CE_MSO_ATTRIBUTES_H_
#define _RF4CE_MSO_ATTRIBUTES_H_

#include "app/framework/plugin/rf4ce-profile/rf4ce-profile.h"

#define MSO_RIB_ATTRIBUTE_PERIPHERAL_IDS_LENGTH                   4
#define MSO_RIB_ATTRIBUTE_RF_STATISTICS_LENGTH                    16
#define MSO_RIB_ATTRIBUTE_VERSIONING_LENGTH                       2
#define MSO_RIB_ATTRIBUTE_BATTERY_STATUS_LENGTH                   11
#define MSO_RIB_ATTRIBUTE_SHORT_RF_RETRY_PERIOD_LENGTH            4
// IR-RF database length has variable length
#define MSO_RIB_ATTRIBUTE_VALIDATION_CONFIGURATION_LENGTH         4
#define MSO_RIB_ATTRIBUTE_GENERAL_PURPOSE_LENGTH                  16

// Bitmask field definitions.
#define MSO_ATTRIBUTE_HAS_REMOTE_WRITE_ACCESS_BIT                 0x01
#define MSO_ATTRIBUTE_IS_ARRAYED_BIT                              0x02

#define MSO_ATTRIBUTES_COUNT                                      8

// Versioning attribute
#define MSO_ATTRIBUTE_VERSIONING_ENTRIES                          3
#define MSO_ATTRIBUTE_VERSIONING_INDEX_SOFTWARE                   0x00
#define MSO_ATTRIBUTE_VERSIONING_INDEX_HARDWARE                   0x01
#define MSO_ATTRIBUTE_VERSIONING_INDEX_IR_DATABASE                0x02

typedef struct {
  EmberAfRf4ceMsoAttributeId id;
  uint8_t size;
  uint8_t bitmask;
  uint8_t dimension; // only for arrayed attributes
} EmAfRf4ceMsoAttributeDescriptor;

typedef struct {
  uint8_t deviceType;
  uint8_t peripheralId[MSO_RIB_ATTRIBUTE_PERIPHERAL_IDS_LENGTH];
} EmAfRf4ceMsoPeripheralIdEntry;

typedef struct {
  EmAfRf4ceMsoPeripheralIdEntry peripheralIds[EMBER_AF_PLUGIN_RF4CE_MSO_PERIPHERAL_ID_ENTRIES];
  uint8_t rfStatistics[MSO_RIB_ATTRIBUTE_RF_STATISTICS_LENGTH];
  uint8_t versioning[MSO_ATTRIBUTE_VERSIONING_ENTRIES][MSO_RIB_ATTRIBUTE_VERSIONING_LENGTH];
  uint8_t batteryStatus[MSO_RIB_ATTRIBUTE_BATTERY_STATUS_LENGTH];
  uint8_t shortRfRetryPeriod[MSO_RIB_ATTRIBUTE_SHORT_RF_RETRY_PERIOD_LENGTH];
  // IR-RF database stored by the application
  uint8_t validationConfiguration[MSO_RIB_ATTRIBUTE_VALIDATION_CONFIGURATION_LENGTH];
  uint8_t generalPurpose[EMBER_AF_PLUGIN_RF4CE_MSO_GENERAL_PURPOSE_ENTRIES][MSO_RIB_ATTRIBUTE_GENERAL_PURPOSE_LENGTH];
} EmAfRf4ceMsoRibAttributes;

// Originator local attributes.
extern EmAfRf4ceMsoRibAttributes emAfRf4ceMsoLocalRibAttributes;

void emAfRf4ceMsoInitAttributes(void);

EmberAfRf4ceStatus emAfRf4ceMsoSetAttributeRequestCallback(uint8_t pairingIndex,
                                                           EmberAfRf4ceMsoAttributeId attributeId,
                                                           uint8_t index,
                                                           uint8_t valueLen,
                                                           const uint8_t *value);

EmberAfRf4ceStatus emAfRf4ceMsoGetAttributeRequestCallback(uint8_t pairingIndex,
                                                           EmberAfRf4ceMsoAttributeId attributeId,
                                                           uint8_t index,
                                                           uint8_t *valueLen,
                                                           uint8_t *value);

void emAfRf4ceMsoSetAttributeResponseCallback(EmberAfRf4ceMsoAttributeId attributeId,
                                              uint8_t index,
                                              EmberAfRf4ceStatus status);

void emAfRf4ceMsoGetAttributeResponseCallback(EmberAfRf4ceMsoAttributeId attributeId,
                                              uint8_t index,
                                              EmberAfRf4ceStatus status,
                                              uint8_t valueLen,
                                              const uint8_t *value);

bool emAfRf4ceMsoAttributeIsSupported(EmberAfRf4ceMsoAttributeId attrId);
bool emAfRf4ceMsoAttributeIsArrayed(EmberAfRf4ceMsoAttributeId attrId);
bool emAfRf4ceMsoAttributeIsRemotelyWritable(EmberAfRf4ceMsoAttributeId attrId);
uint8_t emAfRf4ceMsoGetAttributeLength(EmberAfRf4ceMsoAttributeId attrId);
uint8_t emAfRf4ceMsoGetArrayedAttributeDimension(EmberAfRf4ceMsoAttributeId attrId);
bool emAfRf4ceMsoArrayedAttributeIndexIsValid(uint8_t pairingIndex,
                                                 EmberAfRf4ceMsoAttributeId attributeId,
                                                 uint8_t index,
                                                 bool isGet);

void emAfRf4ceMsoWriteAttributeRecipient(uint8_t pairingIndex,
                                         EmberAfRf4ceMsoAttributeId attributeId,
                                         uint8_t index,
                                         uint8_t valueLen,
                                         const uint8_t *value);

void emAfRf4ceMsoReadAttributeRecipient(uint8_t pairingIndex,
                                        EmberAfRf4ceMsoAttributeId attributeId,
                                        uint8_t index,
                                        uint8_t *valueLen,
                                        uint8_t *value);

uint8_t emAfRf4ceMsoAttributePeripheralIdEntryLookup(uint8_t pairingIndex,
                                                   uint8_t deviceType);
uint8_t emAfRf4ceMsoAttributeGetUnusedPeripheralIdEntryIndex(uint8_t pairingIndex);

#endif //_RF4CE_MSO_ATTRIBUTES_H_
