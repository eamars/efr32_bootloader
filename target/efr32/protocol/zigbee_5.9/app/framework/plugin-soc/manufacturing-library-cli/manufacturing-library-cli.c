// ****************************************************************************
// * manufacturing-library-cli.c
// *
// * Commands for executing manufacturing related tests
// * 
// * Copyright 2016 Silicon Laboratories, Inc.
// ****************************************************************************

#include "app/framework/include/af.h"
#include "app/framework/util/af-main.h"
#include "stack/include/mfglib.h"
#include "app/framework/util/attribute-storage.h"

// -----------------------------------------------------------------------------
// Globals

static uint8_t testPacket[] = { 
  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
  0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
  0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
  0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
  0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
  0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30,
  0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,
  0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40
};

// The max packet size for 802.15.4 is 128, minus 1 byte for the length, and 2 bytes for the CRC.
#define MAX_BUFFER_SIZE 125

// the saved information for the first packet
static uint8_t savedPktLength = 0;
static int8_t savedRssi = 0;
static uint8_t savedLinkQuality = 0;
static uint8_t savedPkt[MAX_BUFFER_SIZE];

static uint16_t mfgCurrentPacketCounter = 0;

static bool inReceivedStream = false;

static bool mfgLibRunning = false;
static bool mfgToneTestRunning = false;
static bool mfgStreamTestRunning = false;

static uint16_t  mfgTotalPacketCounter = 0;


// Add 1 for the length byte which is at the start of the buffer.
ALIGNMENT(2)
static uint8_t   sendBuff[MAX_BUFFER_SIZE + 1];

#define PLUGIN_NAME "Mfglib"

#define MAX_CLI_MESSAGE_SIZE 16

EmberEventControl emberAfPluginManufacturingLibraryCheckSendCompleteEventControl;

static uint16_t savedPacketCount = 0;

#define CHECK_SEND_COMPLETE_DELAY_QS 2

// -----------------------------------------------------------------------------
// Forward Declarations


// -----------------------------------------------------------------------------
// External APIs
// Function to determine whether the manufacturing library functionality is 
// running.  This is used by the network manager and bulb ui plugins to
// determine if it is safe to kick off joining behavoir.
bool emberAfMfglibRunning( void )
{
  return mfgLibRunning;
}

// Some joining behavoir kicks off before the device can receive a CLI command 
// to start the manufacturing library.  Or in the case of devices that use 
// UART for CLI access, they may be asleep.  In this case, we need to set a
// token that gives the manufacturing test a window of opportunity to enable 
// the manufacturin library.  The idea is that fresh devices can more easily
// allow entry into the manufacturing test modes.  It is also intended to
// disable this token via CLI command at the end of manufacturing test so 
// the end customer is not exposed to this functionality.
bool emberAfMfglibEnabled( void )
{
  uint8_t enabled;

#ifndef EMBER_TEST
  halCommonGetToken(&enabled,TOKEN_MFG_LIB_ENABLED);
#else
  return false;
#endif

  emberSerialPrintf(APP_SERIAL, 
                    "MFG_LIB Enabled %x\r\n", enabled); 

  return enabled;
}

// -----------------------------------------------------------------------------

// This is unfortunate but there is no callback indicating when sending is complete
// for all packets.  So we must create a timer that checks whether the packet count
// has increased within the last second.
void emberAfPluginManufacturingLibraryCheckSendCompleteEventHandler(void)
{
  emberEventControlSetInactive(emberAfPluginManufacturingLibraryCheckSendCompleteEventControl);
  if (!inReceivedStream) {
    return;
  }

  if (savedPacketCount == mfgTotalPacketCounter) {
    inReceivedStream = false;
    mfgCurrentPacketCounter = 0;
    emberAfCorePrintln("%p Send Complete %d packets",
                       PLUGIN_NAME,
                       mfgCurrentPacketCounter);
    emberAfCorePrintln("First packet: lqi %d, rssi %d, len %d",
                       savedLinkQuality,
                       savedRssi,
                       savedPktLength);
  } else {
    savedPacketCount = mfgTotalPacketCounter;
    emberEventControlSetDelayQS(emberAfPluginManufacturingLibraryCheckSendCompleteEventControl, 
                                CHECK_SEND_COMPLETE_DELAY_QS);
  }
}

static void fillBuffer(uint8_t* buff, uint8_t length, bool random)
{
  uint8_t i;
  // length byte does not include itself. If the user asks for 10
  // bytes of packet this means 1 byte length, 7 bytes, and 2 bytes CRC
  // this example will have a length byte of 9, but 10 bytes will show
  // up on the receive side
  buff[0] = length;

  if (random) {
    for (i= 1; i < length; i+=2) {
      uint16_t randomNumber = halCommonGetRandom();
      buff[i] = (uint8_t)(randomNumber & 0xFF);
      buff[i+1] = (uint8_t)((randomNumber >> 8)) & 0xFF;
    }
  } else {
    MEMMOVE(buff + 1, testPacket, length);
  }
}

