// *******************************************************************
// * price-server.c
// *
// *
// * Copyright 2007 by Ember Corporation. All rights reserved.              *80*
// *******************************************************************

#include "../../include/af.h"
#include "../../util/common.h"
#include "price-server.h"
#include "price-server-tick.h"

#ifdef EMBER_AF_PLUGIN_TEST_HARNESS
#include "app/framework/plugin/test-harness/test-harness.h"
#endif // EMBER_AF_PLUGIN_TEST_HARNESS

static EmberAfScheduledPrice priceTable[EMBER_AF_PRICE_CLUSTER_SERVER_ENDPOINT_COUNT][EMBER_AF_PLUGIN_PRICE_SERVER_PRICE_TABLE_SIZE];
EmberAfPriceServerInfo info;

// Forward declaration

void emberAfPriceClearBlockPeriodTable( uint8_t endpoint );
static void emberAfPluginPriceServerBlockPeriodUpdateBindings(uint8_t blockPeriodEntryIndex);
static void emberAfPriceClearTierLabelTable( uint8_t endpoint );
static void emberAfPriceClearCalorificValueTable( uint8_t endpoint );
static void emberAfPluginPriceServerClearCo2Value( uint8_t endpoint );
static void emberAfPriceClearConsolidatedBillsTable( uint8_t endpoint );
static void emberAfPriceClearConversionFactorTable( uint8_t endpoint );
static void emberAfPriceClearBillingPeriodTable( uint8_t endpoint );
static void emberAfPriceClearCancelTariffTable( uint8_t endpoint );
static void emberAfPriceClearCurrencyConversionTable( uint8_t endpoint );
static void emAfPriceServerScheduleGetScheduledPrices( uint8_t endpoint );
static void sortCreditPaymentEntries( uint8_t *entries, uint8_t numValidEntries, EmberAfPriceCreditPayment *table );
static void emberAfPriceUpdateConversionFactorAttribs( uint8_t endpoint, uint32_t conversionFactor, uint8_t conversionFactorTrailingDigit );
static void emberAfPriceUpdateCalorificValueAttribs( uint8_t endpoint, uint32_t calorificValue, 
                                                     uint8_t calorificValueUnit, uint8_t calorificValueTrailingDigit );

// Bits 1 through 7 are reserved in the price control field.  These are used
// internally to represent whether the message is valid, active, or is a "start
// now" price.
#define VALID  BIT(1)
#define ACTIVE BIT(2)
#define NOW    BIT(3)
#define priceIsValid(price)   ((price)->priceControl & VALID)
#define priceIsActive(price)  ((price)->priceControl & ACTIVE)
#define priceIsNow(price)     ((price)->priceControl & NOW)
#define priceIsForever(price) ((price)->duration == ZCL_PRICE_CLUSTER_DURATION16_UNTIL_CHANGED)
#define TARIFF_TYPE_DONT_CARE (0xFF)


// Returns true if the price will be current or scheduled at the given time.
static bool priceIsCurrentOrScheduled(const EmberAfScheduledPrice *price,
                                         uint32_t time)
{
  return (priceIsValid(price)
          && priceIsActive(price)
          && (priceIsForever(price)
              || time < price->startTime + (uint32_t)price->duration * 60));
}

// Returns the number of all current or scheduled prices.
static uint8_t scheduledPriceCount(uint8_t endpoint, uint32_t startTime)
{
  uint8_t i, count = 0;
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);

  if (ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX) {
    return 0;
  }

  for (i = 0; i < EMBER_AF_PLUGIN_PRICE_SERVER_PRICE_TABLE_SIZE; i++) {
    if (priceIsCurrentOrScheduled(&priceTable[ep][i], startTime)) {
      count++;
    }
  }
  return count;
}

static void fillPublishPriceCommand(EmberAfScheduledPrice price)
{
  char * const args[] = {"wswwuvuuwvwuwu","wswwuvuuwvwuwuwuuuu","wswwuvuuwvwuwuwuuuuuuuuu"};
  UNUSED_VAR(args);
  emberAfFillExternalBuffer(ZCL_CLUSTER_SPECIFIC_COMMAND|ZCL_FRAME_CONTROL_SERVER_TO_CLIENT
                              | EMBER_AF_DEFAULT_RESPONSE_POLICY_RESPONSES,
                              ZCL_PRICE_CLUSTER_ID,
                              ZCL_PUBLISH_PRICE_COMMAND_ID,
    #if defined(EMBER_AF_PLUGIN_TEST_HARNESS)
                              args[sendSE11PublishPriceCommand],
    #elif defined(EMBER_AF_SE11)
                              "wswwuvuuwvwuwuwuuuu",
    #elif defined(EMBER_AF_SE10)
                              "wswwuvuuwvwuwu",
    #else // EMBER_AF_SE12.
                              "wswwuvuuwvwuwuwuuuuuuuuu",
    #endif
                              price.providerId,
                              price.rateLabel,
                              price.issuerEventID,
                              emberAfGetCurrentTime(),
                              price.unitOfMeasure,
                              price.currency,
                              price.priceTrailingDigitAndTier,
                              price.numberOfPriceTiersAndTier,
                              price.startTime,
                              price.duration,
                              price.price,
                              price.priceRatio,
                              price.generationPrice,
                              price.generationPriceRatio,
                              price.alternateCostDelivered,
                              price.alternateCostUnit,
                              price.alternateCostTrailingDigit,
                              price.numberOfBlockThresholds,
                              price.priceControl,
                              // TODO: these are optional; if we want to implement, we should!
                              0, // NumberOfGenerationTiers
                              0x01, // GenerationTier
                              0, // ExtendedNumberOfPriceTiers
                              0, // ExtendedPriceTier
                              0); // ExtendedRegisterTier
}

typedef struct {
  bool isIntraPan;
  union {
    struct {
      EmberNodeId nodeId;
      uint8_t       clientEndpoint;
      uint8_t       serverEndpoint;
    } intra;
    struct {
      EmberEUI64 eui64;
      EmberPanId panId;
    } inter;
  } pan;
  uint8_t  sequence;
  uint8_t  index;
  uint32_t startTime;
  uint8_t  numberOfEvents;
} GetScheduledPricesPartner;
static GetScheduledPricesPartner partner;

void emberAfPriceClearPriceTable(uint8_t endpoint)
{
  uint8_t i;
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);

  if (ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX) {
    return;
  }

  for (i = 0; i < EMBER_AF_PLUGIN_PRICE_SERVER_PRICE_TABLE_SIZE; i++) {
    priceTable[ep][i].priceControl &= ~VALID;
  }
}


// Retrieves the price at the index.  Returns false if the index is invalid.
bool emberAfPriceGetPriceTableEntry(uint8_t endpoint,
                                       uint8_t index,
                                       EmberAfScheduledPrice *price)
{
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);

  if (ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX || index == 0xFF) {
    return false;
  }

  if (index < EMBER_AF_PLUGIN_PRICE_SERVER_PRICE_TABLE_SIZE) {
    MEMMOVE(price, &priceTable[ep][index], sizeof(EmberAfScheduledPrice));

    // Clear out our internal bits from the price control.
    price->priceControl &= ~ZCL_PRICE_CLUSTER_RESERVED_MASK;

    // If the price is expired or it has an absolute time, set the start time
    // and duration to the original start time and duration.  For "start now"
    // prices that are current or scheduled, set the start time to the special
    // value for "now" and set the duration to the remaining time, if it is not
    // already the special value for "until changed."
    if (priceIsCurrentOrScheduled(&priceTable[ep][index], emberAfGetCurrentTime())
        && priceIsNow(&priceTable[ep][index])) {
      price->startTime = ZCL_PRICE_CLUSTER_START_TIME_NOW;
      if (!priceIsForever(&priceTable[ep][index])) {
        price->duration -= ((emberAfGetCurrentTime()
                             - priceTable[ep][index].startTime)
                            / 60);
      }
    }
    return true;
  }

  return false;
}


// Sets the price at the index.  Returns false if the index is invalid.
bool emberAfPriceSetPriceTableEntry(uint8_t endpoint,
                                       uint8_t index,
                                       const EmberAfScheduledPrice *price)
{
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);

  if (ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX) {
    return false;
  }

  if (index < EMBER_AF_PLUGIN_PRICE_SERVER_PRICE_TABLE_SIZE) {
    if (price == NULL) {
      priceTable[ep][index].priceControl &= ~ACTIVE;
      return true;
    }

    MEMMOVE(&priceTable[ep][index], price, sizeof(EmberAfScheduledPrice));

    // Rember if this is a "start now" price, but store the start time as the
    // current time so the duration can be adjusted.
    if (priceTable[ep][index].startTime == ZCL_PRICE_CLUSTER_START_TIME_NOW) {
      priceTable[ep][index].priceControl |= NOW;
      priceTable[ep][index].startTime = emberAfGetCurrentTime();
    } else {
      priceTable[ep][index].priceControl &= ~NOW;
    }

    priceTable[ep][index].priceControl |= (VALID | ACTIVE);
    return true;
  }
  return false;
}


// Returns the index in the price table of the current price.  The first price
// in the table that starts in the past and ends in the future in considered
// the current price.
uint8_t emberAfGetCurrentPriceIndex(uint8_t endpoint)
{
  uint32_t now = emberAfGetCurrentTime();
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  uint8_t i;

  if (ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX) {
    return ZCL_PRICE_INVALID_ENDPOINT_INDEX;
  }

  for (i = 0; i < EMBER_AF_PLUGIN_PRICE_SERVER_PRICE_TABLE_SIZE; i++) {
    if (priceIsValid(&priceTable[ep][i])) {
      uint32_t endTime = ((priceTable[ep][i].duration
                         == ZCL_PRICE_CLUSTER_DURATION16_UNTIL_CHANGED)
                        ? ZCL_PRICE_CLUSTER_END_TIME_NEVER
                        : priceTable[ep][i].startTime + priceTable[ep][i].duration * 60);

      emberAfPriceClusterPrint("checking price %X, currTime %4x, start %4x, end %4x ",
                               i,
                               now,
                               priceTable[ep][i].startTime,
                               endTime);

      if (priceTable[ep][i].startTime <= now && now < endTime) {
        emberAfPriceClusterPrintln("valid");
        emberAfPriceClusterFlush();
        return i;
      } else {
        emberAfPriceClusterPrintln("no");
        emberAfPriceClusterFlush();
      }
    }
  }

  return ZCL_PRICE_INVALID_INDEX;
}

// Retrieves the current price.  Returns false is there is no current price.
bool emberAfGetCurrentPrice(uint8_t endpoint, EmberAfScheduledPrice *price)
{
  return emberAfPriceGetPriceTableEntry(endpoint, emberAfGetCurrentPriceIndex(endpoint), price);
}

void emberAfPricePrint(const EmberAfScheduledPrice *price)
{
  emberAfPriceClusterPrint("  label: ");
  emberAfPriceClusterPrintString(price->rateLabel);

  emberAfPriceClusterPrint("(%X)\r\n  uom/cur: 0x%X/0x%2X"
                           "\r\n  pid/eid: 0x%4X/0x%4X"
                           "\r\n  ct/st/dur: 0x%4X/0x%4X/",
                           emberAfStringLength(price->rateLabel),
                           price->unitOfMeasure,
                           price->currency,
                           price->providerId,
                           price->issuerEventID,
                           emberAfGetCurrentTime(),
                           price->startTime);
  if (price->duration == ZCL_PRICE_CLUSTER_DURATION16_UNTIL_CHANGED) {
    emberAfPriceClusterPrint("INF");
  } else {
    emberAfPriceClusterPrint("0x%2X", price->duration);
  }
  emberAfPriceClusterFlush();
  emberAfPriceClusterPrintln("\r\n  ptdt/ptrt: 0x%X/0x%X"
                             "\r\n  p/pr: 0x%4x/0x%X"
                             "\r\n  gp/gpr: 0x%4x/0x%X"
                             "\r\n  acd/acu/actd: 0x%4x/0x%X/0x%X",
                             price->priceTrailingDigitAndTier,
                             price->numberOfPriceTiersAndTier,
                             price->price,
                             price->priceRatio,
                             price->generationPrice,
                             price->generationPriceRatio,
                             price->alternateCostDelivered,
                             price->alternateCostUnit,
                             price->alternateCostTrailingDigit);
  emberAfPriceClusterFlush();
  emberAfPriceClusterPrintln("  nobt: 0x%X", price->numberOfBlockThresholds);
  emberAfPriceClusterPrintln("  pc: 0x%X",
                             (price->priceControl
                              & ZCL_PRICE_CLUSTER_RESERVED_MASK));
  emberAfPriceClusterPrint("  price is valid from time 0x%4x until ",
                           price->startTime);
  if (price->duration == ZCL_PRICE_CLUSTER_DURATION16_UNTIL_CHANGED) {
    emberAfPriceClusterPrintln("eternity");
  } else {
    emberAfPriceClusterPrintln("0x%4x",
                               (price->startTime + (price->duration * 60)));
  }
  emberAfPriceClusterFlush();
}


void emberAfPricePrintPriceTable(uint8_t endpoint)
{
#if defined(EMBER_AF_PRINT_ENABLE) && defined(EMBER_AF_PRINT_PRICE_CLUSTER)
  uint8_t i;
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  uint8_t currPriceIndex = emberAfGetCurrentPriceIndex(endpoint);

  if (ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX || currPriceIndex == 0xFF) {
    return;
  }

  emberAfPriceClusterFlush();
  emberAfPriceClusterPrintln("Configured Prices: (total %X, curr index %X)",
                             scheduledPriceCount(endpoint, 0),
                             currPriceIndex);
  emberAfPriceClusterFlush();
  emberAfPriceClusterPrintln("  Note: ALL values given in HEX\r\n");
  emberAfPriceClusterFlush();
  for (i = 0; i < EMBER_AF_PLUGIN_PRICE_SERVER_PRICE_TABLE_SIZE; i++) {
    if (!priceIsValid(&priceTable[ep][i])) {
      continue;
    }
    emberAfPriceClusterPrintln("= PRICE %X =%p",
                               i,
                               (i == currPriceIndex ? " (Current Price)" : ""));
    emberAfPricePrint(&priceTable[ep][i]);
  }
#endif // defined(EMBER_AF_PRINT_ENABLE) && defined(EMBER_AF_PRINT_PRICE_CLUSTER)
}

void emberAfPrintBillingPeriodTable( uint8_t endpoint )
{
#if defined(EMBER_AF_PRINT_ENABLE) && defined(EMBER_AF_PRINT_PRICE_CLUSTER)
  uint8_t i = 0;
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  EmberAfPriceCommonInfo * curInfo = NULL;
  EmberAfPriceBillingPeriod * billingPeriod = NULL;

  emberAfPriceClusterPrintln("= Billing Periods =");
  if (ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX) {
    return;
  }
  for ( i=0; i<EMBER_AF_PLUGIN_PRICE_SERVER_BILLING_PERIOD_TABLE_SIZE; i++){
    curInfo = &info.billingPeriodTable.commonInfos[ep][i];
    billingPeriod = &info.billingPeriodTable.billingPeriods[ep][i];

    emberAfPriceClusterPrintln("  [%d]: valid: 0x%X", i, curInfo->valid);
    emberAfPriceClusterPrintln("  [%d]: startTime: 0x%4X", i, curInfo->startTime);
    emberAfPriceClusterPrintln("  [%d]: issuerEventId: 0x%4X", i, curInfo->issuerEventId);
    emberAfPriceClusterPrintln("  [%d]: providerId: 0x%4X", i, billingPeriod->providerId);
    emberAfPriceClusterPrintln("  [%d]: billingPeriodDuration: 0x%4X", i, billingPeriod->billingPeriodDuration);
    emberAfPriceClusterPrintln("  [%d]: billingPeriodDurationType: 0x%X", i, billingPeriod->billingPeriodDurationType);
    emberAfPriceClusterPrintln("  [%d]: tariffType: 0x%X", i, billingPeriod->tariffType);
  }
#endif // defined(EMBER_AF_PRINT_ENABLE) && defined(EMBER_AF_PRINT_PRICE_CLUSTER)
}

void emberAfPrintConsolidatedBillTableEntry( uint8_t endpoint, uint8_t index )
{
#if defined(EMBER_AF_PRINT_ENABLE) && defined(EMBER_AF_PRINT_PRICE_CLUSTER)
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX) {
    return;
  }
  emberAfPriceClusterPrintln("= Consolidated Bill Table[%d] =", index);
  //uint8_t i = 0;
  //for ( i=0; i<EMBER_AF_PLUGIN_PRICE_SERVER_BILLING_PERIOD_TABLE_SIZE; i++){
  if( index < EMBER_AF_PLUGIN_PRICE_SERVER_CONSOLIDATED_BILL_TABLE_SIZE ){
    emberAfPriceClusterPrintln("  [%d]: valid: 0x%x", index, info.consolidatedBillsTable.commonInfos[ep][index].valid );
    emberAfPriceClusterPrintln("  [%d]: providerId: 0x%4x", index, info.consolidatedBillsTable.consolidatedBills[ep][index].providerId );
    emberAfPriceClusterPrintln("  [%d]: issuerEventId: 0x%x", index, info.consolidatedBillsTable.commonInfos[ep][index].issuerEventId );
    emberAfPriceClusterPrintln("  [%d]: startTime: 0x%4x", index, info.consolidatedBillsTable.commonInfos[ep][index].startTime );
    emberAfPriceClusterPrintln("  [%d]: duration (sec): 0x%4x", index, info.consolidatedBillsTable.commonInfos[ep][index].durationSec );
    emberAfPriceClusterPrintln("  [%d]: billingPeriodDuration: 0x%4x", index, info.consolidatedBillsTable.consolidatedBills[ep][index].billingPeriodDuration );
    emberAfPriceClusterPrintln("  [%d]: billingPeriodType: 0x%x", index, info.consolidatedBillsTable.consolidatedBills[ep][index].billingPeriodDurationType );
    emberAfPriceClusterPrintln("  [%d]: tariffType: 0x%x", index, info.consolidatedBillsTable.consolidatedBills[ep][index].tariffType );
    emberAfPriceClusterPrintln("  [%d]: consolidatedBill: 0x%x", index, info.consolidatedBillsTable.consolidatedBills[ep][index].consolidatedBill );
    emberAfPriceClusterPrintln("  [%d]: currency: 0x%x", index, info.consolidatedBillsTable.consolidatedBills[ep][index].currency );
    emberAfPriceClusterPrintln("  [%d]: billTrailingDigit: 0x%x", index, info.consolidatedBillsTable.consolidatedBills[ep][index].billTrailingDigit );
  }
