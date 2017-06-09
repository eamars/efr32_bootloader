// *******************************************************************
// * reporting.h
// *
// *
// * Copyright 2011 by Ember Corporation. All rights reserved.              *80*
// *******************************************************************

typedef struct {
  uint32_t lastReportTimeMs;
  EmberAfDifferenceType lastReportValue;
  bool reportableChange;
} EmAfPluginReportVolatileData;
extern EmAfPluginReportVolatileData emAfPluginReportVolatileData[];
EmberAfStatus emberAfPluginReportingConfigureReportedAttribute(const EmberAfPluginReportingEntry *newEntry);
void emAfPluginReportingGetEntry(uint8_t index, EmberAfPluginReportingEntry *result);
void emAfPluginReportingSetEntry(uint8_t index, EmberAfPluginReportingEntry *value);
uint8_t emAfPluginReportingAddEntry(EmberAfPluginReportingEntry* newEntry);
EmberStatus emAfPluginReportingRemoveEntry(uint8_t index);
bool emAfPluginReportingDoEntriesMatch(const EmberAfPluginReportingEntry* const entry1,
                                       const EmberAfPluginReportingEntry* const entry2);
uint8_t emAfPluginReportingConditionallyAddReportingEntry(EmberAfPluginReportingEntry* newEntry);
