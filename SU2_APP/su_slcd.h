#ifndef _SU_SLCD_H
#define _SU_SLCD_H

#include "system.h"
#if (RTCC_SOURCE == RTCC_INTERNAL)	
#include "rtcc.h"
#elif (RTCC_SOURCE == RTCC_EXTERNAL)	
#include "i2c_ISL12024_rtc.h"
#endif
#include "vlf_pkt.h"
#include "button.h"

#define LCD_FRAMECHAR '\r'

typedef enum ESU_PAGES {
	SU_PAGE_NONE,
	SU_PAGE_MAIN,
	SU_PAGE_SEARCH,
	SU_PAGE_SETTINGS,
	SU_PAGE_LOG,
	SU_PAGE_COUNT
}eSU_Pages;

#define INCLUDE_MINERADIO_HEADER
#define SET_TO_MINERADIO_BACKGROUND SLCD_SendCmd("S FFFFFF 8B2346")
#define DRAW_MINERADIO_HEADER SLCD_SendCmd("xi 34 0 0")		
#define DRAW_MINERADIO_FOOTER SLCD_SendCmd("xi 35 0 408")	
#define DRAW_MINERADIO_FOOTER4 SLCD_SendCmd("xi 35 0 408"); SLCD_SendCmd("xi 35 0 392")	
//#define SET_DEFAULT_FOOTER_FONT SLCD_SendCmd("f18BC")


#define SET_DEFAULT_FOOTER_FONT	SLCD_SendCmd("fUARIAL12")

//#define SET_DEFAULT_HEADER_FONT SLCD_SendCmd("f18BC")
#define SET_DEFAULT_HEADER_FONT	SLCD_SendCmd("fUARIAL12")
	


#define SLCD_BLACK 0
#define SLCD_WHITE 1
#define SLCD_BLUE 2
#define SLCD_GREEN 3
#define SLCD_CYAN 4
#define SLCD_RED 5
#define SLCD_MAGENTA 6
#define SLCD_BROWN 7
#define SLCD_DARK_GREY 8
#define SLCD_GREY 9
#define SLCD_LIGHT_GREY 10
#define SLCD_LIGHT_BLUE 11
#define SLCD_LIGHT_GREEN 12
#define SLCD_LIGHT_CYAN 13
#define SLCD_LIGHT_RED 14
#define SLCD_LIGHT_MAGENTA 15
#define SLCD_YELLOW 16
#define CLEARSCREEN "z"

struct button {
	int index;
	int state;
};

#define MURX_POWER_MIN 0
#define MURX_POWER_MAX 9

#define ZERO_CROSSING_NONE 255

#define TEST_LOG_VIEW_SIZE 30


typedef struct MU_DATA 
{
	U32 id_u32; 
	union {	
		struct {
				U8 mobility:4;
				U8 bit5:1;
				U8 bit6:1;
				U8 batteryproblem:1;
				U8 masked:1;
		} status;
		U8 mobility;
	} m ;
	U16 x1_power;				// power level 
	U16 y1_power;				// power level 
	U16 z1_power;				// power level 
	char antenna;				// the antenna from which the last communications came from
	union {	
		struct {
			U8 ID_valid:1;
			U8 Mobility_valid:1;
			U8 X1_power_valid:1;
			U8 Y1_power_valid:1;
			U8 Z1_power_valid:1;
		};
		U8 isvalid;
	} v ;
	int packet_count;
} __attribute__((packed)) MuData;		// 1 BYTE PACKING


//FULL_MASK
typedef struct MU_FULLMASK 
{
	U32 id_u32; 
	time_t last_fullmask_time;					// timestamp of last fullmask			
} __attribute__((packed)) MuFullMaskData;		// 1 BYTE PACKING
#define MAXIMUM_MU_FULLMASK_DEVICES_IN_LIST 50
//FULL_MASK END


#define SLCD_FOREGROUND SLCD_BLACK
#define SLCD_BACKGROUND SLCD_WHITE

#define MAXIMUM_MU_DEVICES_IN_LIST 100
#define MAX_SLCD_RESPONSE 128


//#define SU_BTN_SEARCH 10
//#define SU_BTN_SETTINGS 11
//#define SU_BTN_LOG 12
//#define SU_BTN_POWER 13

//#define SU_BTN_LOCATE 20

#if (KEYBOARD_BUTTONS == 16)
#define SU_BTN_UP 30
#define	SU_BTN_DOWN 31
#define	SU_BTN_EXIT 32
#define	SU_BTN_ENTER 33

