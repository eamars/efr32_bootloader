// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"

#include "rf4ce-mso.h"
#include "rf4ce-mso-internal.h"
#ifdef EMBER_SCRIPTED_TEST
#include "rf4ce-mso-test.h"
#endif // EMBER_SCRIPTED_TEST
#include "rf4ce-mso-attributes.h"

#if defined(EMBER_AF_PLUGIN_RF4CE_MSO_IS_ORIGINATOR) || defined(EMBER_SCRIPTED_TEST)

// An originator stores its own set of attributes in RAM.
EmAfRf4ceMsoRibAttributes emAfRf4ceMsoLocalRibAttributes;

static uint8_t msoAttributePeripheralIdEntryLookup(uint8_t deviceType);
static uint8_t msoAttributeGetUnusedPeripheralIdEntryIndex(void);
static bool msoArrayedAttributeIndexIsValid(EmberAfRf4ceMsoAttributeId attributeId,
                                               uint8_t index);

void emAfRf4ceMsoInitAttributes(void)
{
  MEMSET(&emAfRf4ceMsoLocalRibAttributes,
         0xFF,
         sizeof(EmAfRf4ceMsoRibAttributes));

  // Initialize the validation configuration attribute fields to the default
  // values.
  emAfRf4ceMsoLocalRibAttributes.validationConfiguration[0] =
      LOW_BYTE(EMBER_AF_PLUGIN_RF4CE_MSO_LINK_LOST_WAIT_TIME_MS);
  emAfRf4ceMsoLocalRibAttributes.validationConfiguration[1] =
      HIGH_BYTE(EMBER_AF_PLUGIN_RF4CE_MSO_LINK_LOST_WAIT_TIME_MS);
  emAfRf4ceMsoLocalRibAttributes.validationConfiguration[2] =
      LOW_BYTE(EMBER_AF_PLUGIN_RF4CE_MSO_AUTO_CHECK_VALIDATION_PERIOD_MS);
  emAfRf4ceMsoLocalRibAttributes.validationConfiguration[3] =
      HIGH_BYTE(EMBER_AF_PLUGIN_RF4CE_MSO_AUTO_CHECK_VALIDATION_PERIOD_MS);
}

void emAfRf4ceMsoSetAttributeResponseCallback(EmberAfRf4ceMsoAttributeId attributeId,
                                              uint8_t index,
                                              EmberAfRf4ceStatus status)
{
#if defined(EMBER_TEST)
  emberAfCorePrintln("Set Attribute Response 0x%x, %d, 0x%x",
                     attributeId,
                     index,
                     status);
#endif // EMBER_TEST
}

void emAfRf4ceMsoGetAttributeResponseCallback(EmberAfRf4ceMsoAttributeId attributeId,
                                              uint8_t index,
                                              EmberAfRf4ceStatus status,
                                              uint8_t valueLen,
                                              const uint8_t * value)
{
#if defined(EMBER_TEST)
  emberAfCorePrint("Get Attribute Response 0x%x, %d, 0x%x, %d, {",
                   attributeId,
                   index,
                   status,
                   valueLen);
  emberAfCorePrintBuffer(value, valueLen, true);
  emberAfCorePrintln("}");
#endif // EMBER_TEST

  if (!emAfRf4ceMsoAttributeIsSupported(attributeId)
      || valueLen > emAfRf4ceMsoGetAttributeLength(attributeId)
      || (emAfRf4ceMsoAttributeIsArrayed(attributeId)
          && !msoArrayedAttributeIndexIsValid(attributeId,
                                              index))) {
    return;
  }

  if (status == EMBER_AF_RF4CE_STATUS_SUCCESS) {
    switch(attributeId) {
    case EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_PERIPHERAL_IDS:
    {
      uint8_t entryIndex = msoAttributePeripheralIdEntryLookup(index);
      if (entryIndex == 0xFF) {
        entryIndex = msoAttributeGetUnusedPeripheralIdEntryIndex();
      }
      if (entryIndex == 0xFF) {
        return;
      }

      emAfRf4ceMsoLocalRibAttributes.peripheralIds[entryIndex].deviceType = index;
      MEMCOPY(emAfRf4ceMsoLocalRibAttributes.peripheralIds[entryIndex].peripheralId,
              value,
              valueLen);
    }
      break;
    case EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_RF_STATISTICS:
      MEMCOPY(emAfRf4ceMsoLocalRibAttributes.rfStatistics, value, valueLen);
      break;
    case EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_VERSIONING:
      MEMCOPY(emAfRf4ceMsoLocalRibAttributes.versioning[index], value, valueLen);
      break;
    case EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_BATTERY_STATUS:
      MEMCOPY(emAfRf4ceMsoLocalRibAttributes.batteryStatus, value, valueLen);
      break;
    case EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_SHORT_RF_RETRY_PERIOD:
      MEMCOPY(emAfRf4ceMsoLocalRibAttributes.shortRfRetryPeriod, value, valueLen);
      break;
    case EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_IR_RF_DATABASE:
      emberAfPluginRf4ceMsoIncomingIrRfDatabaseAttributeCallback(emberAfRf4ceGetPairingIndex(),
                                                                 index,
                                                                 valueLen,
                                                                 value);
      break;
    case EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_VALIDATION_CONFIGURATION:
      MEMCOPY(emAfRf4ceMsoLocalRibAttributes.validationConfiguration, value, valueLen);
      break;
    case EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_GENERAL_PURPOSE:
      MEMCOPY(emAfRf4ceMsoLocalRibAttributes.generalPurpose[index], value, valueLen);
      break;
    default:
      assert(0);
    }
  }

  if (attributeId == EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_VALIDATION_CONFIGURATION) {
    emAfRf4ceMsoValidationConfigurationResponseCallback(status);
  }
}

static uint8_t msoAttributePeripheralIdEntryLookup(uint8_t deviceType)
{
  uint8_t i;
  for(i=0; i<EMBER_AF_PLUGIN_RF4CE_MSO_PERIPHERAL_ID_ENTRIES; i++) {
    if (emAfRf4ceMsoLocalRibAttributes.peripheralIds[i].deviceType == deviceType) {
      return i;
    }
  }

  return 0xFF;
}

static uint8_t msoAttributeGetUnusedPeripheralIdEntryIndex(void)
{
  return msoAttributePeripheralIdEntryLookup(0xFF);
}

static bool msoArrayedAttributeIndexIsValid(EmberAfRf4ceMsoAttributeId attributeId,
                                               uint8_t index)
{
  if (attributeId == EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_PERIPHERAL_IDS) {
    return (msoAttributePeripheralIdEntryLookup(index) != 0xFF
            || msoAttributeGetUnusedPeripheralIdEntryIndex() != 0xFF);
  } else if (attributeId == EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_IR_RF_DATABASE) {
    // Handled by application callback, we don't check the index here.
    return true;
  } else {
    return (index < emAfRf4ceMsoGetArrayedAttributeDimension(attributeId));
  }
}

#endif // EMBER_AF_PLUGIN_RF4CE_MSO_IS_ORIGINATOR
