// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"

#if defined(EMBER_SCRIPTED_TEST)
#include "rf4ce-gdp-test.h"
#endif // EMBER_SCRIPTED_TEST

#include "app/framework/plugin/rf4ce-profile/rf4ce-profile.h"
#include "rf4ce-gdp.h"
#include "rf4ce-gdp-internal.h"

void emberAfPluginRf4ceGdpNcpInitCallback(bool memoryAllocation)
{
}

void emAfRf4ceGdpSetPushButtonPendingReceivedFlag(bool set)
{
  // Nothing to do for SoC
}

void emAfRf4ceGdpSetProxyBindingFlag(bool set)
{
  // Nothing to do for SoC
}

EmberStatus emAfRf4ceGdpSetDiscoveryResponseAppInfo(bool pushButton,
                                                    uint8_t gdpVersion)
{
  EmberRf4ceApplicationInfo appInfo;
  uint8_t *finger = appInfo.userString;

  MEMCOPY(&appInfo,
          &emAfRf4ceApplicationInfo,
          sizeof(EmberRf4ceApplicationInfo));

  // Bytes 0-7 are "Application specific user string".
  MEMCOPY(finger,
          emAfRf4ceGdpApplicationSpecificUserString,
          USER_STRING_APPLICATION_SPECIFIC_USER_STRING_LENGTH);
  finger += USER_STRING_APPLICATION_SPECIFIC_USER_STRING_LENGTH;

  // Byte 8 is a NULL octet.
  *finger++ = 0x00;

  // Bytes 9-10 are reserved bytes (we set them to 0).
  *finger++ = 0;
  *finger++ = 0;

  // Vendors should use a weighted sum algorithm to calculate the class number
  // that the binding recipient should respond with, in an attempt to let the
  // binding originator pick the best pairing candidate first.
  // Example factors that a vendor could use are: uptime of the binding
  // recipient, IR or other line of sight detection, binding originator type
  // that sent the discovery request, number of pairing table entries, and
  // RSSI of the discovery request. Vendors should utilize the entire span of
  // classes between 0x02 and 0x0E when determining different ranking states.
  // A node with a low ranking should respond with a high class number.
  // TODO: we will add a callback for the application to properly set the
  // class numbers. For now, we use the class number generated at compile
  // time.

  // Byte 11: Tertiary class descriptor
  *finger++ = ((EMBER_AF_PLUGIN_RF4CE_GDP_TERTIARY_CLASS_NUMBER
                << CLASS_DESCRIPTOR_NUMBER_OFFSET)
               | (EMBER_AF_PLUGIN_RF4CE_GDP_TERTIARY_CLASS_DUPLICATE_HANDLING
                  << CLASS_DESCRIPTOR_DUPLICATE_HANDLING_OFFSET));

  // Byte 12: Secondary class descriptor
  *finger++ = ((EMBER_AF_PLUGIN_RF4CE_GDP_SECONDARY_CLASS_NUMBER
                << CLASS_DESCRIPTOR_NUMBER_OFFSET)
               | (EMBER_AF_PLUGIN_RF4CE_GDP_SECONDARY_CLASS_DUPLICATE_HANDLING
                  << CLASS_DESCRIPTOR_DUPLICATE_HANDLING_OFFSET));

  // Byte 13: Primary class descriptor.

  // If the 'push button stimulus flag' is pending the discovery response
  // should have:
  // - class number 'button press indication" in its primary class descriptor.
  // - a duplicate class number handling field set to 'abort binding' in
  //   its primary class descriptor.
  if (pushButton) {
    *finger++ = ((CLASS_NUMBER_BUTTON_PRESS_INDICATION
                  << CLASS_DESCRIPTOR_NUMBER_OFFSET)
                 | (CLASS_DESCRIPTOR_DUPLICATE_HANDLING_ABORT
                    << CLASS_DESCRIPTOR_DUPLICATE_HANDLING_OFFSET));
  } else {
    // If the 'push button stimulus flag' is not pending the discovery
    // response should have:
    // - the user string shall not contain a class descriptor with a class
    //   number 'button press indication' in any of its class descriptors.
    //   TODO: should we check this here or should we just rely on App Builder
    //         generating the correct values?
    // - TODO: the user string shall contain a class descriptor with class
    //   number 'discoverable only' in its primary class descriptor if no
    //   other class descriptor can be configured and if this does not violate
    //   the Maximum Class Number filter in the discovery request.

    // For now we just set it to the value defined in app builder.
    *finger++ = ((EMBER_AF_PLUGIN_RF4CE_GDP_PRIMARY_CLASS_NUMBER
                  << CLASS_DESCRIPTOR_NUMBER_OFFSET)
                 | (EMBER_AF_PLUGIN_RF4CE_GDP_PRIMARY_CLASS_DUPLICATE_HANDLING
                    << CLASS_DESCRIPTOR_DUPLICATE_HANDLING_OFFSET));
  }

  // Byte 14: Discovery LQI threshold
  *finger = EMBER_AF_PLUGIN_RF4CE_GDP_DISCOVERY_RESPONSE_LQI_THRESHOLD;

  // Initialize the profile ID list size to 0.
  appInfo.capabilities &=
        ~EMBER_RF4CE_APP_CAPABILITIES_SUPPORTED_PROFILES_MASK;

  // Add only GDP based supported profiles according to the passed version.
  emAfGdpAddToProfileIdList(emAfRf4ceApplicationInfo.profileIdList,
                            emberAfRf4ceProfileIdListLength(emAfRf4ceApplicationInfo.capabilities),
                            &appInfo,
                            gdpVersion);

  appInfo.capabilities |= EMBER_RF4CE_APP_CAPABILITIES_USER_STRING_BIT;

  return emberAfRf4ceSetApplicationInfo(&appInfo);
}

