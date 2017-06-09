// Copyright 2015 Silicon Laboratories, Inc.

#include PLATFORM_HEADER
#include CONFIGURATION_HEADER
#include EMBER_AF_API_STACK
#include "stack/ip/tls/tls.h"
#include "stack/ip/tls/sha256.h"
#include "zcl-core.h"
#include "zcl-core-well-known.h"

void emberZclGetPublicKeyCallback(const uint8_t **publicKey,
                                  uint16_t *publicKeySize);
EmberZclUid_t emZclUid = {{0}};

// -----------------------------------------------------------------------------
// Constants.

// The All Thread Nodes multicast address is filled in once the ULA prefix is
// known.  It is an RFC3306 address with the format ff33:40:<ula prefix>::1.
static const EmberIpv6Address allThreadNodes = {
  {0xFF, 0x33, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,}
};
#define RFC3306_NETWORK_PREFIX_OFFSET 4

static uint16_t fetchCoapOptions(EmberCoapReadOptions *options,
                                 EmberCoapOptionType wantType,
                                 const uint8_t **valuePointers,
                                 uint16_t *valueLengths,
                                 uint16_t maxOptions);
static bool uriLookup(EmZclContext_t *context, EmZclUriPath *paths);
static bool processQueries(EmZclContext_t *context, EmZclUriQuery *queries);

// URI handlers
static void zclHandler(EmZclContext_t *context);
static void clusterHandler(EmZclContext_t *context);

static bool hexSegmentToInt(const EmZclContext_t *context,
                            uint8_t depth,
                            size_t size,
                            uintmax_t *result);

//----------------------------------------------------------------
// URI path data
//
// First two numbers are:
//
//  - Entries to skip if the match succeeds but there are more segments.
//       0 -> handler will parse additional segments on its own
//       1 -> continue with next entry to parse next segment
//     255 -> no additional segments are allowed (e.g. .../n)
//    Any other value is used to jump to a shared sub-URI, in this
//    case the cluster ID matching, which has two different possible
//    prefixes.
//
//  - Entries to skip if the match fails.
//    Jump forward this many entries to get to the next URI at the same
//    depth (e.g. from zcl/e to zcl/g).  Putting simpler URIs earlier
//    makes it easier to keep track of these.
//
// GET means that only a GET request is allowed.

#define M(x) EM_ZCL_URI_FLAG_METHOD_ ## x
#define GXXX (M(GET))
#define XPXX (M(POST))
#define GPXX (M(GET)|M(POST))
#define GXPX (M(GET)|M(PUT))
#define GXPD (M(GET)|M(PUT)|M(DELETE))

static bool uriManagementCommandIdMatch(EmZclContext_t *context, void *data, uint8_t depth);
static bool uriEndpointIdMatch         (EmZclContext_t *context, void *data, uint8_t depth);
static bool uriGroupIdMatch            (EmZclContext_t *context, void *data, uint8_t depth);
static bool uriClusterIdMatch          (EmZclContext_t *context, void *data, uint8_t depth);
static bool uriAttributeIdMatch        (EmZclContext_t *context, void *data, uint8_t depth);
static bool uriBindingIdMatch          (EmZclContext_t *context, void *data, uint8_t depth);
static bool uriClusterCommandIdMatch   (EmZclContext_t *context, void *data, uint8_t depth);
static bool uriReportingIdMatch        (EmZclContext_t *context, void *data, uint8_t depth);

static EmZclUriQuery attributeUriQueries[] = {
  // zcl/[eg]/XX/<cluster>/a?f=1,2+3,4-5,...
  { emZclUriQueryStringPrefixMatch, "f=", emZclAttributeUriQueryFilterParse    },
  // zcl/[eg]/XX/<cluster>/a?u
  { emZclUriQueryStringPrefixMatch, "u",  emZclAttributeUriQueryUndividedParse },

  // terminator
  { NULL,                           NULL, NULL                                 },
};