#endif // defined(EMBER_AF_PRINT_ENABLE) && defined(EMBER_AF_PRINT_PRICE_CLUSTER)
}


void emberAfPrintConversionTable( uint8_t endpoint ) {
#if defined(EMBER_AF_PRINT_ENABLE) && defined(EMBER_AF_PRINT_PRICE_CLUSTER)
  uint8_t i = 0;
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX) {
    return;
  }
  emberAfPriceClusterPrintln("= Conversion Factors =");
  for (i=0; i<EMBER_AF_PLUGIN_PRICE_SERVER_CONVERSION_FACTOR_TABLE_SIZE; i++){
    emberAfPriceClusterPrintln("  [%d]: startTime: 0x%4x", i, info.conversionFactorTable.commonInfos[ep][i].startTime);
    emberAfPriceClusterPrintln("  [%d]: valid: 0x%X", i, info.conversionFactorTable.commonInfos[ep][i].valid);
    emberAfPriceClusterPrintln("  [%d]: issuerEventId: 0x%X", i, info.conversionFactorTable.commonInfos[ep][i].issuerEventId);
    emberAfPriceClusterPrintln("  [%d]: conversionFactor: 0x%4x", i, info.conversionFactorTable.priceConversionFactors[ep][i].conversionFactor);
    emberAfPriceClusterPrintln("  [%d]: conversionFactorTrailingDigit: 0x%X", i, info.conversionFactorTable.priceConversionFactors[ep][i].conversionFactorTrailingDigit);
  }
#endif // defined(EMBER_AF_PRINT_ENABLE) && defined(EMBER_AF_PRINT_PRICE_CLUSTER)
}

void emberAfPrintCo2ValuesTable( uint8_t endpoint )
{
#if defined(EMBER_AF_PRINT_ENABLE) && defined(EMBER_AF_PRINT_PRICE_CLUSTER)
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX) {
    return;
  }
  emberAfPriceClusterPrintln("= Co2 Values =");
  uint8_t i = 0;
  for ( i=0; i<EMBER_AF_PLUGIN_PRICE_SERVER_CO2_VALUE_TABLE_SIZE; i++){
    emberAfPriceClusterPrintln("  [%d]: valid: 0x%X", i, info.co2ValueTable.commonInfos[ep][i].valid);
    emberAfPriceClusterPrintln("  [%d]: providerId: 0x%4x", i, info.co2ValueTable.co2Values[ep][i].providerId);
    emberAfPriceClusterPrintln("  [%d]: issuerEventId: 0x%4X", i, info.co2ValueTable.commonInfos[ep][i].issuerEventId);
    emberAfPriceClusterPrint("  [%d]: startTime: ", i);
    emberAfPrintTime(info.co2ValueTable.commonInfos[ep][i].startTime);
    emberAfPriceClusterPrintln("  [%d]: tariffType: 0x%X", i, info.co2ValueTable.co2Values[ep][i].tariffType);
    emberAfPriceClusterPrintln("  [%d]: co2Value: 0x%4x", i, info.co2ValueTable.co2Values[ep][i].co2Value);
    emberAfPriceClusterPrintln("  [%d]: co2ValueUnit: 0x%x", i, info.co2ValueTable.co2Values[ep][i].co2ValueUnit);
    emberAfPriceClusterPrintln("  [%d]: co2ValueTrailingDigit: 0x%X", i, info.co2ValueTable.co2Values[ep][i].co2ValueTrailingDigit);  
  }
#endif // defined(EMBER_AF_PRINT_ENABLE) && defined(EMBER_AF_PRINT_PRICE_CLUSTER)
}

void emberAfPrintTierLabelsTable( uint8_t endpoint )
{
#if defined(EMBER_AF_PRINT_ENABLE) && defined(EMBER_AF_PRINT_PRICE_CLUSTER)
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX) {
    return;
  }
  uint8_t i = 0;
  uint8_t j;
  emberAfPriceClusterPrintln("= Tier Labels =");
  for ( i=0; i<EMBER_AF_PLUGIN_PRICE_SERVER_TIER_LABELS_TABLE_SIZE; i++){
    emberAfPriceClusterPrintln("  [%d]: valid: 0x%X", i, info.tierLabelTable.entry[ep][i].valid);
    emberAfPriceClusterPrintln("  [%d]: providerId: 0x%4x", i, info.tierLabelTable.entry[ep][i].providerId);
    emberAfPriceClusterPrintln("  [%d]: issuerEventId: 0x%4x", i, info.tierLabelTable.entry[ep][i].issuerEventId);
    emberAfPriceClusterPrintln("  [%d]: issuerTariffId: 0x%4x", i, info.tierLabelTable.entry[ep][i].issuerTariffId);
    for( j=0; j<info.tierLabelTable.entry[ep][i].numberOfTiers; j++ ){
      emberAfPriceClusterPrintln("  [%d]: tierId[%d]: 0x%X", i, j, info.tierLabelTable.entry[ep][i].tierIds[j]);
      emberAfPriceClusterPrint("  [%d]: tierLabel[%d]: ", i, j);  
      emberAfPriceClusterPrintString(info.tierLabelTable.entry[ep][i].tierLabels[j]);
      emberAfPriceClusterPrintln("");
    }
  }
#endif // defined(EMBER_AF_PRINT_ENABLE) && defined(EMBER_AF_PRINT_PRICE_CLUSTER)
}

void emberAfPrintCalorificValuesTable( uint8_t endpoint )
{
#if defined(EMBER_AF_PRINT_ENABLE) && defined(EMBER_AF_PRINT_PRICE_CLUSTER)
  uint8_t i;
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX) {
    return;
  }
  emberAfPriceClusterPrintln("= Calorific Values =");
  for ( i=0; i<EMBER_AF_PLUGIN_PRICE_SERVER_CALORIFIC_VALUE_TABLE_SIZE; i++){
    emberAfPriceClusterPrintln("  [%d]: startTime: 0x%4x", i, info.calorificValueTable.commonInfos[ep][i].startTime);
    emberAfPriceClusterPrintln("  [%d]: valid: 0x%4x", i, info.calorificValueTable.commonInfos[ep][i].valid);
    emberAfPriceClusterPrintln("  [%d]: issuerEventId: 0x%X", i, info.calorificValueTable.commonInfos[ep][i].issuerEventId);
    emberAfPriceClusterPrintln("  [%d]: calorificValue: 0x%4x", i, info.calorificValueTable.calorificValues[ep][i].calorificValue);
    emberAfPriceClusterPrintln("  [%d]: calorificValueUnit: 0x%X", i, info.calorificValueTable.calorificValues[ep][i].calorificValueUnit);  
    emberAfPriceClusterPrintln("  [%d]: calorificValueTrailingDigit: 0x%X", i, info.calorificValueTable.calorificValues[ep][i].calorificValueTrailingDigit);  
  }
#endif // defined(EMBER_AF_PRINT_ENABLE) && defined(EMBER_AF_PRINT_PRICE_CLUSTER)
}

void emberAfPriceClusterServerInitCallback(uint8_t endpoint)
{
  // set the first entry in the price table
  EmberAfScheduledPrice price;
  price.providerId = 0x00000001;

  // label of "Normal"
  price.rateLabel[0] = 6;
  price.rateLabel[1] = 'N';
  price.rateLabel[2] = 'o';
  price.rateLabel[3] = 'r';
  price.rateLabel[4] = 'm';
  price.rateLabel[5] = 'a';
  price.rateLabel[6] = 'l';

  // first event
  price.issuerEventID= 0x00000001;

  price.unitOfMeasure = EMBER_ZCL_AMI_UNIT_OF_MEASURE_KILO_WATT_HOURS;

  // this is USD = US dollars
  price.currency = 840;

  // top nibble means 2 digits to right of decimal point
  // bottom nibble the current price tier.
  // Valid values are from 1-15 (0 is not valid)
  // and correspond to the tier labels, 1-15.
  price.priceTrailingDigitAndTier = 0x21;

  // initialize the numberOfPriceTiersAndTier
  price.numberOfPriceTiersAndTier =
    (EMBER_AF_PLUGIN_PRICE_SERVER_PRICE_TABLE_SIZE << 4) + 0x00;

  // start time is 0, so it is always valid
  price.startTime = 0x00000000;

  // valid for 1 hr = 60 minutes
  price.duration = 60;

  // price is 0.09 per Kw/Hr
  // we set price as 9 and two digits to right of decimal
  price.price = 9;

  // the next fields arent used
  price.priceRatio = 0xFF;
  price.generationPrice = 0xFFFFFFFFUL;
  price.generationPriceRatio = 0xFF;
  price.alternateCostDelivered = 0xFFFFFFFFUL;
  price.alternateCostUnit = 0xFF;
  price.alternateCostTrailingDigit = 0xFF;
  price.numberOfBlockThresholds = 0xFF;
  price.priceControl = 0x00;

  emberAfPriceSetPriceTableEntry(endpoint, 0, &price);

  partner.index = ZCL_PRICE_INVALID_INDEX;

  // Initialize the price server tick
  emberAfPriceClusterServerInitTick();

  // Clear the block period table.
  emberAfPriceClearBlockPeriodTable( endpoint );

  // Clear the tariff table (and hence, initialize it)
  emberAfPriceClearTariffTable(endpoint);

  // Same with Tier Label table
  emberAfPriceClearTierLabelTable(endpoint);

  // Do likewise with the price matrix table
  emberAfPriceClearPriceMatrixTable(endpoint);

  // And also the block thresholds
  emberAfPriceClearBlockThresholdsTable(endpoint);

  // Clear the Calorific Value table
  emberAfPriceClearCalorificValueTable( endpoint );

  // Clear the CO2 Value table.
  emberAfPluginPriceServerClearCo2Value( endpoint );
  // Clear the Consolidated bills table
  emberAfPriceClearConsolidatedBillsTable( endpoint );

  // Clear the Conversion Factor table
  emberAfPriceClearConversionFactorTable( endpoint );

  // Clear the Billing Period table
  emberAfPriceClearBillingPeriodTable( endpoint );
  
  // Invalidate the tariff cancellation
  emberAfPriceClearCancelTariffTable( endpoint );

  // Invalidate the currency conversion
  emberAfPriceClearCurrencyConversionTable( endpoint );
}

static uint32_t emAfPriceServerSecondsUntilNextGetScheduledPrices;  // Time until next Get Scheduled Prices should be sent.

void emberAfPriceServerSendGetScheduledPrices( uint8_t endpoint ){
  EmberAfScheduledPrice price;
  bool isCurrentOrScheduled;
  uint8_t ep;

  ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if (ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX) {
  //  emberAfPriceClusterPrintln("== ABORT, ep=0xFF, endpoint=%d", endpoint );
    emAfPriceServerSecondsUntilNextGetScheduledPrices = PRICE_EVENT_TIME_NO_PENDING_EVENTS;
    return;
  }

  while( partner.index < EMBER_AF_PLUGIN_PRICE_SERVER_PRICE_TABLE_SIZE ){
    isCurrentOrScheduled = priceIsCurrentOrScheduled(&priceTable[ep][partner.index],
                                                             partner.startTime);
    partner.index++;
    if (isCurrentOrScheduled) {
      emberAfPriceClusterPrintln("TX price at index %X", partner.index - 1);
      emberAfPriceGetPriceTableEntry(endpoint, partner.index - 1, &price);
      fillPublishPriceCommand(price);

      // Rewrite the sequence number of the response so it matches the request.
      appResponseData[1] = partner.sequence;
      if (partner.isIntraPan) {
        emberAfSetCommandEndpoints(partner.pan.intra.serverEndpoint,
                                   partner.pan.intra.clientEndpoint);
        emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, partner.pan.intra.nodeId);
      } else {
        emberAfSendCommandInterPan(partner.pan.inter.panId,
                                   partner.pan.inter.eui64,
                                   EMBER_NULL_NODE_ID,
                                   0, // multicast id - unused
                                   SE_PROFILE_ID);
      }

      partner.numberOfEvents--;
      break;
    }
  }

  if( (partner.numberOfEvents != 0) &&
      (partner.index < EMBER_AF_PLUGIN_PRICE_SERVER_PRICE_TABLE_SIZE) ){
//      minEventDelayQSec = 0;    //MILLISECOND_TICKS_PER_QUARTERSECOND;
      emAfPriceServerSecondsUntilNextGetScheduledPrices = 0;
  }
  else{
    partner.index = ZCL_PRICE_INVALID_INDEX;
//    minEventDelayQSec = PRICE_EVENT_TIME_NO_PENDING_EVENTS;
    emAfPriceServerSecondsUntilNextGetScheduledPrices = PRICE_EVENT_TIME_NO_PENDING_EVENTS;
  }
}

uint32_t emberAfPriceServerSecondsUntilGetScheduledPricesEvent(){
  return emAfPriceServerSecondsUntilNextGetScheduledPrices;
}

static void emAfPriceServerScheduleGetScheduledPrices( uint8_t endpoint ){
  emAfPriceServerSecondsUntilNextGetScheduledPrices = 0;
  emberAfPriceClusterScheduleTickCallback( endpoint, EMBER_AF_PRICE_SERVER_GET_SCHEDULED_PRICES_EVENT_MASK );
}


bool emberAfPriceClusterGetCurrentPriceCallback(uint8_t commandOptions)
{
  EmberAfScheduledPrice price;
  emberAfPriceClusterPrintln("RX: GetCurrentPrice 0x%X", commandOptions);
  if (!emberAfGetCurrentPrice(emberAfCurrentEndpoint(), &price)) {
    emberAfPriceClusterPrintln("no valid price to return!");
    emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_NOT_FOUND);
  } else {
    fillPublishPriceCommand(price); 
    emberAfSendResponse();
  }
  return true;
}

bool emberAfPriceClusterGetScheduledPricesCallback(uint32_t startTime,
                                                      uint8_t numberOfEvents)
{
  EmberAfClusterCommand *cmd = emberAfCurrentCommand();
  uint8_t endpoint = emberAfCurrentEndpoint();
  uint8_t scheduledPrices = 0;

  emberAfPriceClusterPrintln("RX: GetScheduledPrices 0x%4x, 0x%X",
                             startTime,
                             numberOfEvents);

  // Only one GetScheduledPrices can be processed at a time.
  if (partner.index != ZCL_PRICE_INVALID_INDEX) {
    emberAfSendDefaultResponse(cmd, EMBER_ZCL_STATUS_FAILURE);
    return true;
  }

  partner.startTime = (startTime == ZCL_PRICE_CLUSTER_START_TIME_NOW
                       ? emberAfGetCurrentTime()
                       : startTime);
  scheduledPrices = scheduledPriceCount(endpoint, partner.startTime);

  if (numberOfEvents == ZCL_PRICE_CLUSTER_NUMBER_OF_EVENTS_ALL){
    partner.numberOfEvents = scheduledPrices;
  } else {
    partner.numberOfEvents = (scheduledPrices > numberOfEvents)
                             ? numberOfEvents:
                               scheduledPrices;
  }

  if (partner.numberOfEvents == 0) {
    emberAfPriceClusterPrintln("no valid price to return!");
    emberAfSendDefaultResponse(cmd, EMBER_ZCL_STATUS_NOT_FOUND);
  } else {
    partner.isIntraPan = (cmd->interPanHeader == NULL);
    if (partner.isIntraPan) {
      partner.pan.intra.nodeId = cmd->source;
      partner.pan.intra.clientEndpoint = cmd->apsFrame->sourceEndpoint;
      partner.pan.intra.serverEndpoint = cmd->apsFrame->destinationEndpoint;
    } else {
      partner.pan.inter.panId = cmd->interPanHeader->panId;
      MEMMOVE(partner.pan.inter.eui64, cmd->interPanHeader->longAddress, EUI64_SIZE);
    }
    partner.sequence = cmd->seqNum;
    partner.index = 0;

    emAfPriceServerScheduleGetScheduledPrices( emberAfCurrentEndpoint() );
  }
  return true;
}

void emberAfPluginPriceServerPublishPriceMessage(EmberNodeId nodeId,
                                                 uint8_t srcEndpoint,
                                                 uint8_t dstEndpoint,
                                                 uint8_t priceIndex)
{
  EmberStatus status;
  EmberAfScheduledPrice price;
  uint8_t ep = emberAfFindClusterServerEndpointIndex(srcEndpoint, ZCL_PRICE_CLUSTER_ID);

  if (ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX) {
    return;
  }

  if (!emberAfPriceGetPriceTableEntry(srcEndpoint, priceIndex, &price)) {
    emberAfPriceClusterPrintln("Invalid price table entry at index %X", priceIndex);
    return;
  }
  emberAfPriceClusterPrintln("Filling cluster");
  fillPublishPriceCommand(price);

  emberAfSetCommandEndpoints(srcEndpoint, dstEndpoint);
  emberAfGetCommandApsFrame()->options |= EMBER_APS_OPTION_SOURCE_EUI64;
  status = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, nodeId);
  if(status != EMBER_SUCCESS) {
    emberAfPriceClusterPrintln("Error in publish price %X", status);
  }
}

void emberAfPluginPriceServerPublishTariffMessage(EmberNodeId nodeId,
                                                  uint8_t srcEndpoint,
                                                  uint8_t dstEndpoint,
                                                  uint8_t tariffIndex)
{
  EmberStatus status;
  EmberAfScheduledTariff tariff;
  EmberAfPriceCommonInfo info;

  uint8_t ep = emberAfFindClusterServerEndpointIndex(srcEndpoint, ZCL_PRICE_CLUSTER_ID);

  if (ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX) {
    return;
  }

  if (!emberAfPriceGetTariffTableEntry(srcEndpoint, tariffIndex, &info, &tariff)
      || (!info.valid)) {
    emberAfPriceClusterPrintln("Invalid tariff table entry at index %X", tariffIndex);
    return;
  }

  emberAfFillCommandPriceClusterPublishTariffInformation(tariff.providerId,
                                                         info.issuerEventId,
                                                         tariff.issuerTariffId,
                                                         info.startTime,
                                                         tariff.tariffTypeChargingScheme,
                                                         tariff.tariffLabel,
                                                         tariff.numberOfPriceTiersInUse,
                                                         tariff.numberOfBlockThresholdsInUse,
                                                         tariff.unitOfMeasure,
                                                         tariff.currency,
                                                         tariff.priceTrailingDigit,
                                                         tariff.standingCharge,
                                                         tariff.tierBlockMode,
                                                         tariff.blockThresholdMultiplier,
                                                         tariff.blockThresholdDivisor);

  emberAfSetCommandEndpoints(srcEndpoint, dstEndpoint);
  emberAfGetCommandApsFrame()->options |= EMBER_APS_OPTION_SOURCE_EUI64;
  status = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, nodeId);
  if(status != EMBER_SUCCESS) {
    emberAfPriceClusterPrintln("Error in publish tariff %X", status);
  }
}

