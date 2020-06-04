#include "su_slcd.h"
#include <stdio.h>
#include <stdlib.h>
#include "string.h"
#include "sdfunctions.h"
#include "trace.h"

////////////////////////////////////////// function prototyes ////////////////////////////////////
void SU_MainLogPage();
void SU_RunLogDeletePage();
void SU_DisplayLogMenuItem(int index, int selected);
void SU_RunLogViewPage();
void SU_DrawMainLogPage(int index);
void SU_DisplayLogViewPage();
void SU_DrawLogViewGridTitles();
void SU_DrawLogViewPageRowGrid(int row);
void SU_DrawLogViewPageGrid();
int GetLogListSize();
void SU_RedrawLogViewDisplay(int index, BOOL bRedraw);
void SU_DrawLogViewRow(int index, BOOL bSelected);
void SU_UpdateLogViewCountDisplay(int index);
BOOL SU_isLogViewRowVisible(int row);
LogFileData GetLogData(int id);
int GetLogListSize();

extern void SU_DrawFileViewPage(LogFileData data);
extern void SU_RunFileViewPage(int fileindex);
extern void SU_DrawFile(LogFileData fd, int baseline, int maxlines);

/////////////////////////////////////////// globals used by log view /////////////////////////////////
int g_iLogListDisplayBase = 0;

////////////////////////////////////////// local defines ///////////////////////////////////
#define LOGVIEW_PAGE_COUNTSTATS_XPOS 5
#define LOGVIEW_PAGE_COUNTSTATS_YPOS (NAVIGATION_BTN_YPOS - 19)
#define LOGVIEW_INDENT_FROM_GRID 4
#define LOGVIEW_GRID_TITLE_YPOS 50
#define LOGVIEW_GRID_TITLE_HEIGHT 20
#define LOGVIEW_GRID_START_YPOS (LOGVIEW_GRID_TITLE_YPOS + LOGVIEW_GRID_TITLE_HEIGHT)
#define LOGVIEW_GRID_ROW_HEIGHT 26
#define LOGVIEW_GRID_END_YPOS (LOGVIEW_PAGE_COUNTSTATS_YPOS - 4)
#define LOGVIEW_GRID_MAX_ROWS ((int)((LOGVIEW_GRID_END_YPOS - LOGVIEW_GRID_START_YPOS)/LOGVIEW_GRID_ROW_HEIGHT))
#define LOGVIEW_GRID_COL0_XPOS 0
#define LOGVIEW_GRID_COL1_XPOS 80
#define LOGVIEW_GRID_COL2_XPOS 210

#define MAX_FILE_ENTRIES (LOGVIEW_GRID_MAX_ROWS*5)
static long g_logentry_count = 0;
LogFileData g_fileEntries[MAX_FILE_ENTRIES];

int GetLogListSize(BOOL reload)
{
	if (reload) {
		g_logentry_count = SD_GetLastNFilenames(g_fileEntries, MAX_FILE_ENTRIES, PC_PORT);
		if (g_logentry_count > MAX_FILE_ENTRIES)	// make sure there aren't more entries than  bufer space
			g_logentry_count = MAX_FILE_ENTRIES;
	}
	
	return g_logentry_count;
}

LogFileData GetLogData(int id)
{
	return g_fileEntries[id%MAX_FILE_ENTRIES];
}

BOOL SU_isLogViewRowVisible(int row)
{
		// if row is out of range then
	if ((row < g_iLogListDisplayBase) || (row >= (g_iLogListDisplayBase + LOGVIEW_GRID_MAX_ROWS))) {
		return FALSE;
	}
	else
		return TRUE;
}

