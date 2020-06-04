#include "su_slcd.h"
#include <stdio.h>
#include <stdlib.h>
#include "string.h"
#include "DEE Emulation 16-bit.h"
#include "vlf_rxtx.h"
#include "sdfunctions.h"
#if (BOARD_REV == 2)
#include "wi.h"
#include "i2c_isl12024_rtc.h"
#endif

// Note that these options have to be in the 128 to 255 number range to be used as hotspot identifiers
typedef enum eOPTIONMENU {
//	eMENUDATE = 150,		//!
//	eMENUTIME,				//!
//	eMENUPOWER_OFF_DELAY,	//!
	eMENULOCATE_DURATION = 150,	//!
//	eMENULOCATE_DURATION,	//!
	eMENUPOWER_OFF_DELAY,	//!
	eMENULCD_CALIBRATE,
	eMENU_AUDIO_BLANK,
	eMENULCD_BEEP_VOLUME,
	eMENU_PACKET_BEEP,
	eMENU_ZCTHRESHOLD,
	eMENU_K1,
	eMENU_LOCATE_D,
//	eMENU_VIEW_SDLOG,		//!
	eMENU_WIRELESS,
	eMENULCD_INTERFACE_MODE,
	eMENULCD_SCREEN_BRIGHTNESS,
	eMENU_AUTOCALIBRATION		//by Alex
}eOptionMenu;

#define MENU1_SIZE 7
int ai_Menu1[MENU1_SIZE] = {
//	eMENUDATE, 	//!
//	eMENUTIME, 	//!
	eMENULOCATE_DURATION, 
	eMENUPOWER_OFF_DELAY, 
	eMENU_AUDIO_BLANK, 
	eMENULCD_BEEP_VOLUME, 	
//	eMENU_PACKET_BEEP	//!
	eMENU_PACKET_BEEP,	//!
	eMENU_ZCTHRESHOLD, 	//!
	eMENU_K1			//!
	};


//#define MENU2_SIZE 6	//!
//#define MENU2_SIZE 3	//!
#define MENU2_SIZE 4	//!  //by Alex
int ai_Menu2[MENU2_SIZE] = {
//	eMENU_ZCTHRESHOLD, 	//!
//	eMENU_K1, 			//!
	eMENU_LOCATE_D, 
//	eMENU_VIEW_SDLOG, 	//!
	eMENULCD_CALIBRATE, 
	eMENU_WIRELESS, 
	eMENU_AUTOCALIBRATION		//by Alex
	};

//int g_iSelectedOption = (int)eMENUDATE;	//!
int g_iSelectedOption = (int)eMENULOCATE_DURATION;

void SU_DrawSettingsPage(int mode);
void SU_DisplaySettingsMenu(int mode);
void SU_DisplaySettingsMenuItem(int ix, int item,int selected, BOOL bValueOnly);
void SU_SettingDefaults();

void SU_EditOption(int setting);			// edit the selected item
void SU_EditScreenBrightness();
int SU_EditProperyWithSlider(int iProperty, int min, int max, int default_value, char *title, char *units, int type);
void SU_DisplayStandardNavigationButtons(int x, int y, int width, BOOL bWithExit);

int SU_EditUserOption(int setting, char* title1, char* title2, char* pWindowTitle);
void SU_DisplayOptionMenuItem(int buttonix, char* title, int selected, int bisNotDefaultSetting);

void SU_EditTime(BOOL bEditDate);
void SU_DisplayTimeMenuItem(int index, int selected, BOOL bValueOnly, BOOL bInEditMode, int edit_value);




void SU_DrawSettingsPage(int mode)
{
	int adder = NAVIGATION_BTN_BMP_WIDTH_WITH_BORDER;
	int x = SLCD_MAX_X - (adder * 5);

	SetDefaultBackForegnd();		
    SLCD_SendCmd(CLEARSCREEN);			// clear screen
    SLCD_SendCmd("o 0 0");		// set origin to 0,0

#ifdef INCLUDE_MINERADIO_HEADER
	DRAW_MINERADIO_HEADER;
	DRAW_MINERADIO_FOOTER;
#endif

	SetDefaultBackForegnd();		
//	SLCD_SendCmd("f32");
	SLCD_SendCmd("fUARIAL18");
	
#if RUSSIAN
	if (mode)
	{
		SLCD_SendCmd("t \"\xD0\xA0\xD0\xB0\xD1\x81\xD1\x88\xD0\xB8\xD1\x80\xD0\xB5\xD0\xBD\xD0\xBD\xD1\x8B\xD0\xB5 \xD0\xBD\xD0\xB0\xD1\x81\xD1\x82\xD1\x80\xD0\xBE\xD0\xB9\xD0\xBA\xD0\xB8\" 0 50 N");
		SLCD_WriteFooter("","F1=\xD0\x9E\xD1\x81\xD0\xBD\xD0\xBE\xD0\xB2\xD0\xBD\xD1\x8B\xD0\xB5 \xD0\xBD\xD0\xB0\xD1\x81\xD1\x82\xD1\x80\xD0\xBE\xD0\xB9\xD0\xBA\xD0\xB8",
			"F2=\xD0\xA1\xD0\xBF\xD0\xB8\xD1\x81\xD0\xBE\xD0\xBA \xD0\xB8\xD1\x81\xD0\xBA\xD0\xBB\xD1\x8E\xD1\x87\xD1\x91\xD0\xBD\xD0\xBD\xD1\x8B\xD1\x85 \xD0\xB8\xD0\xB7 \xD0\xBF\xD0\xBE\xD0\xB8\xD1\x81\xD0\xBA\xD0\xB0");
	}
	else
	{
		SLCD_SendCmd("t \"\xD0\x9E\xD1\x81\xD0\xBD\xD0\xBE\xD0\xB2\xD0\xBD\xD1\x8B\xD0\xB5 \xD0\xBD\xD0\xB0\xD1\x81\xD1\x82\xD1\x80\xD0\xBE\xD0\xB9\xD0\xBA\xD0\xB8\" 15 50 N");
		SLCD_WriteFooter("","F1=\xD0\xA0\xD0\xB0\xD1\x81\xD1\x88\xD0\xB8\xD1\x80\xD0\xB5\xD0\xBD\xD0\xBD\xD1\x8B\xD0\xB5 \xD0\xBD\xD0\xB0\xD1\x81\xD1\x82\xD1\x80\xD0\xBE\xD0\xB9\xD0\xBA\xD0\xB8",
			"F2=\xD0\xA1\xD0\xBF\xD0\xB8\xD1\x81\xD0\xBE\xD0\xBA \xD0\xB8\xD1\x81\xD0\xBA\xD0\xBB\xD1\x8E\xD1\x87\xD1\x91\xD0\xBD\xD0\xBD\xD1\x8B\xD1\x85 \xD0\xB8\xD0\xB7 \xD0\xBF\xD0\xBE\xD0\xB8\xD1\x81\xD0\xBA\xD0\xB0");
	}
#else
	if (mode)
	{
		SLCD_SendCmd("t \"Advanced Settings\" 15 50 N");
		SLCD_WriteFooter("","F1=Standard Settings ","F2=Full Mask Table");
	}
	else
	{
		SLCD_SendCmd("t \"Settings\" 80 50 N");
		SLCD_WriteFooter("","F1=Advanced Settings ","F2=Full Mask Table");
	}
#endif	

	SetDefaultBackForegnd();		
	SU_DisplaySettingsMenu(mode);

	SLCD_SendCmd("ta LT");		// set test orientation to left, top
//   SLCD_SendCmd("f 12x24");	// set font size		
	SLCD_SendCmd("fUARIAL18");

	SU_InitMainBatteryBar();	// creates and display battery bar

	x += adder;
	SU_DisplayStandardNavigationButtons(x, NAVIGATION_BTN_YPOS, adder, TRUE);
}

#define SETTINGS_MENU_YPOS (50 + 32 + 12)

