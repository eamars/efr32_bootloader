// *****************************************************************************
// * eeprom.h
// *
// * Header file for eeprom plugin API.
// *
// * Copyright 2012 by Silicon Labs.      All rights reserved.              *80*
// *****************************************************************************


void emberAfPluginEepromInit(void);

void emberAfPluginEepromNoteInitializedState(bool state);

uint8_t emberAfPluginEepromGetWordSize(void);
const HalEepromInformationType* emberAfPluginEepromInfo(void);

uint8_t emberAfPluginEepromWrite(uint32_t address, const uint8_t *data, uint16_t totalLength);

uint8_t emberAfPluginEepromRead(uint32_t address, uint8_t *data, uint16_t totalLength);

// Erase has a 32-bit argument, since it's possible to erase more than uint16_t chunk.
// Read and write have only uint16_t for length, because you don't have enough RAM
// for the data buffer
uint8_t emberAfPluginEepromErase(uint32_t address, uint32_t totalLength);

bool emberAfPluginEepromBusy(void);

bool emberAfPluginEepromShutdown(void);

uint8_t emberAfPluginEepromFlushSavedPartialWrites(void);

#if defined(EMBER_TEST)
void emAfPluginEepromFakeEepromCallback(void);
#endif


// Currently there are no EEPROM/flash parts that we support that have a word size
// of 4.  The local storage bootloader has a 2-byte word size and that is the main
// thing we are optimizing for.
#define EM_AF_EEPROM_MAX_WORD_SIZE 2

typedef struct {
  uint32_t address;
  uint8_t data;
} EmAfPartialWriteStruct;

extern EmAfPartialWriteStruct emAfEepromSavedPartialWrites[];
bool emAfIsEepromInitialized(void);

