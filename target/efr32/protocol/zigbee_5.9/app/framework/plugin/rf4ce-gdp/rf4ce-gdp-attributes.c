// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"

#if defined(EMBER_SCRIPTED_TEST)
#include "rf4ce-gdp-test.h"
#endif // EMBER_SCRIPTED_TEST

#include "app/framework/plugin/rf4ce-profile/rf4ce-profile.h"
#include "rf4ce-gdp.h"
#include "rf4ce-gdp-attributes.h"
#include "rf4ce-gdp-poll.h"
#include "rf4ce-gdp-identification.h"
#include "rf4ce-gdp-internal.h"

// This code handles all the attributes-related commands that involve GDP
// attributes.

//------------------------------------------------------------------------------
// External declarations.

// GDP messages dispatchers.
bool emAfRf4ceGdpIncomingGetAttributesResponseOriginatorCallback(void);
bool emAfRf4ceGdpIncomingPullAttributesResponseOriginatorCallback(void);
bool emAfRf4ceGdpIncomingPushAttributesPollNegotiationCallback(void);
bool emAfRf4ceGdpIncomingPullAttributesPollNegotiationCallback(void);
bool emAfRf4ceGdpIncomingPullAttributesResponsePollNegotiationCallback(void);
bool emAfRf4ceGdpIncomingPushAttributesIdentificationCallback(void);
#ifdef EMBER_AF_PLUGIN_RF4CE_GDP_IS_RECIPIENT
bool emAfRf4ceGdpIncomingPushAttributesRecipientCallback(void);
bool emAfRf4ceGdpIncomingGetAttributesRecipientCallback(void);
bool emAfRf4ceGdpIncomingPullAttributesRecipientCallback(void);
#endif

//------------------------------------------------------------------------------
// Forward declarations.

static bool acceptIncomingAttributeCommand(void);
static const EmAfRf4ceGdpAttributeDescriptor *getAttributeDescriptor(uint8_t attrId);
static void handleIncomingGetOrPullAttributesCommand(bool isGet);
static void handleIncomingGetOrPullAttributesResponseCommand(bool isGet);
static void handleIncomingSetOrPushAttributesCommand(bool isSet);

#if defined(EMBER_SCRIPTED_TEST)
static bool arrayedAttributeIsEntryIdValid(uint8_t attrId,
                                              uint16_t entryId);
#endif

//------------------------------------------------------------------------------
// Attribute tables declaration.

// Local node's GDP attributes.
EmAfRf4ceGdpAttributes emAfRf4ceGdpLocalNodeAttributes;

// We maintain a set of attributes for a generic "remote node" to facilitate the
// binding process.
EmAfRf4ceGdpAttributes emAfRf4ceGdpRemoteNodeAttributes;

//------------------------------------------------------------------------------
// Local macros.

#define isAttributeSupported(attrId)                                           \
  (getAttributeDescriptor(attrId) != NULL)

#define getAttributeSize(attrId)                                               \
  (getAttributeDescriptor(attrId)->size)

#define attributeHasRemoteSetAccess(attrId)                                    \
    ((getAttributeDescriptor(attrId)->bitmask                                  \
      & ATTRIBUTE_HAS_REMOTE_SET_ACCESS_BIT) > 0)

#define attributeHasRemoteGetAccess(attrId)                                    \
    ((getAttributeDescriptor(attrId)->bitmask                                  \
      & ATTRIBUTE_HAS_REMOTE_GET_ACCESS_BIT) > 0)

#define attributeHasRemotePullAccess(attrId)                                   \
    ((getAttributeDescriptor(attrId)->bitmask                                  \
      & ATTRIBUTE_HAS_REMOTE_PULL_ACCESS_BIT) > 0)

#define attributeHasRemotePushAccess(attrId)                                   \
    ((getAttributeDescriptor(attrId)->bitmask                                  \
      & ATTRIBUTE_HAS_REMOTE_PUSH_ACCESS_BIT) > 0)

// ------------------------------------------------------------------------------
// Init callback.