void SU_DisplaySettingsMenuItem(int ix, int option_item, int selected, BOOL bValueOnly)
{
//	int row = option_item - eMENUDATE;	// convert option id to row	//!
//	int row = option_item - eMENULOCATE_DURATION;	// convert option id to row	//!

	char buffer[50];
	int x1 = MENU_XPOS_START;
	int y1 = SETTINGS_MENU_YPOS + ((ix) * (MENUITEM_HEIGHT + MENU_HEIGHT_BORDER));
	int x2 = MENU_XPOS_END;
	int y2 = y1 + MENUITEM_HEIGHT;
	char* title;
	char value[60];

//	SLCD_SendCmd("f18BC");		// set font size
//	SLCD_SendCmd("f12x24");		// set font size
//	SLCD_SendCmd("f24");		// set font size
	SLCD_SendCmd("fUARIAL14");	// sets the unicode character set for the title

	if (bValueOnly==FALSE) {
		sprintf(buffer,"s %d 0",(selected)?MENU_SELECTED_COLOR:MENU_NORMAL_COLOR);		// set foreground to Black, background to light grey
		SLCD_SendCmd(buffer);
		sprintf(buffer,"r %d %d %d %d 1",x1, y1, x2, y2);			// draw a rectangle with the foreground color
		SLCD_SendCmd(buffer);
	}

	SetForeBackGndColor(SLCD_BLACK,selected?MENU_SELECTED_COLOR:MENU_NORMAL_COLOR);

	switch ((eOptionMenu)option_item) 
	{
/*		case eMENUDATE:
			title = "Date";
			strftime(value, sizeof(value), "%b %d %Y", gmtime((const time_t * )&g_seconds));
			break;*/	//!
/*		case eMENUTIME:
			title = "Time";
			strftime(value, sizeof(value), "%H:%M:%S", gmtime((const time_t * )&g_seconds));
			break;*/	//!
		case eMENUPOWER_OFF_DELAY:
			#ifdef RUSSIAN
				title = "\xD0\x90\xD0\xB2\xD1\x82\xD0\xBE \xD0\xBE\xD1\x82\xD0\xBA\xD0\xBB.";
				sprintf(value,"%d \xD1\x81\xD0\xB5\xD0\xBA.",g_SuSettings.screen_power_off_seconds);
			#else
				title = "Power Off";
				sprintf(value,"%d secs",g_SuSettings.screen_power_off_seconds);
			#endif
			break;
		case eMENU_LOCATE_D:
			#ifdef RUSSIAN
				title = "\xD0\x91\xD0\xB0\xD0\xB7\xD0\xB0 \xD0\xBE\xD1\x86\xD0\xB5\xD0\xBD\xD0\xBA\xD0\xB8";
				sprintf(value,"%d \xD0\xBC",g_SuSettings.locate_ref_meters);
			#else
				title = "Locate Dist";
				sprintf(value,"%d m",g_SuSettings.locate_ref_meters);
			#endif
			break;
		case eMENULOCATE_DURATION:
			#ifdef RUSSIAN
				title = "\xD0\x98\xD0\xB7\xD0\xBC\xD0\xB5\xD1\x80\xD0\xB5\xD0\xBD\xD0\xB8\xD1\x8F";
				sprintf(value,"%d \xD1\x81\xD0\xB5\xD0\xBA.",g_SuSettings.locate_duration_seconds);
			#else
				title = "Locate Sec";
				sprintf(value,"%d secs",g_SuSettings.locate_duration_seconds);
			#endif
			break;
		case eMENU_WIRELESS:
			#ifdef RUSSIAN
				title = "\xD0\xA1\xD0\xB2\xD1\x8F\xD0\xB7\xD1\x8C";
				sprintf(value,"%s",g_SuSettings.enable_wireless?"\xD0\xB1\xD0\xB5\xD1\x81\xD0\xBF\xD1\x80\xD0\xBE\xD0\xB2\xD0\xBE\xD0\xB4\xD0\xBD\xD0\xB0\xD1\x8F":"\xD0\xBF\xD0\xBE \xD0\xBA\xD0\xB0\xD0\xB1\xD0\xB5\xD0\xBB\xD1\x8E" );
			#else
				title = "TX Comm";
				sprintf(value,"%s",g_SuSettings.enable_wireless?"Wireless":"Cable" );
			#endif
			break;	
/*		case eMENU_VIEW_SDLOG:
			title = "View Log";
			SD_GetStatusMessage(value);
			break;*/	//!
		case eMENULCD_CALIBRATE:
			#ifdef RUSSIAN
				title = "\xD0\xAD\xD0\xBA\xD1\x80\xD0\xB0\xD0\xBD";
				strcpy(value,"\xD0\xBA\xD0\xB0\xD0\xBB\xD0\xB8\xD0\xB1\xD1\x80\xD0\xBE\xD0\xB2\xD0\xB0\xD1\x82\xD1\x8C");
			#else
				title = "LCD";
				strcpy(value,"Calibrate");
			#endif
			break;
//		case eMENULCD_INTERFACE_MODE:
//			title = "User Mode";
//			strcpy(value,(g_SuSettings.advanced_interface == 1)?"Advanced":"Normal");
//			break;
		case eMENU_ZCTHRESHOLD:
			#ifdef RUSSIAN
				title = "\xD0\x9F\xD0\xBE\xD1\x80\xD0\xBE\xD0\xB3";
				sprintf(value,"%04d \xD0\xBC\xD0\x92",ZC_THRESHOLD_COUNTS_TO_MV(g_SuSettings.ZC_trigger_level));
			#else
				title = "Threshold";
				sprintf(value,"%04d mv",ZC_THRESHOLD_COUNTS_TO_MV(g_SuSettings.ZC_trigger_level));
			#endif
			break;
		case eMENU_K1:
			#ifdef RUSSIAN
				title = "K1";
				sprintf(value,"%u",g_SuSettings.K1);
			#else
				title = "K1";
				sprintf(value,"%u",g_SuSettings.K1);
			#endif
			break;
		case eMENULCD_BEEP_VOLUME:
			#ifdef RUSSIAN
				title = "\xD0\x97\xD1\x83\xD0\xBC\xD0\xBC\xD0\xB5\xD1\x80";
				sprintf(value,"%u",g_SuSettings.BeepVolume);
			#else
				title = "Beep Volume";
				sprintf(value,"%u",g_SuSettings.BeepVolume);
			#endif
			break;
		case eMENU_AUDIO_BLANK:
			#ifdef RUSSIAN
				title = "\xD0\x9F\xD0\xB5\xD1\x80\xD0\xB5\xD0\xB4\xD0\xB0\xD1\x82\xD1\x87\xD0\xB8\xD0\xBA";
				sprintf(value,"%s",g_SuSettings.enable_audio_blanking?"\xD0\xB7\xD0\xB2\xD1\x83\xD0\xBA \xD0\xBE\xD1\x82\xD0\xBA\xD0\xBB.":"\xD0\xB7\xD0\xB2\xD1\x83\xD0\xBA \xD0\xB2\xD0\xBA\xD0\xBB." );
			#else
				title = "Audio Blank";
				sprintf(value,"%s",g_SuSettings.enable_audio_blanking?"Enabled":"Disabled" );
			#endif
			break;
		case eMENU_PACKET_BEEP:
			#ifdef RUSSIAN
				title = "\xD0\x9F\xD1\x80\xD0\xB8\xD1\x91\xD0\xBC";
				sprintf(value,"%s",g_SuSettings.enable_beep_on_VLF_packet?"\xD0\xB7\xD0\xB2\xD1\x83\xD0\xBA \xD0\xB2\xD0\xBA\xD0\xBB.":"\xD0\xB7\xD0\xB2\xD1\x83\xD0\xBA \xD0\xBE\xD1\x82\xD0\xBA\xD0\xBB.");
			#else
				title = "Packet Beep";
				sprintf(value,"%s",g_SuSettings.enable_beep_on_VLF_packet?"Enabled":"Disabled");
			#endif
			break;
////////////////////////////////////////////
//////////by Alex
///////////////////////////////////////////

		case eMENU_AUTOCALIBRATION:										//by Alex
			#ifdef RUSSIAN
				title = "\xD0\x90\xD0\xB2\xD1\x82\xD0\xBE\xD0\xBA\xD0\xB0\xD0\xBB\xD0\xB8\xD0\xB1\xD1\x80.";//Автокалибр.
				sprintf(value,"%s",g_SuSettings.enable_autocalibration?"\xD0\x92\xD0\xBA\xD0\xBB.":"\xD0\x92\xD1\x8B\xD0\xBA\xD0\xBB.");	//Вкл./Выкл.
			#else
				title = "Autocalibration.";
				sprintf(value,"%s",g_SuSettings.enable_autocalibration?"auto":"manual");
			#endif
			break;			

		default:
			title = "Err";
			value[0]=0;
			break;
	}


	if (bValueOnly==FALSE) {
		sprintf(buffer,"t \" %s \" %d %d",title,MENUTITLE_XPOS,y1 + 2);			// TITLE
		SLCD_SendCmd(buffer);
	}

	sprintf(buffer,"t \" %s \" %d %d",value,MENUVALUE_XPOS,y1 + 2);			// VALUE
	SLCD_SendCmd(buffer);

	SetDefaultBackForegnd();		

	if (bValueOnly==FALSE) {
		sprintf(buffer,"xs %d %d %d %d %d",option_item,x1, y1, x2, y2);		// Create a hotspot for the menu item - x <n> x0 y0 x1 y1 
		SLCD_SendCmd(buffer);

		SLCD_SendCmd("p 1");
		sprintf(buffer,"r %d %d %d %d",x1, y1, x2, y2);			// draw a rectangle around the button
		SLCD_SendCmd(buffer);
		sprintf(buffer,"l %d %d %d %d",x2+1,y1,x2+1,y2);
		SLCD_SendCmd(buffer);
		sprintf(buffer,"l %d %d %d %d",x1,y2+1,x2,y2+1);
		SLCD_SendCmd(buffer);

		// Draw a column line
		SetForeBackGndColor(SLCD_BLACK,MENU_NORMAL_COLOR);
		sprintf(buffer,"l %d %d %d %d",MENUVALUE_XPOS-2,y1,MENUVALUE_XPOS-2,y2);
		SLCD_SendCmd(buffer);
	}

}