// Look through the price table to find the first free/unused entry;
// return ZCL_PRICE_INVALID_INDEX if price table is full
uint8_t emberAfPriceFindFreePriceIndex(uint8_t endpoint)
{
  uint8_t i;
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint,
                                                   ZCL_PRICE_CLUSTER_ID);
  if (ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX) {
    return ZCL_PRICE_INVALID_INDEX;
  }

  for (i = 0; i < EMBER_AF_PLUGIN_PRICE_SERVER_PRICE_TABLE_SIZE; i++) {
    if (!priceIsCurrentOrScheduled(&priceTable[ep][i],
                                   emberAfGetCurrentTime())) {
      return i;
    }
  }

  emberAfPriceClusterPrintln("Error: Price table full");
  return ZCL_PRICE_INVALID_INDEX;
}

// send replies
void sendValidCmdEntries(uint8_t cmdId,
                         uint8_t ep,
                         uint8_t* validEntries,
                         uint8_t validEntryCount){
  uint8_t i, j;
  uint8_t sentCmd = 0;
  if (validEntryCount == 0){
    goto kickout;
  }

  for (i=0; i<validEntryCount; i++){
    uint8_t tableIndex = validEntries[i];

    // discard command entries. this is used to satisfy additional logic 
    // required by command such as CO2Value
    if (tableIndex == 0xFF){ 
      continue;
    }

    if (cmdId == ZCL_PUBLISH_CONVERSION_FACTOR_COMMAND_ID){
      emberAfPriceClusterPrintln("Sending conversion factor: table[%d]", tableIndex);
      emberAfFillCommandPriceClusterPublishConversionFactor(info.conversionFactorTable.commonInfos[ep][tableIndex].issuerEventId,
                                                            info.conversionFactorTable.commonInfos[ep][tableIndex].startTime,
                                                            info.conversionFactorTable.priceConversionFactors[ep][tableIndex].conversionFactor,
                                                            info.conversionFactorTable.priceConversionFactors[ep][tableIndex].conversionFactorTrailingDigit);
    } else if (cmdId == ZCL_PUBLISH_CALORIFIC_VALUE_COMMAND_ID){
      emberAfPriceClusterPrintln("Sending calorific value: table[%d]", tableIndex);
      emberAfFillCommandPriceClusterPublishCalorificValue(info.calorificValueTable.commonInfos[ep][tableIndex].issuerEventId,
                                                          info.calorificValueTable.commonInfos[ep][tableIndex].startTime,
                                                          info.calorificValueTable.calorificValues[ep][tableIndex].calorificValue,
                                                          info.calorificValueTable.calorificValues[ep][tableIndex].calorificValueUnit,
                                                          info.calorificValueTable.calorificValues[ep][tableIndex].calorificValueTrailingDigit);
    } else if (cmdId == ZCL_PUBLISH_C_O2_VALUE_COMMAND_ID){
      emberAfPriceClusterPrintln("Sending Co2 value: table[%d]", tableIndex);
      emberAfFillCommandPriceClusterPublishCO2Value(info.co2ValueTable.co2Values[ep][tableIndex].providerId,
                                                    info.co2ValueTable.commonInfos[ep][tableIndex].issuerEventId,
                                                    info.co2ValueTable.commonInfos[ep][tableIndex].startTime,
                                                    info.co2ValueTable.co2Values[ep][tableIndex].tariffType,
                                                    info.co2ValueTable.co2Values[ep][tableIndex].co2Value,
                                                    info.co2ValueTable.co2Values[ep][tableIndex].co2ValueUnit,
                                                    info.co2ValueTable.co2Values[ep][tableIndex].co2ValueTrailingDigit);
    } else if( cmdId == ZCL_PUBLISH_TIER_LABELS_COMMAND_ID ){
      emberAfPriceClusterPrintln("Sending tier labels: table[%d]", tableIndex);
      emberAfFillExternalBuffer((ZCL_CLUSTER_SPECIFIC_COMMAND \
                             | ZCL_FRAME_CONTROL_SERVER_TO_CLIENT), \
                            ZCL_PRICE_CLUSTER_ID, \
                            ZCL_PUBLISH_TIER_LABELS_COMMAND_ID, \
                            "wwwuuu", \
                            info.tierLabelTable.entry[ep][tableIndex].providerId, \
                            info.tierLabelTable.entry[ep][tableIndex].issuerEventId, \
                            info.tierLabelTable.entry[ep][tableIndex].issuerTariffId, \
                            0, \
                            0, \
                            info.tierLabelTable.entry[ep][tableIndex].numberOfTiers );
      for( j=0; j<info.tierLabelTable.entry[ep][tableIndex].numberOfTiers; j++ ){
        emberAfPutInt8uInResp( info.tierLabelTable.entry[ep][tableIndex].tierIds[j] );
        emberAfPutStringInResp( info.tierLabelTable.entry[ep][tableIndex].tierLabels[j] );
      }
    } else if (cmdId == ZCL_PUBLISH_BILLING_PERIOD_COMMAND_ID){
      emberAfPriceClusterPrintln("Sending billing period: table[%d]", tableIndex);
      emberAfFillCommandPriceClusterPublishBillingPeriod(info.billingPeriodTable.billingPeriods[ep][tableIndex].providerId,
                                                    info.billingPeriodTable.commonInfos[ep][tableIndex].issuerEventId,
                                                    info.billingPeriodTable.billingPeriods[ep][tableIndex].rawBillingPeriodStartTime,
                                                    info.billingPeriodTable.billingPeriods[ep][tableIndex].billingPeriodDuration,
                                                    info.billingPeriodTable.billingPeriods[ep][tableIndex].billingPeriodDurationType,
                                                    info.billingPeriodTable.billingPeriods[ep][tableIndex].tariffType);
    } else if( cmdId == ZCL_PUBLISH_CONSOLIDATED_BILL_COMMAND_ID ){
      emberAfPriceClusterPrintln("Sending Consolidated Bill: table[%d]", tableIndex);
      emberAfFillCommandPriceClusterPublishConsolidatedBill( info.consolidatedBillsTable.consolidatedBills[ep][tableIndex].providerId,
                                                             info.consolidatedBillsTable.commonInfos[ep][tableIndex].issuerEventId,
                                                             info.consolidatedBillsTable.consolidatedBills[ep][tableIndex].rawStartTimeUtc,
                                                             info.consolidatedBillsTable.consolidatedBills[ep][tableIndex].billingPeriodDuration,
                                                             info.consolidatedBillsTable.consolidatedBills[ep][tableIndex].billingPeriodDurationType,
                                                             info.consolidatedBillsTable.consolidatedBills[ep][tableIndex].tariffType,
                                                             info.consolidatedBillsTable.consolidatedBills[ep][tableIndex].consolidatedBill,
                                                             info.consolidatedBillsTable.consolidatedBills[ep][tableIndex].currency,
                                                             info.consolidatedBillsTable.consolidatedBills[ep][tableIndex].billTrailingDigit );
    } else if( cmdId == ZCL_PUBLISH_CREDIT_PAYMENT_COMMAND_ID ){
      emberAfPriceClusterPrintln("Sending Credit Payment: table[%d]", tableIndex);
      emberAfFillCommandPriceClusterPublishCreditPayment( info.creditPaymentTable.creditPayment[ep][tableIndex].providerId,
                                                          info.creditPaymentTable.commonInfos[ep][tableIndex].issuerEventId,
                                                          info.creditPaymentTable.creditPayment[ep][tableIndex].creditPaymentDueDate,
                                                          info.creditPaymentTable.creditPayment[ep][tableIndex].creditPaymentAmountOverdue,
                                                          info.creditPaymentTable.creditPayment[ep][tableIndex].creditPaymentStatus,
                                                          info.creditPaymentTable.creditPayment[ep][tableIndex].creditPayment,
                                                          info.creditPaymentTable.creditPayment[ep][tableIndex].creditPaymentDate,
                                                          info.creditPaymentTable.creditPayment[ep][tableIndex].creditPaymentRef );
    } else if( cmdId == ZCL_PUBLISH_BLOCK_PERIOD_COMMAND_ID ){
      emberAfPriceClusterPrintln("Sending Block Period: table[%d]", tableIndex);
      emberAfFillCommandPriceClusterPublishBlockPeriod( info.blockPeriodTable.blockPeriods[ep][tableIndex].providerId,
                                                        info.blockPeriodTable.commonInfos[ep][tableIndex].issuerEventId,
                                                        info.blockPeriodTable.blockPeriods[ep][tableIndex].rawBlockPeriodStartTime,
                                                        info.blockPeriodTable.blockPeriods[ep][tableIndex].blockPeriodDuration,
                                                        info.blockPeriodTable.blockPeriods[ep][tableIndex].blockPeriodControl,
                                                        info.blockPeriodTable.blockPeriods[ep][tableIndex].blockPeriodDurationType,
                                                        info.blockPeriodTable.blockPeriods[ep][tableIndex].tariffType,
                                                        info.blockPeriodTable.blockPeriods[ep][tableIndex].tariffResolutionPeriod );
    } else if (cmdId == ZCL_PUBLISH_TARIFF_INFORMATION_COMMAND_ID ){
      EmberAfScheduledTariff * tariff = &info.scheduledTariffTable.scheduledTariffs[ep][tableIndex];
      EmberAfPriceCommonInfo * infos = &info.scheduledTariffTable.commonInfos[ep][tableIndex];

      emberAfPriceClusterPrintln("TX: PublishTariffInfo: table[%d]", tableIndex);
      emberAfFillCommandPriceClusterPublishTariffInformation(tariff->providerId,
                                                             infos->issuerEventId,
                                                             tariff->issuerTariffId,
                                                             infos->startTime,
                                                             tariff->tariffTypeChargingScheme,
                                                             tariff->tariffLabel,
                                                             tariff->numberOfPriceTiersInUse,
                                                             tariff->numberOfBlockThresholdsInUse,
                                                             tariff->unitOfMeasure,
                                                             tariff->currency,
                                                             tariff->priceTrailingDigit,
                                                             tariff->standingCharge,
                                                             tariff->tierBlockMode,
                                                             tariff->blockThresholdMultiplier,
                                                             tariff->blockThresholdDivisor);

    } else { goto kickout; }

    emberAfSendResponse();
    sentCmd++;
    emberAfPriceClusterPrintln("  sent=%d", sentCmd );
  }

  if (sentCmd == 0){
kickout:    emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_NOT_FOUND);
  }
}


// CONVERSION FACTOR
static void emberAfPriceClearConversionFactorTable( uint8_t endpoint ){
  uint8_t i;
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return;
  }
  for( i=0; i<EMBER_AF_PLUGIN_PRICE_SERVER_CONVERSION_FACTOR_TABLE_SIZE; i++ ){
    info.conversionFactorTable.commonInfos[ep][i].valid = false;
  }
}


bool emberAfPriceClusterGetConversionFactorCallback(uint32_t earliestStartTime,
                                                       uint32_t minIssuerEventId,
                                                       uint8_t numberOfRequestedCommands)
{
  uint8_t cmdCount;
  uint8_t validCmds[EMBER_AF_PLUGIN_PRICE_SERVER_CONVERSION_FACTOR_TABLE_SIZE];
  uint8_t endpoint = emberAfCurrentEndpoint();
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return false;
  }
  cmdCount = emberAfPluginPriceCommonFindValidEntries(validCmds,
                                      EMBER_AF_PLUGIN_PRICE_SERVER_CONVERSION_FACTOR_TABLE_SIZE,
                                      info.conversionFactorTable.commonInfos[ep],
                                      earliestStartTime,
                                      minIssuerEventId,
                                      numberOfRequestedCommands);

  sendValidCmdEntries(ZCL_PUBLISH_CONVERSION_FACTOR_COMMAND_ID, ep,
                      validCmds,
                      cmdCount);
  return true;
}


EmberAfStatus emberAfPluginPriceServerConversionFactorAdd(uint8_t endpoint,
                                                          uint32_t issuerEventId,
                                                          uint32_t startTime,
                                                          uint32_t conversionFactor,
                                                          uint8_t conversionFactorTrailingDigit)
{
  EmberAfStatus status = EMBER_ZCL_STATUS_SUCCESS;
  uint8_t index;
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    emberAfPriceClusterPrintln("Error: Index endpoint!");
    status = EMBER_ZCL_STATUS_INVALID_FIELD;
    goto kickout;
  }
  index = emberAfPluginPriceCommonGetCommonMatchingOrUnusedIndex( info.conversionFactorTable.commonInfos[ep],
                                                      EMBER_AF_PLUGIN_PRICE_SERVER_CONVERSION_FACTOR_TABLE_SIZE,
                                                      issuerEventId, startTime, true );

  if (index >= EMBER_AF_PLUGIN_PRICE_SERVER_CONVERSION_FACTOR_TABLE_SIZE){
    emberAfPriceClusterPrintln("Error: Index out of bound!");
    status = EMBER_ZCL_STATUS_INVALID_FIELD;
    goto kickout;
  }

  info.conversionFactorTable.commonInfos[ep][index].startTime = startTime;
  info.conversionFactorTable.commonInfos[ep][index].valid = true;
  info.conversionFactorTable.commonInfos[ep][index].issuerEventId = issuerEventId;
  info.conversionFactorTable.commonInfos[ep][index].durationSec = ZCL_PRICE_CLUSTER_DURATION_SEC_UNTIL_CHANGED;
  info.conversionFactorTable.priceConversionFactors[ep][index].conversionFactor = conversionFactor;
  info.conversionFactorTable.priceConversionFactors[ep][index].conversionFactorTrailingDigit = conversionFactorTrailingDigit;
  emberAfPluginPriceCommonSort( info.conversionFactorTable.commonInfos[ep],
                    (uint8_t *)info.conversionFactorTable.priceConversionFactors[ep],
                    sizeof(EmberAfPriceConversionFactor),
                    EMBER_AF_PLUGIN_PRICE_SERVER_CONVERSION_FACTOR_TABLE_SIZE );
  emberAfPluginPriceCommonUpdateDurationForOverlappingEvents( info.conversionFactorTable.commonInfos[ep],
                                                  EMBER_AF_PLUGIN_PRICE_SERVER_CONVERSION_FACTOR_TABLE_SIZE );

  if (startTime <= emberAfGetCurrentTime()){
    emberAfPriceUpdateConversionFactorAttribs( endpoint, conversionFactor, conversionFactorTrailingDigit );
  }
  emberAfPriceClusterScheduleTickCallback( endpoint, EMBER_AF_PRICE_SERVER_CHANGE_CONVERSION_FACTOR_EVENT_MASK );
  
kickout:
  return status;
}

static void emberAfPriceUpdateConversionFactorAttribs( uint8_t endpoint, uint32_t conversionFactor, uint8_t conversionFactorTrailingDigit ){
  // Assumes the conversion factor table is already sorted.
  emberAfWriteServerAttribute(endpoint,
                              ZCL_PRICE_CLUSTER_ID,
                              ZCL_CONVERSION_FACTOR_ATTRIBUTE_ID,
                              (uint8_t *)&conversionFactor,
                              ZCL_INT32U_ATTRIBUTE_TYPE );
  emberAfWriteServerAttribute(endpoint,
                              ZCL_PRICE_CLUSTER_ID,
                              ZCL_CONVERSION_FACTOR_TRAILING_DIGIT_ATTRIBUTE_ID,
                              (uint8_t *)&conversionFactorTrailingDigit,
                              ZCL_INT8U_ATTRIBUTE_TYPE );
}

uint32_t emberAfPriceServerSecondsUntilConversionFactorEvent( uint8_t endpoint ){
  uint32_t secondsTillNext;
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return 0xFFFFFFFF;
  }

  secondsTillNext = emberAfPluginPriceCommonSecondsUntilSecondIndexActive( info.conversionFactorTable.commonInfos[ep],
                                                               EMBER_AF_PLUGIN_PRICE_SERVER_CONVERSION_FACTOR_TABLE_SIZE );

  return secondsTillNext;

}

void emberAfPriceServerRefreshConversionFactor(uint8_t endpoint){
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);

  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return;
  }

  if( 0 == emberAfPluginPriceCommonSecondsUntilSecondIndexActive( info.conversionFactorTable.commonInfos[ep],
                                                      EMBER_AF_PLUGIN_PRICE_SERVER_CONVERSION_FACTOR_TABLE_SIZE ) ){
    // Replace current with next.
    info.conversionFactorTable.commonInfos[ep][0].valid = 0;
    emberAfPluginPriceCommonSort( info.conversionFactorTable.commonInfos[ep],
                      (uint8_t *)info.conversionFactorTable.priceConversionFactors[ep],
                      sizeof(EmberAfPriceConversionFactor),
                      EMBER_AF_PLUGIN_PRICE_SERVER_CONVERSION_FACTOR_TABLE_SIZE );
    emberAfPluginPriceCommonUpdateDurationForOverlappingEvents( info.conversionFactorTable.commonInfos[ep],
                                                  EMBER_AF_PLUGIN_PRICE_SERVER_CONVERSION_FACTOR_TABLE_SIZE );
    if( info.conversionFactorTable.commonInfos[ep][0].valid ){
      emberAfPriceUpdateConversionFactorAttribs( endpoint,
                                                 info.conversionFactorTable.priceConversionFactors[ep][0].conversionFactor,
                                                 info.conversionFactorTable.priceConversionFactors[ep][0].conversionFactorTrailingDigit );
    }
  }
}

// CALORIFIC VALUE FUNCTIONS
static void emberAfPriceClearCalorificValueTable( uint8_t endpoint ){
  uint8_t i;
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return;
  }
  for( i=0; i<EMBER_AF_PLUGIN_PRICE_SERVER_CALORIFIC_VALUE_TABLE_SIZE; i++ ){
    info.calorificValueTable.commonInfos[ep][i].valid = false;
  }
}