void SU_DrawLogViewRow(int index, BOOL bSelected)
{
	char buffer[50];
	int y;
	int x = 0;
	char strbuffer[30];
	
	if ((index < 0) || (index >= GetLogListSize(FALSE)))	// make sure index is valid
		return;

	LogFileData data = GetLogData(index);

	// if row is out of range then simply return
	if (!SU_isLogViewRowVisible(index)) return;

//	SLCD_SendCmd("f24B");	// set font size
//	SLCD_SendCmd("f14x24");	// set font size
//	SLCD_SendCmd("f12x24");	// set font size
//	SLCD_SendCmd("f16");	// set font size
	SLCD_SendCmd("f16B");	// set font size
//	SLCD_SendCmd("f8x15B");	// set font size
//	SLCD_SendCmd("f18BC");	// set font size
	y = LOGVIEW_GRID_START_YPOS + (LOGVIEW_GRID_ROW_HEIGHT * (index - g_iLogListDisplayBase));
	if (y > (LOGVIEW_GRID_END_YPOS - LOGVIEW_GRID_ROW_HEIGHT))
		return;

	y+=1;
	// draw the background selection color

	SetForeBackGndColor(bSelected?MENU_SELECTED_COLOR:SLCD_BACKGROUND, SLCD_FOREGROUND);

	sprintf(buffer,"r %d %d %d %d 1",0,y, LOGVIEW_GRID_COL1_XPOS -1, y + LOGVIEW_GRID_ROW_HEIGHT -2);			// draw a rectangle with the foreground color
	SLCD_SendCmd(buffer);
	sprintf(buffer,"r %d %d %d %d 1",LOGVIEW_GRID_COL1_XPOS + 1,y, LOGVIEW_GRID_COL2_XPOS -1, y + LOGVIEW_GRID_ROW_HEIGHT -2);			// draw a rectangle with the foreground color
	SLCD_SendCmd(buffer);
	sprintf(buffer,"r %d %d %d %d 1",LOGVIEW_GRID_COL2_XPOS + 1,y, SLCD_MAX_X -1, y + LOGVIEW_GRID_ROW_HEIGHT -2);			// draw a rectangle with the foreground color
	SLCD_SendCmd(buffer);


	// draw the text forground
	if (bSelected)
		SetForeBackGndColor(SLCD_BLACK,SLCD_YELLOW);
	else
		SetDefaultBackForegnd();

	x = LOGVIEW_GRID_COL0_XPOS + LOGVIEW_INDENT_FROM_GRID;

	strcpy(strbuffer,data.filename);
	strbuffer[8] = 0;	// remove the file extension
	sprintf(buffer,"t \"%s\" %d %d",strbuffer,x,y);
	SLCD_SendCmd(buffer);

	x = LOGVIEW_GRID_COL1_XPOS + LOGVIEW_INDENT_FROM_GRID;
	strftime(strbuffer, sizeof(strbuffer), "%b %d %y %H:%M:%S ", gmtime((const time_t * )&data.ftime));
	sprintf(buffer,"t \"%s\" %d %d",strbuffer,x,y);
	SLCD_SendCmd(buffer);

	x = LOGVIEW_GRID_COL2_XPOS + LOGVIEW_INDENT_FROM_GRID;
	sprintf(buffer,"t \"%ld\" %d %d",data.size/1024,x,y);
	SLCD_SendCmd(buffer);
	
	SetDefaultBackForegnd();

}

// Update the display of the number of items on the screen
void SU_UpdateLogViewCountDisplay(int index)
{
	char buffer[50];
	int listsize = GetLogListSize(FALSE);
	// Draw the Row lines	
#ifdef INCLUDE_MINERADIO_HEADER
		SET_TO_MINERADIO_BACKGROUND;
#else
		SetDefaultBackForegnd();
#endif
	SLCD_SendCmd("f18BC");
	if (index >= 0)
		sprintf(buffer,"t \"%2d of %2d   \" %d %d",index+1,listsize,LOGVIEW_PAGE_COUNTSTATS_XPOS,LOGVIEW_PAGE_COUNTSTATS_YPOS);
	else
		sprintf(buffer,"t \"%2d units   \" %d %d",listsize,LOGVIEW_PAGE_COUNTSTATS_XPOS,LOGVIEW_PAGE_COUNTSTATS_YPOS);
	SLCD_SendCmd(buffer);
}