void SU_DisplaySettingsMenu(int mode)
{
	int i;
	SetDefaultBackForegnd();		
	int* pia = mode?ai_Menu2:ai_Menu1;
	int maxix = mode?MENU2_SIZE:MENU1_SIZE;
	
	for (i=0;i<maxix;i++)
		SU_DisplaySettingsMenuItem(i, pia[i],(pia[i]==g_iSelectedOption)?1:0, FALSE);
		
}


void SU_RunSettingsPage()
{
	struct button btn;
	int oldsetting;
	time_t ref_tick = g_seconds;
	int mode = 0;
	int* pia = mode?ai_Menu2:ai_Menu1;
	int maxix = mode?MENU2_SIZE:MENU1_SIZE;
	int mix = 0;
	
	SU_DrawSettingsPage(mode);
	SU_UpdateBatteryLevel(TRUE, TRUE);

	while (1) {				// loop until we get an exit from the page
		if (ref_tick != g_seconds) {
			ref_tick = g_seconds;
			if (mode == 0)
			{
//				SU_DisplaySettingsMenuItem(0, eMENUDATE,(eMENUDATE==g_iSelectedOption)?1:0,TRUE);	//!
//				SU_DisplaySettingsMenuItem(1, eMENUTIME,(eMENUTIME==g_iSelectedOption)?1:0,TRUE);	//!
				SU_DisplaySettingsMenuItem(0, eMENULOCATE_DURATION,(eMENULOCATE_DURATION==g_iSelectedOption)?1:0,TRUE);	//!
				SU_DisplaySettingsMenuItem(1, eMENUPOWER_OFF_DELAY,(eMENUPOWER_OFF_DELAY==g_iSelectedOption)?1:0,TRUE);	//!
			}
			SU_UpdateBatteryLevel(FALSE, TRUE);
			SU_UpdateTime(SU_MAIN_TIME_XPOS,SU_MAIN_TIME_YPOS);
		}
		unsigned short taskStatus = ProcessBackgroundTasks();
		btn = GetUserAction(taskStatus);
		if (btn.index != 0) {
			oldsetting = mix;
			switch (btn.index) {
				case SU_BTN_UP:
					if (mix > 0)
						mix--;
					else
						mix = maxix-1;
					break;
				case SU_BTN_DOWN:
					if (mix < (maxix-1))
						mix++;
					else
						mix = 0;
					break;
				case SU_BTN_F1:
					mode = mode?0:1;
					pia = mode?ai_Menu2:ai_Menu1;
					maxix = mode?MENU2_SIZE:MENU1_SIZE;
					mix = 0;
					oldsetting = mix;
					g_iSelectedOption = pia[mix];
					SU_DrawSettingsPage(mode);
					SU_UpdateBatteryLevel(TRUE, TRUE);
					break;	
				case SU_BTN_F2:
					SU_Run_FullMaskPage();
					SU_DrawSettingsPage(mode);
					SU_UpdateBatteryLevel(TRUE, TRUE);
					break;	
				case SU_BTN_EXIT:
					return;
					break;
				case SU_BTN_ENTER:
					SU_EditOption(g_iSelectedOption);			// edit the selected item
					//maxix = (int)(g_SuSettings.advanced_interface?eMENUMAX:eMENULCD_INTERFACE_MODE+1);
					SU_DrawSettingsPage(mode);
					SU_UpdateBatteryLevel(TRUE, TRUE);
					break;
//				case eMENUDATE:		//!
//				case eMENUTIME:		//!
				case eMENUPOWER_OFF_DELAY:
				case eMENULOCATE_DURATION:
//				case eMENU_VIEW_SDLOG:	//!
				case eMENULCD_CALIBRATE:
//				case eMENULCD_INTERFACE_MODE:
				case eMENU_ZCTHRESHOLD:
				case eMENU_K1:
				case eMENULCD_BEEP_VOLUME:
				case eMENU_AUDIO_BLANK:
				case eMENU_PACKET_BEEP:
				case eMENU_WIRELESS:
				case eMENU_LOCATE_D:
				case eMENU_AUTOCALIBRATION:
//				if (g_iSelectedOption == btn.index)				// if button already selected
					{
						int i;
						g_iSelectedOption = btn.index;
						for (i=0;i<maxix;i++)		// map button to index
						{
							if (pia[i] == g_iSelectedOption)
							{
								mix = i;
								break;
							}
						}
						SU_EditOption(g_iSelectedOption);			// edit the selected item
						SU_DrawSettingsPage(mode);
						SU_UpdateBatteryLevel(TRUE, TRUE);
					}
//					else
//						g_iSelectedOption = btn.index;
					break;
				default:
					break;
			}
			if (oldsetting != mix) 
			{
				SU_DisplaySettingsMenuItem(oldsetting, pia[oldsetting],FALSE,FALSE);
				g_iSelectedOption = pia[mix];
				SU_DisplaySettingsMenuItem(mix, g_iSelectedOption,TRUE,FALSE);
			}
		}
	}
}

void SU_EditScreenBrightness()
{
	char buffer[20];
	int oldsetting = g_SuSettings.screen_brightness;
	g_SuSettings.screen_brightness = SU_EditProperyWithSlider(g_SuSettings.screen_brightness, 
		LCD_BRIGHTNESS_MIN, 
		LCD_BRIGHTNESS_MAX, 
		200,
#ifdef RUSSIAN
		"\xD0\xAF\xD1\x80\xD0\xBA\xD0\xBE\xD1\x81\xD1\x82\xD1\x8C \xD1\x8D\xD0\xBA\xD1\x80\xD0\xB0\xD0\xBD\xD0\xB0",
		"\xD0\xB5\xD0\xB4\xD0\xB8\xD0\xBD\xD0\xB8\xD1\x86",
#else
		"Screen Brightness",
		"counts",
#endif		
		eMENULCD_SCREEN_BRIGHTNESS
		);
	if (oldsetting != g_SuSettings.screen_brightness)
	{
		sprintf(buffer,"xbbs %d",g_SuSettings.screen_brightness);	// temporarily sets background level to the max
		SLCD_SendCmd(buffer);			
		save_current_settings();		// save to non-volatile memory
	}	
}

