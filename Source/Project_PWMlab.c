#include "Project_PWMlab.h"
#include <stdbool.h>

// headers to access GPIO subsystem
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_timer.h"
#include "inc/hw_pwm.h"

// headers to access the TivaWare Library
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include "BITDEFS.H"

#define PWM_TICKS_PER_S 1250000 //seconds so it plays well with freq in Hz
#define ALL_BITS (0xFF<<2)

//Init the pwm on PB6 and PB7 using the BetterPWMDemo sample code
//Make Left Motor PB6 PWM and Right Motor PB7 PWM 
//PB 2 and 3 are the Drive lines for L/R motors
void PWMInit(uint32_t freq)
{
	volatile uint32_t Dummy;
	HWREG(SYSCTL_RCGCPWM) |= SYSCTL_RCGCPWM_R0; // Enable clock to PWM Module (PWM0)
	HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R1; // Enable clock to Port B
	HWREG(SYSCTL_RCC) = (HWREG(SYSCTL_RCC) & ~SYSCTL_RCC_PWMDIV_M) | 
		(SYSCTL_RCC_USEPWMDIV | SYSCTL_RCC_PWMDIV_32); // Select PWM clock as System Clock/32

	while ((HWREG(SYSCTL_PRPWM) & SYSCTL_PRPWM_R0) != SYSCTL_PRPWM_R0) // Make sure PWM module clock has started
	;
	
	HWREG(PWM0_BASE + PWM_O_0_CTL) = 0; // Disable PWM while initializing
	// Program Generator A to go to 1 at rising CompareA, 0 on falling CompareA
	HWREG(PWM0_BASE + PWM_O_0_GENA) = (PWM_0_GENA_ACTCMPAU_ONE | PWM_0_GENA_ACTCMPAD_ZERO);
	// Same for program generator B
	HWREG(PWM0_BASE + PWM_O_0_GENB) = (PWM_0_GENB_ACTCMPBU_ONE | PWM_0_GENB_ACTCMPBD_ZERO);
	// Set PWM Period. since it is counting both up and down, initialize the load register to 1/2 the 
	// desired total period. Also program the match compare registered to 1/2 the desired high time.
	HWREG( PWM0_BASE+PWM_O_0_LOAD) = (PWM_TICKS_PER_S/freq-1)>>1; // 1/freq is period
	// Initial 50% duty cycle for A and B
	HWREG( PWM0_BASE+PWM_O_0_CMPA) = HWREG( PWM0_BASE+PWM_O_0_LOAD)>>1;
	HWREG( PWM0_BASE+PWM_O_0_CMPB) = HWREG( PWM0_BASE+PWM_O_0_LOAD)>>1;
	// Enable PWM outputs
	HWREG( PWM0_BASE+PWM_O_ENABLE) |= (PWM_ENABLE_PWM1EN | PWM_ENABLE_PWM0EN);
	// Configure Port B pins to be PWM outputs - Select Alternate Function
	HWREG(GPIO_PORTB_BASE+GPIO_O_AFSEL) |= (BIT6HI | BIT7HI);
	// Map PWM to those pins, this is a mux value of 4 that we want to use for specifying
	// the function on bits 6 and 7
	HWREG(GPIO_PORTB_BASE+GPIO_O_PCTL) =
		(HWREG(GPIO_PORTB_BASE+GPIO_O_PCTL) & 0x00fffff) + (4<<(6*4)) + (4<<(7*4)); // there are 4 bits per nibble
	HWREG(GPIO_PORTB_BASE+GPIO_O_DEN) |= (BIT2HI | BIT3HI | BIT6HI | BIT7HI); // Enable Port B digital IO
	HWREG(GPIO_PORTB_BASE+GPIO_O_DIR) |= (BIT2HI | BIT3HI | BIT6HI | BIT7HI); // Enable Port B for Output
	// Set up/down count mode, enable the PWM generator and make both generator updates locally
	// Synchronized to 0 count.
	HWREG(PWM0_BASE+ PWM_O_0_CTL) = (PWM_0_CTL_MODE | PWM_0_CTL_ENABLE |
										PWM_0_CTL_GENAUPD_LS | PWM_0_CTL_GENBUPD_LS);
}

//Set the duty cycle from 0 to 99, in 1% increments. Make 100% 99% to avoid complexity
void setDuty(int8_t dutyL, int8_t dutyR)
{
	if(dutyL >= 100) dutyL = 99;
	if(dutyR >= 100) dutyR = 99;
	if(dutyL <= -100) dutyL = -99;
	if(dutyR <= -100) dutyR = -99;
	if(dutyL <0) {
		dutyL = -dutyL;
		HWREG(PWM0_BASE+PWM_O_0_CMPA) = (dutyL)*HWREG(PWM0_BASE+PWM_O_0_LOAD)/100;
		HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) |= (GPIO_PIN_2);
	} else {
		HWREG(PWM0_BASE+PWM_O_0_CMPA) = ((100-dutyL)*HWREG(PWM0_BASE+PWM_O_0_LOAD))/100;
		HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) &= ~(GPIO_PIN_2);
	}
	if(dutyR <0) {
		dutyR = -dutyR;
		HWREG(PWM0_BASE+PWM_O_0_CMPB) = (dutyR)*HWREG(PWM0_BASE+PWM_O_0_LOAD)/100;
		HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) |= (GPIO_PIN_3);
	} else {
		HWREG(PWM0_BASE+PWM_O_0_CMPB) = ((100-dutyR)*HWREG(PWM0_BASE+PWM_O_0_LOAD))/100;
		HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) &= ~(GPIO_PIN_3);
	}

}
