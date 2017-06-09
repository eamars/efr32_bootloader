// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"

#if defined(EMBER_SCRIPTED_TEST)
#include "rf4ce-gdp-test.h"
#endif // EMBER_SCRIPTED_TEST

#include "app/framework/plugin/rf4ce-profile/rf4ce-profile.h"
#include "app/framework/plugin/aes-cmac/aes-cmac.h"
#include "rf4ce-gdp.h"
#include "rf4ce-gdp-internal.h"
#include "rf4ce-gdp-attributes.h"

#define CONTEXT_STRING_LENGTH   9

#if defined(EMBER_AF_PLUGIN_RF4CE_GDP_ENHANCED_SECURITY)

//------------------------------------------------------------------------------
// Global variables.

EmberAfRf4ceGdpKeyExchangeFlags emAfRf4ceGdpLocalKeyExchangeFlags =
    (0
#if defined(EMBER_AF_PLUGIN_RF4CE_GDP_STANDARD_SHARED_SECRET)
    | EMBER_AF_RF4CE_GDP_KEY_EXCHANGE_FLAG_STANDARD_SHARED_SECRET
#endif
#if defined(EMBER_AF_PLUGIN_RF4CE_GDP_VENDOR_SPECIFIC_SECRETS)
    | EMBER_AF_RF4CE_GDP_KEY_EXCHANGE_FLAG_INITIATOR_VENDOR_SPECIFIC_SECRET
    | EMBER_AF_RF4CE_GDP_KEY_EXCHANGE_FLAG_RESPONDER_VENDOR_SPECIFIC_SECRET
#endif
    );

//------------------------------------------------------------------------------
// Local constants, variables and forward declarations.

static const uint8_t standardSharedSecret[GDP_SHARED_SECRET_SIZE] =
    GDP_STANDARD_SHARED_SECRET;
static const char *context = "RF4CE GDP";

static bool internalCall;
static uint8_t securityPairingIndex;
static EmberAfRf4ceGdpRand localRandA;
static EmberAfRf4ceGdpRand localRandB;
static EmberKeyData newKey;

void finishSecurityProcedure(EmberStatus status);
static EmberAfRf4ceGdpKeyExchangeFlags
                 getKeyExchangeParams(bool localNodeIsInitiator,
                                      EmberAfRf4ceGdpKeyExchangeFlags peerFlags,
                                      uint8_t *vendorSpecificSecret);
static void computeTag(bool isTagA, uint8_t pairingIndex, uint8_t *tag);
static bool generateKey(bool localNodeIsInitiator,
                           uint8_t pairingIndex,
                           const uint8_t *sharedSecret,
                           uint8_t *generatedKey);
static bool updateLinkKeyAndResetFrameCounter(void);

//------------------------------------------------------------------------------
// Public APIs.

EmberStatus emberAfRf4ceGdpInitiateKeyExchange(uint8_t pairingIndex)
{
  // The public API can only be called if there is no ongoing binding process.
  if (internalGdpState() != INTERNAL_STATE_NONE) {
    return EMBER_INVALID_CALL;
  }

  return emAfRf4ceGdpInitiateKeyExchangeInternal(pairingIndex, false);
}

//------------------------------------------------------------------------------
// Internal APIs, packet handlers and callbacks.