#define VERTICAL_SLIDER_IMAGE 29
#define VERTICAL_SLIDER_KNOB 30
int SU_EditProperyWithSlider(int iProperty, int minimum, int maximum, int default_value, char *title, char* units, int type)
{
 	char buffer[128];
	struct button btn;
	int level = iProperty - minimum;
	int adder = NAVIGATION_BTN_BMP_WIDTH_WITH_BORDER;
	int x = SLCD_MAX_X - (adder * 5);
	int x_slider = 90;
	int y_slider = 120;
	char changed;
	BOOL bRedraw;
	int newsetting = iProperty;

	SetDefaultBackForegnd();		
    SLCD_SendCmd(CLEARSCREEN);			// clear screen
	SU_DisplayStandardNavigationButtons(x+adder, NAVIGATION_BTN_YPOS, adder,TRUE);

	// todo - read debounce speed
	SLCD_SendCmd("*debounce 10");		// fast response for sliders

	// display right slider and text
	
	sprintf(buffer,"sl %d %d %d %d %d 5 0 1 %u %u 0",
		SLIDER_CONTROL,
		VERTICAL_SLIDER_IMAGE,
		x_slider,
		y_slider + 30, 
		VERTICAL_SLIDER_KNOB,
		0,
		maximum-minimum);
	SLCD_SendCmd(buffer);

#ifdef INCLUDE_MINERADIO_HEADER
	DRAW_MINERADIO_HEADER;
#endif
   

#ifdef RUSSIAN
	DRAW_MINERADIO_FOOTER4;
	SLCD_WriteFooter4("\xD0\x94\xD0\xBB\xD1\x8F \xD0\xB8\xD0\xB7\xD0\xBC\xD0\xB5\xD0\xBD\xD0\xB5\xD0\xBD\xD0\xB8\xD1\x8F \xD0\xB2\xD0\xBE\xD1\x81\xD0\xBF\xD0\xBE\xD0\xBB\xD1\x8C\xD0\xB7\xD1\x83\xD0\xB9\xD1\x82\xD0\xB5\xD1\x81\xD1\x8C",
		"\xD0\xBA\xD0\xBD\xD0\xBE\xD0\xBF\xD0\xBA\xD0\xB0\xD0\xBC\xD0\xB8 \xD1\x81\xD1\x82\xD1\x80\xD0\xB5\xD0\xBB\xD0\xBE\xD0\xBA \xD0\xB8\xD0\xBB\xD0\xB8 \xD1\x80\xD0\xB5\xD0\xB3\xD1\x83\xD0\xBB\xD1\x8F\xD1\x82\xD0\xBE\xD1\x80\xD0\xBE\xD0\xBC", 
		"\xD0\xBD\xD0\xB0 \xD1\x81\xD0\xB5\xD0\xBD\xD1\x81\xD0\xBE\xD1\x80\xD0\xBD\xD0\xBE\xD0\xBC \xD1\x8D\xD0\xBA\xD1\x80\xD0\xB0\xD0\xBD\xD0\xB5",
		"*=\xD0\xA1\xD0\xBE\xD1\x85\xD1\x80\xD0\xB0\xD0\xBD\xD0\xB8\xD1\x82\xD1\x8C \xD0\xB8\xD0\xB7\xD0\xBC\xD0\xB5\xD0\xBD\xD0\xB5\xD0\xBD\xD0\xB8\xD1\x8F"
		);
	#define SLIDER_VALUE_X_POS 50
#else
	DRAW_MINERADIO_FOOTER;
	SLCD_WriteFooter("Use slider or arrow keys",
		"to change setting",
		"Press * to save"
		);
	#define SLIDER_VALUE_X_POS 90
#endif
	

	sprintf(buffer,"sv %d %d",SLIDER_CONTROL,level);			// set to new level
	SLCD_SendCmd(buffer);			

	SetDefaultBackForegnd();		
	
	SLCD_SendCmd("fUARIAL18");
	sprintf(buffer,"t \"%s\" 10 50",title);			// set title
	SLCD_SendCmd(buffer);
	
	SLCD_SendCmd("fUARIAL14");		// set font size
	sprintf(buffer,"t \"%04d %s\" %d %d",level+minimum,units,SLIDER_VALUE_X_POS,y_slider);					// set level value
	SLCD_SendCmd(buffer);

#ifdef RUSSIAN
	sprintf(buffer,"t \"\\xD0\xBF\xD0\xBE \xD1\x83\xD0\xBC\xD0\xBE\xD0\xBB\xD1\x87\xD0\xB0\xD0\xBD\xD0\xB8\xD1\x8E=%04d\" %d %d",default_value,SLIDER_VALUE_X_POS,y_slider+174+24+10);					// set level value
#else
	sprintf(buffer,"t \"default=%04d\" %d %d",default_value,SLIDER_VALUE_X_POS,y_slider+174+24+10);					// set level value
#endif	
	SLCD_SendCmd(buffer);
	

	// implement setting
	while (1) {				// loop until we get an exit from the page
		unsigned short taskStatus = ProcessBackgroundTasks();
		btn = GetUserAction(taskStatus);
		if (btn.index != 0) 
		{
			bRedraw = FALSE;
			switch (btn.index) 
		{
				case SU_BTN_UP:
					level += 1;
					bRedraw = TRUE;
					break;
				case SU_BTN_DOWN:
					level -= 1;
					bRedraw = TRUE;
					break;
				case SU_BTN_EXIT:
					return newsetting;
					break;
				case SU_BTN_ENTER:
					newsetting = level + minimum;
					bRedraw = TRUE;
					break;
				case SLIDER_CONTROL:
					level = btn.state;
					bRedraw = TRUE;
					break;
				default:
					break;
			}
			if (bRedraw == TRUE) 
			{
				if (level > (maximum-minimum)) level = (maximum-minimum);
				if (level < 0) level = 0;
				sprintf(buffer,"sv %d %d",SLIDER_CONTROL,level);								// set to new level
				SLCD_SendCmd(buffer);
							
				changed = (newsetting == (level + minimum))?' ':'*';
				sprintf(buffer,"t \"%04d %s%c \" %d %d",level+minimum,units,changed,SLIDER_VALUE_X_POS,y_slider);	// display level value
				SLCD_SendCmd(buffer);
				
				if (type == eMENULCD_BEEP_VOLUME)
				{
					SetBeepVolume(level);
					if (btn.index == SLIDER_CONTROL)
						SLCD_SendCmd("beep 100");
				}
				else if (type == eMENULCD_SCREEN_BRIGHTNESS)
					SetScreenBrightness(level);
						
					
			}
		}
	}
	// restore debounce speed to original
	SLCD_SendCmd("*debounce 100");		// return debounce speed to default for buttons

	return newsetting;
}


void SU_DisplayOptionMenuItem(int buttonix, char* title, int selected, int bisNotDefaultSetting)
{
	char buffer[60];
	int row;
	char marker = bisNotDefaultSetting?'*':' ';
	
	if (buttonix == SETTINGS_BUTTON1)
		row = 4;
	else
		row = 5;
		
	int x1 = MENU_XPOS_START;
	int y1 = SETTINGS_MENU_YPOS + (row * (MENUITEM_HEIGHT + MENU_HEIGHT_BORDER));
	int x2 = MENU_XPOS_END;
	int y2 = y1 + MENUITEM_HEIGHT;

//	SLCD_SendCmd("f18BC");		// set font size
//	SLCD_SendCmd("f24");		// set font size
	SLCD_SendCmd("fUARIAL14");	// sets the unicode character set for the title

	SetForeBackGndColor((selected)?MENU_SELECTED_COLOR:MENU_NORMAL_COLOR, SLCD_FOREGROUND);

	sprintf(buffer,"r %d %d %d %d 1",x1, y1, x2, y2);			// draw a rectangle with the foreground color
	SLCD_SendCmd(buffer);

	SetForeBackGndColor(SLCD_BLACK,selected?MENU_SELECTED_COLOR:MENU_NORMAL_COLOR);

	sprintf(buffer,"t \" %s%c \" %d %d",title, marker, MENUTITLE_XPOS,y1 + 4);			// TITLE
	SLCD_SendCmd(buffer);

	SetDefaultBackForegnd();		

	sprintf(buffer,"xs %d %d %d %d %d",buttonix,x1, y1, x2, y2);		// Create a hotspot for the menu item - x <n> x0 y0 x1 y1 
	SLCD_SendCmd(buffer);

	SLCD_SendCmd("p 1");
	sprintf(buffer,"r %d %d %d %d",x1, y1, x2, y2);			// draw a rectangle around the button
	SLCD_SendCmd(buffer);
	sprintf(buffer,"l %d %d %d %d",x2+1,y1,x2+1,y2);
	SLCD_SendCmd(buffer);
	sprintf(buffer,"l %d %d %d %d",x1,y2+1,x2,y2+1);
	SLCD_SendCmd(buffer);

}



char* szMonthNames[12] = {"Jan","Feb","Mar","Apr","May", "Jun", "Jul","Aug", "Sep", "Oct", "Nov", "Dec"};

