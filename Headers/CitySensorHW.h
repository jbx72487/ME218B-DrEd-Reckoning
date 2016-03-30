/****************************************************************************
 
  Header file for template service 
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef CitySensorHW_H
#define CitySensorHW_H

#include "ES_Types.h"

// Public Function Prototypes

void InitCitySensorHW ( void );
void CityIntHandler (void);
void CityTOIntHandler (void);
uint16_t GetCityPeriodUS( void);
void EnableCitySensorInts(void);
void DisableCitySensorInts(void);
void ClearFindCityCounter(void);
uint16_t GetActualCityPeriodUS( void);
#endif /* CitySensorHW_H */