bool emberAfPriceClusterGetCalorificValueCallback(uint32_t earliestStartTime,
                                                     uint32_t minIssuerEventId,
                                                     uint8_t numberOfRequestedCommands)
{
  uint8_t cmdCount;
  uint8_t validCmds[EMBER_AF_PLUGIN_PRICE_SERVER_CALORIFIC_VALUE_TABLE_SIZE];
  uint8_t endpoint = emberAfCurrentEndpoint();
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return false;
  }
  cmdCount = emberAfPluginPriceCommonFindValidEntries(validCmds,
                                       EMBER_AF_PLUGIN_PRICE_SERVER_CALORIFIC_VALUE_TABLE_SIZE,
                                       info.calorificValueTable.commonInfos[ep],
                                       earliestStartTime,
                                       minIssuerEventId, 
                                       numberOfRequestedCommands);

  sendValidCmdEntries(ZCL_PUBLISH_CALORIFIC_VALUE_COMMAND_ID, ep,
                      validCmds,
                      cmdCount);
  return true;
}

EmberAfStatus emberAfPluginPriceServerCalorificValueAdd(uint8_t endpoint, 
                                                        uint32_t issuerEventId,
                                                        uint32_t startTime,
                                                        uint32_t calorificValue,
                                                        uint8_t calorificValueUnit,
                                                        uint8_t calorificValueTrailingDigit)
{
  //Lint requires initialization.
  EmberAfStatus status = EMBER_ZCL_STATUS_SUCCESS;
  uint8_t index;
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    emberAfPriceClusterPrintln("Error: Invalid endpoint!");
    status = EMBER_ZCL_STATUS_INVALID_FIELD;
    goto kickout;
  }

  index = emberAfPluginPriceCommonGetCommonMatchingOrUnusedIndex(info.calorificValueTable.commonInfos[ep],
                                                     EMBER_AF_PLUGIN_PRICE_SERVER_CALORIFIC_VALUE_TABLE_SIZE,
                                                     issuerEventId, startTime, true );

  if (index >= EMBER_AF_PLUGIN_PRICE_SERVER_CALORIFIC_VALUE_TABLE_SIZE){
    emberAfPriceClusterPrintln("Error: Index out of bound!");
    status = EMBER_ZCL_STATUS_INVALID_FIELD;
    goto kickout;
  }

  info.calorificValueTable.commonInfos[ep][index].startTime = startTime;
  info.calorificValueTable.commonInfos[ep][index].valid = true;
  info.calorificValueTable.commonInfos[ep][index].issuerEventId = issuerEventId;
  info.calorificValueTable.commonInfos[ep][index].durationSec = ZCL_PRICE_CLUSTER_DURATION_SEC_UNTIL_CHANGED;
  info.calorificValueTable.calorificValues[ep][index].calorificValue = calorificValue;
  info.calorificValueTable.calorificValues[ep][index].calorificValueUnit = calorificValueUnit;
  info.calorificValueTable.calorificValues[ep][index].calorificValueTrailingDigit = calorificValueTrailingDigit;
  emberAfPluginPriceCommonSort( info.calorificValueTable.commonInfos[ep],
                    (uint8_t *)info.calorificValueTable.calorificValues[ep],
                    sizeof(EmberAfPriceCalorificValue),
                    EMBER_AF_PLUGIN_PRICE_SERVER_CALORIFIC_VALUE_TABLE_SIZE );
  emberAfPluginPriceCommonUpdateDurationForOverlappingEvents( info.calorificValueTable.commonInfos[ep],
                                                  EMBER_AF_PLUGIN_PRICE_SERVER_CALORIFIC_VALUE_TABLE_SIZE );


  if (startTime <= emberAfGetCurrentTime()){
    emberAfPriceUpdateCalorificValueAttribs( endpoint, calorificValue, calorificValueUnit, calorificValueTrailingDigit );
  }
  emberAfPriceClusterScheduleTickCallback( endpoint, EMBER_AF_PRICE_SERVER_CHANGE_CALORIFIC_VALUE_EVENT_MASK );
kickout:
  return status;
}


static void emberAfPriceUpdateCalorificValueAttribs( uint8_t endpoint, uint32_t calorificValue, 
                                                     uint8_t calorificValueUnit, uint8_t calorificValueTrailingDigit ){
  // Assumes the conversion factor table is already sorted.
    emberAfWriteServerAttribute(endpoint,
                                ZCL_PRICE_CLUSTER_ID,
                                ZCL_CALORIFIC_VALUE_ATTRIBUTE_ID,
                                (uint8_t *)&calorificValue,
                                ZCL_INT32U_ATTRIBUTE_TYPE);

    emberAfWriteServerAttribute(endpoint,
                                ZCL_PRICE_CLUSTER_ID,
                                ZCL_CALORIFIC_VALUE_UNIT_ATTRIBUTE_ID,
                                (uint8_t *)&calorificValueUnit,
                                ZCL_ENUM8_ATTRIBUTE_TYPE);

    emberAfWriteServerAttribute(endpoint,
                                ZCL_PRICE_CLUSTER_ID,
                                ZCL_CALORIFIC_VALUE_TRAILING_DIGIT_ATTRIBUTE_ID,
                                (uint8_t *)&calorificValueTrailingDigit,
                                ZCL_BITMAP8_ATTRIBUTE_TYPE);


}

uint32_t emberAfPriceServerSecondsUntilCalorificValueEvent( uint8_t endpoint ){
  uint32_t secondsTillNext;
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return 0xFFFFFFFF;
  }

  secondsTillNext = emberAfPluginPriceCommonSecondsUntilSecondIndexActive( info.calorificValueTable.commonInfos[ep],
                                                               EMBER_AF_PLUGIN_PRICE_SERVER_CALORIFIC_VALUE_TABLE_SIZE );
  return secondsTillNext;

}

void emberAfPriceServerRefreshCalorificValue( uint8_t endpoint ){
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return;
  }

  if( 0 == emberAfPluginPriceCommonSecondsUntilSecondIndexActive( info.calorificValueTable.commonInfos[ep],
                                                      EMBER_AF_PLUGIN_PRICE_SERVER_CALORIFIC_VALUE_TABLE_SIZE ) ){
    // Replace current with next.
    info.calorificValueTable.commonInfos[ep][0].valid = 0;
    emberAfPluginPriceCommonSort( info.calorificValueTable.commonInfos[ep],
                      (uint8_t *)info.calorificValueTable.calorificValues[ep],
                      sizeof(EmberAfPriceCalorificValue),
                      EMBER_AF_PLUGIN_PRICE_SERVER_CALORIFIC_VALUE_TABLE_SIZE );
    emberAfPluginPriceCommonUpdateDurationForOverlappingEvents( info.calorificValueTable.commonInfos[ep],
                                                    EMBER_AF_PLUGIN_PRICE_SERVER_CALORIFIC_VALUE_TABLE_SIZE );
    if( info.calorificValueTable.commonInfos[ep][0].valid ){
      emberAfPriceUpdateCalorificValueAttribs( endpoint,
                                               info.calorificValueTable.calorificValues[ep][0].calorificValue,
                                               info.calorificValueTable.calorificValues[ep][0].calorificValueUnit,
                                               info.calorificValueTable.calorificValues[ep][0].calorificValueTrailingDigit );
    }
  }
}

void emberAfPluginPriceServerCalorificValueClear( uint8_t endpoint )
{
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return;
  }
  MEMSET(&info.calorificValueTable, 0, sizeof(EmberAfPriceCalorificValueTable));
}

void emberAfPluginPriceServerConversionFactorClear( uint8_t endpoint )
{
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return;
  }
  MEMSET(&info.conversionFactorTable, 0, sizeof(EmberAfPriceConversionFactorTable));
}

void emberAfPluginPriceServerCo2ValueClear( uint8_t endpoint )
{
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return;
  }
  MEMSET(&info.co2ValueTable, 0, sizeof(EmberAfPriceCO2Table));
}

void emberAfPluginPriceServerConversionFactorPub(uint8_t tableIndex,
                                                 EmberNodeId dstAddr,
                                                 uint8_t srcEndpoint,
                                                 uint8_t dstEndpoint)
{
  EmberStatus status;
  uint8_t ep = emberAfFindClusterServerEndpointIndex(srcEndpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return;
  }
  if (tableIndex >= EMBER_AF_PLUGIN_PRICE_SERVER_CONVERSION_FACTOR_TABLE_SIZE){
    emberAfPriceClusterPrintln("Error: index out of bound!");
    return;
  }

  if (info.conversionFactorTable.commonInfos[ep][tableIndex].valid == 0){
    emberAfPriceClusterPrintln("Error: Table entry not valid!");
    return;
  }

  emberAfSetCommandEndpoints(srcEndpoint, dstEndpoint);
  emberAfFillCommandPriceClusterPublishConversionFactor(info.conversionFactorTable.commonInfos[ep][tableIndex].issuerEventId,
                                                        info.conversionFactorTable.commonInfos[ep][tableIndex].startTime,
                                                        info.conversionFactorTable.priceConversionFactors[ep][tableIndex].conversionFactor,
                                                        info.conversionFactorTable.priceConversionFactors[ep][tableIndex].conversionFactorTrailingDigit);
  status = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, dstAddr);

  if (status != EMBER_SUCCESS){
    emberAfPriceClusterPrintln("Unable to send PublishConversionFactor to 0x%2x: 0x%X", dstAddr, status);
  }
}

bool emberAfPriceClusterGetTierLabelsCallback(uint32_t issuerTariffId)
{
  uint8_t i;
  uint8_t validCmds[EMBER_AF_PLUGIN_PRICE_SERVER_TIER_LABELS_TABLE_SIZE];
  uint8_t endpoint = emberAfCurrentEndpoint();
  uint8_t validCmdCnt = 0;
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return false;
  }

  for (i=0; i<EMBER_AF_PLUGIN_PRICE_SERVER_TIER_LABELS_TABLE_SIZE; i++){
    if( (info.tierLabelTable.entry[ep][i].issuerTariffId == issuerTariffId) &&
        (info.tierLabelTable.entry[ep][i].valid) ){
      validCmds[validCmdCnt] = i;
      validCmdCnt++;
    }
  }
  sendValidCmdEntries( ZCL_PUBLISH_TIER_LABELS_COMMAND_ID, ep, validCmds, validCmdCnt );
  return true;
}

static void emberAfPriceClearTierLabelTable( uint8_t endpoint ){
  uint8_t i;
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return;
  }
  for( i=0; i<EMBER_AF_PLUGIN_PRICE_SERVER_TIER_LABELS_TABLE_SIZE; i++ ){
    info.tierLabelTable.entry[ep][i].valid = 0;
    info.tierLabelTable.entry[ep][i].numberOfTiers = 0;
  }
}

void emberAfPluginPriceServerTierLabelSet(uint8_t  endpoint,
                                          uint8_t  index,
                                          uint8_t  valid,
                                          uint32_t providerId,
                                          uint32_t issuerEventId,
                                          uint32_t issuerTariffId,
                                          uint8_t tierId,
                                          uint8_t* tierLabel)
{
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return;
  }
  if (index >= EMBER_AF_PLUGIN_PRICE_SERVER_TIER_LABELS_TABLE_SIZE){
    emberAfPriceClusterPrintln("Error: Index out of bound!");
    return;
  }

  info.tierLabelTable.entry[ep][index].valid = valid;
  info.tierLabelTable.entry[ep][index].providerId = providerId;
  info.tierLabelTable.entry[ep][index].issuerEventId = issuerEventId;
  info.tierLabelTable.entry[ep][index].issuerTariffId = issuerTariffId;;
  info.tierLabelTable.entry[ep][index].numberOfTiers = 1;    // This command forces only 1 label.
  info.tierLabelTable.entry[ep][index].tierIds[0] = tierId;
  // Truncate string length to not exceed maximum length.
  if( tierLabel[0] > TIER_LABEL_SIZE ){
    tierLabel[0] = TIER_LABEL_SIZE;
  }
  MEMCOPY(info.tierLabelTable.entry[ep][index].tierLabels[0], tierLabel, (TIER_LABEL_SIZE+1));
}

void emberAfPluginPriceServerTierLabelAddLabel( uint8_t endpoint, uint32_t issuerTariffId, uint8_t tierId, uint8_t *tierLabel ){
  uint8_t i;
  uint8_t numberOfTiers;
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return;
  }
  for( i=0; i<EMBER_AF_PLUGIN_PRICE_SERVER_TIER_LABELS_TABLE_SIZE; i++ ){

    if( (info.tierLabelTable.entry[ep][i].issuerTariffId == issuerTariffId) &&
        (info.tierLabelTable.entry[ep][i].numberOfTiers < EMBER_AF_PLUGIN_PRICE_SERVER_MAX_TIERS_PER_TARIFF) &&
         info.tierLabelTable.entry[ep][i].valid ){
      // Found matching entry with room for another label.
      // Truncate string length to not exceed maximum length.
      if( tierLabel[0] > TIER_LABEL_SIZE ){
        tierLabel[0] = TIER_LABEL_SIZE;
      }
      numberOfTiers = info.tierLabelTable.entry[ep][i].numberOfTiers;
      info.tierLabelTable.entry[ep][i].tierIds[numberOfTiers] = tierId;
      MEMCOPY(info.tierLabelTable.entry[ep][i].tierLabels[numberOfTiers], tierLabel, (TIER_LABEL_SIZE+1));
      info.tierLabelTable.entry[ep][i].numberOfTiers++;
      break;
    }
  }
  if( i >= EMBER_AF_PLUGIN_PRICE_SERVER_TIER_LABELS_TABLE_SIZE ){
    emberAfPriceClusterPrintln("Error: Tier Label Add failed - available entry not found.");
  }
}




void emberAfPluginPriceServerTierLabelPub(uint16_t nodeId, uint8_t srcEndpoint, uint8_t dstEndpoint, uint8_t index )
{
  EmberStatus status;
  uint8_t i;
  uint8_t ep = emberAfFindClusterServerEndpointIndex(srcEndpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return;
  }

  if (index >= EMBER_AF_PLUGIN_PRICE_SERVER_TIER_LABELS_TABLE_SIZE){
    emberAfPriceClusterPrintln("Error: Index out of bound!");
    return;
  }
  if( info.tierLabelTable.entry[ep][index].numberOfTiers >= EMBER_AF_PLUGIN_PRICE_SERVER_MAX_TIERS_PER_TARIFF ){
    emberAfPriceClusterPrintln("Error: Invalid number of labels!");
    return;
  }

  emberAfFillExternalBuffer((ZCL_CLUSTER_SPECIFIC_COMMAND \
                          | ZCL_FRAME_CONTROL_SERVER_TO_CLIENT), \
                            ZCL_PRICE_CLUSTER_ID, \
                            ZCL_PUBLISH_TIER_LABELS_COMMAND_ID, \
                            "wwwuuu", \
                            info.tierLabelTable.entry[ep][index].providerId, \
                            info.tierLabelTable.entry[ep][index].issuerEventId, \
                            info.tierLabelTable.entry[ep][index].issuerTariffId, \
                            0, \
                            0, \
                            info.tierLabelTable.entry[ep][index].numberOfTiers );
  for( i=0; i<info.tierLabelTable.entry[ep][index].numberOfTiers; i++ ){
    emberAfPutInt8uInResp( info.tierLabelTable.entry[ep][index].tierIds[i] );
    emberAfPutStringInResp( info.tierLabelTable.entry[ep][index].tierLabels[i] );
  }

  emberAfSetCommandEndpoints(srcEndpoint, dstEndpoint);
  emberAfGetCommandApsFrame()->options |= EMBER_APS_OPTION_SOURCE_EUI64;
  status = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, nodeId);
  if(status != EMBER_SUCCESS) {
    emberAfPriceClusterPrintln("Error: PublishTierLabels failed: %X", status);
  }
}


///  CO2 VALUE
static void emberAfPluginPriceServerClearCo2Value( uint8_t endpoint ){
  uint8_t i;
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return;
  }
  for( i=0; i<EMBER_AF_PLUGIN_PRICE_SERVER_CO2_VALUE_TABLE_SIZE; i++ ){
    info.co2ValueTable.commonInfos[ep][i].valid = false;
  }
}


bool emberAfPriceClusterGetCO2ValueCallback(uint32_t earliestStartTime,
                                               uint32_t minIssuerEventId,
                                               uint8_t numberOfRequestedCommands,
                                               uint8_t tariffType)
{
  uint8_t cmdCount;
  uint8_t validCmds[EMBER_AF_PLUGIN_PRICE_SERVER_CO2_VALUE_TABLE_SIZE];
  uint8_t endpoint = emberAfCurrentEndpoint();
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return false;
  }
  cmdCount = emberAfPluginPriceCommonFindValidEntries(validCmds,
                                       EMBER_AF_PLUGIN_PRICE_SERVER_CO2_VALUE_TABLE_SIZE,
                                       info.co2ValueTable.commonInfos[ep],
                                       earliestStartTime,
                                       minIssuerEventId,
                                       numberOfRequestedCommands);

  // eliminate commands with mismatching tariffType
  // upper nibble is reserved. we'll ignore them.
  if (tariffType != TARIFF_TYPE_UNSPECIFIED){
    uint8_t i;
    for (i=0; i<cmdCount; i++){
      uint8_t index = validCmds[i];
      if ((info.co2ValueTable.co2Values[ep][index].tariffType & TARIFF_TYPE_MASK) != (tariffType & TARIFF_TYPE_MASK)){
        validCmds[i] = ZCL_PRICE_INVALID_INDEX;
      }
    }
  }
  sendValidCmdEntries( ZCL_PUBLISH_C_O2_VALUE_COMMAND_ID, ep, validCmds, cmdCount );
  return true;
}

uint32_t emberAfPriceServerSecondsUntilCO2ValueEvent( uint8_t endpoint ){
  uint32_t secondsTillNext;
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return 0xFFFFFFFF;
  }

  secondsTillNext = emberAfPluginPriceCommonSecondsUntilSecondIndexActive( info.co2ValueTable.commonInfos[ep],
                                                               EMBER_AF_PLUGIN_PRICE_SERVER_CO2_VALUE_TABLE_SIZE );
  return secondsTillNext;

}

