//FULL_MASK 

#ifndef PROTOCOL_2
	#error THIS IMPLEMENTATION IS ONLY DEFINED FOR PROTOCOL 2
#endif

#include "su_slcd.h"
#include <stdio.h>
#include <stdlib.h>
#include "vlf_rxtx.h"
#include "string.h"
#include "sdfunctions.h"
#include "trace.h"
#include "vlf_pkt.h"

extern SuSettings g_SuSettings;

//////////////////////////////////////// VARIABLES ////////////////////////////////////////
static int m_iSelectedFullMaskEntry = -1;
static int m_iFullMask_TableDisplayBase = 0;
static BOOL m_bRunningFullMaskPage = FALSE;

BOOL isRunningFullMaskPage() { return m_bRunningFullMaskPage; };

/////////////////////////////////////////// local prototypes ////////////////////////////
void SU_FullMask_DrawTableTitles();
BOOL SU_isFullMaskRowVisible(int index);
void SU_FullMask_DrawTableGrid();
void SU_FullMask_DrawTableRowGrid(int row);
void SU_FullMask_DrawPage();
void SU_FullMask_UpdatePageStatus(int mode);
void SU_FullMask_DrawRowData(int index, BOOL bSelected);
void SU_FullMask_UpdateCountDisplay();
BOOL SU_SendAndWaitForMU_P2_FullMask(U32 id_u32, char antenna, char minutes);
BOOL SU_SendAndWaitForMU_P2_FullMask_3(U32 id_u32, char antenna, char minutes);
BOOL SU_SendAndWaitForMU_P2_FullMask_Table(U32 id_u32, char antenna, char minutes);
BOOL SU_SendAndWaitForMU_P2_FullMask_3_Table(U32 id_u32, char antenna, char minutes);
//////////////////////////////////////////// local defines ///////////////////////////////

#define FULLMASK_INDENT_FROM_GRID 3
#define FULLMASK_INDENT_FROM_GRID_ADDITIONAL 60
#define FULLMASK_GRID_ROW_HEIGHT 26

#define ID_FULLMASK_GRID_TITLE_YPOS (PAGE_START_BELOW_HEADER + 34)
#define ID_FULLMASK_GRID_START_YPOS (ID_FULLMASK_GRID_TITLE_YPOS + FULLMASK_GRID_ROW_HEIGHT)
#define ID_FULLMASK_GRID_MAX_ROWS 10
#define ID_FULLMASK_GRID_END_YPOS (((ID_FULLMASK_GRID_MAX_ROWS)*FULLMASK_GRID_ROW_HEIGHT) + ID_FULLMASK_GRID_START_YPOS)


//	#define FULLMASK_PAGE_STATUS_XPOS (SLCD_MAX_X/2-20)
#define FULLMASK_PAGE_STATUS_XPOS 30

#define FULLMASK_PAGE_STATUS_YPOS (SLCD_MAX_Y - 64)
#define FULLMASK_PAGE_COUNTSTATS_XPOS 5
#define FULLMASK_PAGE_COUNTSTATS_YPOS (SLCD_MAX_Y - 90)

#define FULLMASK_GRID_COL0_XPOS 0
#define FULLMASK_GRID_COL1_XPOS 95

#define STANDBY_FORMAT   0
#define FULL_MASKING_FORMAT 1
#define UNMASKING_FORMAT 2
#define UNMASKALL_FORMAT 3

// Update the display of the number of items on the screen
void SU_FullMask_UpdateCountDisplay()
{
	char buffer[50];
	int listsize = GetMUFullMaskListSize();
	SetDefaultBackForegnd();		
	SLCD_SendCmd("fUARIAL12");		// sets the unicode character set for the title
#ifdef RUSSIAN
	if (m_iSelectedFullMaskEntry >= 0)
		sprintf(buffer,"t \"%2d \xD0\xB8\xD0\xB7 %2d \" %d %d",m_iSelectedFullMaskEntry+1,listsize,FULLMASK_PAGE_COUNTSTATS_XPOS,FULLMASK_PAGE_COUNTSTATS_YPOS);
	else
		sprintf(buffer,"t \"%2d          \" %d %d",listsize,FULLMASK_PAGE_COUNTSTATS_XPOS,FULLMASK_PAGE_COUNTSTATS_YPOS);
#else
	if (m_iSelectedFullMaskEntry >= 0)
		sprintf(buffer,"t \"%2d of %2d \" %d %d",m_iSelectedFullMaskEntry+1,listsize,FULLMASK_PAGE_COUNTSTATS_XPOS,FULLMASK_PAGE_COUNTSTATS_YPOS);
	else
		sprintf(buffer,"t \"%2d units  \" %d %d",listsize,FULLMASK_PAGE_COUNTSTATS_XPOS,FULLMASK_PAGE_COUNTSTATS_YPOS);
#endif
	SLCD_SendCmd(buffer);
}


