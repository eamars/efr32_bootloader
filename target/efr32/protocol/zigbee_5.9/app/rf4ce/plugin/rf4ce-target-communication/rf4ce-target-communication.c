// Copyright 2014 Silicon Laboratories, Inc.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------

#include PLATFORM_HEADER
#include "stack/include/ember.h"
#include "hal/hal.h"

#ifdef EMBER_AF_API_DEBUG_PRINT
  #include EMBER_AF_API_DEBUG_PRINT
#else
#define emberAfAppPrint(...)
#define emberAfAppPrintln(...)
#endif

#include "app/framework/plugin/rf4ce-profile/rf4ce-profile.h"
#include "app/framework/plugin/rf4ce-gdp/rf4ce-gdp.h"
#include "app/framework/plugin/rf4ce-zrc20/rf4ce-zrc20.h"
#include "app/framework/plugin/rf4ce-gdp-identification-server/rf4ce-gdp-identification-server.h"
#include "app/framework/plugin/rf4ce-zrc20-action-mapping-server/rf4ce-zrc20-action-mapping-server.h"
#include "app/rf4ce/plugin/rf4ce-target-communication/rf4ce-target-communication.h"
#include "plugin/serial/serial.h"
#include "app/framework/plugin/rf4ce-zrc20/rf4ce-zrc20-internal.h"
#include "hal/micro/cortexm3/usb/em_usb.h"

//-----------------------------------------------------------------------------
// Macros
//-----------------------------------------------------------------------------

#define MAX_SIZE_PAYLOAD        (128)

#define APP_SIZE_VERSION        (1)
#define APP_SIZE_MESSAGEID      (1)
#define APP_SIZE_LENGTH         (1)
#define APP_SIZE_PAYLOAD        (MAX_SIZE_PAYLOAD)
#define APP_SIZE_BUFFER ( \
  APP_SIZE_VERSION + \
  APP_SIZE_MESSAGEID + \
  APP_SIZE_LENGTH +\
  APP_SIZE_PAYLOAD)

#define PRE_SIZE_START          (1)
#define PRE_SIZE_PAYLOAD        (APP_SIZE_BUFFER * 2)
#define PRE_SIZE_CHECKSUM       (1)
#define PRE_SIZE_END            (1)
#define PRE_SIZE_BUFFER ( \
  PRE_SIZE_START + \
  PRE_SIZE_PAYLOAD + \
  PRE_SIZE_CHECKSUM +\
  PRE_SIZE_END)

#define PRE_START               (0xC0)
#define PRE_END                 (0xC1)
#define PRE_ESC_PREFIX          (0x7E)
#define PRE_ESC_MODIFIER        (0x20)

#define APP_VERSION             (0)

#define STATUS_VERSION_MAJOR    (0)
#define STATUS_VERSION_MINOR    (0)

#define MESSAGE_ID_STATUS_REQ                   (0)
#define MESSAGE_ID_STATUS_ACK                   (1)
#define MESSAGE_ID_ACTION_REQ                   (10)
#define MESSAGE_ID_ACTION_MAPPING_REQ           (14)
#define MESSAGE_ID_ACTION_MAPPING_ACK           (15)
#define MESSAGE_ID_AUDIO_REQ                    (20)
#define MESSAGE_ID_HEARTBEAT_REQ                (30)
#define MESSAGE_ID_IDENTIFY_REQ                 (40)
#define MESSAGE_ID_IDENTIFY_ACK                 (41)
#define MESSAGE_ID_BIND_INFO_REQ                (50)
#define MESSAGE_ID_BIND_REQ_ACK                 (53)

#define MESSAGE_STATUS_OK                       (0)
#define MESSAGE_STATUS_CHECKCONDITION           (1)
#define MESSAGE_STATUS_BUSY                     (2)

#define MESSAGE_COND_OK                         (0)
#define MESSAGE_COND_DEVICE_HAS_BEEN_RESET      (40)
#define MESSAGE_COND_NEW_ACTION_MAPPINGS        (41)
#define MESSAGE_COND_BEEP_CONTROLLER            (42)

#define REMAP_INDEX_DEFAULT                     (0xff)
#define REMAP_INDEX_LAST                        (0x7f)

#define MAX_PAIRINGS                            (5)

#define AUDIO_FRAME_ACTION_TYPE         (0)
#define AUDIO_FRAME_ACTION_MODIFIER     (0)
#define AUDIO_FRAME_ACTION_VENDOR       EMBER_RF4CE_NULL_VENDOR_ID
#define EMBER_AF_RF4CE_ZRC_ACTION_BANK_HID_KEYBOARD_PAGE_SECTION_A_F23 (0x72)
#define EMBER_AF_RF4CE_ZRC_ACTION_BANK_HID_KEYBOARD_PAGE_SECTION_A_F24 (0x73)

#define DEFAULT_IDENTIFY_FLAGS \
    (EMBER_AF_RF4CE_GDP_CLIENT_NOTIFICATION_IDENTIFY_FLAG_FLASH_LIGHT \
     + EMBER_AF_RF4CE_GDP_CLIENT_NOTIFICATION_IDENTIFY_FLAG_MAKE_SOUND)
