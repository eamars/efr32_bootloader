// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"

#include "rf4ce-mso.h"
#include "rf4ce-mso-internal.h"
#ifdef EMBER_SCRIPTED_TEST
#include "rf4ce-mso-test.h"
#endif // EMBER_SCRIPTED_TEST
#include "rf4ce-mso-attributes.h"

// The IR-RF Database attribute is managed externally.  This lets the other
// checks work for this attribute even though we don't actually do anything
// with it here.
#ifdef EMBER_AF_PLUGIN_RF4CE_MSO_IS_ORIGINATOR
  #define IR_RF_DATABASE_SIZE 0xFF
#else
  #define IR_RF_DATABASE_SIZE 0
#endif

static const EmAfRf4ceMsoAttributeDescriptor attributeDescriptors[MSO_ATTRIBUTES_COUNT] =
{
    {EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_PERIPHERAL_IDS,
     MSO_RIB_ATTRIBUTE_PERIPHERAL_IDS_LENGTH,
     (MSO_ATTRIBUTE_HAS_REMOTE_WRITE_ACCESS_BIT
      | MSO_ATTRIBUTE_IS_ARRAYED_BIT),
      EMBER_AF_PLUGIN_RF4CE_MSO_PERIPHERAL_ID_ENTRIES},

    {EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_RF_STATISTICS,
     MSO_RIB_ATTRIBUTE_RF_STATISTICS_LENGTH,
     MSO_ATTRIBUTE_HAS_REMOTE_WRITE_ACCESS_BIT,
     0},

    {EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_VERSIONING,
     MSO_RIB_ATTRIBUTE_VERSIONING_LENGTH,
     (MSO_ATTRIBUTE_HAS_REMOTE_WRITE_ACCESS_BIT
      | MSO_ATTRIBUTE_IS_ARRAYED_BIT),
     MSO_ATTRIBUTE_VERSIONING_ENTRIES},

    {EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_BATTERY_STATUS,
     MSO_RIB_ATTRIBUTE_BATTERY_STATUS_LENGTH,
     MSO_ATTRIBUTE_HAS_REMOTE_WRITE_ACCESS_BIT,
     0},

    {EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_SHORT_RF_RETRY_PERIOD,
     MSO_RIB_ATTRIBUTE_SHORT_RF_RETRY_PERIOD_LENGTH,
     0,
     0},

    {EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_IR_RF_DATABASE,
     IR_RF_DATABASE_SIZE,
     MSO_ATTRIBUTE_IS_ARRAYED_BIT,
     0}, // since these attributes are managed by the application, we don't
         // check the index for them. This value is meaningless.

    {EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_VALIDATION_CONFIGURATION,
     MSO_RIB_ATTRIBUTE_VALIDATION_CONFIGURATION_LENGTH,
     0,
     0},

    {EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_GENERAL_PURPOSE,
     MSO_RIB_ATTRIBUTE_GENERAL_PURPOSE_LENGTH,
     (MSO_ATTRIBUTE_HAS_REMOTE_WRITE_ACCESS_BIT
      | MSO_ATTRIBUTE_IS_ARRAYED_BIT),
     EMBER_AF_PLUGIN_RF4CE_MSO_GENERAL_PURPOSE_ENTRIES},
};

static const EmAfRf4ceMsoAttributeDescriptor *getAttributeDescriptor(EmberAfRf4ceMsoAttributeId attrId)
{
  uint8_t i;
  for(i=0; i<MSO_ATTRIBUTES_COUNT; i++) {
    if (attributeDescriptors[i].id == attrId) {
      return &attributeDescriptors[i];
    }
  }

  return NULL;
}

bool emAfRf4ceMsoAttributeIsSupported(EmberAfRf4ceMsoAttributeId attrId)
{
  return (getAttributeDescriptor(attrId) != NULL);
}

// Assumes the attribute to be supported.
bool emAfRf4ceMsoAttributeIsArrayed(EmberAfRf4ceMsoAttributeId attrId)
{
  return ((getAttributeDescriptor(attrId)->bitmask
           & MSO_ATTRIBUTE_IS_ARRAYED_BIT) > 0);
}

// Assumes the attribute to be supported.
bool emAfRf4ceMsoAttributeIsRemotelyWritable(EmberAfRf4ceMsoAttributeId attrId)
{
  return ((getAttributeDescriptor(attrId)->bitmask
           & MSO_ATTRIBUTE_HAS_REMOTE_WRITE_ACCESS_BIT) > 0);
}

// Assumes the attribute to be supported.
uint8_t emAfRf4ceMsoGetAttributeLength(EmberAfRf4ceMsoAttributeId attrId)
{
  return getAttributeDescriptor(attrId)->size;
}

// Assumes the attribute to be supported.
uint8_t emAfRf4ceMsoGetArrayedAttributeDimension(EmberAfRf4ceMsoAttributeId attrId)
{
  return getAttributeDescriptor(attrId)->dimension;
}
