#include "su_slcd.h"
#include <stdio.h>
#include <stdlib.h>
#include "string.h"

int g_SelectedMainItem;
#define LOGO_WIDTH 136
#define LOGO_HEIGHT 86

#define MAINMENU_XBORDER 65
#define MAINMENU_YBORDER 20
#define MAINMENUITEM_HEIGHT 32
#define MAINMENU_BUTTONWIDTH 112
#define MAINMENU_BUTTONHEIGHT 106

#define MAINMENUTITLE_XPOS (MAINMENU_XPOS_START + 2)
#define MAINMENU_XPOS_START MAINMENU_XBORDER
#define MAINMENU_XPOS_END (SLCD_MAX_X-MAINMENU_XBORDER)

#define MAIN_MENU_Y_POS (400-MAINMENU_BUTTONHEIGHT)

#define SU_MAIN_LOGO_XPOS ((SLCD_MAX_X - LOGO_WIDTH)/2)
#define SU_MAIN_LOGO_HEIGHT (LOGO_HEIGHT+4)
#define SU_MAIN_LOGO_YPOS 50

#define SU_MAIN_PRODUCT_NAME_XPOS (SLCD_MAX_X/2 - 50)
#define SU_MAIN_PRODUCT_NAME_YPOS (SU_MAIN_LOGO_HEIGHT + SU_MAIN_LOGO_YPOS + 20)

#define SU_MAIN_VERS_YPOS (SU_MAIN_PRODUCT_NAME_YPOS+70)


// Displays any one of the menu items in the main menu - entry is with the button index and whether it is selected or no
#ifdef RUSSIAN	
	#define QUICK_SEARCH_SELECTED_BMP 40
	#define QUICK_SEARCH_UNSELECTED_BMP 41
	#define ID_SEARCH_UNSELECTED_BMP 42
	#define ID_SEARCH_SELECTED_BMP 43
#else
	#define QUICK_SEARCH_SELECTED_BMP 36
	#define QUICK_SEARCH_UNSELECTED_BMP 37
	#define ID_SEARCH_UNSELECTED_BMP 38
	#define ID_SEARCH_SELECTED_BMP 39
#endif

void DisplayMainMenuItem(int buttonix, int selected)
{
	char buffer[50];
	int x1;
	int y1 = MAIN_MENU_Y_POS;

	switch (buttonix) 
	{
		case MAIN_MENU_AUDIO_SEARCH_BUTTON:
			x1 = SLCD_MAX_X/2 - (MAINMENU_BUTTONWIDTH );
			sprintf(buffer,"xi %d %d %d", selected?QUICK_SEARCH_SELECTED_BMP:QUICK_SEARCH_UNSELECTED_BMP, x1, MAIN_MENU_Y_POS);
			SLCD_SendCmd(buffer);
			sprintf(buffer,"xs %d %d %d %d %d",buttonix,x1, y1, x1 + MAINMENU_BUTTONWIDTH, y1+MAINMENU_BUTTONHEIGHT);	// Create a hidden hotspot for the menu item - x <n> x0 y0 x1 y1 
			SLCD_SendCmd(buffer);
			break;
		case MAIN_MENU_ID_SEARCH_BUTTON:
			x1 = SLCD_MAX_X/2 ;
			sprintf(buffer,"xi %d %d %d", selected?ID_SEARCH_SELECTED_BMP:ID_SEARCH_UNSELECTED_BMP, x1, MAIN_MENU_Y_POS);
			SLCD_SendCmd(buffer);
			sprintf(buffer,"xs %d %d %d %d %d",buttonix,x1, y1, x1 + MAINMENU_BUTTONWIDTH, y1+MAINMENU_BUTTONHEIGHT);	// Create a hidden hotspot for the menu item - x <n> x0 y0 x1 y1 
			SLCD_SendCmd(buffer);
			break;
		default:
			break;
	}


}


