#include "su_slcd.h"
#include <stdio.h>
#include <stdlib.h>
#include "vlf_rxtx.h"
#include "string.h"
#include "sdfunctions.h"
#include "trace.h"
#include "vlf_pkt.h"

extern SuSettings g_SuSettings;

//////////////////////////////////////// local globals ////////////////////////////////////////
int g_iSelectedMuIndex = -1;
int g_iMuListDisplayBase = 0;
static BOOL g_bSearchPaused = FALSE;
static BOOL g_bSearching = FALSE;
static BOOL m_bLocateMode = FALSE;
BOOL g_bTransmitting = FALSE;
static BOOL g_bAudioSearchMode = FALSE;

/////////////////////////////////////////// local prototypes ////////////////////////////
void SU_DrawSearchGridTitles();
BOOL SU_isSearchRowVisible(int index);
void SU_DrawSearchPageGrid();
void SU_DrawSearchPageRowGrid(int row);
void SU_DrawSearchPage();
#ifdef PROTOCOL_2
void SU_UpdateP2SearchStatus(int mode);
#endif
#ifdef PROTOCOL_1
void SU_UpdateSearchStatus(int mode, char antenna, int retry, BOOL bMultipleRetries);
#endif

void SU_DrawMUSearchData(int index, BOOL bSelected);
void SU_UpdateCountDisplay();


void SU_SendAndWaitForMU_P1_Mask(MuData* p, char antenna);
void SU_MaskAllUnmaskedMU_P2_Units(void);
void SU_SendAndWaitForMU_P2_Mask(MuData* p, char antenna);

static void SU_Update_Search_RSSI_StripChart();
static void SU_Init_Search_RSSI_StripChart();

//FULL_MASK defines and prototypes /////////////////
extern BOOL SU_SendAndWaitForMU_P2_FullMask(U32 id_u32, char antenna, char minutes);
extern BOOL SU_SendAndWaitForMU_P2_FullMask_3(U32 id_u32, char antenna, char minutes);
extern void SU_RemoveUnitFromMUList(int iRemove);
extern void SU_AddHPTtoFullMaskList(U32 id_u32);
BOOL SU_SendAndWaitForMU_P2_FullMask_Search(MuData* p, char antenna, char minutes);
BOOL SU_SendAndWaitForMU_P2_FullMask_3_Search(MuData* p, char antenna, char minutes);
//FULL_MASK END

//////////////////////////////////////////// local defines ///////////////////////////////

#define SEARCH_MODE_ACTIVE 1
#define SEARCH_MODE_PAUSED 2
#define SEARCH_MODE_FINISHED 3

#define SEARCH_INDENT_FROM_GRID 4
#define SEARCH_GRID_ROW_HEIGHT 26

#define ID_SEARCH_GRID_TITLE_YPOS (PAGE_START_BELOW_HEADER + 34)
#define ID_SEARCH_GRID_START_YPOS (ID_SEARCH_GRID_TITLE_YPOS + SEARCH_GRID_ROW_HEIGHT)
#define ID_SEARCH_GRID_MAX_ROWS 10
#define ID_SEARCH_GRID_END_YPOS (((ID_SEARCH_GRID_MAX_ROWS)*SEARCH_GRID_ROW_HEIGHT) + ID_SEARCH_GRID_START_YPOS)

#define AUDIO_SEARCH_GRID_TITLE_YPOS 228
#define AUDIO_SEARCH_GRID_START_YPOS (AUDIO_SEARCH_GRID_TITLE_YPOS+SEARCH_GRID_ROW_HEIGHT)
#define AUDIO_SEARCH_GRID_MAX_ROWS 5
#define AUDIO_SEARCH_GRID_END_YPOS (((AUDIO_SEARCH_GRID_MAX_ROWS)*SEARCH_GRID_ROW_HEIGHT) + AUDIO_SEARCH_GRID_START_YPOS)


//	#define SEARCH_PAGE_STATUS_XPOS (SLCD_MAX_X/2-20)
#define SEARCH_PAGE_STATUS_XPOS 30

#define SEARCH_PAGE_STATUS_YPOS (SLCD_MAX_Y - 64)
#define SEARCH_PAGE_COUNTSTATS_XPOS 5
#define SEARCH_PAGE_COUNTSTATS_YPOS (SLCD_MAX_Y - 90)

#define SEARCH_GRID_COL0_XPOS 0
#define SEARCH_GRID_COL1_XPOS 110
#define SEARCH_GRID_COL2_XPOS 140
#define SEARCH_GRID_COL3_XPOS 200

#define AUDIOSEARCH_FORMAT 	0
#define ID_SEARCH_FORMAT 	1
#define MASKING_FORMAT   	2
#define SEARCHPAUSED_FORMAT 3
#define SEARCHDONE_FORMAT   4
#define FULL_MASKING_FORMAT 5






/*
Command: cd n x0 y0 x1 y1 t dw bv tv bc <pens>
Arguments: n - chart index from 0 to 9 (maximum 10 charts).
			x0, y0 and x1, y1 are the top left corner and bottom right corners of the chart area
			t - chart type; must be 0, 1, or 3 (see Description, above)
			dw - data width, number of pixels horizontally between chart data points
			bv - bottom data value (lowest y value)
			tv - top data value (highest y value)
			bc - background color in RGB format (3 ASCII hex characters – see SET COLOR DETAILED)
			<pens> - one or more sets of two values: pen width and pen color. Width is 1 or 2, color is same format as "bc" parameter.
Example: cd 0 10 20 110 120 1 4 0 99 333 2 0FF 1 F00
Defines a chart in the rectangular area (10,20), (110,120). Each data value will be 4 horizontal pixels wide. 
The chart (‘Y’) values are scaled from 0 to 99. The background color is dark gray (333). 
Two pens are defined: the first is pen width 2, color teal (0FF), the second is pen width 1, color red (F00).
SLCD+/
*/

#define AUDIO_RSSI_STRIPCHART_Y_START 90
#define AUDIO_RSSI_STRIPCHART_Y_END 206
#define AUDIO_RSSI_STRIPCHART_RANGE (AUDIO_RSSI_STRIPCHART_Y_END-AUDIO_RSSI_STRIPCHART_Y_START)
static void SU_Init_Search_RSSI_StripChart()
{
	char buffer[80];
	sprintf(buffer,"cd 0 1 %d 270 %d 0 5 0 %d 048 1 F80 1 0F0 1 FF0", 
		AUDIO_RSSI_STRIPCHART_Y_START, 
		AUDIO_RSSI_STRIPCHART_Y_END, 
		AUDIO_RSSI_STRIPCHART_RANGE
	); 
	SLCD_SendCmd(buffer);
//	SLCD_SendCmd("cd 0 1 96 270 200 0 5 0 104 048 1 F80 1 0F0 1 FF0");
}

static void SU_Update_Search_RSSI_StripChart()
{
	char buffer[30];
	unsigned int x,y,z;
	long range = 0x10000l;
	int divider = 16;
	
	x = max(g_peak_Clutter_rssi.x,g_peak_Packet_rssi.x);
	y = max(g_peak_Clutter_rssi.y,g_peak_Packet_rssi.y);
	z = max(g_peak_Clutter_rssi.z,g_peak_Packet_rssi.z);
	
	x /=divider;
	y /=divider;
	z /=divider;
	range /=divider;
	
//	sprintf(buffer, "cv 0 %d %d %d", ScaleLogPower(g_x_lastrssi), ScaleLogPower(g_y_lastrssi), ScaleLogPower(g_z_lastrssi));
	sprintf(buffer, "cv 0 %d %d %d", 
		ScaleLogPowerToMax(x, AUDIO_RSSI_STRIPCHART_RANGE, range), 
		ScaleLogPowerToMax(y, AUDIO_RSSI_STRIPCHART_RANGE, range), 
		ScaleLogPowerToMax(z, AUDIO_RSSI_STRIPCHART_RANGE, range)
		);
	SLCD_SendCmd(buffer);
}

// redraws the ID list
void SU_Search_RedrawIDList()
{
	char buffer[50];
	int row;
	int oldpos = g_iMuListDisplayBase;
	int y;
	int maxrows = g_bAudioSearchMode?AUDIO_SEARCH_GRID_MAX_ROWS:ID_SEARCH_GRID_MAX_ROWS;
	int maxentries = GetMUListSize();
	
	if (g_iSelectedMuIndex >= maxentries)
		g_iSelectedMuIndex = maxentries-1;

	if ((g_iSelectedMuIndex >= 0) && (g_iSelectedMuIndex < MAXIMUM_MU_DEVICES_IN_LIST)) {
		if(g_iSelectedMuIndex < g_iMuListDisplayBase) {
			g_iMuListDisplayBase = g_iSelectedMuIndex;
		}
		else { 		
			if (g_iSelectedMuIndex > (g_iMuListDisplayBase + maxrows - 1)) {
				g_iMuListDisplayBase = g_iSelectedMuIndex - maxrows + 1;
			}
		}
	}

	// scroll rectangle up or down and then redraw the new row
	if (abs(g_iMuListDisplayBase-oldpos) == 1) { // if it only moved by 1 row then scroll the screen
		y = g_bAudioSearchMode?AUDIO_SEARCH_GRID_START_YPOS:ID_SEARCH_GRID_START_YPOS;
		if (g_iMuListDisplayBase > oldpos) {
//			y+= SEARCH_GRID_ROW_HEIGHT;
			sprintf(buffer,"k 0 %d %d %d %du",y,SLCD_MAX_X,y + (maxrows*SEARCH_GRID_ROW_HEIGHT),SEARCH_GRID_ROW_HEIGHT);
			SLCD_SendCmd(buffer);
			y = g_iMuListDisplayBase + maxrows - 1;
			SU_DrawMUSearchData(y, (y == g_iSelectedMuIndex)?TRUE:FALSE);
		}
		else {
			sprintf(buffer,"k 0 %d %d %d %dd",y,SLCD_MAX_X,y + (maxrows*SEARCH_GRID_ROW_HEIGHT),SEARCH_GRID_ROW_HEIGHT);
			SLCD_SendCmd(buffer);
			y = g_iMuListDisplayBase;
			SU_DrawMUSearchData(y, (y == g_iSelectedMuIndex)?TRUE:FALSE);
		}
	}
	else 
	{	// Redraw the whole list of devices visible on the screen
		for (row=g_iMuListDisplayBase;row<g_iMuListDisplayBase+maxrows;row++) {
			if (g_MuDeviceList[row].v.ID_valid == 0) 
				break;
			SU_DrawMUSearchData(row, (row == g_iSelectedMuIndex)?TRUE:FALSE);
		}
	}
	SU_UpdateCountDisplay();
}

// redraws the ID list up to the specified index
void SU_Search_RedrawIDList_ToIndex(int index)
{
	int row;
	int maxrows = g_bAudioSearchMode?AUDIO_SEARCH_GRID_MAX_ROWS:ID_SEARCH_GRID_MAX_ROWS;
	int maxentries = GetMUListSize();
	
	if (g_iSelectedMuIndex >= maxentries)
		g_iSelectedMuIndex = maxentries-1;

	if ((g_iSelectedMuIndex >= 0) && (g_iSelectedMuIndex < MAXIMUM_MU_DEVICES_IN_LIST)) {
		if(g_iSelectedMuIndex < g_iMuListDisplayBase) {
			g_iMuListDisplayBase = g_iSelectedMuIndex;
		}
		else { 		
			if (g_iSelectedMuIndex > (g_iMuListDisplayBase + maxrows - 1)) {
				g_iMuListDisplayBase = g_iSelectedMuIndex - maxrows + 1;
			}
		}
	}

	for (row=g_iMuListDisplayBase;row<g_iMuListDisplayBase+maxrows;row++) 
	{
		SU_DrawMUSearchData(row, (row == g_iSelectedMuIndex)?TRUE:FALSE);
		if (row >= index)
			break;
	}
	SU_UpdateCountDisplay();
}