void SU_RedrawLogViewDisplay(int index, BOOL bForceRedraw)
{
	char buffer[50];
	int row;
	int oldpos = g_iLogListDisplayBase;
	int y;

	sprintf(buffer,"index=%d, g_iLogListDisplayBase=%d",index,g_iLogListDisplayBase);
	traceS(buffer);
	if (index >= 0) {
		if(index < g_iLogListDisplayBase) {
			g_iLogListDisplayBase = index;
		}
		else { 		
			if (index > (LOGVIEW_GRID_MAX_ROWS-1)) {
				g_iLogListDisplayBase = index - (LOGVIEW_GRID_MAX_ROWS-1);
			}
			else {
				g_iLogListDisplayBase = 0;
			}
		}

		sprintf(buffer,"index=%d, g_iLogListDisplayBase=%d",index,g_iLogListDisplayBase);
		traceS(buffer);
		
		// scroll rectangle up or down and then redraw the new row
		if ((bForceRedraw==FALSE) && (abs(g_iLogListDisplayBase-oldpos) == 1)) { // if it only moved by 1 row then scroll the screen
			if (g_iLogListDisplayBase > oldpos) {
	//			y = GRID_START_YPOS+GRID_ROW_HEIGHT;
				y = LOGVIEW_GRID_START_YPOS;
				sprintf(buffer,"k 0 %d %d %d %du",y,SLCD_MAX_X,y + (LOGVIEW_GRID_MAX_ROWS*LOGVIEW_GRID_ROW_HEIGHT),LOGVIEW_GRID_ROW_HEIGHT);
				SLCD_SendCmd(buffer);
				y = g_iLogListDisplayBase + LOGVIEW_GRID_MAX_ROWS - 1;
				SU_DrawLogViewRow(y, (y == index)?TRUE:FALSE);
				SU_DrawLogViewPageRowGrid(index);
			}
			else {
				y = LOGVIEW_GRID_START_YPOS;
				sprintf(buffer,"k 0 %d %d %d %dd",y,SLCD_MAX_X,y + (LOGVIEW_GRID_MAX_ROWS*LOGVIEW_GRID_ROW_HEIGHT),LOGVIEW_GRID_ROW_HEIGHT);
				SLCD_SendCmd(buffer);
				y = g_iLogListDisplayBase;
				SU_DrawLogViewRow(y, (y == index)?TRUE:FALSE);
				SU_DrawLogViewPageRowGrid(index);
			}
		}
		else {	// Redraw the whole screen
			for (row=g_iLogListDisplayBase;row<g_iLogListDisplayBase+LOGVIEW_GRID_MAX_ROWS;row++) {
				sprintf(buffer,"displaying index %d", row);
				traceS(buffer);
				SU_DrawLogViewRow(row, (row == index)?TRUE:FALSE);
			}
		}
	}

	SU_UpdateLogViewCountDisplay(index);
}

void SU_DrawLogViewGridTitles()
{
	char buffer[50];
	int y = LOGVIEW_GRID_TITLE_YPOS;
	int x = 0;

	SLCD_SendCmd("f18BC");		// set font size
	SetForeBackGndColor(SLCD_LIGHT_GREY,SLCD_BLACK);// set foreground to grey, background to black
	sprintf(buffer,"r 0 %d %d %d 1",y,SLCD_MAX_X, y + LOGVIEW_GRID_TITLE_HEIGHT);	// draw a rectangle with the foreground color
	SLCD_SendCmd(buffer);

	SetForeBackGndColor(SLCD_BLACK, SLCD_LIGHT_GREY);			// set foreground to Black, background to light grey

	x = LOGVIEW_GRID_COL0_XPOS + LOGVIEW_INDENT_FROM_GRID;
	sprintf(buffer,"t \" filename \" %d %d",x,y);			// id
	SLCD_SendCmd(buffer);

	x = LOGVIEW_GRID_COL1_XPOS + LOGVIEW_INDENT_FROM_GRID;
	sprintf(buffer,"t \" date\" %d %d",x,y);			// acceleration
	SLCD_SendCmd(buffer);

	x = LOGVIEW_GRID_COL2_XPOS + LOGVIEW_INDENT_FROM_GRID;
	sprintf(buffer,"t \" size k\" %d %d",x,y);		// bearing
	SLCD_SendCmd(buffer);
}

