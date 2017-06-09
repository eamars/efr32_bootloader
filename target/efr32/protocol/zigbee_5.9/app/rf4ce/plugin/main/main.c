// Copyright 2014 Silicon Laboratories, Inc.

#include PLATFORM_HEADER
#include CONFIGURATION_HEADER
#include EMBER_AF_API_EMBER_TYPES
#ifdef EZSP_HOST
  #include EMBER_AF_API_EZSP_PROTOCOL
  #include EMBER_AF_API_EZSP
#else
  #include EMBER_AF_API_STACK
#endif
#include EMBER_AF_API_EVENT
#include EMBER_AF_API_HAL
#include EMBER_AF_API_NETWORK_INTERFACE
#ifdef EMBER_AF_API_DEBUG_PRINT
  #include EMBER_AF_API_DEBUG_PRINT
#endif
#include "rf4ce-bookkeeping.h"
#include "rf4ce-callbacks.h"

// If gateway functionality is enabled, we need to initiaze it immediately
// after initializing the HAL.  If the command interpreter is also included, it
// needs to be initialized at the same time.  This has to happen before the
// serial ports are initialized.
#ifdef EMBER_AF_API_GATEWAY
  EmberStatus gatewayInit(MAIN_FUNCTION_PARAMETERS);
  void gatewayWaitForEventsWithTimeout(uint32_t timeoutMs);
  static void _gatewayInit(MAIN_FUNCTION_PARAMETERS);
  static void gatewayTick(void);
  #define GATEWAY_INIT _gatewayInit
  #define GATEWAY_TICK gatewayTick
  #ifdef EMBER_AF_API_COMMAND_INTERPRETER2
    #include EMBER_AF_API_COMMAND_INTERPRETER2
    EmberStatus gatewayCommandInterpreterInit(const char* cliPrompt,
                                              EmberCommandEntry commands[]);
    extern EmberCommandEntry emberCommandTable[];
    #define GATEWAY_CLI_INIT gatewayCommandInterpreterInit
  #else
    #define GATEWAY_CLI_INIT(...)
  #endif
#else
  #define GATEWAY_INIT(MAIN_FUNCTION_ARGUMENTS)
  #define GATEWAY_TICK()
#endif

// If serial functionality is enabled, we will initiaze the serial ports during
// startup.  This has to happen after the HAL and gateway, if applicable, are
// initiazed.
#ifdef EMBER_AF_API_SERIAL
  #include EMBER_AF_API_SERIAL
  #define SERIAL_INIT EMBER_AF_SERIAL_PORT_INIT
#else
  #define SERIAL_INIT()
#endif

// If printing is enabled, we will print some diagnostic information about the
// most recent reset and also during runtime.  On some platforms, extended
// diagnostic information is available.
#if defined(EMBER_AF_API_SERIAL) && defined(EMBER_AF_PRINT_ENABLE)
  #ifdef EMBER_AF_API_DIAGNOSTIC_CORTEXM3
    #include EMBER_AF_API_DIAGNOSTIC_CORTEXM3
  #endif
  static void printResetInformation(void);
  #define PRINT_RESET_INFORMATION printResetInformation
  #define emberAfGuaranteedPrint(...) \
    emberSerialGuaranteedPrintf(APP_SERIAL, __VA_ARGS__)
  #define emberAfGuaranteedPrintln(...)                     \
    do {                                                    \
      emberSerialGuaranteedPrintf(APP_SERIAL, __VA_ARGS__); \
      emberSerialGuaranteedPrintf(APP_SERIAL, "\r\n");      \
    } while (false)
#else
  #define PRINT_RESET_INFORMATION()
  #define emberAfGuaranteedPrint(...)
  #define emberAfGuaranteedPrintln(...)
#endif

// Our entry point is typically main(), except during testing.
#ifdef EMBER_TEST
  #define MAIN nodeMain
#else
  #define MAIN main
#endif

// Initialization and runtime operation is similar on hosts and SoCs. but the
// functions involved and some of the particulars are different.
#ifdef EZSP_HOST
  static void ncpInit(void);
  static void ncpTick(void);
  #define INIT                 ncpInit
  #define TICK                 ncpTick
  #define STACK_STATUS_HANDLER ezspStackStatusHandler
#else
  static void stackInit(void);
  #define INIT                 stackInit
  #define TICK                 emberTick
  #define STACK_STATUS_HANDLER emberStackStatusHandler
#endif

