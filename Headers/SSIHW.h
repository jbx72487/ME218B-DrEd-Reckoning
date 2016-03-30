/****************************************************************************
 
  Header file for template Flat Sate Machine 
  based on the Gen2 Events and Services Framework

 ****************************************************************************/

#ifndef SSIHW_H
#define SSIHW_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */


// Public Function Prototypes

void InitSSIHW ( void );
void TransmitSSI( uint8_t byte2Write);
void SSIEOTIntHandler (void);
uint8_t ReadSSI(void);

#endif /* SSIHW_H */

