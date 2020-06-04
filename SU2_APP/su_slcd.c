#include "su_slcd.h"
#include "trace.h"
#include <stdio.h>
#include <stdlib.h>
#include "string.h"
#include "adc.h"
#include "button.h"
#include "config.h"
#include "vlf_rxtx.h"
#include "sdfunctions.h"


// SU battery voltage is approximately 100% at 6.7V and 0% at 5V.
#define BATTERY_ZERO_LEVEL 5000
#define BATTERY_FULL_LEVEL 6650
#define BATTERY_RANGE (BATTERY_FULL_LEVEL-BATTERY_ZERO_LEVEL)
#define BATTERY_UPDATE_SECONDS 5

////////////////////////////////////// Some globals used by the display functions ///////////////////
MuData g_MuDeviceList[MAXIMUM_MU_DEVICES_IN_LIST];
long g_SLCDErrors = 0;
int g_SLCD_XOFF = FALSE;				// XOFF character used to control data flow to LCD

//FULL_MASK
MuFullMaskData g_MuFullMaskDeviceList[MAXIMUM_MU_FULLMASK_DEVICES_IN_LIST];
//FULL_MASK END

/////////////////////////////////// extern globals /////////////////////////////////////////
extern BOOL g_bSearchPaused;
extern BOOL g_bSearching;
extern int g_iSelectedMuIndex;
extern void SU_SettingDefaults();

/////////////////////////////////////// private prototypes /////////////////////////////////////
void SLCD_DisplayPower(BOOL bOn);
void DisplayMainMenuItem(int buttonix, int selected);

extern void SU_MainLogPage();
void SU_DataReset();


void SLCD_Reset()
{
	time_t ref_tick;
	PortInit(LCD_PORT, 115200l);		// Setup the UART for LCD AT 115K baud
	DelayMsecs(5);
	PortPutCRStr("Initializing SLCD...:",PC_PORT,1);

	PortPutChar(27,LCD_PORT);			// Send an ESC character to break out of any macros	
	// send 3 carriage returns to get it to talk to us 
	// since a reset will revert it back to the default port
	PortPutChar('\r',LCD_PORT);		
	if (g_UnitFlags.DebugSLCD) 
		PortPutStr("CR to SLCD ",PC_PORT,1);
	DelayMsecs(1);
	
	PortPutChar('\r',LCD_PORT);		
	if (g_UnitFlags.DebugSLCD) 
		PortPutStr("CR to SLCD ",PC_PORT,1);
	DelayMsecs(1);

	PortPutChar('\r',LCD_PORT);		
	if (g_UnitFlags.DebugSLCD) 
		PortPutStr("CR to SLCD ",PC_PORT,1);
	DelayMsecs(1);

	// send carriage returns until we get an answer
//	do {
//		PortPutChar('\r',LCD_PORT);		
//		if (g_UnitFlags.DebugSLCD) 
//			PortPutStr("CR to SLCD:\\r",PC_PORT,1);
//		DelayMsecs(10);
//	}
//	while (GetNextLCDSentence() == NULL);

	ref_tick = g_seconds;
	char *response;
	// send carriage returns until we get an answer or 20 seconds whichever comes first
	do 
	{
		PortPutChar('\r',LCD_PORT);		
//		if (g_UnitFlags.DebugSLCD) 
		if ((g_seconds%2)==0)
			PortPutCRStr("CR to SLCD ",PC_PORT,1);
		response = GetNextLCDSentence();
		DelayMsecs(10);
	}
	while ((response == NULL) && ((g_seconds - ref_tick) < 20));
	
	if (response == NULL)
		PortPutCRStr("!!!!!!!!!!!!!!! Error initializing SLCD !!!!!!!!!",PC_PORT,1);
		
    SLCD_SendCmd(CLEARSCREEN);	// clear screen
//	SLCD_SendCmd("*RESET");
//	DelayMsecs(500);
//	DelayMsecs(500);
//	DelayMsecs(500);
//	DelayMsecs(500);
//	DelayMsecs(500);
	
	while (GetNextLCDSentence()!=NULL);	// flush buffer

	SLCD_DisplayPower(TRUE);	// TURN ON THE DISPLAY
	
//	SetBeepVolume(g_SuSettings.BeepVolume);
	
//	SLCD_SendCmd("*RT");	// RESETS CALIBRATION TO INTERNAL DEFAULT
//	while (GetNextLCDSentence() == NULL);	// wait for a response
//	SLCD_SendCmd("tc");		// runs through the touch screen calibration routine
//	while (GetNextLCDSentence()== NULL);	// flush buffer

}


static unsigned int LCD_lowpower_counter = 0;
static char m_lcd_display_on = TRUE;
void SLCD_DisplayPower(BOOL bOn)
{
	if (bOn) {
		SLCD_SendCmd("v on");
		LCD_lowpower_counter = 0;
	}
	else {
		SLCD_SendCmd("v off");
	}
	m_lcd_display_on = bOn;
}


void SetBeepVolume(int volume)
{
	char buffer[10];
	sprintf(buffer,"bv %d",volume);
	SLCD_SendCmd(buffer);	
}



