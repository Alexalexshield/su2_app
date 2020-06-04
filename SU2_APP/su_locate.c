#include "system.h"
#include "su_slcd.h"
#include <stdio.h>
#include <stdlib.h>
#include "vlf_rxtx.h"
#include "string.h"
#include "trace.h"
#include "math.h"


#ifdef DISTANCE_ONLY_METHOD
/*
//////////////////////////////////////// local globals ////////////////////////////////////////

/////////////////////////////////////////// local prototypes ////////////////////////////
void SU_DrawLocateData(int index);
void SU_InitLocateBar( unsigned char index, int x, int y );
//void Test_GenerateMUResponse(int index);
void UpdateLocateZeroCrossings();
void SU_UpdateMode(int countdown, char antenna, int retry);

//////////////////////////////////////////// local defines ///////////////////////////////


#define LOCATE_INDENT 10

#define LOCATE_ID_XPOS 10
#define LOCATE_ID_YPOS 90

#define LOCATE_FONT_HEIGHT 24

#define LOCATE_POWERMETER_XPOS LOCATE_INDENT
#define LOCATE_POWERMETER_YPOS (LOCATE_ID_YPOS+LOCATE_FONT_HEIGHT)
#define LOCATE_POWERMETER_HEIGHT 180

#define LOCATE_NOTES_XPOS LOCATE_INDENT
//#define LOCATE_NOTES_YPOS (LOCATE_POWERMETER_YPOS + LOCATE_POWERMETER_HEIGHT)
#define LOCATE_NOTES_YPOS (LOCATE_POWERMETER_YPOS + LOCATE_FONT_HEIGHT)

#define POWER_BAR_INDENT 	(SLCD_MAX_X/2 + 40)

#define LOCATE_MOBILITY_XPOS LOCATE_INDENT
#define LOCATE_MOBILITY_YPOS (LOCATE_NOTES_YPOS + LOCATE_FONT_HEIGHT)
#define LOCATE_MOBILITYBAR_XPOS (POWER_BAR_INDENT)

#define LOCATE_XPOWER_XPOS LOCATE_INDENT
#define LOCATE_XPOWER_YPOS (LOCATE_NOTES_YPOS + LOCATE_FONT_HEIGHT*2)

#define LOCATE_YPOWER_XPOS LOCATE_INDENT
#define LOCATE_YPOWER_YPOS (LOCATE_XPOWER_YPOS +24)

#define LOCATE_ZPOWER_XPOS LOCATE_INDENT
#define LOCATE_ZPOWER_YPOS (LOCATE_XPOWER_YPOS +48)

#define LOCATE_XPOWERBAR_XPOS (POWER_BAR_INDENT)
#define LOCATE_YPOWERBAR_XPOS (POWER_BAR_INDENT)
#define LOCATE_ZPOWERBAR_XPOS (POWER_BAR_INDENT)

#define LOCATE_MODE_XPOS LOCATE_INDENT
#define LOCATE_MODE_YPOS 380

#define LOCATE_LEVELBAR_WIDTH (SLCD_MAX_X - POWER_BAR_INDENT - 20)
#define LOCATE_LEVELBAR_HEIGHT 14

#define LOCATE_DISTANCE_YPOS (LOCATE_ZPOWER_YPOS+48)
#define LOCATE_ZC_YPOS		 (LOCATE_ZPOWER_YPOS+96) 

/////////////////////////////////////////////////// LOCATE FUNCTIONS /////////////////////////////////////

//Roughly speaking the distance formula for an unmodified MU would be x = (K/RSSI)1/3 where x is distance in meters, RSSI is signal strength in A/D counts pk-pk, and K is a factor that depends on range being used.
//K will have to be adjustable to allow for unit calibration and possible MU TX design changes.
//For distances > ~7.3m the final gain stage would be used (e.g. X2F output, green highlight in xls file) and K would be approx. 403,000. Closer than this would saturate the A/D so pk-pk counts would approach or equal 1024 (op amp may not swing rail to rail).
//For distances < ~7.3m the preamp gain stage would be used (e.g. X1F output  in next rev of board, yellow highlight in xls file) and K would be approx 2,728. 


void SU_DrawLocateData(int index)
{
	char buffer[60];
	int value;
	int x,y;
	unsigned int pkpower;
	double meters;

	if ((index < 0) || (index >= MAXIMUM_MU_DEVICES_IN_LIST))	// make sure index is valid
		return;
	MuData* p = &g_MuDeviceList[index];
	SetDefaultBackForegnd();		

//	SLCD_SendCmd("f24B");	// set font size
//	SLCD_SendCmd("f14x24");	// set font size
	SLCD_SendCmd("f12x24");	// set font size

	///////////////////////////////////////// ID  ///////////////////////////////////////////
	sprintf(buffer,"t \"ID = %8lu  \" %d %d",(p->id_u32 & 0x00ffffff),LOCATE_ID_XPOS,LOCATE_ID_YPOS); // only lower 24 bits of ID are applicable
	SLCD_SendCmd(buffer);


	///////////////////////////////////////// MOBILITY  ///////////////////////////////////////////
	if (p->v.Mobility_valid) {
#ifdef PROTOCOL_1
			value =  min(15,p->m.mobility);
			sprintf(buffer,"t \"Mobility=%2d \" %d %d",(int)(value),LOCATE_MOBILITY_XPOS,LOCATE_MOBILITY_YPOS);
#elif PROTOCOL_2					
			value =  min(15,p->m.status.mobility);
			sprintf(buffer,"t \"Mobility=%2d \" %d %d",(int)(value),LOCATE_MOBILITY_XPOS,LOCATE_MOBILITY_YPOS);			
#else
#error PROTOCOL NOT DEFINED					
#endif
		SLCD_SendCmd(buffer);
		// Set level bar values
		sprintf(buffer,"lv %d %d", SU_MOBILITY_LEVEL_INDEX, value);
		SLCD_SendCmd(buffer);
	}
	else
		sprintf(buffer,"t \"Mobility=NA  \" %d %d",LOCATE_MOBILITY_XPOS,LOCATE_MOBILITY_YPOS);
		
	SLCD_SendCmd(buffer);
	
	if ((p->v.X1_power_valid) && (p->v.Y1_power_valid) && (p->v.Z1_power_valid))
	{
		// use 1F if any 1F readings are THRESHOLD_1F or more
//		if ((p->x1_power >= THRESHOLD_1F) || (p->y1_power >= THRESHOLD_1F)  || (p->z1_power >= THRESHOLD_1F))
		{
			///////////////////////////////////////// 1F RSSI ///////////////////////////////////
			UpdateRssiValue(p->x1_power, "delta X1", SU_XPOWER_LEVEL_INDEX, LOCATE_XPOWER_XPOS, LOCATE_XPOWER_YPOS);
			UpdateRssiValue(p->y1_power, "delta Y1", SU_YPOWER_LEVEL_INDEX, LOCATE_YPOWER_XPOS, LOCATE_YPOWER_YPOS);
			UpdateRssiValue(p->z1_power, "delta Z1", SU_ZPOWER_LEVEL_INDEX, LOCATE_ZPOWER_XPOS, LOCATE_ZPOWER_YPOS);
			pkpower = getPeakPower_1F();
			UpdatePeakRssiValue(pkpower, "Pk delta XYZ 1", LOCATE_ZPOWER_XPOS, LOCATE_ZPOWER_YPOS + 24);
		}
//		else	
		{
			///////////////////////////////////// 2F RSSI /////////////////////////////////
//			UpdateRssiValue(p->x2_power, "delta X2", SU_XPOWER_LEVEL_INDEX, LOCATE_XPOWER_XPOS, LOCATE_XPOWER_YPOS);
//			UpdateRssiValue(p->y2_power, "delta Y2", SU_YPOWER_LEVEL_INDEX, LOCATE_YPOWER_XPOS, LOCATE_YPOWER_YPOS);
//			UpdateRssiValue(p->z2_power, "delta Z2", SU_ZPOWER_LEVEL_INDEX, LOCATE_ZPOWER_XPOS, LOCATE_ZPOWER_YPOS);
//			pkpower = getPeakPower_2F();
//			UpdatePeakRssiValue(pkpower, "Pk delta XYZ 2", LOCATE_ZPOWER_XPOS, LOCATE_ZPOWER_YPOS + 24);
		}
	}
	else 	/////////////////////////// invalid data /////////////////////////////////////////////////
	{
			UpdateRssiValue(0, "delta X1", SU_XPOWER_LEVEL_INDEX, LOCATE_XPOWER_XPOS, LOCATE_XPOWER_YPOS);
			UpdateRssiValue(0, "delta Y1", SU_YPOWER_LEVEL_INDEX, LOCATE_YPOWER_XPOS, LOCATE_YPOWER_YPOS);
			UpdateRssiValue(0, "delta Z1", SU_ZPOWER_LEVEL_INDEX, LOCATE_ZPOWER_XPOS, LOCATE_ZPOWER_YPOS);
			UpdatePeakRssiValue(0, "Pk delta XYZ 1", LOCATE_ZPOWER_XPOS, LOCATE_ZPOWER_YPOS + 24);		
	}
	
	///////////////////////////////////////// Distance ///////////////////////////////////////////
	x = LOCATE_ZPOWER_XPOS;
	y = LOCATE_DISTANCE_YPOS;
	meters = Get_MU_Distance(p);
	if (meters >= 0.0)	
	{
		sprintf(buffer,"t \"Distance= ~%4.1f m    \" %d %d",meters,x,y);
	}
	else
	{
		sprintf(buffer,"t \"Distance= unknown  \" %d %d",x,y);		
	}
	SLCD_SendCmd(buffer);
}

	///////////////////////////////////////// MODE  ///////////////////////////////////////////
void SU_UpdateMode(int countdown, char antenna, int retry)
{
	char buffer[60];
	int x,y;
	char a1=antenna;
	char a2=' ';
	char a3=' ';
	
	x = LOCATE_MODE_XPOS;
	y = LOCATE_MODE_YPOS;
	if (a1 == 'A')
	{
		a1 = 'X'; a2= 'Y'; a3='Z';
	}
	if (countdown > 0)
		sprintf(buffer,"t \"Locating %c%c%c - %3d \" %d %d",a1,a2,a3, countdown,x,y);
	else 
	{
		if (retry ==0)
			sprintf(buffer,"t \"Standby            \" %d %d",x,y);
		else
			sprintf(buffer,"t \"Transmitting %c%c%c  \" %d %d",a1,a2,a3, x,y);
	}
	SLCD_SendCmd(buffer);
}


void UpdateLocateZeroCrossings()
{
	char buffer[60];
	SetDefaultBackForegnd();		
	SLCD_SendCmd("f12x24");	// set font size
	int	x = LOCATE_ZPOWER_XPOS;
	int y = LOCATE_ZC_YPOS;
	sprintf(buffer,"t \"zc X= %2u Y= %2u Z= %2u \" %d %d",g_zero_crossings.x,g_zero_crossings.y,g_zero_crossings.z,x,y);
	SLCD_SendCmd(buffer);
}


// Draws a bar and initializes the level to 0

void SU_DrawLocatePage()
{
	char buf[80];
	int adder = NAVIGATION_BTN_BMP_WIDTH + 4;
	int x = SLCD_MAX_X - (adder * 5);
	int y;

    SLCD_SendCmd(CLEARSCREEN);			// clear screen
	SetDefaultBackForegnd();		
	SU_DisplayStandardNavigationButtons(x+adder, NAVIGATION_BTN_YPOS, adder,TRUE);

	SLCD_SendCmd("f24");
	SLCD_SendCmd("t \"Move the antenna\nto find peaks and nulls\n* = Repeat, X = exit\" 10 400");

	SU_InitMainBatteryBar();	// creates and display battery bar
	SU_InitTXBatteryBar();

//  md <idx> <bitmap> <x> <y> <type> <minVal> <maxVal> <init_val> <minAngle> <maxAngle> <x0 y0> <x1 y1> . . . [x10 y10]>
//	md 1 48 0 0 1 475 515 500 270 90 126 120 -4 0 0 -78 4 0
//	This example defines a meter with index number 1, using bitmap index 48 as the background image. 
//	The type is always 1. The minimum value of 475 for the indicator is at angle 270 degrees (90 degrees to left of vertical), 
//	and the maximum value of 515 is at angle 90 degrees. The indicator will point to initial value 500. 
//	The indicator pivot point is 126 120 and the indicator is drawn as a vertical triangle with polygon points -4 0 0 -78 4 0.

//	sprintf(buf,"md 1 48 %d %d 1 410 590 500 270 90 126 120 -4 0 0 -78 4 0",LOCATE_POWERMETER_XPOS,LOCATE_POWERMETER_YPOS);
//	SLCD_SendCmd(buf);	// draw a semi-circle a <X0> <Y0> <Radius> <Start Angle> <End Angle>


//ld n x0 y0 x1 y1 or inv bv bc <levels>
//Arguments: n - object index from 0 to 9 (maximum 10 charts).
//x0,y0 and x1,y1 are the top left corner and bottom right corners of the object’s area
//or - orientation: 0 = vertical, 1 = horizontal
//inv - invert: 0 = no (low value at bottom / left); 1 = yes (low value at top / right)
//bv - bottom data value; should be 1 if value 0 means no level displayed
//bc - background color in RGB format (3 ASCII hex characters – see SET COLOR DETAILED)
//<levels> - one or more sets of two values: value and associated color. 

//These start with the maximum and go down. At most 3 sets are possible. Color is the same format at the bc parameter.

	x = LOCATE_MOBILITYBAR_XPOS;
	y = LOCATE_MOBILITY_YPOS+2;
	sprintf(buf,"ld %d %d %d %d %d 1 0 1 888 15 0F0 7 FF0 3 F00",SU_MOBILITY_LEVEL_INDEX,x,y,x+LOCATE_LEVELBAR_WIDTH,y+LOCATE_LEVELBAR_HEIGHT);
	SLCD_SendCmd(buf);
	SLCD_SendCmd("p 2");
	sprintf(buf,"l %d %d %d %d",x,y+LOCATE_LEVELBAR_HEIGHT+1,x+LOCATE_LEVELBAR_WIDTH+1,y+LOCATE_LEVELBAR_HEIGHT+1);
	SLCD_SendCmd(buf);
	sprintf(buf,"l %d %d %d %d",x+LOCATE_LEVELBAR_WIDTH+1,y,x+LOCATE_LEVELBAR_WIDTH+1,y+LOCATE_LEVELBAR_HEIGHT+1);
	SLCD_SendCmd(buf);
	sprintf(buf,"lv %d 0", SU_MOBILITY_LEVEL_INDEX);
	SLCD_SendCmd(buf);

	InitRSSIBar(SU_XPOWER_LEVEL_INDEX, LOCATE_XPOWERBAR_XPOS, LOCATE_XPOWER_YPOS+2,LOCATE_LEVELBAR_WIDTH,LOCATE_LEVELBAR_HEIGHT);
	InitRSSIBar(SU_YPOWER_LEVEL_INDEX, LOCATE_YPOWERBAR_XPOS, LOCATE_YPOWER_YPOS+2,LOCATE_LEVELBAR_WIDTH,LOCATE_LEVELBAR_HEIGHT);
	InitRSSIBar(SU_ZPOWER_LEVEL_INDEX, LOCATE_ZPOWERBAR_XPOS, LOCATE_ZPOWER_YPOS+2,LOCATE_LEVELBAR_WIDTH,LOCATE_LEVELBAR_HEIGHT);
	
}


char locate_antenna_order[TX_ANTENNA_NUMBER] = {TX_ANTENNA_X,TX_ANTENNA_Y,TX_ANTENNA_Z};


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// PROTOCOL 1 FUNCTIONS ///////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef PROTOCOL_1

void Send_P1_LocateMessage(U32 id_u32, int seconds, char antenna)
{
	g_bTransmitting = TRUE;	// SET MODE TO TRANSMITTING TO ENSURE RSSI PACKETS IGNORED
	if (g_SuSettings.enable_audio_blanking) AUDIO_MUTE_LATCH = 1;	//normally low to enable the audio board and is high to mute the audio output.
	DelayMsecs(AUDIO_MUTE_DELAY_MS);				// delay to allow audio to be muted
	vlf_SendLocateMsg(id_u32, seconds, antenna);
	set_SU_VLF_RX_Mode(SU_RX_MODE_DECODE);	// tell the MURX module to begin decoding messages
	DelayMsecs(LOCATE_RX_DELAY_MS_P1);						// SU_TX can't receive during the TX period
	AUDIO_MUTE_LATCH = 0;	//normally low to enable the audio board and is high to mute the audio output.
	g_bTransmitting = FALSE;	// RE-ENABLE RSSI PACKET UPDATES
	
}

void SU_RunLocatePage(int index)
{
	struct button btn;
	int locatecountdown;
	int antennacountdown = 3;
	time_t ref_second = g_seconds;
	int antenna_ix = 0;
	int lastResponseCount = 0;
	int lastRSSICount = 0;
	unsigned short taskStatus;
	unsigned long long start_tick = g_tick;

	locatecountdown = g_SuSettings.locate_duration_seconds+LOCATE_TIMEOUT_SECONDS_P1;
		
	set_SU_VLF_RX_Mode(SU_RX_MODE_STANDBY);		// tell the MURX module to wait until it is told to decode
	SU_DrawLocatePage();
	SU_UpdateGeneralStatus(TRUE,TRUE);
	
	vlf_SetMUID(g_MuDeviceList[index].id_u32);	// make sure that all incoming messages are mapped to this id and antenna since MU packets don't always include the ID
	send_SU_RX_ZC_Trigger();
	
	clear_MU_ResponseCount();					// clear the response count so we can tell if we get a response to the locate msg
    clear_SU_ZC_RX_ResponseCount();				// CLEAR THE ZC RESPONSE COUNT	
    clear_RSSI_RX_ResponseCount();
    clear_PeakPower();	 						// clear peak power for new locate.
	Send_P1_LocateMessage(g_MuDeviceList[index].id_u32, g_SuSettings.locate_duration_seconds,locate_antenna_order[antenna_ix] );
    
	while (1) {				// loop until we get an exit from the page
		if (ref_second != g_seconds) {
			ref_second = g_seconds;
			if (locatecountdown) {
				locatecountdown--;
				if (locatecountdown == 0) {
					if (antennacountdown) {	// if we haven't checked all antennas yet, do the next
						antenna_ix++;
						antenna_ix = antenna_ix%TX_ANTENNA_NUMBER;
						antennacountdown--;
						locatecountdown = g_SuSettings.locate_duration_seconds+LOCATE_TIMEOUT_SECONDS_P1;
						clear_MU_ResponseCount();		// clear the response count so we can tell if we get a response to the locate msg
						Send_P1_LocateMessage(g_MuDeviceList[index].id_u32, g_SuSettings.locate_duration_seconds,locate_antenna_order[antenna_ix] );
					}
					else {
						set_SU_VLF_RX_Mode(SU_RX_MODE_STANDBY);	// tell the MURX module to wait until it is told to decode
					}
				}
				else {	// check to see if we got a resonse within SECONDS_TO_WAIT_FOR_LOCATE_RESPONSE
					if (locatecountdown == g_SuSettings.locate_duration_seconds) {
						if (get_MU_ResponseCount() > 0)
							set_SU_VLF_RX_Mode(SU_RX_MODE_LOCATE);	// tell the MURX module to decode locate messages
						else {
							locatecountdown = 1;	// no response so move to the next antenna in 1 second
						}
					}
				}
			}
			SU_UpdateGeneralStatus(TRUE,FALSE);

		}
		
		if ((g_tick - start_tick) > 200) {	// update display every 200 ms
			start_tick = g_tick;
			// update any new messages
			if ((lastResponseCount != get_MU_ResponseCount()) ||  (lastRSSICount != get_RSSI_RX_ResponseCount()) ) {
				SU_DrawLocateData(index,locatecountdown,locate_antenna_order[antenna_ix], antennacountdown);
				SU_UpdateMode(locatecountdown,locate_antenna_order[antenna_ix]);
				lastResponseCount = get_MU_ResponseCount();
				lastRSSICount = get_RSSI_RX_ResponseCount();
			}
			if (get_ZC_RX_ResponseCount() > 0) {
				UpdateLocateZeroCrossings(); 			// display zero crossing data
	    		clear_SU_ZC_RX_ResponseCount();			// CLEAR THE ZC RESPONSE COUNT	
			}
		}

		taskStatus = ProcessBackgroundTasks();
		

		btn = GetUserAction(taskStatus);
		if (btn.index != 0) {
			switch (btn.index) {
				case SU_BTN_UP:
					break;
				case SU_BTN_DOWN:
					break;
				case SU_BTN_EXIT:
					if (locatecountdown != 0) {
						locatecountdown = 0;
						set_SU_VLF_RX_Mode(SU_RX_MODE_STANDBY);	// tell the MURX module to wait until it is told to decode
					}
					return;
					break;
				case SU_BTN_ENTER:		// TODO - POSSIBLY USE ENTER TO MASK?
					break;
				case SU_BTN_PAUSE:
					if (locatecountdown ) {
						locatecountdown = 0;
						SU_DrawLocateData(index,locatecountdown,locate_antenna_order[antenna_ix],antennacountdown);
						set_SU_VLF_RX_Mode(SU_RX_MODE_STANDBY);	// tell the MURX module to decode regular messages until we get the response
					}
					break;
				case SU_BTN_LOCATE:
					if (locatecountdown == 0) {
						antennacountdown = 3; 			// try all 3 antennas if no signal received
						locatecountdown = g_SuSettings.locate_duration_seconds+LOCATE_TIMEOUT_SECONDS_P1;
						SU_DrawLocateData(index,locatecountdown,locate_antenna_order[antenna_ix], antennacountdown);
						clear_MU_ResponseCount();		// clear the response count so we can tell if we get a response to the locate msg
    					clear_PeakPower();	 // clear peak power for new locate.
						Send_P1_LocateMessage(g_MuDeviceList[index].id_u32, g_SuSettings.locate_duration_seconds,locate_antenna_order[antenna_ix] );
					}
					else 
					{
						antennacountdown = 3; 			// reset to 3 retries
    					clear_PeakPower();	 			// clear peak power for new locate.						
					}
					break;
				case SU_BTN_SETTINGS:
					SU_RunSettingsPage();
					SU_DrawLocatePage();
					SU_UpdateGeneralStatus(TRUE,TRUE);
					break;	
				case SU_BTN_LOG:
					SU_RunLogViewPage();
					SU_DrawLocatePage();
					SU_UpdateGeneralStatus(TRUE,TRUE);
					break;
				case SU_BTN_QUICK_SEARCH:
//					if (locatecountdown) {
//						Test_GenerateMUResponse(index);
//						SU_DrawLocateData(index,locatecountdown,locate_antenna_order[antenna_ix]);
//					}
					break;
				default:
					break;
			}
		}
	}

}
#endif


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// PROTOCOL 2 FUNCTIONS ///////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef PROTOCOL_2


#endif


*/

