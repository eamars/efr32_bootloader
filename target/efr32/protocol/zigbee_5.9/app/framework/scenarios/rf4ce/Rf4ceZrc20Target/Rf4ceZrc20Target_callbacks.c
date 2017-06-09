//

// This callback file is created for your convenience. You may add application code
// to this file. If you regenerate this file over a previous version, the previous
// version will be overwritten and any code you have added will be lost.

#include "app/framework/include/af.h"
#include "app/framework/plugin/rf4ce-profile/rf4ce-profile.h"
#include "app/framework/plugin/rf4ce-gdp/rf4ce-gdp.h"
#include "app/framework/plugin/rf4ce-zrc20/rf4ce-zrc20.h"
#include "app/framework/plugin/rf4ce-zrc20/rf4ce-zrc20-internal.h"
#include "app/framework/plugin/rf4ce-zrc20-action-mapping-server/rf4ce-zrc20-action-mapping-server.h"
#include "app/framework/plugin/rf4ce-zrc20-ha-server/rf4ce-zrc20-ha-server.h"
#include "app/framework/plugin/ezmode-commissioning/ez-mode.h"


#define DEMO_READ_ATTRIBUTE_EVENT_DELAY         MILLISECOND_TICKS_PER_SECOND

#define DEMO_DIALOG_MENU_TIMEOUT_QS             80
#define DEMO_SECOND_BUTTON_TIMEOUT_QS           20
#define DEMO_VALIDATION_TIMEOUT_MS              10000
#define DEMO_DEFAULT_SHORT_POLL_INTERVAL_MS     500
#define DEMO_LATENCY_TEST_KEY                   0xFB

#define BUTTON                                  BUTTON0

#define MAPPABLE_ACTION_ENTRY_INDEX_MUTE        3


/* These are arbitrary numbers for demonstration purposes only. */
#define APP_HA_PAIRING_INDEX                0
#define APP_HA_INSTANCE_ID                  0


// HA network state machine
enum {
  STATE_HA_NETWORK_DOWN,
  STATE_HA_NETWORK_JOINING,
  STATE_HA_NETWORK_JOINED_LIGHT_NOT_PAIRED,
  STATE_HA_NETWORK_JOINED_LIGHT_PAIRED,
  STATE_HA_NETWORK_JOINED_LIGHT_PAIRED_MAPPED,
};

// CLI menu state machine
enum {
  STATE_MENU_CLOSED,
  STATE_MENU_HA_JOIN_OPEN,
  STATE_MENU_HA_LIGHT_DISCOVERY_OPEN,
  STATE_MENU_HA_LIGHT_SELECT_OPEN,
  STATE_MENU_HA_LIGHT_MAP_OPEN,
  STATE_MENU_HA_LIGHT_CONTROL_OPEN,
  STATE_MENU_HA_LIGHT_QUERYING_TIME_OPEN,
  STATE_MENU_HA_LIGHT_AUTO_TOGGLING_OPEN,
};


#define fatal(message, status)                                     \
  do {                                                             \
    emberAfCorePrintln("%p: %p (0x%x)", "FATAL", message, status); \
    emberSerialWaitSend(EMBER_AF_PRINT_OUTPUT);                    \
    halReboot();                                                   \
  } while (false)
#define error(message, status)                                     \
  do {                                                             \
    emberAfCorePrintln("%p: %p (0x%x)", "ERROR", message, status); \
    sadTune();                                                     \
  } while (false)
#define warning(message, status)                                     \
  do {                                                               \
    emberAfCorePrintln("%p: %p (0x%x)", "WARNING", message, status); \
    /*waitTune();*/                                                  \
  } while (false)
#define info(message)                              \
  do {                                             \
    emberAfCorePrintln("%p: %p", "INFO", message); \
    waitTune();                                    \
  } while (false)
#define success(message)               \
  do {                                 \
    emberAfCorePrintln("%p", message); \
    happyTune();                       \
  } while (false)
#define wait(message)                  \
  do {                                 \
    emberAfCorePrintln("%p", message); \
    waitTune();                        \
  } while (false)

static uint8_t PGM happyTune[] = {
  NOTE_B4, 1,
  0,       1,
  NOTE_B5, 1,
  0,       0
};
static uint8_t PGM sadTune[] = {
  NOTE_B5, 1,
  0,       1,
  NOTE_B4, 5,
  0,       0
};
static uint8_t PGM waitTune[] = {
  NOTE_B4, 1,
  0,       0
};

#define happyTune() halPlayTune_P(happyTune, true);
#define sadTune()   halPlayTune_P(sadTune,   true);
#define waitTune()  halPlayTune_P(waitTune,  true);


// Event control struct declarations
EmberEventControl autoToggleEventControl;
EmberEventControl dialogMenuEventControl;
EmberEventControl secondButtonTimeoutEventControl;
EmberEventControl buttonEventControl;
EmberEventControl networkStartEventControl;
EmberEventControl readAttributeEventControl;
EmberEventControl readAttributeTimeoutEventControl;

// We only support EZ-Mode Commissioning for the On/Off cluster.
static uint16_t appClusterIds[] = {ZCL_ON_OFF_CLUSTER_ID};
static uint16_t readAttributeEventDelayMs = MILLISECOND_TICKS_PER_SECOND;
static uint16_t readAttributeTimeoutMs = 30 * MILLISECOND_TICKS_PER_SECOND;
static uint16_t autoToggleIntervalMS = 0;
static uint8_t selectedLight = 0;

static uint8_t theirPairingIndex = 0;
static uint8_t theirHaInstanceId = 0;
static uint8_t theirBindingIndex = EMBER_NULL_BINDING;
static uint8_t numCount = 0;
static bool theirOnOff = false;
static bool useApsAcks = false;

#define VALIDATION_CODE_LENGTH 3
static uint8_t expectedValidationCode[VALIDATION_CODE_LENGTH];
static uint8_t currentValidationCode[VALIDATION_CODE_LENGTH];
static uint8_t currentValidationCodeIndex;
static uint8_t currentValidationPairingIndex = 0xFF;


static uint8_t haNetworkState = STATE_HA_NETWORK_DOWN;
static uint8_t cliMenuState = STATE_MENU_CLOSED;


static void userControlPressed(uint8_t pairingIndex,
                               EmberAfRf4ceZrcActionBank actionBank,
                               EmberAfRf4ceZrcActionCode actionCode);

