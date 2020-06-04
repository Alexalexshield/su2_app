//*****************************************************************************
// MineRadio IMSS SU2 system
// 
// 
//*****************************************************************************

#include "system.h"
#include "stdio.h"
#include "FSIO.h"
#include "trace.h"
#include "DEE Emulation 16-bit.h"
#include <string.h>
#include <math.h> 
#include <stdlib.h>
#include "sdfunctions.h"
#include "putfunctions.h"
#include "csv_packet.h"
#include "verterbi.h"
#include "su_slcd.h"
#include "button.h"
#include "vlf_rxtx.h"
#include "pc_cmds.h"
#include "iomapping.h"
#if (RTCC_SOURCE == RTCC_INTERNAL)	
#include "rtcc.h"
#elif (RTCC_SOURCE == RTCC_EXTERNAL)	
#include "i2c_ISL12024_rtc.h"
#endif
#include "button.h"
#include "wiinterface.h"

extern unsigned short ProcessBackgroundTasks();


// Setup configuration bits
//config setups
_CONFIG1( JTAGEN_OFF & GCP_OFF & GWRP_OFF & BKBUG_OFF & COE_OFF & FWDTEN_OFF & ICS_PGx2)
_CONFIG2( FNOSC_FRCPLL & OSCIOFNC_ON & IOL1WAY_OFF) //32 MHz FRC Clock with PLL enabled, no clock out
//_CONFIG2( FNOSC_FRCPLL ) //32 MHz FRC Clock with PLL enabled

#if (BOARD_REV == 2)
int g_lastport = PORT_WIRELESS;
long g_DestUnitID = BROADCAST_EUI;	// this is the default unit id to which we send messages
#endif

volatile SystemError g_SysError;
volatile UnitFlags g_UnitFlags;

void MicroPortInit();

MemProperties g_SDProperties = {0};
unsigned int g_lastbattMV;

unsigned int g_SecondsBetweenStatusLog = 10;
long g_elapsed_seconds=0l;

unsigned int g_Countdown;
BootStatus g_BootStatus;

#define MAIN_LED_OFF 0
#define MAIN_LED_ON 1
#define MAIN_LED_COUNTDOWN 2
#define MAIN_LED_DURATION 100
unsigned int g_led_period = (TICKSPERSECOND*2);	// HAS TO BE GREATER THAN LED_DURATION
unsigned char g_led_mode = MAIN_LED_ON;

extern int16 INTCON1_Trap_Register __attribute__ ((persistent));

//#define TEST_CALCULATIONS
#ifdef TEST_CALCULATIONS
extern void Test_Calculations();
#endif

