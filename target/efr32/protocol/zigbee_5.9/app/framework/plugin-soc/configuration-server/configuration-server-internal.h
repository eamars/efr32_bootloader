// Copyright 2016 Silicon Laboratories, Inc.                                *80*

#ifndef __CONFIGURATION_SERVER_INTERNAL_H__
#define __CONFIGURATION_SERVER_INTERNAL_H__

/** @brief Generic call to read the data from the tokens using the creator.  
 *
 * Local method for reading the configurable tokens from their respective 
 * creators.  Note:  as this is used by the cluster command, data needs to be
 * in the format required for the local command.
 *
 */
void emAfPluginConfigurationServerReadTokenDataFromCreator( uint16_t creator, uint8_t *data );


#endif