// Initialize the local GDP attributes to their respective default values.
void emAfRf4ceGdpAttributesInitCallback(void)
{
#if (defined(EMBER_AF_PLUGIN_RF4CE_GDP_IS_POLL_CLIENT) || defined(EMBER_SCRIPTED_TEST))
  uint8_t pollConstraintsDefault[APL_GDP_POLL_CONSTRAINTS_SIZE] =
      APL_POLL_CONSTRAINTS_DEFAULT;
#endif

  emAfRf4ceGdpLocalNodeAttributes.gdpVersion = APL_GDP_VERSION_DEFAULT;
  emAfRf4ceGdpLocalNodeAttributes.gdpCapabilities =
      (0  // gdpCapabilities
#if defined(EMBER_AF_PLUGIN_RF4CE_GDP_ENHANCED_SECURITY)
       | GDP_CAPABILITIES_SUPPORT_ENHANCED_SECURITY_BIT
#endif
#if defined(EMBER_AF_PLUGIN_RF4CE_GDP_IS_POLL_CLIENT)
       | GDP_CAPABILITIES_SUPPORT_POLL_CLIENT_BIT
#elif defined(EMBER_AF_PLUGIN_RF4CE_GDP_IS_POLL_SERVER)
       | GDP_CAPABILITIES_SUPPORT_POLL_SERVER_BIT
#endif
#if defined(EMBER_AF_PLUGIN_RF4CE_GDP_IS_IDENTIFICATION_CLIENT)
       | GDP_CAPABILITIES_SUPPORT_IDENTIFICATION_CLIENT_BIT
#elif defined(EMBER_AF_PLUGIN_RF4CE_GDP_IS_IDENTIFICATION_SERVER)
       | GDP_CAPABILITIES_SUPPORT_IDENTIFICATION_SERVER_BIT
#endif
#if defined(EMBER_AF_PLUGIN_RF4CE_GDP_EXTENDED_VALIDATION)
       | GDP_CAPABILITIES_SUPPORT_EXTENDED_VALIDATION_BIT
#endif
      );
  emAfRf4ceGdpLocalNodeAttributes.powerStatus = APL_GDP_POWER_STATUS_DEFAULT;
  emAfRf4ceGdpLocalNodeAttributes.autoCheckValidationPeriod =
      APL_GDP_AUTO_CHECK_VALIDATION_PERIOD_DEFAULT;
  emAfRf4ceGdpLocalNodeAttributes.linkLostWaitTime =
      APL_GDP_LINK_LOST_WAIT_TIME_DEFAULT;
  emAfRf4ceGdpLocalNodeAttributes.identificationCapabilities =
      EMBER_AF_PLUGIN_RF4CE_GDP_IDENTIFICATION_CAPABILITIES;

#if (defined(EMBER_AF_PLUGIN_RF4CE_GDP_IS_POLL_CLIENT) || defined(EMBER_SCRIPTED_TEST))
  MEMMOVE(emAfRf4ceGdpLocalNodeAttributes.pollConstraints,
          pollConstraintsDefault,
          APL_GDP_POLL_CONSTRAINTS_SIZE);
#else
  MEMSET(emAfRf4ceGdpLocalNodeAttributes.pollConstraints,
         0x00,
         APL_GDP_POLL_CONSTRAINTS_SIZE);
#endif

  MEMSET(emAfRf4ceGdpLocalNodeAttributes.pollConfiguration,
         0x00,
         APL_GDP_POLL_CONFIGURATION_SIZE);

  // Test attributes
#if defined(EMBER_SCRIPTED_TEST)
  {
    uint8_t i,j;
    for(i=0; i<GDP_ATTRIBUTE_ONE_DIMENSIONAL_ARRAY_TEST_DIMENSION; i++) {
      emAfRf4ceGdpLocalNodeAttributes.oneDimensionalTestAttribute[i] =
          GDP_ATTRIBUTE_ONE_DIMENSIONAL_ARRAY_TEST_DEFAULT;
    }

    for(i=0; i<GDP_ATTRIBUTE_TWO_DIMENSIONAL_ARRAY_TEST_FIRST_DIMENSION; i++) {
      for(j=0; j<GDP_ATTRIBUTE_TWO_DIMENSIONAL_ARRAY_TEST_SECOND_DIMENSION; j++) {
        emAfRf4ceGdpLocalNodeAttributes.twoDimensionalTestAttribute[i][j] =
            GDP_ATTRIBUTE_TWO_DIMENSIONAL_ARRAY_TEST_DEFAULT;
      }
    }
  }
#endif
}

