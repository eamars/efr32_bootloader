//

// This callback file is created for your convenience. You may add application
// code to this file. If you regenerate this file over a previous version, the
// previous version will be overwritten and any code you have added will be
// lost.

#include "app/framework/include/af.h"
#include <stdlib.h>
#include "app/framework/plugin/gas-proxy-function/gas-proxy-function.h"
#include "app/framework/plugin/gbz-message-controller/gbz-message-controller.h"




/** @brief Fragment Transmission Failed
 *
 * This function is called by the Interpan plugin when a fragmented transmission
 * has failed.
 *
 * @param interpanFragmentationStatus The status describing why transmission
 * failed  Ver.: always
 * @param fragmentNum The fragment number that encountered the failure  Ver.:
 * always
 */
void emberAfPluginInterpanFragmentTransmissionFailedCallback(int8u interpanFragmentationStatus,
                                                             int8u fragmentNum)
{
}

/** @brief Message Received Over Fragments
 *
 * This function is called by the Interpan plugin when a fully reconstructed
 * message has been received over inter-PAN fragments, or IPMFs.
 *
 * @param header The inter-PAN header  Ver.: always
 * @param msgLen The message payload length  Ver.: always
 * @param message The message payload  Ver.: always
 */
void emberAfPluginInterpanMessageReceivedOverFragmentsCallback(const EmberAfInterpanHeader *header,
                                                               int8u msgLen,
                                                               int8u *message)
{
}