#else // not DISTANCE_ONLY_METHOD


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////   NEW METHOD INCLUDING DIRECTION   //////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////


#define LOCATE_DISTANCE_XPOS (10)
#define LOCATE_DISTANCE_YPOS (90)

typedef enum LOCATE_STEP
{
	eLOCATE_START = 0,
	eLOCATE_STEP1,
	eLOCATE_STEP2,
	eLOCATE_STEP3,
	eLOCATE_STEP4,
	eLOCATE_STEP5,
	eLOCATE_ERROR
} eLocateStep;

#define PI 3.14159
#define PIEX2 (2.0*PI)
#define RADTODEG(r) (((r)/PIEX2)*360.0)
#define DEGTORAD(d) (((d)/360.0)*PIEX2)


void SU_DrawLocateBackgroundPage(MuData* p)
{
	char buffer[60];
	SetDefaultBackForegnd();		
    SLCD_SendCmd(CLEARSCREEN);
 
#ifdef INCLUDE_MINERADIO_HEADER
	DRAW_MINERADIO_HEADER;
	DRAW_MINERADIO_FOOTER;
#endif
    			
	SetDefaultBackForegnd();		
//	SLCD_SendCmd("f32");
	SLCD_SendCmd("fUARIAL18");

#ifdef RUSSIAN
	sprintf(buffer,"t \"\xD0\x9F\xD0\xBE\xD0\xB8\xD1\x81\xD0\xBA #%08lu\" 10 50 N", (p->id_u32 & 0x00ffffff));
#else	
	sprintf(buffer,"t \"Dist&Dir #%08lu\" 10 50 N", (p->id_u32 & 0x00ffffff));
#endif
	SLCD_SendCmd(buffer);
	
	SU_InitMainBatteryBar();	// creates and display battery bar
	SU_InitTXBatteryBar();

}

