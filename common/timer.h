#ifndef __TIMER0_H
#define __TIMER0_H
#include "time.h"

// comment the next line to use the external clock source
#define USEINTERNALCLOCK 1

#ifdef USEINTERNALCLOCK
#define TMR1_PERIOD ((FCY/1000)-1)
#define TICKSPERSECOND 1000
#define TIMERSOURCE 0
#define TIMERPRESCALER 0		// 0 = 1:1, 1 = 1:8, 2 = 1:64, 3 = 1:256
#else  
/* Timer2 period for 1.024 ms with external 32kz crystal */
#define TMR1_PERIOD (32-1)
#define TICKSPERSECOND 1024
#define TIMERSOURCE 1		// External clock from T1CK pin (on the rising edge)
#define TIMERPRESCALER 0	// 0 = 1:1
#endif


extern volatile unsigned long long g_tick;
extern void TimerInit(void);

#ifdef SU_APP

#if (RTCC_SOURCE == RTCC_EXTERNAL)
	extern volatile time_t g_seconds;
#endif

#else
	extern volatile time_t g_seconds;
#endif


//extern unsigned char TimerIsOverflowEvent(void);

/*********************************************************************
 * EOF
 ********************************************************************/

#endif //__TIMER0_H