/*
// Displays any one of the menu items in the main menu - entry is with the button index and whether it is selected or no
void DisplayMainMenuItem(int buttonix, int selected)
{
	char buffer[50];
	int row = (buttonix - MAIN_MENU_AUDIO_SEARCH_BUTTON);
	int x1 = MAINMENU_XPOS_START;
	int y1 = MAIN_MENU_Y_POS + (row * (MAINMENUITEM_HEIGHT + MAINMENU_YBORDER));
	int x2 = MAINMENU_XPOS_END;
	int y2 = y1 + MAINMENUITEM_HEIGHT;
	char* title;

//	SLCD_SendCmd("f18BC");		// set font size
//	SLCD_SendCmd("f12x24");		// set font size
	SLCD_SendCmd("f24");		// set font size

	SetForeBackGndColor(selected?MENU_SELECTED_COLOR:MENU_NORMAL_COLOR,SLCD_FOREGROUND);

	sprintf(buffer,"r %d %d %d %d 1",x1, y1, x2, y2);			// draw a rectangle with the foreground color
	SLCD_SendCmd(buffer);

	switch (buttonix) {
		case MAIN_MENU_ID_SEARCH_BUTTON:
			title = "ID Search";
			
			sprintf(buffer,"xi %d %d %d", selected?36:37, x1, MAIN_MENU_Y_POS);
			SLCD_SendCmd(buffer);

			break;
		case MAIN_MENU_AUDIO_SEARCH_BUTTON:
			sprintf(buffer,"xi %d %d %d", selected?39:38, x1 + 100, MAIN_MENU_Y_POS);
			SLCD_SendCmd(buffer);
			
			title = "Quick Search";
			break;
		case MAIN_MENU_CALIBRATION_BUTTON:
			title = "Calibration";
			break;
		default:
			title = "Err";
			break;
	}

	SetForeBackGndColor(SLCD_BLACK, selected?MENU_SELECTED_COLOR:MENU_NORMAL_COLOR);
	sprintf(buffer,"t \" %s \" %d %d",title,MAINMENUTITLE_XPOS,y1 + 4);			// show the title
	SLCD_SendCmd(buffer);

	SetDefaultBackForegnd();		
	sprintf(buffer,"xs %d %d %d %d %d",buttonix,x1, y1, x2, y2);	// Create a hidden hotspot for the menu item - x <n> x0 y0 x1 y1 
	SLCD_SendCmd(buffer);

	SLCD_SendCmd("p 1");
	sprintf(buffer,"r %d %d %d %d",x1, y1, x2, y2);			// draw a rectangle around the button
	SLCD_SendCmd(buffer);
	sprintf(buffer,"l %d %d %d %d",x2+1,y1,x2+1,y2);		// add a shadow line to the right
	SLCD_SendCmd(buffer);
	sprintf(buffer,"l %d %d %d %d",x1,y2+1,x2,y2+1);		// add a shadow line below
	SLCD_SendCmd(buffer);
	

}
*/

//#define DEBUG_SHOW_FONT_LIST