void SetForeBackGndColor(int fore, int back)
{
	char buffer[20];
	sprintf(buffer,"s %d %d",fore, back); // set foreground to Black, background to color
	SLCD_SendCmd(buffer);
}

void SetDefaultBackForegnd()
{
	SetForeBackGndColor(SLCD_FOREGROUND,SLCD_BACKGROUND);	
}

void SU_Init()
{
	if (load_non_volotile_settings(&g_SuSettings)) {		// attempt to load settings from non-volatile memory
		default_settings(&g_SuSettings);					// set to factory defaults if it fails
	}

	LCD_lowpower_counter = 0;
	SLCD_DisplayPower(TRUE);	// TURN ON THE DISPLAY
}

void SLCD_IncPowerDownCounter()
{
	// Turn off LCD after no buttons pressed for a while
	if (g_UnitFlags.AutoLCDPowerOff) {
		if (LCD_lowpower_counter < g_SuSettings.screen_power_off_seconds) {
			LCD_lowpower_counter++;
			if (LCD_lowpower_counter >= g_SuSettings.screen_power_off_seconds) {
				SLCD_DisplayPower(FALSE);
			}	
		}
	}
	else {
		LCD_lowpower_counter = 0;
	}
}


// Sends a command to the LCD and waits for a response
void SLCD_SendCmd(char *buf)
{
	int i;
	time_t ref_tick;
	
	if (g_SLCD_XOFF == TRUE) {
		ref_tick = g_seconds;
		int seconds = 0;
		while (g_SLCD_XOFF == TRUE) {			// MAKE SURE WE DON'T GET STUCK WITH THE XOFF SET FOR MORE THAN 10 SECONDS
			if (ref_tick != g_seconds) {
				ref_tick = g_seconds;
				seconds++;
			}
			if (seconds >= 5) {
				g_SLCD_XOFF = FALSE;
			}
		}
	}

	for (i=0;i<strlen(buf);i++) {
		PortPutChar(buf[i],LCD_PORT);
	}
	PortPutChar('\r',LCD_PORT);		// terminate the command

	if (g_UnitFlags.DebugSLCD) 
	{
		PortPutStr("to SLCD:",PC_PORT,1);
		if (strlen(buf) == 0)
			PortPutStr("\\r",PC_PORT,1);
		else
			PortPutCRStr(buf,PC_PORT,1);
	}

	ref_tick = g_seconds;
	char *response;
	// wait for up to 20 ms for a response - no more to prevent a blocking situation
	do 
	{
		response = GetNextLCDSentence();
	}
	while ((response == NULL) && ((g_seconds - ref_tick) < 20));

	struct button btn;
	if (response != NULL) {
		SU_GetResponse(response,&btn);	// call function to enable debug on response
	}
}

// resets a device data struct to an uninitialized default state
void SU_DataResetUnit(MuData* p)
{
	p->v.isvalid = 0;
	p->packet_count = 0;
	p->id_u32 = 0;
}

XYZ_Reading g_zero_crossings;
XYZ_Reading g_last_Clutter_rssi;
XYZ_Reading g_peak_Clutter_rssi;
XYZ_Reading g_last_Packet_rssi;
XYZ_Reading g_peak_Packet_rssi;

void Reset_XYZ_Reading(XYZ_Reading* p)
{
	p->x = 0;
	p->y = 0;
	p->z = 0;
}

void ResetLastRssiReadings()
{
	Reset_XYZ_Reading(&g_last_Clutter_rssi);
	Reset_XYZ_Reading(&g_last_Packet_rssi);
}

void ResetPeakRssiReadings()
{
	Reset_XYZ_Reading(&g_peak_Clutter_rssi);
	Reset_XYZ_Reading(&g_peak_Packet_rssi);
}

// reset the entire list of MU devices to the uninitialized state
void SU_DataReset()
{
	int i;
	for (i=0;i<MAXIMUM_MU_DEVICES_IN_LIST;i++) {
		SU_DataResetUnit(&g_MuDeviceList[i]);
	}
	
	// clear any rssi and zero crossing counters
	Reset_XYZ_Reading(&g_zero_crossings);
	ResetPeakRssiReadings();
	ResetLastRssiReadings();
}

// reset the mobility in the entire list of MU devices to the uninitialized state
void SU_DataClearMobility()
{
	int i;
	for (i=0;i<MAXIMUM_MU_DEVICES_IN_LIST;i++) {
		g_MuDeviceList[i].v.Mobility_valid = 0;
	}
}

// converts delta RSSI to a log power value from 0 to 100 to match the bar graph
/*
int ScaleLogPower(int power)
{
//	return power/10;
	if (power > 1) {
		float value = 33.0 * log10((float)power);
		return (int)value;
	}
	return 0;
}
*/
// converts delta RSSI to a linear power value from 0 to maxvalue to match the bar graph
//#define ATODREADING_MAX 32768

