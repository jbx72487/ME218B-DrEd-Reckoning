/****************************************************************************
 Module
   Encoder.c

 Revision
   1.0.1

 Description
   This is a template file for implementing a simple service under the 
   Gen2 Events and Services Framework.

 Notes

****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/

#include <stdint.h>
#include <stdbool.h>
#include "ES_Configure.h"
#include "ES_Framework.h"

// headers to access GPIO subsystem
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_timer.h"
#include "inc/hw_nvic.h"


// headers to access the TivaWare Library
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"

#include "BITDEFS.H"

#include "CitySensorHW.h"
#include "MasterSM.h"
#include "EventPrinter.h"
/*----------------------------- Module Defines ----------------------------*/
// #define CITY_SENSOR_HW_TEST

#define CITY_COUNTER_THRESHOLD 2
#define ALL_BITS (0xff<<2)
#define BITS_PER_NIBBLE 4

#define SYSTEM_FREQ 40000000
#define ONE_US (SYSTEM_FREQ / 1000000)
#define MAX_PERIOD (ONE_US * 2000) // define MAX_PERIOD to be 2000 us
#define MAX_CITY_PERIOD (1361 * ONE_US)
#define MIN_CITY_PERIOD (472 * ONE_US)

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/

/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint32_t lastTime, thisTime, lastPeriod, thisPeriod;
uint8_t findCityCounter;
/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitCitySensorHW

