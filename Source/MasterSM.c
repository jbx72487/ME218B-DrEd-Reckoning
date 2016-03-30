/****************************************************************************
 Module
   MasterSM.c

 Revision
   2.0.1

 Description
   This is the top level MasterSM machine

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 02/11/16 01:39 jbx      converted from TopHSMTemplate.c
 02/08/12 01:39 jec      converted from MW_MasterMachine.c
 02/06/12 22:02 jec      converted to Gen 2 Events and Services Framework
 02/13/10 11:54 jec      converted During functions to return Event_t
                         so that they match the template
 02/21/07 17:04 jec      converted to pass Event_t to Start...()
 02/20/07 21:37 jec      converted to use enumerated type for events
 02/21/05 15:03 jec      Began Coding
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "MasterSM.h"
#include "BotSM.h"
#include "GameTimerSM.h"
#include "EnablePA25_PB23_PD7_PF0.h"
#include "TeamColorSwitchHW.h"
#include "PACCommSM.h"
#include "LEDsHW.h"
#include "LineSensorsHW.h"
#include "IRSensorHW.h"
#include "CitySensorHW.h"
#include "SSIHW.h"
#include "CAMMotorHW.h"
#include "SwitchesHW.h"
// #include "PWMServo.h"
/*----------------------------- Module Defines ----------------------------*/

#define STATE_ONE 0
#define SERVO_PWM_FREQ 50

/*---------------------------- Module Functions ---------------------------*/

static ES_Event DuringStateOne( ES_Event Event);


/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, though if the top level state machine
// is just a single state container for orthogonal regions, you could get
// away without it
static uint8_t CurrentState;
// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitMasterSM

 Parameters
     uint8_t : the priorty of this service

 Returns
     boolean, False if error in initialization, True otherwise

 Description
     Saves away the priority,  and starts
     the top level state machine
 Notes

 Author
     J. Edward Carryer, 02/06/12, 22:06
****************************************************************************/
bool InitMasterSM ( uint8_t Priority )
{
  ES_Event ThisEvent;

  MyPriority = Priority;  // save our priority

  ThisEvent.EventType = ES_ENTRY;
  // Start the Master State machine

  StartMasterSM( ThisEvent );

	// printf("MasterSM initialized.\r\n");
  return true;
}

/****************************************************************************
 Function
     PostMasterSM

 Parameters
     ES_Event ThisEvent , the event to post to the queue

 Returns
     boolean False if the post operation failed, True otherwise

 Description
     Posts an event to this state machine's queue
 Notes

 Author
     J. Edward Carryer, 10/23/11, 19:25
****************************************************************************/
bool PostMasterSM( ES_Event ThisEvent )
{
  return ES_PostToService( MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunMasterSM

 Parameters
   ES_Event: the event to process

 Returns
   ES_Event: an event to return

 Description
   the run function for the top level state machine 
 Notes
   uses nested switch/case to implement the machine.
 Author
   J. Edward Carryer, 02/06/12, 22:09
****************************************************************************/
ES_Event RunMasterSM( ES_Event CurrentEvent )
{
   bool MakeTransition = false;/* are we making a state transition? */
   uint8_t NextState = CurrentState;
   ES_Event EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
   ES_Event ReturnEvent = { ES_NO_EVENT, 0 }; // assume no error

    switch ( CurrentState )
   {
       case STATE_ONE :       // If current state is state one
         // Execute During function for state one. ES_ENTRY & ES_EXIT are
         // processed here allow the lowere level state machines to re-map
         // or consume the event
         CurrentEvent = DuringStateOne(CurrentEvent);
         break;
      // repeat state pattern as required for other states
    }

    //   If we are making a state transition
    if (MakeTransition == true)
    {
       //   Execute exit function for current state
       CurrentEvent.EventType = ES_EXIT;
       RunMasterSM(CurrentEvent);

       CurrentState = NextState; //Modify state variable

       // Execute entry function for new state
       // this defaults to ES_ENTRY
       RunMasterSM(EntryEventKind);
     }
   // in the absence of an error the top level state machine should
   // always return ES_NO_EVENT, which we initialized at the top of func
   return(ReturnEvent);
}

/****************************************************************************
 Function
     StartMasterSM

 Parameters
     ES_Event CurrentEvent

 Returns
     nothing

 Description
     Does any required initialization for this state machine
 Notes

 Author
     J. Edward Carryer, 02/06/12, 22:15
****************************************************************************/
void StartMasterSM ( ES_Event CurrentEvent )
{
  // if there is more than 1 state to the top level machine you will need 
  // to initialize the state variable
  CurrentState = STATE_ONE;
  // now we need to let the Run function init the lower level state machines
  // use LocalEvent to keep the compiler from complaining about unused var
  RunMasterSM(CurrentEvent);
	// printf("MasterSM started.\r\n");

  return;
}


/***************************************************************************
 private functions
 ***************************************************************************/


static ES_Event DuringStateOne( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        
        // after that start any lower level machines that run in this state
			  StartPACCommSM(Event);
        StartBotSM(Event);
        // repeat the StartxxxSM() functions for concurrent state machines
        StartGameTimerSM(Event);

        // on the lower level
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        RunPACCommSM(Event);
        RunBotSM(Event);
        // repeat for any concurrently running state machines
        RunGameTimerSM(Event);

        // now do any local exit functionality
      
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
        // ReturnEvent = RunLowerLevelSM(Event);
      ReturnEvent = RunPACCommSM(Event);
      ReturnEvent = RunBotSM(Event);
        // repeat for any concurrent lower level machines
      ReturnEvent = RunGameTimerSM(Event);
        // do any activity that is repeated as long as we are in this state
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}



void HardwareInit( void ) {
	PortFunctionInit();
	InitLineSensorsHW();
	InitCitySensorHW();
	InitIRSensorHW();
	InitTeamColorSwitchHW();
	InitLEDsHW();
	InitSSIHW();
	InitSwitchesHW();
	InitCAMMotorHW();
	// InitServoPWM(SERVO_PWM_FREQ);
}
