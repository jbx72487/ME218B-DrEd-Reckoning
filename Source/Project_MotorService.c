/****************************************************************************
 Module
   MotorService.c

 Revision
   1.0.0

 Description
	drives the DC motor for project

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 02/02/16 	 dxchen   
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
// GPIO headers
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "driverlib/gpio.h"
#include "inc/hw_timer.h"
#include "inc/hw_nvic.h"

// Headers to access the TivaWare Library
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include "BITDEFS.H"

#include "Project_MotorService.h"
#include "Project_PWMlab.h"
#include "LineSensorsHW.h"

/*----------------------------- Module Defines ----------------------------*/
#define ALL_BITS (0xff<<2)
#define CLOCK_SPEED 40000000
#define ZERO_RPM_TIMEOUT CLOCK_SPEED/10 // used to be /10
#define RPMTIME 100 // Print every 100ms = 10HZ
#define TICKS_PER_REV 30 // was:3020 // 512 * 5.9 gearbox - Not accurate for our bot yet
#define LINE_FOLLOW_RPM 72
#define LINE_SENSOR_DIFF 1800
#define KP_CONSTANT .035
#define LINE_PROXIMITY 3000
#define COMBINED_LINE 5000
#define SLOWDOWN_MAX 30.0
/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/
void InitEncoderInterrupt(void);
void InitControllerInterrupt(void);
void PrintRPM(void);
uint32_t GetRPM0(void);
uint32_t GetRPM1(void);
void SetMotorDutyCycles(uint8_t DutyCycleLeft, uint8_t DutyCycleRight, Direction_t Direction);
void subroutine(void);
void EnableLineFollowInts(void);
void DisableLineFollowInts(void);

/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;
static uint32_t Period0;
static uint32_t Period1;
static uint32_t LastCapture0;
static uint32_t LastCapture1; 
static uint8_t EdgeFlag = 0;
static uint32_t TargetRPM_L = 0;
static uint32_t TargetRPM_R = 0;

											
/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitMotorService

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and does any 
     other required initialization for this service
 Notes
****************************************************************************/
bool InitMotorService ( uint8_t Priority )
{
  ES_Event ThisEvent;

  MyPriority = Priority;
	// Initialize Ports
	char Dummy;
	HWREG(SYSCTL_RCGCGPIO) |= (SYSCTL_RCGCGPIO_R1); // Port B
	// Kill some time
	Dummy=HWREG(SYSCTL_RCGCGPIO);
	
  // PB6 and PB7 are the L and R PWM pins, so they should be hooked up to 1A and 1B of the L293
	// (Enable Lines have been hardwired HI) -PB0 and PB1 are the L and R Enable Lines-
  // PB2 and PB3 are the L and R B Drive lines

  // These ports are configgeed in the InitPWM, no need to do it here.
	
	// enable PWM
	PWMInit(2500); // Enables PWM at 2500HZ
	SetMotorDutyCycles(0,0,FORWARD);
	
	InitLineSensorsHW();
  
  // Init the Encoder
  InitEncoderInterrupt();

	//subroutine();
	InitControllerInterrupt();
	DisableLineFollowInts();
	
	//EnableLineFollowInts();
  // Start RPM Timer (for printing)
  ES_Timer_InitTimer(RPMTimer, RPMTIME);
	
		printf("Motor service initialized.\r\n");
  // post the initial transition event
  ThisEvent.EventType = ES_INIT;
  if (ES_PostToService( MyPriority, ThisEvent) == true)
  {
      return true;
  }else
  {
      return false;
  }
}

/****************************************************************************
 Function
     PostMotorService

 Parameters
     EF_Event ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise

 Description
     Posts an event to this state machine's queue
 Notes
****************************************************************************/

