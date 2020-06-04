/**************************************************************************************************

**************************************************************************************************/

#ifndef VLF_RXTX_H
#define VLF_RXTX_H

/*************************************************************************************************/
/************************************* Included Files ********************************************/
/*************************************************************************************************/

#include "system.h"

#define SEARCH_TIMEOUT_SECONDS_P1 (7+3)			// PROTOCOL 1 MU's respond at random 1 second slots up to 5 seconds + 2 seconds overhead + 1/2 clock comparison error
#define QUICKSEARCH_TIMEOUT_SECONDS_P1 (7+3)	// PROTOCOL 1 MU's respond at random 1 second slots up to 5 seconds + 2 seconds overhead + 1/2 clock comparison error

#define SEARCH_TIMEOUT_MS_P2 (3500)				// PROTOCOL 2 MU's respond at random 300 ms second slots up to 3 seconds + 500ms overhead
#define QUICKSEARCH_TIMEOUT_MS_P2 (1500)		// PROTOCOL 2 MU's respond immediately with 500ms overhead

#define LOCATE_TIMEOUT_MS_P1 (3000)				// PROTOCOL 1 MU's respond to the Locate command in 2 ms + 2 seconds overhead + 1/2 clock comparison error
#define LOCATE_TIMEOUT_SECONDS_P1 (4)			// PROTOCOL 1 MU's respond to the Locate command in 2 ms + 2 seconds overhead + 1/2 clock comparison error
#define LOCATE_TIMEOUT_MS_P2 (2000)				// PROTOCOL 2 MU's respond after 500ms delay and it takes 300ms to receive it

#define P1_WAIT_FOR_MASK_RESPONSE_MS 8000
#define P2_WAIT_FOR_MASK_RESPONSE_MS 2500
#define P2_WAIT_FOR_FULL_MASK_RESPONSE_MS 2000

#define SUDEM_TRIS (TRISFbits.TRISF6)
#define SUDEM_IO  (PORTFbits.RF6)
#define SUDEM_LATCH _LATF6
#define SUTX_POWER_ON 	(_LATF6 = 1)
#define SUTX_POWER_OFF	(_LATF6 = 0)

//AUDIO MUTE uses port RD11 (pin 45) on the SU2 main controller board. It drives a pulldown resistor of 10K and is normally low to enable the audio board and is high to mute the audio output.
//The mute should be activated during the SU2 Transmit cycle so that it does not overwhelm the speakers/headphones.
#if (BOARD_REV == 1)
#define AUDIO_MUTE_TRIS (_TRISD11)
#define AUDIO_MUTE_LATCH (_LATD11)
#define AUDIO_MUTE_DELAY_MS 20
#else
#define AUDIO_MUTE_TRIS (_TRISE7)
#define AUDIO_MUTE_LATCH (_LATE7)
#define AUDIO_MUTE_DELAY_MS 20
#endif

#if (BOARD_REV == 1)
#define MUDE_TRIS	(TRISBbits.TRISB10)
#define MUDE_IO		(PORTBbits.RB10)
#define MUDE_LATCH	_LATB10
#define SURX_POWER_ON	(_LATB10 = 1)
#define SURX_POWER_OFF	(_LATB10 = 0)
#endif

extern void InitSURXStream();
extern void InitSUTXStream();
extern void ProcessSURXStream();
extern void ProcessSUTXStream();

extern void vlf_SetMUID(U32 id);
extern void vlf_SetNewSearchSequence();
extern void vlf_SendDetectMsg(char antenna);
extern void vlf_SendMaskMsg(U32 id, char antenna);
//FULL_MASK
extern void vlf_SendFullMaskMsg(U32 id, U8 minutes, char antenna);
//FULL_MASK END
extern void vlf_SetTXAntenna(char antenna);
extern void vlf_SendLocateMsg(U32 id, U8 seconds, char antenna);

extern void set_SU_VLF_RX_Mode(char* pmode);
extern void Send_Clutter_request();
extern void Send_ZC_request();

void clear_RSSI_RX_ResponseCount();
unsigned int get_RSSI_RX_ResponseCount();

void clear_SU_ZC_RX_ResponseCount();
unsigned int get_ZC_RX_ResponseCount();

void clear_MU_ResponseCount();
unsigned int get_MU_ResponseCount();
void send_SU_TX_PING();
void send_SU_RX_PING();

void clear_MU_TX_PingCount();
unsigned int get_MU_TX_PingCount();

void clear_MU_RX_PingCount();
unsigned int get_MU_RX_PingCount();

void vlf_SendQuickSearchMsg(char antenna);
void send_SU_RX_ZC_Trigger_Level(int levelx, int levely, int levelz);
void send_SU_RX_ZC_Trigger();

extern unsigned int getPeakTestXYZ_1F();
extern unsigned int getPeakTestXYZ_2F();
extern void clear_PeakTestXYZ();

void Init_SU_Wireless_Stream();
void Process_SU_Wireless_Stream();

extern int g_SU_TX_battmvolts;

#endif //VLF_RXTX_H