struct LOCATE_DATA
{
	double d1;
	double d2;
	double d3;
	double angle1;
	double angle2;
};


#define LOCATE_CIRCLE_ORIGIN_XPOS (SLCD_MAX_X/2)
#define LOCATE_CIRCLE_ORIGIN_YPOS ((SLCD_MAX_Y/2) + 24)
#define LOCATE_ARROW_LENGTH 75
#define LOCATE_ARROW_WIDTH 10
#define LOCATE_CIRCLE_RADIUS 130

#define FLASHING_REF_TEXT_INDEX 0
//Description: Draws a Arc Segment.
//Command: a <X0> <Y0> <Radius> <Start Angle> <End Angle>
//Arguments: <X0> <Y0> Center point of the ARC segment
//<Radius> Radius of arc (Pixels)
//<Start Angle> Starting angle (Degrees)
//<End Angle> Ending angle (Degrees)

void SU_ShowDistanceMsg(char* msg)
{
	char buffer[60];

	SetDefaultBackForegnd();
//	SLCD_SendCmd("f32B");	// set font size
	SLCD_SendCmd("fUARIAL16");
	
	sprintf(buffer,"t \"                           \" %d %d N", LOCATE_DISTANCE_XPOS, LOCATE_DISTANCE_YPOS); // erase any existing messages
	SLCD_SendCmd(buffer);
	sprintf(buffer,"t \"%s\" %d %d N", msg, LOCATE_DISTANCE_XPOS, LOCATE_DISTANCE_YPOS);
	SLCD_SendCmd(buffer);
	
}

void ShowLocateDistance(double meters,char* prefix)
{
	char buffer[60];
	SetDefaultBackForegnd();
//	SLCD_SendCmd("f32B");	// set font size
	SLCD_SendCmd("fUARIAL16");
#ifdef RUSSIAN
	sprintf(buffer,"t \"%s \xD0\xA0\xD0\xB0\xD1\x81\xD1\x81\xD1\x82\xD0\xBE\xD1\x8F\xD0\xBD\xD0\xB8\xD0\xB5 %4.2f\xD0\xBC              \" %d %d",prefix,meters,LOCATE_DISTANCE_XPOS,LOCATE_DISTANCE_YPOS);
#else
	sprintf(buffer,"t \"%s d=%4.2fm              \" %d %d",prefix,meters,LOCATE_DISTANCE_XPOS,LOCATE_DISTANCE_YPOS);
#endif	
	SLCD_SendCmd(buffer);
}


// remember to select the color and pen width beforehand
#define DIRECTION_ARROW_LENGTH 15
#define FROUND(f)  (f < 0.0)?(f-0.5):(f+0.5)
double fround(double f)
{
	if (f < 0.0)
		return (f - 0.5);
	else
		return (f + 0.5);
}