// draws the data for one row
void SU_FullMask_DrawRowData(int index, BOOL bSelected)
{
	char buffer[80];
	int y;
	int x = 0;
	BOOL bIDValid = FALSE;
	int ymax = ID_FULLMASK_GRID_END_YPOS;
	
	if ((index < 0) || (index >= MAXIMUM_MU_FULLMASK_DEVICES_IN_LIST))	// make sure index is valid
		return;

	MuFullMaskData* p = &g_MuFullMaskDeviceList[index];

	// if row is out of range then simply return
	if (!SU_isFullMaskRowVisible(index)) return;

	SLCD_SendCmd("f12x24");	// select fixed font size - note this font only works for numbers and English
	y = ID_FULLMASK_GRID_START_YPOS;
	y += (FULLMASK_GRID_ROW_HEIGHT * (index - m_iFullMask_TableDisplayBase));
	if (y > (ymax - FULLMASK_GRID_ROW_HEIGHT))
		return;

	y+=1;
	// draw the background selection color
	if (bSelected) {
		SetForeBackGndColor(MENU_SELECTED_COLOR,SLCD_BLACK);
	}
	else {
		SetForeBackGndColor(SLCD_BACKGROUND,SLCD_FOREGROUND);
	}
	
	sprintf(buffer,"r %d %d %d %d 1",0,y, SLCD_MAX_X -1, y + FULLMASK_GRID_ROW_HEIGHT -2);			// draw a rectangle with the foreground color
	SLCD_SendCmd(buffer);

	// draw the text forground
	if (bSelected)
		SetForeBackGndColor(SLCD_BLACK,SLCD_YELLOW);
	else
		SetDefaultBackForegnd();

	SLCD_SendCmd("fUARIAL14");		// sets the unicode character set for the title
	x = FULLMASK_GRID_COL0_XPOS + FULLMASK_INDENT_FROM_GRID;

	if (p->id_u32 > 0L) // only draw the number of it is valid i.e. - greater than 0
	{
		sprintf(buffer,"t \"%8lu\" %d %d",(p->id_u32)&0x00ffffff,x,y);	// upper byte is reserved for type
		bIDValid = TRUE;
	}
	else
		sprintf(buffer,"t \"             \" %d %d",x,y);
		
	SLCD_SendCmd(buffer);

	//SLCD_SendCmd("fUARIAL14");		// sets the unicode character set for the title
	x = FULLMASK_GRID_COL1_XPOS + FULLMASK_INDENT_FROM_GRID + FULLMASK_INDENT_FROM_GRID_ADDITIONAL;
	if (bIDValid)
	{
		int32 delta = g_seconds - p->last_fullmask_time;
		int seconds=0;
		if (delta < MU_FULL_MASK_SECONDS)
			seconds = MU_FULL_MASK_SECONDS-delta;

		if (seconds > 0)
		{
			int seconds =(MU_FULL_MASK_SECONDS-(g_seconds - p->last_fullmask_time));
			sprintf(buffer,"t \"%02d:%02d   \" %d %d",seconds/60,seconds%60,x,y);	
		}	
		else
		{
			#ifdef RUSSIAN
				sprintf(buffer,"t \"00:00  \" %d %d",x,y);	
			#else
				sprintf(buffer,"t \"Timeout!  \" %d %d",x,y);	
			#endif
		}
	}
	else
	{
		sprintf(buffer,"t \"          \" %d %d",x,y);	
	}
	SLCD_SendCmd(buffer);
	
	SU_FullMask_DrawTableRowGrid(index);
}

// checks to see if the row is within display range
BOOL SU_isFullMaskRowVisible(int row)
{
		// if row is out of range then
	int maxrows = ID_FULLMASK_GRID_MAX_ROWS;
	
	if ((row < m_iFullMask_TableDisplayBase) || (row >= (m_iFullMask_TableDisplayBase + maxrows))) {
		return FALSE;
	}
	else
		return TRUE;
}


// decides how to redraw the table based on the table size and position of currently selected row
void SU_FullMask_RedrawTable()
{
	char buffer[50];
	int row;
	int oldpos = m_iFullMask_TableDisplayBase;
	int y;
	int maxrows = ID_FULLMASK_GRID_MAX_ROWS;
	int maxentries = GetMUFullMaskListSize();
	
	if (m_iSelectedFullMaskEntry >= maxentries)
		m_iSelectedFullMaskEntry = maxentries-1;

	if ((m_iSelectedFullMaskEntry >= 0) && (m_iSelectedFullMaskEntry < MAXIMUM_MU_FULLMASK_DEVICES_IN_LIST)) 
	{
		if(m_iSelectedFullMaskEntry < m_iFullMask_TableDisplayBase) 
		{
			m_iFullMask_TableDisplayBase = m_iSelectedFullMaskEntry;
		}
		else 
		{ 		
			if (m_iSelectedFullMaskEntry > (m_iFullMask_TableDisplayBase + maxrows - 1)) 
			{
				m_iFullMask_TableDisplayBase = m_iSelectedFullMaskEntry - maxrows + 1;
			}
		}
	}

	// scroll rectangle up or down and then redraw the new row
	if (abs(m_iFullMask_TableDisplayBase-oldpos) == 1) 
	{ // if it only moved by 1 row then scroll the screen
		y = ID_FULLMASK_GRID_START_YPOS;
		if (m_iFullMask_TableDisplayBase > oldpos) 
		{
//			y+= FULLMASK_GRID_ROW_HEIGHT;
			sprintf(buffer,"k 0 %d %d %d %du",y,SLCD_MAX_X,y + (maxrows*FULLMASK_GRID_ROW_HEIGHT),FULLMASK_GRID_ROW_HEIGHT);
			SLCD_SendCmd(buffer);
			y = m_iFullMask_TableDisplayBase + maxrows - 1;
			SU_FullMask_DrawRowData(y, (y == m_iSelectedFullMaskEntry)?TRUE:FALSE);
		}
		else 
		{
			sprintf(buffer,"k 0 %d %d %d %dd",y,SLCD_MAX_X,y + (maxrows*FULLMASK_GRID_ROW_HEIGHT),FULLMASK_GRID_ROW_HEIGHT);
			SLCD_SendCmd(buffer);
			y = m_iFullMask_TableDisplayBase;
			SU_FullMask_DrawRowData(y, (y == m_iSelectedFullMaskEntry)?TRUE:FALSE);
		}
	}
	else 
	{	// Redraw the whole screen
		for (row=m_iFullMask_TableDisplayBase;row<m_iFullMask_TableDisplayBase+maxrows;row++) 
		{
			if (g_MuFullMaskDeviceList[row].id_u32 == 0L) 
				break;
			SU_FullMask_DrawRowData(row, (row == m_iSelectedFullMaskEntry)?TRUE:FALSE);
		}
	}
	SU_FullMask_UpdateCountDisplay();
}