static void mfglibRxHandler(uint8_t *packet, 
                            uint8_t linkQuality, 
                            int8_t rssi)
{
  // This increments the total packets for the whole mfglib session
  // this starts when mfglibStart is called and stops when mfglibEnd
  // is called.
  mfgTotalPacketCounter++;

  mfgCurrentPacketCounter++;

  // If this is the first packet of a transmit group then save the information
  // of the current packet. Don't do this for every packet, just the first one.
  if (!inReceivedStream) {
    inReceivedStream = true;
    mfgCurrentPacketCounter = 1;
    savedRssi = rssi;
    savedLinkQuality = linkQuality;
    savedPktLength = *packet;
    MEMMOVE(savedPkt, (packet+1), savedPktLength); 
  }
}

void emberAfMfglibRxStatistics( uint16_t* packetsReceived,
                                int8_t* savedRssiReturn,
                                uint8_t* savedLqiReturn)
{
  *packetsReceived = mfgTotalPacketCounter;
  *savedRssiReturn = savedRssi;
  *savedLqiReturn = savedLinkQuality;
}

void emberAfMfglibStart( bool wantCallback )
{
  EmberStatus status = mfglibStart(wantCallback ? mfglibRxHandler : NULL);
  emberAfCorePrintln("%p start, status 0x%X",
                     PLUGIN_NAME,
                     status);
  if (status == EMBER_SUCCESS) {
    mfgLibRunning = true;
    mfgTotalPacketCounter = 0;
  }
}

void emAfMfglibStartCommand(void)
{
  bool wantCallback = (bool)emberUnsignedCommandArgument(0);

  emberAfMfglibStart( wantCallback );
}

void emberAfMfglibStop( void )
{
  EmberStatus status = mfglibEnd();
  emberAfCorePrintln("%p end, status 0x%X", 
                     PLUGIN_NAME,
                     status);
  emberAfCorePrintln("rx %d packets while in mfg mode", mfgTotalPacketCounter);
  if (status == EMBER_SUCCESS) {
    mfgLibRunning = false;
  }
}

void emAfMfglibStopCommand(void)
{
  emberAfMfglibStop();
}

void emAfMfglibToneStartCommand(void)
{
  EmberStatus status = mfglibStartTone();
  emberAfCorePrintln("%p start tone 0x%X", PLUGIN_NAME, status);
  if (status == EMBER_SUCCESS) {
    mfgToneTestRunning = true;
  }
}

void emAfMfglibToneStopCommand(void)
{
  EmberStatus status = mfglibStopTone();
  emberAfCorePrintln("%p stop tone 0x%X", PLUGIN_NAME, status);
  if (status == EMBER_SUCCESS) {
    mfgToneTestRunning = false;
  }
}

void emAfMfglibStreamStartCommand(void)
{
  EmberStatus status = mfglibStartStream();
  emberAfCorePrintln("%p start stream 0x%X", PLUGIN_NAME, status);
  if (status == EMBER_SUCCESS) {
    mfgStreamTestRunning = true;
  }
}

void emAfMfglibStreamStopCommand(void)
{
  EmberStatus status = mfglibStopStream();
  emberAfCorePrintln("%p stop stream 0x%X", PLUGIN_NAME, status);
  if (status == EMBER_SUCCESS) {
    mfgStreamTestRunning = false;
  }
}

void emAfMfglibSendCommand(void)
{
  bool random = (emberCommandName()[0] == 'r');
  uint16_t numPackets = (uint16_t)emberUnsignedCommandArgument(0);
  uint8_t length = (uint16_t)emberUnsignedCommandArgument(1);

  if (length > MAX_BUFFER_SIZE) {
    emberAfCorePrintln("Error: Length cannot be bigger than %d", MAX_BUFFER_SIZE);
    return;
  }

  if (numPackets == 0) {
    emberAfCorePrintln("Error: Number of packets cannot be 0.");
    return;
  }

  fillBuffer(sendBuff, length, random);

  // The second parameter to the mfglibSendPacket() is the 
  // number of "repeats", therefore we decrement numPackets by 1.
  numPackets--;
  EmberStatus status = mfglibSendPacket(sendBuff, numPackets);
  emberAfCorePrintln("%p send packet, status 0x%X", PLUGIN_NAME, status);
}

void emAfMfglibSendMessageCommand(void)
{
  emberCopyStringArgument(0, sendBuff, MAX_CLI_MESSAGE_SIZE, false);
  uint16_t numPackets = (uint16_t)emberUnsignedCommandArgument(1);

  if (numPackets == 0) {
    emberAfCorePrintln("Error: Number of packets cannot be 0.");
    return;
  }

  numPackets--;
  EmberStatus status = mfglibSendPacket(sendBuff, numPackets);
  emberAfCorePrintln("%p send message, status 0x%X", PLUGIN_NAME, status);
}