#define DEFAULT_IDENTIFY_TIME                   (10)

#define FREQUENCY_CONVERT_TO_PERIODES(in, out) ((in < out) ? 1 : (in / out))
#define FREQUENCY_STATUS_POLLING                (1)
#define FREQUENCY_AUDIO_END_TEST                (5)

//-----------------------------------------------------------------------------
// Typedefs
//-----------------------------------------------------------------------------

typedef struct
{
  bool  bFlgIdentify;
  bool  bFlgNotify;
} controllerInfoFlags_t;

typedef struct
{
  uint8_t  uType;
  uint8_t  uModi;
  uint8_t  uBank;
  uint8_t  uCode;
  uint16_t wVend;
} actionReq_t;

typedef struct
{
  uint8_t  pairingIndex;
  uint8_t  triggers;
} heartbeatReq_t;


typedef struct
{
  uint8_t  uMaj;
  uint8_t  uMin;
  uint8_t  uStatus;
  uint8_t  uCondit;
} statusReq_t;

//-----------------------------------------------------------------------------
// Local Variables
//-----------------------------------------------------------------------------

static unsigned char appTxBufDat[APP_SIZE_BUFFER];
static unsigned char appRxBufDat[APP_SIZE_BUFFER];

static unsigned char preTxBufDat[PRE_SIZE_BUFFER];
static unsigned char preRxBufDat[PRE_SIZE_BUFFER];
static uint16_t        preRxBufLen;

static uint8_t hostPort;

static uint8_t remapReqDevType     = EMBER_AF_RF4CE_DEVICE_TYPE_TELEVISION;
static uint8_t remapReqBank        = EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC;
static uint8_t remapReqActionIdx   = REMAP_INDEX_DEFAULT;

static bool initFlag;
static bool audioInProgressFlag;
static uint8_t   audioInProgressStopCount;
static controllerInfoFlags_t controllerInfoFlags[MAX_PAIRINGS];

static bool identifyFlg;
static uint8_t   identifyTim;

//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------

EmberEventControl emberAfPluginRf4ceTargetCommunicationEventControl;

//-----------------------------------------------------------------------------
// Local Functions
//-----------------------------------------------------------------------------

static void vPrintBuffer(uint8_t *pBuf, uint16_t wLen)
{
  uint16_t i;

  for (i = 0; i < wLen; i++) {
    emberAfAppPrint("0x%2x, ", pBuf[i]);
  }
  emberAfAppPrintln("");
}

//************************************
// Presentation package handling

// This function is used to encode a single byte.
// If the byte is one of the three predefined, it has to be prefixed
// with a special character and modified.
static void preEncodeByte(uint16_t *pLen, uint8_t uVal)
{
  if ((uVal == PRE_START) || (uVal == PRE_END) || (uVal == PRE_ESC_PREFIX)) {
    preTxBufDat[*pLen] = PRE_ESC_PREFIX;
    *pLen += 1;
    preTxBufDat[*pLen] = uVal ^ PRE_ESC_MODIFIER;
    *pLen += 1;
  } else {
    preTxBufDat[*pLen] = uVal;
    *pLen += 1;
  }
}

// This function is used to encode data at the presentation layer.
// Encoded data is placed in the preTxBufDat buffer,
// and the actual length is returned.
static uint16_t preEncode(uint8_t uLen)
{
  uint16_t preTxBufLen;
  uint8_t sum;
  uint8_t i;

  preTxBufLen = 0;
  sum = 0;
  preTxBufDat[preTxBufLen++] = PRE_START;
  for (i = 0; i < uLen; i++) {
    sum ^= appTxBufDat[i];
    preEncodeByte(&preTxBufLen, appTxBufDat[i]);
  }
  preEncodeByte(&preTxBufLen, sum);
  preTxBufDat[preTxBufLen++] = PRE_END;

  return preTxBufLen;
}

// This function is used decode data at the presentation layer.
// Data is assumed to be in the appRxBufDat buffer.
// Decoded data is available via pointers.
// Returns the number of valid bytes that has been decoded.
static uint8_t preDecode(void)
{
  uint8_t appRxBufLen = 0;
  uint8_t sum;

  sum = 0;
  if ((preRxBufLen > 3)
      && (preRxBufDat[0] == PRE_START)
      && (preRxBufDat[preRxBufLen - 1] == PRE_END)) {
    int i;
    uint8_t dat;

    i = 1;
    while (i < (preRxBufLen - 1)) {
      if (preRxBufDat[i] == PRE_ESC_PREFIX) {
        i++;
        dat = preRxBufDat[i++] ^ PRE_ESC_MODIFIER;
      } else {
        dat = preRxBufDat[i++];
      }
      sum ^= dat;
      if (appRxBufLen < APP_SIZE_BUFFER) {
        appRxBufDat[appRxBufLen++] = dat;
      }
    }
    if (sum == 0) {
      appRxBufLen--;
    } else {
      appRxBufLen = 0;
    }
  }

  return appRxBufLen;
}


