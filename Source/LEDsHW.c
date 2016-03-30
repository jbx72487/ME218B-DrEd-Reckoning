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

#include "LEDsHW.h"


/*----------------------------- Module Defines ----------------------------*/

// #define LEDS_TEST

#define ALL_BITS (0xff<<2)

#define LED_PORT_BASE GPIO_PORTD_BASE
#define RED_LED_PIN BIT6HI
#define BLUE_LED_PIN BIT7HI
#define CAMPAIGN_LED_PIN BIT3HI
#define ALL_LEDs (RED_LED_PIN | BLUE_LED_PIN | CAMPAIGN_LED_PIN)

/*---------------------------- Module Functions ---------------------------*/


/*---------------------------- Module Variables ---------------------------*/


/*------------------------------ Module Code ------------------------------*/


void InitLEDsHW (void) {
		// Initialize Tiva to interact with shift register: 
	
	//enable the clock to Port D by setting bit 3 in register
	HWREG(SYSCTL_RCGCGPIO) |= BIT3HI;

	// wait for clock to be ready
	while ((HWREG(SYSCTL_PRGPIO) & SYSCTL_PRGPIO_R3) != BIT3HI)
	;

	// Connect to digital I/O port by writing to Digital Enable register
	HWREG(LED_PORT_BASE+GPIO_O_DEN) |= ALL_LEDs;
	// Set data direction  to output for these pins by writing to direction register
	HWREG(LED_PORT_BASE+GPIO_O_DIR) |= ALL_LEDs;
	// set initial value to low
	HWREG(LED_PORT_BASE+(GPIO_O_DATA + ALL_BITS)) &= ~ALL_LEDs;
}

void SetLEDOn(WhichLED_t led) {
	switch (led) {
		case RED_LED:
			HWREG(LED_PORT_BASE+(GPIO_O_DATA + ALL_BITS)) |= (RED_LED_PIN);
			break;
		case BLUE_LED:
			HWREG(LED_PORT_BASE+(GPIO_O_DATA + ALL_BITS)) |= (BLUE_LED_PIN);
			break;
		case CAMPAIGN_LED:
			HWREG(LED_PORT_BASE+(GPIO_O_DATA + ALL_BITS)) |= (CAMPAIGN_LED_PIN);
			break;
	}
}

void SetLEDOff(WhichLED_t led) {
	switch (led) {
		case RED_LED:
			HWREG(LED_PORT_BASE+(GPIO_O_DATA + ALL_BITS)) &= ~(RED_LED_PIN);
			break;
		case BLUE_LED:
			HWREG(LED_PORT_BASE+(GPIO_O_DATA + ALL_BITS)) &= ~(BLUE_LED_PIN);
			break;
		case CAMPAIGN_LED:
			HWREG(LED_PORT_BASE+(GPIO_O_DATA + ALL_BITS)) &= ~(CAMPAIGN_LED_PIN);
			break;
	}
}


#ifdef LEDS_TEST
/* test harness for testing this module */
#include "termio.h"
#include "EnablePA25_PB23_PD7_PF0.h"
int main(void)
{
  TERMIO_Init();
	
	PortFunctionInit();
  puts("\r\n In test harness for LEDsHW module: toggling b/w red & blue, campaign on & off (0)\r\n");

  InitLEDsHW();

  while(true) {
    // SetLEDOn(RED_LED);
    SetLEDOn(CAMPAIGN_LED);
    // SetLEDOff(BLUE_LED);

    for (int i = 0; i < 1000000; i++) {}
    	
    for (int i = 0; i < 1000000; i++) {}

    // SetLEDOff(RED_LED);
    // SetLEDOn(BLUE_LED);
    SetLEDOff(CAMPAIGN_LED);
    for (int i = 0; i < 1000000; i++) {}
  }
  return 0;

}
#endif