void emberAfPriceServerRefreshCO2Value( uint8_t endpoint ){
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return;
  }

  if( 0 == emberAfPluginPriceCommonSecondsUntilSecondIndexActive( info.co2ValueTable.commonInfos[ep],
                                                      EMBER_AF_PLUGIN_PRICE_SERVER_CO2_VALUE_TABLE_SIZE ) ){
    // Replace current with next.
    info.co2ValueTable.commonInfos[ep][0].valid = 0;
    emberAfPluginPriceCommonSort( info.co2ValueTable.commonInfos[ep],
                      (uint8_t *)info.co2ValueTable.co2Values[ep],
                      sizeof(EmberAfPriceCo2Value),
                      EMBER_AF_PLUGIN_PRICE_SERVER_CO2_VALUE_TABLE_SIZE );
    emberAfPluginPriceCommonUpdateDurationForOverlappingEvents( info.co2ValueTable.commonInfos[ep],
                                                    EMBER_AF_PLUGIN_PRICE_SERVER_CO2_VALUE_TABLE_SIZE );
  }
}

void emberAfPluginPriceServerCo2ValueAdd(uint8_t endpoint,
                                         uint32_t issuerEventId,
                                         uint32_t startTime,
                                         uint32_t providerId,
                                         uint8_t tariffType,
                                         uint32_t co2Value,
                                         uint8_t co2ValueUnit,
                                         uint8_t co2ValueTrailingDigit)
{
  uint8_t index;
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return;
  }
  index = emberAfPluginPriceCommonGetCommonMatchingOrUnusedIndex( info.co2ValueTable.commonInfos[ep],
                                                      EMBER_AF_PLUGIN_PRICE_SERVER_CO2_VALUE_TABLE_SIZE,
                                                      issuerEventId, startTime, true );
  if (index >= EMBER_AF_PLUGIN_PRICE_SERVER_CO2_VALUE_TABLE_SIZE){
    emberAfPriceClusterPrintln("ERR: Unable to apply new CO2 Value!");
    return;
  }

  info.co2ValueTable.commonInfos[ep][index].startTime = startTime;
  info.co2ValueTable.commonInfos[ep][index].valid = true;
  info.co2ValueTable.commonInfos[ep][index].issuerEventId = issuerEventId;
  info.co2ValueTable.commonInfos[ep][index].durationSec = ZCL_PRICE_CLUSTER_DURATION_SEC_UNTIL_CHANGED;
  info.co2ValueTable.co2Values[ep][index].providerId = providerId;
  info.co2ValueTable.co2Values[ep][index].tariffType = tariffType;
  info.co2ValueTable.co2Values[ep][index].co2Value = co2Value;
  info.co2ValueTable.co2Values[ep][index].co2ValueUnit = co2ValueUnit;
  info.co2ValueTable.co2Values[ep][index].co2ValueTrailingDigit = co2ValueTrailingDigit;
  emberAfPluginPriceCommonSort( info.co2ValueTable.commonInfos[ep],
                    (uint8_t *)info.co2ValueTable.co2Values[ep],
                    sizeof(EmberAfPriceCo2Value),
                    EMBER_AF_PLUGIN_PRICE_SERVER_CO2_VALUE_TABLE_SIZE );
  emberAfPluginPriceCommonUpdateDurationForOverlappingEvents( info.co2ValueTable.commonInfos[ep],
                                                  EMBER_AF_PLUGIN_PRICE_SERVER_CO2_VALUE_TABLE_SIZE );
  emberAfPriceClusterScheduleTickCallback( endpoint, EMBER_AF_PRICE_SERVER_CHANGE_CO2_VALUE_EVENT_MASK );
}

void emberAfPluginPriceServerCo2LabelPub(uint16_t nodeId, uint8_t srcEndpoint, uint8_t dstEndpoint, uint8_t index)
{
  EmberStatus status;
  uint8_t ep = emberAfFindClusterServerEndpointIndex(srcEndpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return;
  }
  if (index >= EMBER_AF_PLUGIN_PRICE_SERVER_CO2_VALUE_TABLE_SIZE){
    emberAfPriceClusterPrintln("Error: Index out of bound!");
    return;
  }
  emberAfFillCommandPriceClusterPublishCO2Value(info.co2ValueTable.co2Values[ep][index].providerId,
                                                info.co2ValueTable.commonInfos[ep][index].issuerEventId,
                                                info.co2ValueTable.commonInfos[ep][index].startTime,
                                                info.co2ValueTable.co2Values[ep][index].tariffType,
                                                info.co2ValueTable.co2Values[ep][index].co2Value,
                                                info.co2ValueTable.co2Values[ep][index].co2ValueUnit,
                                                info.co2ValueTable.co2Values[ep][index].co2ValueTrailingDigit);
  emberAfSetCommandEndpoints(srcEndpoint, dstEndpoint);
  emberAfGetCommandApsFrame()->options |= EMBER_APS_OPTION_SOURCE_EUI64;
  status = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, nodeId);
  if(status != EMBER_SUCCESS) {
    emberAfPriceClusterPrintln("Error: PublishCo2Label failed: %X", status);
  }
}



/// BILLING PERIOD
static void emberAfPriceClearBillingPeriodTable( uint8_t endpoint ){
  uint8_t i;
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return;
  }
  for( i=0; i<EMBER_AF_PLUGIN_PRICE_SERVER_BILLING_PERIOD_TABLE_SIZE; i++ ){
    info.billingPeriodTable.commonInfos[ep][i].valid = 0;
  }
}


bool emberAfPriceClusterGetBillingPeriodCallback(uint32_t earliestStartTime,
                                                    uint32_t minIssuerEventId,
                                                    uint8_t numberOfRequestedEntries,
                                                    uint8_t tariffType) {
  uint32_t issuerEventId;
  uint32_t startTime;
  uint32_t durationSec;
  uint8_t  curTariffType;
  uint8_t  validEntry;

  uint8_t validEntries[EMBER_AF_PLUGIN_PRICE_SERVER_BILLING_PERIOD_TABLE_SIZE];
  uint8_t validEntryIndex = 0;
  uint8_t requestEntryCount = EMBER_AF_PLUGIN_PRICE_SERVER_BILLING_PERIOD_TABLE_SIZE;
  uint8_t i;

  uint8_t endpoint = emberAfCurrentEndpoint();
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return false;
  }
  emberAfPriceClusterPrintln("RX: GetBillingPeriod, 0x%4X, 0x%4X, 0x%X, 0x%X",
                             earliestStartTime,
                             minIssuerEventId, 
                             numberOfRequestedEntries,
                             tariffType);
 
  if ((numberOfRequestedEntries != 0x00) && (requestEntryCount > numberOfRequestedEntries)){
      requestEntryCount = numberOfRequestedEntries;
  }
  if( earliestStartTime == 0 ){
    earliestStartTime = emberAfGetCurrentTime();
  }

  MEMSET(validEntries, 0xFF, EMBER_AF_PLUGIN_PRICE_SERVER_BILLING_PERIOD_TABLE_SIZE);

  for (i=0; i<EMBER_AF_PLUGIN_PRICE_SERVER_BILLING_PERIOD_TABLE_SIZE; i++){
    issuerEventId = info.billingPeriodTable.commonInfos[ep][i].issuerEventId;
    startTime = info.billingPeriodTable.commonInfos[ep][i].startTime;
    durationSec = info.billingPeriodTable.commonInfos[ep][i].durationSec;
    curTariffType = info.billingPeriodTable.billingPeriods[ep][i].tariffType;
    validEntry = info.billingPeriodTable.commonInfos[ep][i].valid;

    if (!validEntry){ continue; }
    emberAfPriceClusterPrintln("info for entry index: %d", i);
    emberAfPriceClusterPrintln("start time: 0x%4x", startTime);
    emberAfPriceClusterPrintln("end time: 0x%4x", startTime + durationSec);
    emberAfPriceClusterPrintln("duration: 0x%4x", durationSec);
  
    // invalid entry tariffType
    if (tariffType != TARIFF_TYPE_DONT_CARE){
      if ((tariffType & TARIFF_TYPE_MASK) !=  (curTariffType & TARIFF_TYPE_MASK)){
        continue;
      }
    }

    if (earliestStartTime > (startTime + durationSec)){
      continue;
    }
    
    if ((minIssuerEventId != 0xFFFFFFFF)
        && (minIssuerEventId > issuerEventId )){
      continue;
    }

    // valid entry
    validEntries[validEntryIndex] = i;
    validEntryIndex++;
    if (requestEntryCount == validEntryIndex){
      break;
    }
  }

  sendValidCmdEntries(ZCL_PUBLISH_BILLING_PERIOD_COMMAND_ID, ep, validEntries, validEntryIndex );
  return true;
}

uint32_t emberAfPriceServerSecondsUntilBillingPeriodEvent( uint8_t endpoint ){
  uint32_t secondsTillNext;
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return 0xFFFFFFFF;
  }

  secondsTillNext = emberAfPluginPriceCommonSecondsUntilSecondIndexActive( info.billingPeriodTable.commonInfos[ep],
                                                               EMBER_AF_PLUGIN_PRICE_SERVER_BILLING_PERIOD_TABLE_SIZE );
  return secondsTillNext;
}

void emberAfPriceServerRefreshBillingPeriod( uint8_t endpoint, bool force ){
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  uint32_t secondsUntilSecondIndexActive;
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return;
  }

  secondsUntilSecondIndexActive
    = emberAfPluginPriceCommonSecondsUntilSecondIndexActive(info.billingPeriodTable.commonInfos[ep],
                                                            EMBER_AF_PLUGIN_PRICE_SERVER_BILLING_PERIOD_TABLE_SIZE);

  if (secondsUntilSecondIndexActive == 0) {
    // Replace current with next.
    info.billingPeriodTable.commonInfos[ep][0].valid = 0;
    emberAfPluginPriceCommonSort( info.billingPeriodTable.commonInfos[ep],
                      (uint8_t *)info.billingPeriodTable.billingPeriods[ep],
                      sizeof(EmberAfPriceBillingPeriod),
                      EMBER_AF_PLUGIN_PRICE_SERVER_BILLING_PERIOD_TABLE_SIZE );
    emberAfPluginPriceCommonUpdateDurationForOverlappingEvents( info.billingPeriodTable.commonInfos[ep],
                                                    EMBER_AF_PLUGIN_PRICE_SERVER_BILLING_PERIOD_TABLE_SIZE );
  } else if (secondsUntilSecondIndexActive == 0xFFFFFFFF
             && force
             && info.billingPeriodTable.commonInfos[ep][0].valid) {
    // The .startTime and .durationSec members are both UTC values measured
    // in seconds. The nextStartTime is therefore a UTC value. Set the start
    // time of the next period to the end time of the current period.
    uint32_t nextStartTime
      = (info.billingPeriodTable.commonInfos[ep][0].startTime
         + info.billingPeriodTable.commonInfos[ep][0].durationSec);
    // The event ID is supposed to be a unique value for each different piece
    // of information from the commodity provider. A higher event ID is supposed
    // to mean that the information is newer. Therefore, let's make sure this
    // next billing period has a higher event ID than the current period.
    uint32_t nextIssuerEventId
      = (info.billingPeriodTable.commonInfos[ep][0].issuerEventId + 1);
    emberAfPluginPriceServerBillingPeriodAdd(endpoint,
                                             nextStartTime,
                                             nextIssuerEventId,
                                             info.billingPeriodTable.billingPeriods[ep][0].providerId,
                                             info.billingPeriodTable.billingPeriods[ep][0].billingPeriodDuration,
                                             info.billingPeriodTable.billingPeriods[ep][0].billingPeriodDurationType,
                                             info.billingPeriodTable.billingPeriods[ep][0].tariffType);
  }
}

EmberStatus emberAfPluginPriceServerBillingPeriodAdd(uint8_t endpoint,
                                                     uint32_t startTime,
                                                     uint32_t issuerEventId,
                                                     uint32_t providerId,
                                                     uint32_t billingPeriodDuration,
                                                     uint8_t billingPeriodDurationType,
                                                     uint8_t tariffType)
{
  uint8_t index;
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return EMBER_BAD_ARGUMENT;
  }
  index = emberAfPluginPriceCommonGetCommonMatchingOrUnusedIndex(info.billingPeriodTable.commonInfos[ep],
                                                                 EMBER_AF_PLUGIN_PRICE_SERVER_BILLING_PERIOD_TABLE_SIZE,
                                                                 issuerEventId,
                                                                 startTime,
                                                                 false );
  if (index >= EMBER_AF_PLUGIN_PRICE_SERVER_BILLING_PERIOD_TABLE_SIZE){
    emberAfPriceClusterPrintln("Error: Index out of bounds!");
    return EMBER_BAD_ARGUMENT;
  }

  // checking for valid tariffType.
  if (tariffType > EMBER_ZCL_TARIFF_TYPE_DELIVERED_AND_RECEIVED_TARIFF){
    emberAfPriceClusterPrintln("Error: Invalid tariff type!");
    return EMBER_BAD_ARGUMENT;
  }

  info.billingPeriodTable.commonInfos[ep][index].valid = true;
  info.billingPeriodTable.commonInfos[ep][index].startTime = emberAfPluginPriceCommonClusterGetAdjustedStartTime( startTime, billingPeriodDurationType );
  info.billingPeriodTable.commonInfos[ep][index].issuerEventId = issuerEventId;
  info.billingPeriodTable.billingPeriods[ep][index].providerId = providerId;
  info.billingPeriodTable.billingPeriods[ep][index].rawBillingPeriodStartTime = startTime;
  info.billingPeriodTable.commonInfos[ep][index].durationSec = 
    emberAfPluginPriceCommonClusterConvertDurationToSeconds( startTime, billingPeriodDuration, billingPeriodDurationType );

  info.billingPeriodTable.billingPeriods[ep][index].billingPeriodDuration = billingPeriodDuration;
  info.billingPeriodTable.billingPeriods[ep][index].billingPeriodDurationType = billingPeriodDurationType;
  info.billingPeriodTable.billingPeriods[ep][index].tariffType = tariffType;

  emberAfPluginPriceCommonSort( info.billingPeriodTable.commonInfos[ep],
                    (uint8_t *)info.billingPeriodTable.billingPeriods[ep],
                    sizeof(EmberAfPriceBillingPeriod),
                    EMBER_AF_PLUGIN_PRICE_SERVER_BILLING_PERIOD_TABLE_SIZE );
  emberAfPluginPriceCommonUpdateDurationForOverlappingEvents( info.billingPeriodTable.commonInfos[ep],
                                                  EMBER_AF_PLUGIN_PRICE_SERVER_BILLING_PERIOD_TABLE_SIZE );

  emberAfPriceClusterScheduleTickCallback( endpoint, EMBER_AF_PRICE_SERVER_CHANGE_BILLING_PERIOD_EVENT_MASK );
  return EMBER_SUCCESS;
}

void emberAfPluginPriceServerBillingPeriodPub(uint16_t nodeId, uint8_t srcEndpoint, uint8_t dstEndpoint, uint8_t index)
{
  EmberStatus status;
  uint8_t ep = emberAfFindClusterServerEndpointIndex(srcEndpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return;
  }
  if (index >= EMBER_AF_PLUGIN_PRICE_SERVER_BILLING_PERIOD_TABLE_SIZE){
    emberAfPriceClusterPrintln("Error: Index out of bounds!");
    return;
  }
  emberAfFillCommandPriceClusterPublishBillingPeriod(info.billingPeriodTable.billingPeriods[ep][index].providerId,
                                                     info.billingPeriodTable.commonInfos[ep][index].issuerEventId,
                                                     info.billingPeriodTable.billingPeriods[ep][index].rawBillingPeriodStartTime,
                                                     info.billingPeriodTable.billingPeriods[ep][index].billingPeriodDuration,
                                                     info.billingPeriodTable.billingPeriods[ep][index].billingPeriodDurationType,
                                                     info.billingPeriodTable.billingPeriods[ep][index].tariffType);

  emberAfSetCommandEndpoints(srcEndpoint, dstEndpoint);
  emberAfGetCommandApsFrame()->options |= EMBER_APS_OPTION_SOURCE_EUI64;
  status = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, nodeId);
  if(status != EMBER_SUCCESS) {
    emberAfPriceClusterPrintln("Error: PublishBillingPeriod failed: 0x%X", status);
  }
}


//   CONSOLIDATED BILL
bool emberAfPriceClusterGetConsolidatedBillCallback( uint32_t earliestStartTime,
                                                        uint32_t minIssuerEventId,
                                                        uint8_t numberOfRequestedCommands,
                                                        uint8_t tariffType ){

  uint8_t  validEntries[EMBER_AF_PLUGIN_PRICE_SERVER_CONSOLIDATED_BILL_TABLE_SIZE];
  uint8_t  validEntriesIndex = 0;
  uint32_t endTimeUtc;
  uint8_t  i;

  uint8_t endpoint = emberAfCurrentEndpoint();
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return false;
  }

  emberAfPriceClusterPrintln("RX: GetConsolidatedBill, 0x%4X, 0x%4X, 0x%X, 0x%X",
                             earliestStartTime,
                             minIssuerEventId, 
                             numberOfRequestedCommands,
                             tariffType);

  if( minIssuerEventId == EVENT_ID_UNSPECIFIED ){
    minIssuerEventId = 0;   // Allow all event IDs
  }
  if( earliestStartTime == 0 ){
    earliestStartTime = emberAfGetCurrentTime();
  }

  for( i=0; i<EMBER_AF_PLUGIN_PRICE_SERVER_CONSOLIDATED_BILL_TABLE_SIZE; i++ ){
    if( info.consolidatedBillsTable.commonInfos[ep][i].valid ){
      emberAfPriceClusterPrintln("===  i=%d, st=0x%4x, ev=%d, tar=%d,   st[]=0x%4x, ev[]=%d, tar[]=%d", 
                                 i, earliestStartTime, minIssuerEventId, tariffType,
                                 info.consolidatedBillsTable.commonInfos[ep][i].startTime,
                                 info.consolidatedBillsTable.commonInfos[ep][i].issuerEventId,
                                 info.consolidatedBillsTable.consolidatedBills[ep][i].tariffType );

      if((tariffType == TARIFF_TYPE_UNSPECIFIED)
          || ((info.consolidatedBillsTable.consolidatedBills[ep][i].tariffType & TARIFF_TYPE_MASK)
              == (tariffType & TARIFF_TYPE_MASK))){
        // According to "Get Consolidated Bill" command documentation, this response should 
        // include consolidated bills that are already active.  So, send back any with
        // end time that is > earliestStartTime.
        endTimeUtc = info.consolidatedBillsTable.commonInfos[ep][i].startTime + info.consolidatedBillsTable.commonInfos[ep][i].durationSec;
        if( (earliestStartTime < endTimeUtc) && (minIssuerEventId <= info.consolidatedBillsTable.commonInfos[ep][i].issuerEventId) ){
          validEntries[validEntriesIndex] = i;
          validEntriesIndex++;
          // NOTE:  Incrementing validEntriesIndex first ensures that all entries are sent if numberOfRequestedCommands == 0.
          if( validEntriesIndex == numberOfRequestedCommands ){
            break;
          }
        }
      }
    }
  }
  // Have a set of valid consolidated bills.  Total number == validEntriesIndex.
  sendValidCmdEntries( ZCL_PUBLISH_CONSOLIDATED_BILL_COMMAND_ID, ep, validEntries, validEntriesIndex );

  return true;
}