int main(void)
{
	
//	test_verterbi();
#ifdef TEST_CALCULATIONS
	Test_Calculations();
#endif


		
	_RCDIV0 = 0;	//set FRC clock divider to 1 for max speed
			
	RCONbits.SWDTEN=0; // Disable Watch Dog Timer

#if PROCESSOR_MIPS == 16	
	CLKDIV = 0x0000; //do not divide
	OSCTUN=0; // Tune FRC oscillator, if FRC is used
	while(OSCCONbits.LOCK!=1) {}; // Wait for PLL to lock
#endif

	g_BootStatus.RCON = RCON;
	if ((RCONbits.POR == 0) && (RCONbits.BOR == 0)) {
		/* last_mode is valid */
	} else {
		INTCON1_Trap_Register = 0;		/* initialize persistent data */
	}
	
	g_BootStatus.INTCON1 = INTCON1_Trap_Register;	// Save the startup register so we know how we got here
	SPLIM -= 48;				// make sure there is enough room on the stack 5to handle trap errors

	RCON = 0;					// clear register for next time
	INTCON1bits.NSTDIS = 0;		// enable nested interrupts

	INTCON1_Trap_Register = INTCON1;	// reset the trap register for logging the status on the next reset	

	MicroPortInit();			// Initialize ports and io mapping
	
	g_SysError.value = 0;		// Clear the general system error flag

	TimerInit();				// Setup the ms-tick timer

#if (RTCC_SOURCE == RTCC_INTERNAL)	
    RTCCInit();					// Setup the RTCC and enable secondary oscillator
	RTCCEnableSecondAlarm();	// enable a 1 second tick interrupt
	g_seconds = RTCCGetTime_t();
#else
	ISL_Init();
	ISL_ReadTime(&g_seconds);
#endif	
	
	PortInit(PC_PORT, 115200l);			// Setup the UART for sending debug info to the PC
 	PortInit(SUTX_UART_PORT, 115200l);			// Setup the UART for sending debug info to the PC
//	PortInit(SURX_UART_PORT, 115200l);			// Setup the UART for sending debug info to the PC
	PortInit(SURX_UART_PORT, 57600l);			// Setup the UART for sending debug info to the PC

	DelayMsecs(1200);			// todo - POSSIBLY reduce it
   	

	DataEEInit();		// Initialize the Flash memory functions for reading and writing config data to memory
	DATA_EE_FLAGS initEEResult = dataEEFlags;
	
	init_unitflags();	// reset the control flags to the factory default 
    
	if (load_config(&g_Config)) {		// attempt to load config data from flash memory
		default_config(&g_Config);		// set to factory defaults if it fails
	}
	
    SLCD_Reset();		// reset the SLCD to talk to the LCD_PORT
    SU_Init();			// initialize the search structures
     
    ADCInit();			// Setup the ADC to read the battery voltage
	
	DelayMsecs(20);
	
//	g_UnitFlags.DebugMisc = 1;		// enable this for debugging
//	g_UnitFlags.DebugSLCD = 1;		// enable this for debugging
//	g_UnitFlags.DebugSD = 1;
//	g_UnitFlags.DebugButtons = 1;
	
	InitPCStream();			// initialize the PC csv packet handler
	InitSURXStream();		// initialize the SURX csv packet handler
	InitSUTXStream();		// initialize the SUTX csv packet handler

#if (BOARD_REV == 2)
	g_UnitFlags.WirelessEnabled = 1;	// set to 0 to disable
//	g_UnitFlags.WirelessEnabled = 0;	// set to 0 to disable
	setWiAddress(&g_destMACAddress, MINERADIO_SU_TX , g_DestUnitID);	// default to any gateway
	setWiAddress(&g_myMACAddress, MINERADIO_SU_CONTROLLER, g_Config.id);
	init_Wireless();		// for this to work, g_UnitFlags.WirelessEnabled must = 1;
	if (g_UnitFlags.WirelessEnabled)	// disable for testing
	{
   		g_lastport = PORT_WIRELESS;
   	}
   	Init_SU_Wireless_Stream(); 
   		
	if (g_SuSettings.enable_wireless)
	{
		SUTX_POWER_OFF;		// disable SU-TX RS485 module to save power
	}
	else
	{
		MRF24J40Sleep();	// PUT wireless into sleep mode
		SUTX_POWER_ON;		// enable the SU-TX RS485 module
	}
#else
	SUTX_POWER_ON;		// enable the SU-TX RS485 module
#endif	
	
	g_SysError.SDNoCard = 1;						// no card mounted yet
	g_SDProperties.MemType = MEMTYPE_UNKNOWN;		// indicate that we don't know what kind of SD memory it is yet
		
	PutStartupMsg(&g_BootStatus,PC_PORT);			// ALWAYS SEND A BOOT MESSAGE TO THE PC
	PutEepromErrorMsg("dataEEprom Init ",initEEResult,PC_PORT);
	PutConfigData(CONFIG_SOURCE_RAM,PC_PORT);		// send the current config data to PC
	PutUnitFlags(PC_PORT);							// send the current unit flags
	DelayMsecs(2);
	g_Countdown = g_SecondsBetweenStatusLog;		// set a counter to do some activity - could be a log entry
	g_lastbattMV = readBattMVolts();
	
	set_SU_VLF_RX_Mode(SU_RX_MODE_STANDBY);	// tell the MURX module to wait until it is told to decode

	SLCD_SendCmd("utf8 on");		// sets unicode ON

	SU_RunMainPage();
	
	return 0;
}




