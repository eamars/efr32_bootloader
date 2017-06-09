/*
 * File: command-handlers-rf4ce.h
 * Description: RF4CE NCP support macros and definitions.
 *
 * Author(s): Maurizio Nanni, maurizio.nanni@silabs.com
 *
 * Copyright 2013 Silicon Laboratories, Inc.                                *80*
 */

#ifndef __RF4CE_SUPPORT_H__
#define __RF4CE_SUPPORT_H__

#define RF4CE_PROFILE_ID_GDP             0x00
#define RF4CE_PROFILE_ID_ZRC_1_1         0x01
#define RF4CE_PROFILE_ID_ZID_1_0         0x02
#define RF4CE_PROFILE_ID_ZRC_2_0         0x03
#define RF4CE_PROFILE_ID_MSO             0xC0

#define RF4CE_DEVICE_TYPE_WILDCARD       0xFF

//------------------------------------------------------------------------------
// ZRC 1.1 defines

#define ZRC_1_1_APLC_MIN_KEY_EXCHANGE_TRANSFER_COUNT 3

//------------------------------------------------------------------------------
// MSO defines

// Primary class descriptor (1 byte) + secondary class descriptor (1 byte)
// + tertiary class descriptor (1 byte) + basic LQI threshold (1 byte) +
// + strict LQI threshold (1 byte)
#define MSO_SET_VALUE_BINDING_RECIPIENT_PARAMETERS_BYTES_LENGTH                5

#define MSO_USER_STRING_LENGTH                                                 9

// Discovery response user string fields
#define MSO_DISCOVERY_RESPONSE_MSO_USER_STRING_OFFSET                         0
#define MSO_DISCOVERY_RESPONSE_MSO_USER_STRING_LENGTH                         MSO_USER_STRING_LENGTH
#define MSO_DISCOVERY_RESPONSE_TERTIARY_CLASS_DESCRIPTOR_OFFSET               10
#define MSO_DISCOVERY_RESPONSE_SECONDARY_CLASS_DESCRIPTOR_OFFSET              11
#define MSO_DISCOVERY_RESPONSE_PRIMARY_CLASS_DESCRIPTOR_OFFSET                12
#define MSO_DISCOVERY_RESPONSE_STRICT_LQI_THRESHOLD_OFFSET                    13
#define MSO_DISCOVERY_RESPONSE_BASIC_LQI_THRESHOLD_OFFSET                     14

#define MSO_NWK_MAX_REPORTED_NODE_DESCRIPTORS                                 16
#define MSO_TERTIARY_CLASS_DESCRIPTOR_OFFSET                                  10
#define MSO_CLASS_DESCRIPTOR_CLASS_NUMBER_MASK                              0x0F
#define MSO_CLASS_DESCRIPTOR_CLASS_NUMBER_OFFSET                               0
#define MSO_CLASS_DESCRIPTOR_DUPLICATE_CLASS_NUMBER_HANDLING_MASK           0x30
#define MSO_CLASS_DESCRIPTOR_DUPLICATE_CLASS_NUMBER_HANDLING_OFFSET            4
#define MSO_CLASS_DESCRIPTOR_APPLY_STRICT_LQI_THRESHOLD_BIT                 0x40
#define MSO_CLASS_DESCRIPTOR_ENABLE_REQUEST_AUTO_VALIDATION_BIT             0x80
#define MSO_DUPLICATE_CLASS_NUMBER_HANDLING_USE_NODE_AS_IS                  0x00
#define MSO_DUPLICATE_CLASS_NUMBER_HANDLING_REMOVE_NODE_DESCRIPTOR          0x01
#define MSO_DUPLICATE_CLASS_NUMBER_HANDLING_RECLASSIFY_NODE_DESCRIPTOR      0x02
#define MSO_DUPLICATE_CLASS_NUMBER_HANDLING_ABORT_BINDING                   0x03

typedef struct {
  uint8_t primaryClassDescriptor;
  uint8_t secondaryClassDescriptor;
  uint8_t tertiaryClassDescriptor;
  uint8_t basicDiscoveryLqiThreshold;
  uint8_t strictDiscoveryLqiThreshold;
} EmMsoRecipientParameters;

//------------------------------------------------------------------------------
// GDP types and defines

#define GDP_APLC_MIN_KEY_EXCHANGE_TRANSFER_COUNT                               3

#define GDP_APPLICATION_SPECIFIC_USER_STRING_LENGTH                            8

