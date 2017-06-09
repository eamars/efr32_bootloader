// Copyright 2013 Silicon Laboratories, Inc.

#ifdef EMBER_AF_RF4CE_NODE_TYPE_CONTROLLER
  // This token stores the pairing index of the current active bind, if any.
  #define CREATOR_PLUGIN_RF4CE_MSO_ACTIVE_BIND_PAIRING_INDEX 0x8726
  #ifdef DEFINETOKENS
    DEFINE_BASIC_TOKEN(PLUGIN_RF4CE_MSO_ACTIVE_BIND_PAIRING_INDEX,
                       uint8_t,
                       0xFF)
  #endif
#endif

#ifdef EMBER_AF_RF4CE_NODE_TYPE_TARGET
  #define CREATOR_PLUGIN_RF4CE_MSO_VALIDATION_TABLE 0x8727
  #ifdef DEFINETYPES
    #include "rf4ce-mso-types.h"
    typedef struct {
      EmberAfRf4ceMsoValidationState state;
      EmberAfRf4ceMsoCheckValidationStatus status;
    } tokTypePluginRf4ceMsoValidation;
  #endif
  #ifdef DEFINETOKENS
    DEFINE_INDEXED_TOKEN(PLUGIN_RF4CE_MSO_VALIDATION_TABLE,
                         tokTypePluginRf4ceMsoValidation,
                         EMBER_RF4CE_PAIRING_TABLE_SIZE,
                         {EMBER_AF_RF4CE_MSO_VALIDATION_STATE_NOT_VALIDATED,
                          EMBER_AF_RF4CE_MSO_CHECK_VALIDATION_STATUS_FAILURE})
  #endif

  #define CREATOR_PLUGIN_RF4CE_MSO_ATTRIBUTE_PERIPHERAL_IDS               0x8728
  #define CREATOR_PLUGIN_RF4CE_MSO_ATTRIBUTE_RF_STATISTICS                0x8729
  #define CREATOR_PLUGIN_RF4CE_MSO_ATTRIBUTE_VERSIONING                   0x8730
  #define CREATOR_PLUGIN_RF4CE_MSO_ATTRIBUTE_BATTERY_STATUS               0x8731
  #define CREATOR_PLUGIN_RF4CE_MSO_ATTRIBUTE_SHOR_RF_RETRY_PERIOD         0x8732
  #define CREATOR_PLUGIN_RF4CE_MSO_ATTRIBUTE_VALIDATION_CONFIGURATION     0x8733
  #define CREATOR_PLUGIN_RF4CE_MSO_ATTRIBUTE_GENERAL_PURPOSE              0x8734

  #ifdef DEFINETYPES
    #include "rf4ce-mso-types.h"
    #include "rf4ce-mso-attributes.h"
    typedef EmAfRf4ceMsoPeripheralIdEntry tokMsoAttributePeripheralIds[EMBER_AF_PLUGIN_RF4CE_MSO_PERIPHERAL_ID_ENTRIES];
    typedef uint8_t tokMsoAttributeRfStatistics[MSO_RIB_ATTRIBUTE_RF_STATISTICS_LENGTH];
    typedef uint8_t tokMsoAttributeVersioning[MSO_ATTRIBUTE_VERSIONING_ENTRIES][MSO_RIB_ATTRIBUTE_VERSIONING_LENGTH];
    typedef uint8_t tokMsoAttributeBatteryStatus[MSO_RIB_ATTRIBUTE_BATTERY_STATUS_LENGTH];
    typedef uint8_t tokMsoAttributeShortRfRetryPeriod[MSO_RIB_ATTRIBUTE_SHORT_RF_RETRY_PERIOD_LENGTH];
    typedef uint8_t tokMsoAttributeValidationConfiguration[MSO_RIB_ATTRIBUTE_VALIDATION_CONFIGURATION_LENGTH];
    typedef uint8_t tokMsoAttributeGeneralPurpose[EMBER_AF_PLUGIN_RF4CE_MSO_GENERAL_PURPOSE_ENTRIES][MSO_RIB_ATTRIBUTE_GENERAL_PURPOSE_LENGTH];
  #endif
  #ifdef DEFINETOKENS
    DEFINE_INDEXED_TOKEN(PLUGIN_RF4CE_MSO_ATTRIBUTE_PERIPHERAL_IDS,
                         tokMsoAttributePeripheralIds,
                         EMBER_RF4CE_PAIRING_TABLE_SIZE,
                         {{0xFF, {0xFF, 0xFF, 0xFF, 0xFF}}})
    DEFINE_INDEXED_TOKEN(PLUGIN_RF4CE_MSO_ATTRIBUTE_RF_STATISTICS,
                         tokMsoAttributeRfStatistics,
                         EMBER_RF4CE_PAIRING_TABLE_SIZE,
                         {0,})
    DEFINE_INDEXED_TOKEN(PLUGIN_RF4CE_MSO_ATTRIBUTE_VERSIONING,
                         tokMsoAttributeVersioning,
                         EMBER_RF4CE_PAIRING_TABLE_SIZE,
                         {{0,}})
    DEFINE_INDEXED_TOKEN(PLUGIN_RF4CE_MSO_ATTRIBUTE_BATTERY_STATUS,
                         tokMsoAttributeBatteryStatus,
                         EMBER_RF4CE_PAIRING_TABLE_SIZE,
                         {0,})
    DEFINE_INDEXED_TOKEN(PLUGIN_RF4CE_MSO_ATTRIBUTE_SHOR_RF_RETRY_PERIOD,
                         tokMsoAttributeShortRfRetryPeriod,
                         EMBER_RF4CE_PAIRING_TABLE_SIZE,
                         {0,})
    DEFINE_INDEXED_TOKEN(PLUGIN_RF4CE_MSO_ATTRIBUTE_VALIDATION_CONFIGURATION,
                         tokMsoAttributeValidationConfiguration,
                         EMBER_RF4CE_PAIRING_TABLE_SIZE,
                         {LOW_BYTE(EMBER_AF_PLUGIN_RF4CE_MSO_LINK_LOST_WAIT_TIME_MS),
                          HIGH_BYTE(EMBER_AF_PLUGIN_RF4CE_MSO_LINK_LOST_WAIT_TIME_MS),
                          LOW_BYTE(EMBER_AF_PLUGIN_RF4CE_MSO_AUTO_CHECK_VALIDATION_PERIOD_MS),
                          HIGH_BYTE(EMBER_AF_PLUGIN_RF4CE_MSO_AUTO_CHECK_VALIDATION_PERIOD_MS)})
    DEFINE_INDEXED_TOKEN(PLUGIN_RF4CE_MSO_ATTRIBUTE_GENERAL_PURPOSE,
                         tokMsoAttributeGeneralPurpose,
                         EMBER_RF4CE_PAIRING_TABLE_SIZE,
                         {{0,}})
  #endif

#endif
