// Copyright 2016 Silicon Laboratories, Inc.

// TODO: Pick creator codes.
#define CREATOR_ZCL_CORE_BINDING_TABLE 0x7A62 // zb - ZCL/IP bindings
#define CREATOR_ZCL_CORE_GROUP_TABLE   0x7A67 // zg - ZCL/IP groups

#ifdef DEFINETYPES
  #include "zcl-core.h"
#endif

#ifdef DEFINETOKENS
  DEFINE_INDEXED_TOKEN(ZCL_CORE_BINDING_TABLE,
                       EmberZclBindingEntry_t,
                       EMBER_ZCL_BINDING_TABLE_SIZE,
                       {EMBER_ZCL_ENDPOINT_NULL,})
  DEFINE_INDEXED_TOKEN(ZCL_CORE_GROUP_TABLE,
                       EmberZclGroupEntry_t,
                       EMBER_ZCL_GROUP_TABLE_SIZE,
                       {EMBER_ZCL_GROUP_NULL,})
#endif