EmberStatus emAfRf4ceGdpSetPairResponseAppInfo(const EmberRf4ceApplicationInfo *pairRequestAppInfo)
{
  EmberRf4ceApplicationInfo appInfo;
   uint8_t appUserString[USER_STRING_APPLICATION_SPECIFIC_USER_STRING_LENGTH] =
       EMBER_AF_PLUGIN_RF4CE_GDP_APPLICATION_SPECIFIC_USER_STRING;
   uint8_t *finger = appInfo.userString;
   uint8_t matchingProfileIdList[EMBER_RF4CE_APPLICATION_PROFILE_ID_LIST_MAX_LENGTH];
   uint8_t matchingProfileIdListLength;

   MEMCOPY(&appInfo,
           &emAfRf4ceApplicationInfo,
           sizeof(EmberRf4ceApplicationInfo));

   // Bytes 0-7 are "Application specific user string".
   MEMCOPY(finger,
           appUserString,
           USER_STRING_APPLICATION_SPECIFIC_USER_STRING_LENGTH);
   finger += USER_STRING_APPLICATION_SPECIFIC_USER_STRING_LENGTH;

   // Byte 8-14 are reserved bytes (we set then to 0).
   MEMSET(finger, 0x00, 7);

   matchingProfileIdListLength =
       emAfCheckDeviceTypeAndProfileIdMatch(0xFF, // we don't bother matching the device type here.
                                            (uint8_t*)pairRequestAppInfo->deviceTypeList,
                                            emberAfRf4ceDeviceTypeListLength(pairRequestAppInfo->capabilities),
                                            emAfRf4ceApplicationInfo.profileIdList,
                                            emberAfRf4ceProfileIdListLength(emAfRf4ceApplicationInfo.capabilities),
                                            (uint8_t*)pairRequestAppInfo->profileIdList,
                                            emberAfRf4ceProfileIdListLength(pairRequestAppInfo->capabilities),
                                            matchingProfileIdList);

   // Set the profile ID list size.
   appInfo.capabilities &=
         ~EMBER_RF4CE_APP_CAPABILITIES_SUPPORTED_PROFILES_MASK;
   appInfo.capabilities |= (matchingProfileIdListLength <<
                            EMBER_RF4CE_APP_CAPABILITIES_SUPPORTED_PROFILES_OFFSET);
   // Set the profile ID list with only the profile IDs that match the incoming
   // pair request.
   MEMCOPY(appInfo.profileIdList,
           matchingProfileIdList,
           matchingProfileIdListLength);

   // Set the user string bit.
   appInfo.capabilities |= EMBER_RF4CE_APP_CAPABILITIES_USER_STRING_BIT;

   return emberAfRf4ceSetApplicationInfo(&appInfo);
}
