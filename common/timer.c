
#include "system.h"
#include "timer.h"
#include "led.h"
/*********************************************************************
 * Function:        TimerInit
 *
 * PreCondition:    None.
 *
 * Input:       	None.	
 *                  
 * Output:      	None.
 *
 * Overview:        Initializes Timer0 for use.
 *
 ********************************************************************/


volatile unsigned long long g_tick=0;

void TimerInit(void)
{	
//	PR1 = (PROCESSOR_MIPS*1000ul)-1; // 0x2FF;
//	IPC0bits.T1IP = 5;
//	T1CON = 0b1000000000010000;
//	IFS0bits.T1IF = 0;

 	g_tick = 0;		/* Reset ms counter */
	TMR1 = 0; 		/* clear timer register */
	T1CON = 0;		/* ensure Timer is in reset state */
	PR1 = (unsigned int)TMR1_PERIOD; 		/* set period1 register */
	IPC0bits.T1IP = 6; 		/* set priority level */
	T1CONbits.TSIDL = 0;	/* continue operation in idle mode */
	T1CONbits.TCS = TIMERSOURCE;	/* select internal or external timer clock */
	T1CONbits.TCKPS = TIMERPRESCALER;
	T1CONbits.TON = 1;	/* enable Timer and start the count */ 	
	IFS0bits.T1IF = 0;	/* reset Timer interrupt flag */
 	IEC0bits.T1IE = 1;	/* enable Timer interrupt */


}

/*********************************************************************
 * Function:        TimerIsOverflowEvent
 *
 * PreCondition:    None.
 *
 * Input:       	None.	
 *                  
 * Output:      	Status.
 *
 * Overview:        Checks for an overflow event, returns TRUE if 
 *					an overflow occured.
 *
 * Note:            This function should be checked at least twice
 *					per overflow period.
 ********************************************************************/
/*unsigned char TimerIsOverflowEvent(void)
{
	if (IFS0bits.T1IF)
	{		
		IFS0bits.T1IF = 0;
		g_tick++;
		return(1);
	}
	return(0);
}
*/
/*---------------------------------------------------------------------
  Function Name: _T1Interrupt
  Description:   Timer1 Interrupt Handler
  Inputs:        None
  Returns:       None
-----------------------------------------------------------------------*/
#ifdef SU_APP
	#if (RTCC_SOURCE == RTCC_EXTERNAL)
		volatile time_t g_seconds = 0;
		static unsigned int counter = 0;
	#endif
#else
	volatile time_t g_seconds = 0;
	static unsigned int counter = 0;
#endif

void _ISRFAST _T1Interrupt( void )
{
	g_tick++;
#ifdef SU_APP
#if (RTCC_SOURCE == RTCC_EXTERNAL)
	if (++counter >= TICKSPERSECOND) {		// only use this if there is no 1 second RTCC tick
		counter = 0;
		g_seconds++;			// increment seconds counter
	}
#endif
#else
	if (++counter >= TICKSPERSECOND) {		// only use this if there is no 1 second RTCC tick
		counter = 0;
		g_seconds++;			// increment seconds counter
	}
#endif
#ifdef MU_APP
	ProcessLEDTasks();
#endif
 	IFS0bits.T1IF = 0;	/* reset Timer interrupt flag */
}


/*********************************************************************
 * EOF
 ********************************************************************/