// Draws the background grid for the units in the search window
void SU_DrawLogViewPageGrid()
{
	char buf[30];
	int y;

	SLCD_SendCmd("p 1");		// This sets the pen width to 1 pixel wide

	
	for (y = LOGVIEW_GRID_START_YPOS; y <= LOGVIEW_GRID_END_YPOS; y+= LOGVIEW_GRID_ROW_HEIGHT) 
	{
		sprintf(buf,"l 0 %d %d %d",y, SLCD_MAX_X, y);
		SLCD_SendCmd(buf);
	}
	
	int ymax = y - LOGVIEW_GRID_ROW_HEIGHT;

	// Draw the title area lines
	y = LOGVIEW_GRID_TITLE_YPOS;
	SetForeBackGndColor(SLCD_BLACK,SLCD_FOREGROUND);	// DRAW IN BLACK
	sprintf(buf,"l %d %d %d %d",LOGVIEW_GRID_COL1_XPOS,y,LOGVIEW_GRID_COL1_XPOS,y+LOGVIEW_GRID_TITLE_HEIGHT);
	SLCD_SendCmd(buf);

	sprintf(buf,"l %d %d %d %d",LOGVIEW_GRID_COL2_XPOS,y,LOGVIEW_GRID_COL2_XPOS,y+LOGVIEW_GRID_TITLE_HEIGHT);
	SLCD_SendCmd(buf);

	// Set the column lines for the remaining grid
	SetDefaultBackForegnd();		
	y = LOGVIEW_GRID_START_YPOS;
	// Draw the column lines
	sprintf(buf,"l %d %d %d %d",LOGVIEW_GRID_COL1_XPOS,y,LOGVIEW_GRID_COL1_XPOS,ymax);
	SLCD_SendCmd(buf);

	sprintf(buf,"l %d %d %d %d",LOGVIEW_GRID_COL2_XPOS,y,LOGVIEW_GRID_COL2_XPOS,ymax);
	SLCD_SendCmd(buf);

}

void SU_DrawLogViewPageRowGrid(int row)
{
	char buf[100];
	int y1,y2;


	SLCD_SendCmd("p 1");		// This sets the pen width to 1 pixel wide

	// Draw the Row lines	
	y1 = LOGVIEW_GRID_START_YPOS;
	y1 += (LOGVIEW_GRID_ROW_HEIGHT * (row - g_iLogListDisplayBase));
	y2 = y1 + LOGVIEW_GRID_ROW_HEIGHT;

	sprintf(buf,"l 0 %d %d %d",y1,SLCD_MAX_X,y1);
	SLCD_SendCmd(buf);

	sprintf(buf,"l 0 %d %d %d",y2,SLCD_MAX_X,y2);
	SLCD_SendCmd(buf);

	// Draw the column lines
	sprintf(buf,"l %d %d %d %d",LOGVIEW_GRID_COL1_XPOS,y1,LOGVIEW_GRID_COL1_XPOS,y2);
	SLCD_SendCmd(buf);

	sprintf(buf,"l %d %d %d %d",LOGVIEW_GRID_COL2_XPOS,y1,LOGVIEW_GRID_COL2_XPOS,y2);
	SLCD_SendCmd(buf);

}


void SU_DisplayLogViewPage()
{
	int x = 0;
	int y = NAVIGATION_BTN_YPOS;
	int adder = (SLCD_MAX_X-NAVIGATION_BTN_BMP_WIDTH)/4;


	SetDefaultBackForegnd();		
    SLCD_SendCmd(CLEARSCREEN);	// clear screen
	SLCD_SendCmd("ta LT");		// set test orientation to left, top
    SLCD_SendCmd("o 0 0");	// set origin to 0,0

#ifdef INCLUDE_MINERADIO_HEADER
	DRAW_MINERADIO_HEADER;
	DRAW_MINERADIO_FOOTER;
#endif

	SetDefaultBackForegnd();		
	SU_DrawLogViewGridTitles();

	SetDefaultBackForegnd();		
	SU_DrawLogViewPageGrid();

	SU_InitBatteryBar( SU_BATTERY_LEVEL_INDEX, SU_MAIN_BATTERY_XPOS,SU_MAIN_BATTERY_YPOS);

    SLCD_SendCmd("f 12x24");		// set font size		

	SU_DisplayStandardNavigationButtons(x+adder,y, adder,TRUE);

}