extern const EmberEventData emAppEvents[];
EmberTaskId emAppTask;

int MAIN(MAIN_FUNCTION_PARAMETERS)
{
  // Initialize the HAL and enable interrupts.
  halInit();
  INTERRUPTS_ON();

  // Initialize the gateway.
  GATEWAY_INIT(MAIN_FUNCTION_ARGUMENTS);

  // Initialize the serial ports.
  SERIAL_INIT();

  // Display diagnostic information about the most recent reset.
  PRINT_RESET_INFORMATION();

  // Initialize a task for the application and plugin events and enable idling.
  emAppTask = emberTaskInit(emAppEvents);
  emberTaskEnableIdling(true);

  // Initialize the stack or EZSP layer.
  INIT();

  // Initialize the application and plugins.  This function is generated.
  emberAfInit();

  // Restart the network from tokens if possible.
  emberAfNetworkInit();

  while (true) {
    // Reset the watchdog timer to prevent a timeout.
    halResetWatchdog();

    // Let the stack or EZSP layer run periodic tasks.
    TICK();

    // Run the gateway periodic task.
    GATEWAY_TICK();

    // Let the application and plugins run periodic tasks.  This function is
    // generated.
    emberAfTick();

    // Run the application and plugin events.
    emberRunTask(emAppTask);
  }
}

void STACK_STATUS_HANDLER(EmberStatus status)
{
  // Notify the application and plugins about the status.  This function is
  // generated.
#ifdef EMBER_TEST
  if (status == EMBER_NETWORK_UP) {
    emberAfCorePrintln("EMBER_NETWORK_UP");
  } else if (status == EMBER_NETWORK_DOWN) {
    emberAfCorePrintln("EMBER_NETWORK_DOWN");
  }
#endif
  emberAfStackStatus(status);
}

#ifdef EMBER_AF_API_GATEWAY

static void _gatewayInit(MAIN_FUNCTION_PARAMETERS)
{
  // If we are using the gateway, it must initialize properly in order for the
  // application to run correctly.  If something goes wrong, we have no choice
  // but to assert.
  EmberStatus status = gatewayInit(MAIN_FUNCTION_ARGUMENTS);
  emberAfGuaranteedPrintln("Gateway Init: 0x%x", status);
  assert(status == EMBER_SUCCESS);

  // If the command interpreter is in use, it must be initialized immediately
  // after the gateway.  This sets up command completion and history.
  GATEWAY_CLI_INIT(EMBER_AF_DEVICE_NAME, emberCommandTable);
}

static void gatewayTick(void)
{
  // If we are using the gateway, the time to our next event is the maximum
  // amount of time we can yield while waiting for data on our monitored file
  // descriptors.  If we have nothing scheduled, we can wait forever.
  uint32_t durationMs = emberMsToNextEvent(emAppEvents, MAX_INT32U_VALUE);
  gatewayWaitForEventsWithTimeout(durationMs);
}

#endif // EMBER_AF_API_GATEWAY

#ifdef EMBER_AF_PRINT_ENABLE

static void printResetInformation(void)
{
  // Information about the most recent reset is printed during startup to aid
  // in debugging.
  emberAfGuaranteedPrintln("Reset info: 0x%x (%p)",
                           halGetResetInfo(),
                           halGetResetString());
#ifdef EMBER_AF_API_DIAGNOSTIC_CORTEXM3
  emberAfGuaranteedPrintln("Extended reset info: 0x%2x (%p)",
                           halGetExtendedResetInfo(),
                           halGetExtendedResetString());
  if (halResetWasCrash()) {
    halPrintCrashSummary(APP_SERIAL);
    halPrintCrashDetails(APP_SERIAL);
    halPrintCrashData(APP_SERIAL);
  }
#endif // EMBER_AF_API_DIAGNOSTIC_CORTEXM3
}

#endif // EMBER_AF_PRINT_ENABLE

#ifdef EZSP_HOST