// Draws the background grid for the units in the search window
void SU_DrawSearchPageGrid()
{
	char buf[30];
	int y;
	int ymax = g_bAudioSearchMode?AUDIO_SEARCH_GRID_END_YPOS:ID_SEARCH_GRID_END_YPOS;

	SLCD_SendCmd("p 1");

	// Draw the Row lines	
	y = g_bAudioSearchMode?AUDIO_SEARCH_GRID_START_YPOS:ID_SEARCH_GRID_START_YPOS;
	for (; y <= ymax; y+= SEARCH_GRID_ROW_HEIGHT) {
		sprintf(buf,"l 0 %d %d %d",y,SLCD_MAX_X,y);
		SLCD_SendCmd(buf);
	}
	
	// Draw the TITLE column lines
	y=g_bAudioSearchMode?AUDIO_SEARCH_GRID_TITLE_YPOS:ID_SEARCH_GRID_TITLE_YPOS;
	SetForeBackGndColor(SLCD_BLACK,SLCD_FOREGROUND);	// DRAW IN BLACK
	sprintf(buf,"l %d %d %d %d",SEARCH_GRID_COL1_XPOS,y,SEARCH_GRID_COL1_XPOS,y + SEARCH_GRID_ROW_HEIGHT);
	SLCD_SendCmd(buf);

	sprintf(buf,"l %d %d %d %d",SEARCH_GRID_COL2_XPOS,y,SEARCH_GRID_COL2_XPOS,y + SEARCH_GRID_ROW_HEIGHT);
	SLCD_SendCmd(buf);

	sprintf(buf,"l %d %d %d %d",SEARCH_GRID_COL3_XPOS,y,SEARCH_GRID_COL3_XPOS,y + SEARCH_GRID_ROW_HEIGHT);
	SLCD_SendCmd(buf);

	
	y=g_bAudioSearchMode?AUDIO_SEARCH_GRID_START_YPOS:ID_SEARCH_GRID_START_YPOS;
	SetDefaultBackForegnd();

	// Draw the GRID column lines
	sprintf(buf,"l %d %d %d %d",SEARCH_GRID_COL1_XPOS,y,SEARCH_GRID_COL1_XPOS,ymax);
	SLCD_SendCmd(buf);

	sprintf(buf,"l %d %d %d %d",SEARCH_GRID_COL2_XPOS,y,SEARCH_GRID_COL2_XPOS,ymax);
	SLCD_SendCmd(buf);

	sprintf(buf,"l %d %d %d %d",SEARCH_GRID_COL3_XPOS,y,SEARCH_GRID_COL3_XPOS,ymax);
	SLCD_SendCmd(buf);

}

void SU_DrawSearchPageRowGrid(int row)
{
	char buf[50];
	int y1,y2;

	SLCD_SendCmd("p 1");
	SetForeBackGndColor(SLCD_BLACK,SLCD_BACKGROUND);	// DRAW IN BLACK

	// Draw the Row lines
	y1 = g_bAudioSearchMode?AUDIO_SEARCH_GRID_START_YPOS:ID_SEARCH_GRID_START_YPOS;
	y1 += (SEARCH_GRID_ROW_HEIGHT * (row - g_iMuListDisplayBase));
	y2 = y1 + SEARCH_GRID_ROW_HEIGHT;

	sprintf(buf,"l 0 %d %d %d",y1,SLCD_MAX_X,y1);		// draw top row-line
	SLCD_SendCmd(buf);

	sprintf(buf,"l 0 %d %d %d",y2,SLCD_MAX_X,y2);		// draw bottom row-line
	SLCD_SendCmd(buf);

	// Draw the column lines
	sprintf(buf,"l %d %d %d %d",SEARCH_GRID_COL1_XPOS,y1,SEARCH_GRID_COL1_XPOS,y2);
	SLCD_SendCmd(buf);

	sprintf(buf,"l %d %d %d %d",SEARCH_GRID_COL2_XPOS,y1,SEARCH_GRID_COL2_XPOS,y2);
	SLCD_SendCmd(buf);

	sprintf(buf,"l %d %d %d %d",SEARCH_GRID_COL3_XPOS,y1,SEARCH_GRID_COL3_XPOS,y2);
	SLCD_SendCmd(buf);

}

// Draws the data corresponding to an MU unit

void SU_DrawSearchGridTitles()
{
	char buffer[50];
	int y = g_bAudioSearchMode?AUDIO_SEARCH_GRID_TITLE_YPOS:ID_SEARCH_GRID_TITLE_YPOS;
	int x = 0;

	sprintf(buffer,"s %d 0",SLCD_LIGHT_GREY);		// set foreground to Black, background to light grey
	SLCD_SendCmd(buffer);

	sprintf(buffer,"r 0 %d %d %d 1",y, SLCD_MAX_X, y+SEARCH_GRID_ROW_HEIGHT);	// draw a rectangle with the foreground color
	SLCD_SendCmd(buffer);

	sprintf(buffer,"s 0 %d",SLCD_LIGHT_GREY);		// set foreground to Black, background to light grey
	SLCD_SendCmd(buffer);

	y++;

#ifdef RUSSIAN
	SLCD_SendCmd("utf8 on");		// sets unicode
	SLCD_SendCmd("fUARIAL12");		// sets the unicode character set for the title
	#define COL1_TITLE "t \" ID\" %d %d"
	#define COL2_TITLE "t \" \xD0\x9C\" %d %d"
	#define COL3_TITLE "t \" \xD0\x94.,\xD0\xBC\" %d %d"
	#define COL4_TITLE "t \" \xD0\x9E\xD1\x82\xD0\xBC\xD0\xB5\xD1\x82\xD0\xBA\xD0\xB8\" %d %d"
#else
	SLCD_SendCmd("f18BC");		// set font size
	#define COL1_TITLE "t \" ID\" %d %d"
	#define COL2_TITLE "t \" M\" %d %d"
	#define COL3_TITLE "t \" D\" %d %d"
	#define COL4_TITLE "t \" Notes\" %d %d"
#endif
	
	x = SEARCH_GRID_COL0_XPOS + SEARCH_INDENT_FROM_GRID;
	sprintf(buffer,COL1_TITLE,x,y);			// id
	SLCD_SendCmd(buffer);

	x = SEARCH_GRID_COL1_XPOS + SEARCH_INDENT_FROM_GRID;
	sprintf(buffer,COL2_TITLE,x,y);			// mobility
	SLCD_SendCmd(buffer);

	x = SEARCH_GRID_COL2_XPOS + SEARCH_INDENT_FROM_GRID;
	sprintf(buffer,COL3_TITLE,x,y);			// distance
	SLCD_SendCmd(buffer);

	x = SEARCH_GRID_COL3_XPOS + SEARCH_INDENT_FROM_GRID;
	sprintf(buffer,COL4_TITLE,x,y);		// NOTES
	SLCD_SendCmd(buffer);

}

BOOL SU_isSearchRowVisible(int row)
{
		// if row is out of range then
	int maxrows = g_bAudioSearchMode?AUDIO_SEARCH_GRID_MAX_ROWS:ID_SEARCH_GRID_MAX_ROWS;
	
	if ((row < g_iMuListDisplayBase) || (row >= (g_iMuListDisplayBase + maxrows))) {
		return FALSE;
	}
	else
		return TRUE;
}


// returns calculated distance value in meters or -1.0 if it could not be calculated
double Get_MU_Distance(MuData* p)
{
	// use 1F if any 1F readings are THRESHOLD_1F or more
	double meters = -1.0;
	unsigned int pkpower;
	if ((p->v.X1_power_valid) && (p->v.Y1_power_valid) && (p->v.Z1_power_valid))
	{
		///////////////////////////////////////// peak RSSI ///////////////////////////////////
		pkpower = max(p->x1_power,p->y1_power);
		pkpower = max(pkpower,p->z1_power);
		
		///////////////////////////////////////// Distance ///////////////////////////////////////////
		if (pkpower > 0)	
			meters = RSSI_TO_METERS(((double)g_SuSettings.K1*K_MULTIPLIER),((double)pkpower));	
	}	
	return meters;
}


void SU_DrawMUSearchData(int index, BOOL bSelected)
{
	char buffer[80];
	int y;
	int x = 0;
	BOOL bIDValid = FALSE;
	int value;
	char maskedCH = '_';
	int ymax = g_bAudioSearchMode?AUDIO_SEARCH_GRID_END_YPOS:ID_SEARCH_GRID_END_YPOS;
	
	if ((index < 0) || (index >= MAXIMUM_MU_DEVICES_IN_LIST))	// make sure index is valid
		return;

	MuData* p = &g_MuDeviceList[index];

	// if row is out of range then simply return
	if (!SU_isSearchRowVisible(index)) return;

//	SLCD_SendCmd("f24B");	// set font size
//	SLCD_SendCmd("f14x24");	// set font size
	SLCD_SendCmd("f12x24");	// set font size
	y = g_bAudioSearchMode?AUDIO_SEARCH_GRID_START_YPOS:ID_SEARCH_GRID_START_YPOS;
	y += (SEARCH_GRID_ROW_HEIGHT * (index - g_iMuListDisplayBase));
	if (y > (ymax - SEARCH_GRID_ROW_HEIGHT))
		return;

	y+=1;
	// draw the background selection color
	if (bSelected) {
		SetForeBackGndColor((m_bLocateMode)?MENU_EDIT_COLOR:MENU_SELECTED_COLOR,SLCD_BLACK);
	}
	else {
		SetForeBackGndColor(SLCD_BACKGROUND,SLCD_FOREGROUND);
	}
	
/*	sprintf(buffer,"r %d %d %d %d 1",0,y, SEARCH_GRID_COL1_XPOS -1, y + SEARCH_GRID_ROW_HEIGHT -2);			// draw a rectangle with the foreground color
	SLCD_SendCmd(buffer);
	sprintf(buffer,"r %d %d %d %d 1",SEARCH_GRID_COL1_XPOS + 1,y, SEARCH_GRID_COL2_XPOS -1, y + SEARCH_GRID_ROW_HEIGHT -2);			// draw a rectangle with the foreground color
	SLCD_SendCmd(buffer);
	sprintf(buffer,"r %d %d %d %d 1",SEARCH_GRID_COL2_XPOS + 1,y, SEARCH_GRID_COL3_XPOS -1, y + SEARCH_GRID_ROW_HEIGHT -2);			// draw a rectangle with the foreground color
	SLCD_SendCmd(buffer);
	sprintf(buffer,"r %d %d %d %d 1",SEARCH_GRID_COL3_XPOS + 1,y, SLCD_MAX_X -1, y + SEARCH_GRID_ROW_HEIGHT -2);			// draw a rectangle with the foreground color
	SLCD_SendCmd(buffer);
*/
	sprintf(buffer,"r %d %d %d %d 1",0,y, SLCD_MAX_X -1, y + SEARCH_GRID_ROW_HEIGHT -2);			// draw a rectangle with the foreground color
	SLCD_SendCmd(buffer);


	// draw the text forground
	if (bSelected)
		SetForeBackGndColor(SLCD_BLACK,SLCD_YELLOW);
	else
		SetDefaultBackForegnd();

	x = SEARCH_GRID_COL0_XPOS + SEARCH_INDENT_FROM_GRID;

	if (p->v.ID_valid) 
	{
		sprintf(buffer,"t \"%8lu\" %d %d",(p->id_u32)&0x00ffffff,x,y);	// upper byte is reserved for type
		bIDValid = TRUE;
	}
	else
		sprintf(buffer,"t \"         \" %d %d",x,y);
	SLCD_SendCmd(buffer);


	x = SEARCH_GRID_COL1_XPOS + SEARCH_INDENT_FROM_GRID;
	if (bIDValid)
	{
		if (p->v.Mobility_valid) 
		{			
			#ifdef PROTOCOL_1
				value =  min(15,p->m.mobility);
				maskedCH = '_';
			#elif PROTOCOL_2					
				value = min(15,p->m.status.mobility);
				if (p->m.status.masked) 
					maskedCH = 'M';
			#else
				#error PROTOCOL NOT DEFINED					
			#endif		
			sprintf(buffer,"t \"%d\" %d %d",(int)(value),x,y);
		}
		else
			sprintf(buffer,"t \"%s\" %d %d",!bIDValid?" ":"_",x,y);
	}
	else
	{
		sprintf(buffer,"t \"  \" %d %d",x,y);
	}
		
	SLCD_SendCmd(buffer);



	x = SEARCH_GRID_COL2_XPOS + SEARCH_INDENT_FROM_GRID;
	if (bIDValid)
	{
		double meters = Get_MU_Distance(p);	
	//	if ((p->v.X1_power_valid) && (p->v.Y1_power_valid) && (p->v.Z1_power_valid))
	//	{
	//		int peak = max(p->x1_power,p->y1_power);
	//		peak = max(p->z1_power,peak);
	//		value =  ScaleLogPower(peak);
	//		sprintf(buffer,"t \"%d\" %d %d",(int)(value),x,y);
	//		
	//	}
		if (meters >= 0.0)
		{
			sprintf(buffer,"t \"%4.1f\" %d %d",meters,x,y);	
		}
		else
			sprintf(buffer,"t \"_     \" %d %d",x,y);
	}
	else
	{
		sprintf(buffer,"t \"     \" %d %d",x,y);
	}
	SLCD_SendCmd(buffer);
		
	x = SEARCH_GRID_COL3_XPOS + SEARCH_INDENT_FROM_GRID;
	if (bIDValid)
	{
		switch (p->antenna)  
		{
			case TX_ANTENNA_X:
			case TX_ANTENNA_Y:
			case TX_ANTENNA_Z:
				#ifdef PROTOCOL_1
					sprintf(buffer,"t \"A=%c  \" %d %d",p->antenna,x,y);
				#elif PROTOCOL_2					
					sprintf(buffer,"t \"%c %3d\" %d %d",maskedCH,p->packet_count+1,x,y);			
				#else
				#error PROTOCOL NOT DEFINED					
				#endif
				break;
			default:
				#ifdef PROTOCOL_1
					sprintf(buffer,"t \%s    \" %d %d",!bIDValid?" ":"_",x,y);
				#elif PROTOCOL_2					
					sprintf(buffer,"t \"%c %3d\" %d %d",maskedCH,p->packet_count+1,x,y);
				#else
				#error PROTOCOL NOT DEFINED					
				#endif
			break;
		}
	}
	else
	{
		sprintf(buffer,"t \"      \" %d %d",x,y);
	}	
	SLCD_SendCmd(buffer);
	
	SU_DrawSearchPageRowGrid(index);
}


