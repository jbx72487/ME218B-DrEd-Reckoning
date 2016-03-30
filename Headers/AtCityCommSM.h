/****************************************************************************
 Template header file for Hierarchical Sate Machines AKA StateCharts
 02/08/12 adjsutments for use with the Events and Services Framework Gen2
 3/17/09  Fixed prototpyes to use Event_t
 ****************************************************************************/

#ifndef AtCityCommSM_H
#define AtCityCommSM_H


// typedefs for the states
// State definitions for use with the query function
typedef enum { AT_CITY_REQUESTING, AT_CITY_WAITING, AT_CITY_QUERYING, AT_CITY_DONE } AtCityCommState_t ;


// Public Function Prototypes

ES_Event RunAtCityCommSM( ES_Event CurrentEvent );
void StartAtCityCommSM ( ES_Event CurrentEvent );
AtCityCommState_t QueryAtCityCommSM ( void );

#endif /*AtCityCommSM_H */