static void printHAJoinDialogMenu(void);
static void printHALightDiscoveryDialogMenu(void);
static void printHALightSelectDialogMenu(void);
static void printHALightMapDialogMenu(void);
static void printHAControlDialogMenu(void);
static void printHALightQueryingTimeDialogMenu(void);
static void printHALightAutoTogglingDialogMenu(void);
static void printLightStatus(void);
static bool bindingMatches(void);
static void SendOnOffAttributeReadCommand(void);
static void controlLight(uint8_t commandId);

static void AmRemap(void);
static void AmUnmap(void);
static void haJoin(void);
static void haLeave(void);
static void haFindLight(void);


EmberCommandEntry emberAfCustomCommands[] = {
    emberCommandEntryAction("remap", AmRemap, "", "Remap action."),
    emberCommandEntryAction("unmap", AmUnmap, "", "Unmap action."),
    emberCommandEntryAction("hajoin",  haJoin, "",  "Find joinable HA network."),
    emberCommandEntryAction("haleave", haLeave, "", "Leave HA network."),
    emberCommandEntryTerminator()
};


/* Event handlers. */
void buttonEventHandler(void)
{
  bool setPending = (halButtonState(BUTTON) == BUTTON_PRESSED);
  emberAfRf4ceGdpPushButton(setPending);
  emberEventControlSetInactive(buttonEventControl);
}

void networkStartEventHandler(void)
{
    EmberStatus status = emberAfRf4ceStart();
    if (status == EMBER_SUCCESS) {
      info("RF4CE node started");
      emberEventControlSetInactive(networkStartEventControl);
    } else {
      error("RF4CE start failed", status);
      emberEventControlSetActive(networkStartEventControl);
    }
}

void readAttributeEventHandler(void)
{
    SendOnOffAttributeReadCommand();

  if (!emberEventControlGetActive(readAttributeTimeoutEventControl)) {
    emberEventControlSetDelayMS(readAttributeTimeoutEventControl,
                                readAttributeTimeoutMs);
  }

  emberEventControlSetDelayMS(readAttributeEventControl,
                              readAttributeEventDelayMs);
}

void readAttributeTimeoutEventHandler(void)
{
  haNetworkState = STATE_HA_NETWORK_JOINED_LIGHT_NOT_PAIRED;
  error("Lost communication with the light", EMBER_NO_RESPONSE);
  emberEventControlSetInactive(readAttributeEventControl);
  emberEventControlSetInactive(readAttributeTimeoutEventControl);
  emberEventControlSetInactive(autoToggleEventControl);
}

void autoToggleEventHandler(void)
{
  emberEventControlSetInactive(autoToggleEventControl);

  controlLight(ZCL_TOGGLE_COMMAND_ID);

  if (autoToggleIntervalMS > 0) {
    emberEventControlSetDelayMS(autoToggleEventControl, autoToggleIntervalMS);
  }
}

void dialogMenuEventHandler(void)
{
  emberEventControlSetInactive(dialogMenuEventControl);

  emberAfCorePrintln("***Dialog MENU now closed***");

  cliMenuState = STATE_MENU_CLOSED;
}

void secondButtonTimeoutEventHandler(void)
{
    emberEventControlSetInactive(secondButtonTimeoutEventControl);
    cliMenuState = STATE_MENU_CLOSED;
    emberAfCorePrintln("***Time window for second button expired***");

    if (EMBER_SUCCESS ==
          emberAfPluginRf4ceZrc20HaMappingToLogicalDeviceAdd(theirPairingIndex,
                                                             theirHaInstanceId,
                                                             selectedLight)) {
      emberAfCorePrintln("Instance %d mapped to light %d",
                         theirHaInstanceId,
                         selectedLight);
      haNetworkState = STATE_HA_NETWORK_JOINED_LIGHT_PAIRED_MAPPED;
    }
}


/** @brief Pre Command Received
 *
 * This callback is the second in the Application Framework's message
 * processing chain. At this point in the processing of incoming over-the-air
 * messages, the application has determined that the incoming message is a ZCL
 * command. It parses enough of the message to populate an
 * EmberAfClusterCommand struct. The Application Framework defines this struct
 * value in a local scope to the command processing but also makes it
 * available through a global pointer called emberAfCurrentCommand, in
 * app/framework/util/util.c. When command processing is complete, this
 * pointer is cleared.
 *
 * @param cmd   Ver.: always
 */
bool emberAfPreCommandReceivedCallback(EmberAfClusterCommand *cmd)
{
    uint8_t i, j;
    uint8_t destIndex = 0, tmpIndex = 0;
    DestStruct dest;
    EmberBindingTableEntry incomingBinding;
    EmberAfRf4ceZrcHomeAutomationAttribute tmpHaAttribute;

  if (bindingMatches()
      && (cmd->apsFrame->profileId == HA_PROFILE_ID
          || cmd->apsFrame->profileId == EMBER_WILDCARD_PROFILE_ID)
      && cmd->apsFrame->clusterId == ZCL_ON_OFF_CLUSTER_ID
      && !cmd->clusterSpecific
      && !cmd->mfgSpecific
      && cmd->direction == ZCL_DIRECTION_SERVER_TO_CLIENT
      && cmd->commandId == ZCL_READ_ATTRIBUTES_RESPONSE_COMMAND_ID
      && cmd->bufLen == 8
      && cmd->payloadStartIndex == 3
      && (emberAfGetInt16u(cmd->buffer, cmd->payloadStartIndex, cmd->bufLen)
          == ZCL_ON_OFF_ATTRIBUTE_ID)
      && (emberAfGetInt8u(cmd->buffer, cmd->payloadStartIndex + 2, cmd->bufLen)
          == EMBER_ZCL_STATUS_SUCCESS)
      && (emberAfGetInt8u(cmd->buffer, cmd->payloadStartIndex + 3, cmd->bufLen)
          == ZCL_BOOLEAN_ATTRIBUTE_TYPE)) {

    bool onOff = emberAfGetInt8u(cmd->buffer,
                                    cmd->payloadStartIndex + 4,
                                    cmd->bufLen);

    dest.type = EMBER_OUTGOING_VIA_BINDING;
    dest.indexOrDestination = emberAfGetBindingIndex();
    if (EMBER_SUCCESS == emberGetBinding(dest.indexOrDestination,
                                         &incomingBinding))
    {
        dest.sourceEndpoint = incomingBinding.local;
        dest.destinationEndpoint = incomingBinding.remote;

        /* Look up all pairingIndex & haInstanceId pairs mapped to the logical
         * device this packet was received from. Update the haAttribute of each. */
        if (EMBER_SUCCESS == emberAfPluginRf4ceZrc20HaLogicalDeviceIndexLookUp(&dest,
                                                                               &destIndex))
        {
            for (i=0; i<EMBER_RF4CE_PAIRING_TABLE_SIZE; i++)
            {
                for (j=0; j<ZRC_HA_SERVER_NUM_OF_HA_INSTANCES; j++)
                {
                    emberAfPluginRf4ceZrc20HaMappingToLogicalDeviceGet(i, j, &tmpIndex);
                    if (destIndex == tmpIndex)
                    {
                        tmpHaAttribute.contents = (uint8_t *)&onOff;
                        tmpHaAttribute.contentsLength = 1;
                        /* If status changed, update HA attribute and set its 'dirty' flag. */
                        emberAfPluginRf4ceZrc20HaServerSetHaAttribute(i,
                                                                      j,
                                                                      ZRC_HA_ON_OFF_ON_OFF_ID,
                                                                      &tmpHaAttribute);
                    }
                }
            }
        }

        if (theirOnOff != onOff) {
          theirOnOff = onOff;
          printLightStatus();
        }
    }

    emberEventControlSetInactive(readAttributeTimeoutEventControl);
  }
  return false;
}