EmberStatus emAfRf4ceGdpInitiateKeyExchangeInternal(uint8_t pairingIndex,
                                                    bool intCall)
{
  EmberStatus status;

  // Check that there is an active binding corresponding to the passed pairing
  // index.
  if ((emAfRf4ceGdpGetPairingBindStatus(pairingIndex)
       & PAIRING_ENTRY_BINDING_STATUS_MASK)
      == PAIRING_ENTRY_BINDING_STATUS_NOT_BOUND) {
    debugScriptCheck("Key Exchange failed: binding not active");
    return EMBER_INVALID_CALL;
  }

  internalCall = intCall;
  securityPairingIndex = pairingIndex;

  // Upon initiation of the Key Exchange procedure, the initiator node transmits
  // a Key Exchange command frame with the challenge sub-type to the responder
  // node. An 8-octet random byte string (RAND-A) is included in this frame.
  // It shall then wait for up to aplcMaxResponseWaitTime duration to receive
  // the key exchange command frame with challenge response sub-type from the
  // responder node. If the timeout expires, the node shall terminate the
  // procedure with failure status.

  if (!emAfRf4ceGdpSecurityGetRandomString(&localRandA)) {
    debugScriptCheck("Key Exchange failed: can't generate random data");
    return EMBER_ERR_FATAL;
  }

  debugScriptCheck("Key Exchange started");

  status = emAfRf4ceGdpKeyExchangeChallenge(securityPairingIndex,
                                           EMBER_RF4CE_NULL_VENDOR_ID,
                                           // Clear the responder vendor-specific bit
                                           (emAfRf4ceGdpLocalKeyExchangeFlags
                                            & ~EMBER_AF_RF4CE_GDP_KEY_EXCHANGE_FLAG_RESPONDER_VENDOR_SPECIFIC_SECRET),
                                           &localRandA);
  if (status == EMBER_SUCCESS) {
    emAfGdpStartCommandPendingTimer(INTERNAL_STATE_GDP_SECURITY_KEY_CHALLENGE_PENDING,
                                    APLC_MAX_RESPONSE_WAIT_TIME_MS);
  } else if (intCall) {
    finishSecurityProcedure(EMBER_SECURITY_FAILURE);
  }

  return status;
}

void emAfRf4ceGdpIncomingKeyExchangeChallenge(EmberAfRf4ceGdpKeyExchangeFlags flags,
                                              const EmberAfRf4ceGdpRand *randA)
{
  EmberAfRf4ceGdpKeyExchangeFlags responseFlags;
  uint8_t vendorSpecificSecret[GDP_SHARED_SECRET_SIZE];
  EmberAfRf4ceGdpTag tagB;
  uint8_t pairingIndex = emberAfRf4ceGetPairingIndex();

  // Only process keyExchangeChallenge commands if we are not in the middle of
  // some other process.
  if (internalGdpState() != INTERNAL_STATE_NONE) {
    debugScriptCheck("Node busy with some other process, KeyExchangeChallenge dropped");
    return;
  }

  // First check that the keyExchangeChallenge command is coming from a pairing
  // for which there is an active binding. If not, silently discard the packet.
  if ((emAfRf4ceGdpGetPairingBindStatus(pairingIndex)
       & PAIRING_ENTRY_BINDING_STATUS_MASK)
      == PAIRING_ENTRY_BINDING_STATUS_NOT_BOUND) {
    debugScriptCheck("No active binding, KeyExchangeChallenge dropped");
    return;
  }

  // We accept an incoming KeyExchangeChallenge() only if the remote node
  // supports enhanced security.
  if (!(emAfRf4ceGdpGetPairingBindStatus(pairingIndex)
        & PAIRING_ENTRY_REMOTE_NODE_SUPPORTS_ENHANCED_SECURITY_BIT)) {
    debugScriptCheck("Remote node does not support enhanced security, KeyExchangeChallenge dropped");
    return;
  }

  // Save RAND-A
  MEMMOVE(localRandA.contents, randA->contents, EMBER_AF_RF4CE_GDP_RAND_SIZE);

  // Upon reception of a Key Exchange command frame with challenge sub-type,
  // the key exchange responder first selects a shared secret from among those
  // supported by the initiator.
  responseFlags = getKeyExchangeParams(false, flags, vendorSpecificSecret);

  // Then, it shall generate an 8-octet random byte string (RAND-B) and shall
  // compute the link key as described in section 7.4.2.
  if (!emAfRf4ceGdpSecurityGetRandomString(&localRandB)) {
    debugScriptCheck("Key Exchange failed: can't generate random data");
    return;
  }

  if (!generateKey(false, // isInitiator
                   pairingIndex,
                   ((responseFlags & EMBER_AF_RF4CE_GDP_KEY_EXCHANGE_FLAG_STANDARD_SHARED_SECRET)
                    ? standardSharedSecret
                    : vendorSpecificSecret),
                   newKey.contents)) {
    debugScriptCheck("Can't compute the new key");
    return;
  }

  // It shall transmit a key exchange command frame with the challenge response
  // sub-type to the initiator node. The flags shall be set to indicate the
  // selected shared secret. The TAG-B field in this frame shall be computed as
  // described in 7.4.3.
  if (responseFlags > 0) {
    computeTag(false, pairingIndex, tagB.contents);
  } else {
    // No secret was selected, we just memset tagB to all zeros.
    MEMSET(tagB.contents, 0x00, EMBER_AF_RF4CE_GDP_TAG_SIZE);
  }

  if (emAfRf4ceGdpKeyExchangeChallengeResponse(pairingIndex,
                                               EMBER_RF4CE_NULL_VENDOR_ID,
                                               responseFlags,
                                               &localRandB,
                                               &tagB) == EMBER_SUCCESS) {
    // If no shared secret was selected, the node shall exit the procedure with
    // failure status after transmission of the challenge response sub-type
    // frame. Otherwise it shall wait for up to aplcMaxResponseWaitTime duration
    // to receive the key exchange command frame with response sub-type from
    // the initiator node.
    if (responseFlags
        & (EMBER_AF_RF4CE_GDP_KEY_EXCHANGE_FLAG_STANDARD_SHARED_SECRET
           | EMBER_AF_RF4CE_GDP_KEY_EXCHANGE_FLAG_RESPONDER_VENDOR_SPECIFIC_SECRET)) {
      emAfGdpStartCommandPendingTimer(INTERNAL_STATE_GDP_SECURITY_KEY_CHALLENGE_RESPONSE_PENDING,
                                      APLC_MAX_RESPONSE_WAIT_TIME_MS);
      securityPairingIndex = pairingIndex;
    } else {
      debugScriptCheck("No secret selected");
      finishSecurityProcedure(EMBER_SECURITY_FAILURE);
    }
  } else {
    debugScriptCheck("Failed to send out the KeyExchangeChallengeResponse");
    finishSecurityProcedure(EMBER_SECURITY_FAILURE);
  }
}