int ScaleLogPowerToMax(int power, int maxvalue, long range)
{
	if (power > 1) {
		float value = (maxvalue/(log10((double)range))) * log10((float)power);
		return (int)value;
	}
	return 0;
}

#ifdef DISTANCE_ONLY_METHOD

static int m_peak_power_1F = 0;
static int m_peak_power_2F = 0;
void clear_PeakPower() { m_peak_power_1F = 0; m_peak_power_2F = 0;}; // clear peak power for new locate.
void UpdatePeakPower_1F(int value)
{
	if (value > m_peak_power_1F)
		m_peak_power_1F = value;
}
void UpdatePeakPower_2F(int value)
{
	if (value > m_peak_power_2F)
		m_peak_power_2F = value;
}

int getPeakPower_1F() { return m_peak_power_1F; };
int getPeakPower_2F() { return m_peak_power_2F; };
#else
void UpdatePeakPower_1F(int value) { };
#endif


// counts the number of valid entries which are always appended to the start of the list
int GetMUListSize()
{
	int i;
	for (i=0;i<MAXIMUM_MU_DEVICES_IN_LIST;i++) {
		if (!g_MuDeviceList[i].v.ID_valid) 
			break;
	}
	return i;
}

MuData* GetMUListEntryByID(U32 id)
{
	int i;
	for (i=0;i<MAXIMUM_MU_DEVICES_IN_LIST;i++) 
	{
		if ((g_MuDeviceList[i].v.ID_valid) && (g_MuDeviceList[i].id_u32 == id))
		{
			return &g_MuDeviceList[i];
		}
	}	
	return NULL;
}

int GetMUEntryIndexByID(U32 id)
{
	int i;
	for (i=0;i<MAXIMUM_MU_DEVICES_IN_LIST;i++) 
	{
		if ((g_MuDeviceList[i].v.ID_valid) && (g_MuDeviceList[i].id_u32 == id))
		{
			return i;
		}
	}	
	return -1;
}

//FULL_MASK
// counts the number of valid entries which are always appended to the start of the list
int GetMUFullMaskListSize()
{
	int i;
	for (i=0;i<MAXIMUM_MU_FULLMASK_DEVICES_IN_LIST;i++) {
		if (g_MuFullMaskDeviceList[i].id_u32 == 0L)
			break;
	}
	return i;
}

int GetFullMaskList_IndexByID(U32 id)
{
	int i;
	for (i=0;i<MAXIMUM_MU_FULLMASK_DEVICES_IN_LIST;i++) {
		if (g_MuFullMaskDeviceList[i].id_u32 == id)
			return i;
	}
	return -1;
}

void SU_FullMaskTableReset()
{
	memset(g_MuFullMaskDeviceList,0,sizeof(g_MuFullMaskDeviceList));
}

//FULL_MASK END
#define DISPLAY_FULLMASK_COUNTDOWN_AS_TIME

void SU_UpdateTime(int x, int y)
{
	#ifdef DISPLAY_FULLMASK_COUNTDOWN_AS_TIME
		static time_t last_time = 0;
	
		time_t ref = g_seconds;
		char buffer[60];
			
		if (last_time != ref) 
		{
			int countdown = SU_FullMaskList_GetShortestCountdownSeconds();
			if (countdown == 0)	// at least one countdown has expired
			{
				#ifdef RUSSIAN
					sprintf(buffer,"t \"00:00      \" %d %d",x,y);	
				#else
					sprintf(buffer,"t \"FM=Timeout!      \" %d %d",x,y);	
				#endif
			}
			else if (countdown < MU_FULL_MASK_SECONDS)	// countdown in effect
			{
				#ifdef RUSSIAN
					sprintf(buffer,"t \"%02d:%02d      \" %d %d",countdown/60,countdown%60,x,y);	
				#else
					sprintf(buffer,"t \"FM=%02d:%02d        \" %d %d",countdown/60,countdown%60,x,y);	
				#endif	

			}
			else	// no devices are fully masked and therefore no countdowns are currently in effect
			{
				sprintf(buffer,"t \"                       \" %d %d",x,y);	
			}
			#ifdef INCLUDE_MINERADIO_HEADER
				SET_TO_MINERADIO_BACKGROUND;
			#else
				SetDefaultBackForegnd();
			#endif		
			SET_DEFAULT_HEADER_FONT;
			SLCD_SendCmd(buffer);
			last_time = ref;
		}
	
	#else
		return;
		static time_t last_time = 0;
	
		time_t ref = g_seconds;
		char buffer[60];
		char szTime[40];
		
	
		if (last_time != ref) {
			strftime(szTime, sizeof(szTime), "%b %d %y %H:%M:%S ", gmtime((const time_t * )&g_seconds));
	//		strftime(szTime,"%b %d %Y %H:%M:%S ",ref);
	#ifdef INCLUDE_MINERADIO_HEADER
			SET_TO_MINERADIO_BACKGROUND;
	#else
			SetDefaultBackForegnd();
	#endif		
			SET_DEFAULT_HEADER_FONT;
			sprintf(buffer,"t \"%s\" %d %d",szTime,x,y);
			SLCD_SendCmd(buffer);
			last_time = ref;
		}
	#endif	
}