static EmZclUriPath zclUriPaths[] = {
  // zcl
  {   1, 255, GXXX, emZclUriPathStringMatch,     "zcl", NULL,                zclHandler },

  // zcl/m
  {   1,   3, GXXX, emZclUriPathStringMatch,     "m",   NULL,                emZclManagementHandler },
  {   1, 255, GXXX, emZclUriPathStringMatch,     "c",   NULL,                emZclManagementCommandHandler },
  { 255, 255, XPXX, uriManagementCommandIdMatch, NULL,  NULL,                emZclManagementCommandIdHandler },

  // zcl/e
  {   1,   2, GXXX, emZclUriPathStringMatch,     "e",   NULL,                emZclUriEndpointHandler },
  // zcl/e/XX
  {   3, 255, GXXX, uriEndpointIdMatch,          NULL,  NULL,                emZclUriEndpointIdHandler },

  // zcl/g
  {   1, 255, GXXX, emZclUriPathStringMatch,     "g",   NULL,                emZclUriGroupHandler },
  // zcl/g/XXXX
  {   1, 255, GXXX, uriGroupIdMatch,             NULL,  NULL,                emZclUriGroupIdHandler },

  // cluster ID, after either zcl/e/XX or zcl/g/XXXX
  {   1, 255, GXXX, uriClusterIdMatch,           NULL,  NULL,                clusterHandler },

  // zcl/[eg]/XX/<cluster>/a:
  {   1,   2, GPXX, emZclUriPathStringMatch,     "a",   attributeUriQueries, emZclUriClusterAttributeHandler },
  // zcl/[eg]/XX/<cluster>/a/XXXX:
  { 255, 255, GXPX, uriAttributeIdMatch,         NULL,  NULL,                emZclUriClusterAttributeIdHandler },

  // zcl/[eg]/XX/<cluster>/b:
  {   1,   2, GPXX, emZclUriPathStringMatch,     "b",   NULL,                emZclUriClusterBindingHandler },
  // zcl/[eg]/XX/<cluster>/b/XX:
  { 255, 255, GXPD, uriBindingIdMatch,           NULL,  NULL,                emZclUriClusterBindingIdHandler },

  // zcl/[eg]/XX/<cluster>/c:
  {   1,   2, GXXX, emZclUriPathStringMatch,     "c",   NULL,                emZclUriClusterCommandHandler },
  // zcl/[eg]/XX/<cluster>/c/XX:
  { 255, 255, XPXX, uriClusterCommandIdMatch,    NULL,  NULL,                emZclUriClusterCommandIdHandler },

  // zcl/[eg]/XX/<cluster>/n:
  {   0,   1, XPXX, emZclUriPathStringMatch,     "n",   NULL,                emZclUriClusterNotificationHandler },

  // zcl/[eg]/XX/<cluster>/r:
  {   1, 255, GPXX, emZclUriPathStringMatch,     "r",   NULL,                emZclUriClusterReportingConfigurationHandler },
  // zcl/[eg]/XX/<cluster>/r/XX:
  { 255, 255, GXPD, uriReportingIdMatch,         NULL,  NULL,                emZclUriClusterReportingConfigurationIdHandler },
};

// TODO: The request info should be stored in the contexts that are passed
// around.  When sending a response, the info should be passed back to the ZCL
// response APIs via the context.  Until this is in place, a global info is
// used.  This means it is not possible to send a response that is not
// piggybacked on the ACKs.  The global info must be set before any handler is
// called, and it must be cleared after a response is sent or all handlers are
// called.
static const EmberCoapRequestInfo *currentInfo = NULL;

void emZclInitHandler(void)
{
  const uint8_t *publicKey = NULL;
  uint16_t publicKeySize = 0;
  emberZclGetPublicKeyCallback(&publicKey, &publicKeySize);

  // TODO: Every device should have a UID, so we should assert if we don't have
  // the public key to generate it.  Most applications don't have public keys
  // yet though, so this isn't doable right now.
  //assert(publicKey != NULL && publicKeySize != 0);

  Sha256State state;
  emSha256Start(&state);
  emSha256HashBytes(&state, (const uint8_t *)"zcl.uid", 7);
  emSha256HashBytes(&state, publicKey, publicKeySize);
  emSha256Finish(&state, emZclUid.bytes);
}