//************************************
// Application package handling


// This function is used to encode data at the application layer.
// Encoded data is placed in the appTxBufDat buffer,
// and the actual length is returned.
static uint8_t appEncode(uint8_t messageId, const uint8_t *pBuf, uint8_t uLen)
{
  uint8_t appTxBufLen;

  appTxBufLen = 0;
  if (uLen <= MAX_SIZE_PAYLOAD) {
    appTxBufDat[appTxBufLen++] = APP_VERSION;
    appTxBufDat[appTxBufLen++] = messageId;
    appTxBufDat[appTxBufLen++] = uLen;
    if ((pBuf != 0) && (uLen)) {
      MEMCOPY((void *)&appTxBufDat[appTxBufLen], pBuf, uLen);
      appTxBufLen += uLen;
    }
  }

  return appTxBufLen;
}

// This function is used decode data at the application layer.
// Data is assumed to be in the appRxBufDat buffer.
// Decoded data is available via pointers.
static void appDecode(uint8_t appRxBufLen, uint8_t *pMid, uint8_t **pBuf, uint8_t *pLen)
{
  if ((appRxBufLen == 2) && (appRxBufDat[0] == APP_VERSION)) {
    *pMid = appRxBufDat[1];
    *pBuf = 0;
    *pLen = 0;
  } else if ((appRxBufLen > 2) && (appRxBufDat[0] == APP_VERSION)) {
    *pMid = appRxBufDat[1];
    *pBuf = &appRxBufDat[3];
    *pLen = appRxBufLen - 3;
  } else {
    *pMid = 0;
    *pBuf = 0;
    *pLen = 0;
  }
}

//************************************
// User data handling

// This function handles byte by byte received from the host computer.
// It looks for the special byte at the beginning and end of a package,
// and calls the decoder at the presentation layer if found.
static uint8_t hostRxOneByte(uint8_t dat)
{
  uint8_t appRxBufLen = 0;

  if (preRxBufLen >= PRE_SIZE_BUFFER) {
    preRxBufLen = 0;
  }
  if (dat == PRE_START) {
    preRxBufLen = 0;
  }
  preRxBufDat[preRxBufLen++] = dat;
  if (dat == PRE_END) {
    appRxBufLen = preDecode();
    preRxBufLen = 0;
  }

  return appRxBufLen;
}

// This function receives byte from the host computer and
// decodes it when a valid package has been received.
// Both the preRxBufDat and appRxBufDat buffers are used.
// Returns the total number of valid bytes in the package.
static uint8_t hostRx(uint8_t port, uint8_t *pMid, uint8_t **pBuf, uint8_t *pLen)
{
  uint8_t appRxBufLen = 0;

  while (emberSerialReadAvailable(port) > 0) {
    uint8_t dat;

    emberSerialReadByte(port, &dat);
    appRxBufLen = hostRxOneByte(dat);
    if (appRxBufLen) {
      appDecode(appRxBufLen, pMid, pBuf, pLen);
      break;
    }
  }

  return appRxBufLen;
}

// This function encodes data at both the application and presentation layer
// and transmits the data to the host computer.
// Both the preTxBufDat and appTxBufDat buffers are used.
static void hostTx(uint8_t port, uint8_t messageId, const uint8_t *pBuf, uint8_t uLen)
{
  uint8_t  appTxBufLen;
  uint16_t preTxBufLen;
  bool rdy = true;

  appTxBufLen = appEncode(messageId, pBuf, uLen);
  preTxBufLen = preEncode(appTxBufLen);

#if EM_SERIAL3_ENABLED
  // Check that out USB device is actually configured and  that there is room in
  // the TX FIFO. Otherwise emberSerialWriteData will fill up the FIFO and
  // block until there is room again in the FIFO. If the device is not connected
  // the FIFO will not get emptied and the function will block indefinitely.
  if (port==3) {
    rdy = ((USBD_GetUsbState() == USBD_STATE_CONFIGURED)
           && (emberSerialWriteAvailable(port) >= preTxBufLen));
  }
#endif

  while (preTxBufLen > 0) {
    uint8_t txLen;
    txLen = (preTxBufLen > 255) ? 255 : preTxBufLen;
    if (rdy) {
      emberSerialWriteData(port, preTxBufDat, txLen);
    }
    preTxBufLen -= txLen;
  }
}

//************************************

// This function handles the start of an audio sequence.
// It must be called before every audio package.
// If a new sequence is in progress, an action code will be sent.
static void hostAudioBegin(uint8_t port)
{
  if (!audioInProgressFlag) {
    emberAfAppPrintln("Audio Begin");
    audioInProgressFlag = 1;
    emberAfTargetCommunicationHostActionTx(port,
      AUDIO_FRAME_ACTION_TYPE,
      AUDIO_FRAME_ACTION_MODIFIER,
      EMBER_AF_RF4CE_ZRC_ACTION_BANK_HID_KEYBOARD_PAGE_SECTION_A,
      EMBER_AF_RF4CE_ZRC_ACTION_BANK_HID_KEYBOARD_PAGE_SECTION_A_F23,
      AUDIO_FRAME_ACTION_VENDOR);
  }
  audioInProgressStopCount = 0;
}

