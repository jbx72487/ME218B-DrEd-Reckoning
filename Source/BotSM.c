/****************************************************************************
 Module
   BotSM.c

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
#include "MasterSM.h"
#include "AtCityCommSM.h"
#include "BotSM.h"
#include "TeamColorSwitchHW.h"
#include "LEDsHW.h"
#include "PACCommSM.h"
#include "EventPrinter.h"
#include "IRSensorHW.h"
#include "AtCityShootSM.h"
#include "CitySensorHW.h"
#include "FindingCityNavSM.h"
#include "Project_MotorService.h"
#include "LineSensorsHW.h"
/*----------------------------- Module Defines ----------------------------*/
// define constants for the states for this machine
// and any other local definesq

#define ENTRY_STATE BOT_IDLE
#define GAME_STATUS_BIT BIT0HI
#define STATUS_REQUEST_BYTE 0xC0
#define LEAVING_CITY_TIME 500
#define AT_CITY_IDLE_TIME 3500 // TODO 37856 is max

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine, things like during
   functions, entry & exit functions.They should be functions relevant to the
   behavior of this state machine
*/
static ES_Event DuringBotIdle( ES_Event Event);
static ES_Event DuringBotFindingCity( ES_Event Event);
static ES_Event DuringBotAtCity( ES_Event Event);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
static BotState_t CurrentState;
static TeamColor_t teamColor;
static bool readyForNewCity;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
    RunBotSM

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
ES_Event RunBotSM( ES_Event CurrentEvent )
{
	// printf("Running bot.\r\n");
   bool MakeTransition = false;/* are we making a state transition? */
   BotState_t NextState = CurrentState;
   ES_Event EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
   ES_Event ReturnEvent = { ES_NO_EVENT, 0 }; // assume no error

    switch ( CurrentState )
   {
       case BOT_IDLE :       // If current state is state one
         // Execute During function for state one. ES_ENTRY & ES_EXIT are
         // processed here allow the lowere level state machines to re-map
         // or consume the event
         CurrentEvent = DuringBotIdle(CurrentEvent);
         //process any events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
               case GAME_STARTED : //If event is event one
                  // Execute action function for state one : event one
									readyForNewCity = true;
									ResetBallCounter();
                  NextState = BOT_FINDING_CITY;//Decide what the next state will be
                  // for internal transitions, skip changing MakeTransition
                  MakeTransition = true; //mark that we are taking a transition
                  // if transitioning to a state with history change kind of entry
                  // EntryEventKind.EventType = ES_ENTRY_HISTORY;
                  // optionally, consume or re-map this event for the upper
                  // level state machine
                  ReturnEvent.EventType = ES_NO_EVENT;

                  break;
                // repeat cases as required for relevant events
              case PAC_MSG_READY:
                 NextState = BOT_IDLE;//Decide what the next state will be
                  // for internal transitions, skip changing MakeTransition
                  MakeTransition = false; //mark that we are taking a transition
                  // if transitioning to a state with history change kind of entry
                  // EntryEventKind.EventType = ES_ENTRY_HISTORY;
                  // optionally, consume or re-map this event for the upper
                  // level state machine
                  ReturnEvent.EventType = ES_NO_EVENT;
								  printf("PAC status: %#010x\t\r\n",GetPACMsg());
                  // if campaign is active, post GAME_STARTED message. otherwise, post SEND2PAC with request status message
                  ES_Event Event2Post;

                  if ((GetPACMsg() & GAME_STATUS_BIT) == GAME_STATUS_BIT) {
                    Event2Post.EventType = GAME_STARTED;
										PostEventPrinter(Event2Post);
                  
                  } else {
                    Event2Post.EventType = SEND2PAC;
                    Event2Post.EventParam = STATUS_REQUEST_BYTE;
										// printf("Campaign inactive.\r\n");
										/* for debugging
										if (QueryPACCommSM() == PAC_READY) printf("PAC ready");
										else if (QueryPACCommSM() == PAC_SENDING) printf("PAC NOT READY: PAC_SENDING!");
										else if (QueryPACCommSM() == PAC_WAITING) printf("PAC NOT READY: PAC_WAITING!");
										*/
									}

                  PostMasterSM(Event2Post);
                 break;
            }
         }
         break;
      // repeat state pattern as required for other states

         case BOT_FINDING_CITY :       // If current state is state one
         // Execute During function for state one. ES_ENTRY & ES_EXIT are
         // processed here allow the lowere level state machines to re-map
         // or consume the event
         CurrentEvent = DuringBotFindingCity(CurrentEvent);
         //process any events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
               case CITY_FOUND : //If event is event one
								 if (readyForNewCity == true) {
										// Execute action function for state one : event one
										NextState = BOT_AT_CITY;//Decide what the next state will be
										// for internal transitions, skip changing MakeTransition
										MakeTransition = true; //mark that we are taking a transition
										// if transitioning to a state with history change kind of entry
										// EntryEventKind.EventType = ES_ENTRY_HISTORY;
										// optionally, consume or re-map this event for the upper
										// level state machine
										ReturnEvent.EventType = ES_NO_EVENT;
								 }
								 break;

                // repeat cases as required for relevant events
                case GAME_OVER : //If event is event one
                  // Execute action function for state one : event one
									
									NextState = BOT_IDLE;//Decide what the next state will be
									// for internal transitions, skip changing MakeTransition
									MakeTransition = true; //mark that we are taking a transition
									// if transitioning to a state with history change kind of entry
									// EntryEventKind.EventType = ES_ENTRY_HISTORY;
									// optionally, consume or re-map this event for the upper
									// level state machine
									ReturnEvent.EventType = ES_NO_EVENT;
								
                  break;
								case ES_TIMEOUT : //If event is event one
                  if (CurrentEvent.EventParam == LEAVING_CITY_TIMER) {
										// Execute action function for state one : event one
										readyForNewCity = true;
										NextState = BOT_FINDING_CITY;//Decide what the next state will be
										// for internal transitions, skip changing MakeTransition
										MakeTransition = false; //mark that we are taking a transition
										// if transitioning to a state with history change kind of entry
										// EntryEventKind.EventType = ES_ENTRY_HISTORY;
										// optionally, consume or re-map this event for the upper
										// level state machine

										ReturnEvent.EventType = ES_NO_EVENT;
									}
								
                  break;
            }
         }
         break;

         case BOT_AT_CITY :       // If current state is state one
         // Execute During function for state one. ES_ENTRY & ES_EXIT are
         // processed here allow the lowere level state machines to re-map
         // or consume the event
         CurrentEvent = DuringBotAtCity(CurrentEvent);
         //process any events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
               case CITY_COMPLETED : //If event is event one
									ClearFindCityCounter();
                // Execute action function for state one : event one
									NextState = BOT_FINDING_CITY;//Decide what the next state will be
									// for internal transitions, skip changing MakeTransition
									MakeTransition = true; //mark that we are taking a transition
									// if transitioning to a state with history change kind of entry
									EntryEventKind.EventType = ES_ENTRY_HISTORY;
									// optionally, consume or re-map this event for the upper
									// level state machine
									ReturnEvent.EventType = ES_NO_EVENT;
									
									// start timer for leaving city
									ES_Timer_InitTimer(LEAVING_CITY_TIMER, LEAVING_CITY_TIME);
									// make us unable to stop for a city until LEAVE_TIMER expires
									readyForNewCity = false;
                  break;
								case ES_TIMEOUT : //If event is event one
                // Execute action function for state one : event one
                  if (CurrentEvent.EventParam == IDLE_TIMER) 
                  {
                    NextState = BOT_FINDING_CITY;//Decide what the next state will be
                    // for internal transitions, skip changing MakeTransition
                    MakeTransition = true; //mark that we are taking a transition
                    // if transitioning to a state with history change kind of entry
                    // EntryEventKind.EventType = ES_ENTRY_HISTORY;
										// optionally, consume or re-map this event for the upper
										// level state machine
										ReturnEvent.EventType = ES_NO_EVENT;
										
										// start timer for leaving city
										ES_Timer_InitTimer(LEAVING_CITY_TIMER, LEAVING_CITY_TIME);
										// make us unable to stop for a city until LEAVE_TIMER expires
										readyForNewCity = false;
                  }
                  break;
                // repeat cases as required for relevant events
                case GAME_OVER : //If event is event one
                  // Execute action function for state one : event one
									NextState = BOT_IDLE;//Decide what the next state will be
									// for internal transitions, skip changing MakeTransition
									MakeTransition = true; //mark that we are taking a transition
									// if transitioning to a state with history change kind of entry
									// EntryEventKind.EventType = ES_ENTRY_HISTORY;
									// optionally, consume or re-map this event for the upper
									// level state machine
									ReturnEvent.EventType = ES_NO_EVENT;
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
       RunBotSM(CurrentEvent);

       CurrentState = NextState; //Modify state variable

       // Execute entry function for new state
       // this defaults to ES_ENTRY
       RunBotSM(EntryEventKind);
     }
   // in the absence of an error the top level state machine should
   // always return ES_NO_EVENT, which we initialized at the top of func
   return(ReturnEvent);
}


