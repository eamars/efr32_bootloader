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
#include EMBER_AF_API_RF4CE_GDP_IDENTIFICATION_CLIENT
#include EMBER_AF_API_RF4CE_ZRC20

// This application is event driven and the following events are used to
// perform operations like starting network operations, initiating the binding
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

// When the binding button is pressed, the binding procedure will be initiated.
// While binding is in progress, an LED is illuminated to provide status to the
// user.
#define BINDING_BUTTON BUTTON0
#define BINDING_LED    BOARDLED2

// When the power button is pressed, a power key press is transmitted to the
// most recently bound target.
#define POWER_BUTTON BUTTON1
static uint8_t powerPairingIndex = 0xFF;

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
  // When the network comes up, we immediately set the default state of the
  // receiver to off because we are a controller device and do not expect
  // unsolicited messages from any of our pairings.  We also cancel the network
  // event because we know we did a warm start, as described above.  If the
  // network goes down, we use the same network event to start network
  // operations again as soon as possible.
  if (status == EMBER_NETWORK_UP) {
    emberAfRf4ceRxEnable(EMBER_AF_RF4CE_PROFILE_WILDCARD, false);
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
  // trivial operations here.  We also always inform the GDP identification
  // client that we detected user interaction when a button is pressed or
  // released.
  emberAfRf4ceGdpIdentificationClientDetectedUserInteraction();
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
  // testing purposes.  A real controller device would typically sleep as often
  // as possible to conserve power.
  return false;
}

void emberAfPluginRf4ceGdpBindingCompleteCallback(EmberAfRf4ceGdpBindingStatus status,
                                                  uint8_t pairingIndex)
{
  // When the binding process completes, we receive notification of the success
  // or failure of the operation.  The power button is always transmitted to
  // the most recently bound target, so we remember its index if the binding
  // was successful.
  if (status == EMBER_SUCCESS) {
    emberAfCorePrintln("%p %p: 0x%x", "Binding", "complete", pairingIndex);
    powerPairingIndex = pairingIndex;
    halPlayTune_P(happyTune, true);
  } else {
    emberAfCorePrintln("%p %p: 0x%x", "Binding", "failed", status);
    halPlayTune_P(sadTune, true);
  }
  halClearLed(BINDING_LED);
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
  // do a basic discovery for any type of ZRC1.x or ZRC2.0 device and hopefully
  // find something to bind with.  If the call succeeds, we will get a binding
  // complete notification with the results.
  if (emberAfRf4ceZrc20Bind(EMBER_AF_RF4CE_DEVICE_TYPE_WILDCARD)
      == EMBER_SUCCESS) {
    halPlayTune_P(waitTune, true);
    halSetLed(BINDING_LED);
  } else {
    halPlayTune_P(sadTune, true);
  }
  emberEventControlSetInactive(bindingEventControl);
}

void powerEventHandler(void)
{
  // The power event is scheduled whenever the power button is pressed.  If we
  // are bound with a target, we transmit a power action to it.  The action is
  // set to atomic, meaning it will not be repeated.  Instead, we count on
  // retransmissions at the MAC layer for ensuring successful delivery.
  // Because the action is not repeated, we do not have to explicitly stop it.
  if (powerPairingIndex != 0xFF) {
    emberAfRf4ceZrc20ActionStart(powerPairingIndex,
                                 EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC,
                                 EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC_ACTION_CODE_POWER,
                                 EMBER_AF_RF4CE_ZRC_MODIFIER_BIT_NONE,
                                 EMBER_RF4CE_NULL_VENDOR_ID,
                                 NULL,  // action data
                                 0,     // action data length
                                 true); // atomic
  }
  emberEventControlSetInactive(powerEventControl);
}
