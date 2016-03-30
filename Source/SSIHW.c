/****************************************************************************
 Module
   SSIHW.c

 Revision
   1.0.1

 Description
   This is a template file for implementing flat state machines under the
   Gen2 Events and Services Framework.

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 01/15/12 11:12 jec      revisions for Gen2 framework
 11/07/11 11:26 jec      made the queue static
 10/30/11 17:59 jec      fixed references to CurrentEvent in RunSSIHWSM()
 10/23/11 18:20 jec      began conversion from SMTemplate.c (02/20/07 rev)
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "SSIHW.h"

#include "EventPrinter.h"
#include "MasterSM.h"

// headers to access GPIO subsystem
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_timer.h"
#include "inc/hw_nvic.h"
#include "inc/hw_ssi.h"


// headers to access the TivaWare Library
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"

#include "BITDEFS.H"


/*----------------------------- Module Defines ----------------------------*/

// #define SSI_HW_TEST

// PA 2 through 5: Clk, Fss, Rx, Tx
#define SSI_PORT_BASE GPIO_PORTA_BASE
#define CLK_PIN BIT2HI
#define FSS_PIN BIT3HI
#define RX_PIN BIT4HI
#define TX_PIN BIT5HI
#define ALL_PINS (CLK_PIN | FSS_PIN | RX_PIN | TX_PIN)

#define CLK_PRESCALER 16 // must be even
#define SERIAL_CLOCK_RATE 249

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
*/

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match that of enum in header file
uint8_t lastByte;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitSSIHW

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, sets up the initial transition and does any
     other required initialization for this state machine
		 
		 initializes hardware for SPI interface
 Notes

 Author
     J. Edward Carryer, 10/23/11, 18:55
****************************************************************************/
void InitSSIHW ( void )
{

    // PA2 - SSI0Clk
  // PA3 - SSI0Fss
  // PA4 - SSI0Rx
  // PA5 - SSI0Tx
    
  // use volatile to avoid over-optimizing
  volatile uint32_t Dummy;
  
  // enable clock to GPIO port (Port A)
  HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R0;
  // enable clock to SSI module (module 0)
  HWREG(SYSCTL_RCGCSSI) |= SYSCTL_RCGCSSI_R0;
  // wait for GPIO port to be ready
  while ((HWREG(SYSCTL_PRGPIO)& SYSCTL_PRGPIO_R0) != SYSCTL_PRGPIO_R0)
    // printf("Waiting for GPIO port to be ready.\r\n");
  ;
  // program GPIO to use the alternate functions on the SSI pins (Port A, all 4 pins)
  HWREG(SSI_PORT_BASE+GPIO_O_AFSEL) |= ALL_PINS;
  // set mux position in GPIOPCTL to select the SSI use of the pins (Port A, mux=2)
  HWREG(SSI_PORT_BASE+GPIO_O_PCTL) =
  ((HWREG(SSI_PORT_BASE+GPIO_O_PCTL) & 0xff0000ff) + 0x00222200);
  // program the port lines for digital I/O
  HWREG(SSI_PORT_BASE+GPIO_O_DEN) |= ALL_PINS;
  // program the required data directions on the port lines
  // outputs (clock out, data out, frame select), input (data in)
  HWREG(SSI_PORT_BASE+GPIO_O_DIR) |= (CLK_PIN | FSS_PIN | TX_PIN);
  HWREG(SSI_PORT_BASE+GPIO_O_DIR) &= ~RX_PIN;
  // if using SPI mode 3 (which we are), program pull-up clock line
  HWREG(SSI_PORT_BASE+GPIO_O_PUR) |= CLK_PIN;
  // wait for the SSI0 to be ready (module 0)
  while ((HWREG(SYSCTL_PRSSI) & SYSCTL_PRSSI_R0) != SYSCTL_PRSSI_R0)
        // printf("Waiting for SSI0 to be ready.\r\n");
  ;
  // make sure that the SSI is disabled before programming mode bits
  HWREG(SSI0_BASE+SSI_O_CR1) &= ~SSI_CR1_SSE;
	//Disable LoopBack Mode
	HWREG(SSI0_BASE+SSI_O_CR1)&=(~SSI_CR1_LBM);
	//Select Clock Source
	HWREG(SSI0_BASE+SSI_O_CC) = (HWREG(SSI0_BASE+SSI_O_CC) & 0xFFFFFFF0);
  // select master mode (MS) & TXRIS indicating End of Transmit (EOT)
  HWREG(SSI0_BASE+SSI_O_CR1) &= ~SSI_CR1_MS;
  HWREG(SSI0_BASE+SSI_O_CR1) |= SSI_CR1_EOT;
  // configure the clock prescaler

  // bitrate = SYSCLK / (CPDVSR * (1+SCR)) = 40M / (80 * (1+99)) = 5000 Hz < 961k
  HWREG(SSI0_BASE+SSI_O_CPSR) = HWREG(SSI0_BASE+SSI_O_CPSR)&(0xFFFFFF00) + (CLK_PRESCALER);
  // configure the clock rate (SCR), phase & polarity (SPH = 1, SPO = 1 in this case),
  HWREG(SSI0_BASE+SSI_O_CR0) = (HWREG(SSI0_BASE+SSI_O_CR0) & ~SSI_CR0_SCR_M) + (SERIAL_CLOCK_RATE<<8);
	HWREG(SSI0_BASE+SSI_O_CR0)|= SSI_CR0_SPO|SSI_CR0_SPH|SSI_CR0_FRF_MOTO|SSI_CR0_DSS_8;
  // set to 1 the local mask (TXIM in the SSI Interrupt Mask register)
  HWREG(SSI0_BASE+SSI_O_IM) |= SSI_IM_TXIM;
  // make sure that the SSI is enabled for operation
	HWREG(SSI0_BASE+SSI_O_CR1)|=BIT1HI;
  // make sure interrupts are enabled globally
  __enable_irq();
  /*
  printf("CR0: %#010x\t CR1: %#010x\r\n",HWREG(SSI0_BASE+SSI_O_CR0),HWREG(SSI0_BASE+SSI_O_CR1)); 
  printf("RCGCSSI: %#010x\t Clock Source: %#010x\r\n",HWREG(SYSCTL_RCGCSSI),HWREG(SSI0_BASE+SSI_O_CC)); 
  printf("Clock Rate: %#010x\t Interrupt Mask: %#010x\r\n",HWREG(SSI0_BASE+SSI_O_CPSR),HWREG(SSI0_BASE+SSI_O_IM)); 
*/
	
  lastByte = 0;

	printf("SSI initialized.\r\n");
}