void SU_UpdateStatus(BOOL bForcedUpdate)
{
	return;
	static time_t last_time = 0;
	char buffer[60];
	char szStatus[40];
	szStatus[0] = 0;

	time_t ref = g_seconds;

	if ( bForcedUpdate || (labs(ref - last_time) > 10)) {	// update every 10 seconds
		last_time = ref;
		SD_GetStatusMessage(szStatus);
#ifdef INCLUDE_MINERADIO_HEADER
		SET_TO_MINERADIO_BACKGROUND;
#else
		SetDefaultBackForegnd();
#endif		
		SET_DEFAULT_HEADER_FONT;
		sprintf(buffer,"t \"%s\" %d %d",szStatus,STATUS_XPOS,STATUS_YPOS);
		SLCD_SendCmd(buffer);
		last_time = ref;
	}
}

void SU_UpdateCOMStatus(BOOL bTest)
{
	char buffer[80];
//	char szStatus[40];
//	szStatus[0] = 0;
#ifdef RUSSIAN
	char* txTitle = g_SuSettings.enable_wireless?"\xD0\xBF\xD0\xB5\xD1\x80\xD0\xB5\xD0\xB4\xD0\xB0\xD1\x82\xD1\x87\xD0\xB8\xD0\xBA":"\xD0\xBF\xD0\xB5\xD1\x80\xD0\xB5\xD0\xB4\xD0\xB0\xD1\x82\xD1\x87\xD0\xB8\xD0\xBA";
#else
	char* txTitle = g_SuSettings.enable_wireless?"TW":"TC";
#endif

	unsigned int tx_ping_count = get_MU_TX_PingCount();
//	unsigned int rx_ping_count = get_MU_RX_PingCount();

#ifdef INCLUDE_MINERADIO_HEADER
		SET_TO_MINERADIO_BACKGROUND;
#else
		SetDefaultBackForegnd();
#endif		

	SET_DEFAULT_HEADER_FONT;

/*	if (tx_ping_count)
		strcat(szStatus,"Tx=Ok ");
	else
		strcat(szStatus,"Tx=?? ");	

	if (rx_ping_count)
		strcat(szStatus,"Rx=Ok");
	else
		strcat(szStatus,"Rx=??");;
	
	strcat(szStatus,"    ");	// MAKE SURE TRAILING CHARACTERS ARE ERASED
	
	sprintf(buffer,"t \"%s\" %d %d",szStatus,COM_STATUS_XPOS,COM_STATUS_YPOS);
	SLCD_SendCmd(buffer);
*/	
	BOOL bBattOk = (g_SU_TX_battmvolts > BATTERY_ZERO_LEVEL)?TRUE:FALSE;
	
	if (tx_ping_count) 
	{
		SLCD_SendCmd("tfx 5");	// deletes flashing
		sprintf(buffer,"t \"%s=Ok\" %d %d N", txTitle,TX_STATUS_XPOS,TX_STATUS_YTEXTPOS);	// draw ok message
		SLCD_SendCmd(buffer);
	}
	else
	{
		SLCD_SendCmd("tfx 5");	// deletes any existing flashing
#ifdef RUSSIAN	
		sprintf(buffer,"t \"                              \" %d %d N", TX_STATUS_XPOS,TX_STATUS_YTEXTPOS);	// ERASE OLD LINE
#else
		sprintf(buffer,"t \"              \" %d %d N", TX_STATUS_XPOS,TX_STATUS_YTEXTPOS);	// ERASE OLD LINE
#endif
		SLCD_SendCmd(buffer);
		sprintf(buffer,"tf 5 500 \"%s= ??\" %d %d T", txTitle, TX_STATUS_XPOS,TX_STATUS_YTEXTPOS);	
		SLCD_SendCmd(buffer);
	}
	
/*	if ((rx_ping_count))
	{
		SLCD_SendCmd("tfx 6");	// deletes flashing
		sprintf(buffer,"t \"Rx=Ok \" %d %d", TX_STATUS_XPOS+60,TX_STATUS_YPOS);	// ERASE OLD LINE
		SLCD_SendCmd(buffer);
	}
	else
	{
//		sprintf(buffer,"t \"%20s\" %d %d", " ", SEARCH_PAGE_STATUS_XPOS,SEARCH_PAGE_STATUS_YPOS);	// ERASE OLD LINE
//		SLCD_SendCmd(buffer);
		sprintf(buffer,"tf 6 500 \"Rx=?? \" %d %d", TX_STATUS_XPOS+60,TX_STATUS_YPOS);	
		SLCD_SendCmd(buffer);
	}
*/
	SU_UpdateTXBatteryLevel(TRUE,(bTest?FALSE:TRUE));
}

