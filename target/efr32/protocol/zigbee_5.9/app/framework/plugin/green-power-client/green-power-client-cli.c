#include "app/framework/include/af.h"
//#include "green-power-proxy-table.h"
#include "green-power-client.h"

#ifndef EMBER_AF_GENERATE_CLI
#error The Green Power Client plugin is not compatible with the legacy CLI.
#endif


void emberAfPluginGreenPowerClientSetProxyEntry(void)
{
  /*
  uint8_t index = emberUnsignedCommandArgument(0);
  uint32_t srcID = emberUnsignedCommandArgument(1);
  uint16_t sinkNodeID = emberUnsignedCommandArgument(2);
  EmberGpProxyTableEntry entry;

  entry.gpd.applicationId = 0;
  entry.gpd.id.sourceId = srcID;
  entry.options = emberUnsignedCommandArgument(3);
  entry.status = EMBER_GP_PROXY_TABLE_ENTRY_STATUS_ACTIVE;
  entry.securityOptions = 0;
  entry.sinkList[0].type = EMBER_GP_SINK_TYPE_LW_UNICAST;
  entry.sinkList[1].type = EMBER_GP_SINK_TYPE_UNUSED;
  entry.sinkList[0].target.target.unicast.sinkNodeId = sinkNodeID;
  entry.sinkList[0].target.target.unicast.sinkEUI[0] = 0x84;
  entry.sinkList[0].target.target.unicast.sinkEUI[1] = 0x40;
  entry.sinkList[0].target.target.unicast.sinkEUI[2] = 0x18;
  entry.sinkList[0].target.target.unicast.sinkEUI[3] = 0x00;
  entry.sinkList[0].target.target.unicast.sinkEUI[4] = 0x00;
  entry.sinkList[0].target.target.unicast.sinkEUI[5] = 0x00;
  entry.sinkList[0].target.target.unicast.sinkEUI[6] = 0x00;
  entry.sinkList[0].target.target.unicast.sinkEUI[7] = 0x00;
  entry.gpdSecurityFrameCounter = 0x00;

  emGpProxyTableSetEntry(index, &entry);
  */
}

void emberAfPluginGreenPowerClientAddSink(void)
{
  /*
  EmberGpAddress addr;
  uint32_t srcID = emberUnsignedCommandArgument(0);
  EmberEUI64 eui64;
  emberCopyBigEndianEui64Argument(1, eui64);
  addr.applicationId = 0;
  addr.id.sourceId = srcID;

  uint8_t index = emGpProxyTableFindOrAllocateEntry(&addr);
  if (index != 0xFF) {
    emGpProxyTableAddSink(index,
                          EMBER_GP_SINK_TYPE_LW_UNICAST,
                          eui64,
                          EMBER_UNKNOWN_NODE_ID,
                          0,
                          0xFFFF);
  }
  */
}
void emberAfPluginGreenPowerClientAddGroupcastSink(void)
{
  /*
  EmberGpAddress addr;
  uint32_t srcID = emberUnsignedCommandArgument(0);
  uint16_t groupID = (uint16_t) emberUnsignedCommandArgument(1);
  addr.applicationId = 0;
  addr.id.sourceId = srcID;

  uint8_t index = emGpProxyTableFindOrAllocateEntry(&addr);
  if (index != 0xFF) {
    emGpProxyTableAddSink(index,
                          EMBER_GP_SINK_TYPE_GROUPCAST,
                          0,
                          EMBER_UNKNOWN_NODE_ID,
                          groupID,
                          0xFFFF);
  }
  */

}

void emberAfPluginGreenPowerClientRemoveProxyTableEntry(void)
{
  /*
  EmberGpAddress addr;
  uint32_t srcID = emberUnsignedCommandArgument(0);
  addr.applicationId = 0;
  addr.id.sourceId = srcID;
  emGpProxyTableRemoveEntry(emGpProxyTableLookup(&addr));
  */
}

void emberAfPluginGreenPowerClientPrintProxyTable(void)
{
  /*
  uint8_t i, j;
  emberAfGreenPowerClusterPrint("Proxy Table:\n");
  for (i = 0; i < emGpProxyTableSize; i++) {
    if (emGpProxyTableEntryInUse(i)) {
      emberAfCorePrint("%d %4x ",i,emGpProxyTable[i].gpd);
      for (j = 0; j < 2; j++) {
        if (emGpProxyTable[i].sinkList[j].type == EMBER_GP_SINK_TYPE_UNUSED) {
          emberAfCorePrint("unused");
        } else if (emGpProxyTable[i].sinkList[j].type == EMBER_GP_SINK_TYPE_GROUPCAST) {
          emberAfCorePrint("GC %2x",emGpProxyTable[i].sinkList[j].target.groupcast.groupID);
        } else if (emGpProxyTable[i].sinkList[j].type == EMBER_GP_SINK_TYPE_LW_UNICAST) {
          emberAfCorePrint("LU:");
          emberAfPrintBigEndianEui64(emGpProxyTable[i].sinkList[j].target.unicast.sinkEUI);
        }
        emberAfCorePrint(" ");
      }
      emberAfCorePrint(" ");
      for (j = 0; j < EMBER_ENCRYPTION_KEY_SIZE; j++)
        emberAfCorePrint("%x",emGpProxyTable[i].gpdKey.contents[j]);
      emberAfCorePrint("\n");



    }
  }

*/
}

void emberAfPluginGreenPowerClientClearProxyTable(void)
{
  /*
  emGpClearProxyTable();
  */
}

void emberAfPluginGreenPowerClientDuplicateFilteringTest(void)
{
  /*
  EmberGpAddress sender;
  sender.endpoint = (uint8_t) emberUnsignedCommandArgument(0);
  sender.applicationId = EMBER_GP_APPLICATION_SOURCE_ID;
  sender.id.sourceId = emberUnsignedCommandArgument(1);
  emGpMessageChecking(&sender, (uint8_t) emberUnsignedCommandArgument(2));
  //emAfGreenPowerFindDuplicateMacSeqNum(&sender, (uint8_t)emberUnsignedCommandArgument(2));
  */
}

void emberAfPluginGreenPowerClientSetKey(void)
{
  /*
  uint8_t index;
  EmberKeyData keyData;
  index = emberUnsignedCommandArgument(0);
  emberCopyKeyArgument(1, &keyData);
  emGpProxyTableSetKey(index, (keyData.contents));
*/
}
