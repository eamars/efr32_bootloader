// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"

#include "rf4ce-gdp-internal.h"

void emberAfPluginRf4ceGdpNcpInitCallback(bool memoryAllocation)
{
  if (!memoryAllocation) {
#if defined(EMBER_AF_PLUGIN_RF4CE_GDP_IS_RECIPIENT)
    // Push the recipient binding parameters to the NCP.
    uint8_t bindingParams[GDP_SET_VALUE_BINDING_RECIPIENT_PARAMETERS_BYTES_LENGTH];

    bindingParams[0] = ((EMBER_AF_PLUGIN_RF4CE_GDP_PRIMARY_CLASS_NUMBER
                         << CLASS_DESCRIPTOR_NUMBER_OFFSET)
                        | (EMBER_AF_PLUGIN_RF4CE_GDP_PRIMARY_CLASS_DUPLICATE_HANDLING
                           << CLASS_DESCRIPTOR_DUPLICATE_HANDLING_OFFSET));
    bindingParams[1] = ((EMBER_AF_PLUGIN_RF4CE_GDP_SECONDARY_CLASS_NUMBER
                         << CLASS_DESCRIPTOR_NUMBER_OFFSET)
                        | (EMBER_AF_PLUGIN_RF4CE_GDP_SECONDARY_CLASS_DUPLICATE_HANDLING
                           << CLASS_DESCRIPTOR_DUPLICATE_HANDLING_OFFSET));
    bindingParams[2] = ((EMBER_AF_PLUGIN_RF4CE_GDP_TERTIARY_CLASS_NUMBER
                         << CLASS_DESCRIPTOR_NUMBER_OFFSET)
                        | (EMBER_AF_PLUGIN_RF4CE_GDP_TERTIARY_CLASS_DUPLICATE_HANDLING
                           << CLASS_DESCRIPTOR_DUPLICATE_HANDLING_OFFSET));
    bindingParams[3] = EMBER_AF_PLUGIN_RF4CE_GDP_DISCOVERY_RESPONSE_LQI_THRESHOLD;

    ezspSetValue(EZSP_VALUE_RF4CE_GDP_BINDING_RECIPIENT_PARAMETERS,
                 GDP_SET_VALUE_BINDING_RECIPIENT_PARAMETERS_BYTES_LENGTH,
                 bindingParams);
#endif // EMBER_AF_PLUGIN_RF4CE_GDP_IS_RECIPIENT

    // Push the application specific user string to the NCP.
    ezspSetValue(EZSP_VALUE_RF4CE_GDP_APPLICATION_SPECIFIC_USER_STRING,
                 USER_STRING_APPLICATION_SPECIFIC_USER_STRING_LENGTH,
                 (uint8_t *)emAfRf4ceGdpApplicationSpecificUserString);
  }
}

void emAfRf4ceGdpSetPushButtonPendingReceivedFlag(bool set)
{
  ezspSetValue(EZSP_VALUE_RF4CE_GDP_PUSH_BUTTON_STIMULUS_RECEIVED_PENDING_FLAG,
               GDP_SET_VALUE_FLAG_LENGTH,
               (uint8_t *)&set);
}

void emAfRf4ceGdpSetProxyBindingFlag(bool set)
{
  ezspSetValue(EZSP_VALUE_RF4CE_GDP_BINDING_PROXY_FLAG,
               GDP_SET_VALUE_FLAG_LENGTH,
               (uint8_t *)&set);
}

EmberStatus emAfRf4ceGdpSetDiscoveryResponseAppInfo(bool pushButton,
                                                    uint8_t gdpVersion)
{
  // Nothing to do at the HOST, the application info is set at the NCP.

  return EMBER_SUCCESS;
}

EmberStatus emAfRf4ceGdpSetPairResponseAppInfo(const EmberRf4ceApplicationInfo *pairRequestAppInfo)
{
  // Nothing to do at the HOST, the application info is set at the NCP.

  return EMBER_SUCCESS;
}