// decides how to redraw the table based on the table size and position of currently selected row
void SU_FullMask_RedrawTableToIndex(int index)
{
	int row;
	int maxrows = ID_FULLMASK_GRID_MAX_ROWS;
	int maxentries = GetMUFullMaskListSize();
	
	if (m_iSelectedFullMaskEntry >= maxentries)
		m_iSelectedFullMaskEntry = maxentries-1;

	if ((m_iSelectedFullMaskEntry >= 0) && (m_iSelectedFullMaskEntry < MAXIMUM_MU_FULLMASK_DEVICES_IN_LIST)) 
	{
		if(m_iSelectedFullMaskEntry < m_iFullMask_TableDisplayBase) 
		{
			m_iFullMask_TableDisplayBase = m_iSelectedFullMaskEntry;
		}
		else 
		{ 		
			if (m_iSelectedFullMaskEntry > (m_iFullMask_TableDisplayBase + maxrows - 1)) 
			{
				m_iFullMask_TableDisplayBase = m_iSelectedFullMaskEntry - maxrows + 1;
			}
		}
	}

	for (row=m_iFullMask_TableDisplayBase;row<m_iFullMask_TableDisplayBase+maxrows;row++) 
	{
		SU_FullMask_DrawRowData(row, (row == m_iSelectedFullMaskEntry)?TRUE:FALSE);
		if (row >= index)
			break;
	}
	SU_FullMask_UpdateCountDisplay();
}


// Draws the background grid for the units in the search window
void SU_FullMask_DrawTableGrid()
{
	char buf[30];
	int y;
	int ymax = ID_FULLMASK_GRID_END_YPOS;

	SLCD_SendCmd("p 1");

	// Draw the Row lines	
	y = ID_FULLMASK_GRID_START_YPOS;
	for (; y <= ymax; y+= FULLMASK_GRID_ROW_HEIGHT) {
		sprintf(buf,"l 0 %d %d %d",y,SLCD_MAX_X,y);
		SLCD_SendCmd(buf);
	}
	
	// Draw the TITLE column lines
	y=ID_FULLMASK_GRID_TITLE_YPOS;
	SetForeBackGndColor(SLCD_BLACK,SLCD_FOREGROUND);	// DRAW IN BLACK
	sprintf(buf,"l %d %d %d %d",FULLMASK_GRID_COL1_XPOS,y,FULLMASK_GRID_COL1_XPOS,y + FULLMASK_GRID_ROW_HEIGHT);
	SLCD_SendCmd(buf);

	
	y=ID_FULLMASK_GRID_START_YPOS;
	SetDefaultBackForegnd();

	// Draw the GRID column lines
	sprintf(buf,"l %d %d %d %d",FULLMASK_GRID_COL1_XPOS,y,FULLMASK_GRID_COL1_XPOS,ymax);
	SLCD_SendCmd(buf);


}

// draws the grid for the row
void SU_FullMask_DrawTableRowGrid(int row)
{
	char buf[50];
	int y1,y2;

	SLCD_SendCmd("p 1");
	SetForeBackGndColor(SLCD_BLACK,SLCD_BACKGROUND);	// DRAW IN BLACK

	// Draw the Row lines
	y1 = ID_FULLMASK_GRID_START_YPOS;
	y1 += (FULLMASK_GRID_ROW_HEIGHT * (row - m_iFullMask_TableDisplayBase));
	y2 = y1 + FULLMASK_GRID_ROW_HEIGHT;

	sprintf(buf,"l 0 %d %d %d",y1,SLCD_MAX_X,y1);		// draw top row-line
	SLCD_SendCmd(buf);

	sprintf(buf,"l 0 %d %d %d",y2,SLCD_MAX_X,y2);		// draw bottom row-line
	SLCD_SendCmd(buf);

	// Draw the column lines
	sprintf(buf,"l %d %d %d %d",FULLMASK_GRID_COL1_XPOS,y1,FULLMASK_GRID_COL1_XPOS,y2);
	SLCD_SendCmd(buf);

}

