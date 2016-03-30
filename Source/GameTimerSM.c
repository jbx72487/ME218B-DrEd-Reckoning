/****************************************************************************
 Module
   GameTimerSM.c

 Revision
   2.0.1

 Description
   This is a template file for implementing state machines.

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 02/07/13 21:00 jec      corrections to return variable (should have been
                         ReturnEvent, not CurrentEvent) and several EV_xxx
                         event names that were left over from the old version
 02/08/12 09:56 jec      revisions for the Events and Services Framework Gen2
 02/13/10 14:29 jec      revised Start and run to add new kind of entry function
                         to make implementing history entry cleaner
 02/13/10 12:29 jec      added NewEvent local variable to During function and
                         comments about using either it or Event as the return
 02/11/10 15:54 jec      more revised comments, removing last comment in during
                         function that belongs in the run function
 02/09/10 17:21 jec      updated comments about internal transitions on During funtion
 02/18/09 10:14 jec      removed redundant call to RunLowerlevelSM in EV_Entry
                         processing in During function
 02/20/07 21:37 jec      converted to use enumerated type for events & states
 02/13/05 19:38 jec      added support for self-transitions, reworked
                         to eliminate repeated transition code
 02/11/05 16:54 jec      converted to implment hierarchy explicitly
 02/25/03 10:32 jec      converted to take a passed event parameter
 02/18/99 10:19 jec      built template from MasterMachine.c
 02/14/99 10:34 jec      Began Coding
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
// Basic includes for a program using the Events and Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"

/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "GameTimerSM.h"
#include "MasterSM.h"
#include "EventPrinter.h"

/*----------------------------- Module Defines ----------------------------*/
// define constants for the states for this machine
// and any other local defines

#define ENTRY_STATE GAME_TIME_IDLE

#define ONE_MIN 60000
#define ONE_SEC 1000

#define GAME_TIME_SECS 138

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine, things like during
   functions, entry & exit functions.They should be functions relevant to the
   behavior of this state machine
*/
static ES_Event DuringGameTimeIdle( ES_Event Event);
static ES_Event DuringGameTimeCounting( ES_Event Event);


/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
static GameTimerState_t CurrentState;
static uint8_t counter;
static uint16_t lastMinuteTime;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
    RunGameTimerSM

 Parameters
   ES_Event: the event to process

 Returns
   ES_Event: an event to return

 Description
   add your description here
 Notes
   uses nested switch/case to implement the machine.
 Author
   J. Edward Carryer, 2/11/05, 10:45AM
