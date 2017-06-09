// File: source-route-common.h
//
// Description: Common code used for managing source routes on both node-based
// and host-based gateways. See source-route.c for node-based gateways and
// source-route-host.c for host-based gateways.
// 
// Copyright 2007 by Ember Corporation. All rights reserved.                *80*

#ifndef __SOURCE_ROUTE_COMMON_H__
#define __SOURCE_ROUTE_COMMON_H__

typedef struct {
  EmberNodeId destination;
  uint8_t closerIndex;          // The entry one hop closer to the gateway.
  uint8_t olderIndex;           // The entry touched before this one.
} SourceRouteTableEntry;

extern uint8_t sourceRouteTableSize;
extern SourceRouteTableEntry *sourceRouteTable;

// A special index. For destinations that are neighbors of the gateway,
// closerIndex is set to 0xFF. For the oldest entry, olderIndex is set to
// 0xFF.
#define NULL_INDEX 0xFF

uint8_t sourceRouteFindIndex(EmberNodeId id);
uint8_t sourceRouteAddEntry(EmberNodeId id, uint8_t furtherIndex);
void sourceRouteInit(void);
uint8_t sourceRouteGetCount(void);
void sourceRouteClearTable(void);
uint8_t sourceRouteAddEntryWithCloserNextHop(EmberNodeId newId,
                                           EmberNodeId closerNodeId);
uint8_t emberGetSourceRouteOverhead(EmberNodeId destination);

#endif // __SOURCE_ROUTE_COMMON_H__