#define	SU_BTN_NORMAL_SEARCH 34
#define	SU_BTN_LOCATE 35
#define	SU_BTN_SETTINGS 36
#define	SU_BTN_LOG 37

#define	SU_BTN_PAUSE 38
#define	SU_BTN_TEST 39
#define	SU_BTN_SPARE2 40
#define SU_BTN_QUICK_SEARCH 41
#define SU_BTN_UPDOWN 42
#define SU_BTN_F1 38
#define SU_BTN_F2 39

#elif (KEYBOARD_BUTTONS == 6)
#define SU_BTN_UP 30
#define	SU_BTN_DOWN 31
#define	SU_BTN_EXIT 32
#define	SU_BTN_ENTER 33
#define SU_BTN_F1 34
#define SU_BTN_F2 35
#define SU_BTN_UPDOWN 36
#define SU_BTN_F1F2 37

#else
#error KEYBOARD BUTTON SIZE NOT DEFINED
#endif


#define SLCD_MAX_X 272
#define SLCD_MAX_Y 480


#define SU_MAIN_BATTERY_YPOS (0)
#define SU_MAIN_BATTERY_XPOS (SLCD_MAX_X - 80)
#define BATTERY_BAR_HEIGHT 12
#define BATTERY_BAR_WIDTH 36

#define SU_MAIN_TIME_XPOS (10)
#define SU_MAIN_TIME_YPOS (0)
#define STATUS_XPOS (10)
#define STATUS_YPOS (SU_MAIN_BATTERY_YPOS + 18)


#define TX_STATUS_YPOS (SU_MAIN_BATTERY_YPOS + 17)
#define TX_STATUS_YTEXTPOS (TX_STATUS_YPOS-4)
#ifdef RUSSIAN
	#define TX_STATUS_XPOS (SU_MAIN_BATTERY_XPOS - 120)
#else
	#define TX_STATUS_XPOS (SU_MAIN_BATTERY_XPOS - 70)
#endif	

#define PAGE_START_BELOW_HEADER 60



// define bitmaps for navigation buttons
//#define NAVIGATION_BTN_UP_BMP 39
//#define NAVIGATION_BTN_DN_BMP 40
//#define NAVIGATION_BTN_BMP_WIDTH 32
//#define NAVIGATION_BTN_BMP_HEIGHT 32

#define NAVIGATION_BTN_UP_BMP 10
#define NAVIGATION_BTN_DN_BMP 11
#define NAVIGATION_BTN_BMP_WIDTH 48
//#define NAVIGATION_BTN_BMP_HEIGHT 48
#define NAVIGATION_BTN_BMP_HEIGHT 24
#define NAVIGATION_BTN_BMP_WIDTH_WITH_BORDER 50


#define NAVIGATION_BTN_YPOS (SLCD_MAX_Y - (NAVIGATION_BTN_BMP_HEIGHT + 4))

#define HELP_YPOS (SLCD_MAX_Y-(3*24))

#define SU_BATTERY_LEVEL_INDEX 0		// INDEX OF BATTERY BAR CONTROL
#define SUTX_BATTERY_LEVEL_INDEX 1		// INDEX OF BATTERY BAR CONTROL
#define SU_MOBILITY_LEVEL_INDEX 2		// INDEX OF ACCELERATION BAR CONTROL
#define SU_XPOWER_LEVEL_INDEX 3		// INDEX OF POWER BAR CONTROL
#define SU_YPOWER_LEVEL_INDEX 4		// INDEX OF POWER BAR CONTROL
#define SU_ZPOWER_LEVEL_INDEX 5		// INDEX OF POWER BAR CONTROL

#define SU_X2POWER_LEVEL_INDEX 6		// INDEX OF POWER BAR CONTROL
#define SU_Y2POWER_LEVEL_INDEX 7		// INDEX OF POWER BAR CONTROL
#define SU_Z2POWER_LEVEL_INDEX 8		// INDEX OF POWER BAR CONTROL

#define LCD_POWER_OFF_MIN 10
#define LCD_POWER_OFF_DEFAULT 1500
#define LCD_POWER_OFF_MAX 3000

#define LCD_BRIGHTNESS_MIN 1
#define LCD_BRIGHTNESS_DEFAULT 50
#define LCD_BRIGHTNESS_MAX 255