// Draws the data corresponding to an MU (also referred to as an HPT) unit
void SU_FullMask_DrawTableTitles()
{
	char buffer[50];
	int y = ID_FULLMASK_GRID_TITLE_YPOS;
	int x = 0;

	sprintf(buffer,"s %d 0",SLCD_LIGHT_GREY);		// set foreground to Black, background to light grey
	SLCD_SendCmd(buffer);

	sprintf(buffer,"r 0 %d %d %d 1",y, SLCD_MAX_X, y+FULLMASK_GRID_ROW_HEIGHT);	// draw a rectangle with the foreground color
	SLCD_SendCmd(buffer);

	sprintf(buffer,"s 0 %d",SLCD_LIGHT_GREY);		// set foreground to Black, background to light grey
	SLCD_SendCmd(buffer);

	y++;

	SLCD_SendCmd("utf8 on");		// sets unicode
	SLCD_SendCmd("fUARIAL14");		// sets the unicode character set for the title
#ifdef RUSSIAN
	#define COL1_TITLE "t \" ID\" %d %d"
	#define COL2_TITLE "t \"\xD0\x9E\xD1\x81\xD1\x82\xD0\xB0\xD0\xB2\xD1\x88\xD0\xB5\xD0\xB5\xD1\x81\xD1\x8F \xD0\xB2\xD1\x80\xD0\xB5\xD0\xBC\xD1\x8F\" %d %d"
#else
	#define COL1_TITLE "t \" ID\" %d %d"
	#define COL2_TITLE "t \" Mask Countdown\" %d %d"
#endif
	
	x = FULLMASK_GRID_COL0_XPOS + FULLMASK_INDENT_FROM_GRID;
	sprintf(buffer,COL1_TITLE,x,y);			// id
	SLCD_SendCmd(buffer);
//	SLCD_SendCmd("fUARIAL12");		// sets the unicode character set for the title
	x = FULLMASK_GRID_COL1_XPOS + FULLMASK_INDENT_FROM_GRID;
	sprintf(buffer,COL2_TITLE,x,y);			// countdown
	SLCD_SendCmd(buffer);
}




// Main drawing routine to draw the full page - but does not redraw the data 
void SU_FullMask_DrawPage()
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
	
	SLCD_SendCmd("fUARIAL18");		// sets the unicode character set for the title
#ifdef RUSSIAN
	SLCD_SendCmd("t \"\xD0\x98\xD1\x81\xD0\xBA\xD0\xBB\xD1\x8E\xD1\x87\xD1\x91\xD0\xBD\xD0\xBD\xD1\x8B\xD0\xB5 \xD0\xB8\xD0\xB7 \xD0\xBF\xD0\xBE\xD0\xB8\xD1\x81\xD0\xBA\xD0\xB0\" 4 50 T");
#else	
	SLCD_SendCmd("t \"Full Mask Table\" 50 50 T");
#endif
		
	SU_FullMask_DrawTableTitles();

	SetDefaultBackForegnd();		
	SU_FullMask_DrawTableGrid();

	SU_InitMainBatteryBar();	// creates and display battery bar
	SU_InitTXBatteryBar();

	SU_DisplayStandardNavigationButtons(x+adder,y, adder,TRUE);
}