static void ncpInit(void)
{
  EzspStatus status;
  uint8_t stackType;
  uint16_t stackVersion;
  uint16_t seed0, seed1;

  // Reset the NCP and initialize the serial protocol.  If this fails, we have
  // to assert because something is wrong.
  status = ezspInit();
  emberAfGuaranteedPrintln("Init: 0x%x", status);
  assert(status == EZSP_SUCCESS);

  // After initializing EZSP and before and commands are set, the host must
  // specify the EZSP protocol version that it will use.
  ezspVersion(EZSP_PROTOCOL_VERSION, &stackType, &stackVersion);

  // The random number generator on the host needs to be seeded with some
  // random data, which we can get from the NCP.
  ezspGetRandomNumber(&seed0);
  ezspGetRandomNumber(&seed1);
  halStackSeedRandom(((uint32_t)seed1 << 16) | (uint32_t)seed0);

  // These are not used in RF4CE and are set to zero to save RAM on the NCP.
  ezspSetConfigurationValue(EZSP_CONFIG_APS_UNICAST_MESSAGE_COUNT, 0);
  ezspSetConfigurationValue(EZSP_CONFIG_BINDING_TABLE_SIZE,        0);
  ezspSetConfigurationValue(EZSP_CONFIG_BROADCAST_ALARM_DATA_SIZE, 0);
  ezspSetConfigurationValue(EZSP_CONFIG_BROADCAST_TABLE_SIZE,      0);
  ezspSetConfigurationValue(EZSP_CONFIG_DISCOVERY_TABLE_SIZE,      0);
  ezspSetConfigurationValue(EZSP_CONFIG_KEY_TABLE_SIZE,            0);
  ezspSetConfigurationValue(EZSP_CONFIG_MAC_FILTER_TABLE_SIZE,     0);
  ezspSetConfigurationValue(EZSP_CONFIG_MAX_END_DEVICE_CHILDREN,   0);
  ezspSetConfigurationValue(EZSP_CONFIG_MULTICAST_TABLE_SIZE,      0);
  ezspSetConfigurationValue(EZSP_CONFIG_NEIGHBOR_TABLE_SIZE,       0);
  ezspSetConfigurationValue(EZSP_CONFIG_ROUTE_TABLE_SIZE,          0);
  ezspSetConfigurationValue(EZSP_CONFIG_SOURCE_ROUTE_TABLE_SIZE,   0);
  ezspSetConfigurationValue(EZSP_CONFIG_UNICAST_ALARM_DATA_SIZE,   0);

  // This is an RF4CE-only device, so it has just a single network.
  ezspSetConfigurationValue(EZSP_CONFIG_SUPPORTED_NETWORKS, 1);

  // The RF4CE tables are set according to the configuration.
  ezspSetConfigurationValue(EZSP_CONFIG_RF4CE_PAIRING_TABLE_SIZE,
                            EMBER_RF4CE_PAIRING_TABLE_SIZE);
  ezspSetConfigurationValue(EZSP_CONFIG_RF4CE_PENDING_OUTGOING_PACKET_TABLE_SIZE,
                            EMBER_RF4CE_PENDING_OUTGOING_PACKET_TABLE_SIZE);

  // The application and plugins are then allowed to do any final configuration
  // that affects memory allocation on the NCP.  This function is generated.
  emberAfNcpInit(true); // memory allocation

  // After the memory allocation is finalized, the packet buffer count on the
  // NCP is maximized to give the stack access to all remaining RAM.
  ezspSetConfigurationValue(EZSP_CONFIG_PACKET_BUFFER_COUNT,
                            EZSP_MAXIMIZE_PACKET_BUFFER_COUNT);

  // Now, the application and plugins can do any configuration that does not
  // affect memory allocation.  This function is generated.
  emberAfNcpInit(false); // configuration
}

static void ncpTick(void)
{
  // Let the EZSP layer handle asynchronous events and then service any
  // callbacks pending from the NCP.
  ezspTick();
  while (ezspCallbackPending()) {
    ezspCallback();
  }
}

void ezspErrorHandler(EzspStatus status)
{
  // TODO: Reset NCP without resetting the host.  Note that this requires a
  // call to emberAfNetworkInit after the initialization of the NCP.
  emberAfGuaranteedPrintln("EZSP Error: 0x%x", status);
  assert(status == EZSP_SUCCESS);
}

void halNcpIsAwakeIsr(bool isAwake)
{
  // TODO: Reset NCP without resetting the host.  Note that this requires a
  // call to emberAfNetworkInit after the initialization of the NCP.
  emberAfGuaranteedPrintln("NCP: %p", (isAwake ? "awake" : "unresponsive"));
  assert(isAwake);
}

#else // EZSP_HOST

static void stackInit(void)
{
  // Initialize the radio and the stack.  If this fails, we have to assert
  // because something is wrong.
  EmberStatus status = emberInit();
  emberAfGuaranteedPrintln("Init: 0x%x", status);
  assert(status == EMBER_SUCCESS);
}

#endif // EZSP_HOST