#define ZC_THRESHOLD_COUNTS_TO_MV(counts) ((int)((((long)counts*3300l)+2048)/4096l))
#define ZC_THRESHOLD_MV_TO_COUNTS(mv) ((int)((((long)mv*4096l)+1650)/3300l))

#define ZC_THRESHOLD_MIN 2048		// approx 1.65 volts
#define ZC_THRESHOLD_MAX 3104		// approx 2.5 volts
#define ZC_THRESHOLD_DEFAULT 3000	// approx 2.42 volts
//#define ZC_THRESHOLD_DEFAULT 2110	// approx 1.7 volts
//#define ZC_THRESHOLD_DEFAULT 2700		// FOR TESTING

#define BEEP_VOLUME_DEFAULT 200
#define DEFAULT_LOCATE_REF_METERS 2

#define K1_DEFAULT	127
#define K1_MIN 64
#define K1_MAX 512
#define K_MULTIPLIER 1000.0

#define K2_DEFAULT 4030
#define K2_MIN 2000
#define K2_MAX 6000



#define SLIDER_CONTROL 128

#define MAIN_MENU_AUDIO_SEARCH_BUTTON 150
#define MAIN_MENU_ID_SEARCH_BUTTON 151
#define MAIN_MENU_CALIBRATION_BUTTON 152

#define SETTINGS_BUTTON1 156 
#define SETTINGS_BUTTON2 157

#define LOG_DELETE_BUTTON 158 
#define LOG_VIEW_BUTTON 159


// these must be in order for the time & date menu to work properly for both time and date
#define EDIT_HOUR_BUTTON 150
#define EDIT_MINUTE_BUTTON 151
#define EDIT_SECOND_BUTTON 152
#define EDIT_DAY_BUTTON 153
#define EDIT_MONTH_BUTTON 154
#define EDIT_YEAR_BUTTON 155


#define MENU_HEIGHT_BORDER 15
#define MENU_WIDTH_BOARDER 3
#define MENU_XPOS_START MENU_WIDTH_BOARDER
#define MENU_XPOS_END (SLCD_MAX_X-MENU_WIDTH_BOARDER)
#define MENU_YPOS 50
#define MENUITEM_HEIGHT 28
#define MENU_SELECTED_COLOR SLCD_YELLOW
#define MENU_NORMAL_COLOR SLCD_LIGHT_GREY
#define MENU_EDIT_COLOR SLCD_LIGHT_GREEN
#define MENUTITLE_XPOS (MENU_XPOS_START + 0)
#define MENUVALUE_XPOS (MENU_XPOS_START + 132)

#define FILE_LINE_HEIGHT 16
#define FILE_TITLE_YPOS 50
#define FILE_BASE_YPOS (FILE_TITLE_YPOS + 32)
#define FILE_MAX_LINES_PER_PAGE ((((SLCD_MAX_Y-10) - FILE_BASE_YPOS)/FILE_LINE_HEIGHT)-1)
#define MAX_DISPLAY_LINE_WIDTH 40


////////////////////////////////////////////////////////////////////////////
#define XON	17
#define XOFF 19
#define THRESHOLD_1F 12	
#define RSSI_TO_METERS(K,rssi) (pow((K/rssi),1.0/3.0))

//////////////////////////////////// external variable declarations
extern MuData g_MuDeviceList[MAXIMUM_MU_DEVICES_IN_LIST];
extern MuData g_TestModeData;

extern long g_SLCDErrors;
extern SuSettings g_SuSettings;
extern BOOL g_bTestMode;
extern unsigned int g_lastbattMV;	// last battery reading

typedef struct XYZ_READINGS {
	unsigned int16 x;
	unsigned int16 y;
	unsigned int16 z;
} XYZ_Reading;

extern XYZ_Reading g_zero_crossings;
extern XYZ_Reading g_last_Clutter_rssi;
extern XYZ_Reading g_peak_Clutter_rssi;
extern XYZ_Reading g_last_Packet_rssi;
extern XYZ_Reading g_peak_Packet_rssi;
extern void Reset_XYZ_Reading(XYZ_Reading* p);
extern void ResetPeakRssiReadings();
extern void ResetLastRssiReadings();
	

extern BOOL g_bTransmitting;		// indicates whether we're currently transmitting or not

//////////////////////////////////// prototype declarations
extern void SU_RunMainPage();
extern void SU_RunSettingsPage();
extern int SU_RunSearchPage(int bAudioSearchMode);
extern void SU_RunLocatePage(int index);
extern void SU_Run_P2_LocatePage(int index);
extern void SU_RunTestPage();
extern int SU_Run_P2_SearchPage(int bAudioSearchMode);
void Send_P2_SearchMsg(int bAudioSearchMode);