#define DIRECTION_ARROW_DEGREES 5.0
void DrawRotatedArrow(double angle, int len, int origin_x, int origin_y)
{
	char buffer[60];
	int x1, y1, x2, y2, x3,y3;
	double ox = origin_x;
	double oy = origin_y;
	double dtemp;

	dtemp = (len * cos(DEGTORAD(angle)));
	x1 = fround(ox + dtemp);
	
	dtemp = (len * sin(DEGTORAD(angle))); 
	y1 =  fround(oy - dtemp);
	
	dtemp =  ((len-DIRECTION_ARROW_LENGTH) * cos(DEGTORAD((angle+DIRECTION_ARROW_DEGREES))));
	x2 = fround(ox + dtemp);
	
	dtemp = ((len-DIRECTION_ARROW_LENGTH) * sin(DEGTORAD((angle+DIRECTION_ARROW_DEGREES)))); 
	y2 = fround(oy - dtemp);

	dtemp = ((len-DIRECTION_ARROW_LENGTH) * cos(DEGTORAD((angle-DIRECTION_ARROW_DEGREES))));
	x3 = fround(ox + dtemp);
	
	dtemp = ((len-DIRECTION_ARROW_LENGTH) * sin(DEGTORAD((angle-DIRECTION_ARROW_DEGREES)))); 
	y3 = fround(oy - dtemp);

	sprintf(buffer,"l %d %d %d %d",	origin_x, origin_y, x1, y1	);
	SLCD_SendCmd(buffer);

	sprintf(buffer,"l %d %d %d %d",	x1, y1, x2, y2	);
	SLCD_SendCmd(buffer);

	sprintf(buffer,"l %d %d %d %d",	x1, y1, x3, y3	);
	SLCD_SendCmd(buffer);	

	sprintf(buffer,"l %d %d %d %d",	x2, y2, x3, y3	);
	SLCD_SendCmd(buffer);	
	
}

void SU_DrawLocateDistance(eLocateStep step, struct LOCATE_DATA* aLocateData, int maxindex)
{
	char buffer[60];
	int x,y;
	int i;
	double scale;
	double offset;
	double dtemp;
	
	SetDefaultBackForegnd();		
//	SLCD_SendCmd("f32B");	// set font size
	SLCD_SendCmd("fUARIAL18");
	x = LOCATE_DISTANCE_XPOS;
	y = LOCATE_DISTANCE_YPOS;
	struct LOCATE_DATA* pData = &aLocateData[maxindex];
	
	double k = (double)g_SuSettings.locate_ref_meters;
	double angle_adjustment = sin((k/2.0)/pData->d1);
	double dist_adjusted = sqrt(pData->d1*pData->d1 + (k*k)); 
	angle_adjustment = RADTODEG(angle_adjustment);

	switch (step)
	{	
		case eLOCATE_START:
			break;
		case eLOCATE_STEP1:
			ShowLocateDistance(pData->d1,"R1:");
			break;
		case eLOCATE_STEP2:
			ShowLocateDistance(pData->d2,"R2:");
			SetForeBackGndColor(SLCD_GREEN, SLCD_BACKGROUND);
			SLCD_SendCmd("p 10");		// This sets the pen width to 10 pixels wide
//			if (pData->d1 >= pData->d2)
			if (pData->d1 >= dist_adjusted)
			{
				sprintf(buffer,"a %d %d %d %d %d", 
					LOCATE_CIRCLE_ORIGIN_XPOS, LOCATE_CIRCLE_ORIGIN_YPOS, LOCATE_CIRCLE_RADIUS, 
					90,270
//					(int)(90 - angle_adjustment), (int)(270 + angle_adjustment)
					);
				SLCD_SendCmd(buffer);
			}
			else
			{
				sprintf(buffer,"a %d %d %d %d %d", 
					LOCATE_CIRCLE_ORIGIN_XPOS, LOCATE_CIRCLE_ORIGIN_YPOS, LOCATE_CIRCLE_RADIUS, 
//					(int)(270 + angle_adjustment), (int)(360)
					270 , 360
					);
				SLCD_SendCmd(buffer);				
				sprintf(buffer,"a %d %d %d %d %d", 
					LOCATE_CIRCLE_ORIGIN_XPOS, LOCATE_CIRCLE_ORIGIN_YPOS, LOCATE_CIRCLE_RADIUS, 
//					(int)(0), (int)(90 - angle_adjustment)
					0, 90
					);
				SLCD_SendCmd(buffer);				
			}
			break;
		case eLOCATE_STEP3:
//			ShowLocateDistance(pData->d3,"R3");
			SLCD_SendCmd(buffer);			
			SLCD_SendCmd("p 10");		// This sets the pen width to 10 pixels wide
			SetForeBackGndColor(SLCD_GREEN, SLCD_BACKGROUND);
			
//			if (pData->d3 < pData->d1)
			if (pData->d3 < dist_adjusted)
			{
				sprintf(buffer,"a %d %d %d %d %d", 
					LOCATE_CIRCLE_ORIGIN_XPOS, LOCATE_CIRCLE_ORIGIN_YPOS, LOCATE_CIRCLE_RADIUS, 
//					(int)(0 + angle_adjustment), (int)(180 - angle_adjustment)
					0,180
					);
				SLCD_SendCmd(buffer);
			}
			else
			{
				sprintf(buffer,"a %d %d %d %d %d", 
					LOCATE_CIRCLE_ORIGIN_XPOS, LOCATE_CIRCLE_ORIGIN_YPOS, LOCATE_CIRCLE_RADIUS, 
//					(int)(180 - angle_adjustment), 360
					180, 360
					);
				SLCD_SendCmd(buffer);				
//				sprintf(buffer,"a %d %d %d %d %d", 
//					LOCATE_CIRCLE_ORIGIN_XPOS, LOCATE_CIRCLE_ORIGIN_YPOS, LOCATE_CIRCLE_RADIUS, 
//					0, (int)angle_adjustment
//					);
//				SLCD_SendCmd(buffer);				
			}

			break;
		case eLOCATE_STEP4:
#ifdef RUSSIAN
			sprintf(buffer,"t \"%4.1f\xD0\xBC  %3.0f\xC2\xB0    \" %d %d",pData->d1, pData->angle1, x, y);
#else
			sprintf(buffer,"t \"%4.1fm  %3.0f\xC2\xB0    \" %d %d",pData->d1, pData->angle1, x,y);
#endif	
			SLCD_SendCmd(buffer);
			
			SetForeBackGndColor(SLCD_GREEN, SLCD_BACKGROUND);

			for (i=0;i <=maxindex;i++)
			{
				struct LOCATE_DATA* p = &aLocateData[i];

				///////////////////////////// DRAW ARROW 1 POINTING IN DIRECTION OF MU /////////////////////////////
				SLCD_SendCmd("p 3");		// This sets the pen width to 10 pixels wide
				DrawRotatedArrow(p->angle1, LOCATE_CIRCLE_RADIUS, LOCATE_CIRCLE_ORIGIN_XPOS, LOCATE_CIRCLE_ORIGIN_YPOS);			

				///////////////////////////// DRAW ARROW 2 POINTING IN DIRECTION OF MU /////////////////////////////
//				SLCD_SendCmd("p 1");		// This sets the pen width to 1 pixels wide
//				DrawRotatedArrow(p->angle2, LOCATE_CIRCLE_RADIUS, LOCATE_CIRCLE_ORIGIN_XPOS, LOCATE_CIRCLE_ORIGIN_YPOS);			
			
			}
			
			//SLCD_SendCmd("f24B");
			SLCD_SendCmd("fUARIAL16");
			SetDefaultBackForegnd();
			sprintf(buffer, "t \"R1\" %d %d N", LOCATE_CIRCLE_ORIGIN_XPOS - 12, LOCATE_CIRCLE_ORIGIN_YPOS-12); // Redraw R1
			SLCD_SendCmd(buffer);

			
			break;
		case eLOCATE_STEP5:
			{
			double max_r12, max_r13;
			sprintf(buffer,"t \" %4.1fm  %3.0f\xC2\xB0    \" %d %d N",pData->d1, pData->angle1, x,y);
			SLCD_SendCmd(buffer);
			offset = (double)(g_SuSettings.locate_ref_meters);
			
			// calculate the maximum radius on the x or y axis to scale the drawing
			max_r12 = pData->d1 + offset + pData->d2;
			max_r13 = pData->d1 + offset + pData->d3;
			
			// use the most recent data for scaling the circles so they are on the same scale
			dtemp = max(max_r12,max_r13);
			if (dtemp == 0.0) dtemp = 1.0;
			scale = (LOCATE_CIRCLE_RADIUS*2)/dtemp;
			
			x = (SLCD_MAX_X - (max_r12*scale))/2.0 + pData->d1*scale;
			y = (LOCATE_CIRCLE_ORIGIN_YPOS-LOCATE_CIRCLE_RADIUS) + (pData->d1 + offset/2)*scale;		
//			y = ((LOCATE_CIRCLE_ORIGIN_YPOS+LOCATE_CIRCLE_RADIUS) - (max_r13*scale))/2.0 + pData->d1*scale;	

			for (i=0;i <=maxindex;i++)
			{	
				SetDefaultBackForegnd();
				SLCD_SendCmd("p 1");		// This sets the pen width to 2 pixels wide
				
				struct LOCATE_DATA* p = &aLocateData[i];
				//////////////////////////////////////// REF 1 circle //////////////////////////////////////
				sprintf(buffer,"c %d %d %d", 
					x, 
					y, 
					(int)(p->d1*scale) );			//c x0 y0 r [f]
				SLCD_SendCmd(buffer);

				sprintf(buffer,"l %d %d %d %d", x - (int)(p->d1*scale), y, x+(int)(p->d1*scale), y );				// draw cross hairs
				SLCD_SendCmd(buffer);
				sprintf(buffer,"l %d %d %d %d", x , y -(int)(p->d1*scale), x, y+(int)(p->d1*scale) );				// draw cross hairs
				SLCD_SendCmd(buffer);

				//////////////////////////////////////// Ref 2 Circle ///////////////////////////////////
				sprintf(buffer,"c %d %d %d", 
					x + (int)(g_SuSettings.locate_ref_meters*scale), 
					y , 
					(int)(p->d2*scale) );			//c x0 y0 r [f]
				SLCD_SendCmd(buffer);

				//////////////////////////////////////// Ref 3 Circle ///////////////////////////////////
				sprintf(buffer,"c %d %d %d", 
					x, 
					y - (int)(g_SuSettings.locate_ref_meters*scale), 
					(int)(p->d3*scale) );			//c x0 y0 r [f]
				SLCD_SendCmd(buffer);
				
	
				
				SLCD_SendCmd("p 3");		// This sets the pen width to 3 pixels wide
				SetForeBackGndColor(SLCD_GREEN, SLCD_BACKGROUND);
/*				dy = (p->d1*scale)*sin(DEGTORAD(p->angle1)); 
				dx = (p->d1*scale)*cos(DEGTORAD(p->angle1));
				sprintf(buffer,"l %d %d %d %d",
					x, 
					y,
					x + (int)dx,
					y - (int)dy
					);
				SLCD_SendCmd(buffer);
*/				
				DrawRotatedArrow(p->angle1, (int)(p->d1*scale), x, y );			

	
			}
			
			//SLCD_SendCmd("f24B");
			SLCD_SendCmd("fUARIAL16");
			SetDefaultBackForegnd();

			/////////////////////////////////// draw REF text /////////////////////////////////////////////////////////
			sprintf(buffer, "t \"R1\" %d %d", x - 12, y-12);
			SLCD_SendCmd(buffer);
		
/*			x = LOCATE_CIRCLE_ORIGIN_XPOS+ (int)(g_SuSettings.locate_ref_meters*scale);
			y = LOCATE_CIRCLE_ORIGIN_YPOS;
			sprintf(buffer, "t \"R2\" %d %d", x -12, y-12);
			SLCD_SendCmd(buffer);
			
			x = LOCATE_CIRCLE_ORIGIN_XPOS;
			y = LOCATE_CIRCLE_ORIGIN_YPOS - (int)(g_SuSettings.locate_ref_meters*scale);
			sprintf(buffer, "t \"R3\" %d %d N", x - 12, y-12);
			SLCD_SendCmd(buffer);
*/			
			}
			break;
		default:	
			break;	
	}	
}