// This function handles the end of an audio sequence.
// It must be called repeatedly.
// If no audio data has been present for some time,
// it will terminate the sequence with an action code.
static void hostAudioEnd(uint8_t port, int frequency)
{
  if (audioInProgressFlag) {
    int periodes = FREQUENCY_CONVERT_TO_PERIODES(frequency,
                                                 FREQUENCY_AUDIO_END_TEST);
    if (audioInProgressStopCount < periodes) {
      audioInProgressStopCount++;
      return;
    }
  }

  if (audioInProgressFlag) {
    emberAfAppPrintln("Audio End");
    audioInProgressFlag = 0;
    audioInProgressStopCount = 0;
    emberAfTargetCommunicationHostActionTx(port,
      AUDIO_FRAME_ACTION_TYPE,
      AUDIO_FRAME_ACTION_MODIFIER,
      EMBER_AF_RF4CE_ZRC_ACTION_BANK_HID_KEYBOARD_PAGE_SECTION_A,
      EMBER_AF_RF4CE_ZRC_ACTION_BANK_HID_KEYBOARD_PAGE_SECTION_A_F24,
      AUDIO_FRAME_ACTION_VENDOR);
  }
}

//************************************

// This function will send information about the heartbeat to the host.
static void hostHeartbeatTx(uint8_t port, uint8_t pairingIndex, uint8_t triggers)
{
  heartbeatReq_t     heartbeat;

  heartbeat.pairingIndex = pairingIndex;
  heartbeat.triggers = triggers;
  hostTx(hostPort,
         MESSAGE_ID_HEARTBEAT_REQ,
         (unsigned char *)&heartbeat,
         sizeof(heartbeat));
}

// The heartbeat callback, called by the rf4ce stack for every heartbeat.
static void heartbeatCallback(uint8_t pairingIndex, uint8_t triggers)
{
  emberAfAppPrintln("Heartbeat: PairIdx=%d, Triggers=0x%x",
                    pairingIndex,
                    triggers);
  if ((pairingIndex < MAX_PAIRINGS)
      && (controllerInfoFlags[pairingIndex].bFlgIdentify)) {
    controllerInfoFlags[pairingIndex].bFlgIdentify = 0;
    emberAfTargetCommunicationControllerIdentify(pairingIndex,
                                                         identifyFlg,
                                                         identifyTim);
  }
  if ((pairingIndex < MAX_PAIRINGS)
      && (controllerInfoFlags[pairingIndex].bFlgNotify)) {
    controllerInfoFlags[pairingIndex].bFlgNotify = 0;
    emberAfTargetCommunicationControllerNotify(pairingIndex);
  }
  hostHeartbeatTx(hostPort, pairingIndex, triggers);
}

//************************************

// This function will be called when a package with information about
// the controller identification has been received.
// It will store the received data and set the identification flags.
// The actual identification will be done at the next heartbeat.
static void hostIdentifyRx(unsigned char *pBuf, unsigned char uLen)
{
  int i;

  // Set default values
  identifyFlg = DEFAULT_IDENTIFY_FLAGS;
  identifyTim = DEFAULT_IDENTIFY_TIME;
  // Set values from the incoming data if present.
  if ((pBuf != 0) && (uLen == 2)) {
    identifyFlg = pBuf[0];
    identifyTim = pBuf[1];
  }
  emberAfAppPrintln("Identify Ack: Flags=0x%x, Time=%d",
                     identifyFlg, identifyTim);
  for (i = 0; i < MAX_PAIRINGS; i++) {
    controllerInfoFlags[i].bFlgIdentify = 1;
  }
}

// This function will send a request for identification data to the host.
static void hostIdentifyTx(uint8_t port)
{
  hostTx(port, MESSAGE_ID_IDENTIFY_REQ, 0, 0);
}

//************************************

// This function will send a request for a remap action to the host computer.
static void hostRemapActionTx(uint8_t port,
                              uint8_t deviceType, uint8_t bank, uint8_t action)
{
  unsigned char xBuf[3];

  xBuf[0] = deviceType;
  xBuf[1] = bank;
  xBuf[2] = action;
  hostTx(port, MESSAGE_ID_ACTION_MAPPING_REQ, xBuf, sizeof(xBuf));
}

// This function will initiate a sequence of remap action requests.
// First we restore default actions so the old mappings are not taking
// up space.
static void hostRemapActionBegin(uint8_t port)
{
  emberAfRf4ceZrc20ActionMappingServerRestoreDefaultAllActions();
  emberAfAppPrintln("--> RemapAction: Begin.");
  remapReqActionIdx = 0;
  remapReqDevType = EMBER_AF_RF4CE_DEVICE_TYPE_TELEVISION;
  hostRemapActionTx(port, remapReqDevType, remapReqBank, remapReqActionIdx);
}