extern void SLCD_Reset();
extern void SU_Init();
void SU_DataClearMobility();
extern void SU_DataReset();

///////////////////////////////////////// external references //////////////////////////////////////

extern void SLCD_SendCmd(char *buf);
extern void SU_DisplayStandardNavigationButtons(int x, int y, int width, BOOL bWithExit);
extern int SU_GetResponse(char* buf, struct button* btn);
extern void SU_InitBatteryBar( unsigned char index, int x, int y );
extern void SU_UpdateTime(int x, int y);
extern void SU_UpdateBatteryLevel(BOOL bForcedUpdate, BOOL bPercentage);
extern void SU_SendText(char *buf);
extern int GetMUListSize();
extern char* GetNextLCDSentence();
extern struct button GetUserAction(unsigned short taskStatus);
extern void SLCD_Power(BOOL bOn);
extern void SU_UpdateStatus(BOOL bForcedUpdate);
extern void UpdateLocatePeakPower(int value);
extern void clear_PeakPower();
extern int getPeakPower_1F();
extern int getPeakPower_2F();

extern char load_non_volotile_settings();
extern char save_current_settings();
extern char load_settings(SuSettings* pSettings);
extern char save_settings(SuSettings* pSettings);
extern void default_settings(SuSettings* pSettings);
extern void SLCD_IncPowerDownCounter();
extern void SU_DataResetUnit(MuData* p);
extern void SU_UpdateUnit(MuData* p);
extern void SU_UpdateGeneralStatus(BOOL bIncludeTime, BOOL bForceNow);
extern void SLCD_putBootloadMessage();
extern void SU_RunLogViewPage();
extern void SU_DrawLocatePage();
extern unsigned short ProcessBackgroundTasks();
extern int ScaleLogPower(int power);
extern int ScaleLogPowerToMax(int power, int max, long range);
extern void SU_UpdateCOMStatus(BOOL bTest);
void StartPingStatus();
void UpdatePeakPower_1F(int value);
void UpdatePeakPower_2F(int value);
void UpdateTestModeData();
extern void InitRSSIBar(int id, int x, int y, int width, int height);
extern void UpdateRssiValue(int rssi, char* label, int barindex, int x, int y);
extern void UpdatePeakRssiValue(int rssi, char* label, int x, int y);
extern double Get_MU_Distance(MuData* p);
extern void SetDefaultBackForegnd();
void SetForeBackGndColor(int back,int fore);
extern void SU_EditScreenBrightness();
void SetBeepVolume(int volume);
void SetScreenBrightness(int value);
void SU_InitMainBatteryBar();
void SU_InitTXBatteryBar();
void SU_UpdateTXBatteryLevel(BOOL bForcedUpdate, BOOL bPercentage);
void SLCD_WriteFooter(char* f1, char* f2, char *f3);
void SLCD_WriteFooter4(char* f1, char* f2, char *f3,char *f4);
void WaitForSUTXResponse(unsigned int msecs);
MuData* GetMUListEntryByID(U32 id);
int GetMUEntryIndexByID(U32 id);

//FULL_MASK
int GetMUFullMaskListSize();
extern MuFullMaskData g_MuFullMaskDeviceList[MAXIMUM_MU_FULLMASK_DEVICES_IN_LIST];
void SU_FullMaskTableReset();
extern void SU_MoveHPTTo_FullMaskList(U32 id_u32);
extern void SU_RemoveHPTFrom_FullMaskList(U32 id_u32);

extern int SU_Run_FullMaskPage();
extern BOOL isRunningFullMaskPage();
#define MU_FULL_MASK_MINUTES 30
//#define MU_FULL_MASK_MINUTES 5
#define MU_FULL_MASK_SECONDS (MU_FULL_MASK_MINUTES*60)
extern int SU_FullMaskList_GetShortestCountdownSeconds();
extern int GetFullMaskList_IndexByID(U32 id);
// uncomment next line to enable testing the addition of IDs without a real MU
//#define ENABLE_TEST_FAKEHPT_MODE
#ifdef ENABLE_TEST_FAKEHPT_MODE
	extern void SLCD_Test_Search(BOOL bRunTest);
	extern void SLCD_Test_MaskReponse(U32 id_u32, U8 minutes);
#endif
//FULL_MASK

#endif //#ifndef _SU_SLCD_H