/** @brief Stack Status
 *
 * This function is called by the application framework from the stack status
 * handler.  This callbacks provides applications an opportunity to be
 * notified of changes to the stack status and take appropriate action.  The
 * return code from this callback is ignored by the framework.  The framework
 * will always process the stack status after the callback returns.
 *
 * @param status   Ver.: always
 */
bool emberAfStackStatusCallback(EmberStatus status)
{
    if (emberGetCurrentNetwork() == EMBER_AF_NETWORK_INDEX_PRO) {
      if (status == EMBER_NETWORK_UP) {
        success("PRO stack UP");
        haNetworkState = STATE_HA_NETWORK_JOINED_LIGHT_NOT_PAIRED;

        // Set the short poll interval to the default value.
        emberAfSetShortPollIntervalMsCallback(DEMO_DEFAULT_SHORT_POLL_INTERVAL_MS);
      } else if (emberAfNetworkState() == EMBER_NO_NETWORK) {
        error("PRO stack DOWN", status);

        haNetworkState = STATE_HA_NETWORK_DOWN;
        emberEventControlSetInactive(readAttributeEventControl);
        emberEventControlSetInactive(readAttributeTimeoutEventControl);
      }
    } else {
      if (status == EMBER_NETWORK_UP) {
        success("RF4CE stack UP");
        emberEventControlSetInactive(networkStartEventControl);
        emberSetCurrentNetwork(EMBER_AF_NETWORK_INDEX_PRO);
        /* Clear all HA server attributes. */
        emberAfPluginRf4ceZrc20HaServerClearAllHaAttributes();
        /* Clear pairingIndex and haInstanceId mapping to logical devices. */
        emberAfPluginRf4ceZrc20HaLogicalDeviceAndInstanceToLogicalDeviceMappingClear();
      } else if (emberAfNetworkState() == EMBER_NO_NETWORK) {
        error("RF4CE stack DOWN", status);
        emberEventControlSetActive(networkStartEventControl);
      }
    }
    return false;
}

/** @brief Main Init
 *
 * This function is called from the application's main function. It gives the
 * application a chance to do any initialization required at system startup.
 * Any code that you would normally put into the top of the application's
 * main() routine should be put into this function.
        Note: No callback
 * in the Application Framework is associated with resource cleanup. If you
 * are implementing your application on a Unix host where resource cleanup is
 * a consideration, we expect that you will use the standard Posix system
 * calls, including the use of atexit() and handlers for signals such as
 * SIGTERM, SIGINT, SIGCHLD, SIGPIPE and so on. If you use the signal()
 * function to register your signal handler, please mind the returned value
 * which may be an Application Framework function. If the return value is
 * non-null, please make sure that you call the returned function from your
 * handler to avoid negating the resource cleanup of the Application Framework
 * itself.
 *
 */
void emberAfMainInitCallback(void)
{
  emberEventControlSetActive(networkStartEventControl);
}

/** @brief emberAfHalButtonIsrCallback
 *
 *
 */
// Hal Button ISR Callback
// This callback is called by the framework whenever a button is pressed on the
// device. This callback is called within ISR context.
void emberAfHalButtonIsrCallback(uint8_t button, uint8_t state)
{
  if (button == BUTTON) {
    emberEventControlSetActive(buttonEventControl);
  }
}

/** @brief Ok To Sleep
 *
 * This function is called by the Idle/Sleep plugin before sleeping.  The
 * application should return true if the device may sleep or false otherwise.
 *
 * @param durationMs The maximum duration in milliseconds that the device will
 * sleep.  Ver.: always
 */
bool emberAfPluginIdleSleepOkToSleepCallback(uint32_t durationMs)
{
  return false;
}

/** @brief Start Validation
 *
 * This function is called by the RF4CE GDP plugin when the application should
 * begin the validation procedure.  The application must complete the
 * validation within the validation wait time to avoid the validation
 * automatically failing due to a timeout.
 *
 * @param pairingIndex The index of the pairing entry.  Ver.: always
 */
void emberAfPluginRf4ceGdpStartValidationCallback(uint8_t pairingIndex)
{
  uint8_t i;

  currentValidationCodeIndex = 0;
  currentValidationPairingIndex = pairingIndex;

  halPlayTune_P(waitTune, true);

  // Generate a random n-digit validation code in [0,9].
  emberAfCorePrint("Enter [");
  for (i = 0; i < VALIDATION_CODE_LENGTH; i++) {
    expectedValidationCode[i] = (halCommonGetRandom() % 10);
    emberAfCorePrint(" %d", expectedValidationCode[i]);
  }
  emberAfCorePrintln(" ] on the controller or press the pairing button on the target");
}

/** @brief Binding Complete
 *
 * This function is called by the RF4CE GDP plugin when the binding operation
 * completes.  If status is ::EMBER_SUCCESS, binding was successful and
 * pairingIndex indicates the index in the pairing table for the remote node.
 *
 * @param status The status of the binding operation.  Ver.: always
 * @param pairingIndex The index of the pairing entry.  Ver.: always
 */