/****************************************************************************
 Function
     StartBotSM

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
void StartBotSM ( ES_Event CurrentEvent )
{
   // to implement entry to a history state or directly to a substate
   // you can modify the initialization of the CurrentState variable
   // otherwise just start in the entry state every time the state machine
   // is started
   if ( ES_ENTRY_HISTORY != CurrentEvent.EventType )
   {
        CurrentState = ENTRY_STATE;
   }

   teamColor = RED;

   // call the entry function (if any) for the ENTRY_STATE
   RunBotSM(CurrentEvent);
}

/****************************************************************************
 Function
     QueryBotSM

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
BotState_t QueryBotSM ( void )
{
   return(CurrentState);
}


TeamColor_t GetTeamColor( void ) {
  return teamColor;
}

/***************************************************************************
 private functions
 ***************************************************************************/

static ES_Event DuringBotIdle( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        // turn off team LEDs
        SetLEDOff(RED_LED);
        SetLEDOff(BLUE_LED);
				SetLEDOff(CAMPAIGN_LED);

			  DisableCitySensorInts();
				// kill time				
				for (int i = 0; i < 1000000; i++) {}

        ES_Event Event2Post;
        Event2Post.EventType = SEND2PAC;
        Event2Post.EventParam = STATUS_REQUEST_BYTE;
        // PostEventPrinter(Event2Post);
				PostMasterSM(Event2Post);

        // after that start any lower level machines that run in this state
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        // repeat for any concurrently running state machines
        // now do any local exit functionality
        
        // read and store team color
        teamColor = ReadTeamColor();
				// teamColor = BLUE;
        // printf("Reading team color & setting LED\r\n");
        switch (teamColor) {
          case RED:
            SetLEDOn(RED_LED);
            printf("Team color: RED");
            break;
          case BLUE:
            SetLEDOn(BLUE_LED);
            printf("Team color: BLUE");
            break;
        }
				SetLEDOn(CAMPAIGN_LED);
				SetShouldShootList(teamColor);
        SetTargetBasedOnMyTeam(teamColor);
				EnableCitySensorInts();

      
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
        // printf("During BOT_IDLE.\r\n");

    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

static ES_Event DuringBotFindingCity( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        
        // after that start any lower level machines that run in this state
        //StartLowerLevelSM( Event );
			StartFindingCityNavSM(Event);
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        //RunLowerLevelSM(Event);
			RunFindingCityNavSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
      SetMotorDutyCycles(0,0, REVERSE); // TODO ideally replace with speed
			ClearLineSensorReadings();
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
        // ReturnEvent = RunLowerLevelSM(Event);
      ReturnEvent = RunFindingCityNavSM(Event);
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
        // printf("During BOT_FINDING_CITY.\r\n");

    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

static ES_Event DuringBotAtCity( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        ES_Timer_InitTimer(IDLE_TIMER, AT_CITY_IDLE_TIME);

        // after that start any lower level machines that run in this state
        //StartLowerLevelSM( Event );
        StartAtCityCommSM(Event);
				StartAtCityShootSM(Event);
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        //RunLowerLevelSM(Event);
        RunAtCityCommSM(Event);
				RunAtCityShootSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
        ES_Timer_StopTimer(IDLE_TIMER);
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
        // ReturnEvent = RunLowerLevelSM(Event);
        RunAtCityCommSM(Event);
				RunAtCityShootSM(Event);
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
        // printf("During BOT_AT_CITY.\r\n");

    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}