// GDP bitmask defines
#define GDP_BITMASK_IS_RECIPIENT_BIT                                        0x01
#define GDP_BITMASK_PUSH_BUTTON_STIMULULS_RECEIVED_PENDING_BIT              0x02
#define GDP_BITMASK_SUPPORTS_PROXY_BINDING_BIT                              0x04

// Primary class descriptor (1 byte) + secondary class descriptor (1 byte)
// + tertiary class descriptor (1 byte) + discovery LQI threshold (1 byte)
#define GDP_SET_VALUE_BINDING_RECIPIENT_PARAMETERS_BYTES_LENGTH                4

#define GDP_SET_VALUE_FLAG_LENGTH                                              1

// GDP application specific user string.
#define GDP_USER_STRING_APPLICATION_SPECIFIC_USER_STRING_OFFSET   0
#define GDP_USER_STRING_APPLICATION_SPECIFIC_USER_STRING_LENGTH   8

// GDP Null byte delimiter
#define GDP_USER_STRING_NULL_BYTE_OFFSET                          8

// GDP discovery request user string bytes.
#define GDP_USER_STRING_DISC_REQUEST_VENDOR_ID_FILTER_OFFSET                    9
#define GDP_USER_STRING_DISC_REQUEST_VENDOR_ID_FILTER_LENGTH                    2
#define GDP_USER_STRING_DISC_REQUEST_MIN_MAX_CLASS_FILTER_OFFSET                11
#define GDP_USER_STRING_DISC_REQUEST_MIN_MAX_CLASS_FILTER_MIN_CLASS_NUM_MASK    0x0F
#define GDP_USER_STRING_DISC_REQUEST_MIN_MAX_CLASS_FILTER_MIN_CLASS_NUM_OFFSET  0
#define GDP_USER_STRING_DISC_REQUEST_MIN_MAX_CLASS_FILTER_MAX_CLASS_NUM_MASK    0xF0
#define GDP_USER_STRING_DISC_REQUEST_MIN_MAX_CLASS_FILTER_MAX_CLASS_NUM_OFFSET  4
#define GDP_USER_STRING_DISC_REQUEST_MIN_LQI_FILTER_OFFSET                      12
#define GDP_USER_STRING_DISC_REQUEST_RESERVED_BYTES_OFFSET                      13
#define GDP_USER_STRING_DISC_REQUEST_RESERVED_BYTES_LENGTH                      2

// GDP class descriptor
#define GDP_CLASS_DESCRIPTOR_NUMBER_MASK                                    0x0F
#define GDP_CLASS_DESCRIPTOR_NUMBER_OFFSET                                     0
#define GDP_CLASS_DESCRIPTOR_DUPLICATE_HANDLING_MASK                        0x30
#define GDP_CLASS_DESCRIPTOR_DUPLICATE_HANDLING_OFFSET                         4
#define GDP_CLASS_DESCRIPTOR_RESERVED_MASK                                  0xC0
// GDP class numbers
#define GDP_CLASS_NUMBER_PRE_COMMISSIONED                                   0x00
#define GDP_CLASS_NUMBER_BUTTON_PRESS_INDICATION                            0x01
// Values 0x2-0xE are implementation specific.
#define GDP_CLASS_NUMBER_DISCOVERABLE_ONLY                                  0x0F

// GDP class descriptor duplicate handling criteria.
#define GDP_CLASS_DESCRIPTOR_DUPLICATE_HANDLING_AS_IS                       0x00
#define GDP_CLASS_DESCRIPTOR_DUPLICATE_HANDLING_RECLASSIFY                  0x01
#define GDP_CLASS_DESCRIPTOR_DUPLICATE_HANDLING_ABORT                       0x02
#define GDP_CLASS_DESCRIPTOR_DUPLICATE_HANDLING_RESERVED                    0x03

#define GDP_VERSION_NONE   0x00
#define GDP_VERSION_1_X    0x01
#define GDP_VERSION_2_0    0x02

// For now the only GDP 1.x based profile is ZID
// When adding new profile IDs to this list, make sure the list stays sorted.
#define GDP_1_X_BASED_PROFILE_ID_LIST                                          \
  {RF4CE_PROFILE_ID_ZID_1_0}
#define GDP_1_X_BASED_PROFILE_ID_LIST_LENGTH      1

// For now the only GDP 2.0 based profile is ZRC 2.0
// When adding new profile IDs to this list, make sure the list stays sorted.
#define GDP_2_0_BASED_PROFILE_ID_LIST                                          \
  {RF4CE_PROFILE_ID_ZRC_2_0}