unsigned short ProcessBackgroundTasks()
{
	static unsigned long long lasttick = 0l;
	static 	time_t lastsec = 0;
	static long total_elapsedms = 0;
	static unsigned int button_counter = 0;
	unsigned short last_state = 0;
	long elapsedms;
	unsigned int elapsed_seconds;
	static unsigned int time_update = 0;
	static unsigned int led_counter = 0;
		
	CLRWDT();
	ProcessPCStream(PC_PORT);
	
	ProcessSURXStream();
	ProcessSUTXStream();
	
#if (BOARD_REV == 2)
	if (g_SuSettings.enable_wireless)
	{
		checkFor_WirelessMessages();		// process any pending wireless messages
		Process_SU_Wireless_Stream();		
	}
#endif

	if (g_tick != lasttick) // Decrement software countdowns to send test data and/or status packet
	{
		if (g_tick > lasttick)
			elapsedms = g_tick-lasttick;
		else
			elapsedms = 1;	// account for timer overflow
		
		total_elapsedms+= elapsedms;
		lasttick = g_tick;
		
		if (button_counter++ >= BUTTON_REFRESH_PERIOD_MS) 
		{
			button_counter = 0;
			last_state = ProcessButtonEvents();
			if (last_state != 0) {
				// any key turns on the lcd if necessary
			}		
			if (g_UnitFlags.DebugButtons) 
				PutButtonState(last_state, PC_PORT);
		}
		
		// flash LED according to led_period;
		switch (g_led_mode) 
		{
			case MAIN_LED_OFF:
				GREEN_LED_POWER_OFF;
				break;
			case MAIN_LED_ON:
				if (led_counter == 1) {
					GREEN_LED_POWER_ON;
				}
				else if (led_counter == MAIN_LED_DURATION) {
					GREEN_LED_POWER_OFF;
				}
				else if (led_counter >= g_led_period) {
					led_counter = 0;
				}
				led_counter++;
				break;
			default:
				break;
		}
	}
	

	if (lastsec != g_seconds) {
		elapsed_seconds = g_seconds-lastsec;
		lastsec = g_seconds;
		
		SLCD_IncPowerDownCounter();
		
		if (time_update++ >= 1) {		// read the real time clock every 30 seconds to currect tick counter
#if (RTCC_SOURCE == RTCC_INTERNAL)	
			g_seconds = RTCCGetTime_t();
#else
			ISL_ReadTime(&g_seconds);
#endif	
			time_update = 0;
		}	
		
		if (g_UnitFlags.SendTime)
			PutTimeStr(PC_PORT);
			
//		if (g_UnitFlags.DebugMisc) {
//			PortPutCRStr("SUTST",SUTX_UART_PORT,1);	// TODO: for testing
//			PortPutCRStr("SUTST",PC_PORT,1);	// TODO: for testing
//		}
		
    
		CheckForSDCardPresence();		// check to see if a card is inserted

		g_elapsed_seconds+= elapsed_seconds;
		if (g_Countdown > elapsed_seconds) {
			g_Countdown-= elapsed_seconds;
		}	
		else {	
			g_Countdown = g_SecondsBetweenStatusLog;

			g_lastbattMV = readBattMVolts();
			if (g_UnitFlags.SendBattMV)
				PutBatteryVoltage(g_lastbattMV, PC_PORT);
				// STORE FILE TO MEMORY
//			if ((g_UnitFlags.LogStatus) && (!g_SysError.SDNoCard)) {
//				SD_power(1); 
//		   		SD_OpenLogFile(PC_PORT);		// open a new file and increment file counter
//				SD_CloseLogFile(PC_PORT); // IF FILE HANDLE VALID THEN FINISH AND CLOSE THE FILE
//				SD_power(0);
//			}
		}
		
		ProcessPCStream(PC_PORT);
	}
	return last_state;
}
	