void SU_DisplayTimeMenuItem(int index, int selected, BOOL bValueOnly, BOOL bInEditMode, int edit_value)
{
	char buffer[50];
	int row = (index - EDIT_HOUR_BUTTON)%3;
	int x1 = MENU_XPOS_START;
	int y1 = SETTINGS_MENU_YPOS + ((row+2) * (MENUITEM_HEIGHT + MENU_HEIGHT_BORDER));
	int x2 = MENU_XPOS_END;
	int y2 = y1 + MENUITEM_HEIGHT;
	char* title;
	char value[50];
	int color = MENU_NORMAL_COLOR;

//	SLCD_SendCmd("f18BC");		// set font size
	SLCD_SendCmd("f24");		// set font size

	if (selected) 
		color = bInEditMode?MENU_EDIT_COLOR:MENU_SELECTED_COLOR;

	if (bValueOnly == FALSE) {
		sprintf(buffer,"s %d 0",color);		// set foreground to Black, background to light grey
		SLCD_SendCmd(buffer);
		sprintf(buffer,"r %d %d %d %d 1",x1, y1, x2, y2);			// draw a rectangle with the foreground color
		SLCD_SendCmd(buffer);
	}
	sprintf(buffer,"s 0 %d",color);		// set foreground to Black, background to light grey
	SLCD_SendCmd(buffer);

	switch (index) {
		case EDIT_HOUR_BUTTON:
			title = "Hour";
			if (bInEditMode)
				sprintf(value,"%02d",edit_value);
			else {
				strftime(value, sizeof(value), "%H", gmtime((const time_t * )&g_seconds));
			}
			break;
		case EDIT_MINUTE_BUTTON:
			title = "Minute";
			if (bInEditMode)
				sprintf(value,"%02d",edit_value);
			else {
				strftime(value, sizeof(value), "%M", gmtime((const time_t * )&g_seconds));
			}
			break;
		case EDIT_SECOND_BUTTON:
			title = "Second";
			if (bInEditMode)
				sprintf(value,"%02d",edit_value);
			else {
				strftime(value, sizeof(value), "%S", gmtime((const time_t * )&g_seconds));
			}
			break;
		case EDIT_YEAR_BUTTON:
			title = "Year";
			if (bInEditMode)
				sprintf(value,"%04d",edit_value);
			else {
				strftime(value, sizeof(value), "%Y", gmtime((const time_t * )&g_seconds));
			}
			break;
		case EDIT_MONTH_BUTTON:
			title = "Month";
			if (bInEditMode)
				sprintf(value,"%s",szMonthNames[(edit_value-1)%12]);
			else {
				strftime(value, sizeof(value), "%b", gmtime((const time_t * )&g_seconds));
			}
			break;
		case EDIT_DAY_BUTTON:
			title = "Day";
			if (bInEditMode)
				sprintf(value,"%02d",edit_value);
			else {
				strftime(value, sizeof(value), "%d", gmtime((const time_t * )&g_seconds));
			}
			break;
		default:
			title = "Err";
			value[0]=0;
			break;
	}


	if (bValueOnly == FALSE) {
		sprintf(buffer,"t \" %s \" %d %d",title,MENUTITLE_XPOS,y1 + 4);			// TITLE
		SLCD_SendCmd(buffer);
	}

	sprintf(buffer,"t \" %s \" %d %d",value,MENUVALUE_XPOS,y1 + 4);			// VALUE
	SLCD_SendCmd(buffer);

	SetDefaultBackForegnd();		
	
	if (bValueOnly == FALSE) {
		sprintf(buffer,"xs %d %d %d %d %d",index,x1, y1, x2, y2);		// Create a hotspot for the menu item - x <n> x0 y0 x1 y1 
		SLCD_SendCmd(buffer);

		SLCD_SendCmd("p 1");
		sprintf(buffer,"r %d %d %d %d",x1, y1, x2, y2);			// draw a rectangle around the button
		SLCD_SendCmd(buffer);
		sprintf(buffer,"l %d %d %d %d",x2+1,y1,x2+1,y2);
		SLCD_SendCmd(buffer);
		sprintf(buffer,"l %d %d %d %d",x1,y2+1,x2,y2+1);
		SLCD_SendCmd(buffer);

		// Draw a column line
		sprintf(buffer,"l %d %d %d %d",MENUVALUE_XPOS-2,y1,MENUVALUE_XPOS-2,y2);
		SLCD_SendCmd(buffer);
	}
}

// this function takes care of editing the hour, minute and second for the time values
void SU_EditTime(BOOL bEditDate)
{
	struct button btn;
	int adder = NAVIGATION_BTN_BMP_WIDTH_WITH_BORDER;
	int x = SLCD_MAX_X - (adder * 5);
	int oldsetting;
	BOOL bInEditMode = FALSE;
	int edit_value=0;
	int edit_min;
	int edit_max;
	int index_min = bEditDate?EDIT_DAY_BUTTON:EDIT_HOUR_BUTTON;
	int index_max = bEditDate?EDIT_YEAR_BUTTON:EDIT_SECOND_BUTTON;
	int index = index_min;
	int i;
	time_t ref_tick = g_seconds;
	struct tm* pt = gmtime((const time_t * )&g_seconds);

	SetDefaultBackForegnd();		
    SLCD_SendCmd(CLEARSCREEN);			// clear screen

#ifdef INCLUDE_MINERADIO_HEADER
	DRAW_MINERADIO_HEADER;
	DRAW_MINERADIO_FOOTER;
#endif
   
	SLCD_WriteFooter("Use arrow keys to move to field","Press * to edit with arrow keys","Press * to save");
	
	SetDefaultBackForegnd();		

	SLCD_SendCmd("f32");
	if (bEditDate)
		SLCD_SendCmd("t \"Set Date\" 50 80");			// set title
	else
		SLCD_SendCmd("t \"Set Time\" 50 80");			// set title
	

	for (i=index_min; i<=index_max;i++)
		SU_DisplayTimeMenuItem(i, (index == i),FALSE, bInEditMode, edit_value);

	SU_DisplayStandardNavigationButtons(x+adder, NAVIGATION_BTN_YPOS, adder,TRUE);

	while (1) {				// loop until we get an exit from the page
		if (ref_tick != g_seconds) {
			ref_tick = g_seconds;
			pt = gmtime((const time_t * )&g_seconds);
			if (bInEditMode == FALSE) {
				for (i=index_min; i<=index_max;i++)
					SU_DisplayTimeMenuItem(i, (index == i),TRUE, bInEditMode, edit_value);
			}
		}
		unsigned short taskStatus = ProcessBackgroundTasks();
		btn = GetUserAction(taskStatus);
		if (btn.index != 0) {
			oldsetting = index;
			switch (btn.index) {
				case SU_BTN_UP:
					if (bInEditMode){
						if (edit_value <  edit_max)
							edit_value ++;
						else 
							edit_value = edit_min;
						SU_DisplayTimeMenuItem((int)index, TRUE,TRUE,bInEditMode,edit_value);
					}
					else
						index--;
					break;
				case SU_BTN_DOWN:
					if (bInEditMode){
						if (edit_value >  edit_min)
							edit_value --;
						else 
							edit_value = edit_max;
						SU_DisplayTimeMenuItem((int)index, TRUE,TRUE,bInEditMode,edit_value);
					}
					else
						index++;
					break;
				case SU_BTN_EXIT:
					if (bInEditMode){
						bInEditMode = FALSE;	// JUST EXIT EDIT MODE - NO SAVING OF CHANGE
						SU_DisplayTimeMenuItem((int)index, TRUE,FALSE,bInEditMode,edit_value);
					}
					else {
						return;
					}
					break;
				case SU_BTN_ENTER:
					if (bInEditMode){
#if (RTCC_SOURCE == RTCC_INTERNAL)	
						switch (index) {
							case EDIT_HOUR_BUTTON:
								RTCCSetBinHour(edit_value);
								break;
							case EDIT_MINUTE_BUTTON:
								RTCCSetBinMin(edit_value);
								break;
							case EDIT_SECOND_BUTTON:
								RTCCSetBinSec(edit_value);
								break;
							case EDIT_YEAR_BUTTON:
								RTCCSetBinYear(edit_value-2000);
								break;
							case EDIT_MONTH_BUTTON:
								RTCCSetBinMonth(edit_value);
								break;
							case EDIT_DAY_BUTTON:
								RTCCSetBinDay(edit_value);
								break;
							default:
								break;
						}
						mRTCCSet();	
						g_seconds = RTCCGetTime_t();	// update the time
#else
						switch (index) {
							case EDIT_HOUR_BUTTON:
								pt->tm_hour = edit_value;
								break;
							case EDIT_MINUTE_BUTTON:
								pt->tm_min = edit_value;
								break;
							case EDIT_SECOND_BUTTON:
								pt->tm_sec = edit_value;
								break;
							case EDIT_YEAR_BUTTON:
								pt->tm_year = (edit_value-1900);
								break;
							case EDIT_MONTH_BUTTON:
								pt->tm_mon = edit_value-1;
								break;
							case EDIT_DAY_BUTTON:
								pt->tm_mday = edit_value;
								break;
							default:
								break;
						}
						ISL_RTC_WriteTMTime(pt);	// write the new time
						ISL_ReadTime(&g_seconds);	// read it back	
#endif
						bInEditMode = FALSE;
					}
					else {
						switch (index) {
							case EDIT_HOUR_BUTTON:
								edit_value = pt->tm_hour;
								edit_min = 0;
								edit_max = 23;
								break;
							case EDIT_MINUTE_BUTTON:
								edit_value = pt->tm_min;
								edit_min = 0;
								edit_max = 59;
								break;
							case EDIT_SECOND_BUTTON:
								edit_value = pt->tm_sec;
								edit_min = 0;
								edit_max = 59;
								break;
							case EDIT_YEAR_BUTTON:
								edit_min = 2000;
								edit_max = 2100;
								edit_value = pt->tm_year+1900;
								if (edit_value < edit_min)
									edit_value = edit_min;
								if (edit_value > edit_max) edit_value = edit_max;
								break;
							case EDIT_MONTH_BUTTON:
								edit_value = pt->tm_mon + 1;
								edit_min = 1;
								edit_max = 12;
								break;
							case EDIT_DAY_BUTTON:
								edit_value = pt->tm_mday;
								edit_min = 1;
								edit_max = 31;
								break;
							default:
								break;
						}
						bInEditMode = TRUE;
					}
					SU_DisplayTimeMenuItem((int)index, TRUE,FALSE,bInEditMode,edit_value);
					break;
				case EDIT_HOUR_BUTTON:
				case EDIT_MINUTE_BUTTON:
				case EDIT_SECOND_BUTTON:
				case EDIT_YEAR_BUTTON:
				case EDIT_MONTH_BUTTON:
				case EDIT_DAY_BUTTON:
					if (bInEditMode){
						bInEditMode = FALSE;	// TODO: DO WE WANT TO SAVE THE CHANGED TIME HERE?
					}
					index = btn.index;
					break;
				default:
					break;
			}
			if (oldsetting != index) {
				if (index < index_min) index = index_max;
				if (index > index_max) index = index_min;
				SU_DisplayTimeMenuItem((int)oldsetting, FALSE,FALSE,bInEditMode,edit_value);
				SU_DisplayTimeMenuItem((int)index, TRUE,FALSE,bInEditMode,edit_value);
			}
		}
	}
}

