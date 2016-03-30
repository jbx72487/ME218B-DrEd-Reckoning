/****************************************************************************
 Template header file for Hierarchical Sate Machines AKA StateCharts
 02/08/12 adjsutments for use with the Events and Services Framework Gen2
 3/17/09  Fixed prototpyes to use Event_t
 ****************************************************************************/

#ifndef AtCityShootSM_H
#define AtCityShootSM_H

#include "TeamColorSwitchHW.h"

// typedefs for the states
// State definitions for use with the query function
typedef enum { SHOOT_IDLE, SHOOT_TURNING, SHOOT_PREPARING, SHOOT_REFINDING_LINE, SHOOT_DONE } AtCityShootState_t ;


// Public Function Prototypes

ES_Event RunAtCityShootSM( ES_Event CurrentEvent );
void StartAtCityShootSM ( ES_Event CurrentEvent );
AtCityShootState_t QueryAtCityShootSM ( void );
void ResetBallCounter(void);
uint8_t GetBallCount(void);
bool GetCityShootStatus(uint8_t whichCity);
void SetShouldShootList(TeamColor_t color);
#endif /*AtCityShootSM_H */