void emZclHandler(EmberCoapCode code,
                  uint8_t *uri,
                  EmberCoapReadOptions *options,
                  const uint8_t *payload,
                  uint16_t payloadLength,
                  const EmberCoapRequestInfo *info)
{
  EmZclContext_t context;
  MEMSET(&context, 0, sizeof(EmZclContext_t));
  context.code = code;
  context.options = options;
  context.payload = payload;
  context.payloadLength = payloadLength;
  context.info = info;
  context.groupId = EMBER_ZCL_GROUP_NULL;
  context.uriPathSegments = fetchCoapOptions(context.options,
                                             EMBER_COAP_OPTION_URI_PATH,
                                             context.uriPath,
                                             context.uriPathLength,
                                             MAX_URI_PATH_SEGMENTS);

  currentInfo = info;

  // simPrint("received %d segments", context.uriPathSegments);
  // All ZCL/IP URIs start with zcl or .well-known, and
  // that is all we handle.
  if (context.uriPathSegments == 0
      || MAX_URI_PATH_SEGMENTS < context.uriPathSegments
      || ! (uriLookup(&context, zclUriPaths)
            || uriLookup(&context, emZclWellKnowUriPaths))) {
    emZclRespond404NotFound();
  }

  currentInfo = NULL;
}

static uint16_t fetchCoapOptions(EmberCoapReadOptions *options,
                                 EmberCoapOptionType wantType,
                                 const uint8_t **valuePointers,
                                 uint16_t *valueLengths,
                                 uint16_t maxOptions)
{
  uint16_t count = 0;
  emberResetReadOptionPointer(options);
  while (true) {
    const uint8_t *valuePointer;
    uint16_t valueLength;
    EmberCoapOptionType type = emberReadNextOption(options,
                                                   &valuePointer,
                                                   &valueLength);
    if (type == EMBER_COAP_NO_OPTION) {
      break;
    } else if (wantType == type) {
      *valuePointers++ = valuePointer;
      *valueLengths++ = valueLength;
      count++;
      if (maxOptions <= count) {
        break;
      }
    }
  }
  return count;
}

static bool uriLookup(EmZclContext_t *context, EmZclUriPath *paths)
{
  uint8_t segmentsMatched = 0;
  bool ret = false;

  while (true) {
    uint8_t skip = paths->failSkip;

    if (paths->match(context, paths->data, segmentsMatched)) {
      segmentsMatched += 1;
      if (segmentsMatched == context->uriPathSegments) {
        if (context->code < EMBER_COAP_CODE_GET
            || EMBER_COAP_CODE_DELETE < context->code
            || (context->code == EMBER_COAP_CODE_GET
                && !READBITS(paths->flags, EM_ZCL_URI_FLAG_METHOD_GET))
            || (context->code == EMBER_COAP_CODE_POST
                && !READBITS(paths->flags, EM_ZCL_URI_FLAG_METHOD_POST))
            || (context->code == EMBER_COAP_CODE_PUT
                && !READBITS(paths->flags, EM_ZCL_URI_FLAG_METHOD_PUT))
            || (context->code == EMBER_COAP_CODE_DELETE
                && !READBITS(paths->flags, EM_ZCL_URI_FLAG_METHOD_DELETE))) {
          emZclRespond405MethodNotAllowed();
          ret = true;
        } else if (paths->action != NULL) {
          if (paths->queries != NULL
              && !processQueries(context, paths->queries)) {
            emZclRespond400BadRequest();
          } else {
            paths->action(context);
          }
          ret = true;
        }
        break;
      } else {
        skip = paths->matchSkip;
      }
    }

    if (skip == 255) {
      break;            // ran out of URIs to try
    } else {
      paths += skip;    // try next URI
    }
  }

  return ret;
}

static bool processQueries(EmZclContext_t *context, EmZclUriQuery *queries)
{
  context->uriQuerySegments = fetchCoapOptions(context->options,
                                               EMBER_COAP_OPTION_URI_QUERY,
                                               context->uriQuery,
                                               context->uriQueryLength,
                                               MAX_URI_PATH_SEGMENTS);

  // If we see a query that we don't know about, then just ignore it for the
  // sake of future compatibility.
  for (size_t segment = 0; segment < context->uriQuerySegments; segment ++) {
    for (EmZclUriQuery *query = queries; query->match != NULL; query ++) {
      if (query->match(context, query->data, segment)
          && !query->parse(context, query->data, segment)) {
        return false;
      }
    }
  }

  return true;
}

//----------------------------------------------------------------
// URI segment matching functions

