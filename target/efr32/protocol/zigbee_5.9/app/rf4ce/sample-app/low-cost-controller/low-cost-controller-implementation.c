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
#include EMBER_AF_API_RF4CE_ZRC20_ACTION_MAPPING_CLIENT
#include EMBER_AF_API_KEY_MATRIX
#include EMBER_AF_API_INFRARED_LED
#include EMBER_AF_API_INFRARED_LED_UIRD

// Bits specifying that an action mapping has vendor specific IR
#define ZRC_IR_CONFIG_VENDOR_SPECIFIC   0x01

// When using encrypted UIRD IR codesets, uncomment the define below with the
// key inserted. The key is given by the IR database provider.
//#define UIRD_DECRYPTION_KEY 0xABABABAB

#ifdef UIRD_DECRYPTION_KEY
  #define SET_UIRD_DECRYPT_KEY() halInfraredLedUirdSetDecryptKey(UIRD_DECRYPTION_KEY)
#else
  #define SET_UIRD_DECRYPT_KEY()
#endif

#if DEVICE_COUNT != 2
  #error "Application assumes two devices. Modify code to handle other number."
#endif

// Struct to hold the current action configuration for a key, depending on
// whether it is set to use the default config or the config from an action
// mapping.
typedef struct
{
  uint8_t  rfSpecified;
  uint8_t  actionBank;
  uint8_t  actionCode;
  uint8_t  atomic;
  uint8_t  irSpecified;
  uint16_t irFormat;
  uint8_t  irCodeLength;
  uint8_t *irCode;
} TxConfig;

// This application is event driven and the following events are used
EmberEventControl networkEventControl;
EmberEventControl bindingEventControl;
EmberEventControl keymatrixEventControl;

// Prototypes
static void keyPressedHandler(uint8_t key);
static void keyReleasedHandler(uint8_t key);
static void decodeActionForKey(uint8_t key, TxConfig *txConfig);

// This table keeps track of what each device (TV or STB) is paired to.
static uint8_t pairingDeviceTable[DEVICE_COUNT] = {
  NO_PAIRING,
  NO_PAIRING
};

// This table keeps track of what devices we haven't yet tried to negotiate
// action mappings for after a reboot
static bool devicesPendingAmNegotiation[DEVICE_COUNT] = {
  true,
  true
};

static const uint8_t deviceType[DEVICE_COUNT] = {
  EMBER_AF_RF4CE_DEVICE_TYPE_SET_TOP_BOX,
  EMBER_AF_RF4CE_DEVICE_TYPE_TELEVISION
};

// State of keys during last scan
static bool lastKeyStates[KEY_MATRIX_NUM_KEYS]; // Initialized to 0s
// Which device mode is active (STB or TV). Device mode is changed with TV
// or STB keys
static uint8_t   activeDevice          = DEVICE_STB;

static bool anyKeyDown            = false;
static bool bindingActive         = false;
static bool pairingKeyPressed     = false;
static bool stbKeyPressed         = false;
static bool tvKeyPressed          = false;

// Macros to handle starting and stopping of RF action code messages
#define START_RF_MESSAGE(actionBank, actionCode, atomic)                   \
    do {emberAfRf4ceZrc20ActionStart(pairingDeviceTable[activeDevice],     \
                                     actionBank,                           \
                                     actionCode,                           \
                                     EMBER_AF_RF4CE_ZRC_MODIFIER_BIT_NONE, \
                                     EMBER_RF4CE_NULL_VENDOR_ID,           \
                                     NULL,    /* action data */            \
                                     0,       /* action data length */     \
                                     atomic); /* atomic */                 \
    } while (0)

#define STOP_RF_MESSAGE(actionBank, actionCode)                            \
    do {emberAfRf4ceZrc20ActionStop(pairingDeviceTable[activeDevice],      \
                                    actionBank,                            \
                                    actionCode,                            \
                                    EMBER_AF_RF4CE_ZRC_MODIFIER_BIT_NONE,  \
                                    EMBER_RF4CE_NULL_VENDOR_ID);           \
    } while (0)