void SU_Draw_Locate_Reference_Points(eLocateStep step)
{
	char buf[60];
	char buf2[20];
	int length = LOCATE_ARROW_LENGTH;
	int x = LOCATE_CIRCLE_ORIGIN_XPOS;
	int y = LOCATE_CIRCLE_ORIGIN_YPOS;
	int w = LOCATE_ARROW_WIDTH;

	SetDefaultBackForegnd();
	
	if (step < eLOCATE_STEP4)
	{
		#ifdef RUSSIAN
			sprintf(buf2,"%d\xD0\xBC",g_SuSettings.locate_ref_meters);
		#else
			sprintf(buf2,"%dm",g_SuSettings.locate_ref_meters);
		#endif
		/////////////////////////////////// draw meters text /////////////////////////////////////////////////////////
		sprintf(buf, "t \"%s\" %d %d", buf2, x - 36, y-(length/2+24));
		SLCD_SendCmd(buf);
		
		sprintf(buf, "t \"%s\" %d %d", buf2, x + length - 36, y + w/2);
		SLCD_SendCmd(buf);
	}
	
	if ((step > eLOCATE_STEP1) && (step != eLOCATE_STEP5))
	{	
		SLCD_SendCmd("p 1");		// set pen width to 3 pixels wide
		sprintf(buf,"c %d %d %d", x, y, LOCATE_CIRCLE_RADIUS );			//c x0 y0 r [f]
		SLCD_SendCmd(buf);
		sprintf(buf,"l %d %d %d %d", x -LOCATE_CIRCLE_RADIUS, y, x+LOCATE_CIRCLE_RADIUS, y );				// draw cross hairs
		SLCD_SendCmd(buf);
		sprintf(buf,"l %d %d %d %d", x , y -LOCATE_CIRCLE_RADIUS, x, y+LOCATE_CIRCLE_RADIUS );				// draw cross hairs
		SLCD_SendCmd(buf);
	}
	
	if (step < eLOCATE_STEP4)
	{ 
		SLCD_SendCmd("p 3");		// set pen width to 3 pixels wide
		///////////////////////////  draw the vertical line  /////////////////////////////////////////////////////////
		sprintf(buf,"l %d %d %d %d", x, y, x, y-length );
		SLCD_SendCmd(buf);
	
		////////////////////////////// draw arrow head ///////////////////////////////
		//	tr x0 y0 x1 y1 x2 y2 [RGB]
		sprintf(buf,"tr %d %d %d %d %d %d 000",
			x- w/2, y-length + w, 
			x+ w/2, y-length + w, 
			x, y-length  		
			);
		SLCD_SendCmd(buf);
	
		///////////////////////////  draw the HORIZONTAL line  /////////////////////////////////////////////////////////
		sprintf(buf,"l %d %d %d %d", x, y, x+length, y );
		SLCD_SendCmd(buf);
		
		////////////////////////////// draw arrow head ///////////////////////////////
		sprintf(buf,"tr %d %d %d %d %d %d 000",
			x+length - w/2, y-w/2, 
			x+length - w/2, y+w/2, 
			x+length, y  		
			);
		SLCD_SendCmd(buf);
	}
	
	//SLCD_SendCmd("f24B");
	SLCD_SendCmd("fUARIAL16");
	SetDefaultBackForegnd();
	
	switch (step)
	{	
		case eLOCATE_START:
			/////////////////////////////////// draw REF text /////////////////////////////////////////////////////////
			sprintf(buf, "tf %d \"R1\" %d %d N", FLASHING_REF_TEXT_INDEX, x - 12, y-12);
			SLCD_SendCmd(buf);
		
			sprintf(buf, "t \"R2\" %d %d", x +length + w , y-12);
			SLCD_SendCmd(buf);
			
			sprintf(buf, "t \"R3\" %d %d", x - 12, y-(length+w+16));
			SLCD_SendCmd(buf);
			break;
		case eLOCATE_STEP1:
			/////////////////////////////////// draw REF text /////////////////////////////////////////////////////////
			sprintf(buf, "tf %d \"R1\" %d %d N", FLASHING_REF_TEXT_INDEX, x - 12, y-12);
			SLCD_SendCmd(buf);
		
			sprintf(buf, "t \"R2\" %d %d", x +length + w  , y-12);
			SLCD_SendCmd(buf);
			
			sprintf(buf, "t \"R3\" %d %d", x - 12, y-(length+w+16));
			SLCD_SendCmd(buf);
			break;
		case eLOCATE_STEP2:
			/////////////////////////////////// draw REF text /////////////////////////////////////////////////////////
			sprintf(buf, "t \"R1\" %d %d", x - 12, y-12);
			SLCD_SendCmd(buf);
		
			sprintf(buf, "tf %d \"R2\" %d %d N", FLASHING_REF_TEXT_INDEX, x +length + w  , y-12);
			SLCD_SendCmd(buf);
			
			sprintf(buf, "t \"R3\" %d %d", x - 12, y-(length+w+16));
			SLCD_SendCmd(buf);
			break;
		case eLOCATE_STEP3:
			/////////////////////////////////// draw REF text /////////////////////////////////////////////////////////
			sprintf(buf, "t \"R1\" %d %d", x - 12, y-12);
			SLCD_SendCmd(buf);
		
			sprintf(buf, "t \"R2\" %d %d", x +length + w , y-12);
			SLCD_SendCmd(buf);
			
			sprintf(buf, "tf %d \"R3\" %d %d N", FLASHING_REF_TEXT_INDEX, x - 12, y-(length+w+16));
			SLCD_SendCmd(buf);
			break;
		case eLOCATE_STEP4:
			/////////////////////////////////// draw REF text /////////////////////////////////////////////////////////
			sprintf(buf, "t \"R1\" %d %d", x - 12, y-12);
			SLCD_SendCmd(buf);
			break;
		case eLOCATE_STEP5:
			break;
		default:	
			break;	
	}	
	
}