// ------------------------------------------------------------------------------
// Incoming attributes-related commands handlers.

void emAfRf4ceGdpIncomingGetAttributes(void)
{
  bool processCommand = false;

#ifdef EMBER_AF_PLUGIN_RF4CE_GDP_IS_RECIPIENT
  // See if the binding code wants to accept this GetAttributes() command.
  processCommand = emAfRf4ceGdpIncomingGetAttributesRecipientCallback();
#endif

  if (processCommand || acceptIncomingAttributeCommand()) {
    handleIncomingGetOrPullAttributesCommand(true);
  }
}

void emAfRf4ceGdpIncomingGetAttributesResponse(void)
{
  // See if the binding code wants to accept this GetAttributesResponse().
  if (emAfRf4ceGdpIncomingGetAttributesResponseOriginatorCallback()
      || acceptIncomingAttributeCommand()) {
    handleIncomingGetOrPullAttributesResponseCommand(true);
  }
}

// The PushAttributes command frame allows a node to push the value of one or
// more of its attributes to a remote node.
void emAfRf4ceGdpIncomingPushAttributes(void)
{
  bool processCommand = false;

#ifdef EMBER_AF_PLUGIN_RF4CE_GDP_IS_RECIPIENT
  // See if the binding code wants to accept this PushAttributes() command.
  processCommand = emAfRf4ceGdpIncomingPushAttributesRecipientCallback();
  emAfRf4ceGdpResetFetchAttributeFinger();
#endif

  // See if the poll negotiation code wants to accept this PushAttributes()
  // command.
  processCommand = (processCommand
                    || emAfRf4ceGdpIncomingPushAttributesPollNegotiationCallback());

  emAfRf4ceGdpResetFetchAttributeFinger();

  // See if the identification code wants to accept this PushAttributes()
  // command.
  processCommand = (processCommand
                    || emAfRf4ceGdpIncomingPushAttributesIdentificationCallback());

  if (processCommand) {
    handleIncomingSetOrPushAttributesCommand(false);
  }
}

void emAfRf4ceGdpIncomingPullAttributes(void)
{
  bool processCommand = false;

#ifdef EMBER_AF_PLUGIN_RF4CE_GDP_IS_RECIPIENT
  // See if the binding code wants to accept this PullAttributes() command.
  processCommand = emAfRf4ceGdpIncomingPullAttributesRecipientCallback();
  emAfRf4ceGdpResetFetchAttributeFinger();
#endif

  // See if the poll negotiation code wants to accept this PullAttributes()
  // command.
  processCommand = (processCommand
                    || emAfRf4ceGdpIncomingPullAttributesPollNegotiationCallback());

  if (processCommand) {
    handleIncomingGetOrPullAttributesCommand(false);
  }
}

void emAfRf4ceGdpIncomingPullAttributesResponse(void)
{
  // - See if the binding code wants to accept this PullAttributesResponse()
  //   command.
  // - See if the poll negotiation code wants to accept this
  //   PullAttributesResponse() command.
  bool processCommand =
      (emAfRf4ceGdpIncomingPullAttributesResponseOriginatorCallback()
       || emAfRf4ceGdpIncomingPullAttributesResponsePollNegotiationCallback());

  if (processCommand) {
    handleIncomingGetOrPullAttributesResponseCommand(false);
  }
}

void emAfRf4ceGdpIncomingSetAttributes(void)
{
  // The SetAttributes command frame allows a node to write the value of one or
  // more attributes of a remote node.
  if (acceptIncomingAttributeCommand()) {
    handleIncomingSetOrPushAttributesCommand(true);
  }
}

//------------------------------------------------------------------------------
// internal APIs.

void emAfRf4ceGdpClearRemoteAttributes(void)
{
  MEMSET(&emAfRf4ceGdpRemoteNodeAttributes,
         0x00,
         sizeof(EmAfRf4ceGdpAttributes));
}