// This function draws the entire main page with logo and buttons - it also initializes the LCD to known states
void SU_DrawMainPage()
{

	char buffer[50];
	char buf2[25];
    // Logo splash screen
	SetDefaultBackForegnd();	
    SLCD_SendCmd(CLEARSCREEN);	// clear screen
//	SLCD_SendCmd("*debounce 100");	// set the button debounce speed to default for buttons - Caution - this removed because it caused problems
	SLCD_SendCmd("ta LT");		// set test orientation to left, top
    SLCD_SendCmd("o 0 0");		// set origin to 0,0
    
   
// use the following to see the loaded font list - you have to use the uart to see the results   
#ifdef DEBUG_SHOW_FONT_LIST
    SLCD_SendCmd("f?");
    DelayMsecs(5000);
    SLCD_SendCmd(CLEARSCREEN);	// clear screen
	
#endif
    
	sprintf(buffer,"xi 1 %d %d", SU_MAIN_LOGO_XPOS, SU_MAIN_LOGO_YPOS);
	SLCD_SendCmd(buffer);
	
#ifdef INCLUDE_MINERADIO_HEADER
	DRAW_MINERADIO_HEADER;
	DRAW_MINERADIO_FOOTER;
#endif

	SetScreenBrightness(g_SuSettings.screen_brightness);	// sets background level of screen
	SetDefaultBackForegnd();

	SLCD_SendCmd("utf8 on");		// sets unicode
	SLCD_SendCmd("fUARIAL18");		// sets the unicode character set for the title

#ifdef RUSSIAN	
//	sprintf(buffer,"t \"%s\" %d %d T","?-d09f; ?-d09e; ?-d098; ?-d0a1; ?-d09a; ?-d09e; ?-d092; ?-d0ab; ?-d099; ?-d09c; ?-d09e; ?-d094; ?-d0a3; ?-d09b; ?-d0ac",SU_MAIN_PRODUCT_NAME_XPOS, SU_MAIN_PRODUCT_NAME_YPOS);
	sprintf(buffer,"t \"%s\" %d %d T","\xd0\x9f\xd0\x9e\xd0\x98\xd0\xa1\xd0\x9a\xd0\x9e\xd0\x92\xd0\xab\xd0\x99 \xd0\x9c\xd0\x9e\xd0\x94\xd0\xa3\xd0\x9b\xd0\xac",5, SU_MAIN_PRODUCT_NAME_YPOS);
	SLCD_SendCmd(buffer);
	sprintf(buffer,"t \"%s\" %d %d T","HELIAN",SU_MAIN_PRODUCT_NAME_XPOS+10, SU_MAIN_PRODUCT_NAME_YPOS+32);
	SLCD_SendCmd(buffer);
// ALWAYS OVERIDE PROTOCOL IN THIS #IFDEF VERSION	
	#ifdef PROTOCOL_1
		sprintf(buf2,"P1 V%s ID:%04ld",SW_VERSION_STR,g_Config.id);
	#elif PROTOCOL_2	
		//?-d0b2; ?-d0b5; ?-d180; ?-d181; ?-d0b8; ?-d18f
		sprintf(buf2,"\xd0\xb2\xd0\xb5\xd1\x80\xd1\x81\xd0\xb8\xd1\x8f %s No:%04ld",SW_VERSION_STR,g_Config.id);
	#else
		#error PROTOCOL NOT DEFINED					
	#endif	
	sprintf(buffer,"t \"%s\" %d %d T",buf2, (SLCD_MAX_X/2 - 70), SU_MAIN_VERS_YPOS);


#else
//	SLCD_SendCmd("f32");		// sets the character set for the title
	sprintf(buffer,"t \"%s\" %d %d T","HELIAN",SU_MAIN_PRODUCT_NAME_XPOS, SU_MAIN_PRODUCT_NAME_YPOS);
	SLCD_SendCmd(buffer);
	sprintf(buffer,"t \"%s\" %d %d T","SEARCH UNIT",SU_MAIN_PRODUCT_NAME_XPOS-40, SU_MAIN_PRODUCT_NAME_YPOS+32);
	SLCD_SendCmd(buffer);
	SET_DEFAULT_FOOTER_FONT;
// ALWAYS OVERIDE PROTOCOL IN THIS #IFDEF VERSION	
	#ifdef PROTOCOL_1
		sprintf(buf2,"P1 V%s ID:%04ld",SW_VERSION_STR,g_Config.id);
	#elif PROTOCOL_2	
		sprintf(buf2,"V%s ID:%04ld",SW_VERSION_STR,g_Config.id);
	#else
		#error PROTOCOL NOT DEFINED					
	#endif	

	sprintf(buffer,"t \"%s\" %d %d T",buf2,SLCD_MAX_X/2 - 50, SU_MAIN_VERS_YPOS);
#endif	
	SLCD_SendCmd("fUARIAL12");		// sets the unicode character set for the version text
	SLCD_SendCmd(buffer);


	SU_InitMainBatteryBar();	// creates and display battery bar
	SU_InitTXBatteryBar();

	DisplayMainMenuItem(MAIN_MENU_AUDIO_SEARCH_BUTTON,(g_SelectedMainItem == MAIN_MENU_AUDIO_SEARCH_BUTTON));
	DisplayMainMenuItem(MAIN_MENU_ID_SEARCH_BUTTON,(g_SelectedMainItem == MAIN_MENU_ID_SEARCH_BUTTON));
//	DisplayMainMenuItem(MAIN_MENU_CALIBRATION_BUTTON,(g_SelectedMainItem == MAIN_MENU_CALIBRATION_BUTTON));

	int adder = NAVIGATION_BTN_BMP_WIDTH + 4;
	int x = SLCD_MAX_X - (adder * 5);
	x += adder*2;
	SU_DisplayStandardNavigationButtons(x, NAVIGATION_BTN_YPOS, adder,FALSE);
	
	#ifdef RUSSIAN	
		//?-d0af; ?-d180; ?-d0ba;?-d0be; ?-d181; ?-d182; ?-d18c
		//?-d09a; ?-d0b0; ?-d0ba; ?-d0b8; ?-d0b1; ?-d180; ?-d0be; ?-d0b2; ?-d0ba; ?-d0b0
		SLCD_WriteFooter("","","F1=\xd0\xaf\xd1\x80\xd0\xba\xd0\xbe\xd1\x81\xd1\x82\xd1\x8c  F2=\xd0\x9a\xd0\xb0\xd0\xbb\xd0\xb8\xd0\xb1\xd1\x80\xd0\xbe\xd0\xb2\xd0\xba\xd0\xb0");
	#else
		SLCD_WriteFooter("","","F1=Brightness  F2= Calibration");
	#endif
}