void emberAfPluginRf4ceGdpBindingCompleteCallback(EmberAfRf4ceGdpBindingStatus status,
                                                  uint8_t pairingIndex)
{
  if (status == EMBER_SUCCESS) {
      theirPairingIndex = pairingIndex;
    emberAfCorePrintln("Binding %p: 0x%x", "complete", pairingIndex);
    halPlayTune_P(happyTune, true);
  } else {
    emberAfCorePrintln("Binding %p: 0x%x", "failed", status);
    halPlayTune_P(sadTune, true);
  }
  currentValidationPairingIndex = 0xFF;
}

/** @brief Client Complete
 *
 * This function is called by the EZ-Mode Commissioning plugin when client
 * commissioning completes.
 *
 * @param bindingIndex The binding index that was created or
 * ::EMBER_NULL_BINDING if an error occurred.  Ver.: always
 */
void emberAfPluginEzmodeCommissioningClientCompleteCallback(uint8_t bindingIndex)
{
    uint8_t i;
    EmberBindingTableEntry bindingEntry;
    DestStruct dest;

    if (bindingIndex == EMBER_NULL_BINDING)
    {
        emberAfCorePrintln("ezmode commissioning %p: 0x%x", "failed", bindingIndex);
        return;
    }
    emberAfCorePrintln("ezmode commissioning %p: 0x%x", "complete", bindingIndex);

    theirBindingIndex = bindingIndex;
    if (EMBER_SUCCESS != emberGetBinding(bindingIndex, &bindingEntry))
    {
        emberAfCorePrintln("binding index out of range");
        return;
    }

    dest.type = EMBER_OUTGOING_VIA_BINDING;
    dest.indexOrDestination = bindingIndex;
    dest.sourceEndpoint = bindingEntry.local;
    dest.destinationEndpoint = bindingEntry.remote;
    if (EMBER_SUCCESS != emberAfPluginRf4ceZrc20HaLogicalDeviceDestinationAdd(&dest, &i))
    {
        emberAfCorePrintln("logical device table full");
        return;
    }
    haNetworkState = STATE_HA_NETWORK_JOINED_LIGHT_PAIRED;
    emberAfCorePrintln("light stored in HA device table");
}



/** @brief Action
 *
 * This function is called by the RF4CE ZRC 2.0 plugin when an action starts
 * or stops.  If the action type of the action record is
 * ::EMBER_AF_RF4CE_ZRC_ACTION_TYPE_START, the application should execute the
 * requested operation repeatedly at some application-specific rate.  When the
 * repetition should stop, the plugin will call the callback again with the
 * action type set to ::EMBER_AF_RF4CE_ZRC_ACTION_TYPE_STOP.  :: or
 * ::EMBER_AF_RF4CE_ZRC_ACTION_TYPE_REPEAT is a special case of
 * ::EMBER_AF_RF4CE_ZRC_ACTION_TYPE_START and means that the start action from
 * the originator was not received and that the originator is still triggering
 * the action.  The application should process a repeat type the same as a
 * start type, but may wish to perform additional operations to compensate for
 * missed actions.  If the action type is
 * ::EMBER_AF_RF4CE_ZRC_ACTION_TYPE_ATOMIC, the application should execute the
 * operation once.  The plugin will not call the callback again for an atomic
 * action.
 *
 * @param record The action record.  Ver.: always
 */
void emberAfPluginRf4ceZrc20ActionCallback(const EmberAfRf4ceZrcActionRecord *record)
{
  if (currentValidationPairingIndex == record->pairingIndex) {
    if ((record->actionType == EMBER_AF_RF4CE_ZRC_ACTION_TYPE_START
         || record->actionType == EMBER_AF_RF4CE_ZRC_ACTION_TYPE_REPEAT
         || record->actionType == EMBER_AF_RF4CE_ZRC_ACTION_TYPE_ATOMIC)
        && record->actionBank == EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC
        && (EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_0
            <= record->actionCode)
        && (record->actionCode
            <= EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_9)
        && currentValidationCodeIndex < VALIDATION_CODE_LENGTH) {
      uint8_t code = (record->actionCode
                    - EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_0);
      currentValidationCode[currentValidationCodeIndex++] = code;
      if (currentValidationCodeIndex == VALIDATION_CODE_LENGTH) {
        if (MEMCOMPARE(expectedValidationCode,
                       currentValidationCode,
                       VALIDATION_CODE_LENGTH)
            == 0) {
          emberAfRf4ceGdpSetValidationStatus(EMBER_AF_RF4CE_GDP_BINDING_STATUS_SUCCESS);
        } else {
          emberAfRf4ceGdpSetValidationStatus(EMBER_AF_RF4CE_GDP_CHECK_VALIDATION_STATUS_FAILURE);
        }
        currentValidationPairingIndex = 0xFF;
      }
    }
  } else {
    emberAfCorePrintln("Action:");
    emberAfCorePrintln("  pairingIndex:        %d",    record->pairingIndex);
    emberAfCorePrintln("  actionType:          0x%x",  record->actionType);
    emberAfCorePrintln("  modifierBits:        0x%x",  record->modifierBits);
    emberAfCorePrintln("  actionPayloadLength: %d",    record->actionPayloadLength);
    emberAfCorePrintln("  actionBank:          0x%x",  record->actionBank);
    emberAfCorePrintln("  actionCode:          0x%x",  record->actionCode);
    emberAfCorePrintln("  actionVendorId:      0x%2x", record->actionVendorId);
    emberAfCorePrint(  "  actionPayload:       ");
    emberAfCorePrintBuffer(record->actionPayload,
                           record->actionPayloadLength,
                           true);
    emberAfCorePrintln("");
    emberAfCorePrintln("  timeMs:              0x%2x", record->timeMs);

    userControlPressed(record->pairingIndex,
                       record->actionBank,
                       record->actionCode);
  }
}

void emberAfPluginRf4ceZrc20HaServerHaActionSentCallback(EmberOutgoingMessageType type,
                                                         uint16_t indexOrDestination,
                                                         EmberApsFrame* apsFrame,
                                                         uint16_t msgLen,
                                                         uint8_t* message,
                                                         EmberStatus status)
{
    UNUSED_VAR(type);
    UNUSED_VAR(indexOrDestination);
    UNUSED_VAR(apsFrame);
    UNUSED_VAR(msgLen);
    UNUSED_VAR(message);
    UNUSED_VAR(status);

    SendOnOffAttributeReadCommand();

    if (!emberEventControlGetActive(readAttributeTimeoutEventControl)) {
      emberEventControlSetDelayMS(readAttributeTimeoutEventControl,
                                  readAttributeTimeoutMs);
    }
}