void SU_DrawSearchPage()
{
    // Logo splash screen
	int y = NAVIGATION_BTN_YPOS;
	int adder = NAVIGATION_BTN_BMP_WIDTH_WITH_BORDER;
	int x = SLCD_MAX_X - (adder * 5);


	SetDefaultBackForegnd();		
    SLCD_SendCmd(CLEARSCREEN);	// clear screen
	SLCD_SendCmd("ta LT");		// set test orientation to left, top
    SLCD_SendCmd("o 0 0");	// set origin to 0,0

#ifdef INCLUDE_MINERADIO_HEADER
	DRAW_MINERADIO_HEADER;
	DRAW_MINERADIO_FOOTER;
#endif

	SetDefaultBackForegnd();		
	
#ifdef RUSSIAN
	SLCD_SendCmd("fUARIAL18");		// sets the unicode character set for the title
	if (g_bAudioSearchMode)
		SLCD_SendCmd("t \"\xD0\x91\xD1\x8B\xD1\x81\xD1\x82\xD1\x80\xD1\x8B\xD0\xB9 \xD0\xBF\xD0\xBE\xD0\xB8\xD1\x81\xD0\xBA\" 60 50 T");
	else
		SLCD_SendCmd("t \"ID \xD0\xBF\xD0\xBE\xD0\xB8\xD1\x81\xD0\xBA\" 90 50 T");

#else	
	SLCD_SendCmd("f32");
	if (g_bAudioSearchMode)
		SLCD_SendCmd("t \"Quick Search\" 65 50 T");
	else
		SLCD_SendCmd("t \"ID Search\" 65 50 T");
#endif
		
	SU_DrawSearchGridTitles();

	SetDefaultBackForegnd();		
	SU_DrawSearchPageGrid();

	SU_InitMainBatteryBar();	// creates and display battery bar
	SU_InitTXBatteryBar();

//    SLCD_SendCmd("f 12x24");		// set font size		
//	sprintf(buffer,"bdc %d %d %d 1 \"%s\" %d %d",SU_BTN_LOCATE,x,y,"L",NAVIGATION_BTN_UP_BMP,NAVIGATION_BTN_DN_BMP);
//	SLCD_SendCmd(buffer);	
	
	SU_DisplayStandardNavigationButtons(x+adder,y, adder,TRUE);

//	x += adder;
//	sprintf(buffer,"bdc %d %d %d 2 \"%s\" \"%s\" %d %d",SU_BTN_PAUSE_RESUME,x,y,"||",">>",NAVIGATION_BTN_UP_BMP,NAVIGATION_BTN_DN_BMP);
//	SLCD_SendCmd(buffer);	

//	x += adder;
//	sprintf(buffer,"bdc %d %d %d 30 \"%s\" %d %d",SU_BTN_UP,x,y,"U",NAVIGATION_BTN_UP_BMP,NAVIGATION_BTN_DN_BMP);
//	SLCD_SendCmd(buffer);	

//	x += adder;
//	sprintf(buffer,"bdc %d %d %d 30 \"%s\" %d %d",SU_BTN_DOWN,x,y,"D",NAVIGATION_BTN_UP_BMP,NAVIGATION_BTN_DN_BMP);
//	SLCD_SendCmd(buffer);	

//	x += adder;
//	sprintf(buffer,"bdc %d %d %d 1 \"%s\" %d %d",SU_BTN_EXIT,x,y,"X",NAVIGATION_BTN_UP_BMP,NAVIGATION_BTN_DN_BMP);
//	SLCD_SendCmd(buffer);


	if (g_bAudioSearchMode)
		SU_Init_Search_RSSI_StripChart();
	

}


typedef struct SEARCH_ANTENNA_STATE 
{
	int antenna;
	int retry;
} SearchAntennaState;

#define SEARCH_RETRIES 1
#define SEARCH_ONEAXIS_AT_A_TIME 1
#define SEARCH_ALL_AXES_BEFORE_RETRY 2
#define HIGHNUMBEROF_DETECT_RESPONSES 5


static BOOL m_bRunningSearchMode = FALSE;



/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// PROTOCOL 1 FUNCTIONS ///////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef PROTOCOL_1

