
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

#include "CAMMotorHW.h"


/*----------------------------- Module Defines ----------------------------*/

// #define CAM_MOTOR_TEST

#define ALL_BITS (0xff<<2)

#define CAM_MOTOR_PORT_BASE GPIO_PORTB_BASE
#define CAM_MOTOR_PIN BIT4HI

/*---------------------------- Module Functions ---------------------------*/


/*---------------------------- Module Variables ---------------------------*/


/*------------------------------ Module Code ------------------------------*/


void InitCAMMotorHW (void) {
		// Initialize Tiva to interact with shift register: 
	
	//enable the clock to Port B by setting bit 1 in register
	HWREG(SYSCTL_RCGCGPIO) |= BIT1HI;

	// wait for clock to be ready
	while ((HWREG(SYSCTL_PRGPIO) & SYSCTL_PRGPIO_R1) != BIT1HI)
	;

	// Connect to digital I/O port by writing to Digital Enable register
	HWREG(CAM_MOTOR_PORT_BASE+GPIO_O_DEN) |= CAM_MOTOR_PIN;
	// Set data direction  to output for these pins by writing to direction register
	HWREG(CAM_MOTOR_PORT_BASE+GPIO_O_DIR) |= CAM_MOTOR_PIN;
	// set initial value to low
	HWREG(CAM_MOTOR_PORT_BASE+(GPIO_O_DATA + ALL_BITS)) &= ~CAM_MOTOR_PIN;
}

void SetCAMMotorOn(void) {
	HWREG(CAM_MOTOR_PORT_BASE+(GPIO_O_DATA + ALL_BITS)) |= (CAM_MOTOR_PIN);
}

void SetCAMMotorOff(void) {
	HWREG(CAM_MOTOR_PORT_BASE+(GPIO_O_DATA + ALL_BITS)) &= ~(CAM_MOTOR_PIN);
}


#ifdef CAM_MOTOR_TEST
/* test harness for testing this module */
#include "termio.h"
#include "EnablePA25_PB23_PD7_PF0.h"
int main(void)
{
  TERMIO_Init();
	
	PortFunctionInit();
  puts("\r\n In test harness for CAMMotorHW module: spinning CAM motor (0)\r\n");
	puts("Press 'g' to turn on the motor, 'h' to turn it off \r\n");
  InitCAMMotorHW();

  while (true) {
	  if ( IsNewKeyReady() ) // new key waiting?
	  {
	  	char key = GetNewKey();
	  	if (key == 'g' || key == 'G') {
	  		SetCAMMotorOn();
	  	} else if (key == 'h' || key == 'H') {
	  		SetCAMMotorOff();
	  	}
	}
  }
  return 0;

}
#endif