// This function will terminate a sequence of remap action requests.
// When the last action mapping has been received, the notify flags will be set.
// Information that new action mappings are available is sent to the controller
// at next heartbeat.
static void hostRemapActionEnd(uint8_t port)
{
  int i;

  if (remapReqDevType == EMBER_AF_RF4CE_DEVICE_TYPE_TELEVISION) {
    emberAfAppPrintln("--> RemapAction: Next device.");
    remapReqActionIdx = 0;
    remapReqDevType = EMBER_AF_RF4CE_DEVICE_TYPE_SET_TOP_BOX;
    hostRemapActionTx(port, remapReqDevType, remapReqBank, remapReqActionIdx);
  } else {
    emberAfAppPrintln("--> RemapAction: End.");
    for (i = 0; i < MAX_PAIRINGS; i++) {
      controllerInfoFlags[i].bFlgNotify = 1;
    }
  }
}

// This function handles action mappings received from the host computer.
// The package received can contain either no action mappings, only rf,
// only ir or both rf and ir.
// It will continue to request new action mappings until all has been
// received for a given device type and bank.
static void hostRemapActionRx(uint8_t port, uint8_t *pBuf, uint8_t uLen)
{
  uint8_t deviceType;
  uint8_t bank;
  uint8_t action;
  uint8_t mappingFlags;
  uint8_t rfLen;
  uint8_t *rfDat;
  uint8_t irLen;
  uint8_t *irDat;

  if (remapReqActionIdx < REMAP_INDEX_LAST) {
    remapReqActionIdx++;
    if (uLen >= 6) {
      deviceType = pBuf[0];
      bank = pBuf[1];
      action = pBuf[2];
      mappingFlags = pBuf[3];
      rfLen = pBuf[4];
      irLen = pBuf[5];
      rfDat = (rfLen > 0) ? &pBuf[6] : 0;
      irDat = (irLen > 0) ? &pBuf[6 + rfLen] : 0;
      if ((6 + rfLen + irLen) == uLen) {
        emberAfTargetCommunicationControllerRemapAction(deviceType,
                                                        bank,
                                                        action,
                                                        mappingFlags,
                                                        rfDat,
                                                        rfLen,
                                                        irDat,
                                                        irLen);
      } else {
        emberAfAppPrintln(
          "RemapAction: Type=%d, Bank=%d, Action=%d, uLen=%d, rfLen=%d, irLen=%d",
          deviceType, bank, action, uLen, rfLen, irLen);
        emberAfAppPrintln("=> ERROR - expected %d, got %d bytes.",
                          6 + rfLen + irLen,
                          uLen);
      }
    } else {
      emberAfAppPrintln("--> RemapAction: Error in data.");
    }
    hostRemapActionTx(port, remapReqDevType, remapReqBank, remapReqActionIdx);
  } else {
    hostRemapActionEnd(port);
  }
}

//************************************

// This function should be called whenever a status package has been received.
// It decodes the status and condition to see if there is a request for
// doing action mapping or controller identify.
static void hostStatusRx(uint8_t port, unsigned char *pBuf, unsigned char uLen)
{
  if ((uLen == 4)
      && (pBuf[0] == STATUS_VERSION_MAJOR)
      && (pBuf[1] == STATUS_VERSION_MINOR)) {
    unsigned char uSta = pBuf[2];
    unsigned char uCon = pBuf[3];

    //emberAfAppPrintln("Host Rx: Status ack");
    initFlag = 0;
    if (uSta == MESSAGE_STATUS_CHECKCONDITION) {
      switch (uCon) {
      case MESSAGE_COND_NEW_ACTION_MAPPINGS:
        emberAfAppPrintln("    ==> New mappings are available.");
        hostRemapActionBegin(port);
        break;

      case MESSAGE_COND_BEEP_CONTROLLER:
        emberAfAppPrintln("    ==> Identify controller.");
        hostIdentifyTx(port);
        break;
      }
    }
  }
}

// This function will send a status request to the host computer.
// It will modify the status package to contain information about
// the target has been reset.
static void hostStatusTx(uint8_t port)
{
  statusReq_t xReq;
  uint8_t       len;

  //emberAfAppPrintln("Host Tx: Status req");
  xReq.uMaj = STATUS_VERSION_MAJOR;
  xReq.uMin = STATUS_VERSION_MINOR;
  xReq.uStatus = initFlag ? MESSAGE_STATUS_CHECKCONDITION : MESSAGE_STATUS_OK;
  xReq.uCondit = initFlag ? MESSAGE_COND_DEVICE_HAS_BEEN_RESET : MESSAGE_COND_OK;
  len = initFlag ? sizeof(xReq) : 2;
  hostTx(port, MESSAGE_ID_STATUS_REQ, (unsigned char *)&xReq, len);
}

//************************************