static void userControlPressed(uint8_t pairingIndex,
                               EmberAfRf4ceZrcActionBank actionBank,
                               EmberAfRf4ceZrcActionCode actionCode)
{
  if (cliMenuState == STATE_MENU_CLOSED) {
    switch (actionCode) {
    case EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_ROOT_MENU:
      emberEventControlSetDelayQS(dialogMenuEventControl,
                                  DEMO_DIALOG_MENU_TIMEOUT_QS);
      if (haNetworkState == STATE_HA_NETWORK_JOINED_LIGHT_PAIRED_MAPPED) {
        cliMenuState = STATE_MENU_HA_LIGHT_CONTROL_OPEN;
        printHAControlDialogMenu();
      } else if (haNetworkState == STATE_HA_NETWORK_JOINED_LIGHT_PAIRED) {
        cliMenuState = STATE_MENU_HA_LIGHT_SELECT_OPEN;
        printHALightSelectDialogMenu();
      } else if (haNetworkState == STATE_HA_NETWORK_DOWN) {
        cliMenuState = STATE_MENU_HA_JOIN_OPEN;
        printHAJoinDialogMenu();
      } else if (haNetworkState == STATE_HA_NETWORK_JOINED_LIGHT_NOT_PAIRED) {
        cliMenuState = STATE_MENU_HA_LIGHT_DISCOVERY_OPEN;
        printHALightDiscoveryDialogMenu();
      }
      break;
    default:
      emberAfCorePrintln("KEY 0x%x", actionCode);
      break;
    }
  } else if (cliMenuState == STATE_MENU_HA_JOIN_OPEN) {
    cliMenuState = STATE_MENU_CLOSED;
    emberEventControlSetInactive(dialogMenuEventControl);
    switch (actionCode) {
    case EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_ROOT_MENU:
      emberEventControlSetDelayQS(dialogMenuEventControl,
                                  DEMO_DIALOG_MENU_TIMEOUT_QS);
      cliMenuState = STATE_MENU_HA_JOIN_OPEN;
      printHAJoinDialogMenu();
      break;
    case EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_0:
        haJoin();
      break;
    default:
      emberAfCorePrintln("Unexpected key 0x%x", actionCode);
      break;
    }
  } else if (cliMenuState == STATE_MENU_HA_LIGHT_DISCOVERY_OPEN) {
    cliMenuState = STATE_MENU_CLOSED;
    emberEventControlSetInactive(dialogMenuEventControl);
    switch (actionCode) {
    case EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_ROOT_MENU:
      emberEventControlSetDelayQS(dialogMenuEventControl,
                                  DEMO_DIALOG_MENU_TIMEOUT_QS);
      cliMenuState = STATE_MENU_HA_LIGHT_DISCOVERY_OPEN;
      printHALightDiscoveryDialogMenu();
      break;
    case EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_0:
        haFindLight();
      break;
    case EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_1:
      haLeave();
      break;
    default:
      emberAfCorePrintln("Unexpected key 0x%x", actionCode);
      break;
    }
  } else if (cliMenuState == STATE_MENU_HA_LIGHT_SELECT_OPEN) {
    cliMenuState = STATE_MENU_CLOSED;
    emberEventControlSetInactive(dialogMenuEventControl);

    if (actionCode == EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_ROOT_MENU) {
      emberEventControlSetDelayQS(dialogMenuEventControl,
                                  DEMO_DIALOG_MENU_TIMEOUT_QS);
      cliMenuState = STATE_MENU_HA_LIGHT_SELECT_OPEN;
      printHALightSelectDialogMenu();
    } else if (actionCode == EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_0) {
        selectedLight = actionCode - EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_0;
        cliMenuState = STATE_MENU_HA_LIGHT_MAP_OPEN;
        numCount = 0;
        printHALightMapDialogMenu();
    } else {
      emberAfCorePrintln("Unexpected key 0x%x", actionCode);
    }
  } else if (cliMenuState == STATE_MENU_HA_LIGHT_MAP_OPEN) {
    emberEventControlSetInactive(dialogMenuEventControl);

    if (actionCode == EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_ROOT_MENU) {
      emberEventControlSetDelayQS(dialogMenuEventControl,
                                  DEMO_DIALOG_MENU_TIMEOUT_QS);
      cliMenuState = STATE_MENU_HA_LIGHT_MAP_OPEN;
      numCount = 0;
      printHALightMapDialogMenu();
    } else if (actionCode >= EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_0
               && actionCode <= EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_9) {
        numCount++;
        if (numCount == 1) {
            theirHaInstanceId = actionCode-EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_0;
            emberEventControlSetDelayQS(secondButtonTimeoutEventControl,
                                        DEMO_SECOND_BUTTON_TIMEOUT_QS);
        } else if (numCount == 2) {
            cliMenuState = STATE_MENU_CLOSED;
            emberEventControlSetInactive(secondButtonTimeoutEventControl);
            theirHaInstanceId =
              (10 * theirHaInstanceId
                + actionCode-EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_0 >= ZRC_HA_SERVER_NUM_OF_HA_INSTANCES)
              ? theirHaInstanceId
              : 10 * theirHaInstanceId
                + actionCode-EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_0;
            if (EMBER_SUCCESS ==
                  emberAfPluginRf4ceZrc20HaMappingToLogicalDeviceAdd(theirPairingIndex,
                                                                     theirHaInstanceId,
                                                                     selectedLight)) {
              emberAfCorePrintln("Instance %d mapped to light %d",
                                 theirHaInstanceId,
                                 selectedLight);
              haNetworkState = STATE_HA_NETWORK_JOINED_LIGHT_PAIRED_MAPPED;
            }
        }
    } else {
      emberAfCorePrintln("Unexpected key 0x%x", actionCode);
    }
  } else if (cliMenuState == STATE_MENU_HA_LIGHT_CONTROL_OPEN) {
    cliMenuState = STATE_MENU_CLOSED;
    emberEventControlSetInactive(dialogMenuEventControl);

    switch (actionCode) {
    case EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_ROOT_MENU:
      emberEventControlSetDelayQS(dialogMenuEventControl,
                                  DEMO_DIALOG_MENU_TIMEOUT_QS);
      cliMenuState = STATE_MENU_HA_LIGHT_CONTROL_OPEN;
      printHAControlDialogMenu();
      break;
    case EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_0:
      printLightStatus();
      break;
    case EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_1:
      controlLight(ZCL_ON_COMMAND_ID);
      break;
    case EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_2:
      controlLight(ZCL_OFF_COMMAND_ID);
      break;
    case EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_3:
      controlLight(ZCL_TOGGLE_COMMAND_ID);
      break;
    case EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_4:
      haLeave();
      break;
    case EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_5:
      cliMenuState = STATE_MENU_HA_LIGHT_QUERYING_TIME_OPEN;
      printHALightQueryingTimeDialogMenu();
      break;
    case EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_6:
      cliMenuState = STATE_MENU_HA_LIGHT_AUTO_TOGGLING_OPEN;
      printHALightAutoTogglingDialogMenu();
      break;
    default:
      emberAfCorePrintln("Unexpected key 0x%x", actionCode);
      break;
    }
  } else if (cliMenuState == STATE_MENU_HA_LIGHT_QUERYING_TIME_OPEN) {
    uint16_t pollRate = DEMO_DEFAULT_SHORT_POLL_INTERVAL_MS;
    cliMenuState = STATE_MENU_CLOSED;
    emberEventControlSetInactive(dialogMenuEventControl);

    switch (actionCode) {
    case EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_ROOT_MENU:
      emberEventControlSetDelayQS(dialogMenuEventControl,
                                  DEMO_DIALOG_MENU_TIMEOUT_QS);
      cliMenuState = STATE_MENU_HA_LIGHT_CONTROL_OPEN;
      printHAControlDialogMenu();
      break;
    case EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_0:
      readAttributeEventDelayMs = 200;
      pollRate = 100;
      break;
    case EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_1:
      readAttributeEventDelayMs = 500;
      pollRate = 250;
      break;
    case EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_2:
      readAttributeEventDelayMs = MILLISECOND_TICKS_PER_SECOND;
      break;
    case EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_3:
      readAttributeEventDelayMs = 2*MILLISECOND_TICKS_PER_SECOND;
      break;
    case EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_4:
      readAttributeEventDelayMs = 5*MILLISECOND_TICKS_PER_SECOND;
      break;
    case EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_5:
      readAttributeEventDelayMs = 10*MILLISECOND_TICKS_PER_SECOND;
      break;
    case EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_6:
      readAttributeEventDelayMs = 60*MILLISECOND_TICKS_PER_SECOND;
      break;
    case EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_7:
      readAttributeEventDelayMs = 0;
      break;
    default:
      emberAfCorePrintln("Unexpected key 0x%x", actionCode);
      return;
    }

    if (readAttributeEventDelayMs > 0) {
      if (readAttributeEventDelayMs < 1000) {
        emberAfCorePrintln("Querying the HA light every %d milliseconds",
                           readAttributeEventDelayMs);
      } else {
        emberAfCorePrintln("Querying the HA light every %d seconds",
                           readAttributeEventDelayMs/1000);
      }
      emberEventControlSetDelayMS(readAttributeEventControl,
                                  readAttributeEventDelayMs);
    } else {
      emberAfCorePrintln("Not querying the HA light");
      emberEventControlSetInactive(readAttributeEventControl);
      emberEventControlSetInactive(readAttributeTimeoutEventControl);
    }

    // Set the short polling interval.
    emberAfPushNetworkIndex(EMBER_AF_NETWORK_INDEX_PRO);
    emberAfSetShortPollIntervalMsCallback(pollRate);
    emberAfPopNetworkIndex();
  } else if (cliMenuState == STATE_MENU_HA_LIGHT_AUTO_TOGGLING_OPEN) {
    cliMenuState = STATE_MENU_CLOSED;
    emberEventControlSetInactive(dialogMenuEventControl);

    switch (actionCode) {
    case EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_ROOT_MENU:
      emberEventControlSetDelayQS(dialogMenuEventControl,
                                  DEMO_DIALOG_MENU_TIMEOUT_QS);
      cliMenuState = STATE_MENU_HA_LIGHT_CONTROL_OPEN;
      printHAControlDialogMenu();
      break;
    case EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_0:
      autoToggleIntervalMS = 500;
      break;
    case EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_1:
      autoToggleIntervalMS = MILLISECOND_TICKS_PER_SECOND;
      break;
    case EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_2:
      autoToggleIntervalMS = 2*MILLISECOND_TICKS_PER_SECOND;
      break;
    case EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_3:
      autoToggleIntervalMS = 5*MILLISECOND_TICKS_PER_SECOND;
      break;
    case EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_4:
      autoToggleIntervalMS = 10*MILLISECOND_TICKS_PER_SECOND;
      break;
    case EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_5:
      autoToggleIntervalMS = 0;
      break;
    default:
      emberAfCorePrintln("Unexpected key 0x%x", actionCode);
      return;
    }

    if (autoToggleIntervalMS > 0) {
      if (autoToggleIntervalMS < 1000) {
        emberAfCorePrintln("Auto-toggling every %dms", autoToggleIntervalMS);
      } else {
        emberAfCorePrintln("Auto-toggling every %ds", autoToggleIntervalMS/1000);
      }
      emberEventControlSetDelayMS(autoToggleEventControl,
                                  autoToggleIntervalMS);
    } else {
      emberAfCorePrintln("Auto-toggling disabled");
      emberEventControlSetInactive(autoToggleEventControl);
    }
  }
}