// Don't call this function directly, use the macros defined in
// rf4ce-gdp-attributes.h instead.
void emAfRf4ceGdpGetOrSetAttribute(EmAfRf4ceGdpAttributes *attributes,
                                   uint8_t attrId,
                                   uint16_t entryId,
                                   bool isGet,
                                   uint8_t *val)
{
  if (isGet) {
    switch (attrId) {
    case EMBER_AF_RF4CE_GDP_ATTRIBUTE_VERSION:
      val[0] = LOW_BYTE(attributes->gdpVersion);
      val[1] = HIGH_BYTE(attributes->gdpVersion);
      break;
    case EMBER_AF_RF4CE_GDP_ATTRIBUTE_CAPABILITIES:
      val[0] = BYTE_0(attributes->gdpCapabilities);
      val[1] = BYTE_1(attributes->gdpCapabilities);
      val[2] = BYTE_2(attributes->gdpCapabilities);
      val[3] = BYTE_3(attributes->gdpCapabilities);
      break;
    case EMBER_AF_RF4CE_GDP_ATTRIBUTE_POWER_STATUS:
      val[0] = attributes->powerStatus;
      break;
    case EMBER_AF_RF4CE_GDP_ATTRIBUTE_POLL_CONSTRAINTS:
      MEMMOVE(val,
              attributes->pollConstraints,
              APL_GDP_POLL_CONSTRAINTS_SIZE);
      break;
    case EMBER_AF_RF4CE_GDP_ATTRIBUTE_POLL_CONFIGURATION:
      MEMMOVE(val,
              attributes->pollConfiguration,
              APL_GDP_POLL_CONFIGURATION_SIZE);
      break;
    case EMBER_AF_RF4CE_GDP_ATTRIBUTE_AUTO_CHECK_VALIDATION_PERIOD:
      val[0] = LOW_BYTE(attributes->autoCheckValidationPeriod);
      val[1] = HIGH_BYTE(attributes->autoCheckValidationPeriod);
      break;
    case EMBER_AF_RF4CE_GDP_ATTRIBUTE_LINK_LOST_WAIT_TIME:
      val[0] = LOW_BYTE(attributes->linkLostWaitTime);
      val[1] = HIGH_BYTE(attributes->linkLostWaitTime);
      break;
    case EMBER_AF_RF4CE_GDP_ATTRIBUTE_IDENTIFICATION_CAPABILITIES:
      val[0] = attributes->identificationCapabilities;
      break;
#if defined(EMBER_SCRIPTED_TEST)
    case EMBER_AF_RF4CE_GDP_ATTRIBUTE_SETTABLE_SCALAR_TEST_1:
      val[0] = LOW_BYTE(attributes->settableScalarTest1);
      val[1] = HIGH_BYTE(attributes->settableScalarTest1);
      break;
    case EMBER_AF_RF4CE_GDP_ATTRIBUTE_SETTABLE_SCALAR_TEST_2:
      val[0] = LOW_BYTE(attributes->settableScalarTest2);
      val[1] = HIGH_BYTE(attributes->settableScalarTest2);
      break;
    case EMBER_AF_RF4CE_GDP_ATTRIBUTE_ONE_DIMENSIONAL_ARRAY_TEST:
      val[0] = LOW_BYTE(attributes->oneDimensionalTestAttribute[entryId]);
      val[1] = HIGH_BYTE(attributes->oneDimensionalTestAttribute[entryId]);
      break;
    case EMBER_AF_RF4CE_GDP_ATTRIBUTE_TWO_DIMENSIONAL_ARRAY_TEST:
      val[0] = BYTE_0(attributes->twoDimensionalTestAttribute[entryId & 0xFF][((entryId & 0xFF00) >> 8)]);
      val[1] = BYTE_1(attributes->twoDimensionalTestAttribute[entryId & 0xFF][((entryId & 0xFF00) >> 8)]);
      val[2] = BYTE_2(attributes->twoDimensionalTestAttribute[entryId & 0xFF][((entryId & 0xFF00) >> 8)]);
      val[3] = BYTE_3(attributes->twoDimensionalTestAttribute[entryId & 0xFF][((entryId & 0xFF00) >> 8)]);
      break;
#endif
    }
  } else {
    switch (attrId) {
    case EMBER_AF_RF4CE_GDP_ATTRIBUTE_VERSION:
      attributes->gdpVersion = HIGH_LOW_TO_INT(val[1], val[0]);
      break;
    case EMBER_AF_RF4CE_GDP_ATTRIBUTE_CAPABILITIES:
      attributes->gdpCapabilities = emberFetchLowHighInt32u(val);
      break;
    case EMBER_AF_RF4CE_GDP_ATTRIBUTE_POWER_STATUS:
      attributes->powerStatus = val[0];
      break;
    case EMBER_AF_RF4CE_GDP_ATTRIBUTE_POLL_CONSTRAINTS:
      MEMMOVE(attributes->pollConstraints,
              val,
              APL_GDP_POLL_CONSTRAINTS_SIZE);
      break;
    case EMBER_AF_RF4CE_GDP_ATTRIBUTE_POLL_CONFIGURATION:
      MEMMOVE(attributes->pollConfiguration,
              val,
              APL_GDP_POLL_CONFIGURATION_SIZE);
      break;
    case EMBER_AF_RF4CE_GDP_ATTRIBUTE_AUTO_CHECK_VALIDATION_PERIOD:
      attributes->autoCheckValidationPeriod = HIGH_LOW_TO_INT(val[1], val[0]);
      break;
    case EMBER_AF_RF4CE_GDP_ATTRIBUTE_LINK_LOST_WAIT_TIME:
      attributes->linkLostWaitTime = HIGH_LOW_TO_INT(val[1], val[0]);
      break;
    case EMBER_AF_RF4CE_GDP_ATTRIBUTE_IDENTIFICATION_CAPABILITIES:
      attributes->identificationCapabilities = val[0];
      break;
#if defined(EMBER_SCRIPTED_TEST)
    case EMBER_AF_RF4CE_GDP_ATTRIBUTE_SETTABLE_SCALAR_TEST_1:
      attributes->settableScalarTest1 = HIGH_LOW_TO_INT(val[1], val[0]);
      break;
    case EMBER_AF_RF4CE_GDP_ATTRIBUTE_SETTABLE_SCALAR_TEST_2:
      attributes->settableScalarTest2 = HIGH_LOW_TO_INT(val[1], val[0]);
      break;
    case EMBER_AF_RF4CE_GDP_ATTRIBUTE_ONE_DIMENSIONAL_ARRAY_TEST:
      attributes->oneDimensionalTestAttribute[entryId] =
          HIGH_LOW_TO_INT(val[1], val[0]);
      break;
    case EMBER_AF_RF4CE_GDP_ATTRIBUTE_TWO_DIMENSIONAL_ARRAY_TEST:
      attributes->twoDimensionalTestAttribute[entryId & 0xFF][(entryId & 0xFF00) >> 8]=
          emberFetchLowHighInt32u(val);
      break;
#endif
    }
  }
}