// Draws the battery bar and initializes the level to 0
// SU battery voltage is approximately 100% at 6.4V and 0% at 5V.
void SU_InitBatteryBar( unsigned char index, int x, int y )
{
	char buf[80];
	y+=2;

//Example: ld 0 10 10 30 200 0 0 1 888 99 F00 50 FF0 40 0F0
//Defines a levelbar in the rectangular area (10,10), (30,200). 
//Levelbar is vertical with the lowest value at the bottom; minimum visible value of 1, 
//with background color gray (888). Three color bands are defined: 
//red (F00) from 99 to 51, yellow (FF0) from 50 to 41, and green (0F0) from 40 to 1.

//	sprintf(buf,"ld %d %d %d %d %d 1 0 1 888 100 0F0 70 FF0 40 F00",index,x,y,x+BATTERY_BAR_WIDTH,y+BATTERY_BAR_HEIGHT);
	sprintf(buf,"ld %d %d %d %d %d 1 0 1 888 100 0F0",index,x,y,x+BATTERY_BAR_WIDTH,y+BATTERY_BAR_HEIGHT);
	SLCD_SendCmd(buf);
	SLCD_SendCmd("p 2");
	sprintf(buf,"l %d %d %d %d",x,y+BATTERY_BAR_HEIGHT+1,x+BATTERY_BAR_WIDTH+1,y+BATTERY_BAR_HEIGHT+1);
	SLCD_SendCmd(buf);
	sprintf(buf,"l %d %d %d %d",x+BATTERY_BAR_WIDTH+1,y,x+BATTERY_BAR_WIDTH+1,y+BATTERY_BAR_HEIGHT+1);
	SLCD_SendCmd(buf);
	sprintf(buf,"l %d %d %d %d",x+BATTERY_BAR_WIDTH+2,y+(BATTERY_BAR_HEIGHT/2)-3,x+BATTERY_BAR_WIDTH+2,y+(BATTERY_BAR_HEIGHT/2)+3);
	SLCD_SendCmd(buf);
	sprintf(buf,"l %d %d %d %d",x+BATTERY_BAR_WIDTH+3,y+(BATTERY_BAR_HEIGHT/2)-2,x+BATTERY_BAR_WIDTH+3,y+(BATTERY_BAR_HEIGHT/2)+2);
	SLCD_SendCmd(buf);
	sprintf(buf,"lv %d 0", index);
	SLCD_SendCmd(buf);
}
//#define DRAW_BATTERY_TEXT

void SU_InitMainBatteryBar()
{
	SU_InitBatteryBar( SU_BATTERY_LEVEL_INDEX, SU_MAIN_BATTERY_XPOS,SU_MAIN_BATTERY_YPOS);
}

void SU_InitTXBatteryBar()
{
	SU_InitBatteryBar( SUTX_BATTERY_LEVEL_INDEX, SU_MAIN_BATTERY_XPOS,TX_STATUS_YPOS);
}

#define FOOTER_FONT_HEIGHT 19
void SLCD_WriteFooter(char* f1, char* f2, char *f3)
{
	char buffer[128];
	int x =2;
	int y = SLCD_MAX_Y - (3*FOOTER_FONT_HEIGHT);
	
	#ifdef INCLUDE_MINERADIO_HEADER
		SET_TO_MINERADIO_BACKGROUND;
	#else
		SetDefaultBackForegnd();
	#endif

	SET_DEFAULT_FOOTER_FONT;
	
	if (strlen(f1) > 0)
	{
		sprintf(buffer,"t \"%s\" %d %d N",f1, x,y);
		SLCD_SendCmd(buffer);
	}

	if (strlen(f2) > 0)
	{
		sprintf(buffer,"t \"%s\" %d %d N",f2,x,y+FOOTER_FONT_HEIGHT);
		SLCD_SendCmd(buffer);
	}
	if (strlen(f3) > 0)
	{
		sprintf(buffer,"t \"%s\" %d %d N",f3,x,y+FOOTER_FONT_HEIGHT*2);
		SLCD_SendCmd(buffer);
	}
}

void SLCD_WriteFooter4(char* f1, char* f2, char *f3,char *f4)
{
	char buffer[128];
	int x =2;
	int y = SLCD_MAX_Y - (4*FOOTER_FONT_HEIGHT);
	
	#ifdef INCLUDE_MINERADIO_HEADER
		SET_TO_MINERADIO_BACKGROUND;
	#else
		SetDefaultBackForegnd();
	#endif

	SET_DEFAULT_FOOTER_FONT;
	
	if (strlen(f1) > 0)
	{
		sprintf(buffer,"t \"%s\" %d %d N",f1, x,y);
		SLCD_SendCmd(buffer);
	}

	if (strlen(f2) > 0)
	{
		sprintf(buffer,"t \"%s\" %d %d N",f2,x,y+FOOTER_FONT_HEIGHT);
		SLCD_SendCmd(buffer);
	}
	if (strlen(f3) > 0)
	{
		sprintf(buffer,"t \"%s\" %d %d N",f3,x,y+FOOTER_FONT_HEIGHT*2);
		SLCD_SendCmd(buffer);
	}
	if (strlen(f4) > 0)
	{
		sprintf(buffer,"t \"%s\" %d %d N",f4,x,y+FOOTER_FONT_HEIGHT*3);
		SLCD_SendCmd(buffer);
	}
}




