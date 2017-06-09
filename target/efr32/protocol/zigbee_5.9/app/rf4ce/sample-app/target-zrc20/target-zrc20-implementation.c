// Copyright 2014 Silicon Laboratories, Inc.

#include PLATFORM_HEADER
#include CONFIGURATION_HEADER
#include EMBER_AF_API_STACK
#include EMBER_AF_API_HAL
#ifdef EMBER_AF_API_DEBUG_PRINT
  #include EMBER_AF_API_DEBUG_PRINT
#endif
#include EMBER_AF_API_RF4CE_PROFILE
#include EMBER_AF_API_RF4CE_GDP
#include EMBER_AF_API_RF4CE_ZRC20

// This application is event driven and the following events are used to
// perform operations like starting network operations, validating the binding
// procedure, and transmitting a power key press.
EmberEventControl networkEventControl;
EmberEventControl bindingEventControl;
EmberEventControl powerEventControl;

// The application plays tunes to provide status to the user.  A rising two-
// tone tune indicates success while a falling two-tone tune indicates failure.
// A single note indicates that an operation has begun.
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

// When the binding button is pressed, the push button state is set to validate
// the current or future binding attempt.  An LED is illuminated during any
// binding attempt to provide status to the user.
#define BINDING_BUTTON BUTTON0
#define BINDING_LED    BOARDLED2

// When the power button is pressed, power saving mode will be toggled.  When
// power saving mode is disabled, the receiver will be enabled.  When power
// saving mode is enabled, the receiver will duty cycle, remaining on for 250
// milliseconds every second.
#define POWER_BUTTON     BUTTON1
#define DUTY_CYCLE_MS    1000
#define ACTIVE_PERIOD_MS 250
static bool powerSavingEnabled = true;
static void setPowerSaving(bool enabled);
#define enablePowerSaving()  setPowerSaving(true)
#define disablePowerSaving() setPowerSaving(false)
#define togglePowerSaving()  setPowerSaving(!powerSavingEnabled)

void emberAfMainInitCallback(void)
{
  // During startup, the Main plugin will attempt to resume network operations
  // based on information in non-volatile memory.  This is known as a warm
  // start.  If a warm start is possible, the stack status handler will be
  // called during initialization with an indication that the network is up.
  // Otherwise, during a cold start, the device comes up without a network.  We
  // always want a network, but we don't know whether we will be doing a cold
  // start and therefore need to start operations ourselves or if we will be
  // doing a warm start and won't have to do anything.  To handle this
  // uncertainty, we set the network event to active, which will schedule it to
  // fire in the next iteration of the main loop, immediately after
  // initialization.  If we receive a stack status notification indicating that
  // the network is up before the event fires, we know we did a warm start and
  // have nothing to do and can therefore cancel the event.  If the event does
  // fire, we know we did a cold start and need to start network operations
  // ourselves.  This logic is handled here and in the stack status and network
  // event handlers below.
  emberEventControlSetActive(networkEventControl);
}

void emberAfStackStatusCallback(EmberStatus status)
{
  // When the network comes up, we immediately enable power saving mode because
  // we expect to start in standby mode.  We also cancel the network event
  // because we know we did a warm start, as described above.  If the network
  // goes down, we use the same network event to start network operations again
  // as soon as possible.
  if (status == EMBER_NETWORK_UP) {
    enablePowerSaving();
    emberEventControlSetInactive(networkEventControl);
    halPlayTune_P(happyTune, true);
  } else if (status == EMBER_NETWORK_DOWN
             && emberNetworkState() == EMBER_NO_NETWORK) {
    emberEventControlSetActive(networkEventControl);
  }
}

void halButtonIsr(uint8_t button, uint8_t state)
{
  // When the binding or power buttons are pressed, we set the corresponding
  // event to active.  This causes the event to fire in the next iteration of
  // the main loop.  This approach is used because button indications are
  // received in an interrupt, so the application must not perform any non-
  // trivial operations here.
  if (button == BINDING_BUTTON && state == BUTTON_PRESSED) {
    emberEventControlSetActive(bindingEventControl);
  } else if (button == POWER_BUTTON && state == BUTTON_PRESSED) {
    emberEventControlSetActive(powerEventControl);
  }
}

bool emberAfPluginIdleSleepOkToSleepCallback(uint32_t durationMs)
{
  // By returning false here, we will prevent the device from sleeping.  This
  // is necessary to ensure that the CLI is usable for demonstration and
  // testing purposes.  A real target could sleep during the inactive period of
  // its duty cycle.
  return false;
}

