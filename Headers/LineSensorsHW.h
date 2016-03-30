/****************************************************************************
 
  Header file for template service 
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef LineSensorsHW_H
#define LineSensorsHW_H

#include "ES_Types.h"

typedef enum {RIGHT_LINE, LEFT_LINE, CENTER_LINE, FRONT_LINE} WhichLineSensor_t; // for the three line sensors


// Public Function Prototypes
void InitLineSensorsHW ( void );
void UpdateAllLineReadings( void);
uint16_t GetLineReading(WhichLineSensor_t whichSensor);
bool Check4LineAligned(void);
bool Check4FrontOnBlackLine(void);
bool Check4CenterOnBlackLine(void);
void ClearLineSensorReadings(void);

#endif /* LineSensorsHW_H */