static bool stringMatch(const uint8_t *bytes,
                        uint8_t bytesLength,
                        void *castString,
                        bool matchBytesLength)
{
  char *string = (char *) castString;
  const uint16_t length = strlen(string);
  return ((bytesLength == length || !matchBytesLength)
          && MEMCOMPARE(bytes, string, length) == 0);
}

bool emZclUriPathStringMatch(EmZclContext_t *context,
                             void *castString,
                             uint8_t depth)
{
  // char temp[100], *string = (char *) castString;
  // MEMCOPY(temp, context->uriPath[depth], context->uriPathLength[depth]);
  // temp[context->uriPathLength[depth]] = 0;
  // fprintf(stderr, "[want %s have %s]\n", string, temp);
  return stringMatch(context->uriPath[depth],
                     context->uriPathLength[depth],
                     castString,
                     true); // match bytes length
}

bool emZclUriQueryStringPrefixMatch(EmZclContext_t *context,
                                    void *castString,
                                    uint8_t depth)
{
  return stringMatch(context->uriQuery[depth],
                     context->uriQueryLength[depth],
                     castString,
                     false); // match string length
}

static bool uriManagementCommandIdMatch(EmZclContext_t *context, void *data, uint8_t depth)
{
  uintmax_t commandId;
  if (hexSegmentToInt(context, depth, sizeof(context->command->commandId), &commandId)) {
    context->command = emZclManagementFindCommand((EmberZclCommandId_t)commandId);
    return context->command != NULL;
  } else {
    return false;
  }
}

static bool uriEndpointIdMatch(EmZclContext_t *context, void *data, uint8_t depth)
{
  uintmax_t endpointId;
  if (hexSegmentToInt(context, depth, sizeof(context->endpoint->endpointId), &endpointId)) {
    context->endpoint = emZclFindEndpoint((EmberZclEndpointId_t)endpointId);
    return (context->endpoint != NULL);
  } else {
    return false;
  }
}

static bool uriGroupIdMatch(EmZclContext_t *context, void *data, uint8_t depth)
{
  uintmax_t groupId;
  if (hexSegmentToInt(context, depth, sizeof(context->groupId), &groupId)
      && emZclHasGroup((EmberZclGroupId_t)groupId)) {
    context->groupId = groupId;
    return true;
  } else {
    return false;
  }
}

static bool uriClusterIdMatch(EmZclContext_t *context, void *data, uint8_t depth)
{
  if (! emZclStringToCluster(context->uriPath[depth],
                             context->uriPathLength[depth],
                             &context->clusterSpec)) {
    return false;
  }

  if (context->groupId != EMBER_ZCL_GROUP_NULL) {
    size_t i;
    for (i = 0; i < emZclEndpointCount; i++) {
      if ((context->groupId == EMBER_ZCL_GROUP_ALL_ENDPOINTS
           || emberZclIsEndpointInGroup(emZclEndpointTable[i].endpointId,
                                        context->groupId))
          && emZclEndpointHasCluster(emZclEndpointTable[i].endpointId,
                                     &context->clusterSpec)) {
        return true;
      }
    }
    return false;
  } else {
    return emZclEndpointHasCluster(context->endpoint->endpointId,
                                   &context->clusterSpec);
  }
}

static bool uriAttributeIdMatch(EmZclContext_t *context, void *data, uint8_t depth)
{
  uintmax_t attributeId;
  if (hexSegmentToInt(context, depth, sizeof(context->attribute->attributeId), &attributeId)) {
    context->attribute = emZclFindAttribute(&context->clusterSpec,
                                            (EmberZclAttributeId_t)attributeId,
                                            false); // exclude remote
    return (context->attribute != NULL);
  } else {
    return false;
  }
}

static bool uriBindingIdMatch(EmZclContext_t *context, void *data, uint8_t depth)
{
  uintmax_t bindingId;
  if (hexSegmentToInt(context, depth, sizeof(context->bindingId), &bindingId)
      && emZclHasBinding(context, (EmberZclBindingId_t)bindingId)) {
    context->bindingId = bindingId;
    return true;
  } else {
    return false;
  }
}

static bool uriClusterCommandIdMatch(EmZclContext_t *context, void *data, uint8_t depth)
{
  uintmax_t commandId;
  if (hexSegmentToInt(context, depth, sizeof(context->command->commandId), &commandId)) {
    context->command = emZclFindCommand(&context->clusterSpec, (EmberZclCommandId_t)commandId);
    return (context->command != NULL);
  } else {
    return false;
  }
}

