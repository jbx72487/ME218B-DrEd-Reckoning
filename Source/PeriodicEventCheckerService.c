/****************************************************************************
 Module
   TimeLEDService.c

 Revision
   1.0.1

 Description
   This is a template file for implementing a simple service under the 
   Gen2 Events and Services Framework.

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 01/16/12 09:58 jec      began conversion from TemplateFSM.c
 11/09/15 	    sam		 using for ME218A Project
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "PeriodicEventCheckerService.h"
#include "SwitchesHW.h"
#include "FindingCityNavSM.h"
#include "LineSensorsHW.h"
#include "MasterSM.h"
#include "EventPrinter.h"
#include "GameTimerSM.h"
#include "CitySensorHW.h"
/*----------------------------- Module Defines ----------------------------*/

#define PERIODIC_EVENT_CHECKER_TIME 20
#define NUM_SWITCHES 7
#define CHECK_GAME_STATUS_TIME 1000

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service 
*/
bool Check4SwitchChange( void);

/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;
static SwitchStatus_t switchStatus[NUM_SWITCHES];
static uint8_t counter;


/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitPeriodicEventCheckerService

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and does any 
     other required initialization for this service
 Notes

 Author
     J. Edward Carryer, 01/16/12, 10:00
****************************************************************************/
bool InitPeriodicEventCheckerService ( uint8_t Priority )
{
  ES_Event ThisEvent;

  MyPriority = Priority;
  /********************************************
   in here you write your initialization code
   *******************************************/
   // store & read initial switch statuses
   for (int i = 0; i < NUM_SWITCHES; i++) {
    switchStatus[i] = ReadSwitch(i);
   }
   // start timer
   ES_Timer_InitTimer(PERIODIC_EVENT_CHECKER_TIMER,PERIODIC_EVENT_CHECKER_TIME);

  // post the initial transition event
  ThisEvent.EventType = ES_INIT;
  if (ES_PostToService( MyPriority, ThisEvent) == true)
  {
      return true;
  }else
  {
      return false;
  }
}

/****************************************************************************
 Function
     PostPeriodicEventCheckerService

 Parameters
     EF_Event ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise

 Description
     Posts an event to this state machine's queue
 Notes

 Author
     J. Edward Carryer, 10/23/11, 19:25
****************************************************************************/
bool PostPeriodicEventCheckerService( ES_Event ThisEvent )
{
	counter = 0;
  return ES_PostToService( MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunPeriodicEventCheckerService

 Parameters
   ES_Event : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   add your description here
 Notes
   
 Author
   J. Edward Carryer, 01/15/12, 15:23
****************************************************************************/
ES_Event RunPeriodicEventCheckerService( ES_Event ThisEvent )
{
  ES_Event ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
  /********************************************
   in here you write your service code
   *******************************************/
	

   if (ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == PERIODIC_EVENT_CHECKER_TIMER) {
		// printf("Debounce timer expired.\r\n");
      UpdateAllLineReadings();
     	// UNCOMMENT IF FRONT_LINE AGAIN Check4FrontOnBlackLine();
		  Check4CenterOnBlackLine();
			Check4LineAligned();
			Check4SwitchChange();
		 // printf("City period reading: %d\r\n", GetActualCityPeriodUS());

		 if (counter == 0) 
			 Check4GameOver();
		 counter = (counter + 1) % (CHECK_GAME_STATUS_TIME / PERIODIC_EVENT_CHECKER_TIME);
    // restart timer 
     ES_Timer_InitTimer(PERIODIC_EVENT_CHECKER_TIMER,PERIODIC_EVENT_CHECKER_TIME);
   }
  return ReturnEvent;
}

bool Check4SwitchChange( void) {
	// debounces switches and repors any changes
	for (int i = 0; i < NUM_SWITCHES; i++) {
      SwitchStatus_t temp = ReadSwitch(i);
			// printf("Switch %d: %d \t",i, temp);
      // if current status is different from last recorded value, post appropriate button changed event
      if (switchStatus[i] != temp) {
        ES_Event Event2Post;

        if (i == SWITCH_CAM) {
          if (temp == SWITCH_ON) {
            Event2Post.EventType = CAM_SWITCH_PRESSED;
          }
        } else if ((i == SWITCH_0) || (i == SWITCH_10)) {
          if (temp == SWITCH_ON) {
            Event2Post.EventType = FRONT_BUMPER_HIT;
          }
        } else if ((i == SWITCH_4) || (i == SWITCH_6)) {
          if (temp == SWITCH_ON) {
            Event2Post.EventType = REAR_BUMPER_HIT;
          }
        } else if (i == SWITCH_2 || i == SWITCH_8) {
					return false;
				}
				
				switchStatus[i] = temp;
				// printf("A switch changed.\r\n");
				PostMasterSM(Event2Post);
				PostEventPrinter(Event2Post);
				return true;
      }
			// printf ("\r\n");
    }
	return false;
}

SwitchStatus_t GetSwitchStatus(WhichSwitch_t whichSwitch) {
  return switchStatus[whichSwitch];
}

/***************************************************************************
 private functions
 ***************************************************************************/

/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