void emberAfPluginRf4ceGdpStartValidationCallback(uint8_t pairingIndex)
{
  // This function is called to begin the validation procedure for a binding
  // attempt.  We simply indicate the attempt to the user and wait for a button
  // push to confirm validation or for a timeout.
  emberAfCorePrintln("Binding %p: 0x%x", "attempt", pairingIndex);
  halPlayTune_P(waitTune, true);
  halSetLed(BINDING_LED);
}

bool emberAfPluginRf4ceGdpIncomingBindProxyCallback(const EmberEUI64 ieeeAddr)
{
  // This function is called to accept a proxy binding attempt.  We always
  // accept proxy bindings after indicating the attempt to the user.
  emberAfCorePrintln("Proxy binding %p", "attempt");
  halPlayTune_P(waitTune, true);
  halSetLed(BINDING_LED);
  return true;
}

void emberAfPluginRf4ceGdpBindingCompleteCallback(EmberAfRf4ceGdpBindingStatus status,
                                                  uint8_t pairingIndex)
{
  // When the binding process completes, we receive notification of the success
  // or failure of the operation.
  if (status == EMBER_SUCCESS) {
    emberAfCorePrintln("Binding %p: 0x%x", "complete", pairingIndex);
    halPlayTune_P(happyTune, true);
  } else {
    emberAfCorePrintln("Binding %p: 0x%x", "failed", status);
    halPlayTune_P(sadTune, true);
  }
  halClearLed(BINDING_LED);
}

void emberAfPluginRf4ceZrc20ActionCallback(const EmberAfRf4ceZrcActionRecord *record)
{
  // When any actions occur, we display the information.
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

  // When HDMI-CEC power commands are received, we enable, disable, or toggle
  // power saving mode.  This allows controllers to manipulate our power
  // between standby and on via over-the-air messages.
  if ((record->actionType == EMBER_AF_RF4CE_ZRC_ACTION_TYPE_START
       || record->actionType == EMBER_AF_RF4CE_ZRC_ACTION_TYPE_REPEAT
       || record->actionType == EMBER_AF_RF4CE_ZRC_ACTION_TYPE_ATOMIC)
      && record->actionBank == EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC
      && record->actionVendorId == EMBER_RF4CE_NULL_VENDOR_ID) {
    switch (record->actionCode) {
    case EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_POWER:
    case EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_POWER_TOGGLE_FUNCTION:
      togglePowerSaving();
      break;
    case EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_POWER_OFF_FUNCTION:
      enablePowerSaving();
      break;
    case EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_POWER_ON_FUNCTION:
      disablePowerSaving();
      break;
    default:
      break;
    }
  }
}

void networkEventHandler(void)
{
  // The network event is scheduled during initialization to handle cold starts
  // and also if the network ever goes down.  In either situation, we want to
  // start network operations.  If the call succeeds, we are done and just wait
  // for the inevitable stack status notification.  If it fails, we set the
  // event to try again right away.
  if (emberAfRf4ceStart() == EMBER_SUCCESS) {
    emberEventControlSetInactive(networkEventControl);
    halPlayTune_P(waitTune, true);
  } else {
    emberEventControlSetActive(networkEventControl);
    halPlayTune_P(sadTune, true);
  }
}

void bindingEventHandler(void)
{
  // The binding event is scheduled whenever the binding button is pressed.  We
  // set the push button state to validate the current binding attempt or to
  // automatically validate a future binding attempt.  The push button state is
  // cleared automatically by the GDP plugin after some time, so the binding
  // button needs to be pressed for each binding attempt.
  emberAfRf4ceGdpPushButton(true);
  halPlayTune_P(waitTune, true);
  emberEventControlSetInactive(bindingEventControl);
}

void powerEventHandler(void)
{
  // The power event is scheduled whenever the power button is pressed.  We
  // simply toggle the power saving state in response.
  togglePowerSaving();
  emberEventControlSetInactive(powerEventControl);
}

static void setPowerSaving(bool enabled)
{
  // The power saving state can be changed by a physical button push or via
  // over-the-air messages from controllers.  Setting the receiver to enabled
  // will leave it on until further notice and allow the device to receive
  // messages.  Enabling power saving mode will duty cycle the receiver so that
  // it is on for each active period and then off until the next cycle starts.
  // Because of MAC- and application-level retries, it is still possible to
  // receive messages while duty cycling if the active period is reasonable.
  powerSavingEnabled = enabled;
  if (powerSavingEnabled) {
    emberAfCorePrintln("Power: %p", "standby");
    emberAfRf4ceSetPowerSavingParameters(DUTY_CYCLE_MS, ACTIVE_PERIOD_MS);
  } else {
    emberAfCorePrintln("Power: %p", "on");
    emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_WILDCARD, true);
  }
}
