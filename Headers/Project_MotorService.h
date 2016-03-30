/****************************************************************************
 
  Header file for template service 
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef Project_MotorService_H
#define Project_MotorService_H

#include "ES_Types.h"

typedef enum { FORWARD, REVERSE, LEFT, RIGHT } Direction_t ; // For keeping track of robot direction

// Public Function Prototypes

bool InitMotorService ( uint8_t Priority );
bool PostMotorService( ES_Event ThisEvent );
ES_Event RunMotorService( ES_Event ThisEvent );
void InputCaptureResponse0(void);
void InputCaptureResponse1(void);
void TimeoutResponse0(void);
void TimeoutResponse1(void);
void ControllerInterruptResponse(void);
void OnLineMotorSpeedsInterrupt(void);
void EnableLineFollowInts(void);
void DisableLineFollowInts(void); 

void SetMotorDutyCycles(uint8_t DutyCycleLeft, uint8_t DutyCycleRight, Direction_t Direction);
void SetMotorRPM(uint32_t RPMleft, uint32_t RPMright);

#endif /* ServTemplate_H */

