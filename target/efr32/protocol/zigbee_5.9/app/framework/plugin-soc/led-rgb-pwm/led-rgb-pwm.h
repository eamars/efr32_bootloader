// *******************************************************************
// * led-rgb-pwm.h
// *
// *
// * Copyright 2015 Silicon Laboratories, Inc.                              *80*
// *******************************************************************
#ifndef __LED_RGB_PWM_H__
#define __LED_RGB_PWM_H__

void emberAfLedRgbPwmComputeRgbFromXy( uint8_t endpoint );
void emberAfLedRgbPwmComputeRgbFromColorTemp( uint8_t endpoint );
void emberAfLedRgbComputeRgbFromHSV( uint8_t endpoint );

#endif