void edit_locate_distance()
{
	int oldsetting = g_SuSettings.locate_ref_meters;
	g_SuSettings.locate_ref_meters = SU_EditProperyWithSlider(g_SuSettings.locate_ref_meters, 
		1, 
		5, 
		DEFAULT_LOCATE_REF_METERS,
#ifdef RUSSIAN
		"\xD0\x91\xD0\xB0\xD0\xB7\xD0\xB0 \xD0\xBE\xD1\x86\xD0\xB5\xD0\xBD\xD0\xBA\xD0\xB8 \xD0\xBD\xD0\xB0\xD0\xBF\xD1\x80\xD0\xB0\xD0\xB2.",
		"\xD0\xBC\xD0\xB5\xD1\x82\xD1\x80\xD0\xB0",
#else
		"Locate Ref meters",
		"meters",
#endif
		eMENULOCATE_DURATION
		);
	if (oldsetting != g_SuSettings.locate_ref_meters)
		save_current_settings();		// save to non-volatile memory
}

void edit_locate_duration()
{
	int oldsetting = g_SuSettings.locate_duration_seconds;
	g_SuSettings.locate_duration_seconds = SU_EditProperyWithSlider(g_SuSettings.locate_duration_seconds, 
		LOCATE_DURATION_MIN, 
		LOCATE_DURATION_MAX, 
		LOCATE_DURATION_DEFAULT,
#ifdef RUSSIAN
		"\xD0\x92\xD1\x80\xD0\xB5\xD0\xBC\xD1\x8F \xD0\xB7\xD0\xB0\xD0\xBC\xD0\xB5\xD1\x80\xD0\xB0 \xD1\x80\xD0\xB0\xD1\x81\xD1\x81\xD1\x82.",
		"\xD1\x81\xD0\xB5\xD0\xBA\xD1\x83\xD0\xBD\xD0\xB4",
#else
		"Locate TX time",
		"seconds",
#endif		
		eMENULOCATE_DURATION
		);
	if (oldsetting != g_SuSettings.locate_duration_seconds)
		save_current_settings();		// save to non-volatile memory
}

void edit_power_off_delay()
{
	int	oldsetting = g_SuSettings.screen_power_off_seconds;
	g_SuSettings.screen_power_off_seconds = SU_EditProperyWithSlider(g_SuSettings.screen_power_off_seconds, 
		LCD_POWER_OFF_MIN, 
		LCD_POWER_OFF_MAX, 
		LCD_POWER_OFF_DEFAULT,
#ifdef RUSSIAN
		"\xD0\x90\xD0\xB2\xD1\x82\xD0\xBE \xD0\xBE\xD1\x82\xD0\xBA\xD0\xBB\xD1\x8E\xD1\x87\xD0\xB5\xD0\xBD\xD0\xB8\xD0\xB5",
		"\xD1\x81\xD0\xB5\xD0\xBA\xD1\x83\xD0\xBD\xD0\xB4",
#else
		"LCD power-off delay",
		"seconds",
#endif		
		eMENUPOWER_OFF_DELAY
		);
	if (oldsetting != g_SuSettings.screen_power_off_seconds)
		save_current_settings();		// save to non-volatile memory
}

void edit_zc_threshold()
{
	int oldsetting = g_SuSettings.ZC_trigger_level;
	int itemp = ZC_THRESHOLD_COUNTS_TO_MV(g_SuSettings.ZC_trigger_level);
	itemp = SU_EditProperyWithSlider(itemp, 
		ZC_THRESHOLD_COUNTS_TO_MV(ZC_THRESHOLD_MIN),
		ZC_THRESHOLD_COUNTS_TO_MV(ZC_THRESHOLD_MAX),
		ZC_THRESHOLD_COUNTS_TO_MV(ZC_THRESHOLD_DEFAULT), 
		#ifdef RUSSIAN
			"\xD0\x9F\xD0\xBE\xD1\x80\xD0\xBE\xD0\xB3 \xD0\xBF\xD1\x80\xD0\xB8\xD1\x91\xD0\xBC\xD0\xB0",
			"\xD0\xBC\xD0\x92",
		#else
			"ZC threshold",
			"mv",
		#endif	
		eMENU_ZCTHRESHOLD
		);
	itemp = ZC_THRESHOLD_MV_TO_COUNTS(itemp);
	if (oldsetting != itemp)
	{
		g_SuSettings.ZC_trigger_level = itemp;
		save_current_settings();		// save to non-volatile memory
		send_SU_RX_ZC_Trigger();		// SEND NEW THRESHOLD TO SU_RX
	}
}	

void edit_k_factor()
{
	int	oldsetting = g_SuSettings.K1;
	g_SuSettings.K1= SU_EditProperyWithSlider(g_SuSettings.K1, 
		K1_MIN, 
		K1_MAX, 
		K1_DEFAULT,
#ifdef RUSSIAN
		"\xD0\x9A\xD0\xBE\xD1\x8D\xD1\x84\xD1\x84\xD0\xB8\xD1\x86\xD0\xB8\xD0\xB5\xD0\xBD\xD1\x82 K1",
		"\xD0\xB5\xD0\xB4\xD0\xB8\xD0\xBD\xD0\xB8\xD1\x86",
#else
		"K1 distance factor",
		"counts",
#endif		
		eMENU_K1
		);
	if (oldsetting != g_SuSettings.K1)
		save_current_settings();		// save to non-volatile memory
}

void edit_beep_volume()
{
	int	oldsetting = g_SuSettings.BeepVolume;
	g_SuSettings.BeepVolume = SU_EditProperyWithSlider(g_SuSettings.BeepVolume, 
		0, 
		255, 
		BEEP_VOLUME_DEFAULT,
#ifdef RUSSIAN
		"\xD0\x93\xD1\x80\xD0\xBE\xD0\xBC\xD0\xBA\xD0\xBE\xD1\x81\xD1\x82\xD1\x8C \xD0\xB7\xD1\x83\xD0\xBC\xD0\xBC\xD0\xB5\xD1\x80\xD0\xB0",
		"\xD0\xB5\xD0\xB4\xD0\xB8\xD0\xBD\xD0\xB8\xD1\x86",
#else
		"Beep Volume",
		"counts",
#endif		
		eMENULCD_BEEP_VOLUME
		);
	if (oldsetting != g_SuSettings.BeepVolume)
	{
		SetBeepVolume(g_SuSettings.BeepVolume);
		save_current_settings();		// save to non-volatile memory
	}


}


