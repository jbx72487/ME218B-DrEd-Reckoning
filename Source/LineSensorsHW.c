/****************************************************************************
 Module
   LineSensorsHW.c

 Revision
   1.0.1

 Description
   Interface for line sensor hardware

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 01/16/12 09:58 jec      began conversion from TemplateFSM.c
 02/10/16		jbx		 using for 218B proj
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/


#include "ES_Configure.h"
#include "ES_Framework.h"
#include "LineSensorsHW.h"
#include "ADMulti.h"
#include "FindingCityNavSM.h"
#include "MasterSM.h"
#include "EventPrinter.h"

// the headers to access the GPIO subsystem
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"

#include "ES_Port.h"

// the headers to access the TivaWare Library
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"

/*----------------------------- Module Defines ----------------------------*/
// #define LINE_SENSORS_HW_TEST


#define NUM_ANALOG_INPUTS 4
#define INDUCTIVE_THRESHOLD 4200
#define CENTER_LINE_SENSOR_THRESHOLD 3500
#define FRONT_LINE_SENSOR_THRESHOLD 3520
#define OFFLINE_COUNTER_THRESHOLD 30

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service                                                               */

/*---------------------------- Module Variables ---------------------------*/
static bool lastCenterOnBlackLine, lastLineAligned, lastFrontOnBlackLine;
static uint16_t lineReadings[NUM_ANALOG_INPUTS];
static uint16_t offLineCounter;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitADService

 Parameters
     uint8_t : the priority of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and does any 
     other required initialization for this service
 Notes

 Author
     J. Edward Carryer, 01/16/12, 10:00
	 Sam, 01/09/16, 12:56
****************************************************************************/
void InitLineSensorsHW ( void )
{
	// initialize A/D pin HW for three line sensors
	ADC_MultiInit(NUM_ANALOG_INPUTS); // Initializes 3 AD channels: PE0, PE1, PE2
	lastCenterOnBlackLine = (GetLineReading(CENTER_LINE) > CENTER_LINE_SENSOR_THRESHOLD);
	lastFrontOnBlackLine = (GetLineReading(FRONT_LINE) > FRONT_LINE_SENSOR_THRESHOLD);  
	lastLineAligned = (GetLineReading(LEFT_LINE) + GetLineReading(RIGHT_LINE) > INDUCTIVE_THRESHOLD);
	
}


void UpdateAllLineReadings( void) {
  static uint32_t results[NUM_ANALOG_INPUTS];
  ADC_MultiRead(results);
  for (int i = 0; i < NUM_ANALOG_INPUTS; i++) {
  	lineReadings[i] = results[i];
  } 
}

uint16_t GetLineReading(WhichLineSensor_t whichSensor) {
	return lineReadings[whichSensor];
}

void ClearLineSensorReadings(void) {
	lastCenterOnBlackLine = false;
	lastLineAligned = false;
	offLineCounter = 0;
	// printf("Clearing line sensor readings\r\n");
}

bool Check4CenterOnBlackLine(void) // send CENTER_LINE_FOUND to MasterSM when getline(front) > 3000
{
  bool thisCenterOnBlackLine = (GetLineReading(CENTER_LINE) > CENTER_LINE_SENSOR_THRESHOLD);
	//printf("Line %d \n\r", GetLineReading(CENTER_LINE));
		// TODO printf("OPTICAL PRE: this: %d \t last: %d \r\n",thisOnBlackLine, lastCenterOnBlackLine);
  if (thisCenterOnBlackLine != lastCenterOnBlackLine) {
			ES_Event ThisEvent;
		if (thisCenterOnBlackLine == true) {
			offLineCounter = 0;
			ThisEvent.EventType = CENTER_LINE_FOUND;
			PostMasterSM(ThisEvent);
			// PostEventPrinter(ThisEvent);
			// printf("OPTICAL LINE FOUND\r\n");
		} else /* {
			ThisEvent.EventType = LINE_LOST;
			PostMasterSM(ThisEvent);
			// PostEventPrinter(ThisEvent);
			// printf("OPTICAL LINE LOST\r\n");
		} */
		lastCenterOnBlackLine = thisCenterOnBlackLine;
		return true;
	} else if (!thisCenterOnBlackLine && !lastCenterOnBlackLine) {
		offLineCounter++;
		if (offLineCounter >= OFFLINE_COUNTER_THRESHOLD) {
			ES_Event Event2Post;
			Event2Post.EventType = LINE_LOST;
			PostMasterSM(Event2Post);
		}
	}
	// printf("OPTICAL POST: this: %d \t last: %d \r\n",thisOnBlackLine, lastCenterOnBlackLine);
  return false;
}

