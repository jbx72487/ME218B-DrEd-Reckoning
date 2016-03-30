// #define TEST

/*----------------------------- Include Files -----------------------------*/
/* include header files for the framework and this service
*/

#include <stdint.h>
#include <stdbool.h>

#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ES_DeferRecall.h"

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"	// Define PART_TM4C123GH6PM in project
#include "driverlib/gpio.h"
 
 #include "termio.h"
#include "BITDEFS.H"

#include "SwitchesHW.h"


/*----------------------------- Module Defines ----------------------------*/

// #define SWITCHES_TEST

#define ALL_BITS (0xff<<2)

#define SWITCHES_PORT_BASE2_6 GPIO_PORTE_BASE
#define SWITCHES_PORT_BASE0_4_8_10_CAM GPIO_PORTF_BASE

#define SWITCH_0_PIN BIT5HI
#define SWITCH_6_PIN BIT4HI

// #define SWITCH_2_PIN BIT0HI
#define SWITCH_4_PIN BIT4HI
// #define SWITCH_8_PIN BIT3HI
#define SWITCH_10_PIN BIT2HI
#define SWITCH_CAM_PIN BIT1HI

#define ALL_PORT_1_PINS (SWITCH_0_PIN | SWITCH_6_PIN)
// #define ALL_PORT_2_PINS (SWITCH_0_PIN | SWITCH_4_PIN | SWITCH_8_PIN | SWITCH_10_PIN | SWITCH_CAM_PIN)
#define ALL_PORT_2_PINS (SWITCH_4_PIN | SWITCH_10_PIN | SWITCH_CAM_PIN)

/*---------------------------- Module Functions ---------------------------*/


/*---------------------------- Module Variables ---------------------------*/


/*------------------------------ Module Code ------------------------------*/


void InitSwitchesHW (void) {
		// Initialize Tiva to interact with shift register: 
	
	//enable the clock to Port E by setting bit 4 in register
	HWREG(SYSCTL_RCGCGPIO) |= BIT4HI;
	//enable the clock to Port F by setting bit 5 in register
	HWREG(SYSCTL_RCGCGPIO) |= BIT5HI;

	// wait for clock to be ready
	while ((HWREG(SYSCTL_PRGPIO) & SYSCTL_PRGPIO_R5) != BIT5HI)
	;

	// Connect to digital I/O port by writing to Digital Enable register
	HWREG(SWITCHES_PORT_BASE2_6+GPIO_O_DEN) |= ALL_PORT_1_PINS;
	HWREG(SWITCHES_PORT_BASE0_4_8_10_CAM+GPIO_O_DEN) |= ALL_PORT_2_PINS;
	
	// Set data direction  to input for these pins by writing to direction register
	HWREG(SWITCHES_PORT_BASE2_6+GPIO_O_DIR) &= ~ALL_PORT_1_PINS;
	HWREG(SWITCHES_PORT_BASE0_4_8_10_CAM+GPIO_O_DIR) &= ~ALL_PORT_2_PINS;

	// active low, so configure pins with pull-up resistors
	HWREG(SWITCHES_PORT_BASE2_6+GPIO_O_CR) |= ALL_PORT_1_PINS;
	HWREG(SWITCHES_PORT_BASE0_4_8_10_CAM+GPIO_O_CR) |= ALL_PORT_2_PINS;
	
	HWREG(SWITCHES_PORT_BASE2_6+GPIO_O_PUR) |= ALL_PORT_1_PINS;
	HWREG(SWITCHES_PORT_BASE0_4_8_10_CAM+GPIO_O_PUR) |= ALL_PORT_2_PINS;

}

SwitchStatus_t ReadSwitch(uint8_t whichSwitch) {
	SwitchStatus_t status;
	uint8_t port1Val = HWREG(SWITCHES_PORT_BASE2_6+(GPIO_O_DATA + ALL_BITS));
	uint8_t port2Val = HWREG(SWITCHES_PORT_BASE0_4_8_10_CAM+(GPIO_O_DATA + ALL_BITS));
	// printf("Port 1: %#08x \t Port 2: %#08x\r\n", port1Val, port2Val);
	switch (whichSwitch) {
		case ((uint8_t) SWITCH_0):
			if ((port1Val & SWITCH_0_PIN) == 0)
				status = SWITCH_ON;
			else status = SWITCH_OFF;
			break;
		case ((uint8_t) SWITCH_2):
			/* if ((port1Val & SWITCH_2_PIN) == 0)
				status = SWITCH_ON;
			else status = SWITCH_OFF;
		*/ status = SWITCH_OFF;
			break;
		case ((uint8_t) SWITCH_4):
			if ((port2Val & SWITCH_4_PIN) == 0)
				status = SWITCH_ON;
			else status = SWITCH_OFF;
			break;
		case ((uint8_t) SWITCH_6):
			if ((port1Val & SWITCH_6_PIN) == 0)
				status = SWITCH_ON;
			else status = SWITCH_OFF;
			break;
		case ((uint8_t) SWITCH_8):
/*			if ((port2Val & SWITCH_8_PIN) == 0)
				status = SWITCH_ON;
			else status = SWITCH_OFF;
					*/ status = SWITCH_OFF;
			break;
		case ((uint8_t) SWITCH_10):
			if ((port2Val & SWITCH_10_PIN) == 0)
				status = SWITCH_ON;
			else status = SWITCH_OFF;
			break;
		case ((uint8_t) SWITCH_CAM):
			if ((port2Val & SWITCH_CAM_PIN) == 0)
				status = SWITCH_ON;
			else status = SWITCH_OFF;
			break;
	}
	return status;
}


#ifdef SWITCHES_TEST
/* test harness for testing this module */
#include "termio.h"
#include "EnablePA25_PB23_PD7_PF0.h"
int main(void)
{
  TERMIO_Init();
	
	PortFunctionInit();
  puts("\r\n In test harness for SwitchesHW module: reading status of all switches (0)\r\n");

  InitSwitchesHW();

  while(true) {
  	printf("0: %d\t2: %d\t4: %d\t6: %d\t8: %d\t10: %d\tCAM: %d\r\n",ReadSwitch(SWITCH_0),ReadSwitch(SWITCH_2),ReadSwitch(SWITCH_4),ReadSwitch(SWITCH_6),ReadSwitch(SWITCH_8),ReadSwitch(SWITCH_10),ReadSwitch(SWITCH_CAM));
  }
  
  return 0;

}
#endif