static void printHAJoinDialogMenu(void)
{
  emberAfCorePrintln("--------------------------------------------");
  emberAfCorePrintln("RF4CE/HA MAIN dialog:");
  emberAfCorePrintln("--------------------------------------------");
  emberAfCorePrintln("(1)        Find HA nwk and join as sleepy");
#ifndef EZSP_HOST
  emberAfCorePrintln("(2)        Print RF4CE network info");
#endif
  emberAfCorePrintln("(MENU)     Re-open this MENU");
  emberAfCorePrintln("--------------------------------------------");
  emberAfCorePrintln("");
}

static void printHALightDiscoveryDialogMenu(void)
{
  emberAfCorePrintln("-----------------------------------");
  emberAfCorePrintln("RF4CE/HA MAIN dialog:");
  emberAfCorePrintln("-----------------------------------");
  emberAfCorePrintln("(1)        Bind to HA light");
  emberAfCorePrintln("(2)        Leave the HA network");
#ifndef EZSP_HOST
  emberAfCorePrintln("(3)        Print RF4CE network info");
#endif
  emberAfCorePrintln("(MENU)     Re-open this MENU");
  emberAfCorePrintln("-----------------------------------");
  emberAfCorePrintln("");
}

static void printHALightSelectDialogMenu(void)
{
  uint8_t i, j;
  DestStruct dest;

  emberAfCorePrintln("-----------------------------------");
  emberAfCorePrintln("RF4CE/HA MAIN dialog:");
  emberAfCorePrintln("-----------------------------------");

  /* Count and list bound lights. */
  j = 0;
  for (i=0; i<emberAfPluginRf4ceZrc20HaLogicalDeviceDestinationTableSize(); i++) {
    GetLogicalDeviceDestination(i, &dest);
    if (dest.type != 0xFF) {
      j++;
      emberAfCorePrintln("(%d)        Select HA light %d", j, j);
    }
  }
  if (!j) {   /* We should never get here. */
    emberAfCorePrintln("(****)     No light bound!");
  }
#ifndef EZSP_HOST
  emberAfCorePrintln("(%d)        Print RF4CE network info", j+1);
#endif
  emberAfCorePrintln("(MENU)     Re-open this MENU");
  emberAfCorePrintln("-----------------------------------");
  emberAfCorePrintln("");
}

