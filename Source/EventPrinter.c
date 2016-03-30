/********************* Included headers *******************/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "EventPrinter.h"

#include "IRSensorHW.h"

#include <stdio.h>

/******************** Module Defines ***********************/


/******************** Private Functions ***********************/

/******************** Module Variables ***********************/
static uint8_t MyPriority;

/******************** Module Code ***********************/
bool InitEventPrinter(uint8_t Priority) {
	MyPriority = Priority;
		// printf("Event printer initialized.\r\n");
	return true;
}

bool PostEventPrinter(ES_Event ThisEvent) {
	if (ES_PostToService(MyPriority, ThisEvent) == true)
		return true;
	else
		return false;
}

// prints messages to the screen depending on which event is happening

ES_Event RunEventPrinter(ES_Event ThisEvent) {
	ES_Event ReturnVal;
	ReturnVal.EventType = ES_NO_EVENT;
	
	switch (ThisEvent.EventType) {
		case ES_TIMEOUT:
			switch (ThisEvent.EventParam) {
				case PAC_TIMER:
					printf("Timer expired: PAC_TIMER\r\n");
					break;			
				case GAME_TIMER:
					printf("Timer expired: GAME_TIMER\r\n");
					break;
				case TURN_TIMER:
					printf("Timer expired: TURN_TIMER\r\n");
					break;
				case IDLE_TIMER:
					printf("Timer expired: IDLE_TIMER\r\n");
					break;
				case LEAVING_CITY_TIMER:
					printf("Timer expired: LEAVING_CITY_TIMER\r\n");
					break;
			}
			break;
		case IR_FOUND:
/*			if (ThisEvent.EventParam == NW)
				printf("\nEventPrinter: IR Beacon: NW \r\n");
			else if (ThisEvent.EventParam == NE)
				printf("\nEventPrinter: IR Beacon: NE \r\n");
			if (ThisEvent.EventParam == SW)
				printf("\nEventPrinter: IR Beacon: SW \r\n");
			else if (ThisEvent.EventParam == SE)
				printf("\nEventPrinter: IR Beacon: SE \r\n");
			else
		*/
				printf("\nEventPrinter: IR_FOUND.\r\n");
			break;
		case  CITY_FOUND:
			printf("\nEventPrinter: CITY_FOUND \n\r");
			break;
		case  SSI_EOT:
			printf("\nEventPrinter: SSI_EOT: %#04x\t \n\r",ThisEvent.EventParam);
			break;
		case  SEND2PAC:
			printf("\nEventPrinter: SEND2PAC: %#04x\t \n\r",ThisEvent.EventParam);
			break;
		case  GAME_STARTED:
			printf("\nEventPrinter: GAME_STARTED \n\r");
			break;	
		case  CITY_COMPLETED:
			printf("\nEventPrinter: CITY_COMPLETED: %#04x\t \n\r", ThisEvent.EventParam);
			break;	
		case  PAC_MSG_READY:
			printf("\nEventPrinter: PAC_MSG_READY\n\r");
			break;	
		case  FRONT_BUMPER_HIT:
			printf("\nEventPrinter: FRONT_BUMPER_HIT \n\r");
			break;	
		case  CAM_SWITCH_PRESSED:
			printf("\nEventPrinter: CAM_SWITCH_PRESSED \n\r");
			break;	
		case  CENTER_LINE_FOUND:
			printf("\nEventPrinter: CENTER_LINE_FOUND \n\r");
			break;	
		case  FRONT_LINE_FOUND:
			printf("\nEventPrinter: FRONT_LINE_FOUND \n\r");
			break;	
		case  REAR_BUMPER_HIT:
			printf("\nEventPrinter: REAR_BUMPER_HIT \n\r");
			break;	
		case  LINE_THRESHOLD_PASSED:
			printf("\nEventPrinter: LINE_THRESHOLD_PASSED \n\r");
			break;	
		case  SHOOT_CITY:
			printf("\nEventPrinter: SHOOT_CITY: %d \n\r", ThisEvent.EventParam);
			break;	
		case  LINE_LOST:
			printf("\nEventPrinter: LINE_LOST \n\r");
			break;	

		case  GAME_OVER:
			printf("\nEventPrinter: GAME_OVER \n\r");
			break;	

	}

	return ReturnVal;
}