void MicroPortInit()
{
	//IO Mapping for SU2_BOARD 
    AD1PCFG = 0xffff;		// set up all AN1 pins to digital pins


////////////////////////////////// IO PIN MAPPING START ///////////////////////////////////////////////
    unlockIO();

	//UART 1 mapping for PC debugging **********************
	RPINR18bits.U1RXR = 30;			// Assign U1RX To Pin RP30
	RPOR8bits.RP16R = U1TX_IO;			// Assign U1TX (function #3 - U1TX_IO) To Pin RP16
	
	//UART 2 mapping for LCD **********************
	RPINR19bits.U2RXR = 20;			// Assign U2RX To Pin RP20 (coming from LCDTXD)
	RPOR12bits.RP25R = U2TX_IO;		// Assign U2TX (function #5 - U2TX_IO) To Pin RP25(output to LCDRXD)
	
	//UART 3 mapping for SU_TX module *********************
	RPINR17bits.U3RXR = 2;			// Assign U3RX To Pin RP2
	RPOR2bits.RP4R = U3TX_IO;		// Assign U1TX (function #28 - U3TX_IO) To Pin RP4
	
	//UART 4 mapping for SU_RX module *****************
	RPINR27bits.U4RXR = 8;			// Assign U4RX To Pin RP8 (coming from SU_RX)
	RPOR4bits.RP9R = U4TX_IO;		// Assign U4TX (function #30 - U4TX_IO) To Pin RP9
	
	// SPI1 MAPPING FOR SDCARD
	//SPI 1 mapping for SDCARD *********************
	RPOR14bits.RP29R = SCK1OUT_IO;		// Assign SCK1 (function #8) To Pin RP29
	RPOR7bits.RP14R = SDO1_IO;			// Assign SDO1 (function #7) To Pin RP14 
	RPINR20bits.SDI1R = 17;				// Assign SDI1R To Pin RP17
	
	//SPI 2 mapping for WIRELESS *********************
	RPOR10bits.RP21R = SCK2OUT_IO;		// Assign SCK2 (function #11) To Pin RP21 - RG6
	RPINR22bits.SDI2R = 26;				// Assign SDI2R To Pin RP26 - RG7	
	RPOR9bits.RP19R = SDO2_IO;			// Assign SDO2 (function #10) To Pin RP19 - RG8

	lockIO();

////////////////////////////////// IO PIN MAPPING END ///////////////////////////////////////////////

	_TRISC13 = 1;			// set SOSCI to input for RTCC module IN CASE WE EVER RUN FROM SECONDARY OSCILLATOR
	
	SUTX_POWER_OFF;
	SUDEM_TRIS = OUTPUT;	// set mutx module enable bit to output
	
#if (BOARD_REV == 1)
	SURX_POWER_ON;
	MUDE_TRIS = OUTPUT;		// set SU_RX module enable bit to output
#endif

	// The mute should be activated during the SU2 Transmit cycle so that it does not overwhelm the speakers/headphones.
	AUDIO_MUTE_LATCH = 0;	//normally low to enable the audio board and is high to mute the audio output.
	AUDIO_MUTE_TRIS = OUTPUT;
	
	////////////////////////////////////// LEDS /////////////////////////////////////////////
	BLUE_LED_POWER_OFF;
	GREEN_LED_POWER_OFF;
	RED_LED_POWER_OFF;
	
	BLUE_LED_TRIS = OUTPUT;
	GREEN_LED_TRIS = OUTPUT;
	RED_LED_TRIS = OUTPUT;
	
	ButtonInit();
}	

