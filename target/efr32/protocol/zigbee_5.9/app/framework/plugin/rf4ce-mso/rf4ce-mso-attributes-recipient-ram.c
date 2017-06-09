// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"

#include "rf4ce-mso.h"
#include "rf4ce-mso-internal.h"
#include "rf4ce-mso-attributes.h"

#ifdef EMBER_SCRIPTED_TEST
#include "rf4ce-mso-test.h"
#endif // EMBER_SCRIPTED_TEST

#if defined(EMBER_AF_PLUGIN_RF4CE_MSO_IS_RECIPIENT)

EmAfRf4ceMsoRibAttributes emAfRf4ceMsoRibAttributes[EMBER_RF4CE_PAIRING_TABLE_SIZE];

// Attributes stored in RAM are always re-initialized upon start-up.
void emAfRf4ceMsoInitAttributes(void)
{
  uint8_t i, j;

  MEMSET(emAfRf4ceMsoRibAttributes,
         0x00,
         sizeof(EmAfRf4ceMsoRibAttributes)*EMBER_RF4CE_PAIRING_TABLE_SIZE);

  // Initialize the device type of all the peripheral ID entries to 0xFF, which
  // means "unused" in this context.
  for(i=0; i<EMBER_RF4CE_PAIRING_TABLE_SIZE; i++) {
    for(j=0; j<EMBER_AF_PLUGIN_RF4CE_MSO_PERIPHERAL_ID_ENTRIES; j++) {
      emAfRf4ceMsoRibAttributes[i].peripheralIds[j].deviceType = 0xFF;
    }

    // Initialize the validation configuration attribute fields to the default
    // values.
    emAfRf4ceMsoRibAttributes[i].validationConfiguration[0] =
        LOW_BYTE(EMBER_AF_PLUGIN_RF4CE_MSO_LINK_LOST_WAIT_TIME_MS);
    emAfRf4ceMsoRibAttributes[i].validationConfiguration[1] =
        HIGH_BYTE(EMBER_AF_PLUGIN_RF4CE_MSO_LINK_LOST_WAIT_TIME_MS);
    emAfRf4ceMsoRibAttributes[i].validationConfiguration[2] =
        LOW_BYTE(EMBER_AF_PLUGIN_RF4CE_MSO_AUTO_CHECK_VALIDATION_PERIOD_MS);
    emAfRf4ceMsoRibAttributes[i].validationConfiguration[3] =
        HIGH_BYTE(EMBER_AF_PLUGIN_RF4CE_MSO_AUTO_CHECK_VALIDATION_PERIOD_MS);
  }
}

uint8_t emAfRf4ceMsoAttributePeripheralIdEntryLookup(uint8_t pairingIndex, uint8_t deviceType)
{
  uint8_t i;
  for(i=0; i<EMBER_AF_PLUGIN_RF4CE_MSO_PERIPHERAL_ID_ENTRIES; i++) {
    if (emAfRf4ceMsoRibAttributes[pairingIndex].peripheralIds[i].deviceType
        == deviceType) {
      return i;
    }
  }

  return 0xFF;
}

uint8_t emAfRf4ceMsoAttributeGetUnusedPeripheralIdEntryIndex(uint8_t pairingIndex)
{
  return emAfRf4ceMsoAttributePeripheralIdEntryLookup(pairingIndex, 0xFF);
}

// These assume sanity check for the passed parameters has been performed.