****************************************************************************/
void InitCitySensorHW ( void )
{
  // use volatile to avoid over-optimizing
  volatile uint32_t Dummy;

  // port PD0, WT2CCP0, alternate function mux 7

  // Enable the clock to the timer (Wide Timer 2)
  HWREG(SYSCTL_RCGCWTIMER) |= SYSCTL_RCGCWTIMER_R2;
  // Enable the clock to the port for the encoder - Port D
  HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R3;
  // Disable both timers before configuring
  HWREG(WTIMER2_BASE+TIMER_O_CTL) &= (~TIMER_CTL_TAEN & ~TIMER_CTL_TBEN);
  // Set up both timers in 32bit wide, individual mode
  HWREG(WTIMER2_BASE+TIMER_O_CFG) = TIMER_CFG_16_BIT;

  //********* set up Timer A in input capture mode 

  // Initialize Timer A ILRs to 0xffff.ffff
  HWREG(WTIMER2_BASE+TIMER_O_TAILR) = 0xffffffff;
  // Set up timer A in capture mode for edge time and up-counting
  HWREG(WTIMER2_BASE+TIMER_O_TAMR) =
    (HWREG(WTIMER2_BASE+TIMER_O_TAMR) & ~TIMER_TAMR_TAAMS) |
      (TIMER_TAMR_TACDIR | TIMER_TAMR_TACMR | TIMER_TAMR_TAMR_CAP);
  // Set the event to rising edge
  HWREG(WTIMER2_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TAEVENT_M;

  // set the alternate function for port D bit 0
  // Tiva Datasheet p. 1351
  HWREG(GPIO_PORTD_BASE+GPIO_O_AFSEL) |= BIT0HI;
  // map bit 0’s alternate function to WT0CCP0
  HWREG(GPIO_PORTD_BASE+GPIO_O_PCTL) =
    (HWREG(GPIO_PORTD_BASE+GPIO_O_PCTL) & 0xfffffff0) + (7<< (0 * BITS_PER_NIBBLE));

  // enable pin for digital I/O
  HWREG(GPIO_PORTD_BASE+GPIO_O_DEN) |= BIT0HI;
  // make pin into an input
  HWREG(GPIO_PORTD_BASE+GPIO_O_DIR) &= BIT0LO;


  
	// enable NVIC interrupt (Wide Timer 2 Timer A interrupt number 98 -> EN3, bit 2)
  HWREG(NVIC_EN3) |= BIT2HI;
  // Tiva datasheet p. 104 onward: Interrupting object --> Interrupt # 
  // Tiva datasheet p. 134: NVIC interrupt # --> EN # 
  // Tiva datasheet p. 142 (really just math: interrupt # - number that's the beginning of the range for that EN#): interrupt # --> bit on EN#

  //************ set up Timer B as one-shot timer for MAX_PERIOD, to figure out when speed = 0

  // Set up timer B in one-shot mode & up-counting
  HWREG(WTIMER2_BASE+TIMER_O_TBMR) = 
  (HWREG(WTIMER2_BASE+TIMER_O_TBMR) & ~TIMER_TBMR_TBMR_M) |
  TIMER_TBMR_TBMR_1_SHOT | TIMER_TBMR_TBCDIR;
  // set timeout to MAX_PERIOD
  HWREG(WTIMER2_BASE+TIMER_O_TBILR) = MAX_PERIOD;
	

  // enable the Timer B in Wide Timer 0 interrupt in the NVC; it's interrupt #99
  HWREG(NVIC_EN3) |= BIT3HI;

  // make sure interrupts are enabled globally
  __enable_irq();

  // initialize period to max period, so that speed will be 0
  lastPeriod = MAX_PERIOD;
  thisPeriod = MAX_PERIOD;
  lastTime = 0;


	// enable timer A & B and enable them to stall while stopped by debugger
  HWREG(WTIMER2_BASE+TIMER_O_CTL) |= (TIMER_CTL_TAEN | TIMER_CTL_TASTALL);
  HWREG(WTIMER2_BASE+TIMER_O_CTL) |= (TIMER_CTL_TBEN | TIMER_CTL_TBSTALL);
	
	ClearFindCityCounter();
}

void EnableCitySensorInts(void) {
  // enable both interrupts

    // enable local capture interrupt for Timer A
  HWREG(WTIMER2_BASE+TIMER_O_IMR) |= TIMER_IMR_CAEIM;

  // enable a local timeout interrupt for Timer B
  HWREG(WTIMER2_BASE+TIMER_O_IMR) |= TIMER_IMR_TBTOIM;

}

void DisableCitySensorInts(void) {
  // enable both interrupts

    // disable local capture interrupt for Timer A
  HWREG(WTIMER2_BASE+TIMER_O_IMR) &= ~TIMER_IMR_CAEIM;

  // disable a local timeout interrupt for Timer B
  HWREG(WTIMER2_BASE+TIMER_O_IMR) &= ~TIMER_IMR_TBTOIM;

  // initialize period to max period, so that speed will be 0
  lastPeriod = MAX_PERIOD;
  thisPeriod = MAX_PERIOD;
  lastTime = 0;

}


void CityIntHandler (void) {
	// printf("Encoder interrupt\r\n");

  // to be executed at each interrupt from this pin
  // Write 1 to clear the interrupt
  HWREG(WTIMER2_BASE+TIMER_O_ICR) = TIMER_ICR_CAECINT;
  // thisTime = input captured time pulled from timer
  thisTime = HWREG(WTIMER2_BASE+TIMER_O_TAR);
  thisPeriod = thisTime - lastTime;

  //if thisPeriod < 1200 us and lastPeriod > 1200us, post CITY_FOUND
  if (( (thisPeriod <= MAX_CITY_PERIOD) && (thisPeriod >= MIN_CITY_PERIOD))) {
    findCityCounter++;
		if (findCityCounter == CITY_COUNTER_THRESHOLD) {
			ES_Event Event2Post;
			Event2Post.EventType = CITY_FOUND;
			// printf("this period when CITY_FOUND: %d\r\n",thisPeriod / ONE_US);
			// PostEventPrinter(Event2Post);
			PostMasterSM(Event2Post);
		}
  }	

  // restart timer for max period by setting timer = 0
  HWREG(WTIMER2_BASE+TIMER_O_TBV) = 0;
  // if one-shot timer had timed out, start timer again
	if (thisPeriod >= MAX_PERIOD) {
    thisPeriod = MAX_PERIOD;
		HWREG(WTIMER2_BASE+TIMER_O_CTL) |= (TIMER_CTL_TBEN | TIMER_CTL_TBSTALL);
  }
		
  lastTime = thisTime;
	lastPeriod = thisPeriod;

	// printf("Interrupt: New encoder edge. Speed = %d, RPM = %d.\r\n", period, GetRPM());
}

void CityTOIntHandler (void) {
  // to be executed each time timer reaches max period
	// write 1 to clear the interrupt
	HWREG(WTIMER2_BASE+TIMER_O_ICR) = TIMER_ICR_TBTOCINT;
  // set period to be max period
  thisPeriod = MAX_PERIOD;
	// printf("Interrupt: Max period reached. Speed = %d, RPM = %d.\r\n", period, GetRPM());
}

uint16_t GetCityPeriodUS( void) {
	return thisPeriod / ONE_US;	
}
/*
uint16_t GetCityPeriodUS( void) {
  static uint32_t lastGoodNum = 0; 
  if (thisPeriod != MAX_PERIOD)
    lastGoodNum = thisPeriod / ONE_US;  
  return lastGoodNum;
}
*/
uint16_t GetActualCityPeriodUS( void) {
	return thisPeriod / ONE_US;	
}

void ClearFindCityCounter(void) {
	findCityCounter = 0;
}




#ifdef CITY_SENSOR_HW_TEST
/* test harness for testing this module */
#include "termio.h"

int main(void)
{
	SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN
			| SYSCTL_XTAL_16MHZ);
	
  TERMIO_Init();
  puts("\r\n In test harness for CitySensorHW: printing CityPeriod (0)\r\n");

  InitCitySensorHW();
	EnableCitySensorInts();
  while (true) {
    printf("City Period: %d \r\n", GetCityPeriodUS());
    for (int i = 0; i < 1000000; i++) {}
  }
  
	return 0;

}
#endif

/***************************************************************************
 private functions
 ***************************************************************************/

/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