#define GDP_2_0_BASED_PROFILE_ID_LIST_LENGTH      1

typedef struct {
  uint8_t primaryClassDescriptor;
  uint8_t secondaryClassDescriptor;
  uint8_t tertiaryClassDescriptor;
  uint8_t discoveryLqiThreshold;
} EmGdpRecipientParameters;

typedef struct {
  EmberEUI64 srcIeeeAddr;
  uint8_t nodeCapabilities;
  EmberRf4ceVendorInfo vendorInfo;
  EmberRf4ceApplicationInfo appInfo;
  uint8_t searchDevType;
} EmGdpDiscoveryOrPairRequestData;

//------------------------------------------------------------------------------
// Stack callbacks handlers.

bool emRf4ceNcpDiscoveryRequestHandler(EmberEUI64 srcIeeeAddr,
                                          uint8_t nodeCapabilities,
                                          EmberRf4ceVendorInfo *vendorInfo,
                                          EmberRf4ceApplicationInfo *appInfo,
                                          uint8_t searchDevType,
                                          uint8_t rxLinkQuality);

bool emRf4ceNcpDiscoveryResponseHandler(bool atCapacity,
                                           uint8_t channel,
                                           EmberPanId panId,
                                           EmberEUI64 srcIeeeAddr,
                                           uint8_t nodeCapabilities,
                                           EmberRf4ceVendorInfo *vendorInfo,
                                           EmberRf4ceApplicationInfo *appInfo,
                                           uint8_t rxLinkQuality,
                                           uint8_t discRequestLqi);

void emRf4ceNcpDiscoveryCompleteHandler(EmberStatus status);

void emRf4ceNcpAutoDiscoveryResponseCompleteHandler(EmberStatus status,
                                                    EmberEUI64 srcIeeeAddr,
                                                    uint8_t nodeCapabilities,
                                                    EmberRf4ceVendorInfo *vendorInfo,
                                                    EmberRf4ceApplicationInfo *appInfo,
                                                    uint8_t searchDevType);

bool emRf4ceNcpPairRequestHandler(EmberStatus status,
                                     uint8_t pairingIndex,
                                     EmberEUI64 srcIeeeAddr,
                                     uint8_t nodeCapabilities,
                                     EmberRf4ceVendorInfo *vendorInfo,
                                     EmberRf4ceApplicationInfo *appInfo,
                                     uint8_t keyExchangeTransferCount);

void emRf4ceNcpPairCompleteHandler(EmberStatus status,
                                   uint8_t pairingIndex,
                                   EmberRf4ceVendorInfo *vendorInfo,
                                   EmberRf4ceApplicationInfo *appInfo);

void emRf4ceNcpUnpairHandler(uint8_t pairingIndex);

void emRf4ceNcpUnpairCompleteHandler(uint8_t pairingIndex);

//------------------------------------------------------------------------------
// Ezsp Command Handlers

EmberStatus emberAfEzspRf4ceDeletePairingTableEntryCommandCallback(uint8_t pairingIndex);

EmberStatus emberAfEzspRf4ceDiscoveryCommandCallback(EmberPanId panId,
                                                     EmberNodeId nodeId,
                                                     uint8_t searchDevType,
                                                     uint16_t discDuration,
                                                     uint8_t maxDiscRepetitions,
                                                     uint8_t discProfileIdListLength,
                                                     uint8_t *discProfileIdList);

EmberStatus emberAfEzspRf4ceEnableAutoDiscoveryResponseCommandCallback(uint16_t duration);

EmberStatus emberAfEzspRf4ceGetNetworkParametersCommandCallback(EmberNodeType* nodeType,
                                                                EmberNetworkParameters* parameters);

void emberAfPluginEzspRf4ceSetValueCommandCallback(EmberAfPluginEzspValueCommandContext* context);
void emberAfPluginEzspRf4ceGetValueCommandCallback(EmberAfPluginEzspValueCommandContext* context);
void emberAfPluginEzspRf4cePolicyCommandCallback(EmberAfPluginEzspPolicyCommandContext* context);
void emberAfPluginEzspRf4ceConfigurationValueCommandCallback(EmberAfPluginEzspConfigurationValueCommandContext* context);
void emberAfPluginEzspRf4ceModifyMemoryAllocationCallback(void);

#endif //__RF4CE_SUPPORT_H__
