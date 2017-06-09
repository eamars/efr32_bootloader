#include "af.h"
#include "rf4ce-mso.h"
#include "rf4ce-mso-internal.h"

void emberAfPluginRf4ceMsoNcpInitCallback(bool memoryAllocation)
{
}

EmberStatus emAfRf4ceMsoSetDiscoveryResponseUserString(void)
{
  EmberRf4ceApplicationInfo applicationInfo;
  uint8_t msoUserString[MSO_DISCOVERY_RESPONSE_MSO_USER_STRING_LENGTH]
    = EMBER_AF_PLUGIN_RF4CE_MSO_USER_STRING;

  // We are going to take the default application info and then rewrite the
  // user string to pass some specifics about ourself to the controller.  The
  // user string must be sent in the discovery response, so we explicitly set
  // the capability to include it, just in case the default application info
  // didn't have one.
  MEMCOPY(&applicationInfo,
          &emAfRf4ceApplicationInfo,
          sizeof(EmberRf4ceApplicationInfo));
  applicationInfo.capabilities |= EMBER_RF4CE_APP_CAPABILITIES_USER_STRING_BIT;
  MEMSET(applicationInfo.userString,
         0,
         EMBER_RF4CE_APPLICATION_USER_STRING_LENGTH);

  MEMCOPY((applicationInfo.userString
           + MSO_DISCOVERY_RESPONSE_MSO_USER_STRING_OFFSET),
          msoUserString,
          EMBER_AF_PLUGIN_RF4CE_MSO_USER_STRING_LENGTH);
  applicationInfo.userString[MSO_DISCOVERY_RESPONSE_TERTIARY_CLASS_DESCRIPTOR_OFFSET]
    = (EMBER_AF_PLUGIN_RF4CE_MSO_TERTIARY_CLASS_NUMBER
       | (EMBER_AF_PLUGIN_RF4CE_MSO_TERTIARY_DUPLICATE_CLASS_NUMBER_HANDLING
          << MSO_CLASS_DESCRIPTOR_DUPLICATE_CLASS_NUMBER_HANDLING_OFFSET)
       | MSO_TERTIARY_APPLY_STRICT_LQI_THRESHOLD_BIT
       | MSO_TERTIARY_ENABLE_REQUEST_AUTO_VALIDATION_BIT);
  applicationInfo.userString[MSO_DISCOVERY_RESPONSE_SECONDARY_CLASS_DESCRIPTOR_OFFSET]
    = (EMBER_AF_PLUGIN_RF4CE_MSO_SECONDARY_CLASS_NUMBER
       | (EMBER_AF_PLUGIN_RF4CE_MSO_SECONDARY_DUPLICATE_CLASS_NUMBER_HANDLING
          << MSO_CLASS_DESCRIPTOR_DUPLICATE_CLASS_NUMBER_HANDLING_OFFSET)
       | MSO_SECONDARY_APPLY_STRICT_LQI_THRESHOLD_BIT
       | MSO_SECONDARY_ENABLE_REQUEST_AUTO_VALIDATION_BIT);
  applicationInfo.userString[MSO_DISCOVERY_RESPONSE_PRIMARY_CLASS_DESCRIPTOR_OFFSET]
    = (EMBER_AF_PLUGIN_RF4CE_MSO_PRIMARY_CLASS_NUMBER
       | (EMBER_AF_PLUGIN_RF4CE_MSO_PRIMARY_DUPLICATE_CLASS_NUMBER_HANDLING
          << MSO_CLASS_DESCRIPTOR_DUPLICATE_CLASS_NUMBER_HANDLING_OFFSET)
       | MSO_PRIMARY_APPLY_STRICT_LQI_THRESHOLD_BIT
       | MSO_PRIMARY_ENABLE_REQUEST_AUTO_VALIDATION_BIT);
  applicationInfo.userString[MSO_DISCOVERY_RESPONSE_STRICT_LQI_THRESHOLD_OFFSET]
    = (EMBER_AF_PLUGIN_RF4CE_MSO_STRICT_LQI_THRESHOLD
       | MSO_FULL_ROLL_BACK_ENABLED_BIT);
  applicationInfo.userString[MSO_DISCOVERY_RESPONSE_BASIC_LQI_THRESHOLD_OFFSET]
    = EMBER_AF_PLUGIN_RF4CE_MSO_BASIC_DISCOVERY_LQI_THRESHOLD;

  return emberAfRf4ceSetApplicationInfo(&applicationInfo);
}
