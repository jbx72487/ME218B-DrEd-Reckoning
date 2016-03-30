/****************************************************************************
 
  Header file for template service 
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef PWM_SERVO_H
#define PWM_SERVO_H

#include "ES_Types.h"

// Public Function Prototypes

void InitServoPWM (uint8_t driveFreq);

void ServoSetDutyCycle (uint8_t dutyCycle);

void CloseFeedGate(void );
void OpenFeedGate(void );

#endif /* PWM_SERVO_H */