// During action mapping transfers, it is a benefit to increase the polling
// rate to get higher throughput.
// This simple logic tests if action mapping transfers are in progress.
static bool communicationRequestFastPolling(void)
{
  bool bSta;

  bSta = (remapReqActionIdx < REMAP_INDEX_LAST) ? 1 : 0;

  return bSta;
}

//************************************

// This is the communication state machine.
// It keeps track of audio transfers from the controller, data packages from
// the host computer and periodically requests status from the host computer.
static void communicationStateMachine(uint8_t port, int frequency)
{
  static int statusTick;
  uint8_t appRxBufLen;
  uint8_t messageId;
  uint8_t *pBuf;
  uint8_t uLen;

  if (audioInProgressFlag) {
    hostAudioEnd(port, frequency);
  }
  else {
    // Anything in?
    appRxBufLen = hostRx(port, &messageId, &pBuf, &uLen);
    if (appRxBufLen) {
      switch (messageId) {

      case MESSAGE_ID_STATUS_ACK:
        hostStatusRx(port, pBuf, uLen);
        break;

      case MESSAGE_ID_ACTION_MAPPING_ACK:
        hostRemapActionRx(port, pBuf, uLen);
        break;

      case MESSAGE_ID_IDENTIFY_ACK:
        hostIdentifyRx(pBuf, uLen);
        break;

      case MESSAGE_ID_BIND_REQ_ACK:
        emberAfAppPrintln("Host Rx: Bind req.");
        emberAfPluginRf4ceTargetCommunicationHostBindingRequestCallback();
        break;
      }
    }

    // Anything out?
    if (communicationRequestFastPolling()) {
      //nop
    } else {
      statusTick++;
      // Send status at 1 Hz
      int periodes = FREQUENCY_CONVERT_TO_PERIODES(frequency,
                                                   FREQUENCY_STATUS_POLLING);
      if (statusTick >= periodes) {
        hostStatusTx(port);
        statusTick = 0;
      }
    }
  }
}

//************************************

//-----------------------------------------------------------------------------
// Global API Functions
//-----------------------------------------------------------------------------

// The event handler is used to call the state machine to maintain the
// serial communication.
void emberAfPluginRf4ceTargetCommunicationEventHandler(void)
{
  communicationStateMachine(hostPort, 10);
  if (communicationRequestFastPolling())
  {
    emberEventControlSetActive(emberAfPluginRf4ceTargetCommunicationEventControl);
  } else {
    emberEventControlSetDelayMS(emberAfPluginRf4ceTargetCommunicationEventControl,
                                1 * MILLISECOND_TICKS_PER_DECISECOND);
  }
}

// The initialization function.
void emberAfTargetCommunicationInit(uint8_t uPort)
{
  int i;

  hostPort = uPort;
  initFlag = 1;
  audioInProgressFlag = 0;
  audioInProgressStopCount = 0;
  for (i = 0; i < MAX_PAIRINGS; i++) {
    controllerInfoFlags[i].bFlgIdentify = 0;
    controllerInfoFlags[i].bFlgNotify = 0;
  }
  emberAfRf4ceGdpSubscribeToHeartbeat(heartbeatCallback);
  // Are both of these required?
  emAfRf4ceZrc20ActionMappingServerClearAllMappableActions();
  emberAfRf4ceZrc20ActionMappingServerRestoreDefaultAllActions();

  emberEventControlSetDelayMS(emberAfPluginRf4ceTargetCommunicationEventControl,
                              1 * MILLISECOND_TICKS_PER_DECISECOND);
}

//************************************

// Send an action to the host computer.
void emberAfTargetCommunicationHostActionTx(uint8_t port,
                                            uint8_t type,
                                            uint8_t modifier,
                                            uint8_t bank,
                                            uint8_t code,
                                            uint16_t vendor)
{
  actionReq_t        xReq;

  emberAfAppPrintln(
    "Action: Type=0x%x, Modifier=0x%x, Bank=0x%x, Code=0x%x, Vendor=0x%2x",
    type, modifier, bank, code, vendor);

  xReq.uType = type;
  xReq.uModi = modifier;
  xReq.uBank = bank;
  xReq.uCode = code;
  xReq.wVend = vendor;
  hostTx(port, MESSAGE_ID_ACTION_REQ, (unsigned char *)&xReq, sizeof(xReq));
}

//************************************

// Send audio data to the host computer.
void emberAfTargetCommunicationHostAudioTx(uint8_t port,
                                           const uint8_t *pBuf,
                                           uint8_t uLen)
{
  hostAudioBegin(port);
  hostTx(port, MESSAGE_ID_AUDIO_REQ, pBuf, uLen);
}

//************************************