void run_menu_item()
{	
	int retval;
	switch( g_SelectedMainItem) 
	{
		case MAIN_MENU_ID_SEARCH_BUTTON:
			SU_DataReset();		// CLEAR THE LIST OF EXISTING MU UNITS - 
		
			#ifdef PROTOCOL_1
				SU_RunSearchPage(FALSE);
			#elif PROTOCOL_2					
				SU_Run_P2_SearchPage(FALSE);
			#else
			#error PROTOCOL NOT DEFINED					
			#endif						
			break;
		case MAIN_MENU_AUDIO_SEARCH_BUTTON:
			SU_DataReset();		// CLEAR THE LIST OF EXISTING MU UNITS - 
			#ifdef PROTOCOL_1
				retval = SU_RunSearchPage(TRUE);
				if (retval == SU_BTN_F2)
				{
					SU_DataReset();		// CLEAR THE LIST OF EXISTING MU UNITS - 
					SU_RunSearchPage(FALSE);
				}			
			#elif PROTOCOL_2					
				retval = SU_Run_P2_SearchPage(TRUE);
				if (retval == SU_BTN_F2)
				{
					SU_DataReset();		// CLEAR THE LIST OF EXISTING MU UNITS - 
					SU_Run_P2_SearchPage(FALSE);
				}			
			#else
			#error PROTOCOL NOT DEFINED					
			#endif			
			break;	
		default:
			break;
	}
	SU_DrawMainPage();
	SU_UpdateGeneralStatus(TRUE,TRUE);
}


void SU_RunMainPage()
{
	struct button btn;
	time_t ref_tick = g_seconds;
	int seconds = 0;
	int oldsetting;
	unsigned short taskStatus;
	g_SelectedMainItem = MAIN_MENU_AUDIO_SEARCH_BUTTON;
	
	SU_DrawMainPage();
	SU_UpdateGeneralStatus(TRUE,TRUE);
	SU_DataReset();			// CLEAR THE LIST OF EXISTING MU UNITS - 
	SU_FullMaskTableReset();	//FULL_MASK	- INITIALIZE THE FULL MASK LIST TO A KNOWN EMPTY STATE	
	StartPingStatus();

	while (1) {				// loop until we get an exit from the page
		if (ref_tick != g_seconds) {
			ref_tick = g_seconds;
			SU_UpdateGeneralStatus(TRUE,FALSE);
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
///by Alex!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
/////////////////////////////
			//if (g_SuSettings.enable_autocalibration){			//by Alex!!!
			//	SU_AutoCalibrate();
			//}
			
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
		if (btn.index != 0) {
			oldsetting = g_SelectedMainItem;
			switch (btn.index) {
				case SU_BTN_UP:
					if (g_SelectedMainItem == MAIN_MENU_AUDIO_SEARCH_BUTTON)
						g_SelectedMainItem = MAIN_MENU_ID_SEARCH_BUTTON;
					else
						g_SelectedMainItem = MAIN_MENU_AUDIO_SEARCH_BUTTON;
					break;
				case SU_BTN_DOWN:
					if (g_SelectedMainItem == MAIN_MENU_ID_SEARCH_BUTTON)
						g_SelectedMainItem = MAIN_MENU_AUDIO_SEARCH_BUTTON;
					else
						g_SelectedMainItem = MAIN_MENU_ID_SEARCH_BUTTON;
					break;
				case SU_BTN_ENTER:
					run_menu_item();
					break;
				case MAIN_MENU_ID_SEARCH_BUTTON:
				case MAIN_MENU_AUDIO_SEARCH_BUTTON:
					g_SelectedMainItem = btn.index;
					run_menu_item();
					break;
				case SU_BTN_UPDOWN:
					SU_RunSettingsPage();
					SU_DrawMainPage();
					SU_UpdateGeneralStatus(TRUE,TRUE);
					break;
				case SU_BTN_F1:
					SU_EditScreenBrightness();
					SU_DrawMainPage();
					SU_UpdateGeneralStatus(TRUE,TRUE);
					break;	
				case SU_BTN_F2:
					SU_RunTestPage();  
					g_SysError.value = 0;		// Clear the general system error flag
					SU_DrawMainPage();
					SU_UpdateGeneralStatus(TRUE,TRUE);					
					break;	
				default:
					break;
			}
					
			if (oldsetting != g_SelectedMainItem) {
				DisplayMainMenuItem(oldsetting,FALSE);
				DisplayMainMenuItem(g_SelectedMainItem,TRUE);
			}
		}
	}
}

