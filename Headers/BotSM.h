/****************************************************************************
 Template header file for Hierarchical Sate Machines AKA StateCharts
 02/08/12 adjsutments for use with the Events and Services Framework Gen2
 3/17/09  Fixed prototpyes to use Event_t
 ****************************************************************************/

#ifndef BotSM_H
#define BotSM_H

#include "TeamColorSwitchHW.h"

// typedefs for the states
// State definitions for use with the query function
typedef enum { BOT_IDLE, BOT_FINDING_CITY, BOT_AT_CITY } BotState_t ;

// Public Function Prototypes

ES_Event RunBotSM( ES_Event CurrentEvent );
void StartBotSM ( ES_Event CurrentEvent );
BotState_t QueryBotSM ( void );

TeamColor_t GetTeamColor( void );

#endif /*BotSM_H */

