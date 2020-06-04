#include "su_slcd.h"
#include <stdio.h>
#include <stdlib.h>
#include "string.h"
#include "sdfunctions.h"

////////////////////////////////////////// function prototyes ////////////////////////////////////
LogFileData GetLogData(int id);
void SU_DrawFileViewPage(LogFileData data);
void SU_RunFileViewPage(int fileindex);
void SU_DrawFile(LogFileData fd, int baseline, int maxlines);

/////////////////////////////////////////// globals used by log view /////////////////////////////////
char filedisplay_buf[FILE_MAX_LINES_PER_PAGE][MAX_DISPLAY_LINE_WIDTH];



void SU_DrawFileViewPage(LogFileData data)
{
 	char buffer[50];
	int x = 0;
	int y = NAVIGATION_BTN_YPOS;
	int adder = (SLCD_MAX_X-NAVIGATION_BTN_BMP_WIDTH)/4;

	SetDefaultBackForegnd();		
    SLCD_SendCmd(CLEARSCREEN);	// clear screen

#ifdef INCLUDE_MINERADIO_HEADER
	DRAW_MINERADIO_HEADER;
//	DRAW_MINERADIO_FOOTER;
#endif

	SetDefaultBackForegnd();		

	SLCD_SendCmd("f 12x24");		// set font size
	sprintf(buffer,"t \"%s\" 2 %d",data.filename, FILE_TITLE_YPOS);
	SLCD_SendCmd(buffer);

	SU_InitMainBatteryBar();	// creates and display battery bar
	SU_DisplayStandardNavigationButtons(x+adder,y, adder,TRUE);

}



void SU_DrawFile(LogFileData fd, int startline, int maxlines)
{
 	char buffer[60];
	int i;
	
	SLCD_SendCmd("f16B");		// set font size
	
	int count = SD_getLogFileLines(fd, filedisplay_buf, startline, startline + FILE_MAX_LINES_PER_PAGE, PC_PORT);

	for (i=0;i<FILE_MAX_LINES_PER_PAGE;i++) {
		if (filedisplay_buf[i][0]!= 0) {
			sprintf(buffer,"t \"%s\" 2 %d",filedisplay_buf[i],FILE_BASE_YPOS + i*FILE_LINE_HEIGHT);
			SLCD_SendCmd(buffer);
		}
	}

	SLCD_SendCmd("f18BC");		// set font size
	if (maxlines == 0) {
		sprintf(buffer,"t \"File is empty\" 2 %d",FILE_BASE_YPOS + FILE_MAX_LINES_PER_PAGE*FILE_LINE_HEIGHT);
	}
	else {
		sprintf(buffer,"t \"Lines %d to %d of %d           \" 2 %d",
			startline+1,
			startline+count,
			maxlines,
			FILE_BASE_YPOS + FILE_MAX_LINES_PER_PAGE*FILE_LINE_HEIGHT
			);
	}
	SLCD_SendCmd(buffer);
}


void SU_RunFileViewPage(int fileindex)
{
	struct button btn;
	int oldsetting;
	time_t ref_tick = g_seconds;
	LogFileData fd = GetLogData(fileindex);
	int maxlines = SD_countfilelines(fd, PC_PORT);
	int pages = maxlines/FILE_MAX_LINES_PER_PAGE;
	if (maxlines%FILE_MAX_LINES_PER_PAGE) pages++;
	int currentpage = 0;

	SU_DrawFileViewPage(fd);
	SU_DrawFile(fd,currentpage*FILE_MAX_LINES_PER_PAGE,maxlines);
	SU_UpdateGeneralStatus(TRUE,TRUE);

	while (1) {				// loop until we get an exit from the page
		if (ref_tick != g_seconds) {
			ref_tick = g_seconds;
			SU_UpdateTime(SU_MAIN_TIME_XPOS,SU_MAIN_TIME_YPOS);
			SU_UpdateGeneralStatus(TRUE,FALSE);
		}
		unsigned short taskStatus = ProcessBackgroundTasks();
		btn = GetUserAction(taskStatus);
		if (btn.index != 0) {
			oldsetting = currentpage;
			switch (btn.index) {
				case SU_BTN_UP:
					if (currentpage > 0)
						currentpage--;
					break;
				case SU_BTN_DOWN:
					if (currentpage < (pages-1))
						currentpage++;
					break;
				case SU_BTN_EXIT:
					return;
					break;
				case SU_BTN_ENTER:
					// todo - 
					break;
				default:
					break;
			}
			if (oldsetting != currentpage) {
				SU_DrawFileViewPage(fd);
				SU_DrawFile(fd,currentpage*FILE_MAX_LINES_PER_PAGE,maxlines);
				SU_UpdateGeneralStatus(TRUE,TRUE);
			}
		}
	}
}