// Send information about the binding to the host computer.
void emberAfTargetCommunicationHostBindInfoTx(uint8_t port,
                                              uint8_t info)
{
  emberAfAppPrintln( "Host Tx: Bind info=%d", info);
  hostTx(port, MESSAGE_ID_BIND_INFO_REQ, &info, 1);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// rf4ce functions

// Send an identification message to the controller.
void emberAfTargetCommunicationControllerIdentify(uint8_t pairingIndex,
                                                  uint8_t flags,
                                                  uint8_t seconds)
{
  EmberStatus status;

  status = emberAfRf4ceGdpIdentificationServerIdentify(pairingIndex,
                                                       flags, seconds);
  emberAfAppPrintln("ServerIdentify: pairingIndex=%d, status=0x%x",
                    pairingIndex, status);
}

//************************************

// Send a notification message to the controller
void emberAfTargetCommunicationControllerNotify(uint8_t pairingIndex)
{
  EmberStatus status;

  status = emberAfRf4ceGdpClientNotification(
        pairingIndex,
        EMBER_AF_RF4CE_PROFILE_REMOTE_CONTROL_2_0,
        EMBER_RF4CE_NULL_VENDOR_ID,
        CLIENT_NOTIFICATION_SUBTYPE_REQUEST_ACTION_MAPPING_NEGOTIATION,
        NULL,
        CLIENT_NOTIFICATION_REQUEST_ACTION_MAPPING_NEGOTIATION_PAYLOAD_LENGTH);
  emberAfAppPrintln("ClientNotify(pairingIndex=%d) - status=0x%x",
                    pairingIndex, status);
}

//************************************

// Set an action mapping in the mapping server.
void emberAfTargetCommunicationControllerRemapAction(uint8_t deviceType,
                                                     uint8_t bank,
                                                     uint8_t action,
                                                     uint8_t mappingFlags,
                                                     uint8_t *rfDat,
                                                     uint8_t rfLen,
                                                     uint8_t *irDat,
                                                     uint8_t irLen)
{
  EmberAfRf4ceZrcMappableAction mappableAction;
  EmberAfRf4ceZrcActionMapping  actionMapping;
  uint8_t bRf = ((rfDat != 0) && (rfLen >= 3));
  uint8_t bIr = ((irDat != 0) && (irLen >= 4));

  mappableAction.actionDeviceType = deviceType;
  mappableAction.actionBank = bank;
  mappableAction.actionCode = action;

  actionMapping.mappingFlags     = mappingFlags
    // Force the RF bit to set if the remap action contains rf mappings
    | (bRf ? EMBER_AF_RF4CE_ZRC_ACTION_MAPPING_MAPPING_FLAGS_RF_SPECIFIED_BIT : 0)
    // Force the IR bit to set if the remap action contains ir mappings
    | (bIr ? EMBER_AF_RF4CE_ZRC_ACTION_MAPPING_MAPPING_FLAGS_IR_SPECIFIED_BIT : 0);
  actionMapping.rfConfig         = bRf ? rfDat[0] : 0;
  actionMapping.rf4ceTxOptions   = bRf ? rfDat[1] : 0;
  actionMapping.actionDataLength = bRf ? (rfLen - 2) : 0;
  actionMapping.actionData       =
    (actionMapping.actionDataLength > 0) ? &rfDat[2] : 0;
  actionMapping.irConfig         = bIr ? irDat[0] : 0;
  actionMapping.irVendorId       =
    bIr ? (uint16_t)(((uint16_t)irDat[2] * 256) + (uint16_t)irDat[1]) : 0;
  actionMapping.irCodeLength     = bIr ? (irLen - 3) : 0;
  actionMapping.irCode           =
    (actionMapping.irCodeLength > 0) ? &irDat[3] : 0;

  if (bRf || bIr) {
    emberAfAppPrintln(
      "emberAfTargetCommunicationControllerRemapAction: DeviceType=0x%x, Bank=0x%x, Action=0x%x",
      deviceType, bank, action);
    emberAfAppPrintln(
      "  MappingFlags=0x%x", actionMapping.mappingFlags);
    emberAfRf4ceZrc20ActionMappingServerRemapAction(
      &mappableAction, &actionMapping);
  }
  if (bRf) {
    emberAfAppPrintln("  RF: Config=0x%x", actionMapping.rfConfig);
    emberAfAppPrintln("  RF: Options=0x%x", actionMapping.rf4ceTxOptions);
    emberAfAppPrintln("  RF: Datalength=%d", actionMapping.actionDataLength);
  }
  if (bIr) {
    emberAfAppPrintln("  IR: Config=0x%x", actionMapping.irConfig);
    emberAfAppPrintln("  IR: Vendor=0x%2x", actionMapping.irVendorId);
    emberAfAppPrintln("  IR: Datalength=%d", actionMapping.irCodeLength);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Test functions

static unsigned char uFillBuffer(int iPat)
{
  if (iPat == 0) {
    appTxBufDat[0]=0x00;
    appTxBufDat[1]=0xC0;
    appTxBufDat[2]=0x05;
    return 3;
  } else {
    appTxBufDat[0]='a';
    appTxBufDat[1]='b';
    return 2;
  }
}

// Test encoding and decoding at application level.
void emAfTargetCommunicationTestAppEncodingAndDecoding(void)
{
  unsigned char appRxBufLen;
  unsigned char appTxBufLen;
  uint16_t        preTxBufLen;
  unsigned char messageId;
  unsigned char *pDat;
  unsigned char uLen;
  unsigned char xBuf[10];

  emberAfAppPrintln("-------------- Test App Encoding --------------");

  MEMCOPY(xBuf, "abc", 3);
  vPrintBuffer(xBuf, 3);
  appTxBufLen = appEncode(5, xBuf, 3);
  vPrintBuffer(appTxBufDat, appTxBufLen);
  preTxBufLen = preEncode(appTxBufLen);
  vPrintBuffer(preTxBufDat, preTxBufLen);
  MEMCOPY(preRxBufDat, preTxBufDat, preTxBufLen);
  preRxBufLen = preTxBufLen;
  vPrintBuffer(preRxBufDat, preRxBufLen);
  appRxBufLen = preDecode();
  vPrintBuffer(appRxBufDat, appRxBufLen);
  appDecode(appRxBufLen, &messageId, &pDat, &uLen);
  emberAfAppPrintln("messageId=%d, pDat=0x%x, uLen=%d", messageId, pDat, uLen);
  vPrintBuffer(pDat, uLen);

  emberAfAppPrintln("----");

  appTxBufLen = appEncode(6, 0, 0);
  vPrintBuffer(appTxBufDat, appTxBufLen);
  preTxBufLen = preEncode(appTxBufLen);
  vPrintBuffer(preTxBufDat, preTxBufLen);
  MEMCOPY(preRxBufDat, preTxBufDat, preTxBufLen);
  preRxBufLen = preTxBufLen;
  appRxBufLen = preDecode();
  appDecode(appRxBufLen, &messageId, &pDat, &uLen);
  emberAfAppPrintln("messageId=%d, pDat=0x%x, uLen=%d", messageId, pDat, uLen);
  vPrintBuffer(pDat, uLen);

  emberAfAppPrintln("-----------------------------------------------");
}


// Test encoding and decoding at presentation level.
void emAfTargetCommunicationTestPreEncodingAndDecoding(void)
{
  unsigned char appRxBufLen;
  unsigned char appTxBufLen;
  uint16_t        preTxBufLen;
  uint16_t        i;

  emberAfAppPrintln("-------------- Test Pre Encoding --------------");

  appTxBufLen = uFillBuffer(0);
  vPrintBuffer(appTxBufDat, appTxBufLen);
  preTxBufLen = preEncode(appTxBufLen);
  vPrintBuffer(preTxBufDat, preTxBufLen);
  MEMCOPY(preRxBufDat, preTxBufDat, preTxBufLen);
  preRxBufLen = preTxBufLen;
  appRxBufLen = preDecode();
  vPrintBuffer(appRxBufDat, appRxBufLen);

  emberAfAppPrintln("----");

  appTxBufLen = uFillBuffer(1);
  vPrintBuffer(appTxBufDat, appTxBufLen);
  preTxBufLen = preEncode(appTxBufLen);
  vPrintBuffer(preTxBufDat, preTxBufLen);
  MEMCOPY(preRxBufDat, preTxBufDat, preTxBufLen);
  preRxBufLen = preTxBufLen;
  appRxBufLen = preDecode();
  vPrintBuffer(appRxBufDat, appRxBufLen);

  emberAfAppPrintln("----");

  appTxBufLen = uFillBuffer(0);
  preTxBufLen = preEncode(appTxBufLen);
  for (i = 0; i < preTxBufLen; i++) {
    appRxBufLen = hostRxOneByte(preTxBufDat[i]);
  }
  emberAfAppPrintln("uAppBufLen=%d", appRxBufLen);
  vPrintBuffer(appRxBufDat, appRxBufLen);

  emberAfAppPrintln("-----------------------------------------------");
}

// Test the serial communication transfer speed for write.
void emAfTargetCommunicationTestUsbTransferSpeed(uint8_t port)
{
  int   i;
  char  xBuf[100];

  uint32_t Ms1 = halCommonGetInt32uMillisecondTick();
  for (i = 0; i < 10; i++) {
    MEMSET(xBuf, 'a'+i, sizeof(xBuf));
    emberSerialWriteData(port, (uint8_t *)xBuf, sizeof(xBuf));
  }
  uint32_t Ms2 = halCommonGetInt32uMillisecondTick();
  emberAfAppPrintln("Ms/tfr=%u", Ms2-Ms1);
}

// By calling this function repeatedly, it will print the number of times it
// has been called pr second.
// It can be used to count the number of some event pr second.
void emAfTargetCommunicationTestPrSecond(void)
{
static uint32_t start;
static uint32_t cnt;

  if (start == 0) {
    start = halCommonGetInt32uMillisecondTick();
  }
  cnt++;
  uint32_t now = halCommonGetInt32uMillisecondTick();
  uint32_t ela = now-start;
  if (ela > 1000) {
    emberAfAppPrintln("Sec: %d", cnt);
    cnt=0;
    start += 1000;
  }
}