void emberAfMainInitCallback(void)
{
  uint8_t device;
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
  // ourselves. This logic is handled here and in the stack status and network
  // event handlers below.

  // The LEDs are excluded from the board header power-up/power-down GPIO
  // settings as we want them to keep their state when transitioning in and out
  // of sleep. Because of this we need to manually set the LEDs to their wanted
  // initial state out of reset.
  halClearLed(GREEN_LED);
  halClearLed(RED_LED);

  emberEventControlSetActive(networkEventControl);
  emberEventControlSetActive(keymatrixEventControl);

  // Read back paired devices from token
  // The pairingDeviceTable holds information about which pairing index is used
  // for the two devices (STB or TV). The entries are set to NO_PAIRING
  // until the controller has been paired to a target in that device mode.
  for (device = 0; device<DEVICE_COUNT; device++) {
    halCommonGetIndexedToken(&pairingDeviceTable[device],
                             TOKEN_PAIRING_DEVICE_TABLE,
                             device);
  }

  // If we already have pairings we try to start action mapping negotiations
  // with them to get any action mappings that we lost during reboot
  // Once a negotiation has been started with one index we will have to wait
  // for it to finish before we can trigger the next one. This is handled
  // in emberAfPluginRf4ceZrc20ActionMappingsNegotiationCompleteCallback().
  for (device = 0; device<DEVICE_COUNT; device++) {
    if (pairingDeviceTable[device] != NO_PAIRING) {
        emberAfRf4ceZrc20StartActionMappingsNegotiation(pairingDeviceTable[device]);
        devicesPendingAmNegotiation[device] = false;
        break;
    }
  }

  // Write decryption key for UIRD IR format if a key is provided.
  SET_UIRD_DECRYPT_KEY();
}

void emberAfPluginRf4ceZrc20ActionMappingsNegotiationCompleteCallback(EmberStatus status)
{
  // We go through the valid pairing indexes that still have pending action
  // mapping negotiations since last reboot and trigger AM negotions for the
  // first one. When the first one has been triggered the next index will have
  // to wait until the next callback.
  uint8_t device;
  for (device = 0; device<DEVICE_COUNT; device++) {
    if ((pairingDeviceTable[device] != NO_PAIRING)
        && (devicesPendingAmNegotiation[device])){
        emberAfRf4ceZrc20StartActionMappingsNegotiation(pairingDeviceTable[device]);
        devicesPendingAmNegotiation[device] = false;
        break;
        }
  }
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
  } else if (status == EMBER_NETWORK_DOWN
             && emberNetworkState() == EMBER_NO_NETWORK) {
    emberEventControlSetActive(networkEventControl);
  }
}

bool emberAfPluginIdleSleepOkToSleepCallback(uint32_t durationMs)
{
  // This function is called by the Idle/Sleep plug-in to ask the application
  // if it is safe to sleep or not.
  if (!anyKeyDown) {
    halKeyMatrixPrepareForSleep();
    return true;
  } else {
    return false;
  }
}

void emberAfPluginIdleSleepWakeUpCallback(uint32_t durationMs)
{
  // This function is called by the Idle/Sleep plug-in after sleeping.

  // Check if any key is down, then scan through all keys to figure out which
  // keys are held down
  if (halKeyMatrixAnyKeyDown()) {
    halKeyMatrixRestoreFromSleep();

    // Send event to kick off key polling
    emberEventControlSetActive(keymatrixEventControl);
  }
}