//------------------------------------------------------------------------------
// Static tables and functions.

// We also maintain a table that stores misc. information about the supported
// attributes.
static const EmAfRf4ceGdpAttributeDescriptor attributesInfo[GDP_ATTRIBUTES_COUNT] = {
    {EMBER_AF_RF4CE_GDP_ATTRIBUTE_VERSION,
     APL_GDP_VERSION_SIZE,
     (ATTRIBUTE_HAS_REMOTE_GET_ACCESS_BIT
      | ATTRIBUTE_HAS_REMOTE_PUSH_ACCESS_BIT),
     0},

    {EMBER_AF_RF4CE_GDP_ATTRIBUTE_CAPABILITIES,
     APL_GDP_CAPABILITIES_SIZE,
     (ATTRIBUTE_HAS_REMOTE_GET_ACCESS_BIT
      | ATTRIBUTE_HAS_REMOTE_PUSH_ACCESS_BIT),
     0},

    {EMBER_AF_RF4CE_GDP_ATTRIBUTE_KEY_EXCHANGE_TRANSFER_COUNT,
     APL_GDP_CAPABILITIES_SIZE,
     0,
     0},

    {EMBER_AF_RF4CE_GDP_ATTRIBUTE_POWER_STATUS,
     APL_GDP_POWER_STATUS_SIZE,
     (ATTRIBUTE_HAS_REMOTE_GET_ACCESS_BIT
      | ATTRIBUTE_HAS_REMOTE_PULL_ACCESS_BIT),

     0},

    {EMBER_AF_RF4CE_GDP_ATTRIBUTE_POLL_CONSTRAINTS,
     APL_GDP_POLL_CONSTRAINTS_SIZE,
     ATTRIBUTE_HAS_REMOTE_PUSH_ACCESS_BIT,
     0},

    {EMBER_AF_RF4CE_GDP_ATTRIBUTE_POLL_CONFIGURATION,
     APL_GDP_POLL_CONFIGURATION_SIZE,
     ATTRIBUTE_HAS_REMOTE_PULL_ACCESS_BIT,
     0},

    {EMBER_AF_RF4CE_GDP_ATTRIBUTE_MAX_PAIRING_CANDIDATES,
     APL_GDP_CAPABILITIES_SIZE,
     0,
     0},

    {EMBER_AF_RF4CE_GDP_ATTRIBUTE_AUTO_CHECK_VALIDATION_PERIOD,
     APL_GDP_AUTO_CHECK_VALIDATION_PERIOD_SIZE,
     ATTRIBUTE_HAS_REMOTE_PULL_ACCESS_BIT,
     0},

    {EMBER_AF_RF4CE_GDP_ATTRIBUTE_BINDING_RECIPIENT_VALIDATION_WAIT_TIME,
     APL_GDP_BINDING_RECIPIENT_VALIDATION_WAIT_TIME_SIZE,
     0,
     0},

    {EMBER_AF_RF4CE_GDP_ATTRIBUTE_BINDING_INITIATOR_VALIDATION_WAIT_TIME,
     APL_GDP_BINDING_ORIGINATOR_VALIDATION_WAIT_TIME_SIZE,
     0,
     0},

    {EMBER_AF_RF4CE_GDP_ATTRIBUTE_LINK_LOST_WAIT_TIME,
     APL_GDP_LINK_LOST_WAIT_TIME_SIZE,
     ATTRIBUTE_HAS_REMOTE_PULL_ACCESS_BIT,
     0},

    {EMBER_AF_RF4CE_GDP_ATTRIBUTE_IDENTIFICATION_CAPABILITIES,
     APL_GDP_IDENTIFICATION_CAPABILITIES_SIZE,
     (ATTRIBUTE_HAS_REMOTE_GET_ACCESS_BIT
      | ATTRIBUTE_HAS_REMOTE_PUSH_ACCESS_BIT),
     0},

#if defined(EMBER_SCRIPTED_TEST)
     ONE_DIMENSION_ARRAYED_ATTRIBUTE_TEST,
     TWO_DIMENSION_ARRAYED_ATTRIBUTE_TEST,
     SCALAR_SETTABLE_ATTRIBUTE_TEST_1,
     SCALAR_SETTABLE_ATTRIBUTE_TEST_2,
#endif
};