void SU_Update_Locate_Msg(MuData* p, eLocateStep step, int error)
{
	char buf[50];

	#ifdef RUSSIAN
		switch (step)
		{	
			case eLOCATE_START:
				SLCD_WriteFooter(
				"\xD0\x9E\xD0\xBF\xD1\x80\xD0\xB5\xD0\xB4\xD0\xB5\xD0\xBB\xD0\xB8\xD1\x82\xD0\xB5 \xD1\x82\xD0\xBE\xD1\x87\xD0\xBA\xD0\xB8 R1, R2 \xD0\xB8 R3",
				"\xD0\xA0\xD0\xB0\xD1\x81\xD0\xBF\xD0\xBE\xD0\xBB\xD0\xBE\xD0\xB6\xD0\xB8\xD1\x82\xD0\xB5 \xD0\xBF\xD1\x80\xD0\xB8\xD1\x91\xD0\xBC\xD0\xBD\xD1\x8B\xD0\xB9 \xD0\xB7\xD0\xBE\xD0\xBD\xD0\xB4 \xD0\xB2 R1", 
				"\xD0\x9D\xD0\xB0\xD0\xB6\xD0\xBC\xD0\xB8\xD1\x82\xD0\xB5 * \xD0\xB4\xD0\xBB\xD1\x8F \xD0\xBD\xD0\xB0\xD1\x87\xD0\xB0\xD0\xBB\xD0\xB0 \xD0\xB8\xD0\xB7\xD0\xBC\xD0\xB5\xD1\x80\xD0\xB5\xD0\xBD\xD0\xB8\xD0\xB9"
				);
				break;
			case eLOCATE_STEP1:
				SLCD_WriteFooter(
					"\xD0\xA0\xD0\xB0\xD1\x81\xD0\xBF\xD0\xBE\xD0\xBB\xD0\xBE\xD0\xB6\xD0\xB8\xD1\x82\xD0\xB5 \xD0\xBF\xD1\x80\xD0\xB8\xD1\x91\xD0\xBC\xD0\xBD\xD1\x8B\xD0\xB9 \xD0\xB7\xD0\xBE\xD0\xBD\xD0\xB4",
					"\xD0\xB2 \xD1\x82\xD0\xBE\xD1\x87\xD0\xBA\xD0\xB5 R1",
					"\xD0\x9D\xD0\xB0\xD0\xB6\xD0\xBC\xD0\xB8\xD1\x82\xD0\xB5 * \xD0\xB4\xD0\xBB\xD1\x8F \xD0\xBD\xD0\xB0\xD1\x87\xD0\xB0\xD0\xBB\xD0\xB0 \xD0\xB8\xD0\xB7\xD0\xBC\xD0\xB5\xD1\x80\xD0\xB5\xD0\xBD\xD0\xB8\xD0\xB9"
					);
				break;
			case eLOCATE_STEP2:
				sprintf(buf,"%d\xD0\xBC \xD0\xB2\xD0\xBF\xD1\x80\xD0\xB0\xD0\xB2\xD0\xBE \xD0\xB2 \xD1\x82\xD0\xBE\xD1\x87\xD0\xBA\xD1\x83 R2", g_SuSettings.locate_ref_meters);
				SLCD_WriteFooter(
					"\xD0\x9F\xD0\xB5\xD1\x80\xD0\xB5\xD0\xBC\xD0\xB5\xD1\x81\xD1\x82\xD0\xB8\xD1\x82\xD0\xB5 \xD0\xBF\xD1\x80\xD0\xB8\xD1\x91\xD0\xBC\xD0\xBD\xD1\x8B\xD0\xB9 \xD0\xB7\xD0\xBE\xD0\xBD\xD0\xB4 \xD0\xBD\xD0\xB0",
					buf,
					"\xD0\x9D\xD0\xB0\xD0\xB6\xD0\xBC\xD0\xB8\xD1\x82\xD0\xB5 * \xD0\xB4\xD0\xBB\xD1\x8F \xD0\xBD\xD0\xB0\xD1\x87\xD0\xB0\xD0\xBB\xD0\xB0 \xD0\xB8\xD0\xB7\xD0\xBC\xD0\xB5\xD1\x80\xD0\xB5\xD0\xBD\xD0\xB8\xD0\xB9"
					);
				break;
			case eLOCATE_STEP3:
				sprintf(buf,"R1 \xD0\xBD\xD0\xB0 %d\xD0\xBC \xD0\xB2\xD0\xBF\xD0\xB5\xD1\x80\xD1\x91\xD0\xB4 \xD0\xB2 \xD1\x82\xD0\xBE\xD1\x87\xD0\xBA\xD1\x83 R3", g_SuSettings.locate_ref_meters);
				SLCD_WriteFooter(
					"\xD0\x9F\xD0\xB5\xD1\x80\xD0\xB5\xD0\xBC\xD0\xB5\xD1\x81\xD1\x82\xD0\xB8\xD1\x82\xD0\xB5 \xD0\xBF\xD1\x80\xD0\xB8\xD1\x91\xD0\xBC\xD0\xBD\xD1\x8B\xD0\xB9 \xD0\xB7\xD0\xBE\xD0\xBD\xD0\xB4 \xD0\xB8\xD0\xB7",
					buf,
					"\xD0\x9D\xD0\xB0\xD0\xB6\xD0\xBC\xD0\xB8\xD1\x82\xD0\xB5 * \xD0\xB4\xD0\xBB\xD1\x8F \xD0\xBD\xD0\xB0\xD1\x87\xD0\xB0\xD0\xBB\xD0\xB0 \xD0\xB8\xD0\xB7\xD0\xBC\xD0\xB5\xD1\x80\xD0\xB5\xD0\xBD\xD0\xB8\xD0\xB9"
					);
				break;
			case eLOCATE_STEP4:
				SLCD_WriteFooter(
					"\xD0\x9C\xD0\xB5\xD1\x81\xD1\x82\xD0\xBE\xD0\xBD\xD0\xB0\xD1\x85\xD0\xBE\xD0\xB6\xD0\xB4\xD0\xB5\xD0\xBD\xD0\xB8\xD0\xB5 \xD0\xB8\xD1\x81\xD0\xBA\xD0\xBE\xD0\xBC\xD0\xBE\xD0\xB3\xD0\xBE \xD1\x82\xD0\xB0\xD0\xB3\xD0\xB0",
					"\xD0\xBE\xD1\x82\xD0\xBD\xD0\xBE\xD1\x81\xD0\xB8\xD1\x82\xD0\xB5\xD0\xBB\xD1\x8C\xD0\xBD\xD0\xBE \xD1\x82\xD0\xBE\xD1\x87\xD0\xBA\xD0\xB8 R1",
					"\xD0\x9D\xD0\xB0\xD0\xB6\xD0\xBC\xD0\xB8\xD1\x82\xD0\xB5 * \xD0\xB4\xD0\xBB\xD1\x8F \xD0\xBF\xD0\xBE\xD0\xB2\xD1\x82\xD0\xBE\xD1\x80\xD0\xB0 \xD0\xB8\xD0\xB7\xD0\xBC\xD0\xB5\xD1\x80\xD0\xB5\xD0\xBD\xD0\xB8\xD0\xB9"
					);
				break;
			case eLOCATE_STEP5:
				SLCD_WriteFooter(
					"Estimated position shown",
					"by cicle intersections", 
					"\xD0\x9D\xD0\xB0\xD0\xB6\xD0\xBC\xD0\xB8\xD1\x82\xD0\xB5 * \xD0\xB4\xD0\xBB\xD1\x8F \xD0\xBF\xD0\xBE\xD0\xB2\xD1\x82\xD0\xBE\xD1\x80\xD0\xBD\xD0\xBE\xD0\xB9 \xD0\xBF\xD0\xBE\xD0\xBF\xD1\x8B\xD1\x82\xD0\xBA\xD0\xB8"
					);
				break;
			case eLOCATE_ERROR:
			default:
				SLCD_WriteFooter(
					"\xD0\x9D\xD0\xB5 \xD1\x83\xD0\xB4\xD0\xB0\xD0\xBB\xD0\xBE\xD1\x81\xD1\x8C \xD0\xBF\xD1\x80\xD0\xBE\xD0\xB8\xD0\xB7\xD0\xB2\xD0\xB5\xD1\x81\xD1\x82\xD0\xB8 \xD0\xBE\xD1\x86\xD0\xB5\xD0\xBD\xD0\xBA\xD1\x83",
					"\xD1\x80\xD0\xB0\xD1\x81\xD1\x81\xD1\x82\xD0\xBE\xD1\x8F\xD0\xBD\xD0\xB8\xD1\x8F\x3A \xD0\xBF\xD0\xBE\xD1\x82\xD0\xB5\xD1\x80\xD1\x8F \xD1\x81\xD0\xB2\xD1\x8F\xD0\xB7\xD0\xB8 \xD1\x81 \xD1\x82\xD0\xB0\xD0\xB3\xD0\xBE\xD0\xBC",
					"\xD0\x9D\xD0\xB0\xD0\xB6\xD0\xBC\xD0\xB8\xD1\x82\xD0\xB5 * \xD0\xB4\xD0\xBB\xD1\x8F \xD0\xBF\xD0\xBE\xD0\xB2\xD1\x82\xD0\xBE\xD1\x80\xD0\xBD\xD0\xBE\xD0\xB9 \xD0\xBF\xD0\xBE\xD0\xBF\xD1\x8B\xD1\x82\xD0\xBA\xD0\xB8"
					);
				break;			
		}
	#else
		switch (step)
		{	
			case eLOCATE_START:
				sprintf(buf,"Select R1 with %dm to R2 and R3", g_SuSettings.locate_ref_meters);
				SLCD_WriteFooter(buf,"Position antenna at R1", "Press * to measure");
				break;
			case eLOCATE_STEP1:
				SLCD_WriteFooter("Return to R1 position","Position antenna at R1", "Press * to measure");
				break;
			case eLOCATE_STEP2:
				sprintf(buf,"Move %dm right to R2", g_SuSettings.locate_ref_meters);
				SLCD_WriteFooter(buf,"Position antenna at R2", "Press * to measure");
				break;
			case eLOCATE_STEP3:
				sprintf(buf,"Move %dm forward from R1", g_SuSettings.locate_ref_meters);
				SLCD_WriteFooter(buf,"Position antenna at R3", "Press * to measure");
				break;
			case eLOCATE_STEP4:
				SLCD_WriteFooter("Estimated position shown","by the green arrow", "Press * to try again");
				break;
			case eLOCATE_STEP5:
				SLCD_WriteFooter("Estimated position shown","by cicle intersections", "Press * to try again");
				break;
			case eLOCATE_ERROR:
			default:
				SLCD_WriteFooter("An error occured","Measurement unsuccessful.", "Press * to try again");
				break;			
		}
	#endif	
}

