//

// This callback file is created for your convenience. You may add application
// code to this file. If you regenerate this file over a previous version, the
// previous version will be overwritten and any code you have added will be
// lost.

#include "app/framework/include/af.h"

#include EMBER_AF_API_NETWORK_CREATOR
#include EMBER_AF_API_NETWORK_CREATOR_SECURITY

#define CENTRALIZED_NETWORK_ENDPOINT (1)

#ifdef EZSP_HOST
  static uint16_t transientKeyTimeoutMS(void)
  {
    uint16_t timeoutS;
    ezspGetConfigurationValue(EZSP_CONFIG_TRANSIENT_KEY_TIMEOUT_S, &timeoutS);
    return (timeoutS * MILLISECOND_TICKS_PER_SECOND);
  }
#else
  #define transientKeyTimeoutMS() \
    (emberTransientKeyTimeoutS * MILLISECOND_TICKS_PER_SECOND)
#endif

static uint32_t networkOpenTimeMS = 0, networkCloseTimeMS = 0;

EmberEventControl commissioningEventControl;
EmberEventControl ledEventControl;
static uint8_t mostRecentButton;

void commissioningEventHandler(void)
{
  EmberStatus status;
  
  emberEventControlSetInactive(commissioningEventControl);

  if (emberAfNetworkState() == EMBER_JOINED_NETWORK) {
    if (mostRecentButton == BUTTON0) {
      status = emberAfPluginNetworkCreatorSecurityOpenNetwork();
      emberAfCorePrintln("%p network: 0x%X", "Open", status);
      if (status == EMBER_SUCCESS) {
        networkOpenTimeMS = halCommonGetInt32uMillisecondTick();
        networkCloseTimeMS = networkOpenTimeMS + transientKeyTimeoutMS();
        emberEventControlSetDelayMS(ledEventControl, LED_BLINK_PERIOD_MS << 1);
      }
    } else if (mostRecentButton == BUTTON1) {
      status = emberAfPluginNetworkCreatorSecurityCloseNetwork();
      networkOpenTimeMS = networkCloseTimeMS = 0;
      emberAfCorePrintln("%p network: 0x%X", "Close", status);
    }
  } else {
    status = emberAfPluginNetworkCreatorStart(true); // centralized
    emberAfCorePrintln("%p network %p: 0x%X",
                       "Form centralized",
                       "start",
                       status);
  }
}

void ledEventHandler(void)
{
  emberEventControlSetInactive(ledEventControl);

  if (emberAfNetworkState() == EMBER_JOINED_NETWORK) {
    uint32_t now = halCommonGetInt32uMillisecondTick();
    if (((networkOpenTimeMS < networkCloseTimeMS)
         && (now > networkOpenTimeMS && now < networkCloseTimeMS))
        || ((networkCloseTimeMS < networkOpenTimeMS)
            && (now > networkOpenTimeMS || now < networkCloseTimeMS))) {
      // The network is open.
      halToggleLed(COMMISSIONING_STATUS_LED);
    } else {
      // The network is closed.
      halSetLed(COMMISSIONING_STATUS_LED);
      networkOpenTimeMS = networkCloseTimeMS = 0;
    }
    emberEventControlSetDelayMS(ledEventControl, LED_BLINK_PERIOD_MS << 1);
  } else {
    halClearLed(COMMISSIONING_STATUS_LED);
  }
}

/** @brief Hal Button Isr
 *
 * This callback is called by the framework whenever a button is pressed on the
 * device. This callback is called within ISR context.
 *
 * @param button The button which has changed state, either BUTTON0 or BUTTON1
 * as defined in the appropriate BOARD_HEADER.  Ver.: always
 * @param state The new state of the button referenced by the button parameter,
 * either ::BUTTON_PRESSED if the button has been pressed or ::BUTTON_RELEASED
 * if the button has been released.  Ver.: always
 */
void emberAfHalButtonIsrCallback(uint8_t button,
                                 uint8_t state)
{
  if (state == BUTTON_RELEASED) {
    mostRecentButton = button;
    emberEventControlSetActive(commissioningEventControl);
  }
}


/** @brief Complete
 *
 * This callback notifies the user that the network creation process has
 * completed successfully.
 *
 * @param network The network that the network creator plugin successfully
 * formed. Ver.: always
 * @param usedSecondaryChannels Whether or not the network creator wants to
 * form a network on the secondary channels Ver.: always
 */
void emberAfPluginNetworkCreatorCompleteCallback(const EmberNetworkParameters *network,
                                                 bool usedSecondaryChannels)
{
  emberAfCorePrintln("%p network %p: 0x%X",
                     "Form centralized",
                     "complete",
                     EMBER_SUCCESS);

  emberEventControlSetActive(ledEventControl);
}