void emAfMfglibStatusCommand(void)
{
  uint8_t channel = mfglibGetChannel();
  int8_t power = mfglibGetPower();
  uint16_t powerMode = emberGetTxPowerMode();
  int8_t synOffset = mfglibGetSynOffset();
  uint8_t options = mfglibGetOptions();
  emberAfCorePrintln("Channel: %d", channel);
  emberAfCorePrintln("Power: %d", power);
  emberAfCorePrintln("Power Mode: 0x%2X", powerMode);
  emberAfCorePrintln("Syn Offset: %d", synOffset);
  emberAfCorePrintln("Options: 0x%X", options);
  emberAfCorePrintln("%p running: %p", PLUGIN_NAME, (mfgLibRunning ? "yes" : "no"));
  emberAfCorePrintln("%p tone test running: %p", PLUGIN_NAME, (mfgToneTestRunning ? "yes" : "no"));
  emberAfCorePrintln("%p stream test running: %p", PLUGIN_NAME, (mfgStreamTestRunning ? "yes": "no"));
  emberAfCorePrintln("Total %p packets received: %d", PLUGIN_NAME, mfgTotalPacketCounter);
}

void emAfMfglibSetChannelCommand(void)
{
  uint8_t channel = (uint8_t)emberUnsignedCommandArgument(0);
  EmberStatus status = mfglibSetChannel(channel);
  emberAfCorePrintln("%p set channel, status 0x%X", PLUGIN_NAME, status);
}

void emAfMfglibSetPowerAndModeCommand(void)
{
  int8_t power = (int8_t)emberSignedCommandArgument(0);
  uint16_t mode = (uint16_t)emberUnsignedCommandArgument(1);
  EmberStatus status = mfglibSetPower(mode, power);
  emberAfCorePrintln("%p set power and mode, status 0x%X", PLUGIN_NAME, status);
}

void emAfMfglibTestModCalCommand(void)
{
#ifndef EMBER_TEST
  uint8_t channel = (uint8_t)emberUnsignedCommandArgument(0);
  uint32_t durationMs = emberUnsignedCommandArgument(1);
  if (durationMs == 0) {
    emberAfCorePrintln("Performing continuous Mod DAC Calibation.  Reset part to stop.");
  } else {
    emberAfCorePrintln("Mod DAC Calibration running for %u ms", durationMs);
  }
  mfglibTestContModCal(channel, durationMs);
#endif
}

void emAfMfglibSynoffsetCommand(void)
{
  int8_t synOffset = (int8_t)emberSignedCommandArgument(0);
  bool toneTestWasRunning = mfgToneTestRunning;
  if (toneTestWasRunning) {
    emAfMfglibToneStopCommand();
  }

  mfglibSetSynOffset(synOffset);

  if (toneTestWasRunning) {
    emAfMfglibToneStartCommand();
  }
}

void emAfMfglibSleepCommand( void ) {
  uint32_t sleepDurationMS = (uint32_t)emberUnsignedCommandArgument(0);

  // turn off the radio
  emberStackPowerDown();

  ATOMIC(
    // turn off board and peripherals
    halPowerDown();
    // turn micro to power save mode - wakes on external interrupt
    // or when the time specified expires
    halSleepForMilliseconds(&sleepDurationMS);
    // power up board and peripherals
    halPowerUp();
  );
  // power up radio
  emberStackPowerUp();

  emberAfEepromNoteInitializedStateCallback(false);

  // Allow the stack to time out any of its events and check on its
  // own network state.
  emberTick();
}

// Function to program a custom EUI64 into the chip.  
// Example:
// plugin mfglib programEui { 01 02 03 04 05 06 07 08 }
// Note:  this command is OTP.  It only works once.  To re-run, you
// must erase the chip.  
void emAfMfglibProgramEuiCommand( void )
{
  EmberEUI64 eui64;

  emberAfCopyBigEndianEui64Argument(0, eui64);
  
  // potentially verify first few bytes for customer OUI

#ifndef EMBER_TEST
  // OK, we verified the customer OUI.  Let's program it here.
  halInternalSetMfgTokenData(TOKEN_MFG_CUSTOM_EUI_64,(uint8_t *) &eui64, EUI64_SIZE);
#endif
}

void emAfMfglibEnableMfglib( void )
{
#ifndef EMBER_TEST
  uint8_t enabled = (uint8_t) emberSignedCommandArgument(0);

  halCommonSetToken( TOKEN_MFG_LIB_ENABLED, &enabled );
#endif
}

void emAfMfglibPrintcalData( void )
{
#if (!defined(EMBER_TEST) && (defined(PHY_EM250) || defined(PHY_EM3XX)))
  tokTypeStackCalData calData = {0,};
  tokTypeStackCalData *calDataToken = &calData;
  uint16_t i;

  for(i=0; i<16; i++) {
    halStackGetIdxTokenPtrOrData(calDataToken,
                                 TOKEN_STACK_CAL_DATA,
                                 i);
    emberSerialPrintf(APP_SERIAL, "Cal Data: %x %x %x %x\r\n",
                      calData.tempAtLna,
                      calData.modDac,
                      calData.tempAtModDac,
                      calData.lna);
  }
#endif
}

void emAfMfglibSetOptions(void)
{
  uint8_t options = (uint8_t)emberUnsignedCommandArgument(0);
  EmberStatus status = mfglibSetOptions(options);
  emberAfCorePrintln("%p set options, status 0x%X", PLUGIN_NAME, status);
}

