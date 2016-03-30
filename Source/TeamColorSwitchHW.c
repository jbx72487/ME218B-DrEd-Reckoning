
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

#include "TeamColorSwitchHW.h"


/*----------------------------- Module Defines ----------------------------*/

// #define TEAM_COLOR_SWITCH_TEST

#define ALL_BITS (0xff<<2)

#define TEAM_COLOR_SWITCH_PORT_BASE GPIO_PORTD_BASE
#define TEAM_COLOR_SWITCH_PIN BIT1HI

/*---------------------------- Module Functions ---------------------------*/


/*---------------------------- Module Variables ---------------------------*/


/*------------------------------ Module Code ------------------------------*/


void InitTeamColorSwitchHW (void) {
	// Initialize Tiva to interact with shift register:

	//enable the clock to Port D by setting bit 3 in register
	HWREG(SYSCTL_RCGCGPIO) |= BIT3HI;

	// wait for clock to be ready
	while ((HWREG(SYSCTL_PRGPIO) & SYSCTL_PRGPIO_R3) != BIT3HI)
	;

	// Connect to digital I/O port by writing to Digital Enable register
	HWREG(TEAM_COLOR_SWITCH_PORT_BASE+GPIO_O_DEN) |= TEAM_COLOR_SWITCH_PIN;
	// Set data direction to input for these pins by writing to direction register
	HWREG(TEAM_COLOR_SWITCH_PORT_BASE+GPIO_O_DIR) &= ~TEAM_COLOR_SWITCH_PIN;

		// active low, so configure pins with pull-up resistors
	HWREG(TEAM_COLOR_SWITCH_PORT_BASE+GPIO_O_CR) |= TEAM_COLOR_SWITCH_PIN;
	HWREG(TEAM_COLOR_SWITCH_PORT_BASE+GPIO_O_PUR) |= TEAM_COLOR_SWITCH_PIN;

}

TeamColor_t ReadTeamColor( void) {
	uint8_t color = HWREG(TEAM_COLOR_SWITCH_PORT_BASE+(GPIO_O_DATA + ALL_BITS)) & TEAM_COLOR_SWITCH_PIN;
	if (color == 0)
		return RED;
	else
		return BLUE;
}

#ifdef TEAM_COLOR_SWITCH_TEST
/* test harness for testing this module */
#include "termio.h"

int main(void)
{
  TERMIO_Init();
  puts("\r\n In test harness for TeamColorSwitchHW module: reading switch status (0)\r\n");

  InitTeamColorSwitchHW();

  uint8_t color;

  while(true) {
    color = ReadTeamColor();
    if (color == RED)
    	printf("Team color: RED \r\n");
    else if (color == BLUE)
    	printf("Team color: BLUE \r\n");
    for (int i = 0; i < 1000000; i++) {}
  }
  return 0;
}

#endif