static void printHALightMapDialogMenu(void)
{
  emberAfCorePrintln("-----------------------------------");
  emberAfCorePrintln("RF4CE/HA MAIN dialog:");
  emberAfCorePrintln("-----------------------------------");
  emberAfCorePrintln("");
  emberAfCorePrintln("(****) Select instance to map: 0-31");
#ifndef EZSP_HOST
  emberAfCorePrintln("");
#endif
  emberAfCorePrintln("(MENU)     Re-open this MENU");
  emberAfCorePrintln("-----------------------------------");
  emberAfCorePrintln("");
}

static void printHAControlDialogMenu(void)
{
  emberAfCorePrintln("----------------------------------------------------");
  emberAfCorePrintln("RF4CE/HA MAIN dialog:");
  emberAfCorePrintln("----------------------------------------------------");
  emberAfCorePrintln("(1)        Print most recent light status");
  emberAfCorePrintln("(2)        Turn the light ON");
  emberAfCorePrintln("(3)        Turn the light OFF");
  emberAfCorePrintln("(4)        TOGGLE the light");
  emberAfCorePrintln("(5)        Leave the HA network");
  emberAfCorePrintln("(6)        Set light querying time");
  emberAfCorePrintln("(7)        Enable/disable auto-toggling the light");
#ifndef EZSP_HOST
  emberAfCorePrintln("(8)        Print RF4CE network info");
#endif
  emberAfCorePrintln("(MENU)     Re-open this MENU");
  emberAfCorePrintln("----------------------------------------------------");
  emberAfCorePrintln("");
}

static void printHALightQueryingTimeDialogMenu(void)
{
  emberAfCorePrintln("------------------------------------");
  emberAfCorePrintln("HA light QUERYING TIME dialog:");
  emberAfCorePrintln("------------------------------------");
  emberAfCorePrintln("(1)        200 milliseconds");
  emberAfCorePrintln("(2)        500 milliseconds");
  emberAfCorePrintln("(3)        1 second (default)");
  emberAfCorePrintln("(4)        2 seconds");
  emberAfCorePrintln("(5)        5 seconds");
  emberAfCorePrintln("(6)        10 seconds");
  emberAfCorePrintln("(7)        60 seconds");
  emberAfCorePrintln("(8)        Don't query the light");
  emberAfCorePrintln("(MENU)     Go back to MAIN menu");
  emberAfCorePrintln("------------------------------------");
  emberAfCorePrintln("");
}

static void printHALightAutoTogglingDialogMenu(void)
{
  emberAfCorePrintln("------------------------------------");
  emberAfCorePrintln("HA light AUTO-TOGGLING dialog:");
  emberAfCorePrintln("------------------------------------");
  emberAfCorePrintln("(1)        500 milliseconds");
  emberAfCorePrintln("(2)        1 second");
  emberAfCorePrintln("(3)        2 seconds");
  emberAfCorePrintln("(4)        5 seconds");
  emberAfCorePrintln("(5)        10 seconds");
  emberAfCorePrintln("(6)        Disable auto-toggling");
  emberAfCorePrintln("(MENU)     Go back to MAIN menu");
  emberAfCorePrintln("------------------------------------");
  emberAfCorePrintln("");
}

static void printLightStatus(void)
{
  emberAfCorePrintln("------------------------");
  emberAfCorePrintln("Light status: %p", (theirOnOff ? "ON" : "OFF"));
  emberAfCorePrintln("------------------------");
  emberAfCorePrintln("");
}

static bool bindingMatches(void)
{
  EmberBindingTableEntry incomingBinding, theirBinding;
  uint8_t incomingBindingIndex = emberAfGetBindingIndex();
  return (incomingBindingIndex == theirBindingIndex
          || ((emberGetBinding(incomingBindingIndex, &incomingBinding)
               == EMBER_SUCCESS)
              && (emberGetBinding(theirBindingIndex, &theirBinding)
                  == EMBER_SUCCESS)
              && incomingBinding.type == theirBinding.type
              && incomingBinding.local == theirBinding.local
              && incomingBinding.remote == theirBinding.remote
              && (MEMCOMPARE(incomingBinding.identifier,
                             theirBinding.identifier,
                             EUI64_SIZE)
                  == 0)
              && incomingBinding.networkIndex == theirBinding.networkIndex));
}

static void SendOnOffAttributeReadCommand(void)
{
  DestStruct dest;
  EmberStatus status;

  uint8_t attributeIds[] = {
    LOW_BYTE(ZCL_ON_OFF_ATTRIBUTE_ID), HIGH_BYTE(ZCL_ON_OFF_ATTRIBUTE_ID),
  };
  uint16_t bytes =
    emberAfFillExternalBuffer((ZCL_PROFILE_WIDE_COMMAND
                               | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER
                               | ZCL_DISABLE_DEFAULT_RESPONSE_MASK),
                              ZCL_ON_OFF_CLUSTER_ID,
                              ZCL_READ_ATTRIBUTES_COMMAND_ID,
                              "b",
                              attributeIds,
                              sizeof(attributeIds));

  if (bytes == 0) {
    status = EMBER_MESSAGE_TOO_LONG;
  } else {
    emberAfGetCommandApsFrame()->options = ((useApsAcks)
                                            ? EMBER_APS_OPTION_RETRY
                                            : EMBER_APS_OPTION_NONE);
    if (EMBER_SUCCESS ==
            (status = emberAfPluginRf4ceZrc20HaLogicalDeviceDestinationGet(theirPairingIndex,
                                                                           theirHaInstanceId,
                                                                           &dest))) {
        status = emberAfSendCommandUnicast(EMBER_OUTGOING_VIA_BINDING,
                                           dest.indexOrDestination);
    } else {
        emberAfCorePrintln("Binding index not found");
    }
  }
  //if (status != EMBER_SUCCESS) {
  //  warning("Query failed", status);
  //}
  emberAfCorePrintln("ON/OFF attribute read sent: 0x%x", status);
}