void SU_UpdateBattery(int barix, unsigned int mvolts, int x, int y, BOOL bForcedUpdate, BOOL bPercentage)
{
	static time_t last_time = 0;
	int pcent;
	unsigned long mv;
	unsigned long elapsed_seconds = labs(g_seconds - last_time);

	time_t ref = g_seconds;
	char buffer[50];

	if ( bForcedUpdate || (elapsed_seconds > BATTERY_UPDATE_SECONDS)) {	// update every N seconds
#ifdef INCLUDE_MINERADIO_HEADER
		SLCD_SendCmd("S FFFFFF 8B2356");
#else
		SetDefaultBackForegnd();
#endif		
		SET_DEFAULT_HEADER_FONT;
		mv = mvolts;		// readBattMVolts();
		if (mv <= BATTERY_ZERO_LEVEL)
			pcent = 0;
		else if (mv >= (BATTERY_FULL_LEVEL-50))
			pcent = 99;		// PEAK AT 99 JUST TO PREVENT USE OF 3 CHARACTERS FOR 100	
		else {
			pcent = ((((mv - BATTERY_ZERO_LEVEL + 50) * 100))/(BATTERY_RANGE)); 	// BATTERY_PERCENTAGE(mv);
		}
		//pcent = (int)(ref%100);
#ifdef DRAW_BATTERY_TEXT
		sprintf(buffer,"t \"B=%02d%%\" %d %d",pcent,x,y);
		if (level < 80) {
			SLCD_SendCmd("s 5 1");		// set foreground to Black = 0 and background to Red = 5
		}
		else {
			SetDefaultBackForegnd();		
		}
		SLCD_SendCmd(buffer);
		SetDefaultBackForegnd();		
#else
		sprintf(buffer,"lv %d %d",barix,pcent);
		SLCD_SendCmd(buffer);
		
		if (bPercentage)
		{
			sprintf(buffer,"t \"%02d%% \" %d %d",pcent,x + BATTERY_BAR_WIDTH + 8,y);
			SLCD_SendCmd(buffer);
		}
		else	
		{
			sprintf(buffer,"t \"%1.1fV\" %d %d",(double)((mvolts+50.0)/1000.0),x + BATTERY_BAR_WIDTH + 8,y);
			SLCD_SendCmd(buffer);			
		}
#endif
		last_time = ref;
	}
}

void SU_UpdateBatteryLevel(BOOL bForcedUpdate, BOOL bPercentage)
{
	SU_UpdateBattery(SU_BATTERY_LEVEL_INDEX, g_lastbattMV, SU_MAIN_BATTERY_XPOS, SU_MAIN_BATTERY_YPOS, bForcedUpdate, bPercentage);
}

void SU_UpdateTXBatteryLevel(BOOL bForcedUpdate, BOOL bPercentage)
{
	SU_UpdateBattery(SUTX_BATTERY_LEVEL_INDEX, g_SU_TX_battmvolts, SU_MAIN_BATTERY_XPOS, TX_STATUS_YPOS, bForcedUpdate, bPercentage);
}


void SetScreenBrightness(int value)
{
	char buffer[30];
	sprintf(buffer,"xbbs %d",value);	// sets background level of screen
	SLCD_SendCmd(buffer);			
}


void SU_UpdateGeneralStatus(BOOL bIncludeTime, BOOL bForceNow)
{
	if (bIncludeTime)
		SU_UpdateTime(SU_MAIN_TIME_XPOS,SU_MAIN_TIME_YPOS);
	SU_UpdateBatteryLevel(bForceNow, TRUE);
	SU_UpdateStatus(bForceNow);
}

void StartPingStatus()
{
	clear_MU_TX_PingCount();
	clear_MU_RX_PingCount();
	send_SU_TX_PING();		// send a ping to check for connections
	send_SU_RX_PING();
}


#define FIX_BUG_IN_MAPPING