void emberAfPluginRf4ceGdpBindingCompleteCallback(EmberAfRf4ceGdpBindingStatus status,
                                                  uint8_t pairingIndex)
{
  // When the binding process completes, we receive notification of the success
  // or failure of the operation.

  if (status == EMBER_SUCCESS) {
    emberAfAppPrintln("%p %p: 0x%x", "Binding", "complete", pairingIndex);

    // We store the pairing index for the active device
    pairingDeviceTable[activeDevice] = pairingIndex;

    // If the other device is not paired to anything we use the same pairing
    // index for this device as well. This simplifies systems where you only
    // have one target, but want to update action mappings for both pairing IDs.
    // Otherwise the user would have to pair both devices with the same target
    // manually.
    if (pairingDeviceTable[(activeDevice+1)%2] == NO_PAIRING) {
      pairingDeviceTable[(activeDevice+1)%2] = pairingIndex;
      halCommonSetIndexedToken(TOKEN_PAIRING_DEVICE_TABLE,
                             (activeDevice+1)%2,
                             &pairingDeviceTable[(activeDevice+1)%2]);
    }

  } else {
    emberAfAppPrintln("%p %p: 0x%x", "Binding", "failed", status);

    // We set the pairing index for the active device to NO_PAIRING
    // as the pairing entry for this index was deleted at the beginning
    // of the pairing and the pairing attempt did not provide a new pairing
    // entry
    pairingDeviceTable[activeDevice]=NO_PAIRING;
  }

  // Store pairing index for current device to token
  halCommonSetIndexedToken(TOKEN_PAIRING_DEVICE_TABLE,
                             activeDevice,
                             &pairingDeviceTable[activeDevice]);

  bindingActive = false;
  halClearLed(RED_LED);
}

void networkEventHandler(void)
{
  // The network event is scheduled during initialization to handle cold starts
  // and also if the network ever goes down.  In either situation, we want to
  // start network operations.  If the call succeeds, we are done and just wait
  // for the inevitable stack status notification.  If it fails, we set the
  // event to try again right away.

  if (emberAfRf4ceStart() == EMBER_SUCCESS){
    emberEventControlSetInactive(networkEventControl);
  } else {
    emberEventControlSetActive(networkEventControl);
  }
}

void bindingEventHandler(void)
{
  // When this function is called we do a basic discovery for any type of ZRC1.x
  // or ZRC2.0 device and hopefully find something to bind with.  If the call
  // succeeds, we will get a binding complete notification with the results.
  //
  // If there already is a pairing entry for the device we need to delete
  // it first to leave room for a new pairing entry. If both devices are using
  // the same pairing entry we cannot delete this entry as the other device is
  // still using it. Instead we can then delete the other pairing entry which
  // none of the devices then no longer point to.

  emberEventControlSetInactive(bindingEventControl);

  if (pairingDeviceTable[activeDevice] != NO_PAIRING) {
    // If there already is a pairing index stored for the current device
    // we need to delete an old table entry to leave room for a new entry before
    // attempting to bind.
    if ( pairingDeviceTable[DEVICE_TV] != pairingDeviceTable[DEVICE_STB]) {
      // If the TV and STB device are using different pairing
      // indexes, we need to delete the current device pairing index.
      emberAfRf4ceSetPairingTableEntry(pairingDeviceTable[activeDevice], NULL);
    } else {
      // If the STB mode and TV mode are both using the same pairing index, we
      // delete the pairing index that is not used by any of the devices as this
      // may be used for the new pairing (unless pairing to the same index
      // again).
      emberAfRf4ceSetPairingTableEntry((pairingDeviceTable[activeDevice]+1)%2,
                                       NULL);
    }
  }

  // Attempt to bind to a target
  if (emberAfRf4ceZrc20Bind(EMBER_AF_RF4CE_DEVICE_TYPE_WILDCARD)
      == EMBER_SUCCESS) {
    emberAfAppPrintln("%p to %p", "Binding", activeDevice?"TV":"STB");
    bindingActive = true;
    halSetLed(RED_LED);
  }
}

void keymatrixEventHandler(void)
{
  // This function is called periodically when not sleeping to scan through all
  // keys and perform appropriate actions based on changes in the status of the
  // keys.

  uint32_t key;
  uint8_t keyState;

  emberEventControlSetInactive(keymatrixEventControl);

  anyKeyDown = false;

  // Call this to scan the key-matrix and update key value memory array
  halKeyMatrixScan();

  // Read out key values
  for (key = 0; key < KEY_MATRIX_NUM_KEYS; key++) {
    keyState = halKeyMatrixGetValue(key);

    // Compare last key values against the new key values
    // and sort out which keys have changed (press, release)
    // since the last scan.
    if (keyState) {
      anyKeyDown = true;
      if (!lastKeyStates[key]) {
        keyPressedHandler(key);
      }
   } else {
      if (lastKeyStates[key]) {
        keyReleasedHandler(key);
      }
    }
    lastKeyStates[key] = keyState;
  } // for key

  // GREEN LED lit while keys are pressed
  if (anyKeyDown) {
    halSetLed(GREEN_LED);
  } else {
    halClearLed(GREEN_LED);
  }

  // Only schedule another scan if a key is down
  if (anyKeyDown) {
    emberEventControlSetDelayMS(keymatrixEventControl, KEY_SCAN_PERIOD_MS);
  }
}