static void controlLight(uint8_t commandId)
{
  EmberStatus status;
  uint16_t bytes;

  if (commandId == ZCL_ON_COMMAND_ID) {
    emberAfCorePrintln("Turning the light ON...");
  } else if (commandId == ZCL_OFF_COMMAND_ID) {
    emberAfCorePrintln("Turning the light OFF...");
  } else {
    emberAfCorePrintln("Toggling the light...");
  }
  emberAfCorePrintln("");

  emberAfPushNetworkIndex(EMBER_AF_NETWORK_INDEX_PRO);
  bytes = emberAfFillExternalBuffer((ZCL_CLUSTER_SPECIFIC_COMMAND
                                     | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER
                                     | ZCL_DISABLE_DEFAULT_RESPONSE_MASK),
                                    ZCL_ON_OFF_CLUSTER_ID,
                                    commandId,
                                    "");
  if (bytes == 0) {
    status = EMBER_MESSAGE_TOO_LONG;
  } else {
    emberAfGetCommandApsFrame()->options = ((useApsAcks)
                                            ? EMBER_APS_OPTION_RETRY
                                            : EMBER_APS_OPTION_NONE);
    status = emberAfSendCommandUnicast(EMBER_OUTGOING_VIA_BINDING,
                                       theirBindingIndex);
  }
  emberAfPopNetworkIndex();

  if (status != EMBER_SUCCESS) {
    error("Light command send failed", status);
  }
}


static void AmRemap(void)
{
  EmberStatus status;

  /* Remap TV's mute button to record. */
  EmberAfRf4ceZrcMappableAction ma = {
      .actionDeviceType = EMBER_AF_RF4CE_DEVICE_TYPE_TELEVISION,
      .actionBank = EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC,
      .actionCode = EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_MUTE };
  uint8_t ad[] = { 0, /* action payload */
                 EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC,
                 EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_RECORD };
  uint8_t ic[] = { 0x80, 0x81, 0x82, 0x84 };
  EmberAfRf4ceZrcActionMapping am = {
      .mappingFlags = EMBER_AF_RF4CE_ZRC_ACTION_MAPPING_MAPPING_FLAGS_RF_SPECIFIED_BIT
                      | EMBER_AF_RF4CE_ZRC_ACTION_MAPPING_MAPPING_FLAGS_IR_SPECIFIED_BIT
                      | EMBER_AF_RF4CE_ZRC_ACTION_MAPPING_MAPPING_FLAGS_RF_DESCRIPTOR_FIRST_BIT,
      .rfConfig = 0x01 /* minimum number of transmissions */
                  | RF4CE_ZRC_ACTION_MAPPING_RF_CONFIG_KEEP_TRANSMITTING_UNTIL_KEY_RELEASE
                  | RF4CE_ZRC_ACTION_MAPPING_RF_CONFIG_ATOMIC_ACTION,
      .rf4ceTxOptions = 0xEE,
      .actionDataLength = sizeof(ad),
      .actionData = ad,
      .irConfig = RF4CE_ZRC_ACTION_MAPPING_IR_CONFIG_VENDOR_SPECIFIC,
      .irVendorId = 0xBBCC,
      .irCodeLength = sizeof(ic),
      .irCode = ic };

  status = emberAfRf4ceZrc20ActionMappingServerRemapAction(&ma,   /* mappable action */
                                                           &am);  /* action mapping */
  emberAfAppPrintln("%p 0x%x", "remap mute action", status);
}

static void AmUnmap(void)
{
  EmberStatus status;

  /* Unmap TV's mute button to record. */
  EmberAfRf4ceZrcMappableAction ma = {
      .actionDeviceType = EMBER_AF_RF4CE_DEVICE_TYPE_TELEVISION,
      .actionBank = EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC,
      .actionCode = EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_MUTE };

  status = emberAfRf4ceZrc20ActionMappingServerRestoreDefaultAction(&ma);
  emberAfAppPrintln("%p 0x%x", "unmap mute action", status);
}

static void haJoin(void)
{
    EmberStatus status;

    if (haNetworkState != STATE_HA_NETWORK_DOWN) {
      emberAfCorePrintln("Node already joined to the HA network");
      return;
    }

    emberAfPushNetworkIndex(EMBER_AF_NETWORK_INDEX_PRO);
    status = emberAfStartSearchForJoinableNetwork();
    if (status == EMBER_SUCCESS) {
      haNetworkState = STATE_HA_NETWORK_JOINING;
      info("Searching for joinable network");
    } else {
      error("Error searching for joinable networks", status);
    }
    emberAfPopNetworkIndex();
}

static void haLeave(void)
{
    EmberStatus status;

    if (haNetworkState != STATE_HA_NETWORK_JOINED_LIGHT_NOT_PAIRED
        && haNetworkState != STATE_HA_NETWORK_JOINED_LIGHT_PAIRED
        && haNetworkState != STATE_HA_NETWORK_JOINED_LIGHT_PAIRED_MAPPED) {
      emberAfCorePrintln("Node not joined to the HA network");
      return;
    }

    emberAfPushNetworkIndex(EMBER_AF_NETWORK_INDEX_PRO);
    status = emberLeaveNetwork();
    if (status == EMBER_SUCCESS) {
      info("Leaving network");
    } else {
      error("Error leaving network", status);
    }
    emberAfPopNetworkIndex();
}

static void haFindLight(void)
{
    uint8_t endpoint = emberAfPrimaryEndpoint();
    EmberStatus status;

    emberAfPushNetworkIndex(EMBER_AF_NETWORK_INDEX_PRO);
    status = emberAfEzmodeClientCommission(endpoint,
                                           EMBER_AF_EZMODE_COMMISSIONING_CLIENT_TO_SERVER,
                                           appClusterIds,
                                           COUNTOF(appClusterIds));
    emberAfAppPrintln("%p 0x%x", "client", status);
    emberAfPopNetworkIndex();
}

