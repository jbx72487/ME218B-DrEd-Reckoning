/****************************************************************************
 Module
   AtCityCommSM.c

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

#include "AtCityCommSM.h"
#include "SSIHW.h"
#include "MasterSM.h"
#include "EventPrinter.h"
#include "BotSM.h"
#include "CitySensorHW.h"
#include "PACCommSM.h"
#include "AtCityShootSM.h"

/*----------------------------- Module Defines ----------------------------*/
// define constants for the states for this machine
// and any other local defines

#define ENTRY_STATE AT_CITY_REQUESTING

#define REQUEST_WAIT_TIME 180

#define RED_REQUEST_BYTE 0x80
#define BLUE_REQUEST_BYTE 0xB0
#define QUERY_BYTE 0x70
#define RECEIVING_BYTE 0x00

#define RESPONSE_READY_MASK (0xFF << 2*8)
#define RESPONSE_IS_READY (0xAA << 2*8)

#define CITY_MASK (0x0F << 1*8)

#define ACK_MASK (0xC0 << 1*8)
#define ACK_IS_NACK (0x00 << 1*8)
#define ACK_IS_ACK (0x40 << 1*8)
#define ACK_IS_BLOCKED (0x80 << 1*8)
#define ACK_IS_BUSY (0xC0 << 1*8)

#define COLOR_MASK (0x30 << 1*8)
#define COLOR_IS_BLUE (0x10 << 1*8)
#define COLOR_IS_RED (0x20 << 1*8)

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine, things like during
   functions, entry & exit functions.They should be functions relevant to the
   behavior of this state machine