void SU_RunLogViewPage()
{
	struct button btn;
	time_t ref_tick = g_seconds;
	int oldsetting;
	int listsize;
	int index = -1;

	g_iLogListDisplayBase = -2;		// reset the display base to none on display
	SU_UpdateTime(SU_MAIN_TIME_XPOS,SU_MAIN_TIME_YPOS);
	SU_UpdateBatteryLevel(TRUE, TRUE);

	SU_DisplayLogViewPage();
	listsize = GetLogListSize(TRUE);
	index = listsize - 1;	// set the index to the newest file
	
	if (index >=0)
		SU_RedrawLogViewDisplay(index, TRUE);	// make sure newest file is showing

	while (1) {				// loop until we get an exit from the page
		if (ref_tick != g_seconds) {
			ref_tick = g_seconds;
			SU_UpdateTime(SU_MAIN_TIME_XPOS,SU_MAIN_TIME_YPOS);
			SU_UpdateBatteryLevel(FALSE, TRUE);
		}
		unsigned short taskStatus = ProcessBackgroundTasks();
		btn = GetUserAction(taskStatus);
		if (btn.index != 0) {
			oldsetting = index;
			listsize = GetLogListSize(FALSE);
			switch (btn.index) {
				case SU_BTN_ENTER:
					if (index >=0) {
						SU_RunFileViewPage(index);
						SU_DisplayLogViewPage();
						SU_RedrawLogViewDisplay(index,TRUE);	
					}
					break;
				case SU_BTN_UP:
					if (index >= 0) {
						if (index > 0)
							index--;
						else
							index = listsize -1;
					}
					break;
				case SU_BTN_DOWN:
					if (index >= 0) {
						if (index >= (listsize-1)) 
							index = 0;
						else
							index++;

					}
					break;
				case SU_BTN_EXIT:
					return;
					break;
				default:
					break;
			}
			if (oldsetting != index)
			{
				SU_DrawLogViewRow(oldsetting, FALSE);
				if (!SU_isLogViewRowVisible(index))
					SU_RedrawLogViewDisplay(index,FALSE);
				else
					SU_DrawLogViewRow(index, TRUE);
				SU_UpdateLogViewCountDisplay(index);
			}
		}
	}
}

#ifdef INCLUDE_EXTRA_LOG_FUNDCTIONS
////////////////////////////// THESE FUNCTIONS CURRENTLY NOT USED - BUT COULD BE IF DELETE WAS ENABLED /////////////
////////////////////////////// The code is left here for possible future activation //////////////////////////
//////////////////////////////  Keep in mind that it is untested and so should be treated as a starting point
void SU_RunLogDeletePage()
{
// TODO
}

void SU_DisplayLogMenuItem(int index, int selected)
{
	char buffer[50];
	char value[50];
	char* title;
	int row;
	
	if (index == LOG_DELETE_BUTTON)
		row = 6;
	else
		row = 7;
		
	int x1 = MENU_XPOS_START;
	int y1 = MAIN_MENU_Y_POS + (row * (MENUITEM_HEIGHT + MENU_BORDER));
	int x2 = MENU_XPOS_END;
	int y2 = y1 + MENUITEM_HEIGHT;

	SLCD_SendCmd("f24");		// set font size

	SetForeBackGndColor(selected?MENU_SELECTED_COLOR:MENU_NORMAL_COLOR, SLCD_BACKGROUND);

	sprintf(buffer,"r %d %d %d %d 1",x1, y1, x2, y2);			// draw a rectangle with the foreground color
	SLCD_SendCmd(buffer);

	SetForeBackGndColor(SLCD_FOREGROUND,selected?MENU_SELECTED_COLOR:MENU_NORMAL_COLOR);

	
	switch (index) {
		case LOG_DELETE_BUTTON:
			title = "Delete All Log Files";
			break;
		case LOG_VIEW_BUTTON:
			title = "View Log Files";
			break;
		default:
			title = "Err";
			value[0]=0;
			break;
	}

	sprintf(buffer,"t \" %s \" %d %d",title,MENUTITLE_XPOS,y1 + 4);			// TITLE
	SLCD_SendCmd(buffer);
	SetDefaultBackForegnd();		

	
	sprintf(buffer,"xs %d %d %d %d %d",index,x1, y1, x2, y2);		// Create a hotspot for the menu item - x <n> x0 y0 x1 y1 
	SLCD_SendCmd(buffer);

	SLCD_SendCmd("p 1");
	sprintf(buffer,"r %d %d %d %d",x1, y1, x2, y2);			// draw a rectangle around the button
	SLCD_SendCmd(buffer);
	sprintf(buffer,"l %d %d %d %d",x2+1,y1,x2+1,y2);
	SLCD_SendCmd(buffer);
	sprintf(buffer,"l %d %d %d %d",x1,y2+1,x2,y2+1);
	SLCD_SendCmd(buffer);

}