void emAfRf4ceGdpIncomingKeyExchangeChallengeResponse(EmberAfRf4ceGdpKeyExchangeFlags flags,
                                                      const EmberAfRf4ceGdpRand *randB,
                                                      const EmberAfRf4ceGdpTag *tagB)
{
  EmberAfRf4ceGdpKeyExchangeFlags responseFlags;
  uint8_t vendorSpecificSecret[GDP_SHARED_SECRET_SIZE];
  uint8_t pairingIndex = emberAfRf4ceGetPairingIndex();

  if (internalGdpState()
      != INTERNAL_STATE_GDP_SECURITY_KEY_CHALLENGE_PENDING) {
    debugScriptCheck("Challenge not pending, KeyExchangeChallengeResponse dropped");
    return;
  }

  if (pairingIndex != securityPairingIndex) {
    debugScriptCheck("KeyExchangeChallengeResponse from wrong pairing");
    return;
  }

  // Upon reception of a Key Exchange command frame with the challenge response
  // sub-type, the initiator shall determine the shared secret based on the
  // flags field. If no secret is selected, it shall exit the procedure with
  // failure status.
  responseFlags = getKeyExchangeParams(true, flags, vendorSpecificSecret);

  if ((responseFlags
       & (EMBER_AF_RF4CE_GDP_KEY_EXCHANGE_FLAG_STANDARD_SHARED_SECRET
          | EMBER_AF_RF4CE_GDP_KEY_EXCHANGE_FLAG_INITIATOR_VENDOR_SPECIFIC_SECRET))
       == 0) {
    debugScriptCheck("No secret selected");
    finishSecurityProcedure(EMBER_SECURITY_FAILURE);
  } else {
    EmberAfRf4ceGdpTag tagBCheck;

    // Save RAND-B
    MEMMOVE(localRandB.contents, randB->contents, EMBER_AF_RF4CE_GDP_RAND_SIZE);

    // Otherwise, the node shall compute the link key as described in section
    // 7.4.2.
    if (!generateKey(true, // isInitiator
                     pairingIndex,
                    ((responseFlags & EMBER_AF_RF4CE_GDP_KEY_EXCHANGE_FLAG_STANDARD_SHARED_SECRET)
                     ? standardSharedSecret
                     : vendorSpecificSecret),
                    newKey.contents)) {
      debugScriptCheck("Can't compute the new key");
      finishSecurityProcedure(EMBER_SECURITY_FAILURE);
    }

    // It shall then verify the frame by computing the value of TAG-B as
    // described in section 7.4.3 and comparing it with the included value.
    // If the verification fails, the node shall exit the procedure with failure
    // status.
    computeTag(false, pairingIndex, tagBCheck.contents);
    if (MEMCOMPARE(tagBCheck.contents,
                   tagB->contents,
                   EMBER_AF_RF4CE_GDP_TAG_SIZE) != 0) {
      debugScriptCheck("TAG-B doesn't match");
      finishSecurityProcedure(EMBER_SECURITY_FAILURE);
    } else {
      EmberAfRf4ceGdpTag tagA;

      // Otherwise the Key Exchange initiator shall transmit a Key Exchange
      // command frame with the response sub-type to the Key Exchange Responder.
      // The TAG-A field in this frame shall be computed as described in
      // section 7.4.3.
      computeTag(true, pairingIndex, tagA.contents);

      if (emAfRf4ceGdpKeyExchangeResponse(pairingIndex,
                                          EMBER_RF4CE_NULL_VENDOR_ID,
                                          &tagA) != EMBER_SUCCESS) {
        debugScriptCheck("Failed to send out the KeyExchangeResponse");
        finishSecurityProcedure(EMBER_SECURITY_FAILURE);
      }

      // If we successfully send out a KeyExchangeResponse, we wait for the
      // emAfRf4ceGdpKeyExchangeResponseSent() handler to fire.
    }
  }
}

