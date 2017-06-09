// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "app/framework/plugin/rf4ce-mso/rf4ce-mso.h"
#include "rf4ce-mso-ir-rf-database-recipient.h"

#ifdef EMBER_AF_LEGACY_CLI
  #error The RF4CE MSO IR-RF Database Recipient plugin is not compatible with the legacy CLI.
#endif

#define RF_PAYLOAD_MAX 32
#define IR_CODE_MAX 32
static EmberAfRf4ceMsoIrRfDatabaseEntry cliEntry;
static uint8_t rfPressedPayload[RF_PAYLOAD_MAX];
static uint8_t rfRepeatedPayload[RF_PAYLOAD_MAX];
static uint8_t rfReleasedPayload[RF_PAYLOAD_MAX];
static uint8_t irCode[IR_CODE_MAX];

// plugin rf4ce-mso-ir-rf-database-recipient add-flags <flags:1>
void emberAfRf4ceMsoIrRfDatabaseRecipientAddFlagsCommand(void)
{
  cliEntry.flags
    = (EmberAfRf4ceMsoIrRfDatabaseFlags)emberUnsignedCommandArgument(0);
}

// plugin rf4ce-mso-ir-rf-database-recipient add-pressed <rf config:1> <tx options:1> <payload:n>
void emberAfRf4ceMsoIrRfDatabaseRecipientAddPressedCommand(void)
{
  cliEntry.rfPressedDescriptor.rfConfig
    = (EmberAfRf4ceMsoIrRfDatabaseRfConfig)emberUnsignedCommandArgument(0);
  cliEntry.rfPressedDescriptor.txOptions
    = (EmberRf4ceTxOption)emberUnsignedCommandArgument(1);
  cliEntry.rfPressedDescriptor.payloadLength
    = emberCopyStringArgument(2,
                              rfPressedPayload,
                              RF_PAYLOAD_MAX,
                              false);
  cliEntry.rfPressedDescriptor.payload = rfPressedPayload;
}

// plugin rf4ce-mso-ir-rf-database-recipient add-repeated <rf config:1> <tx options:1> <payload:n>
void emberAfRf4ceMsoIrRfDatabaseRecipientAddRepeatedCommand(void)
{
  cliEntry.rfRepeatedDescriptor.rfConfig
    = (EmberAfRf4ceMsoIrRfDatabaseRfConfig)emberUnsignedCommandArgument(0);
  cliEntry.rfRepeatedDescriptor.txOptions
    = (EmberRf4ceTxOption)emberUnsignedCommandArgument(1);
  cliEntry.rfRepeatedDescriptor.payloadLength
    = emberCopyStringArgument(2,
                             rfRepeatedPayload,
                             RF_PAYLOAD_MAX,
                             false);
  cliEntry.rfRepeatedDescriptor.payload = rfRepeatedPayload;
}

// plugin rf4ce-mso-ir-rf-database-recipient add-released <rf config:1> <tx options:1> <payload:n>
void emberAfRf4ceMsoIrRfDatabaseRecipientAddReleasedCommand(void)
{
  cliEntry.rfReleasedDescriptor.rfConfig
    = (EmberAfRf4ceMsoIrRfDatabaseRfConfig)emberUnsignedCommandArgument(0);
  cliEntry.rfReleasedDescriptor.txOptions
    = (EmberRf4ceTxOption)emberUnsignedCommandArgument(1);
  cliEntry.rfReleasedDescriptor.payloadLength
    = emberCopyStringArgument(2,
                              rfReleasedPayload,
                              RF_PAYLOAD_MAX,
                              false);
  cliEntry.rfReleasedDescriptor.payload = rfReleasedPayload;
}

// plugin rf4ce-mso-ir-rf-database-recipient add-ir <ir config:1> <ir code:n>
void emberAfRf4ceMsoIrRfDatabaseRecipientAddIrCommand(void)
{
  cliEntry.irDescriptor.irConfig
    = (EmberAfRf4ceMsoIrRfDatabaseRfConfig)emberUnsignedCommandArgument(0);
  cliEntry.irDescriptor.irCodeLength
    = emberCopyStringArgument(1,
                              irCode,
                              IR_CODE_MAX,
                              false);
  cliEntry.irDescriptor.irCode = irCode;
}

// plugin rf4ce-mso-ir-rf-database-recipient add <key code:1>
void emberAfRf4ceMsoIrRfDatabaseRecipientAddCommand(void)
{
  EmberAfRf4ceMsoKeyCode keyCode
    = (EmberAfRf4ceMsoKeyCode)emberUnsignedCommandArgument(0);
  EmberStatus status = emberAfRf4ceMsoIrRfDatabaseRecipientAdd(keyCode,
                                                               &cliEntry);
  emberAfAppPrintln("%p 0x%x", "add", status);
}

// plugin rf4ce-mso-ir-rf-database-recipient remove <key code:1>
void emberAfRf4ceMsoIrRfDatabaseRecipientRemoveCommand(void)
{
  EmberAfRf4ceMsoKeyCode keyCode
    = (EmberAfRf4ceMsoKeyCode)emberUnsignedCommandArgument(0);
  EmberStatus status = emberAfRf4ceMsoIrRfDatabaseRecipientRemove(keyCode);
  emberAfAppPrintln("%p 0x%x", "remove", status);
}