void SU_DrawMainLogPage(int index)
{
	int adder = NAVIGATION_BTN_BMP_WIDTH_WITH_BORDER;
	int x = SLCD_MAX_X - (adder * 5);

	SetDefaultBackForegnd();		
    SLCD_SendCmd(CLEARSCREEN);			// clear screen
	SLCD_SendCmd("f24");
	SLCD_SendCmd("t \"use Up and Down keys to\nand * choose the action\nPress X to exit\" 5 400 T");

	SU_DisplayLogMenuItem(LOG_DELETE_BUTTON, (index == LOG_DELETE_BUTTON));
	SU_DisplayLogMenuItem(LOG_VIEW_BUTTON, (index == LOG_VIEW_BUTTON));

	SU_InitMainBatteryBar();	// creates and display battery bar
	SU_InitTXBatteryBar();
	SU_DisplayStandardNavigationButtons(x+adder, NAVIGATION_BTN_YPOS, adder,TRUE);

}

void SU_MainLogPage()
{
	struct button btn;
	int index = LOG_VIEW_BUTTON;
	int oldsetting;
	time_t ref_tick = g_seconds;

	SU_DrawMainLogPage(index);
	SU_UpdateBatteryLevel(TRUE, TRUE);
	SU_UpdateTXBatteryLevel(SU_MAIN_BATTERY_XPOS,SU_MAIN_BATTERY_YPOS,TRUE, TRUE);

	while (1) {				// loop until we get an exit from the page
		if (ref_tick != g_seconds) {
			ref_tick = g_seconds;
			SU_UpdateTime(SU_MAIN_TIME_XPOS,SU_MAIN_TIME_YPOS);
			SU_UpdateBatteryLevel(FALSE, TRUE);
		}
		unsigned short taskStatus = ProcessBackgroundTasks();
		btn = GetUserAction(taskStatus);
		if (btn.index != 0) {
			oldsetting = index;
			switch (btn.index) {
				case SU_BTN_UP:
					index--;
					break;
				case SU_BTN_DOWN:
					index++;
					break;
				case SU_BTN_EXIT:
					return;
					break;
				case SU_BTN_ENTER:
					switch (index) {
						case LOG_DELETE_BUTTON:
							SU_RunLogDeletePage();
							break;
						case LOG_VIEW_BUTTON:
							SU_RunLogViewPage();
							break;
						default:
							break;
					}
					SU_DrawMainLogPage(index);
					break;
				case LOG_DELETE_BUTTON:
					index = LOG_DELETE_BUTTON;
					break;
				case LOG_VIEW_BUTTON:
					index = LOG_VIEW_BUTTON;
					break;
				default:
					break;
			}
			if (oldsetting != index) {
				if (index > LOG_VIEW_BUTTON) index = LOG_DELETE_BUTTON;
				if (index < LOG_DELETE_BUTTON) index = LOG_VIEW_BUTTON;
				SU_DisplayLogMenuItem((int)oldsetting, FALSE);
				SU_DisplayLogMenuItem((int)index, TRUE);
			}

		}
	}
}

#endif