void emAfRf4ceGdpKeyExchangeResponseSent(EmberStatus status)
{
  // After completing the transmission of the key exchange frame with
  // response sub-type, the initiator shall immediately update the value
  // of the security key field in the pairing entry corresponding to the
  // key exchange responder with the link key and reset the incoming
  // frame counter field.
  if (status != EMBER_SUCCESS
      || !updateLinkKeyAndResetFrameCounter()) {
    finishSecurityProcedure(EMBER_SECURITY_FAILURE);
    return;
  }

  // It shall then wait for up to aplcMaxResponseWaitTime duration to
  // receive the key exchange command frame with confirm sub-type from
  // the responder node. If the timeout expires, the node shall terminate
  // the procedure with failure status. In this case the node shall not
  // revert back the updates to the security fields in the pairing table
  // entry.
  emAfGdpStartCommandPendingTimer(INTERNAL_STATE_GDP_SECURITY_KEY_RESPONSE_PENDING,
                                  APLC_MAX_RESPONSE_WAIT_TIME_MS);
}

void emAfRf4ceGdpIncomingKeyExchangeResponse(const EmberAfRf4ceGdpTag *tagA)
{
  EmberAfRf4ceGdpTag tagACheck;
  uint8_t pairingIndex = emberAfRf4ceGetPairingIndex();

  if (pairingIndex != securityPairingIndex) {
    debugScriptCheck("KeyExchangeResponse from wrong pairing");
    return;
  }

  // Upon reception of a Key Exchange command frame with the response sub-type,
  // the responder shall verify the value of TAG-A included in the packet. If
  // the verification fails, the node shall exit the procedure with failure
  // status.
  computeTag(true, pairingIndex, tagACheck.contents);
  if (MEMCOMPARE(tagACheck.contents,
                 tagA->contents,
                 EMBER_AF_RF4CE_GDP_TAG_SIZE) != 0) {
    debugScriptCheck("TAG-A doesn't match");
    finishSecurityProcedure(EMBER_SECURITY_FAILURE);
  } else {
    // Otherwise the responder shall first update the value of the security key
    // field in the pairing entry corresponding to the initiator with the link
    // key and reset the incoming frame counter value.
    // It shall then transmit the Key Exchange command frame with the confirm
    // sub-type to the initiator. This frame shall be transmitted with the
    // security enabled at the network layer. It shall then exit the procedure
    // with a success status.
    if (!updateLinkKeyAndResetFrameCounter()
        || emAfRf4ceGdpKeyExchangeConfirm(pairingIndex,
                                          EMBER_RF4CE_NULL_VENDOR_ID)
                                     != EMBER_SUCCESS) {
      finishSecurityProcedure(EMBER_SECURITY_FAILURE);
    } else {
      finishSecurityProcedure(EMBER_SUCCESS);
    }
  }
}

