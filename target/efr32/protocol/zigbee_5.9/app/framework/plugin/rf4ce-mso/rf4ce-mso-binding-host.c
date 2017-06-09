#include "af.h"
#include "rf4ce-mso.h"
#include "rf4ce-mso-internal.h"


void emberAfPluginRf4ceMsoNcpInitCallback(bool memoryAllocation)
{

  if (!memoryAllocation) {
    uint8_t msoUserString[MSO_USER_STRING_LENGTH]
      = EMBER_AF_PLUGIN_RF4CE_MSO_USER_STRING;

    ezspSetValue(EZSP_VALUE_RF4CE_MSO_USER_STRING,
                 MSO_USER_STRING_LENGTH,
                 msoUserString);

#ifdef EMBER_AF_PLUGIN_RF4CE_MSO_IS_RECIPIENT
    {
      uint8_t bindingParams[MSO_SET_VALUE_BINDING_RECIPIENT_PARAMETERS_BYTES_LENGTH];

      // Primary class descriptor
      bindingParams[0] = (EMBER_AF_PLUGIN_RF4CE_MSO_PRIMARY_CLASS_NUMBER
                          | (EMBER_AF_PLUGIN_RF4CE_MSO_PRIMARY_DUPLICATE_CLASS_NUMBER_HANDLING
                             << MSO_CLASS_DESCRIPTOR_DUPLICATE_CLASS_NUMBER_HANDLING_OFFSET)
                          | MSO_PRIMARY_APPLY_STRICT_LQI_THRESHOLD_BIT
                          | MSO_PRIMARY_ENABLE_REQUEST_AUTO_VALIDATION_BIT);
      // Secondary class descriptor
      bindingParams[1] = (EMBER_AF_PLUGIN_RF4CE_MSO_SECONDARY_CLASS_NUMBER
                          | (EMBER_AF_PLUGIN_RF4CE_MSO_SECONDARY_DUPLICATE_CLASS_NUMBER_HANDLING
                             << MSO_CLASS_DESCRIPTOR_DUPLICATE_CLASS_NUMBER_HANDLING_OFFSET)
                          | MSO_SECONDARY_APPLY_STRICT_LQI_THRESHOLD_BIT
                          | MSO_SECONDARY_ENABLE_REQUEST_AUTO_VALIDATION_BIT);
      // Tertiary class descriptor
      bindingParams[2] = (EMBER_AF_PLUGIN_RF4CE_MSO_TERTIARY_CLASS_NUMBER
                          | (EMBER_AF_PLUGIN_RF4CE_MSO_TERTIARY_DUPLICATE_CLASS_NUMBER_HANDLING
                             << MSO_CLASS_DESCRIPTOR_DUPLICATE_CLASS_NUMBER_HANDLING_OFFSET)
                          | MSO_TERTIARY_APPLY_STRICT_LQI_THRESHOLD_BIT
                          | MSO_TERTIARY_ENABLE_REQUEST_AUTO_VALIDATION_BIT);
      // Basic LQI threshold
      bindingParams[3] = EMBER_AF_PLUGIN_RF4CE_MSO_BASIC_DISCOVERY_LQI_THRESHOLD;
      // Strict LQI threshold
      bindingParams[4] = (EMBER_AF_PLUGIN_RF4CE_MSO_STRICT_LQI_THRESHOLD
                          | MSO_FULL_ROLL_BACK_ENABLED_BIT);

      ezspSetValue(EZSP_VALUE_RF4CE_MSO_BINDING_RECIPIENT_PARAMETERS,
                   MSO_SET_VALUE_BINDING_RECIPIENT_PARAMETERS_BYTES_LENGTH,
                   bindingParams);
    }
#endif // EMBER_AF_PLUGIN_RF4CE_MSO_IS_RECIPIENT
  }
}

EmberStatus emAfRf4ceMsoSetDiscoveryResponseUserString(void)
{
  // Nothing to do at the HOST, the user string is set at the NCP.

  return EMBER_SUCCESS;
}