#error UNCOMMENT NEXT BLOCK REQUIRED
/*
void SU_UpdateSearchStatus(int mode, char antenna, int retry, BOOL bMultipleRetries)
{
	char buffer[50];
	SetDefaultBackForegnd();		
	SLCD_SendCmd("f18BC");
//	SLCD_SendCmd("f8x16");
	char *msg = "";
	char rbuf[10];
	strcpy(rbuf,"   ");
	char a1=antenna;
	char a2=' ';
	char a3=' ';
	
	
	switch (mode) {
		case AUDIOSEARCH_FORMAT:
			msg = "Quick Search";
			if (bMultipleRetries)
				sprintf(rbuf,"-%d ",retry);
			break;
		case ID_SEARCH_FORMAT:
			msg = "ID Search   ";
			if (bMultipleRetries)
				sprintf(rbuf,"-%d ",retry);
			break;
		case MASKING_FORMAT:
			msg = "Masking MU  ";
			if (bMultipleRetries)
				sprintf(rbuf,"-%d ",retry);
			break;
		case SEARCHPAUSED_FORMAT:
			msg = "Paused      ";
			a1 = ' ';
			break;
		case SEARCHDONE_FORMAT:
			msg = "Search Done ";
			a1 = ' ';
			break;
	}
	if (a1 == 'A')
	{
		a1 = 'X'; a2= 'Y'; a3='Z';
	}
		
	sprintf(buffer,"t \"%s %c%c%c%s   \" %d %d",msg, a1,a2,a3, rbuf, SEARCH_PAGE_STATUS_XPOS,SEARCH_PAGE_STATUS_YPOS);

	SLCD_SendCmd(buffer);
}

void SU_Mask_P1_Unit(int index, char antenna_ID)
{
	if (index >=0) {
		MuData* p = &g_MuDeviceList[index];
		SU_UpdateSearchStatus(MASKING_FORMAT, antenna_ID, 3, (SEARCH_RETRIES>1)?TRUE:FALSE);
		// Start with antenna on which the signal was received
		SU_SendAndWaitForMU_P1_Mask(p,antenna_ID);
		if (!p->v.Mobility_valid)
		{
			// if it fails then move to the next antenna
			if (++antenna_ID > TX_ANTENNA_Z) antenna_ID = TX_ANTENNA_X;
			SU_UpdateSearchStatus(MASKING_FORMAT, antenna_ID, 2, (SEARCH_RETRIES>1)?TRUE:FALSE);
			SU_SendAndWaitForMU_P1_Mask(p,antenna_ID);
			if (!p->v.Mobility_valid)
			{
				// and try the last antenna if it still fails
				if (++antenna_ID > TX_ANTENNA_Z) antenna_ID = TX_ANTENNA_X;
				SU_UpdateSearchStatus(MASKING_FORMAT, antenna_ID, 1, (SEARCH_RETRIES>1)?TRUE:FALSE);
				SU_SendAndWaitForMU_P1_Mask(p,antenna_ID);
			}
		}
	}
}

void SU_MaskAllUnmaskedMU_P1_Units(char antenna_ID)
{
	int i;
	int listsize = GetMUListSize();
	for (i=0;i<listsize;i++) {
		if (g_MuDeviceList[i].v.Mobility_valid == 0) 
		{
			SU_Mask_P1_Unit(i,antenna_ID);
		}
	}
}

void Sent_P1_detectMsg(char antenna)
{
	g_bTransmitting = TRUE;	// SET MODE TO TRANSMITTING TO ENSURE RSSI PACKETS IGNORED
	if (g_SuSettings.enable_audio_blanking) AUDIO_MUTE_LATCH = 1;	//normally low to enable the audio board and is high to mute the audio output.
	DelayMsecs(AUDIO_MUTE_DELAY_MS);				// delay to allow audio to be muted
	vlf_SendDetectMsg(antenna);			// send a search command
	set_SU_VLF_RX_Mode(SU_RX_MODE_DECODE);	// tell the MURX module to begin decoding messages
	WaitForSUTXResponse(DETECT_RX_DELAY_MS_P1);					// SU_TX can't receive during the TX period
//	DelayMsecs(DETECT_RX_DELAY_MS_P1);			// Delay to prevent audio of TX from being picked up		
	AUDIO_MUTE_LATCH = 0;	//normally low to enable the audio board and is high to mute the audio output.
	g_bTransmitting = FALSE;	// RE-ENABLE RSSI PACKET UPDATES
}

// runs the main search page - note that bQuickSearch is similar to Search except that no masking occurs and the timeouts are shorter
int SU_RunSearchPage(int bQuickSearch)
{
	struct button btn;
	time_t ref_second = g_seconds;
	int oldsetting;
	int listsize;
	time_t search_counter;
	SearchAntennaState state[3];
	int antenna_ix = 0;
	int antennacountdown = 3;
	BOOL bRetry;
	int waitForResponseSeconds;
	BOOL bRequestToMask = bQuickSearch?FALSE:TRUE;
	
	m_bRunningSearchMode = TRUE;
	m_bLocateMode = FALSE;	
	state[0].antenna = TX_ANTENNA_X;
	state[1].antenna = TX_ANTENNA_Y;
	state[2].antenna = TX_ANTENNA_Z;
	state[0].retry = SEARCH_RETRIES;
	state[1].retry = SEARCH_RETRIES;
	state[2].retry = SEARCH_RETRIES;
	
	vlf_SetNewSearchSequence();		// set a new sequence number so that all MU's will respond
	g_bSearching = TRUE;
	g_bSearchPaused = FALSE;
//	SLCD_Test_Search(FALSE);			// Init the test function for searching	
	g_iSelectedMuIndex = -1;		// reset the index of the selected one to none
	g_iMuListDisplayBase = 0;		// reset the display base to the beginning of the list
	SU_DrawSearchPage(bQuickSearch);

	if ((g_UnitFlags.LogStatus) && (!g_SysError.SDNoCard)) {
		SD_power(1); 
   		SD_OpenLogFile(PC_PORT);		// open a new file and increment file counter
	}

	set_SU_VLF_RX_Mode(SU_RX_MODE_STANDBY);	// tell the MURX module to wait until it is told to decode
	send_SU_RX_ZC_Trigger();
	SU_UpdateGeneralStatus(TRUE,TRUE);

	Sent_P1_detectMsg(state[antenna_ix].antenna);			// send an ID Search command
	SU_UpdateSearchStatus(bQuickSearch?AUDIOSEARCH_FORMAT:ID_SEARCH_FORMAT, state[antenna_ix].antenna,state[antenna_ix].retry, (SEARCH_RETRIES>1)?TRUE:FALSE);
	search_counter = g_seconds;		// set the reference time for the next search command if necessary
	int oldlistsize = GetMUListSize();	// remember the prevous list size;
    clear_SU_ZC_RX_ResponseCount();			// CLEAR THE ZC RESPONSE COUNT	

	while (1) {				// loop until we get an exit from the page
		if (ref_second != g_seconds) {
			ref_second = g_seconds;
			SU_UpdateGeneralStatus(TRUE,FALSE);
			
			// timeout slightly faster for quicksearch
			waitForResponseSeconds = bQuickSearch?QUICKSEARCH_TIMEOUT_SECONDS_P1:SEARCH_TIMEOUT_SECONDS_P1;

			if (g_bSearching && ((g_seconds - search_counter) >= waitForResponseSeconds)) 
			{
				if (bRequestToMask) // if request to mask buffered, mask all it now
				{
					if (g_iSelectedMuIndex >=0) {
						SU_MaskAllUnmaskedMU_P1_Units(state[antenna_ix].antenna);
						// if search is over then restore message
						if ((!bQuickSearch) && antennacountdown==0)
						{
							SU_UpdateSearchStatus(SEARCHDONE_FORMAT, state[antenna_ix].antenna,state[antenna_ix].retry, (SEARCH_RETRIES>1)?TRUE:FALSE);
							set_SU_VLF_RX_Mode(SU_RX_MODE_STANDBY);	// tell the MURX module to wait until it is told to decode						
						}
					}
					
				}

					
				// if we got the maximum responses or we had decode errors, then retry
				if (antennacountdown && g_bSearchPaused) {
					SU_UpdateSearchStatus(SEARCHPAUSED_FORMAT, state[antenna_ix].antenna,state[antenna_ix].retry, (SEARCH_RETRIES>1)?TRUE:FALSE);
					set_SU_VLF_RX_Mode(SU_RX_MODE_STANDBY);	// tell the MURX module to wait until it is told to decode
				}
				else {
					bRetry = FALSE;
					if (((GetMUListSize() - oldlistsize) >=HIGHNUMBEROF_DETECT_RESPONSES) || (g_SysError.MU_DecodeError)) {
						if (state[antenna_ix].retry) {
							state[antenna_ix].retry--;	// retry with the same antenna
							bRetry = TRUE;
						}
					}
					if (bRetry == FALSE) { // if the search was successful or we ran out of retries on the antenna then move on to the next antenna
						if (antennacountdown) {
							antennacountdown--;
							antenna_ix++;
							antenna_ix = antenna_ix % TX_ANTENNA_NUMBER;
							if (antennacountdown == 0) {
								if (bQuickSearch) {		// reset counter to beginning in quick search mode
									antenna_ix = 0;
									antennacountdown = 3;
									state[0].retry = SEARCH_RETRIES;
									state[1].retry = SEARCH_RETRIES;
									state[2].retry = SEARCH_RETRIES;
								}
								else {	// quit the search
									g_bSearching = FALSE;
									g_bSearchPaused = FALSE;
									SU_UpdateSearchStatus(SEARCHDONE_FORMAT, state[antenna_ix].antenna,state[antenna_ix].retry, (SEARCH_RETRIES>1)?TRUE:FALSE);
									set_SU_VLF_RX_Mode(SU_RX_MODE_STANDBY);	// tell the MURX module to wait until it is told to decode
									bRequestToMask = FALSE;		// DONE MASKING
								}
							}
						}
					}
					
					// if we still have an antenna to search then do so now
					if (antennacountdown) {
						SU_UpdateSearchStatus(bQuickSearch?AUDIOSEARCH_FORMAT:ID_SEARCH_FORMAT, state[antenna_ix].antenna,state[antenna_ix].retry, (SEARCH_RETRIES>1)?TRUE:FALSE);
						Sent_P1_detectMsg(state[antenna_ix].antenna);			// send an ID Search command
						search_counter = g_seconds;		// set the reference time for the next search command if necessary
					}
				}
				oldlistsize = GetMUListSize();	// remember the current list size;	
			}
		}
//		if (get_ZC_RX_ResponseCount() > 0) {
			// todo - display zero crossing data
//    		clear_SU_ZC_RX_ResponseCount();			// CLEAR THE ZC RESPONSE COUNT	
//		}

		unsigned short taskStatus = ProcessBackgroundTasks();
		
		btn = GetUserAction(taskStatus);
		if (btn.index != 0) {
			oldsetting = g_iSelectedMuIndex;
			listsize = GetMUListSize();
			switch (btn.index) {
				// EITHER TOGGLE QUICK AND NORMAL SEARCH MODES OR START ANOTHER OF THE EXISTING MODE
				case SU_BTN_NORMAL_SEARCH:		
				case SU_BTN_QUICK_SEARCH:
					if ((bQuickSearch == TRUE) && (btn.index == SU_BTN_NORMAL_SEARCH)) {
						bQuickSearch = FALSE;	
						SU_DataClearMobility();		// clear all mobility values to mask old ones
						oldlistsize = 0; 			// force a masking of all devices in list
					}
					if ((bQuickSearch == FALSE) && (btn.index == SU_BTN_QUICK_SEARCH)) {
						bQuickSearch = TRUE;
						vlf_SetNewSearchSequence();		// set a new sequence number so that all MU's will respond
					}		
					if (!g_bSearching) {
						antenna_ix = 0;
						antennacountdown = 3;
						state[0].retry = SEARCH_RETRIES;
						state[1].retry = SEARCH_RETRIES;
						state[2].retry = SEARCH_RETRIES;
						g_bSearching = TRUE;
						oldlistsize = GetMUListSize();	// remember the current list size;								
						SU_UpdateSearchStatus(bQuickSearch?AUDIOSEARCH_FORMAT:ID_SEARCH_FORMAT, state[antenna_ix].antenna,state[antenna_ix].retry, (SEARCH_RETRIES>1)?TRUE:FALSE);
						Sent_P1_detectMsg(state[antenna_ix].antenna);			// send a search command
						search_counter = g_seconds;		// set the reference time for the next search command if necessary
					}
				case SU_BTN_ENTER:
//					if ((g_iSelectedMuIndex >=0) && (bQuickSearch == FALSE))	// NO MASKING IN QUICK SEARCH
//					{
//						if (g_bSearching) 	// buffer request if searching
//						{
//							bRequestToMask = TRUE;	// SET A REQUEST TO MASK THE NEXT TIME THROUGH
//						}
//						else // do it now
//						{
//							SU_Mask_P1_Unit(g_iSelectedMuIndex, TX_ANTENNA_X);					
//						}
//					}
//					break;

// 				FALL THROUGH FROM ENTER BUTTON
				case SU_BTN_LOCATE:
					if (g_iSelectedMuIndex >= 0) {
						int b_OldSearchState = g_bSearchPaused;
						m_bLocateMode = TRUE;
						m_bRunningSearchMode = FALSE;
						g_bSearchPaused = TRUE;
						SU_RunLocatePage(g_iSelectedMuIndex);  
						m_bLocateMode = FALSE;
						m_bRunningSearchMode = TRUE;
						g_bSearchPaused = b_OldSearchState;
						if (g_bSearching) {		// continue from where last search left off
							if (g_bSearchPaused)
								SU_UpdateSearchStatus(SEARCHPAUSED_FORMAT, state[antenna_ix].antenna,state[antenna_ix].retry, (SEARCH_RETRIES>1)?TRUE:FALSE);
							else {
								oldlistsize = GetMUListSize();	// remember the current list size;
								SU_UpdateSearchStatus(bQuickSearch?AUDIOSEARCH_FORMAT:ID_SEARCH_FORMAT, state[antenna_ix].antenna,state[antenna_ix].retry, (SEARCH_RETRIES>1)?TRUE:FALSE);
								Sent_P1_detectMsg(state[antenna_ix].antenna);			// send an ID Search command
								search_counter = g_seconds;		// set the reference time for the next search command if necessary
							}
						}
						else {
							SU_UpdateSearchStatus(SEARCHDONE_FORMAT, state[antenna_ix].antenna,state[antenna_ix].retry, (SEARCH_RETRIES>1)?TRUE:FALSE);
						}
						SU_DrawSearchPage();
						SU_Search_RedrawIDList();
						SU_UpdateGeneralStatus(TRUE,TRUE);
					}					
					break;
				case SU_BTN_UP:
					if (g_iSelectedMuIndex >= 0) {
						if (g_iSelectedMuIndex > 0)
							g_iSelectedMuIndex--;
						else
							g_iSelectedMuIndex = listsize -1;
					}
					break;
				case SU_BTN_DOWN:
					if (g_iSelectedMuIndex >= 0) {
						if (g_iSelectedMuIndex >= (listsize-1)) 
							g_iSelectedMuIndex = 0;
						else
							g_iSelectedMuIndex++;

					}
					break;
				case SU_BTN_EXIT:
					g_bSearching = FALSE;

					set_SU_VLF_RX_Mode(SU_RX_MODE_STANDBY);	// tell the MURX module to wait until it is told to decode
					//SUTX_POWER_OFF;
					//SURX_POWER_OFF;
					if ((g_UnitFlags.LogStatus) && (!g_SysError.SDNoCard)) {
						SD_CloseLogFile(PC_PORT); // IF FILE HANDLE VALID THEN FINISH AND CLOSE THE FILE
						SD_power(0);
					}
					m_bRunningSearchMode = FALSE;
					m_bLocateMode = FALSE;
					return SU_BTN_EXIT;
					break;
				case SU_BTN_PAUSE:
					if ((g_bSearching) && (antennacountdown)) {
						g_bSearchPaused = TRUE;
					}
					break;
				case SU_BTN_SETTINGS:
					SU_RunSettingsPage();
					SU_DrawSearchPage();
					SU_Search_RedrawIDList();
					SU_UpdateGeneralStatus(TRUE,TRUE);
					break;	
				case SU_BTN_LOG:			// ignore log button from here
					break;	
				default:
					break;
			}
			if (oldsetting != g_iSelectedMuIndex)
			{
				SU_DrawMUSearchData(oldsetting, FALSE);
				
				if (!SU_isSearchRowVisible(g_iSelectedMuIndex))
					SU_Search_RedrawIDList();
				else
				{
					SU_DrawMUSearchData(g_iSelectedMuIndex, TRUE);
				}
				SU_UpdateCountDisplay();
			}
		}
	}
	return SU_BTN_EXIT;
}

// sends a single mask message to an antenna in the P1 protocol
void SU_SendAndWaitForMU_P1_Mask(MuData* p, char antenna)
{
	unsigned long long lasttick = 0l;
	unsigned long long elapsedms=0;
	
	g_bTransmitting = TRUE;	// SET MODE TO TRANSMITTING TO ENSURE RSSI PACKETS IGNORED
	if (g_SuSettings.enable_audio_blanking) AUDIO_MUTE_LATCH = 1;	//normally low to enable the audio board and is high to mute the audio output.
	DelayMsecs(AUDIO_MUTE_DELAY_MS);			// delay to allow audio to be muted
	vlf_SendMaskMsg(p->id_u32,antenna);
	set_SU_VLF_RX_Mode(SU_RX_MODE_DECODE);		// tell the MURX module to begin decoding messages
	WaitForSUTXResponse(MASK_RX_DELAY_MS_P1);
	//DelayMsecs(MASK_RX_DELAY_MS_P1);			// Delay to prevent audio of TX from being picked up		
	AUDIO_MUTE_LATCH = 0;						//normally low to enable the audio board and is high to mute the audio output.
	g_bTransmitting = FALSE;					// RE-ENABLE RSSI PACKET UPDATES
	
	p->v.Mobility_valid = 0;	// RESET TO NOT VALID
	lasttick = g_tick;
	do {
		ProcessSURXStream();
		if (g_tick != lasttick) // update ate 1ms intervals
		{
			if (g_tick > lasttick)
				elapsedms += g_tick-lasttick;
			else
				elapsedms += 1;	// account for timer overflow
						
			lasttick = g_tick;
		}
		if (p->v.Mobility_valid)
			break;
	}
	while (elapsedms < P1_WAIT_FOR_MASK_RESPONSE_MS);
}
*/