void emAfRf4ceGdpIncomingKeyExchangeConfirm(bool secured)
{
  uint8_t pairingIndex = emberAfRf4ceGetPairingIndex();

  if (internalGdpState()
      != INTERNAL_STATE_GDP_SECURITY_KEY_RESPONSE_PENDING) {
    debugScriptCheck("Response not pending, KeyExchangeConfirm dropped");
    return;
  }

  if (pairingIndex != securityPairingIndex) {
    debugScriptCheck("KeyExchangeConfirm from wrong pairing");
    return;
  }

  // Upon reception of the Key Exchange command frame with confirm sub-type,
  // the initiator shall verify that the frame was received with security
  // enabled at the network layer. If not, it shall silently discard the frame.
  // Otherwise, it shall exit the procedure with success status and notify the
  // application.
  if (!secured) {
    debugScriptCheck("KeyExchangeConfirm not encrypted, dropping it");
    return;
  }

  finishSecurityProcedure(EMBER_SUCCESS);
}

void emAfRf4ceGdpSecurityValidationCompleteCallback(uint8_t pairingIndex)
{
  EmberRf4cePairingTableEntry entry;
  EmberStatus status = emberAfRf4ceGetPairingTableEntry(pairingIndex, &entry);

  assert(status == EMBER_SUCCESS);

  // Every time a pairing is created or updated, we need to store the pairing
  // key since it is used to compute new link keys during the key exchange
  // procedure.
  emAfRf4ceGdpSetPairingKey(pairingIndex, &entry.securityLinkKey);
}

//------------------------------------------------------------------------------
// Event handlers.

void emAfPendingCommandEventHandlerSecurity(void)
{
  switch(internalGdpState()) {
  case INTERNAL_STATE_GDP_SECURITY_KEY_CHALLENGE_PENDING:
    debugScriptCheck("KeyExchangeChallenge timeout");
    break;
  case INTERNAL_STATE_GDP_SECURITY_KEY_CHALLENGE_RESPONSE_PENDING:
    debugScriptCheck("KeyExchangeChallengeResponse timeout");
    break;
  case INTERNAL_STATE_GDP_SECURITY_KEY_RESPONSE_PENDING:
    debugScriptCheck("KeyExchangeResponse timeout");
    break;
  }

  finishSecurityProcedure(EMBER_SECURITY_TIMEOUT);
}

//------------------------------------------------------------------------------
// Static functions.

void finishSecurityProcedure(EmberStatus status)
{
  uint16_t savedInternalState = internalGdpState();

  emberEventControlSetInactive(emberAfPluginRf4ceGdpPendingCommandEventControl);

  setInternalState(INTERNAL_STATE_NONE);

  if (!internalCall
      // Initiator states
      && (savedInternalState == INTERNAL_STATE_GDP_SECURITY_KEY_CHALLENGE_PENDING
          || savedInternalState == INTERNAL_STATE_GDP_SECURITY_KEY_RESPONSE_PENDING)) {
    emberAfPluginRf4ceGdpKeyExchangeCompleteCallback(status);
  }
}