int mapButtons(unsigned short status)
{
	int index = 0;
#if (KEYBOARD_BUTTONS == 6)
	// if no SLCD buttons pressed then check for other buttons
		if (status & BUTTON_ENTER)
			 index = SU_BTN_ENTER;
		else if ((status & BUTTON_DOWN) && (status & BUTTON_UP))
			 index = SU_BTN_UPDOWN;
		else if ((status & BUTTON_F1) && (status & BUTTON_F2))
			 index = SU_BTN_F1F2;
		else if ((status & BUTTON_F1_2) && (status & BUTTON_F2))		// added to work around bug on one of the prototype boards
			 index = SU_BTN_F1F2;
		else if (status & BUTTON_DOWN)
			 index = SU_BTN_DOWN;
		else if (status & BUTTON_UP)
			 index = SU_BTN_UP;
		else if (status & BUTTON_EXIT)
			 index = SU_BTN_EXIT;
		else if (status & BUTTON_F1)
			index = SU_BTN_F1;
		else if (status & BUTTON_F2)
			index = SU_BTN_F2;
		else if (status & BUTTON_ENTER_2)		// added to work around bug on one of the prototype boards
			index = SU_BTN_ENTER;
		else if (status & BUTTON_F1_2)			// added to work around bug on one of the prototype boards
			index = SU_BTN_F1;
#endif
	
#if (KEYBOARD_BUTTONS == 16)
	// if no SLCD buttons pressed then check for other buttons
	{
		if (status & BUTTON_ENTER)
			 index = SU_BTN_ENTER;
		else if ((status & BUTTON_DOWN) && (status & BUTTON_UP))
			 index = SU_BTN_UPDOWN;
		else if ((status & BUTTON_F1) && (status & BUTTON_F2))
			 index = SU_BTN_F1F2;
		else if (status & BUTTON_DOWN)
			 index = SU_BTN_DOWN;
		else if (status & BUTTON_UP)
			 index = SU_BTN_UP;
		else if (status & BUTTON_EXIT)
			 index = SU_BTN_EXIT;
		else if (status & BUTTON_F1)
			index = SU_BTN_F1;
		else if (status & BUTTON_F2)
			index = SU_BTN_F2;
		else if (status & BUTTON_SEARCH)
			index = SU_BTN_NORMAL_SEARCH;
		else if (status & BUTTON_LOCATE)
			index = SU_BTN_LOCATE;
		else if (status & BUTTON_SETTINGS)
			index = SU_BTN_SETTINGS;
		else if (status & BUTTON_PAUSE)
			index = SU_BTN_PAUSE;
		else if (status & BUTTON_LOG)
			index = SU_BTN_LOG;
		else if (status & BUTTON_TEST)
			index = SU_BTN_TEST;
		else if (status & BUTTON_SPARE2)
			index = SU_BTN_SPARE2;
		else if (status & BUTTON_QUICK_SEARCH)
			index = SU_BTN_QUICK_SEARCH;
	}
#endif	
	return index;
}

struct button GetUserAction(unsigned short status)
{
	char* response;
	struct button btn;
	btn.index = 0;
	response = GetNextLCDSentence();
	if (response != NULL) {
		SU_GetResponse(response,&btn);
	}

	// if no SLCD buttons pressed then check for other buttons
	if ((btn.index == 0) && (status)) 
	{
		btn.index = mapButtons(status);
		if (status & BEEPABLE_BUTTONS)
			SLCD_SendCmd("beep 100");
	}

	if (btn.index != 0) {
		if (m_lcd_display_on == FALSE) {	
			SLCD_DisplayPower(TRUE);	// ANY KEY TURNS ON THE DISPLAY
			btn.index = 0;				// absorb first key if display is off
		}
		LCD_lowpower_counter = 0;
	}
	return btn;
}


void SU_DisplayStandardNavigationButtons(int x, int y, int width, BOOL bWithExit)
{
	return;
// 	char buffer[64];
//	sprintf(buffer,"bdc %d %d %d 1 \"%s\" %d %d",SU_BTN_ENTER,x,y,"*",NAVIGATION_BTN_UP_BMP,NAVIGATION_BTN_DN_BMP);
//	SLCD_SendCmd(buffer);	
//
//	x += width;
//	sprintf(buffer,"bdc %d %d %d 30 \"%s\" %d %d",SU_BTN_UP,x,y,"U",NAVIGATION_BTN_UP_BMP,NAVIGATION_BTN_DN_BMP);
//	SLCD_SendCmd(buffer);	
//
//	x += width;
//	sprintf(buffer,"bdc %d %d %d 30 \"%s\" %d %d",SU_BTN_DOWN,x,y,"D",NAVIGATION_BTN_UP_BMP,NAVIGATION_BTN_DN_BMP);
//	SLCD_SendCmd(buffer);	
//
//	if (bWithExit) {
//		x += width;
//		sprintf(buffer,"bdc %d %d %d 1 \"%s\" %d %d",SU_BTN_EXIT,x,y,"X",NAVIGATION_BTN_UP_BMP,NAVIGATION_BTN_DN_BMP);
//		SLCD_SendCmd(buffer);	
//	}
}





// 0 = nothing of interest
// 1 = button pressed
int SU_GetResponse(char* buf, struct button* btn)
{
	int i;

	switch (buf[0])   {
	 // command prompt
	 case '>':
		if( buf[1] == '\r' )   // end of string
			return 0;
		break;

	 // momentary button or hotspot press
	 case 'x':
		i = atoi(&buf[1]);
		if( i > 255 )
		{
		    traceS2("Bad rx string",buf);
			return(0);
		}
		btn->index = i;
		return 1;
		break;

	 case 'l':		 // slider level
		i = atoi(&buf[1]);
		if( i > 255 )
		{
	       traceS2("Bad rx string",buf);
		   return(0);
		}
		btn->index = i;
		btn->state = atoi(&buf[5]);
		return(1);
		break;
	 case 's':		 // latching button press
		i = atoi(&buf[1]);
		if( i > 255 )
		{
	       traceS2("Bad rx string",buf);
		   return(0);
		}
		btn->index = i/10;
		btn->state = i%10;
		return(1);
		break;

	 case '!':
		g_SLCDErrors++;
 	    traceS("Command sent to SLCD generated a syntax error");
		break;

	 case '^':
		g_SLCDErrors++;
  	    traceS("SLCD input buffer overflow");
		break;

	 case '?':
		g_SLCDErrors++;
   	    traceS("SLCD input framing error");
		break;

	 case ':':
   	    traceS2("SLCD text response:",&buf[1]);
		break;

	 case XON:
		 g_SLCD_XOFF = FALSE;
   	     traceS("Got XON character");
		 break;
	 case XOFF:
		 g_SLCD_XOFF = TRUE;
   	     traceS("Got XOFF character");
		 break;

	 default:
   	    traceS2("SLCD response:",&buf[0]);
		break;

  }
   return(0);
}






