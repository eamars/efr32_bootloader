// *****************************************************************************
// * ota-storage-simple-simulation.c
// *
// * This code will load a file from disk into the 'Simple Storage' plugin.
// * 
// * Copyright 2012 by Ember Corporation. All rights reserved.              *80*
// *****************************************************************************

#include "app/framework/include/af.h"
#include "app/framework/plugin/ota-common/ota.h"

#if defined(EMBER_TEST)

#include "app/util/serial/command-interpreter2.h"

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

//------------------------------------------------------------------------------
// Globals

#define MAX_READ_SIZE 512
#define MAX_PATH_SIZE 512

//------------------------------------------------------------------------------
// Forward Declarations

//------------------------------------------------------------------------------

static bool loadFileIntoOtaStorage(const char* file)
{
  FILE* fh = fopen(file, "rb");
  char cwd[MAX_PATH_SIZE];
    
  assert(NULL != getcwd(cwd, MAX_PATH_SIZE));

  emberAfOtaBootloadClusterFlush();
  otaPrintln("Current directory: '%s'", cwd);
  emberAfOtaBootloadClusterFlush();

  if (fh == NULL) {
    otaPrintln("Failed to open file: %p",
               strerror(errno));
    return false;
  }

  struct stat buffer;
  if (0 != stat(file, &buffer)) {
    otaPrintln("Failed to stat() file: %p",
               strerror(errno));
    return false;
  }

  EmberAfOtaStorageStatus status = emberAfOtaStorageClearTempDataCallback();
  if (status != EMBER_AF_OTA_STORAGE_SUCCESS) {
    otaPrintln("Failed to delete existing OTA file.");
    return false;
  }
  
  off_t offset = 0;
  while (offset < buffer.st_size) {
    uint8_t data[MAX_READ_SIZE];
    off_t readSize = (buffer.st_size - offset > MAX_READ_SIZE
                      ? MAX_READ_SIZE
                      : buffer.st_size - offset);
    size_t readAmount = fread(data, 1, readSize, fh);
    if (readAmount != readSize) {
      otaPrintln("Failed to read %d bytes from file at offset 0x%4X",
                 readSize,
                 offset);
      status = EMBER_AF_OTA_STORAGE_ERROR;
      goto loadStorageDone;
    }

    status = emberAfOtaStorageWriteTempDataCallback(offset,
                                                    readAmount,
                                                    data);
    if (status != EMBER_AF_OTA_STORAGE_SUCCESS) {
      otaPrintln("Failed to load data into temp storage.");
      goto loadStorageDone;
    }

    offset += readAmount;
  }

  status = emberAfOtaStorageFinishDownloadCallback(offset);

  if (status == EMBER_AF_OTA_STORAGE_SUCCESS) {
    uint32_t totalSize;
    uint32_t offset;
    EmberAfOtaImageId id;
    status = emberAfOtaStorageCheckTempDataCallback(&offset,
                                                    &totalSize,
                                                    &id);

    if (status != EMBER_AF_OTA_STORAGE_SUCCESS) {
      otaPrintln("Failed to validate OTA file.");
      goto loadStorageDone;
    }

    otaPrintln("Loaded image successfully.");
  }

 loadStorageDone:
  fclose(fh);
  return status;
}

#endif // EMBER_TEST

#define MAX_FILENAME_SIZE   255

// TODO: this should be gated once we set up a gating mechanism for the 
// generated CLI
void emAfOtaLoadFileCommand(void)
{
  char filename[MAX_FILENAME_SIZE];
  uint8_t length = emberCopyStringArgument(0,
                                           filename,
                                           MAX_FILENAME_SIZE,
                                           false);
  if (length >= MAX_FILENAME_SIZE) {
    otaPrintln("OTA ERR: filename '%s' is too long (max %d chars)",
               filename,
               MAX_FILENAME_SIZE-1);
    return;
  }
  filename[length] = '\0';
  otaPrintln("Loading from file: '%s'", filename);
  loadFileIntoOtaStorage(filename);  
}