****************************************************************************/
ES_Event RunGameTimerSM( ES_Event CurrentEvent )
{
   bool MakeTransition = false;/* are we making a state transition? */
   GameTimerState_t NextState = CurrentState;
   ES_Event EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
   ES_Event ReturnEvent = CurrentEvent; // assume we are not consuming event

   switch ( CurrentState )
   {
       case GAME_TIME_IDLE :       // If current state is state one
         // Execute During function for state one. ES_ENTRY & ES_EXIT are
         // processed here allow the lower level state machines to re-map
         // or consume the event
         CurrentEvent = DuringGameTimeIdle(CurrentEvent);
         //process any events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
               case GAME_STARTED : //If event is event one
                  // Execute action function for state one : event one
                  NextState = GAME_TIME_COUNTING;//Decide what the next state will be
                  // for internal transitions, skip changing MakeTransition
                  MakeTransition = true; //mark that we are taking a transition
                  // if transitioning to a state with history change kind of entry
                  EntryEventKind.EventType = ES_ENTRY_HISTORY;
                  // optionally, consume or re-map this event for the upper
                  // level state machine
                  ReturnEvent.EventType = ES_NO_EVENT;

                  // start game timer for 2:18 / 3
                  // printf("Starting game timer\r\n");

                  break;
                // repeat cases as required for relevant events
            }
         }
         break;
      // repeat state pattern as required for other states
       case GAME_TIME_COUNTING :       // If current state is state one
         // Execute During function for state one. ES_ENTRY & ES_EXIT are
         // processed here allow the lower level state machines to re-map
         // or consume the event
         CurrentEvent = DuringGameTimeCounting(CurrentEvent);
         //process any events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
               case ES_TIMEOUT : //If event is event one
                  if (CurrentEvent.EventParam == GAME_TIMER) {

                      // for internal transitions, skip changing MakeTransition
                      MakeTransition = true; //mark that we are taking a transition
                      // if transitioning to a state with history change kind of entry
                      // EntryEventKind.EventType = ES_ENTRY_HISTORY;
                      // optionally, consume or re-map this event for the upper
                      // level state machine
                      ReturnEvent.EventType = ES_NO_EVENT;
                      counter ++;
										
                      // printf("%d minute have passed.\r\n", counter);
											
											// Execute action function for state one : event one
                      NextState = GAME_TIME_COUNTING;//Decide what the next state will be
                  }
                  break;
                // repeat cases as required for relevant events
               case GAME_OVER : //If event is event one
										// for internal transitions, skip changing MakeTransition
										MakeTransition = true; //mark that we are taking a transition
										// if transitioning to a state with history change kind of entry
										// EntryEventKind.EventType = ES_ENTRY_HISTORY;
										// optionally, consume or re-map this event for the upper
										// level state machine
										ReturnEvent.EventType = ES_NO_EVENT;
									
										// printf("%d minute have passed.\r\n", counter);
										
										// Execute action function for state one : event one
										NextState = GAME_TIME_IDLE;//Decide what the next state will be
                  break;

            }
         }
         break;
    }
    //   If we are making a state transition
    if (MakeTransition == true)
    {
       //   Execute exit function for current state
       CurrentEvent.EventType = ES_EXIT;
       RunGameTimerSM(CurrentEvent);

       CurrentState = NextState; //Modify state variable

       //   Execute entry function for new state
       // this defaults to ES_ENTRY
       RunGameTimerSM(EntryEventKind);
     }
     return(ReturnEvent);
}
/****************************************************************************
 Function
     StartGameTimerSM

 Parameters
     None

 Returns
     None

 Description
     Does any required initialization for this state machine
 Notes

 Author
     J. Edward Carryer, 2/18/99, 10:38AM
****************************************************************************/
void StartGameTimerSM ( ES_Event CurrentEvent )
{
   // to implement entry to a history state or directly to a substate
   // you can modify the initialization of the CurrentState variable
   // otherwise just start in the entry state every time the state machine
   // is started
   if ( ES_ENTRY_HISTORY != CurrentEvent.EventType )
   {
        CurrentState = ENTRY_STATE;
   }
   // call the entry function (if any) for the ENTRY_STATE
   RunGameTimerSM(CurrentEvent);
}

/****************************************************************************
 Function
     QueryGameTimerSM

 Parameters
     None

 Returns
     TemplateState_t The current state of the Template state machine

 Description
     returns the current state of the Template state machine
 Notes

 Author
     J. Edward Carryer, 2/11/05, 10:38AM
****************************************************************************/
GameTimerState_t QueryGameTimerSM ( void )
{
   return(CurrentState);
}

/***************************************************************************
 private functions
 ***************************************************************************/

static ES_Event DuringGameTimeIdle( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        counter = 0;
        // after that start any lower level machines that run in this state
        //StartLowerLevelSM( Event );
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        //RunLowerLevelSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
      
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
        // ReturnEvent = RunLowerLevelSM(Event);
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}


static ES_Event DuringGameTimeCounting( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
				ES_Timer_InitTimer(GAME_TIMER,ONE_MIN);
				lastMinuteTime = ES_Timer_GetTime();
        // after that start any lower level machines that run in this state
        //StartLowerLevelSM( Event );
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        //RunLowerLevelSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
      
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
        // ReturnEvent = RunLowerLevelSM(Event);
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

int16_t GetTimeLeft (void) {
	// returns the number of seconds left in the game
	uint8_t totalSec = counter * 60;
	uint32_t thisTime = ES_Timer_GetTime();
	if (thisTime > lastMinuteTime)
		totalSec += ((thisTime - lastMinuteTime) / ONE_SEC);
	else {
		// if the time has rolled over, subtract the difference from 
		thisTime += (1<<16);
		totalSec += ((thisTime - lastMinuteTime) / ONE_SEC);
	}
	// printf("thisTime: %d \t lastMinuteTime: %d \t counter: %d \t totalSec: %d\r\n", thisTime, lastMinuteTime, counter, totalSec);
	return ((int16_t) GAME_TIME_SECS - (int16_t) totalSec);
}

bool Check4GameOver (void) {
	int16_t timeLeft = GetTimeLeft();
	// printf("Time remaining: %d\r\n", timeLeft);
	if (timeLeft <= 0) {
		ES_Event Event2Post;
		Event2Post.EventType = GAME_OVER;
		PostMasterSM(Event2Post);
		return true;
	} else
		return false;
}
