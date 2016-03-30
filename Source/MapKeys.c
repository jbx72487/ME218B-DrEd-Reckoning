/****************************************************************************
 Module
   MapKeys.c

 Revision
   1.0.1

 Description
   This service maps keystrokes to events 

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 02/06/14 14:44 jec      tweaked to be a more generic key-mapper
 02/07/12 00:00 jec      converted to service for use with E&S Gen2
 02/20/07 21:37 jec      converted to use enumerated type for events
 02/21/05 15:38 jec      Began coding
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
#include <stdio.h>
#include <ctype.h>
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "MapKeys.h"
#include "MasterSM.h"
#include "PACCommSM.h"
#include "EventPrinter.h"
#include "GameTimerSM.h"
#include "BotSM.h"
#include "AtCityCommSM.h"
#include "AtCityShootSM.h"
#include "FindingCityNavSM.h"

/*----------------------------- Module Defines ----------------------------*/

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/
void ReportAllStates(void);


/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;


/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitMapKeys

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and does any 
     other required initialization for this service
 Notes

 Author
     J. Edward Carryer, 02/07/12, 00:04
****************************************************************************/
bool InitMapKeys ( uint8_t Priority )
{
  MyPriority = Priority;
		// printf("Map Keys initialized.\r\n");

  return true;
}

/****************************************************************************
 Function
     PostMapKeys

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
bool PostMapKeys( ES_Event ThisEvent )
{
  return ES_PostToService( MyPriority, ThisEvent);
}


/****************************************************************************
 Function
    RunMapKeys

 Parameters
   ES_Event : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   maps keys to Events for HierMuWave Example
 Notes
   
 Author
   J. Edward Carryer, 02/07/12, 00:08
****************************************************************************/
ES_Event RunMapKeys( ES_Event ThisEvent )
{
  ES_Event ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors

    if ( ThisEvent.EventType == ES_NEW_KEY) // there was a key pressed
    {
			printf("Key entered: \t %c\r\n",toupper(ThisEvent.EventParam));
        switch ( toupper(ThisEvent.EventParam))
        {
          // this sample is just a dummy so it posts a ES_NO_EVENT
            case '0':
              ThisEvent.EventType = ES_NO_EVENT; 
              break;
            case 'A':
              ThisEvent.EventType = GAME_STARTED;
              break;
            case 'B':
              ThisEvent.EventType = CITY_FOUND;
              break;
            case 'C':
              ThisEvent.EventType = IR_FOUND;
              break;
            case 'D':
              ThisEvent.EventType = SHOOT_CITY;
							ThisEvent.EventParam = 8;
              break;
            case 'S':
              ThisEvent.EventType = SEND2PAC;
							ThisEvent.EventParam = (0xB0 | 5);
              break;
            case 'F':
              ThisEvent.EventType = CITY_COMPLETED;
              break;
            case 'G':
              ThisEvent.EventType = FRONT_BUMPER_HIT;
              break;
            case 'H':
              ThisEvent.EventType = ES_TIMEOUT;
              ThisEvent.EventParam = TURN_TIMER;
						break;
            case 'Z':
              ThisEvent.EventType = GAME_OVER;
              break;
            case '?':
              ReportAllStates();
              break;
						default:
							ReportAllStates();
							ThisEvent.EventType = ES_NO_EVENT; 
              break;
        }
        PostEventPrinter(ThisEvent);
        PostMasterSM(ThisEvent);
    }
    
  return ReturnEvent;
}


void ReportAllStates() {
  printf("GameTimerSM: ");
  switch (QueryGameTimerSM()) {
    case GAME_TIME_IDLE:
      printf("GAME_TIME_IDLE\r\n");
      break;
    case GAME_TIME_COUNTING:
      printf("GAME_TIME_COUNTING\r\n");
      break;
  }

  printf("PACCommSM: ");
  switch(QueryPACCommSM()) {
    case PAC_READY:
      printf("PAC_READY\r\n");
      break;
    case PAC_SENDING:
      printf("PAC_SENDING\r\n");
      break;
    case PAC_WAITING:
      printf("PAC_WAITING2SEND\r\n");
      break;
  }
  printf("BotSM: ");
  switch (QueryBotSM()) {
    case BOT_IDLE:
      printf("BOT_IDLE\r\n");
      break;
    case BOT_FINDING_CITY:
      printf("BOT_FINDING_CITY\r\n");
      break;
    case BOT_AT_CITY:
      printf("BOT_AT_CITY\r\n");
      break;
  }
  printf("FindingCityNavSM: ");
  switch (QueryFindingCityNavSM()) {
    case Looking4LineFor:
      printf("Looking4LineFor\r\n");
      break;
    case Looking4LineRev:
      printf("Looking4LineRev\r\n");
      break;
    case RotatingOnLine:
      printf("RotatingOnLine\r\n");
      break;
    case FollowingLine:
      printf("FollowingLine\r\n");
      break;
  }

  printf("AtCityComm: ");
  switch(QueryAtCityCommSM()) {
    case AT_CITY_REQUESTING:
      printf("AT_CITY_REQUESTING\r\n");
      break;
    case AT_CITY_WAITING:
      printf("AT_CITY_WAITING\r\n");
      break;
    case AT_CITY_QUERYING:
      printf("AT_CITY_QUERYING\r\n");
      break;
    case AT_CITY_DONE:
      printf("AT_CITY_DONE\r\n");
      break;
		
  }

	printf("AtCityShootSM: ");
  switch(QueryAtCityShootSM()) {
    case SHOOT_IDLE:
      printf("SHOOT_IDLE\r\n");
      break;
    case SHOOT_TURNING:
      printf("SHOOT_TURNING\r\n");
      break;
    case SHOOT_PREPARING:
      printf("SHOOT_PREPARING\r\n");
      break;
    case SHOOT_REFINDING_LINE:
      printf("SHOOT_REFINDING_LINE\r\n");
      break;
    case SHOOT_DONE:
      printf("SHOOT_DONE\r\n");
      break;
  }
}