static bool uriReportingIdMatch(EmZclContext_t *context, void *data, uint8_t depth)
{
  uintmax_t reportingConfigurationId;
  if (hexSegmentToInt(context, depth, sizeof(context->reportingConfigurationId), &reportingConfigurationId)
      && emZclHasReportingConfiguration(context->endpoint->endpointId,
                                        &context->clusterSpec,
                                        (EmberZclReportingConfigurationId_t)reportingConfigurationId)) {
    context->reportingConfigurationId = reportingConfigurationId;
    return true;
  } else {
    return false;
  }
}

//----------------------------------------------------------------
// URI handlers

// get zcl:
static const uint8_t egmPayload[] = { // ["e", "g", "m"]
  0x9F, 0x61, 0x65, 0x61, 0x67, 0x61, 0x6D, 0xFF
};
static void zclHandler(EmZclContext_t *context)
{
  emZclRespond205ContentCbor(egmPayload, sizeof(egmPayload));
}

// GET zcl/e/XX/[cs]XXXX:
// GET zcl/e/XX/[cs]XXXXXXXX:
// GET zcl/g/XXXX/[cs]XXXX:
// GET zcl/g/XXXX/[cs]XXXXXXXX:
static const uint8_t abcnrPayload[] = { // ["a", "b", "c", "n", "r"]
  0x85, 0x61, 0x61, 0x61, 0x62, 0x61, 0x63, 0x61, 0x6e, 0x61, 0x72,
};
static void clusterHandler(EmZclContext_t *context)
{
  emZclRespond205ContentCbor(abcnrPayload, sizeof(abcnrPayload));
}

static bool hexSegmentToInt(const EmZclContext_t *context,
                            uint8_t depth,
                            size_t size,
                            uintmax_t *result)
{
  return (context->uriPathLength[depth] <= size * 2 // bytes to nibbles
          && emZclHexStringToInt(context->uriPath[depth],
                                 context->uriPathLength[depth],
                                 result));
}

bool emberZclCompareClusterSpec(const EmberZclClusterSpec_t *s1,
                                const EmberZclClusterSpec_t *s2)
{
  return (s1->role == s2->role
          && s1->manufacturerCode == s2->manufacturerCode
          && s1->id == s2->id);
}

void emberZclReverseClusterSpec(const EmberZclClusterSpec_t *s1,
                                EmberZclClusterSpec_t *s2)
{
  s2->role = (s1->role == EMBER_ZCL_ROLE_CLIENT
              ? EMBER_ZCL_ROLE_SERVER
              : EMBER_ZCL_ROLE_CLIENT);
  s2->manufacturerCode = s1->manufacturerCode;
  s2->id = s1->id;
}

EmberStatus emZclRespondNoPayload(EmberCoapCode code)
{
  EmberStatus status = emberCoapRespondWithCode(currentInfo, code);
  currentInfo = NULL;
  return status;
}

EmberStatus emZclRespond201Created(const uint8_t *locationPath)
{
  EmberStatus status = emberCoapRespondWithPath(currentInfo,
                                                EMBER_COAP_CODE_201_CREATED,
                                                locationPath,
                                                NULL, // options
                                                0,    // number of options
                                                NULL, // payload
                                                0);   // payload length
  currentInfo = NULL;
  return status;
}

EmberStatus emZclRespond202Deleted(void)
{
  return emZclRespondNoPayload(EMBER_COAP_CODE_202_DELETED);
}

EmberStatus emZclRespond204Changed(void)
{
  return emZclRespondNoPayload(EMBER_COAP_CODE_204_CHANGED);
}

EmberStatus emZclRespond204ChangedCborState(const CborState *state)
{
  EmberCoapOption options[] = {
    {EMBER_COAP_OPTION_CONTENT_FORMAT, NULL, 1, EMBER_COAP_CONTENT_FORMAT_CBOR,},
  };
  EmberStatus status =  emberCoapRespondWithPath(currentInfo,
                                                 EMBER_COAP_CODE_204_CHANGED,
                                                 NULL, // location path
                                                 options,
                                                 COUNTOF(options),
                                                 state->start,
                                                 emCborEncodeSize(state));
  currentInfo = NULL;
  return status;
}