#endif // PROTOCOL 1


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// PROTOCOL 2 FUNCTIONS ///////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////




///////////////////////////////////////// PROTOCOL 2 SEARCH WINDOW ///////////////////////////////////
// DIFFERENT FROM PROTOCOL 1 IN THE SENSE THAT IT SENDS ON X,Y AND Z AND LISTENS FOR ANY ANSWER 
// RATHER THAN LOOPING ANNENNAS AND LISTENING TO EACH ONE INDIVIDUALLY
// runs the main search page FOR PROTOCOL 2 ONLY - note that bQuickSearch is similar to Search except that no masking occurs and the timeouts are shorter
// sends either a quicksearch or detect message to all antennas
#ifdef PROTOCOL_2

	void SU_UpdateP2SearchStatus(int mode)
	{
		//char buffer[128];
		char buffer[256];
		
		#ifdef INCLUDE_MINERADIO_HEADER
			SET_TO_MINERADIO_BACKGROUND;
		#else
			SetDefaultBackForegnd();
		#endif	
	
		
		char *msg = "";
		
		switch (mode)
		{
			case AUDIOSEARCH_FORMAT:
				#ifdef RUSSIAN
					msg = "\xD0\x98\xD0\xB4\xD1\x91\xD1\x82 \xD0\xBF\xD0\xBE\xD0\xB8\xD1\x81\xD0\xBA";
				#else
					msg = "Searching ";
				#endif
				break;
			case ID_SEARCH_FORMAT:
				#ifdef RUSSIAN
					msg = "\xD0\x98\xD0\xB4\xD1\x91\xD1\x82 \xD0\xBF\xD0\xBE\xD0\xB8\xD1\x81\xD0\xBA";
				#else
					msg = "Searching ";
				#endif
				break;
			case MASKING_FORMAT:
				#ifdef RUSSIAN
					msg = "\xD0\x9C\xD0\xB0\xD1\x81\xD0\xBA\xD0\xB8\xD1\x80\xD0\xBE\xD0\xB2\xD0\xB0\xD0\xBD\xD0\xB8\xD0\xB5";
				#else
					msg = "Masking MU ";
				#endif	
				break;
			case SEARCHPAUSED_FORMAT:
				#ifdef RUSSIAN
					msg = "\xD0\x9F\xD0\xBE\xD0\xB8\xD1\x81\xD0\xBA \xD0\xBF\xD1\x80\xD0\xB8\xD0\xBE\xD1\x81\xD1\x82\xD0\xB0\xD0\xBD\xD0\xBE\xD0\xB2\xD0\xBB\xD0\xB5\xD0\xBD";
				#else
					msg = "Paused     ";
				#endif	
				break;
			case SEARCHDONE_FORMAT:
				#ifdef RUSSIAN
					msg = "\xD0\x9F\xD0\xBE\xD0\xB8\xD1\x81\xD0\xBA \xD0\xBE\xD0\xBA\xD0\xBE\xD0\xBD\xD1\x87\xD0\xB5\xD0\xBD";
				#else
					msg = "Search Done";
				#endif	
				break;
			case FULL_MASKING_FORMAT:
				#ifdef RUSSIAN
					msg = "\xD0\x98\xD1\x81\xD0\xBA\xD0\xBB\xD1\x8E\xD1\x87\xD0\xB5\xD0\xBD\xD0\xB8\xD0\xB5 \xD0\xB8\xD0\xB7 \xD0\xBF\xD0\xBE\xD0\xB8\xD1\x81\xD0\xBA\xD0\xB0";
				#else
					msg = "Full Masking";
				#endif	
				break;
		}
	
		SLCD_SendCmd("utf8 on");		// sets unicode
		SLCD_SendCmd("fUARIAL14");		// sets the unicode character set for the title
		
		if (mode == SEARCHDONE_FORMAT)
		{
			SLCD_SendCmd("tfx 0");	// delete flashing if enabled
			sprintf(buffer,"t \"%45s\" %d %d", " ", SEARCH_PAGE_STATUS_XPOS,SEARCH_PAGE_STATUS_YPOS);	// ERASE OLD LINE
			SLCD_SendCmd(buffer);
			#ifdef RUSSIAN
				sprintf(buffer,"t \"%40s\" %d %d",msg, SEARCH_PAGE_STATUS_XPOS,SEARCH_PAGE_STATUS_YPOS);
			#else
				sprintf(buffer,"t \"%30s\" %d %d",msg, SEARCH_PAGE_STATUS_XPOS,SEARCH_PAGE_STATUS_YPOS);
			#endif	
		}
		else
		{
			SLCD_SendCmd("tfx 0");	// delete flashing if enabled
			sprintf(buffer,"t \"%45s\" %d %d", " ", SEARCH_PAGE_STATUS_XPOS,SEARCH_PAGE_STATUS_YPOS);	// ERASE OLD LINE
			SLCD_SendCmd(buffer);
			#ifdef RUSSIAN
				sprintf(buffer,"tf 0 500 \"%40s\" %d %d T",msg, SEARCH_PAGE_STATUS_XPOS,SEARCH_PAGE_STATUS_YPOS);	
			#else
				sprintf(buffer,"tf 0 500 \"%30s\" %d %d T",msg, SEARCH_PAGE_STATUS_XPOS,SEARCH_PAGE_STATUS_YPOS);	
			#endif	
		}
	
		SLCD_SendCmd(buffer);
		
	
		if (g_bAudioSearchMode)
		{
			/*
			if (g_bSearchPaused)
			{
				#ifdef RUSSIAN
					sprintf(buffer, "*=\xD0\x98\xD1\x81\xD0\xBA\xD0\xBB\xD1\x8E\xD1\x87\xD0\xB8\xD1\x82\xD1\x8C \xD0\xB8\xD0\xB7 \xD0\xBF\xD0\xBE\xD0\xB8\xD1\x81\xD0\xBA\xD0\xB0 \xD0\xBD\xD0\xB0 %d \xD0\xBC\xD0\xB8\xD0\xBD\xD1\x83\xD1\x82", MU_FULL_MASK_MINUTES);
					SLCD_WriteFooter("",
//						"*=\xD0\x9E\xD0\xBF\xD1\x80\xD0\xB5\xD0\xB4\xD0\xB5\xD0\xBB\xD0\xB8\xD1\x82\xD1\x8C \xD0\xBC\xD0\xB5\xD1\x81\xD1\x82\xD0\xBE\xD0\xBD\xD0\xB0\xD1\x85\xD0\xBE\xD0\xB6\xD0\xB4\xD0\xB5\xD0\xBD\xD0\xB8\xD0\xB5",
//						"*=\xD0\x98\xD1\x81\xD0\xBA\xD0\xBB\xD1\x8E\xD1\x87\xD0\xB8\xD1\x82\xD1\x8C \xD0\xB8\xD0\xB7 \xD0\xBF\xD0\xBE\xD0\xB8\xD1\x81\xD0\xBA\xD0\xB0",
						buffer,
						"F1=\xD0\x92\xD0\xBE\xD0\xB7\xD0\xBE\xD0\xB1\xD0\xBD\xD0\xBE\xD0\xB2\xD0\xB8\xD1\x82\xD1\x8C  F2=ID \xD0\xBF\xD0\xBE\xD0\xB8\xD1\x81\xD0\xBA     "
						);
				#else
					SLCD_WriteFooter("","","*=Full Mask F1=Continue F2=ID Search   ");	//FULL_MASK
					//SLCD_WriteFooter("","","*=Locate F1=Continue F2=ID Search   ");
				#endif		
			}
			else 
			{
				#ifdef RUSSIAN
					sprintf(buffer, "*=\xD0\x98\xD1\x81\xD0\xBA\xD0\xBB\xD1\x8E\xD1\x87\xD0\xB8\xD1\x82\xD1\x8C \xD0\xB8\xD0\xB7 \xD0\xBF\xD0\xBE\xD0\xB8\xD1\x81\xD0\xBA\xD0\xB0 \xD0\xBD\xD0\xB0 %d \xD0\xBC\xD0\xB8\xD0\xBD\xD1\x83\xD1\x82", MU_FULL_MASK_MINUTES);
					SLCD_WriteFooter("",
//						"*=\xD0\x9E\xD0\xBF\xD1\x80\xD0\xB5\xD0\xB4\xD0\xB5\xD0\xBB\xD0\xB8\xD1\x82\xD1\x8C \xD0\xBC\xD0\xB5\xD1\x81\xD1\x82\xD0\xBE\xD0\xBD\xD0\xB0\xD1\x85\xD0\xBE\xD0\xB6\xD0\xB4\xD0\xB5\xD0\xBD\xD0\xB8\xD0\xB5",
//						"*=\xD0\x98\xD1\x81\xD0\xBA\xD0\xBB\xD1\x8E\xD1\x87\xD0\xB8\xD1\x82\xD1\x8C \xD0\xB8\xD0\xB7 \xD0\xBF\xD0\xBE\xD0\xB8\xD1\x81\xD0\xBA\xD0\xB0",
						buffer,
						"F1=\xD0\x9F\xD1\x80\xD0\xB8\xD0\xBE\xD1\x81\xD1\x82\xD0\xB0\xD0\xBD\xD0\xBE\xD0\xB2\xD0\xB8\xD1\x82\xD1\x8C F2=ID \xD0\xBF\xD0\xBE\xD0\xB8\xD1\x81\xD0\xBA    "
						);
				#else
					SLCD_WriteFooter("","","*=Full Mask F1=Pause F2=ID Search      "); 	//FULL_MASK
					//SLCD_WriteFooter("","","*=Locate F1=Pause F2=ID Search      ");
				#endif		
			}
			*/
			#ifdef RUSSIAN
				sprintf(buffer, "*=\xD0\x98\xD1\x81\xD0\xBA\xD0\xBB\xD1\x8E\xD1\x87\xD0\xB8\xD1\x82\xD1\x8C \xD0\xB8\xD0\xB7 \xD0\xBF\xD0\xBE\xD0\xB8\xD1\x81\xD0\xBA\xD0\xB0 \xD0\xBD\xD0\xB0 %d \xD0\xBC\xD0\xB8\xD0\xBD\xD1\x83\xD1\x82", MU_FULL_MASK_MINUTES);
				SLCD_WriteFooter("", buffer,
					"F1=\xD0\x98\xD1\x81\xD0\xBA\xD0\xBB\xD1\x8E\xD1\x87\xD1\x91\xD0\xBD\xD0\xBD\xD1\x8B\xD0\xB5 \xD1\x82\xD0\xB0\xD0\xB3\xD0\xB8  F2=ID \xD0\xBF\xD0\xBE\xD0\xB8\xD1\x81\xD0\xBA    "
					);
			#else
				sprintf(buffer, "*=Full Mask for %d min.", MU_FULL_MASK_MINUTES);
				SLCD_WriteFooter("",buffer,"F1=Full Mask Table    F2=ID Search");	//FULL_MASK
			#endif		

		}
		else
		{
			#ifdef RUSSIAN
				SLCD_WriteFooter("",
						"*=\xD0\x9E\xD0\xBF\xD1\x80\xD0\xB5\xD0\xB4\xD0\xB5\xD0\xBB\xD0\xB8\xD1\x82\xD1\x8C \xD0\xBC\xD0\xB5\xD1\x81\xD1\x82\xD0\xBE\xD0\xBD\xD0\xB0\xD1\x85\xD0\xBE\xD0\xB6\xD0\xB4\xD0\xB5\xD0\xBD\xD0\xB8\xD0\xB5",
						"F1=\xD0\x9F\xD0\xBE\xD0\xB2\xD1\x82\xD0\xBE\xD1\x80  F2=\xD0\x9C\xD0\xB0\xD1\x81\xD0\xBA\xD0\xB8\xD1\x80\xD0\xBE\xD0\xB2\xD0\xB0\xD1\x82\xD1\x8C      ");
			#else
				SLCD_WriteFooter("","","*=Locate   F1=Retry   F2=Mask          ");
			#endif		
		}

	}