void SU_DrawLocateBackground(MuData* pMU, eLocateStep LocateStep, int response_count)
{
	SU_DrawLocateBackgroundPage(pMU);
	SU_UpdateGeneralStatus(TRUE,TRUE);
	SU_Update_Locate_Msg(pMU,LocateStep, response_count);
	SU_Draw_Locate_Reference_Points(LocateStep);
}

double CalculateLocateDistance(XYZ_Reading* p)
{
	U16	peak = max(p->x, p->y);
	peak = max(peak, p->z);
	return RSSI_TO_METERS((double)(g_SuSettings.K1*K_MULTIPLIER),(double)peak);
}


void dumpLocateData(struct LOCATE_DATA* p)
{
	char buffer[60];
	sprintf(buffer,"Locate Data: %1.1f, %1.1f, %1.1f, %1.1f, %1.1f",
		p->d1,
		p->d2,
		p->d3,
		p->angle1,
		p->angle2
		);
	PortPutCRStr(buffer,PC_PORT,1);
}


// O = angle opposite side a
// right of angle O is C and to the left is B
// cos(O) = (b^2 + c^2 - a^2)/(2 x b x c)
// returns angle in radians
double CalculateOppositeAngle(double a, double b, double c)
{
	double denom = (2.0*(b*c));
	double value = (b*b + c*c - (a*a));

	if (denom != 0.0)
		value = value/denom;
	
	if (value <= -1.0)
		return PI;
	else if (value >= 1.0)
		return 0.0;
		
	return acos(value);
}


//#define TEST_CALCULATIONS
#ifdef TEST_CALCULATIONS
// O = angle opposite side a
// right of angle O is C and to the left is B
// a = sqrt(b^2* cos^2(O) + b^2*sin^2(O) + c*c-2*b*c*cos(O)
double CalculateOppositeSideA(double alpha, double b, double c)
{
	double cosO = cos(DEGTORAD(alpha));
	double sinO = sin(DEGTORAD(alpha));
	double value = (b * b) * (cosO * cosO) + (b * b) * (sinO * sinO) + (c * c)-(2.0 * (b * c * cosO));
	if (value < 0.0) return 0.0;
	return sqrt(value);
}

//#define TEST_OMEGA_ANGLE 25.0
//#define TEST_OMEGA_ANGLE 45.0
//#define TEST_OMEGA_ANGLE 55.0
//#define TEST_OMEGA_ANGLE 80.0
//#define TEST_OMEGA_ANGLE 134.0
//#define TEST_OMEGA_ANGLE 220.0
//#define TEST_OMEGA_ANGLE 300.0
#endif

///////////////////////////////////////////// CalculateLocateDirection ///////////////////////////////////////

#define AMBIGUOUS_ANGLE 3.0

void CalculateLocateDirection(struct LOCATE_DATA* p)
{
	////////////////////////////////// calculate angles of two triangles  ///////////////////////////
	double k = ((double)g_SuSettings.locate_ref_meters );
	double alpha = CalculateOppositeAngle(p->d2, p->d1, k);
	double theta = CalculateOppositeAngle(p->d3, k, p->d1);
	alpha = RADTODEG(alpha);
	theta = RADTODEG(theta);
	
	double angle_adjustment = sin((k/2.0)/p->d1);
	double dist_adjusted = sqrt(p->d1*p->d1 + (k*k));
	double ambiguous_diff = p->d1 - (p->d1 * cos(DEGTORAD(AMBIGUOUS_ANGLE)));

	// if d2 is within AMBIGUOUS_ANGLE degrees of 90 or 270 degrees
	if ((fabs(p->d2 - dist_adjusted)) < ambiguous_diff )
	{
		if (p->d3 < p->d1)
			p->angle1 = 90.0;
		else
			p->angle1 = 270.0;
		p->angle2 = p->angle1;	
	}
	// Angle is pretty close to zero
	else if ((fabs((p->d2 + k) - p->d1)) <  ambiguous_diff)
	{
		p->angle1 = 0.0;
		p->angle2 = p->angle1;
	}
	// Angle is pretty close to 180
	else if ((fabs((p->d2 - k) - p->d1)) <  ambiguous_diff)
	{
		p->angle1 = 180.0;
		p->angle2 = p->angle1;
	}
	/////////// else if d3 is within AMBIGUOUS_ANGLE degrees of 180 or 0 degrees
	else if ((fabs(p->d3 - dist_adjusted)) < ambiguous_diff )
	{
		if (p->d2 < p->d1)
			p->angle1 = 0.0;
		else
			p->angle1 = 180.0;		
		p->angle2 = p->angle1;	
	}
	// amgle is pretty close to 90
	else if ((fabs((p->d3 + k) - p->d1)) <  ambiguous_diff)
	{
		p->angle1 = 90.0;
		p->angle2 = p->angle1;
	}
	// Angle is pretty close to 270
	else if ((fabs((p->d3 - k) - p->d1)) <  ambiguous_diff)
	{
		p->angle1 = 270.0;
		p->angle2 = p->angle1;
	}
	// else the values are not ambiguous and can be use to calculate angles
	else if (p->d2 < dist_adjusted)		// quadrant 1 or 4
	{
		if (p->d3 < dist_adjusted)	//  quadrant is 1
		{
			p->angle1 = alpha;
			p->angle2 = (90.0 - theta);
		}
		else				//  quadrant is 4
		{
			p->angle1 = 360.0 - alpha;
			p->angle2 = 360 - (theta - 90.0);
		}
		
	}
	else					//  quadrant is 2 or 3
	{
		if (p->d3 < dist_adjusted)	//  quadrant is 2
		{
			p->angle1 = alpha;
			p->angle2 = 90.0 + theta;
		}
		else				//  quadrant is 3
		{
			p->angle1 = 360.0 - alpha;
			p->angle2 = 90.0 + theta;
		}		
	}
	
	if (g_UnitFlags.DebugMisc)
		dumpLocateData(p);
	
}

#ifdef TEST_CALCULATIONS
void Test_Calculations()
{
	struct LOCATE_DATA data;
	CalculateLocateDirection(&data);
}
#endif

// wait a maximum amount of time for an ACK from the TX to say it is finished
void WaitForSUTXResponse(unsigned int msecs)
{
	unsigned long long lasttick = g_tick;
	unsigned long long elapsedms=0;
	int response_count = 0;
	
	clear_MU_TX_PingCount();
	lasttick = g_tick;
	do {
		ProcessSUTXStream();
		if (g_tick != lasttick) // check at 1ms intervals
		{
			if (g_tick > lasttick)
				elapsedms += g_tick-lasttick;	
			else
				elapsedms++;	// account for timer overflow
						
			lasttick = g_tick;
		}
		response_count = get_MU_TX_PingCount();
	}
	while ((response_count == 0) && (elapsedms < msecs));	
}

/////////////////////////////////////////////// SendandWait_LocateMessage /////////////////////////////////////////////////
#define P2_WAIT_FOR_LOCATE_RESPONSE_MS 2500
int SendandWait_LocateMessage(MuData* p, int seconds, XYZ_Reading* pPeaks)
{
	int count;
	unsigned long long lasttick;
	unsigned long long elapsedms=0;
	time_t end_seconds = g_seconds + seconds + 1;;
	int response_count;
	int start_packet_count = p->packet_count;
	XYZ_Reading this_reading;
	
	pPeaks->x = 0;
	pPeaks->y = 0;
	pPeaks->z = 0;
	
	// Wait for a response for up to 3 times
	for (count = 0; count < 3; count++)
	{
		g_bTransmitting = TRUE;	// SET MODE TO TRANSMITTING TO ENSURE RSSI PACKETS IGNORED
		if (g_SuSettings.enable_audio_blanking) AUDIO_MUTE_LATCH = 1;	//normally low to enable the audio board and is high to mute the audio output.
		DelayMsecs(AUDIO_MUTE_DELAY_MS);				// delay to allow audio to be muted
	
		vlf_SendLocateMsg(p->id_u32, seconds, TX_ANTENNA_X);
//		DelayMsecs(LOCATE_RX_DELAY_MS_P2);	
		WaitForSUTXResponse(LOCATE_RX_DELAY_MS_P2);					// SU_TX can't receive during the TX period
		vlf_SendLocateMsg(p->id_u32, seconds, TX_ANTENNA_Y);
//		DelayMsecs(LOCATE_RX_DELAY_MS_P2);						// SU_TX can't receive during the TX period
		WaitForSUTXResponse(LOCATE_RX_DELAY_MS_P2);						// SU_TX can't receive during the TX period
		vlf_SendLocateMsg(p->id_u32, seconds, TX_ANTENNA_Z);
		
//		DelayMsecs(LOCATE_RX_DELAY_MS_P2);			// Delay to prevent audio of TX from being picked up		
		WaitForSUTXResponse(LOCATE_RX_DELAY_MS_P2);						// SU_TX can't receive during the TX period
		AUDIO_MUTE_LATCH = 0;						// normally low to enable the audio board and is high to mute the audio output.
		g_bTransmitting = FALSE;					// RE-ENABLE RSSI PACKET UPDATES
		set_SU_VLF_RX_Mode(SU_RX_MODE_DECODE);		// tell the MURX module to decode regular messages until we get the response
		
		
		// Wait for up to 2 seconds for a response
		// Note that when packets come in, they will automatically update a peakhold on x,y and z 
		end_seconds = g_seconds + seconds + 2;
		
		lasttick = g_tick;
		do {
			ProcessSURXStream();
			if (g_tick != lasttick) // check at 1ms intervals
			{
				if (g_tick > lasttick)
					elapsedms += g_tick-lasttick;	
				else
					elapsedms++;	// account for timer overflow
							
				lasttick = g_tick;
			}
			response_count = p->packet_count - start_packet_count;
		}
		while ((response_count == 0) && (elapsedms < P2_WAIT_FOR_LOCATE_RESPONSE_MS));	

		if (response_count)	// got a response so exit retry loop
			break;
	}
	
		// If we got a response then wait for rest of loop
	if (response_count)
	{
		pPeaks->x = p->x1_power;
		pPeaks->y = p->y1_power;
		pPeaks->z = p->z1_power;
		response_count = p->packet_count;
		
		while (g_seconds < end_seconds)
		{
			ProcessSURXStream();
			if (response_count != p->packet_count)
			{
				pPeaks->x = max(pPeaks->x, p->x1_power);
				pPeaks->y = max(pPeaks->y, p->y1_power);
				pPeaks->z = max(pPeaks->z, p->z1_power);
				response_count = p->packet_count;

				/////////////////////////////// update distance value ////////////////////////
				this_reading.x = p->x1_power;
				this_reading.y = p->y1_power;
				this_reading.z = p->z1_power;

				CalculateLocateDistance(&this_reading);
				ShowLocateDistance(CalculateLocateDistance(pPeaks)," ");
			}
		}
	}
	
	set_SU_VLF_RX_Mode(SU_RX_MODE_STANDBY);	// tell the MURX module to wait until it is told to decode
		
	return (p->packet_count - start_packet_count);
}