static void keyPressedHandler(uint8_t key)
{
  // This function is used to decode what happens when a is pressed (up to down
  // transition). First the special keys (PAIR, STB, TV) are handled
  // before the regular keys are handled.

  TxConfig txConfig;
  uint8_t keyRfCode = defaultActionsRf[key].code;

  // Check non-mappable special keys

  // Both the PAIR key and the TV or STB key must be down at the same time to
  // initiate pairing to the selected device
  if (key == DBG_KEY_PAIR) {
    emberAfDebugPrintln("PAIR pressed");
    pairingKeyPressed = true;
    if ((stbKeyPressed || tvKeyPressed) && !bindingActive) {
      emberEventControlSetActive(bindingEventControl);
    }
  } else if (key == K_STB) {
    emberAfDebugPrintln("STB pressed");
    stbKeyPressed = true;
    if (!bindingActive) {
      activeDevice = DEVICE_STB;
      emberAfAppPrintln("STB mode");
      if (pairingKeyPressed) {
        emberEventControlSetActive(bindingEventControl);
      }
    }
  } else if (key == K_TV) {
    emberAfDebugPrintln("TV pressed");
    tvKeyPressed = true;
    if (!bindingActive) {
      activeDevice = DEVICE_TV;
      emberAfAppPrintln("TV mode");
      if (pairingKeyPressed) {
        emberEventControlSetActive(bindingEventControl);
      }
    }

  } else if (keyRfCode != 0xFF) {
    // Check for match in regular keys
    emberAfDebugPrintln("Regular key pressed: 0x%x", keyRfCode);

    // Decode what to do when this key is pressed
    decodeActionForKey(key, &txConfig);

    // Start RF if we have a valid pairing and know what to send
    if (txConfig.rfSpecified
        && (pairingDeviceTable[activeDevice] != NO_PAIRING)) {
      START_RF_MESSAGE(txConfig.actionBank,
                       txConfig.actionCode,
                       txConfig.atomic);

      emberAfAppPrintln("RF Start. Bank: 0x%x Code: 0x%x",
                        txConfig.actionBank,
                        txConfig.actionCode);
    }
    if (txConfig.irSpecified) {
      emberAfAppPrintln("IR Start");
      halInfraredLedStart(txConfig.irFormat, txConfig.irCode, txConfig.irCodeLength);
    }
  }
}

static void keyReleasedHandler(uint8_t key)
{
  // This function is used to decode what happens when a is released (down to up
  // transition). First the special keys (PAIR, STB, TV) are handled
  // before the regular keys are handled.

  TxConfig txConfig;
  uint8_t keyRfCode = defaultActionsRf[key].code;

   // Check non-mappable special keys
  if (key == DBG_KEY_PAIR) {
    emberAfDebugPrintln("PAIR released");
    pairingKeyPressed = false;
  } else if (key == K_STB) {
    emberAfDebugPrintln("STB released");
    stbKeyPressed = false;
  } else if (key == K_TV) {
    emberAfDebugPrintln("TV released");
    tvKeyPressed = false;

  } else if (keyRfCode != 0xFF) {
    // Check for match in regular keys
    emberAfDebugPrintln("Regular key released: 0x%x", keyRfCode);

    // Decode what to do when this key is released
    decodeActionForKey(key, &txConfig);

    // Stop RF if not atomic and we have a valid pairing
    if (txConfig.rfSpecified
        && !txConfig.atomic
        && (pairingDeviceTable[activeDevice] != NO_PAIRING)) {
      STOP_RF_MESSAGE(txConfig.actionBank,
                      txConfig.actionCode);

      emberAfAppPrintln("RF Stop. Bank: 0x%x Code: 0x%x",
                        txConfig.actionBank,
                        txConfig.actionCode);
    }
    if (txConfig.irSpecified) {
      emberAfAppPrintln("IR Stop");
      halInfraredLedStop(txConfig.irFormat, txConfig.irCode, txConfig.irCodeLength);
    }
  }
}

