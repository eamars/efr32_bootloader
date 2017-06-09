// Copyright 2014 Silicon Laboratories, Inc.

#include "af.h"
#include "app/framework/plugin/rf4ce-mso/rf4ce-mso.h"
#include "rf4ce-mso-ir-rf-database-originator.h"

#ifdef EMBER_AF_LEGACY_CLI
  #error The RF4CE MSO IR-RF Database Originator plugin is not compatible with the legacy CLI.
#endif

#define RF_PAYLOAD_MAX 32
#define IR_CODE_MAX 32
static EmberAfRf4ceMsoIrRfDatabaseEntry entry;
static uint8_t rfPressedPayload[RF_PAYLOAD_MAX];
static uint8_t rfRepeatedPayload[RF_PAYLOAD_MAX];
static uint8_t rfReleasedPayload[RF_PAYLOAD_MAX];
static uint8_t irCode[IR_CODE_MAX];

static void setRfDescriptorCommand(EmberAfRf4ceMsoIrRfDatabaseRfDescriptor *rfDescriptor);

// plugin rf4ce-mso-ir-rf-database-originator set-flags <flags:1>
void emberAfRf4ceMsoIrRfDatabaseOriginatorSetFlagsCommand(void)
{
  entry.flags
    = (EmberAfRf4ceMsoIrRfDatabaseFlags)emberUnsignedCommandArgument(0);
}

// plugin rf4ce-mso-ir-rf-database-originator set-pressed <rf config:1> <tx options:1> <payload:n>
void emberAfRf4ceMsoIrRfDatabaseOriginatorSetPressedCommand(void)
{
  entry.rfPressedDescriptor.payload = rfPressedPayload;
  setRfDescriptorCommand(&entry.rfPressedDescriptor);
}

// plugin rf4ce-mso-ir-rf-database-originator set-repeated <rf config:1> <tx options:1> <payload:n>
void emberAfRf4ceMsoIrRfDatabaseOriginatorSetRepeatedCommand(void)
{
  entry.rfRepeatedDescriptor.payload = rfRepeatedPayload;
  setRfDescriptorCommand(&entry.rfRepeatedDescriptor);
}

// plugin rf4ce-mso-ir-rf-database-originator set-released <rf config:1> <tx options:1> <payload:n>
void emberAfRf4ceMsoIrRfDatabaseOriginatorSetReleasedCommand(void)
{
  entry.rfReleasedDescriptor.payload = rfReleasedPayload;
  setRfDescriptorCommand(&entry.rfReleasedDescriptor);
}

// plugin rf4ce-mso-ir-rf-database-originator set-ir <ir config:1> <ir code:n>
void emberAfRf4ceMsoIrRfDatabaseOriginatorSetIrCommand(void)
{
  entry.irDescriptor.irCode = irCode;
  entry.irDescriptor.irConfig
    = (EmberAfRf4ceMsoIrRfDatabaseIrConfig)emberUnsignedCommandArgument(0);
  entry.irDescriptor.irCode
    = emberStringCommandArgument(1, &entry.irDescriptor.irCodeLength);
  entry.irDescriptor.irCodeLength
    = emberCopyStringArgument(1,
                              (uint8_t *)entry.irDescriptor.irCode,
                              IR_CODE_MAX,
                              false); // no padding
}

// plugin rf4ce-mso-ir-rf-database-originator set <key code:1>
void emberAfRf4ceMsoIrRfDatabaseOriginatorSetCommand(void)
{
  EmberAfRf4ceMsoKeyCode keyCode
    = (EmberAfRf4ceMsoKeyCode)emberUnsignedCommandArgument(0);
  EmberStatus status = emberAfRf4ceMsoIrRfDatabaseOriginatorSet(keyCode,
                                                                &entry);
  emberAfAppPrintln("%p 0x%x", "set", status);
}

// plugin rf4ce-mso-ir-rf-database-originator clear <key code:1>
void emberAfRf4ceMsoIrRfDatabaseOriginatorClearCommand(void)
{
  EmberAfRf4ceMsoKeyCode keyCode
    = (EmberAfRf4ceMsoKeyCode)emberUnsignedCommandArgument(0);
  EmberStatus status = emberAfRf4ceMsoIrRfDatabaseOriginatorClear(keyCode);
  emberAfAppPrintln("%p 0x%x", "clear", status);
}

// plugin rf4ce-mso-ir-rf-database-originator clear-all
void emberAfRf4ceMsoIrRfDatabaseOriginatorClearAllCommand(void)
{
  emberAfRf4ceMsoIrRfDatabaseOriginatorClearAll();
}

static void setRfDescriptorCommand(EmberAfRf4ceMsoIrRfDatabaseRfDescriptor *rfDescriptor)
{
  rfDescriptor->rfConfig
    = (EmberAfRf4ceMsoIrRfDatabaseRfConfig)emberUnsignedCommandArgument(0);
  rfDescriptor->txOptions
    = (EmberRf4ceTxOption)emberUnsignedCommandArgument(1);
  rfDescriptor->payloadLength
    = emberCopyStringArgument(2,
                              (uint8_t *)rfDescriptor->payload,
                              RF_PAYLOAD_MAX,
                              false); // no padding
}