///////////////////////////////////////////// SU_Run_P2_LocatePage //////////////////////////////////////////

#define LOCATE_PEAK_HISTORY 10
void SU_Run_P2_LocatePage(int index)
{
	struct button btn;
	time_t ref_second = g_seconds;
	int seconds = 0;
	eLocateStep LocateStep = eLOCATE_START;
	int response_count = 0;
	XYZ_Reading Ref1Peaks,Ref2Peaks,Ref3Peaks;
	eLocateStep previousStep;
	unsigned short taskStatus;
	MuData* pMU = &g_MuDeviceList[index];
	struct LOCATE_DATA aLocateData[LOCATE_PEAK_HISTORY];
	int locate_history_ix = 0;
	int i;
		
	StartPingStatus();
	set_SU_VLF_RX_Mode(SU_RX_MODE_STANDBY);	// tell the MURX module to wait until it is told to decode
	send_SU_RX_ZC_Trigger();				// set the trigger level on the SU_RX

	SU_DrawLocateBackground(pMU, LocateStep, 0);
	
	DelayMsecs(200);		// delay to prevent user from pressing the * key twice in a row to skip the start phase
    
	while (1) {				// loop until we get an exit from the page

		if (ref_second != g_seconds) {
			ref_second = g_seconds;
			SU_UpdateGeneralStatus(TRUE,FALSE);
			seconds++;
			if (seconds >= 5)
			{
				SU_UpdateCOMStatus(FALSE);	// check to see if we got a response from the last ping
				StartPingStatus();		// reset and check again
				seconds = 0;
			}

		}
		
		taskStatus = ProcessBackgroundTasks();

		btn = GetUserAction(taskStatus);
		previousStep = LocateStep;
		
		if (btn.index != 0) {
			switch (btn.index) {
				case SU_BTN_UP:
					break;
				case SU_BTN_DOWN:
					break;
				case SU_BTN_EXIT:
					{				
						set_SU_VLF_RX_Mode(SU_RX_MODE_STANDBY);	// tell the MURX module to wait until it is told to decode
						DelayMsecs(20);	// delay a bit to allow any screen update activity to finish
						return;
					}
					break;
				case SU_BTN_ENTER:	
					switch (LocateStep)
					{	
						case eLOCATE_START:
						case eLOCATE_STEP1:
							#ifdef RUSSIAN
								SU_ShowDistanceMsg("R1: \xD0\x9E\xD1\x86\xD0\xB5\xD0\xBD\xD0\xBA\xD0\xB0 \xD1\x80\xD0\xB0\xD1\x81\xD1\x81\xD1\x82\xD0\xBE\xD1\x8F\xD0\xBD\xD0\xB8\xD1\x8F");
							#else
								SU_ShowDistanceMsg("Measuring d1");
							#endif	
							response_count = SendandWait_LocateMessage(pMU, g_SuSettings.locate_duration_seconds, &Ref1Peaks);	
							if (response_count)
							{
								aLocateData[locate_history_ix].d1 = CalculateLocateDistance(&Ref1Peaks);
								SU_DrawLocateBackground(pMU, eLOCATE_STEP2, response_count);		// Draw next page
								SU_DrawLocateDistance(eLOCATE_STEP1, aLocateData, locate_history_ix);			// Draw results from this page
								LocateStep = eLOCATE_STEP2;		// move to next page
							}
							else
								SU_DrawLocateBackground(pMU, eLOCATE_ERROR, response_count);		// redraw current page
							break;
						case eLOCATE_STEP2:
							#ifdef RUSSIAN
								SU_ShowDistanceMsg("R2: \xD0\x9E\xD1\x86\xD0\xB5\xD0\xBD\xD0\xBA\xD0\xB0 \xD1\x80\xD0\xB0\xD1\x81\xD1\x81\xD1\x82\xD0\xBE\xD1\x8F\xD0\xBD\xD0\xB8\xD1\x8F");
							#else
								SU_ShowDistanceMsg("Measuring d2");
							#endif							
							response_count = SendandWait_LocateMessage(pMU, g_SuSettings.locate_duration_seconds, &Ref2Peaks);	
							if (response_count)
							{
								aLocateData[locate_history_ix].d2 = CalculateLocateDistance(&Ref2Peaks);
								SU_DrawLocateBackground(pMU, eLOCATE_STEP3, response_count);		// Draw next page
								SU_DrawLocateDistance(eLOCATE_STEP2, aLocateData, locate_history_ix);			// Draw results from this page
								LocateStep++;		// move to next page
							}
							else
								SU_DrawLocateBackground(pMU, eLOCATE_ERROR, response_count);		// redraw current page
							
							break;
						case eLOCATE_STEP3:
							#ifdef RUSSIAN
								SU_ShowDistanceMsg("R3: \xD0\x9E\xD1\x86\xD0\xB5\xD0\xBD\xD0\xBA\xD0\xB0 \xD1\x80\xD0\xB0\xD1\x81\xD1\x81\xD1\x82\xD0\xBE\xD1\x8F\xD0\xBD\xD0\xB8\xD1\x8F");
							#else
								SU_ShowDistanceMsg("Measuring d3");
							#endif							
							response_count = SendandWait_LocateMessage(pMU, g_SuSettings.locate_duration_seconds, &Ref3Peaks);	
							if (response_count)
							{
								aLocateData[locate_history_ix].d3 = CalculateLocateDistance(&Ref3Peaks);
								LocateStep++;		// move to next page
#ifdef TEST_CALCULATIONS
								struct LOCATE_DATA test_data;
								double angle;
								SU_DrawLocateBackground(pMU, eLOCATE_STEP4, response_count);					// Draw next page
								for (angle = 0.0; angle <= 360.0; angle += 5.0)
								{
									test_data.d1 = 7;
									test_data.d2 = CalculateOppositeSideA(angle, test_data.d1, (double)g_SuSettings.locate_ref_meters);
									test_data.d3 = CalculateOppositeSideA(angle-90, (double)g_SuSettings.locate_ref_meters, test_data.d1);
									CalculateLocateDirection(&test_data);
									SU_DrawLocateDistance(eLOCATE_STEP4, &test_data, 1);			// Draw final results
								}
#endif							
								CalculateLocateDirection(&aLocateData[locate_history_ix]);
								SU_ShowDistanceMsg("");
								SU_DrawLocateDistance(eLOCATE_STEP3, aLocateData, locate_history_ix);			// Draw final results
								DelayMsecs(2000);
								SU_DrawLocateBackground(pMU, eLOCATE_STEP4, response_count);					// Draw next page
								SU_DrawLocateDistance(eLOCATE_STEP4, aLocateData, locate_history_ix);			// Draw final results
							}
							else
								SU_DrawLocateBackground(pMU, eLOCATE_ERROR, response_count);		// redraw current page

							break;
						case eLOCATE_STEP4:
						case eLOCATE_STEP5:
							if (locate_history_ix < (LOCATE_PEAK_HISTORY-1))
								locate_history_ix++;
							else
							{
								// roll history window back in time
								for (i=0;i<(LOCATE_PEAK_HISTORY-1);i++)
									aLocateData[i] = aLocateData[i+1];
							}
							LocateStep = eLOCATE_STEP1;
							SU_DrawLocateBackground(pMU, LocateStep, 0); 
							response_count = 0;
							break;
						default:	
							break;	
					}
					break;
				case SU_BTN_F1F2:
					if ((LocateStep == eLOCATE_STEP4) || (LocateStep == eLOCATE_STEP5))
					{
						if (LocateStep == eLOCATE_STEP4)
							LocateStep = eLOCATE_STEP5;
						else 
							LocateStep = eLOCATE_STEP4;
						SU_DrawLocateBackground(pMU, LocateStep, response_count);					// Draw next page
						SU_DrawLocateDistance(LocateStep, aLocateData, locate_history_ix);			// Draw final results
					}	
					break;
//				case SU_BTN_UPDOWN:
//					SU_RunSettingsPage();
//					SU_DrawLocatePage();
//					SU_UpdateGeneralStatus(TRUE,TRUE);
//					break;	
				default:
					break;
			}

		}
	}
}

#endif  // not DISTANCE_ONLY_METHOD

