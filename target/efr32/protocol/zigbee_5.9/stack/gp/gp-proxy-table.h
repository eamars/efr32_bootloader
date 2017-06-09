/*
 * File: gp-proxy-table.h
 * Description: Zigbee GP token definitions used by the stack.
 *
 * Author(s): Jeffrey Rosenberger, jeffrey.rosenberger@silabs.com
 *
 * Copyright 2015 Silicon Laboratories, Inc.                                *80*
 */

//these are declared in the config/ember-configuration.c
extern uint8_t emGpProxyTableSize;
extern EmberGpProxyTableEntry *emGpProxyTable;

//#define EMBER_GP_PROXY_TABLE_ENTRY_STATUS_MASK 0x01

uint8_t emGpProxyTableEntryInUse(uint8_t index);
void emGpClearProxyTable(void);

/*
#define emGpProxyTableEntryInUse(index)                                  \
  ((emGpProxyTable[(index)].status                                          \
    & EMBER_GP_PROXY_TABLE_ENTRY_STATUS_MASK)                        \
    == EMBER_GP_PROXY_TABLE_ENTRY_STATUS_ACTIVE)
    */

#define emGpProxyTableEntryUnused(index)                                  \
  (emGpProxyTable[(index)].status                                          \
    == EMBER_GP_PROXY_TABLE_ENTRY_STATUS_UNUSED)


#define  emGpProxyTableGetAddr(index)                                     \
  (&(emGpProxyTable[(index)].gpd))

#define emGpProxyTableSetSecurityFrameCounter(index, sfc)                             \
  (emGpProxyTable[(index)].gpdSecurityFrameCounter = (sfc))

#define emGpProxyTableGetSecurityFrameCounter(index) \
  (emGpProxyTable[(index)].gpdSecurityFrameCounter)

#define emGpProxyTableGetOptions(index) \
  (emGpProxyTable[(index)].options)

#define emGpProxyTableSetOptions(index,o) \
  (emGpProxyTable[(index)].options=0)

#define emGpProxyTableGetSecurityOptions(index) \
  (emGpProxyTable[(index)].securityOptions)

#define emGpProxyTableSetSecurityOptions(index,o) \
  (emGpProxyTable[(index)].securityOptions=o)

#define emGpProxyTableGetSinkList(index) \
  (emGpProxyTable[(index)].sinkList)

#define emGpProxyTableEntryHasLinkKey(index)  \
  (((emGpProxyTable[(index)].securityOptions) & 0x1C ) == 0x10)

#define emGpProxyTableGetSecurityKey(index)                               \
  (emGpProxyTable[(index)].gpdKey)

#define emGpProxyTableSetStatus(index,s) \
  (emGpProxyTable[(index)].status=(s))

#define emGpProxyTableGetStatus(index) \
  (emGpProxyTable[(index)].status)

#define emGpProxyTableGetAssignedAlias(index) \
  (emGpProxyTable[(index)].assignedAlias)

#define emGpProxyTableSetInRange(index) \
  (emGpProxyTable[(index)].options |= GP_PROXY_TABLE_OPTIONS_IN_RANGE)


void emGpProxyTableInit();
EmberStatus emGpProxyTableSetEntry(uint8_t proxyIndex,
                                           EmberGpProxyTableEntry *entry);
EmberStatus emGpProxyTableGetEntry(uint8_t proxyIndex,
                                           EmberGpProxyTableEntry *entry);
uint8_t emGpProxyTableGetFreeEntryIndex(void);
uint8_t emGpProxyTableLookup(const EmberGpAddress *addr);
uint8_t emGpProxyTableFindOrAllocateEntry(const EmberGpAddress *addr);
//void emGpProxyTableAddSink(uint8_t index,uint16_t options,EmberEUI64 sinkIeeeAddress,EmberNodeId sinkNwkAddress,uint16_t sinkGroupId,uint32_t gpdSecurityFrameCounter,uint8_t *gpdKey,uint16_t assignedAlias,uint8_t forwardingRadius);
void emGpProxyTableAddSink(uint8_t index,
                     //      uint16_t options,
                           uint8_t commMode,
                           EmberEUI64 sinkIeeeAddress,
                           EmberNodeId sinkNwkAddress,
                           uint16_t sinkGroupId,
//                           uint32_t gpdSecurityFrameCounter,
 //                          uint8_t *gpdKey,
                           uint16_t assignedAlias
   //                        uint8_t forwardingRadius)
                           );
void emGpProxyTableRemoveSink(uint8_t index,EmberEUI64 sinkIeeeAddress);
uint8_t emGpProxyTableLookup(const EmberGpAddress *addr);
void emGpProxyTableRemoveEntry(uint8_t index);
void emGpProxyTableSetKey(uint8_t index, uint8_t * gpdKey);
void emGpProxyTableGetKey(uint8_t index, EmberKeyData *key);
bool emGpAddressMatch(const EmberGpAddress *a1,const EmberGpAddress *a2);
bool emberGpProxyTableProcessGpPairing(uint32_t options,
                                       EmberGpAddress* addr,
                                       uint8_t commMode,
                                       uint16_t sinkNwkAddress,
                                       uint16_t sinkGroupId,
                                       uint16_t assignedAlias,
                                       uint8_t* sinkIeeeAddress,
                                       EmberKeyData *gpdKey);