void SU_EditOption(int setting)			// edit the selected item
{
	int newsetting;
	
	switch ((eOptionMenu)setting) 
	{
/*		case eMENUDATE:
			SU_EditTime(TRUE);
			break;*/	//!
/*		case eMENUTIME:
			SU_EditTime(FALSE);
			break;*/	//!
		case eMENUPOWER_OFF_DELAY:
			edit_power_off_delay();
			break;
		case eMENULOCATE_DURATION:
			edit_locate_duration();
			break;
/*		case eMENU_VIEW_SDLOG:
			SU_RunLogViewPage();
			break;*/	//!
		case eMENULCD_CALIBRATE:	// this one is a bit tricky since we have to wait for the response from both *RT and tc before continuing
			DelayMsecs(500);
			while (GetNextLCDSentence()!=NULL);	// flush buffer
//			SLCD_SendCmd("*RT");	// RESETS CALIBRATION TO INTERNAL DEFAULT
//			while (GetNextLCDSentence() == NULL);	// wait for a response
			SLCD_SendCmd("tc");		// runs through the touch screen calibration routine
//			while (GetNextLCDSentence()== NULL);	// wait for a response
			SetDefaultBackForegnd();		
			break;
/*		case eMENULCD_INTERFACE_MODE:
			{
				newsetting = SU_EditUserOption(g_SuSettings.advanced_interface, "Normal User Mode", "Advanced User Mode", "User mode");
				if (newsetting != g_SuSettings.advanced_interface)
				{
					g_SuSettings.advanced_interface = newsetting;
					save_current_settings();		// save to non-volatile memory
				}
			}
			break;
*/
		case eMENU_AUDIO_BLANK:
			#ifdef RUSSIAN
				newsetting = SU_EditUserOption(g_SuSettings.enable_audio_blanking, 
				"\xD0\xBE\xD0\xB7\xD0\xB2\xD1\x83\xD1\x87\xD0\xB8\xD0\xB2\xD0\xB0\xD0\xBD\xD0\xB8\xD0\xB5 \xD0\xB2\xD0\xBA\xD0\xBB\xD1\x8E\xD1\x87\xD0\xB5\xD0\xBD\xD0\xBE", 
				"\xD0\xBE\xD0\xB7\xD0\xB2\xD1\x83\xD1\x87\xD0\xB8\xD0\xB2\xD0\xB0\xD0\xBD\xD0\xB8\xD0\xB5 \xD0\xBE\xD1\x82\xD0\xBA\xD0\xBB\xD1\x8E\xD1\x87\xD0\xB5\xD0\xBD\xD0\xBE",
				"\xD0\xA1\xD0\xB8\xD0\xB3\xD0\xBD\xD0\xB0\xD0\xBB \xD0\xBF\xD0\xB5\xD1\x80\xD0\xB5\xD0\xB4\xD0\xB0\xD1\x82\xD1\x87\xD0\xB8\xD0\xBA\xD0\xB0"
				);
			#else
				newsetting = SU_EditUserOption(g_SuSettings.enable_audio_blanking, 
				"Disable Audio Blank", 
				"Enable Audio Blank",
				"Audio Blank on TX"
				);
			#endif	
			if (newsetting != g_SuSettings.enable_audio_blanking)
			{
				g_SuSettings.enable_audio_blanking = newsetting;
				save_current_settings();		// save to non-volatile memory
			}
			break;
		case eMENU_PACKET_BEEP:
			#ifdef RUSSIAN
				newsetting = SU_EditUserOption(g_SuSettings.enable_beep_on_VLF_packet, 
				"\xD0\xBD\xD0\xB5 \xD0\xBE\xD0\xB7\xD0\xB2\xD1\x83\xD1\x87\xD0\xB8\xD0\xB2\xD0\xB0\xD1\x82\xD1\x8C \xD0\xB7\xD1\x83\xD0\xBC\xD0\xBC\xD0\xB5\xD1\x80\xD0\xBE\xD0\xBC", 
				"\xD0\xBE\xD0\xB7\xD0\xB2\xD1\x83\xD1\x87\xD0\xB8\xD0\xB2\xD0\xB0\xD1\x82\xD1\x8C \xD0\xB7\xD1\x83\xD0\xBC\xD0\xBC\xD0\xB5\xD1\x80\xD0\xBE\xD0\xBC",
				"\xD0\x9F\xD1\x80\xD0\xB8\xD1\x91\xD0\xBC \xD1\x81\xD0\xBE\xD0\xBE\xD0\xB1\xD1\x89\xD0\xB5\xD0\xBD\xD0\xB8\xD0\xB9"
				);
			#else
				newsetting = SU_EditUserOption(g_SuSettings.enable_beep_on_VLF_packet, 
				"Disable Beep on Packet", 
				"Enable Beep on Packet",
				"Beep on Packet RX"
				);
			#endif	
			if (newsetting != g_SuSettings.enable_beep_on_VLF_packet)
			{
				g_SuSettings.enable_beep_on_VLF_packet = newsetting;
				save_current_settings();		// save to non-volatile memory
			}
			break;	
		case eMENU_WIRELESS:
			#ifdef RUSSIAN
			 newsetting = SU_EditUserOption(g_SuSettings.enable_wireless, 
				"\xD0\xBF\xD0\xBE \xD0\xBA\xD0\xB0\xD0\xB1\xD0\xB5\xD0\xBB\xD1\x8E",
				"\xD0\xB1\xD0\xB5\xD1\x81\xD0\xBF\xD1\x80\xD0\xBE\xD0\xB2\xD0\xBE\xD0\xB4\xD0\xBD\xD0\xB0\xD1\x8F", 
				"\xD0\xA1\xD0\xB2\xD1\x8F\xD0\xB7\xD1\x8C \xD1\x81 \xD0\xBF\xD0\xB5\xD1\x80\xD0\xB5\xD0\xB4\xD0\xB0\xD1\x82\xD1\x87\xD0\xB8\xD0\xBA\xD0\xBE\xD0\xBC"
				);
			#else
			 newsetting = SU_EditUserOption(g_SuSettings.enable_wireless, 
				"Tx Comm over Cable",
				"TX Comm over Wireless", 
				"TX Comm Method"
				);
			#endif	
			if (newsetting != g_SuSettings.enable_wireless)
			{
				g_SuSettings.enable_wireless = newsetting;
				save_current_settings();		// save to non-volatile memory
#if (BOARD_REV == 2)
				if (g_SuSettings.enable_wireless)
				{
					MRF24J40Wake(); 	// wake up the wireless
					SUTX_POWER_OFF;		// disable SU-TX RS485 module to save power
				}
				else
				{
					MRF24J40Sleep();	// PUT wireless into sleep mode
					SUTX_POWER_ON;		// enable the SU-TX RS485 module
				}
#endif
			}
			break;	

			
		case eMENU_ZCTHRESHOLD:
			edit_zc_threshold();
			break;
		case eMENU_K1:
			edit_k_factor();
			break;
		case eMENULCD_BEEP_VOLUME:
			edit_beep_volume();
			break;
		case eMENU_LOCATE_D:
			edit_locate_distance();
			break;
////////////////////////////////////////
//// by Alex
/////
		case eMENU_AUTOCALIBRATION:
			#ifdef RUSSIAN
			 newsetting = SU_EditUserOption(g_SuSettings.enable_autocalibration, 
				"\xD0\xA0\xD1\x83\xD1\x87\xD0\xBD\xD0\xBE\xD0\xB9",//Ручной
				"\xD0\x90\xD0\xB2\xD1\x82\xD0\xBE\xD0\xBC\xD0\xB0\xD1\x82\xD0\xB8\xD1\x87\xD0\xB5\xD1\x81\xD0\xBA\xD0\xB8\xD0\xB9",// Автоматический
				"\xD0\xA0\xD0\xB5\xD0\xB6\xD0\xB8\xD0\xBC \xD0\xBA\xD0\xB0\xD0\xBB\xD0\xB8\xD0\xB1\xD1\x80\xD0\xBE\xD0\xB2\xD0\xBA\xD0\xB8"   //Режим калибровки
				);
			#else
			 newsetting = SU_EditUserOption(g_SuSettings.enable_autocalibration,
				"Manual",
				"Auto", 
				"Calibration Method"
				);
			#endif	
			if (newsetting != g_SuSettings.enable_autocalibration)
			{
				g_SuSettings.enable_autocalibration = newsetting;
				save_current_settings();		// save to non-volatile memory
			}
			break;	
///////////////////////////////////////////////////

		default:
			break;
	}
}