bool Check4FrontOnBlackLine(void) // send CENTER_LINE_FOUND to MasterSM when getline(front) > 3000
{
  bool thisFrontOnBlackLine = (GetLineReading(FRONT_LINE) > FRONT_LINE_SENSOR_THRESHOLD);
	//printf("Line %d \n\r", GetLineReading(CENTER_LINE));
		// TODO printf("OPTICAL PRE: this: %d \t last: %d \r\n",thisOnBlackLine, lastCenterOnBlackLine);
  if (thisFrontOnBlackLine != lastFrontOnBlackLine) {
			ES_Event ThisEvent;
		if (thisFrontOnBlackLine == true) {
			ThisEvent.EventType = FRONT_LINE_FOUND;
			PostMasterSM(ThisEvent);
			// PostEventPrinter(ThisEvent);
			// printf("OPTICAL LINE FOUND\r\n");
		} else /* {
			ThisEvent.EventType = LINE_LOST;
			PostMasterSM(ThisEvent);
			// PostEventPrinter(ThisEvent);
			// printf("OPTICAL LINE LOST\r\n");
		} */
		lastFrontOnBlackLine = thisFrontOnBlackLine;
		return true;
	}
	// printf("OPTICAL POST: this: %d \t last: %d \r\n",thisOnBlackLine, lastCenterOnBlackLine);
  return false;
}


bool Check4LineAligned(void) // send LINE_THRESHOLD_PASSED to MasterSM when GetLine(left)+getline(right) are both >5000
{
  bool thisLineAligned = (GetLineReading(LEFT_LINE) + GetLineReading(RIGHT_LINE) > INDUCTIVE_THRESHOLD);
	// printf("SUM: %d\r\n", GetLineReading(LEFT_LINE) + GetLineReading(RIGHT_LINE));
	// TODO printf("INDUCTIVE PRE: this: %d \t last: %d \r\n",thisLineAligned, lastLineAligned);
  if (thisLineAligned != lastLineAligned) {
		ES_Event ThisEvent;
		if (thisLineAligned == true) {
			ThisEvent.EventType = LINE_THRESHOLD_PASSED;
			PostMasterSM(ThisEvent);
			// PostEventPrinter(ThisEvent); 
			// printf("INDUCTIVE LINE FOUND\r\n");
		} /* else {
			// ThisEvent.EventType = LINE_LOST;
			// PostMasterSM(ThisEvent);
			// PostEventPrinter(ThisEvent);
			// printf("INDUCTIVE LINE LOST\r\n");
		} */
		lastLineAligned = thisLineAligned;
		return true;
		// printf("INDUCTIVE POST: this: %d \t last: %d \r\n",thisLineAligned, lastLineAligned);
	}
  return false;

}

void ClearOffLineCounter( void) {
	offLineCounter = 0;
}


/***************************************************************************
 private functions
 ***************************************************************************/




/*------------------------------- Test Harness -------------------------------*/

#ifdef LINE_SENSORS_HW_TEST

#include "termio.h"
#define clrScrn() 	printf("\x1b[2J")
#define goHome()	printf("\x1b[1,1H")
#define clrLine()	printf("\x1b[K")


int main(void) {
	TERMIO_Init();
	clrScrn();
	
	// When doing testing, it is useful to announce just which program
	// is running.
	puts("\rStarting Test Harness for \r");
	printf("LineSensorsHW\r\n");
	printf("%s %s\n",__TIME__, __DATE__);
	printf("\n\r\n");

	printf("Initialize hardware, then continuously read analog input from all three line sensors");

	InitLineSensorsHW();

	while (true) {
		UpdateAllLineReadings();
		printf("Left: %d \t Right: %d \t Center: %d \t Front: %d \r\n", GetLineReading(LEFT_LINE), GetLineReading(RIGHT_LINE), GetLineReading(CENTER_LINE), GetLineReading(FRONT_LINE));
		for (int i = 0; i < 1000000; i++) {}
	}

	return 0;
}

#endif


/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

