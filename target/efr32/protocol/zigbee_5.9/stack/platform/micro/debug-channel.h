/** @file stack/platform/micro/debug-channel.h
* 
* @brief Functions that provide access to the debug channel.
 *
 * <!-- Copyright 2005 by Ember Corporation. All rights reserved.-->   
 */

#ifndef __DEBUG_CHANNEL_H__
#define __DEBUG_CHANNEL_H__

/** @name Debug Channel Functions */

//@{

EmberStatus halStackDebugPutBuffer(EmberMessageBuffer buffer);
uint8_t emDebugAddInitialFraming(uint8_t *buff, uint16_t debugType);

#if DEBUG_LEVEL >= BASIC_DEBUG
EmberStatus emDebugInit(void);
void emDebugPowerDown(void);
void emDebugPowerUp(void);
bool halStackDebugActive(void);
void emDebugReceiveData(void);
#else
#define emDebugInit() EMBER_SUCCESS
#define emDebugPowerDown()
#define emDebugPowerUp()
#define emDebugReceiveData()
#endif

//@}  // end of Debug Channel Functions

#endif //__DEBUG_CHANNEL_H__