static EmberAfRf4ceGdpKeyExchangeFlags
                 getKeyExchangeParams(bool localNodeIsInitiator,
                                      EmberAfRf4ceGdpKeyExchangeFlags peerFlags,
                                      uint8_t *vendorSpecificSecret)
{
  EmberAfRf4ceGdpKeyExchangeFlags retFlags = 0x00;
  uint8_t peerVendorSpecificParam =
      (uint8_t)(peerFlags >> EMBER_AF_RF4CE_GDP_KEY_EXCHANGE_FLAG_VENDOR_SPECIFIC_PARAMETER_OFFSET);
  uint8_t localVendorSpecificParam;

  if ((emAfRf4ceGdpLocalKeyExchangeFlags
       & ((localNodeIsInitiator)
          ? EMBER_AF_RF4CE_GDP_KEY_EXCHANGE_FLAG_INITIATOR_VENDOR_SPECIFIC_SECRET
          : EMBER_AF_RF4CE_GDP_KEY_EXCHANGE_FLAG_RESPONDER_VENDOR_SPECIFIC_SECRET))
      && (peerFlags
          & ((localNodeIsInitiator)
              ? EMBER_AF_RF4CE_GDP_KEY_EXCHANGE_FLAG_RESPONDER_VENDOR_SPECIFIC_SECRET
              : EMBER_AF_RF4CE_GDP_KEY_EXCHANGE_FLAG_INITIATOR_VENDOR_SPECIFIC_SECRET))
      && emberAfPluginRf4ceGdpVendorSpecificKeyExchangeCallback(peerVendorSpecificParam,
                                                                &localVendorSpecificParam,
                                                                vendorSpecificSecret)) {
    // Both nodes support vendor specific secrets, and the application decided to
    // use a vendor-specific secret.
    retFlags = (((localNodeIsInitiator)
                 ? EMBER_AF_RF4CE_GDP_KEY_EXCHANGE_FLAG_INITIATOR_VENDOR_SPECIFIC_SECRET
                 : EMBER_AF_RF4CE_GDP_KEY_EXCHANGE_FLAG_RESPONDER_VENDOR_SPECIFIC_SECRET)
                | (localVendorSpecificParam
                   << EMBER_AF_RF4CE_GDP_KEY_EXCHANGE_FLAG_VENDOR_SPECIFIC_PARAMETER_OFFSET));
  } else if ((emAfRf4ceGdpLocalKeyExchangeFlags
              & EMBER_AF_RF4CE_GDP_KEY_EXCHANGE_FLAG_STANDARD_SHARED_SECRET)
             && (peerFlags & EMBER_AF_RF4CE_GDP_KEY_EXCHANGE_FLAG_STANDARD_SHARED_SECRET)) {
    // Both nodes support standard shared secret.
    retFlags = EMBER_AF_RF4CE_GDP_KEY_EXCHANGE_FLAG_STANDARD_SHARED_SECRET;
  }

  return retFlags;
}

static void computeTag(bool isTagA, uint8_t pairingIndex, uint8_t *tag)
{
  uint8_t message[EMBER_AF_RF4CE_GDP_RAND_SIZE*2];
  uint8_t hashedMessage[EMBER_AF_RF4CE_GDP_RAND_SIZE*2];

  MEMMOVE(message + ((isTagA) ? 0 : EMBER_AF_RF4CE_GDP_RAND_SIZE),
          localRandA.contents,
          EMBER_AF_RF4CE_GDP_RAND_SIZE);
  MEMMOVE(message + ((isTagA) ? EMBER_AF_RF4CE_GDP_RAND_SIZE : 0),
          localRandB.contents,
          EMBER_AF_RF4CE_GDP_RAND_SIZE);

  emberAfPluginAesMacAuthenticate(newKey.contents,
                                  message,
                                  EMBER_AF_RF4CE_GDP_RAND_SIZE*2,
                                  hashedMessage);

  // Each Tag value shall be truncated by using the lowest order 4 octets.
  MEMMOVE(tag, hashedMessage, EMBER_AF_RF4CE_GDP_TAG_SIZE);
}

