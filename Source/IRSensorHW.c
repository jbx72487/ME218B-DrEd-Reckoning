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

#include "IRSensorHW.h"
#include "MasterSM.h"
#include "EventPrinter.h"

/*---------------------------- Module Defines ---------------------------*/

// #define IR_SENSOR_HW_TEST

#define ALL_BITS (0xff<<2)
#define BITS_PER_NIBBLE 4

#define SYSTEM_FREQ 40000000
#define ONE_US (SYSTEM_FREQ / 1000000)
#define MAX_PERIOD (ONE_US * 2000) // define MAX_PERIOD to be 2000 us

// #define MAX_IR_PERIOD (900 * ONE_US)
#define SE_NW_FREQ_CUTOFF 1350
#define NW_NE_FREQ_CUTOFF 1575
#define NE_SW_FREQ_CUTOFF 1825
#define MIN_IR_PERIOD (480 * ONE_US)
#define ONE_SEC 40000000

#define BLUE_PERIOD_MIN (SYSTEM_FREQ/1300)
#define BLUE_PERIOD_MAX (SYSTEM_FREQ/1200)
#define RED_PERIOD_MIN (SYSTEM_FREQ/1500)
#define RED_PERIOD_MAX (SYSTEM_FREQ/1400)

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/
uint8_t ConvertToBeaconFreq(uint32_t period);


/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint32_t lastTime, thisTime, thisPeriod, targetPeriodMin, targetPeriodMax;
static bool lastPeriodInRange, thisPeriodInRange;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitIRSensorHW

****************************************************************************/
void InitIRSensorHW ( void )
{
  // use volatile to avoid over-optimizing
  volatile uint32_t Dummy;

  // port PD2, WT3CCP0, alternate function mux 7

  // Enable the clock to the timer (Wide Timer 3)
  HWREG(SYSCTL_RCGCWTIMER) |= SYSCTL_RCGCWTIMER_R3;
  // Enable the clock to the port for the encoder - Port D
  HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R3;
  // Disable both timers before configuring
  HWREG(WTIMER3_BASE+TIMER_O_CTL) &= (~TIMER_CTL_TAEN & ~TIMER_CTL_TBEN);
  // Set up both timers in 32bit wide, individual mode
  HWREG(WTIMER3_BASE+TIMER_O_CFG) = TIMER_CFG_16_BIT;

  //********* set up Timer A in input capture mode 

  // Initialize Timer A ILRs to 0xffff.ffff
  HWREG(WTIMER3_BASE+TIMER_O_TAILR) = 0xffffffff;
  // Set up timer A in capture mode for edge time and up-counting
  HWREG(WTIMER3_BASE+TIMER_O_TAMR) =
    (HWREG(WTIMER3_BASE+TIMER_O_TAMR) & ~TIMER_TAMR_TAAMS) |
      (TIMER_TAMR_TACDIR | TIMER_TAMR_TACMR | TIMER_TAMR_TAMR_CAP);
  // Set the event to rising edge
  HWREG(WTIMER3_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TAEVENT_M;

  // set the alternate function for port D bit 2
  // Tiva Datasheet p. 1351
  HWREG(GPIO_PORTD_BASE+GPIO_O_AFSEL) |= BIT2HI;
  // map bit 0’s alternate function to WT2CCP0
  HWREG(GPIO_PORTD_BASE+GPIO_O_PCTL) =
    (HWREG(GPIO_PORTD_BASE+GPIO_O_PCTL) & 0xfffff0ff) + (7<< (2 * BITS_PER_NIBBLE));

  // enable pin for digital I/O
  HWREG(GPIO_PORTD_BASE+GPIO_O_DEN) |= BIT2HI;
  // make pin into an input
  HWREG(GPIO_PORTD_BASE+GPIO_O_DIR) &= BIT2LO;

  // enable local capture interrupt
  HWREG(WTIMER3_BASE+TIMER_O_IMR) |= TIMER_IMR_CAEIM;
  
  // Tiva datasheet p. 104 onward: Interrupting object --> Interrupt # 
  // Tiva datasheet p. 134: NVIC interrupt # --> EN # 
  // Tiva datasheet p. 142 (really just math: interrupt # - number that's the beginning of the range for that EN#): interrupt # --> bit on EN#

  //************ set up Timer B as one-shot timer for MAX_PERIOD, to figure out when speed = 0

  // Set up timer B in one-shot mode & up-counting
  HWREG(WTIMER3_BASE+TIMER_O_TBMR) = 
  (HWREG(WTIMER3_BASE+TIMER_O_TBMR) & ~TIMER_TBMR_TBMR_M) |
  TIMER_TBMR_TBMR_1_SHOT | TIMER_TBMR_TBCDIR;
  // set timeout to MAX_PERIOD
  HWREG(WTIMER3_BASE+TIMER_O_TBILR) = MAX_PERIOD;
	
  // enable a local timeout interrupt
  HWREG(WTIMER3_BASE+TIMER_O_IMR) |= TIMER_IMR_TBTOIM;
	

  // make sure interrupts are enabled globally
  __enable_irq();

  // initialize period to max period, so that speed will be 0
  thisPeriod = MAX_PERIOD;
	lastPeriodInRange = false;
	thisPeriodInRange = false;
	lastTime = 0;

	// enable timer A & B and enable them to stall while stopped by debugger
  HWREG(WTIMER3_BASE+TIMER_O_CTL) |= (TIMER_CTL_TAEN | TIMER_CTL_TASTALL);
  HWREG(WTIMER3_BASE+TIMER_O_CTL) |= (TIMER_CTL_TBEN | TIMER_CTL_TBSTALL);
	
	targetPeriodMin = BLUE_PERIOD_MIN;
	targetPeriodMax = BLUE_PERIOD_MAX;
}
void SetTargetBasedOnMyTeam(TeamColor_t color) {
	// sets the IR target period to be the opposite team’s attack ad bucket
	if (color == RED) {
		targetPeriodMin = BLUE_PERIOD_MIN;
		targetPeriodMax = BLUE_PERIOD_MAX;
    printf("Shooting target color: BLUE\r\n");		
	} else {
		targetPeriodMin = RED_PERIOD_MIN;
		targetPeriodMax = RED_PERIOD_MAX;
    printf("Shooting target: BLUE\r\n");	}
}

void IRIntHandler (void) {
	// printf("Encoder interrupt\r\n");

  // to be executed at each interrupt from this pin
  // Write 1 to clear the interrupt
  HWREG(WTIMER3_BASE+TIMER_O_ICR) = TIMER_ICR_CAECINT;
  // thisTime = input captured time pulled from timer
  thisTime = HWREG(WTIMER3_BASE+TIMER_O_TAR);
  thisPeriod = thisTime - lastTime;

  //if thisPeriod is a valid IR beacon and lastPeriod wasn't, post IR_FOUND
  if ((thisPeriod <= targetPeriodMax) && (thisPeriod >= targetPeriodMin))
		thisPeriodInRange = true;
	else
		thisPeriodInRange = false;
	if (thisPeriodInRange && !lastPeriodInRange) {
    ES_Event Event2Post;
    Event2Post.EventType = IR_FOUND;
		// printf("CORRECT IR FREQ FOUND!.\r\n");
//     Event2Post.EventParam = ConvertToBeaconFreq(thisPeriod);
		PostMasterSM(Event2Post);
		// PostEventPrinter(Event2Post);

  }	
	

  // restart timer for max period by setting timer = 0
  HWREG(WTIMER3_BASE+TIMER_O_TBV) = 0;
  // if one-shot timer had timed out, start timer again
	if (thisPeriod >= MAX_PERIOD) {
    thisPeriod = MAX_PERIOD;
		HWREG(WTIMER3_BASE+TIMER_O_CTL) |= (TIMER_CTL_TBEN | TIMER_CTL_TBSTALL);
  }
	
  lastTime = thisTime;
	lastPeriodInRange = thisPeriodInRange;

	// printf("Interrupt: New encoder edge. Speed = %d, RPM = %d.\r\n", period, GetRPM());
}

void IRTOIntHandler (void) {
  // to be executed each time timer reaches max period
	// write 1 to clear the interrupt
	HWREG(WTIMER3_BASE+TIMER_O_ICR) = TIMER_ICR_TBTOCINT;
  // set period to be max period
  thisPeriod = MAX_PERIOD;
	// printf("Interrupt: Max period reached. Speed = %d, RPM = %d.\r\n", period, GetRPM());
}

void EnableIRInts( void ) {
  // enable both interrupts for IR

    // enable NVIC interrupt (Wide Timer 2 Timer A interrupt number 100 -> EN3, bit 4)
  HWREG(NVIC_EN3) |= BIT4HI;

  // enable the Timer B in Wide Timer 0 interrupt in the NVC; it's interrupt #101
  HWREG(NVIC_EN3) |= BIT5HI;

}

void DisableIRInts (void) {
  // disable both interrupts for IR

    // enable NVIC interrupt (Wide Timer 2 Timer A interrupt number 100 -> EN3, bit 4)
  HWREG(NVIC_EN3) &= ~BIT4HI;

  // enable the Timer B in Wide Timer 0 interrupt in the NVC; it's interrupt #101
  HWREG(NVIC_EN3) &= ~BIT5HI;


} 

uint32_t GetIRPeriod( void) {
  return thisPeriod;
}


uint32_t GetIRFreq( void) {
  return ONE_SEC / thisPeriod;
}



#ifdef IR_SENSOR_HW_TEST
/* test harness for testing this module */
#include "termio.h"

int main(void)
{
	
	SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN
			| SYSCTL_XTAL_16MHZ);
	
  TERMIO_Init();
  puts("\r\n In test harness for IRSensorHW: printing IRPeriod (0)\r\n");

  InitIRSensorHW();

  EnableIRInts();

  SetTargetBasedOnMyTeam(BLUE);

  while (true) {
    printf("IR Period: %d \t Frequency: %d \t\r\n", GetIRPeriod(), GetIRFreq());
    for (int i = 0; i < 1000000; i++) {}
  }
  
	return 0;

}
#endif

/***************************************************************************
 private functions
 ***************************************************************************/

/*
uint8_t ConvertToBeaconFreq(uint32_t period) {
  uint32_t freq = SYSTEM_FREQ / period;
  if (freq < SE_NW_FREQ_CUTOFF)
    return NW;
  else if (freq < NW_NE_FREQ_CUTOFF)
    return NE;
  else if (freq < NE_SW_FREQ_CUTOFF)
    return NW;
  else
    return SW;
}
*/


/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