// Updates the page status such as Standby and Full Mask and draws the footer based on the currrent mode
void SU_FullMask_UpdatePageStatus(int mode)
{
	//char buffer[128];
	char buffer[256];
	
	#ifdef INCLUDE_MINERADIO_HEADER
		SET_TO_MINERADIO_BACKGROUND;
	#else
		SetDefaultBackForegnd();
	#endif	

	
	char *msg = "";

#ifdef RUSSIAN
	SLCD_SendCmd("utf8 on");		// sets unicode
	SLCD_SendCmd("fUARIAL14");		// sets the unicode character set for the title
	SLCD_SendCmd("tfx 0");	// delete flashing if enabled
	sprintf(buffer,"t \"%45s\" %d %d", " ", FULLMASK_PAGE_STATUS_XPOS,FULLMASK_PAGE_STATUS_YPOS);	// ERASE OLD LINE
	SLCD_SendCmd(buffer);
	if (mode == STANDBY_FORMAT)
	{
		SLCD_WriteFooter("*=\xD0\x92\xD0\xB5\xD1\x80\xD0\xBD\xD1\x83\xD1\x82\xD1\x8C \xD0\xB2 \xD0\xBF\xD0\xBE\xD0\xB8\xD1\x81\xD0\xBA \xD0\xB2\xD1\x8B\xD0\xB1\xD1\x80\xD0\xB0\xD0\xBD\xD0\xBD\xD1\x8B\xD0\xB9 \xD1\x82\xD0\xB0\xD0\xB3     ",
				"F1=\xD0\x92\xD0\xB5\xD1\x80\xD0\xBD\xD1\x83\xD1\x82\xD1\x8C \xD0\xB2 \xD0\xBF\xD0\xBE\xD0\xB8\xD1\x81\xD0\xBA \xD0\xB2\xD0\xB5\xD1\x81\xD1\x8C \xD1\x81\xD0\xBF\xD0\xB8\xD1\x81\xD0\xBE\xD0\xBA",
				"F2=\xD0\x9F\xD1\x80\xD0\xBE\xD0\xB4\xD0\xBB\xD0\xB8\xD1\x82\xD1\x8C \xD0\xB8\xD1\x81\xD0\xBA\xD0\xBB\xD1\x8E\xD1\x87\xD0\xB5\xD0\xBD\xD0\xB8\xD0\xB5 \xD0\xB4\xD0\xBB\xD1\x8F \xD0\xB2\xD1\x81\xD0\xB5\xD1\x85");
	}
	else
	{
		switch (mode)
		{
			case FULL_MASKING_FORMAT:
				msg = "\xD0\x98\xD1\x81\xD0\xBA\xD0\xBB\xD1\x8E\xD1\x87\xD0\xB5\xD0\xBD\xD0\xB8\xD0\xB5 \xD0\xB8\xD0\xB7 \xD0\xBF\xD0\xBE\xD0\xB8\xD1\x81\xD0\xBA\xD0\xB0";
				break;
			case UNMASKING_FORMAT:
			case UNMASKALL_FORMAT:
				msg = "\xD0\x92\xD0\xBA\xD0\xBB\xD1\x8E\xD1\x87\xD0\xB5\xD0\xBD\xD0\xB8\xD0\xB5 \xD0\xB2 \xD0\xBF\xD0\xBE\xD0\xB8\xD1\x81\xD0\xBA";
				break;
			default:
				msg = "";	
		}
		sprintf(buffer,"tf 0 500 \"%40s\" %d %d T", msg, FULLMASK_PAGE_STATUS_XPOS,FULLMASK_PAGE_STATUS_YPOS);
		SLCD_SendCmd(buffer);
		SLCD_WriteFooter("                                                                     ","                                                                     ","                                                                     ");
	}
#else	
	switch (mode)
	{
		case STANDBY_FORMAT:
			msg = "Standby    ";
			break;
		case FULL_MASKING_FORMAT:
			msg = "Full Masking ";
			break;
		case UNMASKING_FORMAT:
			msg = "Unmasking    ";
			break;
		case UNMASKALL_FORMAT:
			msg = "Unmasking All";
			break;
		default:
			msg = "             ";	
	}
	SLCD_SendCmd("utf8 on");		// sets unicode
	SLCD_SendCmd("fUARIAL14");		// sets the unicode character set for the title
	if (mode == STANDBY_FORMAT)
	{
		SLCD_SendCmd("tfx 0");	// delete flashing if enabled
		sprintf(buffer,"t \"%45s\" %d %d", " ", FULLMASK_PAGE_STATUS_XPOS,FULLMASK_PAGE_STATUS_YPOS);	// ERASE OLD LINE
		SLCD_SendCmd(buffer);
		sprintf(buffer,"t \"%30s\" %d %d",msg, FULLMASK_PAGE_STATUS_XPOS,FULLMASK_PAGE_STATUS_YPOS);
	}
	else
	{
		SLCD_SendCmd("tfx 0");	// delete flashing if enabled
		sprintf(buffer,"t \"%45s\" %d %d", " ", FULLMASK_PAGE_STATUS_XPOS,FULLMASK_PAGE_STATUS_YPOS);	// ERASE OLD LINE
		SLCD_SendCmd(buffer);
		sprintf(buffer,"tf 0 500 \"%30s\" %d %d T",msg, FULLMASK_PAGE_STATUS_XPOS,FULLMASK_PAGE_STATUS_YPOS);	
	}
	SLCD_SendCmd(buffer);
	SLCD_WriteFooter("","*=Unmask Selected","F1=Unmask All   F2=Mask All  ");
#endif
}