static void emberAfPriceClearConsolidatedBillsTable( uint8_t endpoint ){
  uint8_t i;
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return;
  }
  for( i=0; i<EMBER_AF_PLUGIN_PRICE_SERVER_CONSOLIDATED_BILL_TABLE_SIZE; i++ ){
    info.consolidatedBillsTable.commonInfos[ep][i].valid = 0;
  }
}


void emberAfPluginPriceServerConsolidatedBillAdd( uint8_t endpoint,
                                                  uint32_t startTime,
                                                  uint32_t issuerEventId,
                                                  uint32_t providerId,
                                                  uint32_t billingPeriodDuration,
                                                  uint8_t billingPeriodDurationType,
                                                  uint8_t tariffType,
                                                  uint32_t consolidatedBill,
                                                  uint16_t currency,
                                                  uint8_t billTrailingDigit )
{
  uint32_t adjStartTime;
  uint8_t index;
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return;
  }

  adjStartTime = emberAfPluginPriceCommonClusterGetAdjustedStartTime( startTime, billingPeriodDurationType );
  index = emberAfPluginPriceCommonGetCommonMatchingOrUnusedIndex( info.consolidatedBillsTable.commonInfos[ep],
                                                      EMBER_AF_PLUGIN_PRICE_SERVER_CONSOLIDATED_BILL_TABLE_SIZE,
                                                      issuerEventId, adjStartTime, false );
  if (index >= EMBER_AF_PLUGIN_PRICE_SERVER_CONSOLIDATED_BILL_TABLE_SIZE){
    emberAfPriceClusterPrintln("Error: Index out of bound!");
    return;
  }

  info.consolidatedBillsTable.commonInfos[ep][index].valid = true;
  info.consolidatedBillsTable.commonInfos[ep][index].startTime = adjStartTime;
  info.consolidatedBillsTable.commonInfos[ep][index].durationSec = 
                      emberAfPluginPriceCommonClusterConvertDurationToSeconds( startTime, billingPeriodDuration, billingPeriodDurationType );


  info.consolidatedBillsTable.commonInfos[ep][index].issuerEventId = issuerEventId;

  info.consolidatedBillsTable.consolidatedBills[ep][index].providerId = providerId;
  info.consolidatedBillsTable.consolidatedBills[ep][index].rawStartTimeUtc = startTime;
  info.consolidatedBillsTable.consolidatedBills[ep][index].billingPeriodDuration = billingPeriodDuration;
  info.consolidatedBillsTable.consolidatedBills[ep][index].billingPeriodDurationType = billingPeriodDurationType;
  info.consolidatedBillsTable.consolidatedBills[ep][index].tariffType = tariffType;
  info.consolidatedBillsTable.consolidatedBills[ep][index].consolidatedBill = consolidatedBill;
  info.consolidatedBillsTable.consolidatedBills[ep][index].currency = currency;
  info.consolidatedBillsTable.consolidatedBills[ep][index].billTrailingDigit = billTrailingDigit;

  emberAfPluginPriceCommonSort( info.consolidatedBillsTable.commonInfos[ep],
                    (uint8_t *)info.consolidatedBillsTable.consolidatedBills[ep],
                    sizeof(EmberAfPriceConsolidatedBills),
                    EMBER_AF_PLUGIN_PRICE_SERVER_CONSOLIDATED_BILL_TABLE_SIZE );
  emberAfPluginPriceCommonUpdateDurationForOverlappingEvents( info.consolidatedBillsTable.commonInfos[ep],
                                                  EMBER_AF_PLUGIN_PRICE_SERVER_CONSOLIDATED_BILL_TABLE_SIZE );

}

void emberAfPluginPriceServerConsolidatedBillPub(uint16_t nodeId,  uint8_t srcEndpoint, uint8_t dstEndpoint, uint8_t index )
{
  EmberStatus status;
  EmberAfPriceConsolidatedBills *pbill;

  uint8_t ep = emberAfFindClusterServerEndpointIndex(srcEndpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return;
  }
  if (index >= EMBER_AF_PLUGIN_PRICE_SERVER_CONSOLIDATED_BILL_TABLE_SIZE){
    emberAfPriceClusterPrintln("Error: Index out of bounds!");
    return;
  }
  pbill = &info.consolidatedBillsTable.consolidatedBills[ep][index];

  emberAfFillCommandPriceClusterPublishConsolidatedBill( pbill->providerId,
                                                         info.consolidatedBillsTable.commonInfos[ep][index].issuerEventId,
                                                         pbill->rawStartTimeUtc,
                                                         pbill->billingPeriodDuration,
                                                         pbill->billingPeriodDurationType,
                                                         pbill->tariffType,
                                                         pbill->consolidatedBill,
                                                         pbill->currency,
                                                         pbill->billTrailingDigit );

  emberAfSetCommandEndpoints(srcEndpoint, dstEndpoint);
  emberAfGetCommandApsFrame()->options |= EMBER_APS_OPTION_SOURCE_EUI64;
  status = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, nodeId);
  if(status != EMBER_SUCCESS) {
    emberAfPriceClusterPrintln("Error: PublishConsolidatedBill failed: %x", status);
  }
}

// CPP EVENT

void emberAfPluginPriceServerCppEventSet( uint8_t endpoint, uint8_t valid, uint32_t providerId, uint32_t issuerEventId, uint32_t startTime,
                                          uint16_t durationInMinutes, uint8_t tariffType, uint8_t cppPriceTier, uint8_t cppAuth ){

// For now, just a single CPP event.  Support multiple??
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return;
  }
  info.cppTable.commonInfos[ep].valid = valid;
  info.cppTable.commonInfos[ep].issuerEventId = issuerEventId;
  info.cppTable.commonInfos[ep].startTime = startTime;
  info.cppTable.commonInfos[ep].durationSec = (durationInMinutes * 60);
  info.cppTable.cppEvent[ep].durationInMinutes = durationInMinutes;
  info.cppTable.cppEvent[ep].providerId = providerId;
  info.cppTable.cppEvent[ep].tariffType = tariffType;
  info.cppTable.cppEvent[ep].cppPriceTier = cppPriceTier;
  info.cppTable.cppEvent[ep].cppAuth = cppAuth;
}

void emberAfPluginPriceServerCppEventPrint( uint8_t endpoint ){
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return;
  }
  emberAfPriceClusterPrintln("Cpp Event:");
  emberAfPriceClusterPrintln("  providerId=%d", info.cppTable.cppEvent[ep].providerId);
  emberAfPriceClusterPrintln("  issuerEventId=%d", info.cppTable.commonInfos[ep].issuerEventId);
  emberAfPriceClusterPrintln("  startTime=0x%4x", info.cppTable.commonInfos[ep].startTime);
  emberAfPriceClusterPrintln("  durationInMins=%d", info.cppTable.cppEvent[ep].durationInMinutes);
  emberAfPriceClusterPrintln("  tariffType=%d", info.cppTable.cppEvent[ep].tariffType);
  emberAfPriceClusterPrintln("  cppPriceTier=%d", info.cppTable.cppEvent[ep].cppPriceTier);
  emberAfPriceClusterPrintln("  cppAuth=%d", info.cppTable.cppEvent[ep].cppAuth);
}


void emberAfPluginPriceServerCppEventPub( uint16_t nodeId, uint8_t srcEndpoint, uint8_t dstEndpoint ){

  EmberStatus status;
  EmberAfPriceCppEvent *cppEvent;
  uint8_t ep = emberAfFindClusterServerEndpointIndex(srcEndpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return;
  }

  cppEvent = &info.cppTable.cppEvent[ep];
  emberAfFillCommandPriceClusterPublishCppEvent( cppEvent->providerId, info.cppTable.commonInfos[ep].issuerEventId, 
                                                 info.cppTable.commonInfos[ep].startTime,
                                                 cppEvent->durationInMinutes, cppEvent->tariffType,
                                                 cppEvent->cppPriceTier, cppEvent->cppAuth );
  emberAfSetCommandEndpoints(srcEndpoint, dstEndpoint);
  emberAfGetCommandApsFrame()->options |= EMBER_APS_OPTION_SOURCE_EUI64;
  status = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, nodeId);
  if(status != EMBER_SUCCESS) {
    emberAfPriceClusterPrintln("Error: PublishCppEvent failed: %x", status);
  }
}

bool emberAfPriceClusterCppEventResponseCallback(uint32_t issuerEventId, uint8_t cppAuth){

  uint8_t endpoint = emberAfCurrentEndpoint();
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return false;
  }
  emberAfPriceClusterPrintln("Rx: Cpp Event Response, issuerEventId=0x%4x, auth=%d,   storedCppAuth=%d", issuerEventId, cppAuth,
      info.cppTable.cppEvent[ep].cppAuth );
  if( info.cppTable.cppEvent[ep].cppAuth == EMBER_AF_PLUGIN_PRICE_CPP_AUTH_PENDING ){
    // Update the CPP auth status based on the Cpp Event Response we received from the client.
    // Send another CPP Event with the updated status to confirm.  See D.4.2.4.12.4,  Fig D-88.
//    info.cppEvent.cppAuth = cppAuth;
//    info.cppEvent.commonInfos.issuerEventId++;    // Test 12.68 Item 5 requires this message have a different event ID than 
                                                  // the event ID used in the Pending status message.  IS THIS RIGHT?
    emberAfPriceClusterPrintln("Send CPP Event, stat=%d", cppAuth );
    emberAfFillCommandPriceClusterPublishCppEvent( info.cppTable.cppEvent[ep].providerId,
                                                   info.cppTable.commonInfos[ep].issuerEventId,
                                                   info.cppTable.commonInfos[ep].startTime,
                                                   info.cppTable.cppEvent[ep].durationInMinutes,
                                                   info.cppTable.cppEvent[ep].tariffType,
                                                   info.cppTable.cppEvent[ep].cppPriceTier,
                                                   cppAuth );



    emberAfSendResponse();
  }
  //else{
    // PER SE TEST 12.70, item 3-5, server should not send a message if an asynch CPP Event Response is received.
    //emberAfSendImmediateDefaultResponse( EMBER_ZCL_STATUS_SUCCESS );
  //}
  return true;
}


// CREDIT PAYMENT
bool emberAfPriceClusterGetCreditPaymentCallback(uint32_t latestEndTime, uint8_t numberOfRecords){
  uint8_t  validEntries[EMBER_AF_PLUGIN_PRICE_SERVER_CREDIT_PAYMENT_TABLE_SIZE];
  uint8_t  validEntriesIndex = 0;
  uint8_t  i;
  uint8_t endpoint = emberAfCurrentEndpoint();
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return false;
  }

  emberAfPriceClusterPrintln("Rx: GetCreditPayment, endTime=%d, nr=%d", latestEndTime, numberOfRecords);
  if( numberOfRecords == 0 ){
    numberOfRecords = EMBER_AF_PLUGIN_PRICE_SERVER_CREDIT_PAYMENT_TABLE_SIZE;
  }
  if( latestEndTime == 0 ){
    latestEndTime = emberAfGetCurrentTime();
  }

  for( i=0; i<EMBER_AF_PLUGIN_PRICE_SERVER_CREDIT_PAYMENT_TABLE_SIZE; i++ ){
    if( (info.creditPaymentTable.commonInfos[ep][i].valid) && (info.creditPaymentTable.creditPayment[ep][i].creditPaymentDate <= latestEndTime) ){
      validEntries[validEntriesIndex] = i;
      validEntriesIndex++;
      // NOTE:  Incrementing validEntriesIndex first ensures that all entries are sent if numberOfEvents == 0.
      if( validEntriesIndex == numberOfRecords ){
        break;
      }
    }
  }
  if( validEntriesIndex == 0 ){
    emberAfPriceClusterPrintln("No matching credit payments");
  }
  sortCreditPaymentEntries( validEntries, validEntriesIndex, info.creditPaymentTable.creditPayment[ep] );

  sendValidCmdEntries( ZCL_PUBLISH_CREDIT_PAYMENT_COMMAND_ID, ep, validEntries, validEntriesIndex );

  return true;
}

void sortCreditPaymentEntries( uint8_t *entries, uint8_t numValidEntries, EmberAfPriceCreditPayment *table ){
  // The valid entries should be sorted from latest "credit payment date" to earliest "credit payment date".
  uint32_t latestPaymentDate = 0xFFFFFFFF;
  uint32_t latestRemainingDate;
  uint32_t latestRemainingIndex;
  uint32_t paymentDate;
  uint8_t i,j;
  uint8_t sortedEntries[255];

  for( i=0; i<numValidEntries; i++ ){
    latestRemainingDate = 0;  // Reset this, then go through all entries to find the latest.
    latestRemainingIndex = 0;
    for( j=0; j<numValidEntries; j++ ){
      // Find OLDEST from remaining entries.
      paymentDate = table[entries[j]].creditPaymentDate;
      if( (paymentDate < latestPaymentDate)  && (paymentDate > latestRemainingDate) ){
        latestRemainingDate = paymentDate;
        latestRemainingIndex = j;
      }
    }
    // Store the oldest found
    latestPaymentDate = table[entries[latestRemainingIndex]].creditPaymentDate;
    sortedEntries[i] = entries[latestRemainingIndex];
  }
  // Now copy the sorted entries back into the "entries" table.
  MEMCOPY( entries, sortedEntries, numValidEntries );
}

void emberAfPluginPriceServerCreditPaymentPub(uint16_t nodeId, uint8_t srcEndpoint, uint8_t dstEndpoint, uint8_t index)
{
  EmberStatus status;
  EmberAfPriceCreditPayment *pcredit;
  uint8_t ep = emberAfFindClusterServerEndpointIndex(srcEndpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return;
  }

  if (index >= EMBER_AF_PLUGIN_PRICE_SERVER_CREDIT_PAYMENT_TABLE_SIZE){
    emberAfPriceClusterPrintln("Error: Index out of bound!");
    return;
  }
  else if( !info.creditPaymentTable.commonInfos[ep][index].valid ){
    emberAfPriceClusterPrintln("Error: Invalid index!");
    return;
  }
  pcredit = &info.creditPaymentTable.creditPayment[ep][index];

  emberAfFillCommandPriceClusterPublishCreditPayment( pcredit->providerId,
                                                      info.creditPaymentTable.commonInfos[ep][index].issuerEventId,
                                                      pcredit->creditPaymentDueDate,
                                                      pcredit->creditPaymentAmountOverdue,
                                                      pcredit->creditPaymentStatus,
                                                      pcredit->creditPayment,
                                                      pcredit->creditPaymentDate,
                                                      pcredit->creditPaymentRef );

  emberAfSetCommandEndpoints(srcEndpoint, dstEndpoint);
  emberAfGetCommandApsFrame()->options |= EMBER_APS_OPTION_SOURCE_EUI64;
  status = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, nodeId);
  if(status != EMBER_SUCCESS) {
    emberAfPriceClusterPrintln("Error: PublishCreditPayment failed: %x", status);
  }
}



void emberAfPluginPriceServerCreditPaymentSet( uint8_t endpoint, uint8_t index, uint8_t valid,
                                               uint32_t providerId, uint32_t issuerEventId,
                                               uint32_t creditPaymentDueDate, uint32_t creditPaymentOverdueAmount,
                                               uint8_t creditPaymentStatus, uint32_t creditPayment,
                                               uint32_t creditPaymentDate, uint8_t *creditPaymentRef ){
  uint8_t creditPaymentRefLength;
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return;
  }
  if( index >= EMBER_AF_PLUGIN_PRICE_SERVER_CREDIT_PAYMENT_TABLE_SIZE){
    emberAfPriceClusterPrintln("Error: Index out of bound!");
    return;
  }
  creditPaymentRefLength = creditPaymentRef[0];
  if( creditPaymentRefLength > CREDIT_PAYMENT_REF_STRING_LEN ){
    // Truncate reference string if necessary.
    creditPaymentRefLength = CREDIT_PAYMENT_REF_STRING_LEN;
    creditPaymentRef[0] = CREDIT_PAYMENT_REF_STRING_LEN;
  }

  info.creditPaymentTable.commonInfos[ep][index].valid = valid;
  info.creditPaymentTable.commonInfos[ep][index].issuerEventId = issuerEventId;
  info.creditPaymentTable.commonInfos[ep][index].durationSec = ZCL_PRICE_CLUSTER_DURATION_SEC_UNTIL_CHANGED;
  info.creditPaymentTable.creditPayment[ep][index].providerId = providerId;
  info.creditPaymentTable.creditPayment[ep][index].creditPaymentDueDate = creditPaymentDueDate;
  info.creditPaymentTable.creditPayment[ep][index].creditPaymentAmountOverdue = creditPaymentOverdueAmount;
  info.creditPaymentTable.creditPayment[ep][index].creditPaymentStatus = creditPaymentStatus;
  info.creditPaymentTable.creditPayment[ep][index].creditPayment = creditPayment;
  info.creditPaymentTable.creditPayment[ep][index].creditPaymentDate = creditPaymentDate;
  if( creditPaymentRefLength ){
    MEMCOPY( info.creditPaymentTable.creditPayment[ep][index].creditPaymentRef, creditPaymentRef, creditPaymentRefLength+1 );
  }
}

// CURRENCY CONVERSION
static void emberAfPriceClearCurrencyConversionTable( uint8_t endpoint ){
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep != ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    info.currencyConversionTable.commonInfos[ep].valid = false;
  }
}
  