*/
static ES_Event DuringAtCityRequesting( ES_Event Event);
static ES_Event DuringAtCityWaiting( ES_Event Event);
static ES_Event DuringAtCityQuerying( ES_Event Event);
static ES_Event DuringAtCityDone( ES_Event Event);
static uint8_t ConvertToFreqCode(uint16_t cityPeriod);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
static AtCityCommState_t CurrentState;
static uint8_t ackCounter;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
    RunAtCityCommSM

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
ES_Event RunAtCityCommSM( ES_Event CurrentEvent )
{
   bool MakeTransition = false;/* are we making a state transition? */
   AtCityCommState_t NextState = CurrentState;
   ES_Event EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
   ES_Event ReturnEvent = CurrentEvent; // assume we are not consuming event
	
	 uint32_t msg;
	 
   switch ( CurrentState )
   {
       case AT_CITY_REQUESTING :       // If current state is state one
         // Execute During function for state one. ES_ENTRY & ES_EXIT are
         // processed here allow the lower level state machines to re-map
         // or consume the event
         CurrentEvent = DuringAtCityRequesting(CurrentEvent);
         //process any events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
               case PAC_MSG_READY : //If event is event one
								 if (ackCounter == 1) {
										// Execute action function for state one : event one
										NextState = AT_CITY_QUERYING;//Decide what the next state will be
										// for internal transitions, skip changing MakeTransition
										MakeTransition = true; //mark that we are taking a transition
										// if transitioning to a state with history change kind of entry
										// EntryEventKind.EventType = ES_ENTRY_HISTORY;
										// optionally, consume or re-map this event for the upper
										// level state machine
										ReturnEvent.EventType = ES_NO_EVENT;
								 } else if (ackCounter == 0) {
										// Execute action function for state one : event one
										NextState = AT_CITY_WAITING;//Decide what the next state will be
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
							 case BAD_FREQ : //If event is event one
                  // Execute action function for state one : event one
                  NextState = AT_CITY_REQUESTING;//Decide what the next state will be
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
      // repeat state pattern as required for other states
       case AT_CITY_WAITING :       // If current state is state one
           // Execute During function for state one. ES_ENTRY & ES_EXIT are
           // processed here allow the lower level state machines to re-map
           // or consume the event
           CurrentEvent = DuringAtCityWaiting(CurrentEvent);
           //process any events
           if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
           {
              switch (CurrentEvent.EventType)
              {
								case ES_TIMEOUT : //If event is event one
                  if (CurrentEvent.EventParam == REQUEST_WAIT_TIMER) {
										// Execute action function for state one : event one
										// printf("200ms request timer after first request expired. About to continue capturing.\r\n");
										NextState = AT_CITY_QUERYING;//Decide what the next state will be
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
              }
           }
          break;
				case AT_CITY_QUERYING :       // If current state is state one
         // Execute During function for state one. ES_ENTRY & ES_EXIT are
         // processed here allow the lower level state machines to re-map
         // or consume the event
         CurrentEvent = DuringAtCityQuerying(CurrentEvent);
         //process any events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
						

            switch (CurrentEvent.EventType)
            {
               case PAC_MSG_READY : //If event is event one
									msg = GetPACMsg();
									// printf("PAC msg: %#10x \r\n",msg);
                  // if the response is ready, look @ the response
									if ((msg & RESPONSE_READY_MASK) == RESPONSE_IS_READY) {
										if ((msg & ACK_MASK) != ACK_IS_NACK) {
											// printf("PSC msg: %#10x \r\n", msg);
											// if request wasn't acknowledged or we were blocked, return to requesting & set counter to zero
											ES_Event Event2Post;
											Event2Post.EventType = SHOOT_CITY;
											Event2Post.EventParam = (msg & CITY_MASK) >> 1*8;
											PostMasterSM(Event2Post);
											PostEventPrinter(Event2Post);
										}

                    if (((msg & COLOR_MASK) == COLOR_IS_RED && GetTeamColor() == RED) || ((msg & COLOR_MASK) == COLOR_IS_BLUE && GetTeamColor() == BLUE) ) {
											printf("City color = team color. City completed.\r\n");
											NextState = AT_CITY_DONE;//Decide what the next state will be
											// for internal transitions, skip changing MakeTransition
											MakeTransition = true; //mark that we are taking a transition
											// if transitioning to a state with history change kind of entry
											// EntryEventKind.EventType = ES_ENTRY_HISTORY;
											// optionally, consume or re-map this event for the upper
											// level state machine
											ReturnEvent.EventType = ES_NO_EVENT;
										} else if ((msg & ACK_MASK) == ACK_IS_NACK) {
                      printf("City not acknowledged \r\n");
                      ackCounter = 0;
  										NextState = AT_CITY_REQUESTING;//Decide what the next state will be
                      // for internal transitions, skip changing MakeTransition
                      MakeTransition = true; //mark that we are taking a transition
                      // if transitioning to a state with history change kind of entry
                      // EntryEventKind.EventType = ES_ENTRY_HISTORY;
                      // optionally, consume or re-map this event for the upper
                      // level state machine
                      ReturnEvent.EventType = ES_NO_EVENT;
										}
										else if ((msg & ACK_MASK) == ACK_IS_BLOCKED) {
											// if we're out of balls or if we're supposed to shoot from this city, stay here and keep requesting
											if (GetBallCount() == 0 || GetCityShootStatus((msg & CITY_MASK) >> 1*8) == true) {
												printf("City blocked and either we're supposed to shoot here or we're out of balls, so we're staying\r\n");
												ackCounter = 0;
												NextState = AT_CITY_REQUESTING;//Decide what the next state will be
												// for internal transitions, skip changing MakeTransition
												MakeTransition = true; //mark that we are taking a transition
												// if transitioning to a state with history change kind of entry
												// EntryEventKind.EventType = ES_ENTRY_HISTORY;
												// optionally, consume or re-map this event for the upper
												// level state machine
												ReturnEvent.EventType = ES_NO_EVENT;
											} else {
												printf("City blocked and nothing interesting to do here, so we're gonna try to shoot from another city.\r\n");
												NextState = AT_CITY_DONE;//Decide what the next state will be
												// for internal transitions, skip changing MakeTransition
												MakeTransition = true; //mark that we are taking a transition
												// if transitioning to a state with history change kind of entry
												// EntryEventKind.EventType = ES_ENTRY_HISTORY;
												// optionally, consume or re-map this event for the upper
												// level state machine
												ReturnEvent.EventType = ES_NO_EVENT;
											}
										}
                    // otherwise, if request was acknowledged...
                    else if ((msg & ACK_MASK) == ACK_IS_ACK) {
                      if (ackCounter == 0) {
                        printf("Request 1 of 2 acknowledged\r\n");

                        // if it's the first time our request was acknowledged, increment ackCounter & keep REQUESTING
                        ackCounter++;
                        NextState = AT_CITY_REQUESTING;//Decide what the next state will be
                        // for internal transitions, skip changing MakeTransition
                        MakeTransition = true; //mark that we are taking a transition
                        // if transitioning to a state with history change kind of entry
                        // EntryEventKind.EventType = ES_ENTRY_HISTORY;
                        // optionally, consume or re-map this event for the upper
                        // level state machine
                        ReturnEvent.EventType = ES_NO_EVENT;
                      } else { // if it's not the first time, go to AT_CITY_DONE, post CITY_COMPLETED if relevant
                        printf("Request 2 of 2 acknowledged\r\n");
                        NextState = AT_CITY_DONE;//Decide what the next state will be
                        // for internal transitions, skip changing MakeTransition
                        MakeTransition = true; //mark that we are taking a transition
                        // if transitioning to a state with history change kind of entry
                        // EntryEventKind.EventType = ES_ENTRY_HISTORY;
                        // optionally, consume or re-map this event for the upper
                        // level state machine
                        ReturnEvent.EventType = ES_NO_EVENT;
                      }
                    }
                  } else { // if response isn't ready, keep querying
                    NextState = AT_CITY_QUERYING;//Decide what the next state will be
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
            }
         }
         break;
			 case AT_CITY_DONE :       // If current state is state one
           // Execute During function for state one. ES_ENTRY & ES_EXIT are
           // processed here allow the lower level state machines to re-map
           // or consume the event
           CurrentEvent = DuringAtCityDone(CurrentEvent);
           //process any events
           if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
           {
              switch (CurrentEvent.EventType)
              {
                  // repeat cases as required for relevant events
              }
           }
          break;
    }
    //   If we are making a state transition
    if (MakeTransition == true)
    {
       //   Execute exit function for current state
       CurrentEvent.EventType = ES_EXIT;
       RunAtCityCommSM(CurrentEvent);

       CurrentState = NextState; //Modify state variable

       //   Execute entry function for new state
       // this defaults to ES_ENTRY
       RunAtCityCommSM(EntryEventKind);
     }
     return(ReturnEvent);
}
/****************************************************************************
 Function
     StartAtCityCommSM

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
void StartAtCityCommSM ( ES_Event CurrentEvent )
{
   // to implement entry to a history state or directly to a substate
   // you can modify the initialization of the CurrentState variable
   // otherwise just start in the entry state every time the state machine
   // is started
   if ( ES_ENTRY_HISTORY != CurrentEvent.EventType )
   {
        CurrentState = ENTRY_STATE;
   }
	 // printf("AtCityCommSM started.\r\n");
	 ackCounter = 0;
   // call the entry function (if any) for the ENTRY_STATE
   RunAtCityCommSM(CurrentEvent);
}

/****************************************************************************
 Function
     QueryAtCityCommSM

 Parameters
     None

 Returns
     AtCityCommState_t The current state of the Template state machine

 Description
     returns the current state of the Template state machine
 Notes

 Author
     J. Edward Carryer, 2/11/05, 10:38AM
****************************************************************************/
AtCityCommState_t QueryAtCityCommSM ( void )
{
   return(CurrentState);
}

/***************************************************************************
 private functions
 ***************************************************************************/

static ES_Event DuringAtCityRequesting( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        // write request to SSI
				uint16_t cityPeriod = GetCityPeriodUS();
				if (cityPeriod > 2000) printf("CAN'T READ CITY FREQ.\r\n");
				ES_Event Event2Post;
				uint8_t cityFreqCode = ConvertToFreqCode(cityPeriod);
				Event2Post.EventType = SEND2PAC;
				
				if (GetTeamColor() == RED) {
					Event2Post.EventParam = (RED_REQUEST_BYTE | cityFreqCode);
				} else {
					Event2Post.EventParam = (BLUE_REQUEST_BYTE | cityFreqCode);
				}
				printf("city period: %d \t freq: %#03x \t byte: %#04x \r\n",cityPeriod, cityFreqCode, Event2Post.EventParam);

				PostMasterSM(Event2Post);
				PostEventPrinter(Event2Post);
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
				// printf("During AT_CITY_REQUESTING.\r\n");

    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

static ES_Event DuringAtCityWaiting( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
				ES_Timer_InitTimer(REQUEST_WAIT_TIMER, REQUEST_WAIT_TIME);


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

        // if game state is campaigning, post GAME_STARTED event

    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
        // ReturnEvent = RunLowerLevelSM(Event);
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
				// printf("During AT_CITY_QUERYING.\r\n");

    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

static ES_Event DuringAtCityQuerying( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
				ES_Event Event2Post;
				Event2Post.EventType = SEND2PAC;
        Event2Post.EventParam = QUERY_BYTE;
				PostMasterSM(Event2Post);
				// PostEventPrinter(Event2Post);  
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

        // if game state is campaigning, post GAME_STARTED event

    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
        // ReturnEvent = RunLowerLevelSM(Event);
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
				// printf("During AT_CITY_QUERYING.\r\n");

    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}


static ES_Event DuringAtCityDone( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        ackCounter = 0;
				if (QueryAtCityShootSM() == SHOOT_DONE) {
					ES_Event Event2Post;
					Event2Post.EventType = CITY_COMPLETED;
					// Event2Post.EventParam = (msg & CITY_MASK) >> 1*8;
					PostMasterSM(Event2Post);
					PostEventPrinter(Event2Post);
				}

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

        // if game state is campaigning, post GAME_STARTED event

    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
        // ReturnEvent = RunLowerLevelSM(Event);
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
				// printf("During AT_CITY_QUERYING.\r\n");

    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}


static uint8_t ConvertToFreqCode(uint16_t cityPeriod) {
// convert uint16_t city period to code of magnetic field
// before entering this function, these values are confirmed to be [472, 1361]
  if (cityPeriod > 1305)
    return 0;
  else if (cityPeriod > 1249)
    return 1;
  else if (cityPeriod > 1194)
    return 2;
  else if (cityPeriod > 1110)
    return 3;
  else if (cityPeriod > 1083)
    return 4;
  else if (cityPeriod > 1027)
    return 5;
  else if (cityPeriod > 972)
    return 6;
  else if (cityPeriod > 916)
    return 7;
  else if (cityPeriod > 861)
    return 8;
  else if (cityPeriod > 805)
    return 9;
  else if (cityPeriod > 750)
    return 10;
  else if (cityPeriod > 694)
    return 11;
  else if (cityPeriod > 639)
    return 12;
  else if (cityPeriod > 583)
    return 13;
  else if (cityPeriod > 528)
    return 14;
  else
    return 15;
}