EmberStatus emZclRespond205ContentCbor(const uint8_t *payload,
                                       uint16_t payloadLength)
{
  EmberCoapOption options[] = {
    {EMBER_COAP_OPTION_CONTENT_FORMAT, NULL, 1, EMBER_COAP_CONTENT_FORMAT_CBOR,},
  };
  EmberStatus status = emberCoapRespondWithPath(currentInfo,
                                                EMBER_COAP_CODE_205_CONTENT,
                                                NULL, // location path
                                                options,
                                                COUNTOF(options),
                                                payload,
                                                payloadLength);
  currentInfo = NULL;
  return status;
}

EmberStatus emZclRespond205ContentCborState(const CborState *state)
{
  return emZclRespond205ContentCbor(state->start, emCborEncodeSize(state));
}

EmberStatus emZclRespond205ContentLinkFormat(const uint8_t *payload,
                                             uint16_t payloadLength)
{
  EmberCoapOption options[] = {
    {EMBER_COAP_OPTION_CONTENT_FORMAT, NULL, 1, EMBER_COAP_CONTENT_FORMAT_LINK_FORMAT,},
  };
  EmberStatus status = emberCoapRespondWithPath(currentInfo,
                                                EMBER_COAP_CODE_205_CONTENT,
                                                NULL, // location path
                                                options,
                                                COUNTOF(options),
                                                payload,
                                                payloadLength);
  currentInfo = NULL;
  return status;
}

EmberStatus emZclRespond400BadRequest(void)
{
  return emZclRespondNoPayload(EMBER_COAP_CODE_400_BAD_REQUEST);
}

EmberStatus emZclRespond404NotFound(void)
{
  return emZclRespondNoPayload(EMBER_COAP_CODE_404_NOT_FOUND);
}

EmberStatus emZclRespond405MethodNotAllowed(void)
{
  return emZclRespondNoPayload(EMBER_COAP_CODE_405_METHOD_NOT_ALLOWED);
}

EmberStatus emZclRespond413RequestEntityTooLarge(void)
{
  return emZclRespondNoPayload(EMBER_COAP_CODE_413_REQUEST_ENTITY_TOO_LARGE);
}

EmberStatus emZclRespond500InternalServerError(void)
{
  return emZclRespondNoPayload(EMBER_COAP_CODE_500_INTERNAL_SERVER_ERROR);
}

bool emZclGetMulticastAddress(EmberIpv6Address *dst)
{
  EmberNetworkParameters parameters = {{0}};
  
  if (dst == NULL){
    return false;
  }

  MEMCOPY(dst,
          &allThreadNodes, 
          sizeof(EmberIpv6Address));
  emberGetNetworkParameters(&parameters);
  MEMCOPY(dst->bytes + RFC3306_NETWORK_PREFIX_OFFSET,
          &parameters.ulaPrefix,
          sizeof(EmberIpv6Prefix));
  return true;
}

// parses "</zcl/e/EE/[cs]CCCC>" into EmZclUriContext_t struct
uint16_t emZclParseUri(const uint8_t *payload, EmZclUriContext_t *context)
{
  char zclTag[] = "</zcl/e/";
  uint8_t zclTagLen = strlen(zclTag);
  char *finger = (char *)payload;
  if (MEMCOMPARE(finger, zclTag, zclTagLen) == 0) {
    uintmax_t endpointId;
    char *delimiter = NULL;
    char *endBracket = NULL;
    EmberZclClusterSpec_t clusterSpec;
    finger += zclTagLen;

    // parsing endpoint number
    if ((delimiter = strchr(finger, '/')) == NULL) {
      return 0;
    }

    if (!emZclHexStringToInt((const uint8_t *)finger,
                             delimiter - finger,
                             &endpointId)) {
      return 0;
    }

    finger = delimiter;
    finger++;

    // parsing cluster 
    if ((endBracket = strchr(finger, '>')) == NULL){
      return 0;
    }

    if (!emZclStringToCluster((const uint8_t *)finger,
                              endBracket - finger,
                              &clusterSpec)) {
      return 0;
    }
    
    context->endpointId = endpointId;
    MEMCOPY(context->clusterSpec, &clusterSpec, sizeof(EmberZclClusterSpec_t));
    return (uint16_t)(endBracket - (char *)payload + 1);
  }

  return 0;
}