// Main loop running the Full Mask page
int SU_Run_FullMaskPage()
{
	struct button btn;
	time_t ref_second = g_seconds;
	int seconds = 0;
	int oldsetting;
	int listsize;
	unsigned long long lasttick = 0l;
	unsigned long total_elapsedms;
	
	m_bRunningFullMaskPage = TRUE;
	listsize = GetMUFullMaskListSize();
	if (listsize == 0)
		m_iSelectedFullMaskEntry = -1;		// reset the index of the selected one to none
	else
		m_iSelectedFullMaskEntry = 0;
		
	m_iFullMask_TableDisplayBase = 0;	// reset the display base to the beginning of the list
	SU_FullMask_DrawPage();
	SU_FullMask_RedrawTable();

	set_SU_VLF_RX_Mode(SU_RX_MODE_STANDBY);	// tell the MURX module to wait until it is told to decode
	SU_UpdateGeneralStatus(TRUE,TRUE);		// display battery level etc...
	SU_FullMask_UpdatePageStatus(STANDBY_FORMAT);
	lasttick = g_tick;
	total_elapsedms = 0l;	// reset the elapsed ms counter for the next search command if necessary
		
	while (1) 	// loop until we get an exit from the page
	{			
		if (g_tick != lasttick) // update at 1ms intervals
		{
			if (g_tick > lasttick)
				total_elapsedms += g_tick-lasttick;
			else
				total_elapsedms += 1;	// account for timer overflow
			
			lasttick = g_tick;
			
			if (ref_second != g_seconds) // update at second intervals
			{
				ref_second = g_seconds;
				SU_UpdateGeneralStatus(TRUE,FALSE); // display battery level etc...
				//SU_FullMask_RedrawTable();	// refresh IDs in table to show the countdowns every second
				seconds++;
				if (seconds >= 5)
				{
					SU_UpdateCOMStatus(FALSE);	// check to see if we got a response from the last ping
					StartPingStatus();			// reset and check again
					SU_FullMask_RedrawTable();	// refresh IDs in table to show the countdowns every 5 seconds
					seconds = 0;
				}
			}
		}
			
		unsigned short taskStatus = ProcessBackgroundTasks();
		
		btn = GetUserAction(taskStatus);
		if (btn.index != 0) 
		{
			oldsetting = m_iSelectedFullMaskEntry;
			listsize = GetMUFullMaskListSize();
		
			switch (btn.index) 
			{
				case SU_BTN_UP:
					if (m_iSelectedFullMaskEntry >= 0) 
					{
						if (m_iSelectedFullMaskEntry > 0)
							m_iSelectedFullMaskEntry--;
						else
							m_iSelectedFullMaskEntry = listsize -1;
					}
					break;
				case SU_BTN_DOWN:
					if (m_iSelectedFullMaskEntry >= 0) 
					{
						if (m_iSelectedFullMaskEntry >= (listsize-1)) 
							m_iSelectedFullMaskEntry = 0;
						else
							m_iSelectedFullMaskEntry++;

					}
					break;
			
				case SU_BTN_EXIT:
					set_SU_VLF_RX_Mode(SU_RX_MODE_STANDBY);	// tell the MURX module to wait until it is told to decode
					m_bRunningFullMaskPage = FALSE;
					return SU_BTN_EXIT;
					break;
			
				case SU_BTN_F2:  //"F2=Mask All  "
					if (listsize > 0) 
					{
						SU_FullMask_UpdatePageStatus(FULL_MASKING_FORMAT);
						int i;
						for (i=0;i<listsize;i++)
						{
#ifdef ENABLE_TEST_FAKEHPT_MODE
							if(SU_SendAndWaitForMU_P2_FullMask_3(g_MuFullMaskDeviceList[i].id_u32,TX_ANTENNA_ALL, MU_FULL_MASK_MINUTES))
#else
							if(SU_SendAndWaitForMU_P2_FullMask_3_Table(g_MuFullMaskDeviceList[i].id_u32, TX_ANTENNA_ALL, MU_FULL_MASK_MINUTES))
#endif
								SU_MoveHPTTo_FullMaskList(g_MuFullMaskDeviceList[i].id_u32);
							SU_FullMask_RedrawTable();
						}
						SU_FullMask_UpdatePageStatus(STANDBY_FORMAT);	
					}
					break;	
				case SU_BTN_ENTER: //"*=Unmask Selected"
					if (m_iSelectedFullMaskEntry >= 0) 
					{
						if((g_seconds - g_MuFullMaskDeviceList[m_iSelectedFullMaskEntry].last_fullmask_time) < MU_FULL_MASK_SECONDS)
						{
							SU_FullMask_UpdatePageStatus(UNMASKING_FORMAT);
#ifdef ENABLE_TEST_FAKEHPT_MODE
							if(SU_SendAndWaitForMU_P2_FullMask_3(g_MuFullMaskDeviceList[m_iSelectedFullMaskEntry].id_u32,TX_ANTENNA_ALL, 0))
#else
							if(SU_SendAndWaitForMU_P2_FullMask_3_Table(g_MuFullMaskDeviceList[m_iSelectedFullMaskEntry].id_u32, TX_ANTENNA_ALL, 0))
#endif
								SU_RemoveHPTFrom_FullMaskList(g_MuFullMaskDeviceList[m_iSelectedFullMaskEntry].id_u32);
						}	
						else
							SU_RemoveHPTFrom_FullMaskList(g_MuFullMaskDeviceList[m_iSelectedFullMaskEntry].id_u32);
						SU_FullMask_RedrawTableToIndex(listsize);
						SU_FullMask_UpdatePageStatus(STANDBY_FORMAT);
					}
					break;
				case SU_BTN_F1: //"F1=Unmask All"
					if (listsize > 0) 
					{
						int i;
						for (i=(listsize-1);i>=0;i--)
						{
							if((g_seconds - g_MuFullMaskDeviceList[i].last_fullmask_time) < MU_FULL_MASK_SECONDS)
							{
								SU_FullMask_UpdatePageStatus(UNMASKALL_FORMAT);
#ifdef ENABLE_TEST_FAKEHPT_MODE
								if(SU_SendAndWaitForMU_P2_FullMask_3(g_MuFullMaskDeviceList[i].id_u32,TX_ANTENNA_ALL, 0))
#else
								if(SU_SendAndWaitForMU_P2_FullMask_3_Table(g_MuFullMaskDeviceList[i].id_u32, TX_ANTENNA_ALL, 0))
#endif
									SU_RemoveHPTFrom_FullMaskList(g_MuFullMaskDeviceList[i].id_u32);
							}
							else
								SU_RemoveHPTFrom_FullMaskList(g_MuFullMaskDeviceList[i].id_u32);
							SU_FullMask_RedrawTableToIndex(i);
						}
						SU_FullMask_UpdatePageStatus(STANDBY_FORMAT);	
					}
					break;
					
/*	Settings page is disabled since this page can be called from settings and to call it again can create excessive depth on the stack
				case SU_BTN_UPDOWN:
					m_bRunningFullMaskPage = FALSE;
					SU_RunSettingsPage();
					m_bRunningFullMaskPage = TRUE;
					SU_FullMask_DrawPage();
					SU_FullMask_RedrawTable();
					SU_UpdateGeneralStatus(TRUE,TRUE);
					SU_FullMask_UpdatePageStatus(STANDBY_FORMAT);
					break;	*/				
				default:
					break;
			}
			if (oldsetting != m_iSelectedFullMaskEntry)
			{
				SU_FullMask_DrawRowData(oldsetting, FALSE);
				if (!SU_isFullMaskRowVisible(m_iSelectedFullMaskEntry))
					SU_FullMask_RedrawTable();
				else
				{
					SU_FullMask_DrawRowData(m_iSelectedFullMaskEntry, TRUE);
				}

				SU_FullMask_UpdateCountDisplay();
			}
		}
	}
	return SU_BTN_EXIT;
}