// According to the GDP specs, node A is the initiator, while node B is the
// responder.
static bool generateKey(bool localNodeIsInitiator,
                           uint8_t pairingIndex,
                           const uint8_t *sharedSecret,
                           uint8_t *generatedKey)
{
  EmberRf4cePairingTableEntry entry;
  EmberKeyData pairingKey;
  EmberEUI64 macA;
  EmberEUI64 macB;
  uint8_t k_dk[EMBER_ENCRYPTION_KEY_SIZE];
  uint8_t aux[CONTEXT_STRING_LENGTH + 2*EUI64_SIZE + EMBER_ENCRYPTION_KEY_SIZE];

  if (emberAfRf4ceGetPairingTableEntry(pairingIndex, &entry) != EMBER_SUCCESS) {
    return false;
  }

  // Read the original link key from the pairing key table.
  emAfRf4ceGdpGetPairingKey(pairingIndex, &pairingKey);

  if (localNodeIsInitiator) {
    emberAfGetEui64(macA);
    MEMCOPY(macB, entry.destLongId, EUI64_SIZE);
  } else {
    MEMCOPY(macA, entry.destLongId, EUI64_SIZE);
    emberAfGetEui64(macB);
  }

  // Write (randA || randB) in aux
  MEMMOVE(aux,
          localRandA.contents,
          EMBER_AF_RF4CE_GDP_RAND_SIZE);
  MEMMOVE(aux + EMBER_AF_RF4CE_GDP_RAND_SIZE,
          localRandB.contents,
          EMBER_AF_RF4CE_GDP_RAND_SIZE);

  // Compute K_dk = AES-128-CMAC(RAND-A || RAND-B, Shared secret)
  emberAfPluginAesMacAuthenticate(aux,
                                  sharedSecret,
                                  EMBER_AF_RF4CE_GDP_RAND_SIZE*2,
                                  k_dk);

  // context
  MEMMOVE(aux, context, CONTEXT_STRING_LENGTH);
  // label (MAC-A || MAC-B)
  MEMMOVE(aux + CONTEXT_STRING_LENGTH, macA, EUI64_SIZE);
  MEMMOVE(aux + CONTEXT_STRING_LENGTH + EUI64_SIZE, macB, EUI64_SIZE);
  // pairing key
  MEMMOVE(aux + CONTEXT_STRING_LENGTH + 2*EUI64_SIZE,
          pairingKey.contents,
          EMBER_ENCRYPTION_KEY_SIZE);

  // Compute Link Key = AES-128-CMAC(K_dk, context || label || pairing key)
  emberAfPluginAesMacAuthenticate(k_dk,
                                  aux,
                                  CONTEXT_STRING_LENGTH + 2*EUI64_SIZE + EMBER_ENCRYPTION_KEY_SIZE,
                                  generatedKey);

  return true;
}

static bool updateLinkKeyAndResetFrameCounter(void)
{
  EmberRf4cePairingTableEntry entry;

  if (emberAfRf4ceGetPairingTableEntry(securityPairingIndex, &entry)
         != EMBER_SUCCESS) {
    return false;
  }

  debugScriptCheck("Setting the new link key");
  MEMMOVE(entry.securityLinkKey.contents,
          newKey.contents,
          EMBER_ENCRYPTION_KEY_SIZE);

  entry.frameCounter = 0; // reset the frame counter

  if (emberAfRf4ceSetPairingTableEntry(securityPairingIndex, &entry)
      != EMBER_SUCCESS) {
    return false;
  }

  return true;
}

#else // !defined(EMBER_AF_PLUGIN_RF4CE_GDP_ENHANCED_SECURITY)

EmberStatus emberAfRf4ceGdpInitiateKeyExchange(uint8_t pairingIndex)
{
  return EMBER_INVALID_CALL;
}

EmberStatus emAfRf4ceGdpInitiateKeyExchangeInternal(uint8_t pairingIndex,
                                                    bool intCall)
{
  return EMBER_INVALID_CALL;
}

void emAfRf4ceGdpIncomingKeyExchangeChallenge(EmberAfRf4ceGdpKeyExchangeFlags flags,
                                              const EmberAfRf4ceGdpRand *randA)
{
}

void emAfRf4ceGdpIncomingKeyExchangeChallengeResponse(EmberAfRf4ceGdpKeyExchangeFlags flags,
                                                      const EmberAfRf4ceGdpRand *randB,
                                                      const EmberAfRf4ceGdpTag *tagB)
{
}

void emAfRf4ceGdpIncomingKeyExchangeResponse(const EmberAfRf4ceGdpTag *tagA)
{
}

void emAfRf4ceGdpKeyExchangeResponseSent(EmberStatus status)
{
}

void emAfRf4ceGdpIncomingKeyExchangeConfirm(bool secured)
{
}

void emAfPendingCommandEventHandlerSecurity(void)
{
}

void emAfRf4ceGdpSecurityValidationCompleteCallback(uint8_t pairingIndex)
{
}

#endif // EMBER_AF_PLUGIN_RF4CE_GDP_ENHANCED_SECURITY