static char lcd_sentence[MAX_SLCD_RESPONSE];
static char lcd_count = 0;
static unsigned long lcd_bytesreceived = 0l;

// parses the LCD RX queue looking for sentences ending with a \r 
// if completed, returns a pointer to the sentence and resets itself to build another sentence
// if no sentence is ready, it returns NULL
char* GetNextLCDSentence()
{
	unsigned char rxbyte;
	
	while (PortChReady(LCD_PORT)) {
		lcd_bytesreceived++;
		rxbyte = PortGetCh(LCD_PORT);
		if (rxbyte != LCD_FRAMECHAR) {
			lcd_sentence[lcd_count%MAX_SLCD_RESPONSE] = rxbyte;	// MOD PREVENTS OVERFLOWING THE BUFFER BY WRAPPING IT AROUND
			lcd_count++;
		}
		else {
			lcd_sentence[lcd_count%MAX_SLCD_RESPONSE] = rxbyte;	// MOD PREVENTS OVERFLOWING THE BUFFER BY WRAPPING IT AROUND
			lcd_count++;
			lcd_sentence[lcd_count%MAX_SLCD_RESPONSE] = 0;	// MOD PREVENTS OVERFLOWING THE BUFFER BY WRAPPING IT AROUND
			lcd_count = 0;				// RESET THE COUNT BACK TO THE BEGINNING OF THE BUFFER READY FOR THE NEXT STRING
			if (g_UnitFlags.DebugSLCD) 
			{
				PortPutStr("from SLCD:",PC_PORT,1);
				PortPutCRStr(lcd_sentence,PC_PORT,1);
			}
			lcd_sentence[MAX_SLCD_RESPONSE-1] = 0;	// make sure the longest string is null terminated
			return lcd_sentence;
		}
	}

	return NULL;
}

void SLCD_putBootloadMessage()
{
	SLCD_DisplayPower(1);
	SetDefaultBackForegnd();		
    SLCD_SendCmd(CLEARSCREEN);			// clear screen
	SLCD_SendCmd("f24");
	SLCD_SendCmd("t \"Bootload Mode...\" 20 50");
}

//ld n x0 y0 x1 y1 or inv bv bc <levels>
//Arguments: n - object index from 0 to 9 (maximum 10 charts).
//x0,y0 and x1,y1 are the top left corner and bottom right corners of the object’s area
//or - orientation: 0 = vertical, 1 = horizontal
//inv - invert: 0 = no (low value at bottom / left); 1 = yes (low value at top / right)
//bv - bottom data value; should be 1 if value 0 means no level displayed
//bc - background color in RGB format (3 ASCII hex characters – see SET COLOR DETAILED)
//<levels> - one or more sets of two values: value and associated color. 

// Draws a bar and initializes the level to 0
void InitRSSIBar(int id, int x, int y, int width, int height)
{
	char buf[80];
	sprintf(buf,"ld %d %d %d %d %d 1 0 1 888 100 0F0 70 FF0 30 F00",id,x,y,x+width,y+height);
	SLCD_SendCmd(buf);
	SLCD_SendCmd("p 2");
	sprintf(buf,"l %d %d %d %d",x,y+height+1,x+width+1,y+height+1);
	SLCD_SendCmd(buf);
	sprintf(buf,"l %d %d %d %d",x+width+1,y,x+width+1,y+height+1);
	SLCD_SendCmd(buf);
	sprintf(buf,"lv %d 0", id);
	SLCD_SendCmd(buf);	
}

void UpdateRssiValue(int rssi, char* label, int barindex, int x, int y)
{
	char buffer[60];
	if (rssi >= 0) {
		sprintf(buffer,"t \"%s= %3d \" %d %d",label,(rssi),x,y);
		SLCD_SendCmd(buffer);
		sprintf(buffer,"lv %d %d", barindex,ScaleLogPowerToMax(rssi, 100, 32768l));
		SLCD_SendCmd(buffer);
	}
	else {
		sprintf(buffer,"t \"%s=      \" %d %d",label,x,y);
		SLCD_SendCmd(buffer);
	}
}


void UpdatePeakRssiValue(int rssi, char* label, int x, int y)
{
	char buffer[60];
	sprintf(buffer,"t \"%s= %d \" %d %d",label, rssi, x,y);
	SLCD_SendCmd(buffer);
}