// sends full mask message to all 3 anntenna and then listens for a response in the P2 protocol
// returns TRUE if the operation was successful
// When minutes is =0, it is successful if the HPT is unmasked 
// When minutes is !=0 , it is successful if the HPT is masked
BOOL SU_SendAndWaitForMU_P2_FullMask(U32 id_u32, char antenna, char minutes)
{
	unsigned long long lasttick;
	unsigned long long elapsedms=0;
	int nInitialCount; 
	MuData* p = GetMUListEntryByID(id_u32);
	if (p == NULL)
		nInitialCount = 0;
	else
		nInitialCount = p->packet_count;

	g_bTransmitting = TRUE;	// SET MODE TO TRANSMITTING TO ENSURE RSSI PACKETS IGNORED
	if (g_SuSettings.enable_audio_blanking) AUDIO_MUTE_LATCH = 1;	//normally low to enable the audio board and is high to mute the audio output.
	DelayMsecs(AUDIO_MUTE_DELAY_MS);				// delay to allow audio to be muted

	if (antenna == TX_ANTENNA_ALL) 
	{
		vlf_SendFullMaskMsg(id_u32, minutes, TX_ANTENNA_X);
		WaitForSUTXResponse(MASK_RX_DELAY_MS_P2);			// SU_TX can't receive next command while it is transmitting		
		vlf_SendFullMaskMsg(id_u32, minutes, TX_ANTENNA_Y);				
		WaitForSUTXResponse(MASK_RX_DELAY_MS_P2);			// SU_TX can't receive next command while it is transmitting		
		vlf_SendFullMaskMsg(id_u32, minutes, TX_ANTENNA_Z);
	}
	else
	{
		vlf_SendFullMaskMsg(id_u32, minutes, antenna);
	}
	set_SU_VLF_RX_Mode(SU_RX_MODE_DECODE);
	WaitForSUTXResponse(MASK_RX_DELAY_MS_P2);			// SU_TX can't receive next command while it is transmitting		
	AUDIO_MUTE_LATCH = 0;				// normally low to enable the audio board and is high to mute the audio output.
	g_bTransmitting = FALSE;			// RE-ENABLE RSSI PACKET UPDATES

	lasttick = g_tick;
	
#ifdef ENABLE_TEST_FAKEHPT_MODE
	SLCD_Test_MaskReponse(id_u32,minutes);		// send a fake response for this ID
#endif
		
	do 
	{
		ProcessSURXStream();
		if (g_tick != lasttick) // update ate 1ms intervals
		{
			if (g_tick > lasttick)
				elapsedms += g_tick-lasttick;
			else
				elapsedms += 1;	// account for timer overflow
						
			lasttick = g_tick;
		}
			MuData* p = GetMUListEntryByID(id_u32);
	
		if (p == NULL)
			p = GetMUListEntryByID(id_u32);
			
		if (p!= NULL)
		{
			if (nInitialCount != p->packet_count)
				break;
		}
	}
	while (elapsedms < P2_WAIT_FOR_FULL_MASK_RESPONSE_MS);

	if (p == NULL)
		p = GetMUListEntryByID(id_u32);
	
	if ((p!=NULL) && (nInitialCount != p->packet_count))
	{
		if ((minutes == 0) && (p->m.status.masked == 0))
			return TRUE;
		else if ((minutes != 0) && (p->m.status.masked == 1))
			return TRUE;		
	}

	return FALSE;
}

// attempts to apply the fullmask (either clearing or setting a mask) 3 times and returns TRUE if successful
BOOL SU_SendAndWaitForMU_P2_FullMask_3(U32 id_u32, char antenna, char minutes)
{
	if(SU_SendAndWaitForMU_P2_FullMask(id_u32, antenna, minutes))
		return TRUE;
	else
	{
		if(SU_SendAndWaitForMU_P2_FullMask(id_u32, antenna, minutes))
			return TRUE;
		else
			return SU_SendAndWaitForMU_P2_FullMask(id_u32, antenna, minutes);
	}
}