void Send_P2_SearchMsg(int bQuickSearch)
{
	g_bTransmitting = TRUE;	// SET MODE TO TRANSMITTING TO ENSURE RSSI PACKETS IGNORED
	if (g_SuSettings.enable_audio_blanking) AUDIO_MUTE_LATCH = 1;	//normally low to enable the audio board and is high to mute the audio output.
	DelayMsecs(AUDIO_MUTE_DELAY_MS);				// delay to allow audio to be muted
	if (bQuickSearch)
	{
		vlf_SendQuickSearchMsg(TX_ANTENNA_X);		// send a quick-search command
		DelayMsecs(QUICKSEARCH_RX_DELAY_MS_P2);		// SU_TX can't receive next command while it is transmitting 
		vlf_SendQuickSearchMsg(TX_ANTENNA_Y);		// send a quick-search command
		DelayMsecs(QUICKSEARCH_RX_DELAY_MS_P2);		// SU_TX can't receive next command while it is transmitting
		vlf_SendQuickSearchMsg(TX_ANTENNA_Z);		// send a quick-search command
		set_SU_VLF_RX_Mode(SU_RX_MODE_DECODE);		// tell the MURX module to begin decoding messages
		DelayMsecs(QUICKSEARCH_RX_DELAY_MS_P2);		// Delay to prevent audio of TX from being picked up
	}
	else
	{
		vlf_SendDetectMsg(TX_ANTENNA_X);			// send a search command
		DelayMsecs(DETECT_RX_DELAY_MS_P2);				// SU_TX can't receive next command while it is transmitting
		vlf_SendDetectMsg(TX_ANTENNA_Y);			// send a search command
		DelayMsecs(DETECT_RX_DELAY_MS_P2);				// SU_TX can't receive next command while it is transmitting
		vlf_SendDetectMsg(TX_ANTENNA_Z);			// send a search command
		set_SU_VLF_RX_Mode(SU_RX_MODE_DECODE);		// tell the MURX module to begin decoding messages
		DelayMsecs(DETECT_RX_DELAY_MS_P2);				// Delay to prevent audio of TX from being picked up
	}	
	AUDIO_MUTE_LATCH = 0;	//normally low to enable the audio board and is high to mute the audio output.
	g_bTransmitting = FALSE;	// RE-ENABLE RSSI PACKET UPDATES
}