int SU_EditUserOption(int saved_setting, char* title1, char* title2, char* pWindowTitle)
{
	struct button btn;
	int cursorpos = saved_setting?1:0;
	int adder = NAVIGATION_BTN_BMP_WIDTH_WITH_BORDER;
	int x = SLCD_MAX_X - (adder * 5);
	int oldsetting;
	char buffer[128];
	BOOL bSaved;
	
	SetDefaultBackForegnd();		
    SLCD_SendCmd(CLEARSCREEN);			// clear screen
 
   
#ifdef RUSSIAN
	#ifdef INCLUDE_MINERADIO_HEADER
		DRAW_MINERADIO_HEADER;
		DRAW_MINERADIO_FOOTER4;
	#endif
	SLCD_WriteFooter4(
	"\xD0\x94\xD0\xBB\xD1\x8F \xD0\xB8\xD0\xB7\xD0\xBC\xD0\xB5\xD0\xBD\xD0\xB5\xD0\xBD\xD0\xB8\xD1\x8F \xD0\xB2\xD0\xBE\xD1\x81\xD0\xBF\xD0\xBE\xD0\xBB\xD1\x8C\xD0\xB7\xD1\x83\xD0\xB9\xD1\x82\xD0\xB5\xD1\x81\xD1\x8C",
	"\xD0\xBA\xD0\xBD\xD0\xBE\xD0\xBF\xD0\xBA\xD0\xB0\xD0\xBC\xD0\xB8 \xD0\xB8\xD0\xBB\xD0\xB8 \xD0\xB2\xD1\x8B\xD0\xB1\xD0\xB5\xD1\x80\xD0\xB5\xD1\x82\xD0\xB5 \xD0\xBD\xD1\x83\xD0\xB6\xD0\xBD\xD1\x8B\xD0\xB9",
	"\xD0\xBF\xD1\x83\xD0\xBD\xD0\xBA\xD1\x82 \xD0\xBD\xD0\xB0 \xD1\x81\xD0\xB5\xD0\xBD\xD1\x81\xD0\xBE\xD1\x80\xD0\xBD\xD0\xBE\xD0\xBC \xD1\x8D\xD0\xBA\xD1\x80\xD0\xB0\xD0\xBD\xD0\xB5",
	"*=\xD0\xA1\xD0\xBE\xD1\x85\xD1\x80\xD0\xB0\xD0\xBD\xD0\xB8\xD1\x82\xD1\x8C \xD0\xB2\xD1\x8B\xD0\xB1\xD0\xBE\xD1\x80"
	);
#else
	#ifdef INCLUDE_MINERADIO_HEADER
		DRAW_MINERADIO_HEADER;
		DRAW_MINERADIO_FOOTER;
	#endif
	SLCD_WriteFooter("","Use arrow keys to change setting","Press * to save");
#endif
	
	SetDefaultBackForegnd();		
	//SLCD_SendCmd("f32");
	SLCD_SendCmd("fUARIAL18");	// sets the unicode character set for the title
	sprintf(buffer,"t \"%s\" 10 50",pWindowTitle);			// set title
	SLCD_SendCmd(buffer);

	SU_DisplayOptionMenuItem(SETTINGS_BUTTON1, title1, (cursorpos == 0), FALSE );
	SU_DisplayOptionMenuItem(SETTINGS_BUTTON2, title2, (cursorpos == 1), FALSE );

	SU_DisplayStandardNavigationButtons(x+adder, NAVIGATION_BTN_YPOS, adder,TRUE);

	while (1) {				// loop until we get an exit from the page
		unsigned short taskStatus = ProcessBackgroundTasks();
		btn = GetUserAction(taskStatus);
		if (btn.index != 0) {
			oldsetting = cursorpos;
			bSaved = FALSE;
			switch (btn.index) 
			{
				case SU_BTN_UP:
				case SU_BTN_DOWN:
					if (cursorpos == 1)
						cursorpos = 0;
					else
						cursorpos = 1;
					break;
				case SU_BTN_EXIT:
					return saved_setting;
					break;
				case SU_BTN_ENTER:
					bSaved = TRUE;
					break;
				case SETTINGS_BUTTON1:
					return 0;
					break;
				case SETTINGS_BUTTON2:
					return 1;
					break;
/*				case SETTINGS_BUTTON1:
					if (cursorpos == 0)			// if we're already here then treat it as an enter
						bSaved = TRUE;
					else
						cursorpos = 0;
					break;
				case SETTINGS_BUTTON2:
					if (cursorpos == 1)			// if we're already here then treat it as an enter
						bSaved = TRUE;
					else 
						cursorpos = 1;
					break;
*/
				default:
					break;
			}
			if (bSaved)
			{
				saved_setting = cursorpos;
				SU_DisplayOptionMenuItem(SETTINGS_BUTTON1,title1, (cursorpos == 0), FALSE );
				SU_DisplayOptionMenuItem(SETTINGS_BUTTON2,title2, (cursorpos == 1), FALSE );
			}
			else if (oldsetting != cursorpos) 
			{
				SU_DisplayOptionMenuItem(SETTINGS_BUTTON1,title1, (cursorpos == 0), ((saved_setting == 1) && (cursorpos == 0)) );
				SU_DisplayOptionMenuItem(SETTINGS_BUTTON2,title2, (cursorpos == 1), ((saved_setting == 0) && (cursorpos == 1)) );
			}
		}
	}
	return saved_setting;
}



SuSettings g_SuSettings;
#define SETTINGS_EEPROM_VERSION 5
////////////////////////////////////// LOADING AND SAVING SETTINGS FROM EEPROM OR FLASH MEMORY ///////////////////
char load_non_volotile_settings()
{
	return load_settings(&g_SuSettings);
}


char load_settings(SuSettings* pSettings)
{
	char error = 0;
	unsigned char*  p = (unsigned char*)pSettings;
	int count;
	unsigned int DEEaddr = SETTINGS_EEPROM_OFFSET;

    dataEEFlags.val = 0;		// CLEAR STATUS FLAGS

    char c = DataEERead(DEEaddr++);

    // When a record is saved, first byte is written as version marker to indicate
    // that a valid record was saved.  This change has been made to so old EEPROM contents 
	// will get overwritten.  
    if(c == SETTINGS_EEPROM_VERSION)
    {
        for ( count = 0; count < sizeof(SuSettings); count++ ) {
            *p++ = DataEERead(DEEaddr++);
            if (dataEEFlags.val) {
	            error = 1;
	            break;
	        }
	    }  
    }
    else
    	error = 1;
    	
    return error;
}	



char save_current_settings()
{
	return save_settings(&g_SuSettings);
}


// saves the config settings to flash eeprom  
char save_settings(SuSettings* pSettings)
{
	int c;
	int len = sizeof(SuSettings);
    unsigned char* p = (unsigned char*)pSettings;
 	unsigned int DEEaddr = SETTINGS_EEPROM_OFFSET;
   
    dataEEFlags.val = 0;		// CLEAR STATUS FLAGS

    DataEEWrite((unsigned char)SETTINGS_EEPROM_VERSION,DEEaddr++);

	if (dataEEFlags.val == 0 ) {
	    for ( c = 0; c < len; c++ )
    	{
        	DataEEWrite(*p++,DEEaddr++);
        	if (dataEEFlags.val)
        		break;
    	}
 	}
    
	return dataEEFlags.val;
}


void default_settings(SuSettings* pSettings)
{
	pSettings->screen_brightness = LCD_BRIGHTNESS_DEFAULT;
	pSettings->locate_duration_seconds = LOCATE_DURATION_DEFAULT;
	pSettings->screen_power_off_seconds = LCD_POWER_OFF_DEFAULT;
	pSettings->ZC_trigger_level = ZC_THRESHOLD_DEFAULT;	// threshold value for detecting zero-crossings
	pSettings->ZCx_trigger_adjust = 0;			// adjust value to be added to ZC_trigger_level for X
	pSettings->ZCy_trigger_adjust = 0;			// adjust value to be added to ZC_trigger_level for y
	pSettings->ZCz_trigger_adjust = 0;			// adjust value to be added to ZC_trigger_level for z
	pSettings->K1 = K1_DEFAULT;
	pSettings->BeepVolume = BEEP_VOLUME_DEFAULT;
	pSettings->advanced_interface = 0;
	pSettings->enable_audio_blanking = TRUE;
	pSettings->enable_beep_on_VLF_packet = TRUE; 
	pSettings->enable_wireless = TRUE; //FALSE;
//	pSettings->enable_wireless = FALSE;
	pSettings->locate_ref_meters = DEFAULT_LOCATE_REF_METERS;
	pSettings->enable_autocalibration = TRUE; //FALSE;		//by Alex
}	