// plugin rf4ce-mso-ir-rf-database-recipient remove-all
void emberAfRf4ceMsoIrRfDatabaseRecipientRemoveAllCommand(void)
{
  emberAfRf4ceMsoIrRfDatabaseRecipientRemoveAll();
  emberAfAppPrintln("%p 0x%x", "remove-all", 0x00);
}

// plugin rf4ce-mso-ir-rf-database-recipient get-callback <key code:1>
void emberAfRf4ceMsoIrRfDatabaseRecipientGetCallbackCommand(void)
{
  uint8_t i;
  uint8_t buffer[128];
  uint8_t bufferLength = sizeof(buffer);
  uint8_t* finger = buffer;
  EmberAfRf4ceMsoIrRfDatabaseEntry entry;
  EmberAfRf4ceMsoKeyCode keyCode
    = (EmberAfRf4ceMsoKeyCode)emberUnsignedCommandArgument(0);
  EmberAfRf4ceStatus status
    = emberAfPluginRf4ceMsoGetIrRfDatabaseAttributeCallback(0,
                                                            keyCode,
                                                            &bufferLength,
                                                            buffer);

  if (status == EMBER_AF_RF4CE_STATUS_SUCCESS) {
    entry.flags = *finger++;
    emberAfAppPrintln("flags: 0x%x", entry.flags);
    if (emberAfRf4ceMsoIrRfDatabaseEntryHasRfPressedDescriptor(&entry)) {
      entry.rfPressedDescriptor.rfConfig = *finger++;
      entry.rfPressedDescriptor.txOptions = *finger++;
      entry.rfPressedDescriptor.payloadLength = *finger++;
      emberAfAppPrintln("rfConfig: 0x%x", entry.rfPressedDescriptor.rfConfig);
      emberAfAppPrintln("txOptions: 0x%x", entry.rfPressedDescriptor.txOptions);
      emberAfAppPrintln("payloadLength: 0x%x", entry.rfPressedDescriptor.payloadLength);
      emberAfAppPrint("payload: ");
      for (i=0; i<entry.rfPressedDescriptor.payloadLength; i++) {
        emberAfAppPrint("0x%x ", finger[i]);
      }
      emberAfAppPrintln("");
      finger += entry.rfPressedDescriptor.payloadLength;
    }
    if (emberAfRf4ceMsoIrRfDatabaseEntryHasRfRepeatedDescriptor(&entry)) {
      entry.rfRepeatedDescriptor.rfConfig = *finger++;
      entry.rfRepeatedDescriptor.txOptions = *finger++;
      entry.rfRepeatedDescriptor.payloadLength = *finger++;
      emberAfAppPrintln("rfConfig: 0x%x", entry.rfRepeatedDescriptor.rfConfig);
      emberAfAppPrintln("txOptions: 0x%x", entry.rfRepeatedDescriptor.txOptions);
      emberAfAppPrintln("payloadLength: 0x%x", entry.rfRepeatedDescriptor.payloadLength);
      emberAfAppPrint("payload: ");
      for (i=0; i<entry.rfRepeatedDescriptor.payloadLength; i++) {
        emberAfAppPrint("0x%x ", finger[i]);
      }
      emberAfAppPrintln("");
      finger += entry.rfRepeatedDescriptor.payloadLength;
    }
    if (emberAfRf4ceMsoIrRfDatabaseEntryHasRfReleasedDescriptor(&entry)) {
      entry.rfReleasedDescriptor.rfConfig = *finger++;
      entry.rfReleasedDescriptor.txOptions = *finger++;
      entry.rfReleasedDescriptor.payloadLength = *finger++;
      emberAfAppPrintln("rfConfig: 0x%x", entry.rfReleasedDescriptor.rfConfig);
      emberAfAppPrintln("txOptions: 0x%x", entry.rfReleasedDescriptor.txOptions);
      emberAfAppPrintln("payloadLength: 0x%x", entry.rfReleasedDescriptor.payloadLength);
      emberAfAppPrint("payload: ");
      for (i=0; i<entry.rfReleasedDescriptor.payloadLength; i++) {
        emberAfAppPrint("0x%x ", finger[i]);
      }
      emberAfAppPrintln("");
      finger += entry.rfReleasedDescriptor.payloadLength;
    }
    if (emberAfRf4ceMsoIrRfDatabaseEntryHasIrDescriptor(&entry)) {
      entry.irDescriptor.irConfig = *finger++;
      entry.irDescriptor.irCodeLength = *finger++;
      emberAfAppPrintln("irConfig: 0x%x", entry.irDescriptor.irConfig);
      emberAfAppPrintln("irCodeLength: 0x%x", entry.irDescriptor.irCodeLength);
      emberAfAppPrint("irCode: ");
      for (i=0; i<entry.irDescriptor.irCodeLength; i++) {
        emberAfAppPrint("0x%x ", finger[i]);
      }
      emberAfAppPrintln("");
    }
  }

  emberAfAppPrintln("%p 0x%x", "get-callback", status);
}