// walks through the table of full-masked units to either update it or add to the list
void SU_AddHPTtoFullMaskList(U32 id_u32)
{
	int i;
	BOOL bFound = FALSE;

	int listsize = GetMUFullMaskListSize();
	
	if (id_u32 != 0) // update the data for the ID if it is valid
	{
		for (i=0;i<listsize;i++) 
		{
			if ((g_MuFullMaskDeviceList[i].id_u32 == id_u32))
			{
				traceS("Updating existing Fullmask Entry");
				g_MuFullMaskDeviceList[i].last_fullmask_time = g_seconds;
				bFound = TRUE;
				break;
			}
		}
	
		if ((!bFound) && (listsize < (MAXIMUM_MU_FULLMASK_DEVICES_IN_LIST-1))) 
		{
			traceS("Adding New FullMask ID");
			g_MuFullMaskDeviceList[listsize].id_u32 = id_u32;	// insert the entry	
			g_MuFullMaskDeviceList[listsize].last_fullmask_time = g_seconds;
		}
	}
}


void SU_MoveHPTTo_FullMaskList(U32 id_u32)
{
	int iLast = GetMUListSize() - 1;
	int index = GetMUEntryIndexByID(id_u32);
	int i;

	if (index >= 0)	// make sure index is valid
	{
		SU_AddHPTtoFullMaskList(id_u32);
		for (i=index;i<iLast;i++)		// shift them all up to fill this entry
		{
			memcpy(&g_MuDeviceList[i], &g_MuDeviceList[i+1], sizeof(MuData));
		}
		SU_DataResetUnit(&g_MuDeviceList[iLast]);
		
	}
}

void SU_RemoveHPTFrom_FullMaskList(U32 id_u32)
{
	int iLast = GetMUFullMaskListSize()-1;
	int index = GetFullMaskList_IndexByID(id_u32);
	int i;
	
	if (index >= 0)	// make sure index is valid
	{
		for (i = index;i<iLast;i++) 	// shift them all up to fill this entry
		{
			memcpy(&g_MuFullMaskDeviceList[i], &g_MuFullMaskDeviceList[i+1], sizeof(MuFullMaskData));
		}
		g_MuFullMaskDeviceList[iLast].id_u32 = 0;	// flag the entry as deleted	
		g_MuFullMaskDeviceList[iLast].last_fullmask_time = 0;
	}
}

int SU_FullMaskList_GetShortestCountdownSeconds()
{
	int i;
	U32 min_countdown = MU_FULL_MASK_SECONDS;
	int listsize = GetMUFullMaskListSize();
	int seconds;
	int32 delta;
	
	for (i=0;i<listsize;i++) 
	{
		delta = g_seconds - g_MuFullMaskDeviceList[i].last_fullmask_time;
		if (delta < MU_FULL_MASK_SECONDS)
			seconds = MU_FULL_MASK_SECONDS-delta;
		else
			seconds = 0;			
		if (seconds < min_countdown)
		{
			min_countdown = seconds;
		}
	}
	return min_countdown;
}

// sends full mask message to all 3 anntenna and then listens for a response in the P2 protocol
BOOL SU_SendAndWaitForMU_P2_FullMask_Table(U32 id_u32, char antenna, char minutes)
{
	unsigned long long lasttick;
	unsigned long long elapsedms=0;
	unsigned int nInitialCount = get_MU_ResponseCount();
	unsigned int nLastCount;

	g_bTransmitting = TRUE;	// SET MODE TO TRANSMITTING TO ENSURE RSSI PACKETS IGNORED
	if (g_SuSettings.enable_audio_blanking) AUDIO_MUTE_LATCH = 1;	//normally low to enable the audio board and is high to mute the audio output.
	DelayMsecs(AUDIO_MUTE_DELAY_MS);				// delay to allow audio to be muted

	if (antenna == TX_ANTENNA_ALL) 
	{
		vlf_SendFullMaskMsg(id_u32, minutes, TX_ANTENNA_X);
		WaitForSUTXResponse(MASK_RX_DELAY_MS_P2);			// SU_TX can't receive next command while it is transmitting		
		vlf_SendFullMaskMsg(id_u32, minutes, TX_ANTENNA_Y);				
		WaitForSUTXResponse(MASK_RX_DELAY_MS_P2);			// SU_TX can't receive next command while it is transmitting		
		vlf_SendFullMaskMsg(id_u32, minutes, TX_ANTENNA_Z);
	}
	else
	{
		vlf_SendFullMaskMsg(id_u32, minutes, antenna);
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
		nLastCount = get_MU_ResponseCount();
	}
	while ((nInitialCount == nLastCount) && (elapsedms < P2_WAIT_FOR_FULL_MASK_RESPONSE_MS));
	if(nInitialCount == nLastCount)
		return FALSE;
	else
		return TRUE;
}

BOOL SU_SendAndWaitForMU_P2_FullMask_3_Table(U32 id_u32, char antenna, char minutes)
{
	if(SU_SendAndWaitForMU_P2_FullMask_Table(id_u32, antenna, minutes))
		return TRUE;
	else
	{
		if(SU_SendAndWaitForMU_P2_FullMask_Table(id_u32, antenna, minutes))
			return TRUE;
		else
			return SU_SendAndWaitForMU_P2_FullMask_Table(id_u32, antenna, minutes);
	}
}

//FULL_MASK END