// ISR for EOT interrupt
void SSIHWEOTIntHandler (void)  {
  // disable the EOT interrupt in the NVIC
  HWREG(NVIC_EN0) &= ~BIT7HI;

  // clear the interrupt
  HWREG(SSI0_BASE+SSI_O_ICR) |= SSI_ICR_EOTIC;

  // declare Event2Post
  ES_Event Event2Post;
  Event2Post.EventType = SSI_EOT;
  PostMasterSM(Event2Post);
	// PostEventPrinter(Event2Post);
  // printf("thisByte: %d \t lastByte: %d\r\n", thisByte, lastByte);
  

}

void TransmitSSI( uint8_t byte2Write) {
	// printf("Writing a byte to the data register: %#04x\r\n", byte2Write);
	// write the byte to the SSI Data Register
	// enable the NVIC interrupt for the SSI when starting to transmit (SSI0 Tx interrupt - #7)
	HWREG(NVIC_EN0) |= BIT7HI;
	
	HWREG(SSI0_BASE+SSI_O_DR) = byte2Write;
	
}

// get latest byte sent in from SSI
uint8_t ReadSSI(void) {
  // grab the byte from the SSI Data Register and store it in thisByte
	lastByte = HWREG(SSI0_BASE+SSI_O_DR);
	return lastByte;
}




/***************************************************************************
 private functions
 ***************************************************************************/



/*------------------------------- Test Harness -------------------------------*/

#ifdef SSI_HW_TEST

#include "termio.h"
#define clrScrn()   printf("\x1b[2J")
#define goHome()  printf("\x1b[1,1H")
#define clrLine() printf("\x1b[K")

#include "EnablePA25_PB23_PD7_PF0.h"


int main(void) {
  TERMIO_Init();
  clrScrn();
  
  // When doing testing, it is useful to announce just which program
  // is running.
	PortFunctionInit();

  puts("\rStarting Test Harness for \r");
  printf("SSIHW\r\n");
  printf("%s %s\n",__TIME__, __DATE__);
  printf("\n\r\n");

  printf("Initialize hardware, then transmit 0xC0 to SSI\r\n");

  InitSSIHW();


  while (true) {
		for (int i = 0; i < 1000000; i++) {}
    TransmitSSI(0xC0);
    TransmitSSI(0x00);
    TransmitSSI(0x00);
		TransmitSSI(0x00);
		TransmitSSI(0x00);
		for (int i = 0; i < 1000000; i++) {}
		printf("%#04x \t \r\n",ReadSSI());
		printf("%#04x \t \r\n",ReadSSI());
		printf("%#04x \t \r\n",ReadSSI());
		printf("%#04x \t \r\n",ReadSSI());
		printf("%#04x \t \r\n\n\n\n",ReadSSI());
		
  }

  return 0;
}

#endif



/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/


