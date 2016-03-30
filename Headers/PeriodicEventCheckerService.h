/****************************************************************************
 
  Header file for template service 
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef PeriodicEventCheckerService_H
#define PeriodicEventCheckerService_H

#include "ES_Types.h"
#include "SwitchesHW.h"

// Public Function Prototypes

bool InitPeriodicEventCheckerService ( uint8_t Priority );
bool PostPeriodicEventCheckerService( ES_Event ThisEvent );
ES_Event RunPeriodicEventCheckerService( ES_Event ThisEvent );
SwitchStatus_t GetSwitchStatus(WhichSwitch_t whichSwitch);

#endif /* PeriodicEventCheckerService_H */

