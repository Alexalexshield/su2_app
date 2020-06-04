
#define SU2_BOARD

#ifdef MU_APP
//#define PROCESSOR_MIPS 40 
//#define PROCESSOR_MIPS 8 
#define PROCESSOR_MIPS 16
//#define PROCESSOR_MIPS 4
#else
//#define PROCESSOR_MIPS 40 
//#define PROCESSOR_MIPS 8 
#define PROCESSOR_MIPS 16
//#define PROCESSOR_MIPS 4
#endif

#define SYSCLK (PROCESSOR_MIPS * 2000000ul)
#define FCY (SYSCLK/2)

#define GetSystemClock()		(SYSCLK)      
#define GetInstructionClock()	(GetSystemClock()/2)
#define GetPeripheralClock()	GetInstructionClock()
#define CLOCK_FREQ 		(SYSCLK)



#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#ifndef FALSE
#define FALSE   0
#endif

#ifndef TRUE
#define TRUE    1
#endif

#ifndef NULL
#define NULL    0
#endif


#define int8    char
#define int16   short
#define int32   long
#define int64   long long
#define U8 unsigned char
#define U16 unsigned short
#define U32 unsigned long

#define INPUT   1
#define OUTPUT  0

#include <p24fxxxx.h>

#define ROM					const
#define memcmppgm2ram(a,b,c)	memcmp(a,b,c)
#define memcpypgm2ram(a,b,c)	memcpy(a,b,c)
#define strcpypgm2ram(a, b)		strcpy(a,b)
#define Reset()					asm("reset")
#define SLEEP()					Sleep()
#define CLRWDT()				ClrWdt()
#define NOP()					Nop()
#define DISABLE_WDT()			(RCONbits.SWDTEN = 0)
#define ENABLE_WDT()			(RCONbits.SWDTEN = 1)

#define RTCC_INTERNAL 0
#define RTCC_EXTERNAL 1	

#define	DEFAULT_ID	2

#ifdef SU_APP
#define SOURCE_PREFIX "SU"
#define MICRO_ID_STR "SU2"
#define SW_VERSION_STR "2.4"  //used to be 2.3
#define MICRO_ID_1 'S'
#define MICRO_ID_2 'U'
#define MICRO_ID_3 '2'
#define BOARD_REV 2
//#define RTCC_SOURCE RTCC_INTERNAL
#define RTCC_SOURCE RTCC_EXTERNAL	

// NOTE - LEDS ON CONTROLLER ARE CATHODE TYPE SO LOGIC IS 1=ON
/////////////////////// LED 1 is Red on the SU_RX board rev 2
#define RED_LED_TRIS 		(_TRISD3)
#define RED_LED_LATCH 		_LATD3
#define RED_LED_POWER_ON 	(RED_LED_LATCH = 1)
#define RED_LED_POWER_OFF	(RED_LED_LATCH = 0)
#define TOGGLE_RED_LED 	(__builtin_btg((unsigned int *)&LATD, 3))

/////////////////////// LED 2 is Green on the SU_RX board rev 2
#define GREEN_LED_TRIS 		(_TRISD2)
#define GREEN_LED_LATCH 	_LATD2
#define GREEN_LED_POWER_ON 	(GREEN_LED_LATCH = 1)
#define GREEN_LED_POWER_OFF	(GREEN_LED_LATCH = 0)
#define TOGGLE_GREEN_LED 	(__builtin_btg((unsigned int *)&LATD, 2))

////////////// LED 3 is BLUE on the SU_RX board rev 2
#define BLUE_LED_TRIS 		(_TRISD1)
#define BLUE_LED_LATCH 		_LATD1
#define BLUE_LED_POWER_ON 	(BLUE_LED_LATCH = 1)
#define BLUE_LED_POWER_OFF	(BLUE_LED_LATCH = 0)
#define TOGGLE_BLUE_LED 	(__builtin_btg((unsigned int *)&LATD, 3))
			
#endif

#ifdef SU_TX
#define SOURCE_PREFIX "TX"
#define MICRO_ID_STR "SU2_TX"
#define SW_VERSION_STR "2.2"
#define MICRO_ID_1 'S'
#define MICRO_ID_2 'U'
#define MICRO_ID_3 'T'
#define BOARD_REV 2
#endif

#ifdef SU_RX
#define SOURCE_PREFIX "RX"
#define MICRO_ID_STR "SU2_RX"
#define SW_VERSION_STR "2.1"
#define MICRO_ID_1 'S'
#define MICRO_ID_2 'U'
#define MICRO_ID_3 'R'
#define BOARD_REV 2

// NOTE - LEDS ON CONTROLLER ARE SURFACE MOUNT TYPE SO LOGIC IS 0=ON
/////////////////////// LED 1 is Red on the SU_RX board rev 2
#define RED_LED_TRIS 		(_TRISD1)
#define RED_LED_LATCH 		(_LATD1)
#define RED_LED_POWER_ON 	(_LATD1 = 0)
#define RED_LED_POWER_OFF	(_LATD1 = 1)
#define TOGGLE_RED_LED 	(__builtin_btg((unsigned int *)&LATD, 1))

/////////////////////// LED 2 is Green on the SU_RX board rev 2
#define GREEN_LED_TRIS 		(_TRISD2)
#define GREEN_LED_LATCH 	(_LATD2)
#define GREEN_LED_POWER_ON 	(_LATD2 = 0)
#define GREEN_LED_POWER_OFF	(_LATD2 = 1)
#define TOGGLE_GREEN_LED 	(__builtin_btg((unsigned int *)&LATD, 2))

////////////// LED 3 is BLUE on the SU_RX board rev 2
#define BLUE_LED_TRIS 		(_TRISD3)
#define BLUE_LED_LATCH 		(_LATD3)
#define BLUE_LED_POWER_ON 	(_LATD3 = 0)
#define BLUE_LED_POWER_OFF	(_LATD3 = 1)
#define TOGGLE_BLUE_LED 	(__builtin_btg((unsigned int *)&LATD, 3))


#endif

#ifdef MU_APP
#define SOURCE_PREFIX "MU"
#define MICRO_ID_STR "MU2"
#define SW_VERSION_STR "1.3"
#define MICRO_ID_1 'M'
#define MICRO_ID_2 'U'
#define MICRO_ID_3 '2'
#endif

// OFFSET IN FLASH (OR EEPROM) MEMORY FOR SAVING CONFIG OR SETTINGS STRUCTURES
#define CONFIG_EEPROM_OFFSET 4
#define SETTINGS_EEPROM_OFFSET 32


#include "iomapping.h"
#include "GenericTypeDefs.h"
#include "csv_packet.h"
#include "delays.h"
#include "timer.h"
#include "port.h"
#include "config.h"
#include "su_struct.h"
#ifndef MU_APP
#include "adc.h"
#endif

extern volatile UnitFlags g_UnitFlags;
extern volatile SystemError g_SysError;


/*****************************************************************************
 * EOF
 *****************************************************************************/
