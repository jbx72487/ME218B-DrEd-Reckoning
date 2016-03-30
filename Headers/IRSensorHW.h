/****************************************************************************
 
  Header file for template service 
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef IRSensorHW_H
#define IRSensorHW_H

#include "ES_Types.h"
#include "BotSM.h"

// Public Function Prototypes

typedef enum {NW, NE, SW, SE} WhichBeacon_t;


void InitIRSensorHW ( void );
void SetTargetBasedOnMyTeam(TeamColor_t color);
void IRIntHandler (void);
void IRTOIntHandler (void);
uint32_t GetIRPeriod( void);
uint32_t GetIRFreq( void);
void EnableIRInts( void );
void DisableIRInts (void);


#endif /* IRSensorHW_H */

