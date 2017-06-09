// *****************************************************************************
// * security-sensor.c
// *
// * Generic security sensor interface code.
// *
// * This implements an IAS Zone Server security based security device.  It
// * manages a tamper and contact sensor with the tamper-switch and gpio-sensor
// * plugins, and uses connection-manager to handle all stack network activity
// *
// *
// * Copyright 2015 Silicon Laboratories, Inc.                              *80*
// *****************************************************************************

#ifdef EMBER_SCRIPTED_TEST
#include "app/framework/plugin-soc/security-sensor/security-sensor-test.h"
#endif

#include "app/framework/include/af.h"
#include "app/framework/plugin-soc/connection-manager/connection-manager.h"
#include "app/framework/plugin/ias-zone-server/ias-zone-server.h"
#include EMBER_AF_API_LED_BLINK
#include EMBER_AF_API_GPIO_SENSOR

//------------------------------------------------------------------------------
// Plugin specific macros

#define IAS_ZONE_ENDPOINT \
  EMBER_AF_PLUGIN_SECURITY_SENSOR_IAS_ZONE_ENDPOINT

#define MAX_TIME_BETWEEN_PRESSES_MS \
  EMBER_AF_PLUGIN_SECURITY_SENSOR_TIME_BETWEEN_PRESSES_MS

#define INITIAL_REPORT_DELAY_SECONDS  5
#define INITIAL_REPORT_DELAY_QS   (INITIAL_REPORT_DELAY_SECONDS * 4)
// Status bit definitions used when generating report to IAS Zone Server
#define STATUS_ALARM    0x0001
#define STATUS_NO_ALARM 0x0000
#define STATUS_TAMPER    0x0004
#define STATUS_NO_TAMPER 0x0000

#define GATEWAY_BOOT_DELAY_MS 100

//------------------------------------------------------------------------------
// Plugin private types and enums

//------------------------------------------------------------------------------
// Forward Declaration of plugin private functions
static void changeTamperStatus(uint8_t status);
static void sendZoneAlarmUpdate(void);

//------------------------------------------------------------------------------
// Global Variables

// Event used internal to plugin
EmberEventControl emberAfPluginSecuritySensorInitEventControl;
EmberEventControl emberAfPluginSecuritySensorInitialReportEventControl;

// State variable to track if the device has been tampered with
static uint8_t tamperStatus = STATUS_NO_TAMPER;

// State variable to track the contact sensor state
static uint8_t contactStatus = STATUS_NO_ALARM;

//------------------------------------------------------------------------------
// Implemented Functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Plugin consumed callback implementations

// security-sensor framework init callback.
void emberAfPluginSecuritySensorInitCallback(void)
{
  // Need to give the gateway time to populate its UI and start listening for
  // messages from the contact sensor
  emberEventControlSetDelayMS(emberAfPluginSecuritySensorInitEventControl,
                              GATEWAY_BOOT_DELAY_MS);
}

// When the contact sensor status changes, update the global status variable and
// the cluster's attribute
void emberAfPluginGpioSensorStateChangedCallback(uint8_t newAlarmState)
{
  emberAfAppPrintln("Security Switch new state:  %x", newAlarmState);
  if (newAlarmState == HAL_GPIO_SENSOR_ACTIVE) {
    contactStatus = STATUS_ALARM;
  } else {
    contactStatus = STATUS_NO_ALARM;
  }
  sendZoneAlarmUpdate();
}

// Once the tamper switch goes active, update the tamper alarm state to none
void emberAfPluginTamperSwitchTamperActiveCallback(void)
{
  emberAfAppPrintln("Activating tamper mode");
  changeTamperStatus(STATUS_NO_TAMPER);
}

// If the tamper alarm trips, update the attribute to show that alarm is active
void emberAfPluginTamperSwitchTamperAlarmCallback(void)
{
  emberAfAppPrintln("Tamper detected!");
  changeTamperStatus(STATUS_TAMPER);
}

// This callback is executed when a network join attempt finishes.  It is needed
// to generate the initial alarm state when the device first joins a network
void emberAfPluginSecuritySensorStackStatusCallback(EmberStatus status)
{
  if (status == EMBER_NETWORK_UP) {
    //Provisioning takes about five seconds, so delay a bit before sending the
    //initial status
    if (halGpioSensorGetSensorValue() == HAL_GPIO_SENSOR_ACTIVE) {
      contactStatus = STATUS_ALARM;
    } else {
      contactStatus = STATUS_NO_ALARM;
    }
    sendZoneAlarmUpdate();
  }
}

//------------------------------------------------------------------------------
// Plugin event handlers

// security-sensor init event handler.  This is called at a point when all other
// threads have finished running their initialization functions.  All init
// functions that depend on other plugins being in a sane state should go here.
//   initialize super sleepy features
//   set up IRQ for reed switch pin
//   determine state of network and start trying to join, if necessary
//   blink UI LED
void emberAfPluginSecuritySensorInitEventHandler(void)
{
  emberAfAppPrintln("Security Sensor:  init Handler. %d",
                    halCommonGetInt16uMillisecondTick());

  emberEventControlSetInactive(emberAfPluginSecuritySensorInitEventControl);

  // Set which LED is going to be the activity LED
  halLedBlinkSetActivityLed(BOARD_ACTIVITY_LED);
}

void emberAfPluginSecuritySensorInitialReportEventHandler(void)
{
  emberEventControlSetInactive(
    emberAfPluginSecuritySensorInitialReportEventControl);
}

//------------------------------------------------------------------------------
// Plugin private function implementations

// Update the IAS Zone Server based on the current state of the (globally
// defined) tamper and contact status variables.
static void sendZoneAlarmUpdate(void)
{
  if (emberAfNetworkState() == EMBER_NO_NETWORK) {
    emberAfPluginConnectionManagerLeaveNetworkAndStartSearchForNewOne();
  }

  // status to send will be the combination of the contact status and the
  // tamper status.
  emberAfPluginIasZoneServerUpdateZoneStatus(IAS_ZONE_ENDPOINT,
                                             contactStatus | tamperStatus,
                                             0);     // QS since status occurred
}

// Helper function to update the tamper status variable
static void changeTamperStatus(uint8_t status)
{
  emberAfAppPrintln( "TAMPER STATUS: %2x", status);
  tamperStatus = status;
  sendZoneAlarmUpdate();
}
