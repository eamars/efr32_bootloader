// Copyright 2015 Silicon Laboratories, Inc.

#include PLATFORM_HEADER
#include CONFIGURATION_HEADER
#include "zcl-core.h"

const EmZclEndpointEntry_t *emZclFindEndpoint(EmberZclEndpointId_t endpointId)
{
  EmberZclEndpointIndex_t index = emberZclEndpointIdToIndex(endpointId, NULL);
  return (index == EMBER_ZCL_ENDPOINT_INDEX_NULL
          ? NULL
          : &emZclEndpointTable[index]);
}

bool emZclEndpointHasCluster(EmberZclEndpointId_t endpointId,
                             const EmberZclClusterSpec_t *clusterSpec)
{
  const EmZclEndpointEntry_t *endpoint = emZclFindEndpoint(endpointId);
  if (endpoint != NULL) {
    for (size_t i = 0; endpoint->clusterSpecs[i] != NULL; i++) {
      if (emberZclCompareClusterSpec(clusterSpec, endpoint->clusterSpecs[i])) {
        return true;
      }
    }
  }
  return false;
}

bool emZclMultiEndpointDispatch(const EmZclContext_t *context,
                                EmZclMultiEndpointHandler handler,
                                CborState *state,
                                void *data)
{
  if (context->groupId == EMBER_ZCL_GROUP_NULL) {
    return (*handler)(context, state, data);
  } else if (!emCborEncodeIndefiniteMap(state)) {
    return false;
  } else {
    for (size_t i = 0; i < emZclEndpointCount; i++) {
      if ((context->groupId == EMBER_ZCL_GROUP_ALL_ENDPOINTS
           || emberZclIsEndpointInGroup(emZclEndpointTable[i].endpointId,
                                        context->groupId))
          && emZclEndpointHasCluster(emZclEndpointTable[i].endpointId,
                                     &context->clusterSpec)) {
        ((EmZclContext_t *)context)->endpoint = &emZclEndpointTable[i];
        if (!emCborEncodeValue(state,
                               EMBER_ZCLIP_TYPE_UNSIGNED_INTEGER,
                               sizeof(emZclEndpointTable[i].endpointId),
                               (const uint8_t *)&emZclEndpointTable[i].endpointId)
            || !(*handler)(context, state, data)) {
          return false;
        }
      }
    }
    return emCborEncodeBreak(state);
  }
}

EmberZclEndpointIndex_t emberZclEndpointIdToIndex(EmberZclEndpointId_t endpointId,
                                                  const EmberZclClusterSpec_t *clusterSpec)
{
  EmberZclEndpointIndex_t index = 0;
  for (size_t i = 0;
       (i < emZclEndpointCount
        && emZclEndpointTable[i].endpointId <= endpointId);
       i++) {
    if (clusterSpec == NULL
        || emZclEndpointHasCluster(emZclEndpointTable[i].endpointId,
                                   clusterSpec)) {
      if (endpointId == emZclEndpointTable[i].endpointId) {
        return index;
      } else {
        index++;
      }
    }
  }
  return EMBER_ZCL_ENDPOINT_INDEX_NULL;
}

EmberZclEndpointId_t emberZclEndpointIndexToId(EmberZclEndpointIndex_t index,
                                               const EmberZclClusterSpec_t *clusterSpec)
{
  for (size_t i = 0; i < emZclEndpointCount; i++) {
    if (clusterSpec == NULL
        || emZclEndpointHasCluster(emZclEndpointTable[i].endpointId,
                                   clusterSpec)) {
      if (index == 0) {
        return emZclEndpointTable[i].endpointId;
      } else {
        index--;
      }
    }
  }
  return EMBER_ZCL_ENDPOINT_NULL;
}

// zcl/e:
//   GET: list endpoints.
//   OTHER: not allowed.
void emZclUriEndpointHandler(EmZclContext_t *context)
{
  CborState state;
  uint8_t buffer[128];
  emCborEncodeIndefiniteArrayStart(&state, buffer, sizeof(buffer));
  for (size_t i = 0; i < emZclEndpointCount; i++) {
    if (!emCborEncodeValue(&state,
                           EMBER_ZCLIP_TYPE_UNSIGNED_INTEGER,
                           sizeof(EmberZclEndpointId_t),
                           &emZclEndpointTable[i].endpointId)) {
      emZclRespond500InternalServerError();
      return;
    }
  }
  emCborEncodeBreak(&state);
  emZclRespond205ContentCborState(&state);
}

// zcl/e/XX:
//   GET: list clusters on endpoint.
//   OTHER: not allowed.
void emZclUriEndpointIdHandler(EmZclContext_t *context)
{
  CborState state;
  uint8_t buffer[128];
  emCborEncodeIndefiniteArrayStart(&state, buffer, sizeof(buffer));
  for (size_t i = 0; context->endpoint->clusterSpecs[i] != NULL; i++) {
    uint8_t clusterId[EMBER_ZCL_URI_PATH_CLUSTER_ID_MAX_LENGTH];
    emZclClusterToString(context->endpoint->clusterSpecs[i], clusterId);
    if (!emCborEncodeValue(&state,
                           EMBER_ZCLIP_TYPE_MAX_LENGTH_STRING,
                           sizeof(clusterId),
                           clusterId)) {
      emZclRespond500InternalServerError();
      return;
    }
  }
  emCborEncodeBreak(&state);
  emZclRespond205ContentCborState(&state);
}