bool PostMotorService( ES_Event ThisEvent )
{
  return ES_PostToService( MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunMotorService

 Parameters
   ES_Event : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   add your description here
 Notes
   
 Author
   J. Edward Carryer, 01/15/12, 15:23
****************************************************************************/

ES_Event RunMotorService( ES_Event ThisEvent )
{
  ES_Event ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
	// printf("Time Interval %d \n ", Period);
  // if(EdgeFlag){SetLED(CalculateLED(Period));}; ///////////////////////// Sam commented out
  // setDuty(GetADTime()); // GetADTime() disabed to give PI control
	EdgeFlag = 0;
  if (ThisEvent.EventType == ES_TIMEOUT){
			PrintRPM();
			ES_Timer_InitTimer(RPMTimer, RPMTIME);
  }
  return ReturnEvent;
}

/***************************************************************************
 private functions
 ***************************************************************************/

void InitEncoderInterrupt(void)
{ // C6 is working, C4 is not
  // Enables the Interrupt using the lecture Interrupt Code Sample. Detect on Port C4 for left, C6 for right
  // Start by enabling the clock to the timer (Wide Timer 0) Left, WT1 Right
  HWREG(SYSCTL_RCGCWTIMER) |= SYSCTL_RCGCWTIMER_R0;
  HWREG(SYSCTL_RCGCWTIMER) |= SYSCTL_RCGCWTIMER_R1;
  // Enable the clock to Port C
  // Since we added this port C clock init, we can immediately start configuring the timer, no need for further delay.
  HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R2;
  // Disable Timer A and Timer B before configuring
  HWREG(WTIMER0_BASE+TIMER_O_CTL) &= (~TIMER_CTL_TAEN & ~TIMER_CTL_TBEN);
  HWREG(WTIMER1_BASE+TIMER_O_CTL) &= (~TIMER_CTL_TAEN & ~TIMER_CTL_TBEN);
  // Set it up in 32bit wide (Individ, not concat) mode
  HWREG(WTIMER0_BASE+TIMER_O_CFG) = TIMER_CFG_16_BIT;
  HWREG(WTIMER1_BASE+TIMER_O_CFG) = TIMER_CFG_16_BIT;
  // Use the full 32bit count, so initialize Interval Load register to max
  HWREG(WTIMER0_BASE+TIMER_O_TAILR) = 0xffffffff;
  HWREG(WTIMER1_BASE+TIMER_O_TAILR) = 0xffffffff;
  // Set up Timer A in capture mode (TAMR =3, TAAMS = 0),
  // for edge time (TACMR = 1) and up-ounting (TACDIR=1)
  HWREG(WTIMER0_BASE+TIMER_O_TAMR) = (HWREG(WTIMER0_BASE+TIMER_O_TAMR) & ~TIMER_TAMR_TAAMS) | (TIMER_TAMR_TACDIR | TIMER_TAMR_TACMR | TIMER_TAMR_TAMR_CAP);
  HWREG(WTIMER1_BASE+TIMER_O_TAMR) = (HWREG(WTIMER1_BASE+TIMER_O_TAMR) & ~TIMER_TAMR_TAAMS) | (TIMER_TAMR_TACDIR | TIMER_TAMR_TACMR | TIMER_TAMR_TAMR_CAP);
  // Set up Timer B as one shot and counting up
  HWREG(WTIMER0_BASE+TIMER_O_TBMR) = (HWREG(WTIMER0_BASE+TIMER_O_TBMR) & ~TIMER_TBMR_TBMR_M) | (TIMER_TAMR_TACDIR | TIMER_TBMR_TBMR_1_SHOT);
  HWREG(WTIMER1_BASE+TIMER_O_TBMR) = (HWREG(WTIMER1_BASE+TIMER_O_TBMR) & ~TIMER_TBMR_TBMR_M) | (TIMER_TAMR_TACDIR | TIMER_TBMR_TBMR_1_SHOT);
  // To set the event to rising edge, we need to modify the TAEVENT bits in 
  // GPTMCTL. Rising Edge = 00, so we clear the TAEVENT bits
  HWREG(WTIMER0_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TAEVENT_M;
  HWREG(WTIMER1_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TAEVENT_M;
  // Now set up the port to do the capture (clock was enabled earlier)
  // Start by setting the alternate function for Port C bit 4 (WT0CCP0)
  HWREG(GPIO_PORTC_BASE+GPIO_O_AFSEL) |= BIT4HI | BIT6HI;
  // Then, map bit 4 alternate function to WT0CCP0
  // 7 is the mux value to select WT0CCP0, 16 to shift it over to the right 
  // nibble for bit 4 (4 bits/nibble * 4 bits)
  HWREG(GPIO_PORTC_BASE+GPIO_O_PCTL) = (HWREG(GPIO_PORTC_BASE+GPIO_O_PCTL) & 0xfff0ffff) + (7<<16);
  HWREG(GPIO_PORTC_BASE+GPIO_O_PCTL) = (HWREG(GPIO_PORTC_BASE+GPIO_O_PCTL) & 0xfff0ffff) + (7<<(6*4));
  // Enable pin on Port C for digital IO
  HWREG(GPIO_PORTC_BASE+GPIO_O_DEN) |= BIT4HI | BIT6HI;
  // Make pin 4 on port C into an Input
  HWREG(GPIO_PORTC_BASE+GPIO_O_DIR) &= BIT4LO | BIT6LO;
  // Back to the timer to enable a local capture interrupt
  HWREG(WTIMER0_BASE+TIMER_O_IMR) |= TIMER_IMR_CAEIM;
  HWREG(WTIMER1_BASE+TIMER_O_IMR) |= TIMER_IMR_CAEIM;
  // Enable the Timer A in Wide Timer 0 interrupt in the NVIC, it is interrupt 94 so it 
  // appears in EN2 at bit 30, Timer B is Bit 31
  HWREG(NVIC_EN2) |= BIT30HI;
  HWREG(NVIC_EN2) |= BIT31HI;
  HWREG(NVIC_EN3) |= BIT0HI;
  HWREG(NVIC_EN3) |= BIT1HI;
  
  // Configure Timer B timeout for 0 RPM
  HWREG(WTIMER0_BASE+TIMER_O_TBILR) = ZERO_RPM_TIMEOUT;
  HWREG(WTIMER1_BASE+TIMER_O_TBILR) = ZERO_RPM_TIMEOUT;
  
  //Enable the timeout interrupt for Timer B
  HWREG(WTIMER0_BASE+TIMER_O_IMR) |= TIMER_IMR_TBTOIM;
  HWREG(WTIMER1_BASE+TIMER_O_IMR) |= TIMER_IMR_TBTOIM;

  // Enable interrupts globablly
  __enable_irq();
  
  // Kick off the timers and enable them to stall when stopped by the debugger
  HWREG(WTIMER0_BASE+TIMER_O_CTL) |= (TIMER_CTL_TAEN | TIMER_CTL_TASTALL | TIMER_CTL_TBEN | TIMER_CTL_TBSTALL);
  HWREG(WTIMER1_BASE+TIMER_O_CTL) |= (TIMER_CTL_TAEN | TIMER_CTL_TASTALL | TIMER_CTL_TBEN | TIMER_CTL_TBSTALL);
}

// Interrupt Response Function for Timer A WT0

void InputCaptureResponse0(void)
{
  uint32_t ThisCapture0;
  // Clear the source of the interrupt, the input capture event
  HWREG(WTIMER0_BASE+TIMER_O_ICR) = TIMER_ICR_CAECINT;
  // Grab the captured time and calculate the period
  ThisCapture0 = HWREG(WTIMER0_BASE+TIMER_O_TAR);
  Period0 = ThisCapture0 - LastCapture0;
  // Update LastCapture
  LastCapture0 = ThisCapture0;
  EdgeFlag = 1;
  // Reset the one shot
  HWREG(WTIMER0_BASE+TIMER_O_TBV) = 0;
  //if it has timed out, restart the B Timer
  if(Period0 > ZERO_RPM_TIMEOUT)
  {
    HWREG(WTIMER0_BASE+TIMER_O_CTL) |= (TIMER_CTL_TBEN | TIMER_CTL_TBSTALL);
  }
}

// Interrupt Response Function for Timer A WT1
void InputCaptureResponse1(void)
{
  uint32_t ThisCapture1;
  // Clear the source of the interrupt, the input capture event
  HWREG(WTIMER1_BASE+TIMER_O_ICR) = TIMER_ICR_CAECINT;
  // Grab the captured time and calculate the period
  ThisCapture1 = HWREG(WTIMER1_BASE+TIMER_O_TAR);
  Period1 = ThisCapture1 - LastCapture1;
  // Update LastCapture
  LastCapture1 = ThisCapture1;
  // EdgeFlag = 1;
  // Reset the one shot
  HWREG(WTIMER1_BASE+TIMER_O_TBV) = 0;
  //if it has timed out, restart the B Timer
  if(Period1 > ZERO_RPM_TIMEOUT)
  {
    HWREG(WTIMER1_BASE+TIMER_O_CTL) |= (TIMER_CTL_TBEN | TIMER_CTL_TBSTALL);
  }
}

// Interrupt Response Function for Timer B WT0
void TimeoutResponse0(void)
{
  // Clear the source of the interrupt, the timeout
  HWREG(WTIMER0_BASE+TIMER_O_ICR) = TIMER_ICR_TBTOCINT;
  Period0 = ZERO_RPM_TIMEOUT;
	//printf("timeout \n\r");

}

// Interrupt Response Function for Timer B WT1
void TimeoutResponse1(void)
{
  // Clear the source of the interrupt, the timeout
  HWREG(WTIMER1_BASE+TIMER_O_ICR) = TIMER_ICR_TBTOCINT;
  Period1 = ZERO_RPM_TIMEOUT;
}

// Calculate and Print RPM to the screen
void PrintRPM(void)
{
  //NewToggleMeasurement();
  uint32_t RPM0 = (60*(CLOCK_SPEED/Period0)/TICKS_PER_REV); 
  uint32_t RPM1 = (60*(CLOCK_SPEED/Period1)/TICKS_PER_REV); 
  //NewToggleMeasurement();
	//printf("period0: %d period1: %d \n\r", Period0, Period1);
  //printf("RPM: %d RPM2: %d \n\r", RPM0, RPM1);
	//printf("RPM: %d RPM2: %d \n\r", RPM0, RPM1);
	
}

// PI Controller Interrupt Setup
void InitControllerInterrupt(void)
{
	volatile uint32_t Dummy; 
	// Enable wide timer 5 A and B, A is for RPM and B is for Line follow
	HWREG(SYSCTL_RCGCWTIMER) |= SYSCTL_RCGCWTIMER_R5;
	
	// Kill some time
	Dummy = HWREG(SYSCTL_RCGCGPIO);
	
	// Disable timer a and B
	HWREG(WTIMER5_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TAEN;
  HWREG(WTIMER5_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TBEN;
	// 32 bit timer
	HWREG(WTIMER5_BASE+TIMER_O_CFG) = TIMER_CFG_16_BIT;
	// Set as periodic timer
	HWREG(WTIMER5_BASE+TIMER_O_TAMR) =
(HWREG(WTIMER5_BASE+TIMER_O_TAMR)& ~TIMER_TAMR_TAMR_M)| TIMER_TAMR_TAMR_PERIOD;

  HWREG(WTIMER5_BASE+TIMER_O_TBMR) =
(HWREG(WTIMER5_BASE+TIMER_O_TBMR)& ~TIMER_TBMR_TBMR_M)| TIMER_TBMR_TBMR_PERIOD;
	
	// Set timeout to 2ms -- 40,000,000 ticks per sec = 80,000 ticks per 2ms
	HWREG(WTIMER5_BASE+TIMER_O_TAILR) = 80000;
  HWREG(WTIMER5_BASE+TIMER_O_TBILR) = 800000;
	
	// Reduce the priority- priorities are 0 by default TODO: FOLLOWUP DATASHEET
	//HWREG(NVIC_PRI24) = (HWREG(NVIC_PRI24) & (BIT5LO & BIT6LO & BIT7LO)) + (1<<4);
	
	// Set the timeout interupt
	HWREG(WTIMER5_BASE+TIMER_O_IMR) |= TIMER_IMR_TATOIM | TIMER_IMR_TBTOIM;
	
	// interrupt 104, 105
	// Enable Timer A in Wide Timer 5 interrupt in the NVIC, int 92 EN2, bit 28
	HWREG(NVIC_EN3) |= BIT8HI | BIT9HI;
  // Enable global interrupt
	__enable_irq();
	// kick off the timer with debugger stall enabled
	HWREG(WTIMER5_BASE+TIMER_O_CTL) |= (TIMER_CTL_TAEN |TIMER_CTL_TASTALL | TIMER_CTL_TBEN |TIMER_CTL_TBSTALL);
}

// PI Controller Interrupt Response function, timer Wide 5A
void ControllerInterruptResponse(void)
{
	// 0 and 1 correspond to L and R motors
	// First step- clear the interrupt
	HWREG(WTIMER5_BASE+TIMER_O_ICR) = TIMER_ICR_TATOCINT;
	static float integrand0 = 0;
  static float integrand1 = 0;
	
	//Control terms are in volts, eventually need to command PWM so 5V maps to 100 duty for PWM
	int16_t targetRPM0 = TargetRPM_L; // Get the RPM we want to drive TODO: write this in state machine
  int16_t targetRPM1 = TargetRPM_R;
  Direction_t Direction = FORWARD; // TODO: Need getDirection Function in main state machine. 

	int16_t errorRPM0 = targetRPM0 - GetRPM0();
  int16_t errorRPM1 = targetRPM1 - GetRPM1();

	
	integrand0 += (float)errorRPM0 * .002; // .002 because the interrupt runs every 2ms, so the discrete integral is 2ms.
	integrand1 += (float)errorRPM1 * .002;
	float kP = 3.0;
	float kI = 1.0;
	float kIeffort0 = kI*integrand0;
  float kIeffort1 = kI*integrand1;
	
	// Prevents integrator windup
	if(kIeffort0 > 5) kIeffort0 = 5;
  if(kIeffort1 > 5) kIeffort1 = 5;
	if(kIeffort0 < 0) kIeffort0 = 0;
  if(kIeffort1 < 0) kIeffort1 = 0;
	
	float controlOutput0 = kP*(float)errorRPM0 + kIeffort0;
  float controlOutput1 = kP*(float)errorRPM1 + kIeffort1;
	
	// Clamps output to doable values
	if(controlOutput0 > 5) controlOutput0 = 5;
  if(controlOutput1 > 5) controlOutput1 = 5;
	if(controlOutput0 < 0) controlOutput0 = 0;
  if(controlOutput1 < 0) controlOutput1 = 0;
	float duty0 = controlOutput0 * 20.0;
  float duty1 = controlOutput1 * 20.0;
	//SetMotorDutyCycles((uint8_t)duty0, (uint8_t)duty1, Direction);
	
}

uint32_t GetRPM0(void){
  return (60*(CLOCK_SPEED/Period0)/TICKS_PER_REV); 
}

uint32_t GetRPM1(void){
  return (60*(CLOCK_SPEED/Period1)/TICKS_PER_REV); 
}

// Other functions can call this to set the target RPM
void SetMotorRPM(uint32_t RPMleft, uint32_t RPMright){
  TargetRPM_L = RPMleft;
  TargetRPM_R = RPMright;

}

void SetMotorDutyCycles(uint8_t DutyCycleLeft, uint8_t DutyCycleRight, Direction_t Direction) {
	if (Direction == FORWARD) {
		setDuty(DutyCycleLeft, DutyCycleRight);
	}
	else if (Direction == REVERSE) {
		setDuty(-1*DutyCycleLeft, -1*DutyCycleRight);
	} 
	else if (Direction == LEFT) {
		setDuty(DutyCycleLeft, -1*DutyCycleRight);
	}
	else if (Direction == RIGHT) {
		setDuty(-1*DutyCycleLeft, DutyCycleRight);
	}
}
void OnLineMotorSpeedsInterrupt(void) {
  // First step- clear the interrupt WideTimer 5B
  HWREG(WTIMER5_BASE+TIMER_O_ICR) = TIMER_ICR_TBTOCINT;
  // read Line Sensors
  uint32_t LeftSensor = GetLineReading(LEFT_LINE);
  uint32_t RightSensor = GetLineReading(RIGHT_LINE);
	//printf("Left: %d \t Right: %d \t Front: %d \t", GetLineReading(LEFT_LINE), GetLineReading(RIGHT_LINE), GetLineReading(CENTER_LINE));

  //static float integrand = 0;
  
  //Control terms are in volts, eventually need to command PWM so 5V maps to 100 duty for PWM
  //int16_t targetdiff = 0; // Get the RPM we want to drive TODO: write this in state machine
  //Direction_t Direction = FORWARD; // TODO: Need getDirection Function in main state machine. 

  int32_t errordiff = LeftSensor - RightSensor;
  int32_t errorabs = abs(errordiff);
  //float slowdown= SLOWDOWN_MAX / LINE_SENSOR_DIFF * errorabs;
	
	float slowdown = SLOWDOWN_MAX*0;

  // 3500 - 1500= 2000 is biggest difference, should map to LINE_FOLLOW_RPM duty
	float float_diff = LINE_SENSOR_DIFF;
  float kP = .055;//LINE_FOLLOW_RPM/ float_diff;
  // taking out integration for now
  //integrand += (float)errordiff * .002; // .002 because the interrupt runs every 2ms, so the discrete integral is 2ms.
  //float kI = 1.0;
  //float kIeffort = kI*integrand;
  
  // Prevents integrator windup
  //if(kIeffort > 5) kIeffort = 5;
  //if(kIeffort < 0) kIeffort = 0;
  
  //float controlOutput = kP*(float)errordiff + kIeffort;
  
  float controlOutput = kP * (float)errordiff;
  // At 50% duty cycle no load, goes 90 rpm
  // Clamps output to doable values
  if(controlOutput > LINE_FOLLOW_RPM) controlOutput = LINE_FOLLOW_RPM;
  if(controlOutput < -LINE_FOLLOW_RPM) controlOutput = -LINE_FOLLOW_RPM;
  uint32_t RPM0 = 0;
	uint32_t RPM1 = 0;
	if(controlOutput >0 ){
      RPM0 = LINE_FOLLOW_RPM - (controlOutput) - slowdown; // slow down left motor to veer left
      RPM1 = LINE_FOLLOW_RPM - slowdown;
  } else{
      RPM1 = LINE_FOLLOW_RPM + (controlOutput) - slowdown; // slow down right motor to veer left
      RPM0 = LINE_FOLLOW_RPM - slowdown;
  }
	//printf("Duty0: %d \t Duty1: %d \r\n", RPM0, RPM1);
  //SetMotorRPM(RPM0, RPM1); // This calls Project_MotorService
	SetMotorDutyCycles(RPM0, RPM1, REVERSE);
}

void subroutine(void) {
  uint32_t LeftSensor = GetLineReading(LEFT_LINE);
  uint32_t RightSensor = GetLineReading(RIGHT_LINE);

  while(GetLineReading(CENTER_LINE) < LINE_PROXIMITY) {
		  printf("Left: %d \t Right: %d \t Front: %d \r\n", GetLineReading(LEFT_LINE), GetLineReading(RIGHT_LINE), GetLineReading(CENTER_LINE));
    SetMotorDutyCycles(50,50, REVERSE);
  }
  SetMotorDutyCycles(0,0,REVERSE);
	
  if(GetLineReading(LEFT_LINE) > GetLineReading(RIGHT_LINE)){
    while(GetLineReading(RIGHT_LINE) + GetLineReading(LEFT_LINE)< COMBINED_LINE){
      SetMotorDutyCycles(40,40,LEFT);
    }
  } else{
    while(GetLineReading(RIGHT_LINE) + GetLineReading(LEFT_LINE)< COMBINED_LINE){
      SetMotorDutyCycles(40,40,RIGHT);
    }
  }
  SetMotorDutyCycles(0,0,REVERSE);
	

}

void EnableLineFollowInts(void) {
  // enable wide timer 5B
  // enable a local timeout interrupt for Timer B
  HWREG(WTIMER5_BASE+TIMER_O_IMR) |= TIMER_IMR_TBTOIM;

}

void DisableLineFollowInts(void) {
  // disable wide timer 5B
  // disable a local timeout interrupt for Timer B
  HWREG(WTIMER5_BASE+TIMER_O_IMR) &= ~TIMER_IMR_TBTOIM;
}


/*
void OnLineMotorSpeedsInterrupt(void) {
  // First step- clear the interrupt WideTimer 5B
  HWREG(WTIMER5_BASE+TIMER_O_ICR) = TIMER_ICR_TBTOCINT;
  // read Line Sensors
  uint32_t LeftSensor = GetLineReading(LEFT_LINE);
  uint32_t RightSensor = GetLineReading(RIGHT_LINE);

  //static float integrand = 0;
  
  //Control terms are in volts, eventually need to command PWM so 5V maps to 100 duty for PWM
  //int16_t targetdiff = 0; // Get the RPM we want to drive TODO: write this in state machine
  //Direction_t Direction = FORWARD; // TODO: Need getDirection Function in main state machine. 

  int16_t errordiff = LeftSensor - RightSensor;

  // 4098 - 0 is biggest difference, should map to 60 RPM. 
  float kP = .02; // a bit higher than .0147 
  // taking out integration for now
  //integrand += (float)errordiff * .002; // .002 because the interrupt runs every 2ms, so the discrete integral is 2ms.
  //float kI = 1.0;
  //float kIeffort = kI*integrand;
  
  // Prevents integrator windup
  //if(kIeffort > 5) kIeffort = 5;
  //if(kIeffort < 0) kIeffort = 0;
  
  //float controlOutput = kP*(float)errordiff + kIeffort;
  
  float controlOutput = kP * (float)errordiff;
  // At 50% duty cycle no load, goes 90 rpm
  // Clamps output to doable values
  if(controlOutput > LINE_FOLLOW_RPM) controlOutput = LINE_FOLLOW_RPM;
  if(controlOutput < -LINE_FOLLOW_RPM) controlOutput = -LINE_FOLLOW_RPM;
  uint32_t RPM0 = 0;
	uint32_t RPM1 = 0;
	if(controlOutput >0 ){
      RPM0 = LINE_FOLLOW_RPM - (controlOutput); // slow down left motor to veer left
      RPM1 = LINE_FOLLOW_RPM;
  } else{
      RPM1 = LINE_FOLLOW_RPM - (controlOutput); // slow down right motor to veer left
      RPM0 = LINE_FOLLOW_RPM;
  }
  SetMotorRPM(RPM0, RPM1); // This calls Project_MotorService

}
*/



/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/