// Assumes that the attribute is supported.
static const EmAfRf4ceGdpAttributeDescriptor *getAttributeDescriptor(uint8_t attrId)
{
  uint8_t i;

  for(i=0; i<GDP_ATTRIBUTES_COUNT; i++) {
    if (attrId == attributesInfo[i].id) {
      return &(attributesInfo[i]);
    }
  }

  return NULL;
}

#if defined(EMBER_SCRIPTED_TEST)
static bool arrayedAttributeIsEntryIdValid(uint8_t attrId,
                                              uint16_t entryId)
{
  EmAfRf4ceGdpAttributeDescriptor *attrPtr =
      (EmAfRf4ceGdpAttributeDescriptor*)getAttributeDescriptor(attrId);

  // The entryId field shall only be included for arrayed attributes:
  // - For attributes that consist of a one dimensional array containing the
  //   2-octet index of the entry.
  // - For attributes that consist of a two dimensional array, the first octet
  //   contains the index of the entry in the first dimension of the array; the
  //   second octet contains the index of the entry in the second dimension of
  //   the array.

  if (attrPtr->bitmask & ATTRIBUTE_IS_TWO_DIMENSIONAL_ARRAYED) {
    return ((entryId & 0x00FF) <  (attrPtr->dimension & 0x00FF)
            && (entryId & 0xFF00) <  (attrPtr->dimension & 0xFF00));
  } else {
    return (entryId < attrPtr->dimension);
  }
}
#endif