static void decodeActionForKey(uint8_t key, TxConfig *txConfig)
{
  // This function used to decode the action that should be taken when a key is
  // pressed or released. It figures out if the key has an action mapping to be
  // used or if the default action is to be used.

  EmberStatus status;
  EmberAfRf4ceZrcActionMapping actionMapping;
  bool useMappedAction;
  uint8_t keyRfCode = defaultActionsRf[key].code;
  uint8_t keyRfConf = defaultActionsRf[key].config;

  status =
    emberAfRf4ceZrc20ActionMappingClientGetActionMapping(pairingDeviceTable[activeDevice],
                                                         deviceType[activeDevice],
                                                         EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC,
                                                         keyRfCode,
                                                         &actionMapping);

  // Check if mapped action or default action should be used
  if (status == EMBER_SUCCESS) {
    if (actionMapping.mappingFlags
        & EMBER_AF_RF4CE_ZRC_ACTION_MAPPING_MAPPING_FLAGS_USE_DEFAULT_BIT) {
      useMappedAction = false;
    } else {
      useMappedAction = true;
    }
  } else {
    useMappedAction = false;
  }

  // Set values in txConfig struct to mapped or default actions.
  if (useMappedAction) {
    txConfig->rfSpecified      = actionMapping.mappingFlags
                                 & EMBER_AF_RF4CE_ZRC_ACTION_MAPPING_MAPPING_FLAGS_RF_SPECIFIED_BIT;
    txConfig->irSpecified      = actionMapping.mappingFlags
                                 & EMBER_AF_RF4CE_ZRC_ACTION_MAPPING_MAPPING_FLAGS_IR_SPECIFIED_BIT;
    // Store RF specific configs
    // Make sure actionMapping is correct length
    if ((txConfig->rfSpecified) && (actionMapping.actionDataLength == 3)) {
      txConfig->actionBank     = EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC;
      txConfig->actionCode     = actionMapping.actionData[2];
      txConfig->atomic         = actionMapping.rfConfig
                                 & RF4CE_ZRC_ACTION_MAPPING_RF_CONFIG_ATOMIC_ACTION;
    } else {
      txConfig->actionBank     = 0;
      txConfig->actionCode     = 0;
      txConfig->atomic         = 0;
    }
    // Store IR specific configs
    if (txConfig->irSpecified) {
      uint16_t format;

      format = ((actionMapping.irConfig & ZRC_IR_CONFIG_VENDOR_SPECIFIC)
                == ZRC_IR_CONFIG_VENDOR_SPECIFIC)
                ? actionMapping.irVendorId
                : HAL_INFRARED_LED_DB_FORMAT_SIRD;
      txConfig->irFormat       = format;
      txConfig->irCodeLength   = actionMapping.irCodeLength;
      txConfig->irCode         = actionMapping.irCode;
    } else {
      txConfig->irCodeLength   = 0;
      txConfig->irCode         = NULL;
    }
  } else {
    // Default action
    txConfig->rfSpecified      = true;
    txConfig->actionBank       = EMBER_AF_RF4CE_ZRC_ACTION_BANK_HDMI_CEC;
    txConfig->actionCode       = keyRfCode;
    txConfig->atomic           = (keyRfConf
                                 & RF4CE_ZRC_ACTION_MAPPING_RF_CONFIG_ATOMIC_ACTION)
                                 == RF4CE_ZRC_ACTION_MAPPING_RF_CONFIG_ATOMIC_ACTION;
    if(activeDevice == DEVICE_TV) {
      txConfig->irSpecified    = defaultActionsIr[key].length != 0;
      txConfig->irFormat       = defaultActionsIr[key].format;
      txConfig->irCodeLength   = defaultActionsIr[key].length;
      txConfig->irCode         = (uint8_t *)defaultActionsIr[key].data;
    } else {
      txConfig->irSpecified    = false;
      txConfig->irFormat       = 0;
      txConfig->irCodeLength   = 0;
      txConfig->irCode         = NULL;
    }
  }
}