int SU_Run_P2_SearchPage(int bAudioSearch)
{
	struct button btn;
	time_t ref_second = g_seconds;
	int seconds = 0;
	int oldsetting;
	int listsize;
	int retry_countdown = 3;
	BOOL bRetry;
	BOOL bRequestToMask = bAudioSearch?FALSE:TRUE;
	unsigned long long lasttick = 0l;
	unsigned long total_elapsedms;
	unsigned long timeout_ms;
	unsigned long long start_tick = g_tick;
	
	g_bAudioSearchMode = bAudioSearch;
	m_bRunningSearchMode = TRUE;
	m_bLocateMode = FALSE;	
	
	vlf_SetNewSearchSequence();		// set a new sequence number so that all MU's will respond
	g_bSearching = TRUE;
	g_bSearchPaused = FALSE;
	g_iSelectedMuIndex = -1;		// reset the index of the selected one to none
	g_iMuListDisplayBase = 0;		// reset the display base to the beginning of the list
	SU_DrawSearchPage();

	if ((g_UnitFlags.LogStatus) && (!g_SysError.SDNoCard)) {
		SD_power(1); 
   		SD_OpenLogFile(PC_PORT);		// open a new file and increment file counter
	}

	set_SU_VLF_RX_Mode(SU_RX_MODE_STANDBY);	// tell the MURX module to wait until it is told to decode
	send_SU_RX_ZC_Trigger();
	SU_UpdateGeneralStatus(TRUE,TRUE);

	int oldlistsize = GetMUListSize();			// remember the prevous list size;
    clear_SU_ZC_RX_ResponseCount();				// CLEAR THE ZC RESPONSE COUNT	
	clear_RSSI_RX_ResponseCount();			// clear the RX response count
	clear_MU_ResponseCount();				// clear the response count so we can tell if we get a response to the locate msg
	ResetLastRssiReadings();
	ResetPeakRssiReadings();

	SU_UpdateP2SearchStatus(g_bAudioSearchMode?AUDIOSEARCH_FORMAT:ID_SEARCH_FORMAT);
	Send_P2_SearchMsg(g_bAudioSearchMode);			// send a search command
	lasttick = g_tick;
	total_elapsedms = 0l;	// reset the elapsed ms counter for the next search command if necessary
		
#ifdef ENABLE_TEST_FAKEHPT_MODE
	SLCD_Test_Search(FALSE);	// - this is just for tesing - Init the test function for searching
#endif
	
	while (1) {				// loop until we get an exit from the page
		if (g_tick != lasttick) // update ate 1ms intervals
		{
			if (g_tick > lasttick)
				total_elapsedms += g_tick-lasttick;
			else
				total_elapsedms += 1;	// account for timer overflow
			
			lasttick = g_tick;
			
			// Update RSSI values if available
			if (g_bAudioSearchMode && (g_tick - start_tick) > 100) {	// update display every 100 ms
				start_tick = g_tick;
				if (get_MU_ResponseCount() || get_RSSI_RX_ResponseCount())	// if we got any RSSI updates from a packet or clutter response, update it
				{
					SU_Update_Search_RSSI_StripChart();
				    clear_RSSI_RX_ResponseCount();			// clear the RX response count
					clear_MU_ResponseCount();				// clear the response count so we can tell if we get a response to the locate msg
					ResetLastRssiReadings();
					ResetPeakRssiReadings();
				}
				Send_Clutter_request();	// request background clutter 
				
//				if (get_ZC_RX_ResponseCount() > 0) {
//					// todo - display zero crossing data if needed
//		    		clear_SU_ZC_RX_ResponseCount();			// CLEAR THE ZC RESPONSE COUNT	
//				}
			}			
		
			if (ref_second != g_seconds) // update at second intervals
			{
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
			
			// timeout slightly faster for quicksearch
			timeout_ms = (g_bAudioSearchMode)?QUICKSEARCH_TIMEOUT_MS_P2:SEARCH_TIMEOUT_MS_P2;
			if (g_bSearching && (total_elapsedms >= timeout_ms)) 
			{
				total_elapsedms = 0;
				if (bRequestToMask) // if request to mask buffered, mask all it now
				{
					if (g_iSelectedMuIndex >=0) 
					{
						//FULL_MASK
						if (g_bAudioSearchMode)
						{
							SU_UpdateP2SearchStatus(FULL_MASKING_FORMAT);
#ifdef ENABLE_TEST_FAKEHPT_MODE
							if(SU_SendAndWaitForMU_P2_FullMask_3(g_MuDeviceList[g_iSelectedMuIndex].id_u32,TX_ANTENNA_ALL, MU_FULL_MASK_MINUTES))
#else
							if(SU_SendAndWaitForMU_P2_FullMask_3_Search(&g_MuDeviceList[g_iSelectedMuIndex],TX_ANTENNA_ALL, MU_FULL_MASK_MINUTES))
#endif
							{
								SU_MoveHPTTo_FullMaskList(g_MuDeviceList[g_iSelectedMuIndex].id_u32);
								listsize = GetMUListSize();
								SU_Search_RedrawIDList_ToIndex(listsize+1);
							}
							SU_UpdateP2SearchStatus(AUDIOSEARCH_FORMAT);
							bRequestToMask  = FALSE;
						}
						else
						//FULL_MASK END
						{
							SU_UpdateP2SearchStatus(MASKING_FORMAT);
							SU_MaskAllUnmaskedMU_P2_Units();
						}
														
						// if search is over then restore message
						if ((!g_bAudioSearchMode) && retry_countdown==0)
						{
							SU_UpdateP2SearchStatus(SEARCHDONE_FORMAT);
							set_SU_VLF_RX_Mode(SU_RX_MODE_STANDBY);	// tell the MURX module to wait until it is told to decode						
						}
					}
					
				}
					
				if (retry_countdown && g_bSearchPaused) 
				{
				}
				else 
				{
					bRetry = FALSE;
					// if we got the maximum responses or we had decode errors, then retry
					if (((GetMUListSize() - oldlistsize) >=HIGHNUMBEROF_DETECT_RESPONSES) 
						|| (g_SysError.MU_DecodeError)) 
					{
						bRetry = TRUE;
					}
					
					if (bRetry == FALSE) // if the search was successful then decrement retry counter
					{ 
						if (retry_countdown) 
						{
							retry_countdown--;
							if (retry_countdown == 0) 
							{
								if (g_bAudioSearchMode) // reset counter to beginning in quick search mode
								{		
									retry_countdown = 3;
								}
								else 
								{	// quit the search
									g_bSearching = FALSE;
									g_bSearchPaused = FALSE;
									SU_UpdateP2SearchStatus(SEARCHDONE_FORMAT);
									set_SU_VLF_RX_Mode(SU_RX_MODE_STANDBY);	// tell the MURX module to wait until it is told to decode
									bRequestToMask = FALSE;		// DONE MASKING
								}
							}
						}
					}
					
					
					if (retry_countdown) // if we still have more retries left then do it again
					{
						if (!g_bAudioSearchMode)
							SU_UpdateP2SearchStatus(g_bAudioSearchMode?AUDIOSEARCH_FORMAT:ID_SEARCH_FORMAT);
															
						Send_P2_SearchMsg(g_bAudioSearchMode);			// send a search command
						
						lasttick = g_tick;		// start counting from now
						total_elapsedms = 0l;	// reset the elapsed ms counter for the next search command if necessary
					}
				}
				oldlistsize = GetMUListSize();	// remember the current list size;	
			}
		}

		unsigned short taskStatus = ProcessBackgroundTasks();
		
		btn = GetUserAction(taskStatus);
		if (btn.index != 0) {
			oldsetting = g_iSelectedMuIndex;
			listsize = GetMUListSize();
			if ((g_iSelectedMuIndex < 0) && (listsize)) g_iSelectedMuIndex = 0;
		
			switch (btn.index) {
				case SU_BTN_UP:
					if (g_iSelectedMuIndex >= 0) {
						if (g_iSelectedMuIndex > 0)
							g_iSelectedMuIndex--;
						else
							g_iSelectedMuIndex = listsize -1;
					}
					break;
				case SU_BTN_DOWN:
					if (g_iSelectedMuIndex >= 0) {
						if (g_iSelectedMuIndex >= (listsize-1)) 
							g_iSelectedMuIndex = 0;
						else
							g_iSelectedMuIndex++;

					}
					break;
			
				case SU_BTN_EXIT:
					g_bSearching = FALSE;
					set_SU_VLF_RX_Mode(SU_RX_MODE_STANDBY);	// tell the MURX module to wait until it is told to decode
					if ((g_UnitFlags.LogStatus) && (!g_SysError.SDNoCard)) {
						SD_CloseLogFile(PC_PORT); // IF FILE HANDLE VALID THEN FINISH AND CLOSE THE FILE
						SD_power(0);
					}
					m_bRunningSearchMode = FALSE;
					m_bLocateMode = FALSE;
					return SU_BTN_EXIT;
					break;
			
				case SU_BTN_F2:
					if (g_bAudioSearchMode == FALSE)			// NO MASKING IN QUICK SEARCH
					{
						if (g_iSelectedMuIndex >=0)	
						{
							if (g_bSearching) 	// buffer request if searching
							{
								bRequestToMask = g_bAudioSearchMode?FALSE:TRUE;	// SET A REQUEST TO MASK THE NEXT TIME THROUGH
							}
							else // do it now
							{
								SU_UpdateP2SearchStatus(MASKING_FORMAT);
								SU_SendAndWaitForMU_P2_Mask(&g_MuDeviceList[g_iSelectedMuIndex],TX_ANTENNA_ALL);
								SU_UpdateP2SearchStatus(SEARCHDONE_FORMAT);
							}
						}
					}
					else		// Switch to ID mode if in Audio mode to avoid creating new log file for the session
					{
						g_bAudioSearchMode = FALSE;	
						SU_UpdateP2SearchStatus(SEARCHDONE_FORMAT);
						SU_DataClearMobility();		// clear all mobility values to mask old ones
						oldlistsize = 0; 			// force a masking of all devices in list
						SU_DrawSearchPage();
						retry_countdown = 3;
						bRequestToMask = TRUE;
						g_bSearching = TRUE;
						oldlistsize = GetMUListSize();	// remember the current list size;	
						SU_Search_RedrawIDList();							
						SU_UpdateP2SearchStatus(AUDIOSEARCH_FORMAT);
						Send_P2_SearchMsg(g_bAudioSearchMode);			// send a search command
						lasttick = g_tick;		// start counting from now
						total_elapsedms = 0l;	// reset the elapsed ms counter for the next search command if necessary
/*
						g_bSearching = FALSE;
						set_SU_VLF_RX_Mode(SU_RX_MODE_STANDBY);	// tell the MURX module to wait until it is told to decode
						if ((g_UnitFlags.LogStatus) && (!g_SysError.SDNoCard)) {
							SD_CloseLogFile(PC_PORT); // IF FILE HANDLE VALID THEN FINISH AND CLOSE THE FILE
							SD_power(0);
						}
						m_bRunningSearchMode = FALSE;
						m_bLocateMode = FALSE;
						return SU_BTN_F2;
*/						
					}
					break;	

				case SU_BTN_ENTER:
					if (g_iSelectedMuIndex >= 0) 
					{
						//FULL_MASK
						if (g_bAudioSearchMode)	// *=FULL_MASK in quick search mode
						{
							if (g_bSearching) 	// buffer request if searching
							{
								bRequestToMask = TRUE;	// SET A REQUEST TO MASK THE NEXT TIME THROUGH
							}
							else // do it now
							{
								SU_UpdateP2SearchStatus(FULL_MASKING_FORMAT);
#ifdef ENABLE_TEST_FAKEHPT_MODE
								if(SU_SendAndWaitForMU_P2_FullMask_3(g_MuDeviceList[g_iSelectedMuIndex].id_u32,TX_ANTENNA_ALL, MU_FULL_MASK_MINUTES))
#else
								if(SU_SendAndWaitForMU_P2_FullMask_3_Search(&g_MuDeviceList[g_iSelectedMuIndex],TX_ANTENNA_ALL, MU_FULL_MASK_MINUTES))
#endif
								{
									SU_MoveHPTTo_FullMaskList(g_MuDeviceList[g_iSelectedMuIndex].id_u32);
									SU_Search_RedrawIDList_ToIndex(listsize);
								}
								SU_UpdateP2SearchStatus(AUDIOSEARCH_FORMAT);	
							}
						}
						else
						//FULL_MASK END
						{
							int b_OldSearchState = g_bSearchPaused;
							m_bLocateMode = TRUE;
							m_bRunningSearchMode = FALSE;
							g_bSearchPaused = TRUE;
							SU_Run_P2_LocatePage(g_iSelectedMuIndex);  
							m_bLocateMode = FALSE;
							m_bRunningSearchMode = TRUE;
							g_bSearchPaused = b_OldSearchState;
							
							SU_DrawSearchPage();
							SU_Search_RedrawIDList();
							
							if (g_bSearching) {		// continue from where last search left off
								if (g_bSearchPaused)
									SU_UpdateP2SearchStatus(SEARCHPAUSED_FORMAT);
								else {
									oldlistsize = GetMUListSize();	// remember the current list size;
									SU_UpdateP2SearchStatus(g_bAudioSearchMode?AUDIOSEARCH_FORMAT:ID_SEARCH_FORMAT);
									Send_P2_SearchMsg(g_bAudioSearchMode);			// send a search command
									lasttick = g_tick;		// start counting from now
									total_elapsedms = 0l;	// reset the elapsed ms counter for the next search command if necessary
								}
							}
							else 
							{
								SU_UpdateP2SearchStatus(SEARCHDONE_FORMAT);
							}
							SU_UpdateGeneralStatus(TRUE,TRUE);
						}
					}
					else
					{
#ifdef ENABLE_TEST_FAKEHPT_MODE
						SLCD_Test_Search(TRUE);	// - this is just for tesing - adds a unit
#endif
					}					
					break;
				case SU_BTN_F1:
					if (g_bAudioSearchMode)
					{
						//FULL_MASK						
						m_bRunningSearchMode = FALSE;	// TURN OFF BACKGROUND UPDATES FROM MU PACKETS
						SU_Run_FullMaskPage();
						m_bRunningSearchMode = TRUE;
						SU_DrawSearchPage();
						SU_Search_RedrawIDList();
						SU_UpdateGeneralStatus(TRUE,TRUE);
						SU_UpdateP2SearchStatus(g_bAudioSearchMode?AUDIOSEARCH_FORMAT:ID_SEARCH_FORMAT);
						//FULL_MASK	END				
/*
						if (g_bSearching) 
						{
							if (g_bSearchPaused == TRUE)
							{
								g_bSearchPaused = FALSE;
								retry_countdown = 3;
								SU_UpdateP2SearchStatus(AUDIOSEARCH_FORMAT);
								
							}
							else
							{
								g_bSearchPaused = TRUE;
								SU_UpdateP2SearchStatus(SEARCHPAUSED_FORMAT);
								set_SU_VLF_RX_Mode(SU_RX_MODE_STANDBY);	// tell the MURX module to wait until it is told to decode
							}
						}
						else
							g_bSearchPaused = FALSE;
*/							
					}
					else
					{
						if (!g_bSearching) {
							retry_countdown = 3;
							g_bSearching = TRUE;
							oldlistsize = GetMUListSize();	// remember the current list size;								
							SU_UpdateP2SearchStatus(g_bAudioSearchMode?AUDIOSEARCH_FORMAT:ID_SEARCH_FORMAT);
							Send_P2_SearchMsg(g_bAudioSearchMode);			// send a search command
							lasttick = g_tick;		// start counting from now
							total_elapsedms = 0l;	// reset the elapsed ms counter for the next search command if necessary
							bRequestToMask = TRUE;
						}
					}
					break;
					
				case SU_BTN_UPDOWN:
					m_bRunningSearchMode = FALSE;	// TURN OFF BACKGROUND UPDATES FROM MU PACKETS
					SU_RunSettingsPage();
					m_bRunningSearchMode = TRUE;
					SU_DrawSearchPage();
					SU_Search_RedrawIDList();
					SU_UpdateGeneralStatus(TRUE,TRUE);
					SU_UpdateP2SearchStatus(g_bAudioSearchMode?AUDIOSEARCH_FORMAT:ID_SEARCH_FORMAT);
					break;					
				default:
					break;
			}
			if (oldsetting != g_iSelectedMuIndex)
			{
				SU_DrawMUSearchData(oldsetting, FALSE);
				if (!SU_isSearchRowVisible(g_iSelectedMuIndex))
					SU_Search_RedrawIDList();
				else
				{
					SU_DrawMUSearchData(g_iSelectedMuIndex, TRUE);
				}

				SU_UpdateCountDisplay();
			}
		}
	}
	return SU_BTN_EXIT;
}



// sends mask message to all 3 anntenna and then listens for a response in the P2 protocol
void SU_SendAndWaitForMU_P2_Mask(MuData* p, char antenna)
{
	unsigned long long lasttick;
	unsigned long long elapsedms=0;

	g_bTransmitting = TRUE;	// SET MODE TO TRANSMITTING TO ENSURE RSSI PACKETS IGNORED
	if (g_SuSettings.enable_audio_blanking) AUDIO_MUTE_LATCH = 1;	//normally low to enable the audio board and is high to mute the audio output.
	DelayMsecs(AUDIO_MUTE_DELAY_MS);				// delay to allow audio to be muted

	if (antenna == TX_ANTENNA_ALL) 
	{
		vlf_SendMaskMsg(p->id_u32,TX_ANTENNA_X);
		//DelayMsecs(MASK_RX_DELAY_MS_P2);			// SU_TX can't receive next command while it is transmitting		
		WaitForSUTXResponse(MASK_RX_DELAY_MS_P2);			// SU_TX can't receive next command while it is transmitting		
		vlf_SendMaskMsg(p->id_u32,TX_ANTENNA_Y);				
//		DelayMsecs(MASK_RX_DELAY_MS_P2);			// SU_TX can't receive next command while it is transmitting		
		WaitForSUTXResponse(MASK_RX_DELAY_MS_P2);			// SU_TX can't receive next command while it is transmitting		
		vlf_SendMaskMsg(p->id_u32,TX_ANTENNA_Z);				
	}
	else
	{
		vlf_SendMaskMsg(p->id_u32,antenna);				
	}
	p->m.status.masked = 0;				// RESET STATUS TO INVALID
	set_SU_VLF_RX_Mode(SU_RX_MODE_DECODE);
//	DelayMsecs(MASK_RX_DELAY_MS_P2);	// Delay to prevent audio of TX from being picked up		
	WaitForSUTXResponse(MASK_RX_DELAY_MS_P2);			// SU_TX can't receive next command while it is transmitting		
	AUDIO_MUTE_LATCH = 0;				// normally low to enable the audio board and is high to mute the audio output.
	g_bTransmitting = FALSE;			// RE-ENABLE RSSI PACKET UPDATES

	lasttick = g_tick;
	do {
		ProcessSURXStream();
		if (g_tick != lasttick) // update ate 1ms intervals
		{
			if (g_tick > lasttick)
				elapsedms += g_tick-lasttick;
			else
				elapsedms += 1;	// account for timer overflow
						
			lasttick = g_tick;
		}
	}
	while ((!p->m.status.masked) && (elapsedms < P2_WAIT_FOR_MASK_RESPONSE_MS));	
}

// go through each of the new IDs and send a mask message
void SU_MaskAllUnmaskedMU_P2_Units(void)
{
	int i;
	int listsize = GetMUListSize();
	
	for (i=0;i<listsize;i++) 
	{
		if (g_MuDeviceList[i].m.status.masked == 0)
			SU_SendAndWaitForMU_P2_Mask(&g_MuDeviceList[i],TX_ANTENNA_ALL);
	}
}

// sends full mask message to all 3 anntenna and then listens for a response in the P2 protocol
BOOL SU_SendAndWaitForMU_P2_FullMask_Search(MuData* p, char antenna, char minutes)
{
	unsigned long long lasttick;
	unsigned long long elapsedms=0;
	int nInitialCount = p->packet_count;

	g_bTransmitting = TRUE;	// SET MODE TO TRANSMITTING TO ENSURE RSSI PACKETS IGNORED
	if (g_SuSettings.enable_audio_blanking) AUDIO_MUTE_LATCH = 1;	//normally low to enable the audio board and is high to mute the audio output.
	DelayMsecs(AUDIO_MUTE_DELAY_MS);				// delay to allow audio to be muted

	if (antenna == TX_ANTENNA_ALL) 
	{
		vlf_SendFullMaskMsg(p->id_u32, minutes, TX_ANTENNA_X);
		WaitForSUTXResponse(MASK_RX_DELAY_MS_P2);			// SU_TX can't receive next command while it is transmitting		
		vlf_SendFullMaskMsg(p->id_u32, minutes, TX_ANTENNA_Y);				
		WaitForSUTXResponse(MASK_RX_DELAY_MS_P2);			// SU_TX can't receive next command while it is transmitting		
		vlf_SendFullMaskMsg(p->id_u32, minutes, TX_ANTENNA_Z);
	}
	else
	{
		vlf_SendFullMaskMsg(p->id_u32, minutes, antenna);
	}
	set_SU_VLF_RX_Mode(SU_RX_MODE_DECODE);
	WaitForSUTXResponse(MASK_RX_DELAY_MS_P2);			// SU_TX can't receive next command while it is transmitting		
	AUDIO_MUTE_LATCH = 0;				// normally low to enable the audio board and is high to mute the audio output.
	g_bTransmitting = FALSE;			// RE-ENABLE RSSI PACKET UPDATES

	lasttick = g_tick;
	do {
		ProcessSURXStream();
		if (g_tick != lasttick) // update ate 1ms intervals
		{
			if (g_tick > lasttick)
				elapsedms += g_tick-lasttick;
			else
				elapsedms += 1;	// account for timer overflow
						
			lasttick = g_tick;
		}
	}
	while ((nInitialCount == p->packet_count) && (elapsedms < P2_WAIT_FOR_FULL_MASK_RESPONSE_MS));
	if(nInitialCount == p->packet_count)
		return FALSE;
	else
		return TRUE;
}

BOOL SU_SendAndWaitForMU_P2_FullMask_3_Search(MuData* p, char antenna, char minutes)
{
	if(SU_SendAndWaitForMU_P2_FullMask_Search(p, antenna, minutes))
		return TRUE;
	else
	{
		if(SU_SendAndWaitForMU_P2_FullMask_Search(p, antenna, minutes))
			return TRUE;
		else
			return SU_SendAndWaitForMU_P2_FullMask_Search(p, antenna, minutes);
	}
}

#endif	// PROTOCOL 2



// Update the display of the number of items on the screen
void SU_UpdateCountDisplay()
{
	char buffer[50];
	int listsize = GetMUListSize();
	SetDefaultBackForegnd();		
	SLCD_SendCmd("fUARIAL12");		// sets the unicode character set for the title
#ifdef RUSSIAN
	if (g_iSelectedMuIndex >= 0)
		sprintf(buffer,"t \"%2d \xD0\xB8\xD0\xB7 %2d \" %d %d",g_iSelectedMuIndex+1,listsize,SEARCH_PAGE_COUNTSTATS_XPOS,SEARCH_PAGE_COUNTSTATS_YPOS);
	else
		sprintf(buffer,"t \"%2d          \" %d %d",listsize,SEARCH_PAGE_COUNTSTATS_XPOS,SEARCH_PAGE_COUNTSTATS_YPOS);
#else
	if (g_iSelectedMuIndex >= 0)
		sprintf(buffer,"t \"%2d of %2d \" %d %d",g_iSelectedMuIndex+1,listsize,SEARCH_PAGE_COUNTSTATS_XPOS,SEARCH_PAGE_COUNTSTATS_YPOS);
	else
		sprintf(buffer,"t \"%2d units  \" %d %d",listsize,SEARCH_PAGE_COUNTSTATS_XPOS,SEARCH_PAGE_COUNTSTATS_YPOS);
#endif
	SLCD_SendCmd(buffer);
}

// walks through the table of units to either update an existing one if found otherwise add it to the list
void SU_UpdateUnit(MuData* p)
{
	int i;
	BOOL bFound = FALSE;

	int listsize = GetMUListSize();
	
	if ((m_bRunningSearchMode == FALSE) && (m_bLocateMode == FALSE) && (isRunningFullMaskPage()==FALSE))	// IGNORE INPUT IF WE'RE NOT SHOWING IT
		return;
		
	if (p->v.ID_valid) 	// allow any packet type to update the data for the ID if it is valid
	{
		if (g_SuSettings.enable_beep_on_VLF_packet) 
			SLCD_SendCmd("beep 25");

		if (listsize) 
		{
			for (i=0;i<listsize;i++) {
				if ((g_MuDeviceList[i].id_u32 == p->id_u32)) {
					traceS("Updating existing ID");
					g_MuDeviceList[i].antenna = p->antenna;
						
					if (p->v.Mobility_valid)	{					// only change if valid
						g_MuDeviceList[i].m.mobility = p->m.mobility;
						g_MuDeviceList[i].v.Mobility_valid = 1;
//						traceS("Mobility Valid");
					}
					
					/////////////////////////// 1F RSSI ///////////////////////////////////////
					if (p->v.X1_power_valid)	{						// only change if valid
						g_MuDeviceList[i].x1_power = p->x1_power;
						g_MuDeviceList[i].v.X1_power_valid = 1;
						UpdatePeakPower_1F(p->x1_power);
					}
					
					if (p->v.Y1_power_valid)	{						// only change if valid
						g_MuDeviceList[i].y1_power = p->y1_power;
						g_MuDeviceList[i].v.Y1_power_valid = 1;
						UpdatePeakPower_1F(p->y1_power);

					}
					
					if (p->v.Z1_power_valid)	{						// only change if valid
						g_MuDeviceList[i].z1_power = p->z1_power;
						g_MuDeviceList[i].v.Z1_power_valid = 1;
						UpdatePeakPower_1F(p->z1_power);
					}
					/////////////////////////// 2F RSSI ///////////////////////////////////////

/*					if (p->v.X2_power_valid)	{						// only change if valid
						g_MuDeviceList[i].x2_power = p->x2_power;
						g_MuDeviceList[i].v.X2_power_valid = 1;
						UpdatePeakPower_2F(p->x2_power);
					}
					
					if (p->v.Y2_power_valid)	{						// only change if valid
						g_MuDeviceList[i].y2_power = p->y2_power;
						g_MuDeviceList[i].v.Y2_power_valid = 1;
						UpdatePeakPower_2F(p->y2_power);
					}
					
					if (p->v.Z2_power_valid)	{						// only change if valid
						g_MuDeviceList[i].z2_power = p->z2_power;
						g_MuDeviceList[i].v.Z2_power_valid = 1;
						UpdatePeakPower_2F(p->z2_power);
					}
*/						
					bFound = TRUE;
					g_MuDeviceList[i].packet_count++;
					
					if (m_bRunningSearchMode)
					{
						SU_DrawMUSearchData(i, (i == g_iSelectedMuIndex)?TRUE:FALSE);	// deselect previous
					}
					break;
				}
			}
		}
	
		if ((!bFound) && (listsize < (MAXIMUM_MU_DEVICES_IN_LIST-1))) {
			traceS("Adding New ID");
			g_MuDeviceList[listsize] = *p;	// insert the entry	
	
			if (m_bRunningSearchMode) {	// update display in search mode

				if (g_iSelectedMuIndex >= 0) {
					SU_DrawMUSearchData(g_iSelectedMuIndex, FALSE);	// deselect previous
				}
				g_iSelectedMuIndex = listsize;	// automatically select the last in the list			// set to new entry
				if (!SU_isSearchRowVisible(g_iSelectedMuIndex))
					SU_Search_RedrawIDList();
				else
				{
					SU_DrawMUSearchData(listsize, (listsize == g_iSelectedMuIndex)?TRUE:FALSE);
				}	
	
				SU_UpdateCountDisplay();
			}
			else if (isRunningFullMaskPage() == TRUE)
			{
				g_iSelectedMuIndex = listsize;	// automatically select the last in the list
			}
		}

	}

}




#ifdef ENABLE_TEST_FAKEHPT_MODE
// test routine to mimic receiving a packet from the murx module with the id of a new MU unit
void SLCD_Test_Search(BOOL bRunTest)
{
	static int count = 0;
	static long id = 16777211;
	char buffer[50];
	int i;

	traceS("SLCD_Test_Search");
	if (bRunTest) {			// add a unit
		if ((g_bSearching) && (!g_bSearchPaused)) {
			if (count < MAXIMUM_MU_DEVICES_IN_LIST) 
			{
				for (i=0;i<5;i++)	// add 5 new devices
				{
					sprintf(buffer,"%s\r\n",SU_TX_ACK_CSV_HEADER);
					PortRx_pushBytes(buffer,strlen(buffer),SUTX_UART_PORT);	
					sprintf(buffer,"%s,%lu,5,10,10,10\r\n",SU_RX_MU_RESPONSE,id++);
					PortRx_pushBytes(buffer,strlen(buffer), SURX_UART_PORT);	
					PortPutStr(buffer,PC_PORT,1);
				}
			}
			else
				g_bSearching = FALSE;
		}	
	}		
	else // if not adding then this is an initialization call
	{		
		count = 0;
		id = 16777211;
	}
}

// test routine to mimic receiving a packet from the murx module with the mobility and power
void SLCD_Test_MaskReponse(U32 id_u32, U8 minutes)
{
	char buffer[50];
	U8 status = 0x05;
	
	if (id_u32 == 0L)
		id_u32 = rand()%100;	// INSERT RANDOM ID IF ID NOT SPECIFIED
	if (minutes != 0)
		status |= 0x80;			// set the masked bit if we are trying to mask it - otherwise we leave it at 0 for unmask
		
	traceS("SLCD_Test_MaskReponse");
	sprintf(buffer,"%s,%ld,%d,%d,%d,%d\r\n",SU_RX_MU_RESPONSE,id_u32,status,rand()%15,rand()%15,rand()%15);
	PortRx_pushBytes(buffer,strlen(buffer),SURX_UART_PORT);	
	PortPutStr(buffer,PC_PORT,1);
}
#endif