static bool acceptIncomingAttributeCommand(void)
{
  return (((emAfRf4ceGdpGetPairingBindStatus(emberAfRf4ceGetPairingIndex())
            & PAIRING_ENTRY_BINDING_STATUS_MASK)
           == PAIRING_ENTRY_BINDING_STATUS_BOUND_ORIGINATOR
           || (emAfRf4ceGdpGetPairingBindStatus(emberAfRf4ceGetPairingIndex())
               & PAIRING_ENTRY_BINDING_STATUS_MASK)
              == PAIRING_ENTRY_BINDING_STATUS_BOUND_RECIPIENT)
          && internalGdpState() == INTERNAL_STATE_NONE);
}

static void handleIncomingGetOrPullAttributesCommand(bool isGet)
{
  EmberAfRf4ceGdpAttributeIdentificationRecord idRecord;
  EmberAfRf4ceGdpAttributeStatusRecord statusRecord;

  emAfRf4ceGdpResetFetchAttributeFinger();

  // Set the outgoing message to "GetAttributeResponse" or
  // "PullAttributeResponse".
  emAfRf4ceGdpStartAttributesCommand((isGet)
                                     ? EMBER_AF_RF4CE_GDP_COMMAND_GET_ATTRIBUTES_RESPONSE
                                     : EMBER_AF_RF4CE_GDP_COMMAND_PULL_ATTRIBUTES_RESPONSE);

  while(emAfRf4ceGdpFetchAttributeIdentificationRecord(&idRecord)) {
    EmberAfRf4ceGdpAttributeId attributeId = idRecord.attributeId;
    uint8_t attributeVal[MAX_GDP_ATTRIBUTE_SIZE];

    // The GetAttributesResponse command frame shall contain the same number
    // of attribute status records as attribute identifiers included in the
    // command frame.
    statusRecord.attributeId = attributeId;
    statusRecord.value = (uint8_t*)attributeVal;
    statusRecord.entryId = idRecord.entryId;

    // The recipient shall check that the attribute identifier in the
    // attribute identification record corresponds to a supported attribute.
    // If the attribute is not supported, in its response the recipient shall
    // include the attribute identifier, shall set the attribute status field
    // of the corresponding attribute status record to indicated unsupported
    // attribute and shall not include the attribute length and attribute
    // value fields. The recipient shall then move on to the next attribute
    // identifier.
    if (!isAttributeSupported(attributeId)) {
      statusRecord.status =
          EMBER_AF_RF4CE_GDP_ATTRIBUTE_STATUS_UNSUPPORTED_ATTRIBUTE;
    // Table 9 indicates for each attribute its remote access rights.
    } else if ((isGet && !attributeHasRemoteGetAccess(attributeId))
               || (!isGet && !attributeHasRemotePullAccess(attributeId))) {
      statusRecord.status =
          EMBER_AF_RF4CE_GDP_ATTRIBUTE_STATUS_ILLEGAL_REQUEST;
    } else { // supported attribute with read access right.
      statusRecord.valueLength = getAttributeSize(attributeId);

#if defined(EMBER_SCRIPTED_TEST)
      // If the indices in the entry identifier fall outside the supported
      // range for this attribute, in its response the recipient shall
      // indicate an invalid entry.
      if (IS_ARRAY_ATTRIBUTE(attributeId)
          && !arrayedAttributeIsEntryIdValid(attributeId,
                                             idRecord.entryId)) {
        statusRecord.status = EMBER_AF_RF4CE_GDP_ATTRIBUTE_STATUS_INVALID_ENTRY;
      } else {
#endif // EMBER_SCRIPTED_TEST
        statusRecord.status = EMBER_AF_RF4CE_GDP_ATTRIBUTE_STATUS_SUCCESS;

        if (isGet) {
          emAfRf4ceGdpGetLocalAttribute(attributeId,
                                        statusRecord.entryId,
                                        attributeVal);
        } else {
          emAfRf4ceGdpGetRemoteAttribute(attributeId,
                                         statusRecord.entryId,
                                         attributeVal);
        }
#if defined(EMBER_SCRIPTED_TEST)
      }
#endif // EMBER_SCRIPTED_TEST
    }

    emAfRf4ceGdpAppendAttributeStatusRecord(&statusRecord);
  }

  emAfRf4ceGdpSendAttributesCommand(emberAfRf4ceGetPairingIndex(),
                                    EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE,
                                    EMBER_RF4CE_NULL_VENDOR_ID); // TODO: vendorId
}