bool emberAfPriceClusterGetCurrencyConversionCommandCallback(void){
  uint8_t endpoint = emberAfCurrentEndpoint();
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return false;
  }
  emberAfPriceClusterPrintln("Rx: GetCurrencyConversion");
  if( info.currencyConversionTable.commonInfos[ep].valid ){
    emberAfFillCommandPriceClusterPublishCurrencyConversion( info.currencyConversionTable.currencyConversion[ep].providerId,
                                                             info.currencyConversionTable.commonInfos[ep].issuerEventId,
                                                             info.currencyConversionTable.commonInfos[ep].startTime,
                                                             info.currencyConversionTable.currencyConversion[ep].oldCurrency,
                                                             info.currencyConversionTable.currencyConversion[ep].newCurrency,
                                                             info.currencyConversionTable.currencyConversion[ep].conversionFactor,
                                                             info.currencyConversionTable.currencyConversion[ep].conversionFactorTrailingDigit,
                                                             info.currencyConversionTable.currencyConversion[ep].currencyChangeControlFlags );
    emberAfSendResponse();
  }
  else{
    emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_NOT_FOUND);
  }
  return true;
}

void emberAfPluginPriceServerCurrencyConversionPub( uint16_t nodeId, uint8_t srcEndpoint, uint8_t dstEndpoint ){
  uint8_t status;
  uint8_t ep = emberAfFindClusterServerEndpointIndex(srcEndpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return;
  }
  if( info.currencyConversionTable.commonInfos[ep].valid ){
    emberAfFillCommandPriceClusterPublishCurrencyConversion( info.currencyConversionTable.currencyConversion[ep].providerId,
                                                             info.currencyConversionTable.commonInfos[ep].issuerEventId,
                                                             info.currencyConversionTable.commonInfos[ep].startTime,
                                                             info.currencyConversionTable.currencyConversion[ep].oldCurrency,
                                                             info.currencyConversionTable.currencyConversion[ep].newCurrency,
                                                             info.currencyConversionTable.currencyConversion[ep].conversionFactor,
                                                             info.currencyConversionTable.currencyConversion[ep].conversionFactorTrailingDigit,
                                                             info.currencyConversionTable.currencyConversion[ep].currencyChangeControlFlags );
    emberAfSetCommandEndpoints(srcEndpoint, dstEndpoint);
    emberAfGetCommandApsFrame()->options |= EMBER_APS_OPTION_SOURCE_EUI64;
    status = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, nodeId);
    if(status != EMBER_SUCCESS) {
      emberAfPriceClusterPrintln("Error: PublishCreditPayment failed: %x", status);
    }
  }
  else{
    emberAfPriceClusterPrintln("Error: Invalid Currency Conversion");
  }
}

void emberAfPluginPriceServerCurrencyConversionSet( uint8_t endpoint, uint8_t valid, uint32_t providerId, uint32_t issuerEventId,
                                                    uint32_t startTime, uint16_t oldCurrency, uint16_t newCurrency,
                                                    uint32_t conversionFactor, uint8_t conversionFactorTrailingDigit,
                                                    uint32_t currencyChangeControlFlags ){

  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return;
  }
  emberAfPriceClusterPrintln("Set Currency Conversion: val=%d, pid=%d, eid=%d, start=%d, flag=%d",
      valid, providerId, issuerEventId, startTime, currencyChangeControlFlags );
  info.currencyConversionTable.commonInfos[ep].valid = valid;
  info.currencyConversionTable.currencyConversion[ep].providerId = providerId;
  info.currencyConversionTable.commonInfos[ep].issuerEventId = issuerEventId;
  info.currencyConversionTable.commonInfos[ep].startTime = startTime;
  info.currencyConversionTable.currencyConversion[ep].oldCurrency = oldCurrency;
  info.currencyConversionTable.currencyConversion[ep].newCurrency = newCurrency;
  info.currencyConversionTable.currencyConversion[ep].conversionFactor = conversionFactor;
  info.currencyConversionTable.currencyConversion[ep].conversionFactorTrailingDigit = conversionFactorTrailingDigit;
  info.currencyConversionTable.currencyConversion[ep].currencyChangeControlFlags = currencyChangeControlFlags;
}


// TARIFF CANCELLATION
static void emberAfPriceClearCancelTariffTable( uint8_t endpoint ){
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep != ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    info.cancelTariffTable.cancelTariff[ep].valid = false;
  }
}

bool emberAfPriceClusterGetTariffCancellationCallback(void){

  uint8_t endpoint = emberAfCurrentEndpoint();
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return false;
  }
  emberAfPriceClusterPrintln("Rx: GetTariffCancellation");
  if( info.cancelTariffTable.cancelTariff[ep].valid ){
    emberAfFillCommandPriceClusterCancelTariff( info.cancelTariffTable.cancelTariff[ep].providerId,
                                                info.cancelTariffTable.cancelTariff[ep].issuerTariffId,
                                                info.cancelTariffTable.cancelTariff[ep].tariffType );
    emberAfSendResponse();
  }
  else{
    emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_NOT_FOUND);
  }
  return true;
}

void emberAfPluginPriceServerTariffCancellationSet( uint8_t endpoint, uint8_t valid, uint32_t providerId, uint32_t issuerTariffId, uint8_t tariffType ){
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return;
  }
  info.cancelTariffTable.cancelTariff[ep].valid = valid;
  info.cancelTariffTable.cancelTariff[ep].providerId = providerId;
  info.cancelTariffTable.cancelTariff[ep].issuerTariffId = issuerTariffId;
  info.cancelTariffTable.cancelTariff[ep].tariffType = tariffType;
}

void emberAfPluginPriceServerTariffCancellationPub( uint16_t nodeId, uint8_t srcEndpoint, uint8_t dstEndpoint ){
  uint8_t status;
  uint8_t ep = emberAfFindClusterServerEndpointIndex(srcEndpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return;
  }
  if( info.cancelTariffTable.cancelTariff[ep].valid ){
    emberAfFillCommandPriceClusterCancelTariff( info.cancelTariffTable.cancelTariff[ep].providerId,
                                                info.cancelTariffTable.cancelTariff[ep].issuerTariffId,
                                                info.cancelTariffTable.cancelTariff[ep].tariffType );
    emberAfSetCommandEndpoints( srcEndpoint, dstEndpoint );
    emberAfGetCommandApsFrame()->options |= EMBER_APS_OPTION_SOURCE_EUI64;
    status = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, nodeId);
    if( status != EMBER_SUCCESS ){
      emberAfPriceClusterPrintln( "Error: Tariff Cancellation failed: 0x%x", status );
    }
  }
  else{
    emberAfPriceClusterPrintln("Error: Invalid Cancel Tariff");
  }
}

/// BLOCK PERIOD
void emberAfPriceClearBlockPeriodTable( uint8_t endpoint ){
  uint8_t i;
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return;
  }
  for( i=0; i<EMBER_AF_PLUGIN_PRICE_SERVER_BLOCK_PERIOD_TABLE_SIZE; i++ ){
    info.blockPeriodTable.commonInfos[ep][i].valid = 0;
  }
}

void emberAfPluginPriceServerBlockPeriodAdd(uint8_t endpoint,
                                            uint32_t providerId,
                                            uint32_t issuerEventId,
                                            uint32_t blockPeriodStartTime,
                                            uint32_t blockPeriodDuration,
                                            uint8_t  blockPeriodControl,
                                            uint8_t blockPeriodDurationType,
                                            uint32_t thresholdMultiplier,
                                            uint32_t thresholdDivisor,
                                            uint8_t  tariffType,
                                            uint8_t tariffResolutionPeriod )
{
  uint8_t index;
  uint32_t adjStartTime;
  EmberAfPriceCommonInfo * curInfo = NULL;
  EmberAfPriceBlockPeriod * curBlockPeriod = NULL;

  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return;
  }

  adjStartTime = emberAfPluginPriceCommonClusterGetAdjustedStartTime(blockPeriodStartTime,
                                                                     blockPeriodDurationType);


  index = emberAfPluginPriceCommonGetCommonMatchingOrUnusedIndex(info.blockPeriodTable.commonInfos[ep],
                                                                 EMBER_AF_PLUGIN_PRICE_SERVER_BLOCK_PERIOD_TABLE_SIZE,
                                                                 issuerEventId,
                                                                 adjStartTime,
                                                                 true);

  if( index < EMBER_AF_PLUGIN_PRICE_SERVER_BLOCK_PERIOD_TABLE_SIZE ){
    curInfo = &info.blockPeriodTable.commonInfos[ep][index];
    curBlockPeriod = &info.blockPeriodTable.blockPeriods[ep][index];
    curInfo->valid = true;
    curInfo->actionsPending = true;
    curInfo->startTime = adjStartTime;

    curInfo->durationSec = emberAfPluginPriceCommonClusterConvertDurationToSeconds(blockPeriodStartTime,
                                                                                   blockPeriodDuration,
                                                                                   blockPeriodDurationType);
    curInfo->issuerEventId = issuerEventId;

    curBlockPeriod->providerId = providerId;
    if (blockPeriodStartTime == 0x00){
      curBlockPeriod->rawBlockPeriodStartTime = adjStartTime;
    } else {
      curBlockPeriod->rawBlockPeriodStartTime = blockPeriodStartTime;
    }

    curBlockPeriod->blockPeriodDuration     = blockPeriodDuration;
    curBlockPeriod->blockPeriodControl      = blockPeriodControl;
    curBlockPeriod->blockPeriodDurationType = blockPeriodDurationType;
    curBlockPeriod->thresholdMultiplier     = thresholdMultiplier;
    curBlockPeriod->thresholdDivisor        = thresholdDivisor;
    curBlockPeriod->tariffType              = tariffType;
    curBlockPeriod->tariffResolutionPeriod  = tariffResolutionPeriod;

    emberAfPluginPriceServerBlockPeriodUpdateBindings(index);

    emberAfPluginPriceCommonSort(info.blockPeriodTable.commonInfos[ep],
                                 (uint8_t *)info.blockPeriodTable.blockPeriods[ep],
                                 sizeof(EmberAfPriceBlockPeriod),
                                 EMBER_AF_PLUGIN_PRICE_SERVER_BLOCK_PERIOD_TABLE_SIZE);
    emberAfPluginPriceCommonUpdateDurationForOverlappingEvents(info.blockPeriodTable.commonInfos[ep],
                                                               EMBER_AF_PLUGIN_PRICE_SERVER_BLOCK_PERIOD_TABLE_SIZE );
    emberAfPriceClusterScheduleTickCallback(endpoint, EMBER_AF_PRICE_SERVER_CHANGE_BLOCK_PERIOD_EVENT_MASK );
  } else {
    emberAfPriceClusterPrintln("ERR: Unable to add new BlockPeriod entry!");
  }
}

void emberAfPluginPriceServerBlockPeriodPrint( uint8_t endpoint, uint8_t index ){
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return;
  }
  if( index < EMBER_AF_PLUGIN_PRICE_SERVER_BLOCK_PERIOD_TABLE_SIZE ){
    emberAfPriceClusterPrintln("= Block Period [%d]:", index);
    emberAfPriceClusterPrintln("  valid=%d", info.blockPeriodTable.commonInfos[ep][index].valid );
    emberAfPriceClusterPrintln("  providerId=0x%4X", info.blockPeriodTable.blockPeriods[ep][index].providerId );
    emberAfPriceClusterPrintln("  issuerEventId=0x%4X", info.blockPeriodTable.commonInfos[ep][index].issuerEventId );
    emberAfPriceClusterPrintln("  startTime=0x%4X", info.blockPeriodTable.commonInfos[ep][index].startTime );
    emberAfPriceClusterPrintln("  duration=0x%4X", info.blockPeriodTable.commonInfos[ep][index].durationSec );
    emberAfPriceClusterPrintln("  rawBlkPeriodStartTime=0x%4X", info.blockPeriodTable.blockPeriods[ep][index].rawBlockPeriodStartTime );
    emberAfPriceClusterPrintln("  rawBlkPeriodDuration=0x%4X", info.blockPeriodTable.blockPeriods[ep][index].blockPeriodDuration );
    emberAfPriceClusterPrintln("  durationType=0x%X", info.blockPeriodTable.blockPeriods[ep][index].blockPeriodDurationType );
    emberAfPriceClusterPrintln("  blockPeriodControl=0x%X", info.blockPeriodTable.blockPeriods[ep][index].blockPeriodControl );
    emberAfPriceClusterPrintln("  tariffType=0x%X", info.blockPeriodTable.blockPeriods[ep][index].tariffType );
    emberAfPriceClusterPrintln("  tariffResolutionPeriod=0x%X", info.blockPeriodTable.blockPeriods[ep][index].tariffResolutionPeriod );
  }
}

static void emberAfPluginPriceServerBlockPeriodUpdateBindings(uint8_t blockPeriodEntryIndex){
  // Only process BlockPeriod commands for now.
  EmberBindingTableEntry candidate;
  uint8_t i;

  // search for binding entry with Simple Metering cluster.
  for (i = 0; i < EMBER_BINDING_TABLE_SIZE; i++) {
    if (emberGetBinding(i, &candidate) == EMBER_SUCCESS
        && candidate.type == EMBER_UNICAST_BINDING
        && candidate.clusterId == ZCL_PRICE_CLUSTER_ID) {
      EmberNodeId nodeId = emberLookupNodeIdByEui64(candidate.identifier);
      if (nodeId != EMBER_NULL_NODE_ID){
        emberAfPluginPriceServerBlockPeriodPub(nodeId, 
                                               candidate.local,
                                               candidate.remote,
                                               blockPeriodEntryIndex);
      } else {
        emberAfPriceClusterPrintln("ERR: Unable to find nodeId for binding entry(%d).", i);
      }
    }
  }
}

void emberAfPluginPriceServerPriceUpdateBindings(){
  // Only process BlockPeriod commands for now.
  EmberBindingTableEntry candidate;
  uint8_t i;

  // search for binding entry with Simple Metering cluster.
  for (i = 0; i < EMBER_BINDING_TABLE_SIZE; i++) {
    if (emberGetBinding(i, &candidate) == EMBER_SUCCESS
        && candidate.type == EMBER_UNICAST_BINDING
        && candidate.clusterId == ZCL_PRICE_CLUSTER_ID) {
      bool sentMsg = false;
      EmberNodeId nodeId = emberLookupNodeIdByEui64(candidate.identifier);
      if (nodeId != EMBER_NULL_NODE_ID){
        uint8_t priceEntryIndex = emberAfGetCurrentPriceIndex(candidate.local);
        if (priceEntryIndex != 0xFF){
          emberAfPluginPriceServerPublishPriceMessage(nodeId,
                                                      candidate.local,
                                                      candidate.remote,
                                                      priceEntryIndex);
          sentMsg = true;
        }
      }

      if (!sentMsg){
        emberAfPriceClusterPrintln("ERR: Unable to sent PublishPrice command to binding entry(%d).", i);
      }
    }
  }
}


void emberAfPluginPriceServerBlockPeriodPub(uint16_t nodeId,
                                            uint8_t srcEndpoint,
                                            uint8_t dstEndpoint,
                                            uint8_t index){
  uint8_t status;
  uint8_t ep = emberAfFindClusterServerEndpointIndex(srcEndpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return;
  }
  if( (index < EMBER_AF_PLUGIN_PRICE_SERVER_BLOCK_PERIOD_TABLE_SIZE) &&
      (info.blockPeriodTable.commonInfos[ep][index].valid) ){
    emberAfFillCommandPriceClusterPublishBlockPeriod( info.blockPeriodTable.blockPeriods[ep][index].providerId,
                                                      info.blockPeriodTable.commonInfos[ep][index].issuerEventId,
                                                      info.blockPeriodTable.blockPeriods[ep][index].rawBlockPeriodStartTime,
                                                      info.blockPeriodTable.blockPeriods[ep][index].blockPeriodDuration,
                                                      info.blockPeriodTable.blockPeriods[ep][index].blockPeriodControl,
                                                      info.blockPeriodTable.blockPeriods[ep][index].blockPeriodDurationType,
                                                      info.blockPeriodTable.blockPeriods[ep][index].tariffType,
                                                      info.blockPeriodTable.blockPeriods[ep][index].tariffResolutionPeriod);
    emberAfSetCommandEndpoints( srcEndpoint, dstEndpoint );
    emberAfGetCommandApsFrame()->options |= EMBER_APS_OPTION_SOURCE_EUI64;
    status = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, nodeId);
    if( status != EMBER_SUCCESS ){
      emberAfPriceClusterPrintln( "Error: Block Period failed: 0x%x", status );
    }
  }
  else{
    emberAfPriceClusterPrintln("Error: Invalid Block Period");
  }
}

uint32_t emberAfPriceServerSecondsUntilTariffInfoEvent(uint8_t endpoint){
  uint32_t secTillNext;
  uint8_t futureEntryIndex;
  uint8_t i;
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);

  if (ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX) {
    return 0;
  }

  // First, check if the Active index is currently applied.
  i = emberAfPluginPriceCommonServerGetActiveIndex((EmberAfPriceCommonInfo *)info.scheduledTariffTable.commonInfos[ep],
                                       EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE);
  if((i < EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE)
      && (info.scheduledTariffTable.commonInfos[ep][i].actionsPending)){
    // action req'ed.
    return 0;
  }

  futureEntryIndex = emberAfPluginPriceCommonServerGetFutureIndex((EmberAfPriceCommonInfo *)info.scheduledTariffTable.commonInfos[ep],
                                                       EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE,
                                                       &secTillNext);
  if ((futureEntryIndex == ZCL_PRICE_INVALID_INDEX)
      || !info.scheduledTariffTable.commonInfos[ep][futureEntryIndex].actionsPending) {
    secTillNext = ZCL_PRICE_CLUSTER_END_TIME_NEVER; 
  }

  return secTillNext;
}

uint32_t emberAfPriceServerSecondsUntilActivePriceMatrixEvent(uint8_t endpoint){
  uint32_t secTillNext;
  uint8_t futureEntryIndex;
  uint8_t i;
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);

  if (ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX) { return 0; }

  // First, check if the Active index is currently applied.
  i = emberAfPluginPriceCommonServerGetActiveIndex((EmberAfPriceCommonInfo *)info.scheduledPriceMatrixTable.commonInfos[ep],
                                                   EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE);
  if((i < EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE)
      && (info.scheduledPriceMatrixTable.commonInfos[ep][i].actionsPending)){
    // action req'ed.
    return 0;
  }

  futureEntryIndex = emberAfPluginPriceCommonServerGetFutureIndex((EmberAfPriceCommonInfo *)info.scheduledPriceMatrixTable.commonInfos[ep],
                                                                  EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE,
                                                                  &secTillNext);
  if ((futureEntryIndex == ZCL_PRICE_INVALID_INDEX)
      || !info.scheduledPriceMatrixTable.commonInfos[ep][futureEntryIndex].actionsPending) {
    secTillNext = ZCL_PRICE_CLUSTER_END_TIME_NEVER; 
  }

  return secTillNext;
}

