/****************************************************************************
 Template header file for Hierarchical Sate Machines AKA StateCharts
 02/08/12 adjsutments for use with the Events and Services Framework Gen2
 3/17/09  Fixed prototpyes to use Event_t
 ****************************************************************************/

#ifndef PACCommSM_H
#define PACCommSM_H


// typedefs for the states
// State definitions for use with the query function
typedef enum { PAC_READY, PAC_SENDING, PAC_WAITING } PACCommState_t ;


// Public Function Prototypes

ES_Event RunPACCommSM( ES_Event CurrentEvent );
void StartPACCommSM ( ES_Event CurrentEvent );
PACCommState_t QueryPACCommSM ( void );
uint32_t GetPACMsg(void);

#endif /*PACCommSM_H */

