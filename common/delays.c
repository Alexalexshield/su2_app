#include "system.h"
#include "delays.h"

void DelayulUSecs(unsigned long usecs)
{
	if (usecs >= 1000000ul) {
		DelaySecs((unsigned int)(usecs/1000000ul));
		usecs = usecs%1000000ul;
	}
	if (usecs >= 1000ul) {
		DelayMsecs((unsigned int)(usecs/1000ul));
		usecs = usecs%1000ul;
	}
	if (usecs > 2ul) {
		DelayUsecs((unsigned int)usecs);
	}
}
	
void DelaySecs(unsigned int secs)
{
	while(secs--)
		DelayMsecs(1000);
}	

void DelayMsecs(unsigned int msecs) 
{
	while (msecs--) {
		DelayUsecs(1000);//
//		DelayTMR4Usecs(1000);
	}
}	

	
// warning - value must be less than 4000 to not overflow when runing at 16 MIPS
/*
void DelayTMR4Usecs(unsigned int usecs) 
{
	if (usecs < 2) usecs = 2;	// mimimum of 2 usecs
	TMR4 = 0; 				// clear timer2 register 
	T4CON = 0;			// ensure Timer 2 is in reset state 
	PR4 = (unsigned int)((PROCESSOR_MIPS*usecs)-1); 		// set period1 register 
	T4CONbits.TCS = 0;	// select internal timer clock 
	T4CONbits.TCKPS = 0;	// 0 = 1:1,
	IFS1bits.T4IF = 0;	// reset Timer 2 interrupt flag 
	T4CONbits.TON = 1;	// enable Timer 2 and start the count  	
 	while (!IFS1bits.T4IF);
	T4CONbits.TON = 0;	// disable Timer 4   
}
*/

// this delay is approximate
void DelayUsecs(unsigned int usecs) 
{
	if (usecs <= 1) return;
    register unsigned int count = (((long)usecs) * PROCESSOR_MIPS)*10/75;
    while (count--) {
    } 
}        
   
   
// goes to sleep and uses secondary oscillator interrupt to wake up
// uses multiples of 32.768 usec only
/*
static volatile int g_T4_Count;

void Delay_LowPower_TMR4Usecs(unsigned int usecs) 
{
	if (usecs < 66) {
		DelayTMR4Usecs(usecs);
		return;
	}
	TMR4 = 0; 			// clear timer4 register 
	T4CON = 0;			// ensure Timer 4 is in reset state 
	IPC6bits.T4IP = 5; 		// set priority level 
	PR4 = (unsigned int)(((usecs+15)/33)-1); 		// set period1 register 
	T4CONbits.TSIDL = 0;	// continue operation in idle mode 
	T4CONbits.TCS = 1;	// External clock from T1CK pin (on the rising edge) 32.768 khz crystal (secondary clock)
	T4CONbits.TCKPS = 0;	// 0 = 1:1,
	g_T4_Count = 0;
	T4CONbits.TON = 1;	// enable Timer 4 and start the count  
	IFS1bits.T4IF = 0;	// reset Timer 4 interrupt flag 
	IEC1bits.T4IE = 1;	// enable Timer interrupt 	
// 	while (!IFS1bits.T4IF);
 	while (g_T4_Count==0) Sleep();
 		
	T4CONbits.TON = 0;	// disable Timer 4   
#if PROCESSOR_MIPS == 16	
	while(OSCCONbits.LOCK!=1) {}; // Wait for PLL to lock
#endif	
}
  
   
void __attribute__((__interrupt__, no_auto_psv)) _T4Interrupt( void )
{
	g_T4_Count++;
 	IFS1bits.T4IF = 0;	// reset Timer interrupt flag 
}

*/