static void handleIncomingGetOrPullAttributesResponseCommand(bool isGet)
{
  EmberAfRf4ceGdpAttributeStatusRecord statusRecord;

  emAfRf4ceGdpResetFetchAttributeFinger();

  while(emAfRf4ceGdpFetchAttributeStatusRecord(&statusRecord)) {
    EmberAfRf4ceGdpAttributeId attributeId = statusRecord.attributeId;

    if (isAttributeSupported(attributeId)
        && statusRecord.status == EMBER_AF_RF4CE_GDP_ATTRIBUTE_STATUS_SUCCESS) {
      if (isGet) {
        emAfRf4ceGdpSetRemoteAttribute(attributeId,
                                       statusRecord.entryId,
                                       statusRecord.value);
      } else {
        emAfRf4ceGdpSetLocalAttribute(attributeId,
                                      statusRecord.entryId,
                                      statusRecord.value);
      }
    }
  }
}

static void handleIncomingSetOrPushAttributesCommand(bool isSet)
{
  EmberAfRf4ceGdpAttributeRecord record;
 EmberAfRf4ceGdpResponseCode responseCode =
     EMBER_AF_RF4CE_GDP_RESPONSE_CODE_SUCCESSFUL;

 emAfRf4ceGdpResetFetchAttributeFinger();

 while(emAfRf4ceGdpFetchAttributeRecord(&record)) {
   EmberAfRf4ceGdpAttributeId attributeId = record.attributeId;

   if (!isAttributeSupported(attributeId)) {
     // If the attribute is not supported, the recipient shall set the
     // response code of the GenericResponse command frame to indicate an
     // invalid parameter and ignore the rest of the frame.
     responseCode = EMBER_AF_RF4CE_GDP_RESPONSE_CODE_INVALID_PARAMETER;
     break;
   // Table 9 indicates for each attribute its remote access rights.
   } else if ((isSet && !attributeHasRemoteSetAccess(attributeId))
              || (!isSet && !attributeHasRemotePushAccess(attributeId))
#if defined(EMBER_SCRIPTED_TEST)
              || (IS_ARRAY_ATTRIBUTE(attributeId)
                  && !arrayedAttributeIsEntryIdValid(attributeId,
                                                     record.entryId))
#endif // EMBER_SCRIPTED_TEST
   ) {
     // if the attribute record field corresponds to an arrayed attribute,
     // the recipient shall check that the entry identifier in the attribute
     // record addresses an entry that exists in the array. If the indices in
     // the entry identifier fall outside the supported range for the
     // attribute, the recipient shall set the response code of the
     // GenericResponse command frame to indicate an invalid entry and ignore
     // the rest of the frame.
     // For SetAttributes() commands we also check that the attribute the remote
     // is attempting to set has the remote write right.
     responseCode = EMBER_AF_RF4CE_GDP_RESPONSE_CODE_UNSUPPORTED_REQUEST;
     break;
   } else {
     // Update the remote attribute value.
     if (isSet) {
       emAfRf4ceGdpSetLocalAttribute(attributeId,
                                     record.entryId,
                                     record.value);
     } else {
       emAfRf4ceGdpSetRemoteAttribute(attributeId,
                                      record.entryId,
                                      record.value);
     }
   }
 }

 emberAfRf4ceGdpGenericResponse(emberAfRf4ceGetPairingIndex(),
                                EMBER_AF_RF4CE_PROFILE_GENERIC_DEVICE,
                                EMBER_RF4CE_NULL_VENDOR_ID, // TODO
                                responseCode);
}
