/****************************************************************************
 Template header file for Hierarchical Sate Machines AKA StateCharts
 02/08/12 adjsutments for use with the Events and Services Framework Gen2
 3/17/09  Fixed prototpyes to use Event_t
 ****************************************************************************/

#ifndef FindingCityNavSM_H
#define FindingCityNavSM_H


// typedefs for the states
// State definitions for use with the query function
typedef enum { STATE_ZERO, STATE_ONE, STATE_TWO } FindingCityNavState_t ;


// Public Function Prototypes

ES_Event RunFindingCityNavSM( ES_Event CurrentEvent );
void StartFindingCityNavSM ( ES_Event CurrentEvent );
FindingCityNavState_t QueryFindingCityNavSM ( void );

#endif /*FindingCityNavSM_H */

