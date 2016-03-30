/****************************************************************************
 Template header file for Hierarchical Sate Machines AKA StateCharts
 02/08/12 adjsutments for use with the Events and Services Framework Gen2
 3/17/09  Fixed prototpyes to use Event_t
 ****************************************************************************/

#ifndef GameTimerSM_H
#define GameTimerSM_H


// typedefs for the states
// State definitions for use with the query function
typedef enum { GAME_TIME_IDLE, GAME_TIME_COUNTING } GameTimerState_t ;


// Public Function Prototypes

ES_Event RunGameTimerSM( ES_Event CurrentEvent );
void StartGameTimerSM ( ES_Event CurrentEvent );
GameTimerState_t QueryGameTimerSM ( void );

int16_t GetTimeLeft (void);
bool Check4GameOver (void);
#endif /*GameTimerSM_H */