uint32_t emberAfPriceServerSecondsUntilActiveBlockThresholdsEvent(uint8_t endpoint){
  uint32_t secTillNext;
  uint8_t futureEntryIndex;
  uint8_t i;
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  EmberAfPriceCommonInfo *inf = info.scheduledBlockThresholdsTable.commonInfos[ep];


  if (ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX) { return 0; }

  // First, check if the Active index is currently applied.
  i = emberAfPluginPriceCommonServerGetActiveIndex(inf,
                                                   EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE);

  if((i < EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE) && (inf[i].actionsPending)){
    // action req'ed.
    return 0;
  }

  futureEntryIndex = emberAfPluginPriceCommonServerGetFutureIndex(inf,
                                                                  EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE,
                                                                  &secTillNext);
  if ((futureEntryIndex == ZCL_PRICE_INVALID_INDEX)
      || !inf[futureEntryIndex].actionsPending) {
    secTillNext = ZCL_PRICE_CLUSTER_END_TIME_NEVER; 
  }

  return secTillNext;
}

uint32_t emberAfPriceServerSecondsUntilBlockPeriodEvent( uint8_t endpoint ){
  uint32_t secTillNext;
  uint32_t startOfBlockPeriod;
  EmberAfStatus status;
  uint8_t i;
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return 0xFFFFFFFF;
  }

  // First, check if the Active index is currently applied.
  i = emberAfPluginPriceCommonServerGetActiveIndex( (EmberAfPriceCommonInfo *)&info.blockPeriodTable.commonInfos[ep], EMBER_AF_PLUGIN_PRICE_SERVER_BLOCK_PERIOD_TABLE_SIZE );
  if( i < EMBER_AF_PLUGIN_PRICE_SERVER_BLOCK_PERIOD_TABLE_SIZE ){
    // Check if the block period attributes match this block period.
    status = emberAfReadAttribute(endpoint,
                                  ZCL_PRICE_CLUSTER_ID,
                                  ZCL_START_OF_BLOCK_PERIOD_ATTRIBUTE_ID,
                                  CLUSTER_MASK_SERVER,
                                  (uint8_t *)&startOfBlockPeriod,
                                  4,
                                  NULL);
    if( (status == EMBER_ZCL_STATUS_SUCCESS)
        && (startOfBlockPeriod != info.blockPeriodTable.commonInfos[ep][i].startTime) 
        && (info.blockPeriodTable.commonInfos[ep][i].actionsPending)){
      // Attributes are not current.
      return 0;
    }
  }

  emberAfPluginPriceCommonServerGetFutureIndex(info.blockPeriodTable.commonInfos[ep],
                                               EMBER_AF_PLUGIN_PRICE_SERVER_BLOCK_PERIOD_TABLE_SIZE,
                                               &secTillNext);
  return secTillNext;
}

void emberAfPriceServerRefreshBlockPeriod( uint8_t endpoint, bool repeat ){
  uint8_t i;
  EmberAfPriceCommonInfo * curInfo = NULL;
  EmberAfPriceBlockPeriod * curBlockPeriod = NULL;
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);

  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return;
  }

  i = emberAfPluginPriceCommonServerGetActiveIndex((EmberAfPriceCommonInfo *)&info.blockPeriodTable.commonInfos[ep],
                                                   EMBER_AF_PLUGIN_PRICE_SERVER_BLOCK_PERIOD_TABLE_SIZE  );

  if( i >=  EMBER_AF_PLUGIN_PRICE_SERVER_BLOCK_PERIOD_TABLE_SIZE ){
    return;
  }

  curInfo = &info.blockPeriodTable.commonInfos[ep][i];
  curBlockPeriod = &info.blockPeriodTable.blockPeriods[ep][i];

  if (repeat) {
    // Make sure to check whether the current block period calls for it to
    // repeat, before we add a new entry.
    if (curInfo->valid
        && (curBlockPeriod->blockPeriodControl
            & EMBER_AF_BLOCK_PERIOD_CONTROL_REPEATING_BLOCK)) {
      // The .startTime and .durationSec members are both UTC values measured
      // in seconds. The nextStartTime is therefore a UTC value. Set the start
      // time of the next period to the end time of the current period.
      uint32_t nextStartTime = (curInfo->startTime + curInfo->durationSec);
      // The event ID is supposed to be a unique value for each different piece
      // of information from the commodity provider. A higher event ID is supposed
      // to mean that the information is newer. Therefore, let's make sure this
      // next billing period has a higher event ID than the current period.
      uint32_t nextIssuerEventId = (curInfo->issuerEventId + 1);
      emberAfPluginPriceServerBlockPeriodAdd(endpoint,
                                             curBlockPeriod->providerId,
                                             nextIssuerEventId,
                                             nextStartTime,
                                             curBlockPeriod->blockPeriodDuration,
                                             curBlockPeriod->blockPeriodControl,
                                             curBlockPeriod->blockPeriodDurationType,
                                             curBlockPeriod->thresholdMultiplier,
                                             curBlockPeriod->thresholdDivisor,
                                             curBlockPeriod->tariffType,
                                             curBlockPeriod->tariffResolutionPeriod);
    }
    return;
  }

  emberAfWriteServerAttribute(endpoint,
                              ZCL_PRICE_CLUSTER_ID,
                              ZCL_START_OF_BLOCK_PERIOD_ATTRIBUTE_ID,
                              (uint8_t *)&curBlockPeriod->rawBlockPeriodStartTime,
                              ZCL_UTC_TIME_ATTRIBUTE_TYPE );
  emberAfWriteServerAttribute(endpoint,
                              ZCL_PRICE_CLUSTER_ID,
                              ZCL_BLOCK_PERIOD_DURATION_MINUTES_ATTRIBUTE_ID,
                              (uint8_t *)&curBlockPeriod->blockPeriodDuration,
                              ZCL_INT24U_ATTRIBUTE_TYPE );
  emberAfWriteServerAttribute(endpoint,
                              ZCL_PRICE_CLUSTER_ID,
                              ZCL_THRESHOLD_MULTIPLIER_ATTRIBUTE_ID,
                              (uint8_t *)&curBlockPeriod->thresholdMultiplier,
                              ZCL_INT24U_ATTRIBUTE_TYPE );
  emberAfWriteServerAttribute(endpoint,
                              ZCL_PRICE_CLUSTER_ID,
                              ZCL_THRESHOLD_DIVISOR_ATTRIBUTE_ID,
                              (uint8_t *)&curBlockPeriod->thresholdDivisor,
                              ZCL_INT24U_ATTRIBUTE_TYPE );
  emberAfWriteServerAttribute(endpoint,
                              ZCL_PRICE_CLUSTER_ID,
                              ZCL_BLOCK_PERIOD_DURATION_TYPE_ATTRIBUTE_ID,
                              (uint8_t *)&curBlockPeriod->blockPeriodDurationType,
                              ZCL_BITMAP8_ATTRIBUTE_TYPE );

  emberAfPluginPriceServerNewActiveBlockPeriodInformationCallback(curInfo,
                                                                  curBlockPeriod);
  curInfo->actionsPending = false;
}

void emberAfPriceServerRefreshPriceMatrixInformation(uint8_t endpoint) {
  uint8_t i;
  EmberAfPriceCommonInfo * curInfo = NULL;
  EmberAfScheduledPriceMatrix * curPm = NULL;
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);

  if (ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX){ return; }

  i = emberAfPluginPriceCommonServerGetActiveIndex((EmberAfPriceCommonInfo *)info.scheduledPriceMatrixTable.commonInfos[ep],
                                                   EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE);

  if ((i >= EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE)
      || !info.scheduledPriceMatrixTable.commonInfos[ep][i].actionsPending) {
    return;
  }

  // Notify application 
  curInfo = &info.scheduledPriceMatrixTable.commonInfos[ep][i];
  curPm = &info.scheduledPriceMatrixTable.scheduledPriceMatrix[ep][i];
  emberAfPluginPriceServerNewActivePriceMatrixCallback(curInfo, curPm);
  curInfo->actionsPending = false;
}

void emberAfPriceServerRefreshBlockThresholdsInformation(uint8_t endpoint) {
  uint8_t i;
  EmberAfPriceCommonInfo * curInfo = NULL;
  EmberAfScheduledBlockThresholds * curBt = NULL;
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);

  if (ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX){ return; }

  i = emberAfPluginPriceCommonServerGetActiveIndex((EmberAfPriceCommonInfo *)info.scheduledBlockThresholdsTable.commonInfos[ep],
                                                   EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE);

  if ((i >= EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE)
      || !info.scheduledBlockThresholdsTable.commonInfos[ep][i].actionsPending) {
    return;
  }

  // Notify application 
  curInfo = &info.scheduledBlockThresholdsTable.commonInfos[ep][i];
  curBt = &info.scheduledBlockThresholdsTable.scheduledBlockThresholds[ep][i];
  emberAfPluginPriceServerNewActiveBlockThresholdsInformationCallback(curInfo, curBt);
  curInfo->actionsPending = false;
}

void emberAfPriceServerRefreshTariffInformation(uint8_t endpoint){
  uint8_t i;
  EmberAfScheduledTariff * tariff = NULL;
  EmberAfPriceCommonInfo * curInfo = NULL;
  EmberAfTariffType tariffType = EMBER_ZCL_TARIFF_TYPE_DELIVERED_TARIFF;
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);

  if (ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX){
    return;
  }

  i = emberAfPluginPriceCommonServerGetActiveIndex((EmberAfPriceCommonInfo *)info.scheduledTariffTable.commonInfos[ep],
                                       EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE);

  if ((i >= EMBER_AF_PLUGIN_PRICE_SERVER_TARIFF_TABLE_SIZE)
      || !info.scheduledTariffTable.commonInfos[ep][i].actionsPending) {
    return;
  }


  // start updating attributes
  curInfo = &info.scheduledTariffTable.commonInfos[ep][i];
  tariff = &info.scheduledTariffTable.scheduledTariffs[ep][i];
  tariffType = (EmberAfTariffType) tariff->tariffTypeChargingScheme & TARIFF_TYPE_MASK;

  // notify application for newly activated Tariff Information
  emberAfPluginPriceServerNewActiveTariffInformationCallback(curInfo, tariff);
  
  if ((tariffType == EMBER_ZCL_TARIFF_TYPE_DELIVERED_TARIFF)
      || (tariffType == EMBER_ZCL_TARIFF_TYPE_DELIVERED_AND_RECEIVED_TARIFF)){
    emberAfPriceClusterPrintln("Info: Updating ZCL attributes with following tariff information.");
    emberAfWriteAttribute(endpoint, ZCL_PRICE_CLUSTER_ID,
                          ZCL_TARIFF_LABEL_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
                          (uint8_t*)&(tariff->tariffLabel), ZCL_OCTET_STRING_ATTRIBUTE_TYPE);
    emberAfWriteAttribute(endpoint, ZCL_PRICE_CLUSTER_ID,
                          ZCL_NUMBER_OF_PRICE_TIERS_IN_USE_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
                          &tariff->numberOfPriceTiersInUse, ZCL_INT8U_ATTRIBUTE_TYPE);
    emberAfWriteAttribute(endpoint, ZCL_PRICE_CLUSTER_ID,
                          ZCL_NUMBER_OF_BLOCK_THRESHOLDS_IN_USE_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
                          &tariff->numberOfBlockThresholdsInUse, ZCL_INT8U_ATTRIBUTE_TYPE);
    emberAfWriteAttribute(endpoint, ZCL_PRICE_CLUSTER_ID,
                          ZCL_TIER_BLOCK_MODE_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
                          &tariff->tierBlockMode, ZCL_INT8U_ATTRIBUTE_TYPE);
    emberAfWriteAttribute(endpoint, ZCL_PRICE_CLUSTER_ID,
                          ZCL_TARIFF_UNIT_OF_MEASURE_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
                          &tariff->unitOfMeasure, ZCL_INT8U_ATTRIBUTE_TYPE);
    emberAfWriteAttribute(endpoint, ZCL_PRICE_CLUSTER_ID,
                          ZCL_TARIFF_CURRENCY_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
                          (uint8_t *)&tariff->currency, ZCL_INT16U_ATTRIBUTE_TYPE);
    emberAfWriteAttribute(endpoint, ZCL_PRICE_CLUSTER_ID,
                          ZCL_TARIFF_PRICE_TRAILING_DIGIT_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
                          &tariff->priceTrailingDigit, ZCL_INT8U_ATTRIBUTE_TYPE);
    emberAfWriteAttribute(endpoint, ZCL_PRICE_CLUSTER_ID,
                          ZCL_STANDING_CHARGE_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
                          (uint8_t *)&tariff->standingCharge, ZCL_INT32U_ATTRIBUTE_TYPE);
    emberAfWriteAttribute(endpoint, ZCL_PRICE_CLUSTER_ID,
                          ZCL_THRESHOLD_MULTIPLIER_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
                          (uint8_t *)&tariff->blockThresholdMultiplier, ZCL_INT32U_ATTRIBUTE_TYPE);
    emberAfWriteAttribute(endpoint, ZCL_PRICE_CLUSTER_ID,
                          ZCL_THRESHOLD_DIVISOR_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
                          (uint8_t *)&tariff->blockThresholdDivisor, ZCL_INT32U_ATTRIBUTE_TYPE);
    curInfo->actionsPending = false;
  }

  if ((tariffType == EMBER_ZCL_TARIFF_TYPE_RECEIVED_TARIFF)
      || (tariffType == EMBER_ZCL_TARIFF_TYPE_DELIVERED_AND_RECEIVED_TARIFF)){
    emberAfWriteAttribute(endpoint, ZCL_PRICE_CLUSTER_ID,
                          ZCL_RX_TARIFF_LABEL_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
                          (uint8_t*)&(tariff->tariffLabel), ZCL_OCTET_STRING_ATTRIBUTE_TYPE);
    emberAfWriteAttribute(endpoint, ZCL_PRICE_CLUSTER_ID,
                          ZCL_RX_NUMBER_OF_PRICE_TIERS_IN_USE_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
                          &tariff->numberOfPriceTiersInUse, ZCL_INT8U_ATTRIBUTE_TYPE);
    emberAfWriteAttribute(endpoint, ZCL_PRICE_CLUSTER_ID,
                          ZCL_RX_NUMBER_OF_BLOCK_THRESHOLDS_IN_USE_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
                          &tariff->numberOfBlockThresholdsInUse, ZCL_INT8U_ATTRIBUTE_TYPE);
    emberAfWriteAttribute(endpoint, ZCL_PRICE_CLUSTER_ID,
                          ZCL_RX_TIER_BLOCK_MODE_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
                          &tariff->tierBlockMode, ZCL_INT8U_ATTRIBUTE_TYPE);
    emberAfWriteAttribute(endpoint, ZCL_PRICE_CLUSTER_ID,
                          ZCL_RX_THRESHOLD_MULTIPLIER_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
                          (uint8_t *)&tariff->blockThresholdMultiplier, ZCL_INT32U_ATTRIBUTE_TYPE);
    emberAfWriteAttribute(endpoint, ZCL_PRICE_CLUSTER_ID,
                          ZCL_RX_THRESHOLD_DIVISOR_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
                          (uint8_t *)&tariff->blockThresholdDivisor, ZCL_INT32U_ATTRIBUTE_TYPE);
     curInfo->actionsPending = false;
  }

}

bool emberAfPriceClusterGetBlockPeriodsCallback(uint32_t startTime, uint8_t numberOfEvents, uint8_t tariffType){

  uint8_t  validEntries[EMBER_AF_PLUGIN_PRICE_SERVER_BLOCK_PERIOD_TABLE_SIZE];
  uint8_t  validEntriesIndex = 0;
  uint32_t endTimeUtc;
  uint8_t  i;

  uint8_t endpoint = emberAfCurrentEndpoint();
  uint8_t ep = emberAfFindClusterServerEndpointIndex(endpoint, ZCL_PRICE_CLUSTER_ID);
  if( ep == ZCL_PRICE_INVALID_ENDPOINT_INDEX ){
    return false;
  }
  emberAfPriceClusterPrintln("RX: Get Block Period");
  if( startTime == 0 ){
    startTime = emberAfGetCurrentTime();
  }

  for( i=0; i<EMBER_AF_PLUGIN_PRICE_SERVER_BLOCK_PERIOD_TABLE_SIZE; i++ ){
    if( info.blockPeriodTable.commonInfos[ep][i].valid ){
      if((tariffType == TARIFF_TYPE_UNSPECIFIED)
          || ((info.blockPeriodTable.blockPeriods[ep][i].tariffType & TARIFF_TYPE_MASK) == (tariffType & TARIFF_TYPE_MASK))){
        // According to "Get Consolidated Bill" command documentation, this response should 
        // include consolidated bills that are already active.  So, send back any with
        // end time that is > earliestStartTime.
        endTimeUtc = info.blockPeriodTable.commonInfos[ep][i].startTime + info.blockPeriodTable.commonInfos[ep][i].durationSec;
        if( (startTime < endTimeUtc) ){
          validEntries[validEntriesIndex] = i;
          // NOTE:  Incrementing validEntriesIndex first ensures that all entries are sent if numberOfEvents == 0.
          validEntriesIndex++;
          if( validEntriesIndex == numberOfEvents ){
            break;
          }
        }
      }
    }
  }
  // Have a set of valid block periods.  Total number == validEntriesIndex.
  sendValidCmdEntries( ZCL_PUBLISH_BLOCK_PERIOD_COMMAND_ID, ep, validEntries, validEntriesIndex );


  return true;
}

bool emberAfPriceClusterPriceAcknowledgementCallback(uint32_t providerId,
                                                        uint32_t issuerEventId,
                                                        uint32_t priceAckTime,
                                                        uint8_t control)
{
  emberAfPriceClusterPrintln("RX: PriceAcknowledgement 0x%4x, 0x%4x, 0x%4x, 0x%x",
                             providerId,
                             issuerEventId,
                             priceAckTime,
                             control);
  return true;
}