void emAfRf4ceMsoWriteAttributeRecipient(uint8_t pairingIndex,
                                         EmberAfRf4ceMsoAttributeId attributeId,
                                         uint8_t index,
                                         uint8_t valueLen,
                                         const uint8_t *value)
{
  switch(attributeId) {
  case EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_PERIPHERAL_IDS:
  {
    uint8_t entryIndex = emAfRf4ceMsoAttributePeripheralIdEntryLookup(pairingIndex,
                                                                    index);
    if (entryIndex == 0xFF) {
      entryIndex = emAfRf4ceMsoAttributeGetUnusedPeripheralIdEntryIndex(pairingIndex);
    }

    emAfRf4ceMsoRibAttributes[pairingIndex].peripheralIds[entryIndex].deviceType
      = index;
    MEMCOPY(emAfRf4ceMsoRibAttributes[pairingIndex].peripheralIds[entryIndex].peripheralId,
            value,
            valueLen);
  }
    break;
  case EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_RF_STATISTICS:
    MEMCOPY(emAfRf4ceMsoRibAttributes[pairingIndex].rfStatistics,
            value,
            valueLen);
    break;
  case EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_VERSIONING:
    MEMCOPY(emAfRf4ceMsoRibAttributes[pairingIndex].versioning[index],
            value,
            valueLen);
    break;
  case EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_BATTERY_STATUS:
    MEMCOPY(emAfRf4ceMsoRibAttributes[pairingIndex].batteryStatus,
            value,
            valueLen);
    break;
  case EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_SHORT_RF_RETRY_PERIOD:
    MEMCOPY(emAfRf4ceMsoRibAttributes[pairingIndex].shortRfRetryPeriod,
            value,
            valueLen);
    break;
  //case EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_IR_RF_DATABASE:
  // IR_RF database attributes can't be written remotely.
  //  break;
  case EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_VALIDATION_CONFIGURATION:
    MEMCOPY(emAfRf4ceMsoRibAttributes[pairingIndex].validationConfiguration,
            value,
            valueLen);
    break;
  case EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_GENERAL_PURPOSE:
    MEMCOPY(emAfRf4ceMsoRibAttributes[pairingIndex].generalPurpose[index],
            value,
            valueLen);
    break;
  default:
    assert(0);
  }
}

void emAfRf4ceMsoReadAttributeRecipient(uint8_t pairingIndex,
                                        EmberAfRf4ceMsoAttributeId attributeId,
                                        uint8_t index,
                                        uint8_t *valueLen,
                                        uint8_t *value)
{
  // We know the length of all the attributes except the IR-RF Database, which
  // is managed externally.
  if (attributeId != EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_IR_RF_DATABASE) {
    *valueLen = emAfRf4ceMsoGetAttributeLength(attributeId);
  }
  switch(attributeId) {
  case EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_PERIPHERAL_IDS:
  {
    uint8_t entryIndex = emAfRf4ceMsoAttributePeripheralIdEntryLookup(pairingIndex,
                                                                    index);
    MEMCOPY(value,
            emAfRf4ceMsoRibAttributes[pairingIndex].peripheralIds[entryIndex].peripheralId,
            *valueLen);
  }
    break;
  case EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_RF_STATISTICS:
    MEMCOPY(value,
            emAfRf4ceMsoRibAttributes[pairingIndex].rfStatistics,
            *valueLen);
    break;
  case EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_VERSIONING:
    MEMCOPY(value,
            emAfRf4ceMsoRibAttributes[pairingIndex].versioning[index],
            *valueLen);
    break;
  case EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_BATTERY_STATUS:
    MEMCOPY(value,
            emAfRf4ceMsoRibAttributes[pairingIndex].batteryStatus,
            *valueLen);
    break;
  case EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_SHORT_RF_RETRY_PERIOD:
    MEMCOPY(value,
            emAfRf4ceMsoRibAttributes[pairingIndex].shortRfRetryPeriod,
            *valueLen);
    break;
  case EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_IR_RF_DATABASE:
    emberAfPluginRf4ceMsoGetIrRfDatabaseAttributeCallback(pairingIndex,
                                                          index,
                                                          valueLen,
                                                          value);
    break;
  case EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_VALIDATION_CONFIGURATION:
    MEMCOPY(value,
            emAfRf4ceMsoRibAttributes[pairingIndex].validationConfiguration,
            *valueLen);
    break;
  case EMBER_AF_RF4CE_MSO_ATTRIBUTE_ID_GENERAL_PURPOSE:
    MEMCOPY(value,
            emAfRf4ceMsoRibAttributes[pairingIndex].generalPurpose[index],
            *valueLen);
    break;
  default:
    assert(0);
  }
}

#endif // EMBER_AF_PLUGIN_RF4CE_MSO_IS_RECIPIENT
