#include "system.h"
#include "su_slcd.h"
#include <stdio.h>
#include <stdlib.h>
#include "vlf_rxtx.h"
#include "string.h"
#include "trace.h"

#define USE_STRIP_CHART


MuData g_TestModeData;
//////////////////////////////////////// local globals ////////////////////////////////////////
BOOL g_bTestMode = FALSE;

/////////////////////////////////////////// local prototypes ////////////////////////////
BOOL SU_AutoCalibrate(void);			//by Alex

void SU_InitTestBar( unsigned char index, int x, int y );
void UpdateTestZeroCrossings();
void SU_DrawTestData(void);
#ifdef USE_STRIP_CHART
static void SU_Update_RSSI_StripChart();
static void SU_Init_RSSI_StripChart();
static void SU_Update_ZC_StripChart(int x, int y, int z);
static void SU_Init_ZC_StripChart();
#endif
//////////////////////////////////////////// local defines ///////////////////////////////

#define TEST_FONT_HEIGHT 24

#define TEST_INDENT 10
#define TEST_YPOS (TEST_FONT_HEIGHT*4)


#define POWER_BAR_INDENT 	(SLCD_MAX_X/2 + 40)

#define TEST_X1POWER_LEVEL_INDEX 1		// INDEX OF POWER BAR CONTROL
#define TEST_Y1POWER_LEVEL_INDEX 2		// INDEX OF POWER BAR CONTROL
#define TEST_Z1POWER_LEVEL_INDEX 3		// INDEX OF POWER BAR CONTROL

#define TEST_X2POWER_LEVEL_INDEX 4		// INDEX OF POWER BAR CONTROL
#define TEST_Y2POWER_LEVEL_INDEX 5		// INDEX OF POWER BAR CONTROL
#define TEST_Z2POWER_LEVEL_INDEX 6		// INDEX OF POWER BAR CONTROL


#define TEST_X1POWER_XPOS TEST_INDENT
#define TEST_Y1POWER_XPOS TEST_INDENT
#define TEST_Z1POWER_XPOS TEST_INDENT
#define TEST_X2POWER_XPOS TEST_INDENT
#define TEST_Y2POWER_XPOS TEST_INDENT
#define TEST_Z2POWER_XPOS TEST_INDENT

#define TEST_X1POWERBAR_XPOS (POWER_BAR_INDENT)
#define TEST_Y1POWERBAR_XPOS (POWER_BAR_INDENT)
#define TEST_Z1POWERBAR_XPOS (POWER_BAR_INDENT)
#define TEST_X2POWERBAR_XPOS (POWER_BAR_INDENT)
#define TEST_Y2POWERBAR_XPOS (POWER_BAR_INDENT)
#define TEST_Z2POWERBAR_XPOS (POWER_BAR_INDENT)

#define TEST_LEVELBAR_WIDTH (SLCD_MAX_X - POWER_BAR_INDENT - 20)
#define TEST_LEVELBAR_HEIGHT 14

#define TEST_X1POWER_YPOS (TEST_YPOS)
#define TEST_Y1POWER_YPOS (TEST_X1POWER_YPOS +TEST_FONT_HEIGHT)
#define TEST_Z1POWER_YPOS (TEST_Y1POWER_YPOS +TEST_FONT_HEIGHT)
#define TEST_PEAK_DELTA_1_YPOS (TEST_Z1POWER_YPOS + TEST_FONT_HEIGHT)

#define TEST_X2POWER_YPOS (TEST_PEAK_DELTA_1_YPOS +TEST_FONT_HEIGHT*2)
#define TEST_Y2POWER_YPOS (TEST_X2POWER_YPOS +TEST_FONT_HEIGHT)
#define TEST_Z2POWER_YPOS (TEST_Y2POWER_YPOS +TEST_FONT_HEIGHT)
#define TEST_PEAK_DELTA_2_YPOS (TEST_Z2POWER_YPOS + TEST_FONT_HEIGHT)

#define TEST_ZC_XPOS TEST_INDENT
#define TEST_ZC_YPOS (TEST_PEAK_DELTA_2_YPOS + TEST_FONT_HEIGHT*2)

#define HORIZONTAL_SLIDER_IMAGE_ID 9
#define WIDTH_OF_SLIDER 145
#define TEST_ZC_THRESHOLD_XPOS (SLCD_MAX_X - WIDTH_OF_SLIDER - 10)	
#define TEST_ZC_THRESHOLD_YPOS (TEST_ZC_YPOS + TEST_FONT_HEIGHT+0)

#define TEST_BATTERY_MV_XPOS TEST_INDENT
#define TEST_BATTERY_MV_YPOS (SLCD_MAX_Y - TEST_FONT_HEIGHT*3)




#ifdef USE_STRIP_CHART

// define a strip chart with 8 pens (each must have pen width of 1):
// -- index is 1
// -- corners are 6,150 and 266,389
// -- type is STRIP (0)
// -- data width is 5 pixels
// -- min/max data values are 0/160
// -- chart background color is a dark blue (RGB of 048)
// -- pen colors are:
//    -- orange (F80)
//    -- red    (F00)
//    -- green  (0F0)
//    -- blue   (00F)
//    -- purple (F0F)
//    -- yellow (FF0)
//    -- cyan   (0FF)
//    -- white  (FFF)
//cd 1 6 150 266 389 0 5 0 160 048 1 F80 1 F00 1 0F0 1 00F 1 F0F 1 FF0 1 0FF 1 FFF
// put a black border around it:
//S 000 FFF
//r 4 148 267 390

/*
Command: cd n x0 y0 x1 y1 t dw bv tv bc <pens>
Arguments: n - chart index from 0 to 9 (maximum 10 charts).
			x0, y0 and x1, y1 are the top left corner and bottom right corners of the chart area
			t - chart type; must be 0, 1, or 3 (see Description, above)
			dw - data width, number of pixels horizontally between chart data points
			bv - bottom data value (lowest y value)
			tv - top data value (highest y value)
			bc - background color in RGB format (3 ASCII hex characters Ц see SET COLOR DETAILED)
			<pens> - one or more sets of two values: pen width and pen color. Width is 1 or 2, color is same format as "bc" parameter.
Example: cd 0 10 20 110 120 1 4 0 99 333 2 0FF 1 F00
Defines a chart in the rectangular area (10,20), (110,120). Each data value will be 4 horizontal pixels wide. 
The chart (СYТ) values are scaled from 0 to 99. The background color is dark gray (333). 
Two pens are defined: the first is pen width 2, color teal (0FF), the second is pen width 1, color red (F00).
SLCD+/
*/

#define CALRSSI_STRIPCHART_X_START 1
#define CALRSSI_STRIPCHART_X_END 271
#define CALRSSI_STRIPCHART_Y_START 90
#define CALRSSI_STRIPCHART_Y_END 206
#define CALRSSI_STRIPCHART_RANGE (CALRSSI_STRIPCHART_Y_END-CALRSSI_STRIPCHART_Y_START)
#define CALRSSI_STRIPCHART_INDEX 0

static void SU_Init_RSSI_StripChart()
{
	char buffer[100];
	sprintf(buffer,"cd %d %d %d %d %d 0 5 0 %d 048 2 F00 2 0F0 2 FF0", 
		CALRSSI_STRIPCHART_INDEX,
		CALRSSI_STRIPCHART_X_START,
		CALRSSI_STRIPCHART_Y_START, 
		CALRSSI_STRIPCHART_X_END,
		CALRSSI_STRIPCHART_Y_END, 
		CALRSSI_STRIPCHART_RANGE
	); 
	SLCD_SendCmd(buffer);
	
	// put a black border above and below it:
	SetForeBackGndColor(SLCD_BLACK, SLCD_BACKGROUND);
	
	sprintf(buffer,"r %d %d %d %d",
		CALRSSI_STRIPCHART_X_START,
		CALRSSI_STRIPCHART_Y_START-3,
		CALRSSI_STRIPCHART_X_END,
		CALRSSI_STRIPCHART_Y_START-2
		);
	SLCD_SendCmd(buffer);

	sprintf(buffer,"r %d %d %d %d",
		CALRSSI_STRIPCHART_X_START,
		CALRSSI_STRIPCHART_Y_END+1,
		CALRSSI_STRIPCHART_X_END,
		CALRSSI_STRIPCHART_Y_END+2
		);
	SLCD_SendCmd(buffer);

}

static void SU_Update_RSSI_StripChart()
{
	char buffer[30];
	unsigned int x,y,z;

//	x = max(g_peak_Clutter_rssi.x,g_peak_Packet_rssi.x);
//	y = max(g_peak_Clutter_rssi.y,g_peak_Packet_rssi.y);
//	z = max(g_peak_Clutter_rssi.z,g_peak_Packet_rssi.z);

	x = max(g_last_Clutter_rssi.x,g_last_Packet_rssi.x);
	y = max(g_last_Clutter_rssi.y,g_last_Packet_rssi.y);
	z = max(g_last_Clutter_rssi.z,g_last_Packet_rssi.z);
	

	sprintf(buffer, "cv %d %d %d %d", 
		CALRSSI_STRIPCHART_INDEX,
		ScaleLogPowerToMax(x, CALRSSI_STRIPCHART_RANGE, 32768l), 
		ScaleLogPowerToMax(y, CALRSSI_STRIPCHART_RANGE, 32768l), 
		ScaleLogPowerToMax(z, CALRSSI_STRIPCHART_RANGE, 32768l)
		);
	SLCD_SendCmd(buffer);
	
}


#define ZC_STRIPCHART_X_START 1
#define ZC_STRIPCHART_X_END 271
#define ZC_STRIPCHART_Y_START 270
#define ZC_STRIPCHART_Y_END 320
#define ZC_STRIPCHART_INDEX 1

static void SU_Init_ZC_StripChart()
{
	char buffer[100];
	sprintf(buffer,"cd %d %d %d %d %d 0 5 0 50 048 2 F00 2 0F0 2 FF0 1 FFF", 
		ZC_STRIPCHART_INDEX,
		ZC_STRIPCHART_X_START,
		ZC_STRIPCHART_Y_START, 
		ZC_STRIPCHART_X_END,
		ZC_STRIPCHART_Y_END 
	); 
	SLCD_SendCmd(buffer);
	
	SetForeBackGndColor(SLCD_BLACK, SLCD_BACKGROUND);
	sprintf(buffer,"r %d %d %d %d",
		ZC_STRIPCHART_X_START,
		ZC_STRIPCHART_Y_START-3,
		ZC_STRIPCHART_X_END,
		ZC_STRIPCHART_Y_START-2
		);
	SLCD_SendCmd(buffer);

	sprintf(buffer,"r %d %d %d %d",
		ZC_STRIPCHART_X_START,
		ZC_STRIPCHART_Y_END+1,
		ZC_STRIPCHART_X_END,
		ZC_STRIPCHART_Y_END+2
		);
	SLCD_SendCmd(buffer);}

static void SU_Update_ZC_StripChart(int x, int y, int z)
{
	char buffer[30];
	sprintf(buffer, "cv %d %d %d %d 20", ZC_STRIPCHART_INDEX, x, y, z);
	SLCD_SendCmd(buffer);
}
#endif


//Description: Creates a slider object using background and slider control bitmaps.
//Command: sl idx bg x y slider off ornt inv cont hi lo
//Arguments: idx - slider index. Must be in the range 128 to 136 (maximum 8 sliders). Note that slider indices are shared with hotspot indices; that is if a slider is defined with index 128, hotspot index 128 cannot be used.
//bg Ц background bitmap index
//x, y - top left corner to place the background bitmap
//slider Ц slider control (e.g. knob / button) bitmap index
//off Ц slider offset from the edge of the background bitmap
//ornt Ц orientation: 0 = vertical; 1 = horizontal
//inv Ц invert: 0 = top / left is low; 1 = bottom / right is low
//cont Ц continuous touch: 0 = slider cannot be moved by sliding the touch point 1 = slider can be moved by sliding the touch point
//hi Ц maximum slider value
//lo Ц minimum slider value

static void InitThresholdSlider()
{
//	return;
	char buffer[80];
//	sprintf(buffer,"sl %d 41 %d %d 45 5 1 0 1 %d %d",SLIDER_CONTROL,TEST_ZC_THRESHOLD_XPOS,TEST_ZC_THRESHOLD_YPOS,ZC_THRESHOLD_MAX,ZC_THRESHOLD_MIN);
	// id 16 is the knob image and id HORIZONTAL_SLIDER_IMAGE_ID is the background for the slider
	// slider know is image 8
	sprintf(buffer,"sl %d %d %d %d 8 5 1 0 1 %d %d",SLIDER_CONTROL,HORIZONTAL_SLIDER_IMAGE_ID,TEST_ZC_THRESHOLD_XPOS,TEST_ZC_THRESHOLD_YPOS,ZC_THRESHOLD_MAX,ZC_THRESHOLD_MIN);
	SLCD_SendCmd(buffer);
}

void UpdateThresholdLevel(int level)
{
	char buffer[30];

	SetDefaultBackForegnd();		
	SLCD_SendCmd("fUARIAL14");	// set font size
	char changed = (g_SuSettings.ZC_trigger_level == level)?' ':'*';
#ifdef RUSSIAN
	sprintf(buffer,"t \"%04d \xD0\xBC\xD0\x92%c\" %d %d", 
#else
	sprintf(buffer,"t \"%04d mv%c\" %d %d", 
#endif
		ZC_THRESHOLD_COUNTS_TO_MV(level), 
		changed,
		TEST_INDENT,
		TEST_ZC_THRESHOLD_YPOS
		);
		
	SLCD_SendCmd(buffer);
		
	sprintf(buffer,"sv %d %d",SLIDER_CONTROL,level);			// set to new level
	SLCD_SendCmd(buffer);
}


/////////////////////////////////////////////
///by Alex
void UpdateThresholdLevelA(int level)
{
	char buffer[30];
/*
	SetDefaultBackForegnd();		
	SLCD_SendCmd("fUARIAL14");	// set font size
	char changed = (g_SuSettings.ZC_trigger_level == level)?' ':'*';
#ifdef RUSSIAN
	sprintf(buffer,"t \"%04d \xD0\xBC\xD0\x92%c\" %d %d", 
#else
	sprintf(buffer,"t \"%04d mv%c\" %d %d", 
#endif
		ZC_THRESHOLD_COUNTS_TO_MV(level), 
		changed,
		TEST_INDENT,
		TEST_ZC_THRESHOLD_YPOS
		);
		
	SLCD_SendCmd(buffer);
*/		
	sprintf(buffer,"sv %d %d",SLIDER_CONTROL,level);			// set to new level
	SLCD_SendCmd(buffer);
}


void SU_DrawTestData()
{
	char buffer[60];

	SetDefaultBackForegnd();		

//	SLCD_SendCmd("f24B");	// set font size
//	SLCD_SendCmd("f14x24");	// set font size

	///////////////////////////////////////// 1F RSSI ///////////////////////////////////

#ifdef USE_STRIP_CHART
	unsigned int x,y,z,peak;
	SLCD_SendCmd("f12x24");	// set font size
//	SLCD_SendCmd("f18BC");	// set font size
//	SLCD_SendCmd("f8x16");	// set font size

	x = max(g_peak_Clutter_rssi.x,g_peak_Packet_rssi.x);
	y = max(g_peak_Clutter_rssi.y,g_peak_Packet_rssi.y);
	z = max(g_peak_Clutter_rssi.z,g_peak_Packet_rssi.z);
	peak = max(x,y);
	peak = max(peak,z);
	
	x = max(g_last_Clutter_rssi.x,g_last_Packet_rssi.x);
	y = max(g_last_Clutter_rssi.y,g_last_Packet_rssi.y);
	z = max(g_last_Clutter_rssi.z,g_last_Packet_rssi.z);
	

//	sprintf(buffer,"t \"X= %3u Y= %3u Z= %3u\" 0 210",ScaleLogPower(x), ScaleLogPower(y), ScaleLogPower(z));		
//	sprintf(buffer,"t \"Peak = %3u\" 0 234", ScaleLogPower(peak));
//	sprintf(buffer,"t \"%5u %5u %5u %5u\" 0 236",x, y, z, peak);		
//	SLCD_SendCmd(buffer);

	sprintf(buffer,"t \"%5u \" 0 236",x);		
	SLCD_SendCmd(buffer);
	sprintf(buffer,"t \"%5u \" 70 236",y);		
	SLCD_SendCmd(buffer);
	sprintf(buffer,"t \"%5u \" 140 236",z);		
	SLCD_SendCmd(buffer);
	sprintf(buffer,"t \"%5u \" 210 236",peak);		
	SLCD_SendCmd(buffer);

//	sprintf(buffer,"t \"Peak = %5u   \" 0 234", peak);
//	SLCD_SendCmd(buffer);
	SU_Update_RSSI_StripChart();
	
#else
	SLCD_SendCmd("f12x24");	// set font size
	MuData* p = &g_TestModeData;
	UpdateRssiValue(p->x1_power, "delta X1", TEST_X1POWER_LEVEL_INDEX, TEST_X1POWER_XPOS, TEST_X1POWER_YPOS);
	UpdateRssiValue(p->y1_power, "delta Y1", TEST_Y1POWER_LEVEL_INDEX, TEST_Y1POWER_XPOS, TEST_Y1POWER_YPOS);
	UpdateRssiValue(p->z1_power, "delta Z1", TEST_Z1POWER_LEVEL_INDEX, TEST_Z1POWER_XPOS, TEST_Z1POWER_YPOS);
	UpdatePeakRssiValue(getPeakTestXYZ_1F(), "Pk delta XYZ 1", TEST_Z1POWER_XPOS, TEST_PEAK_DELTA_1_YPOS);
#endif
	

	///////////////////////////////////// 2F RSSI /////////////////////////////////

#ifdef USE_STRIP_CHART
#else
	UpdateRssiValue(p->x2_power, "delta X2", TEST_X2POWER_LEVEL_INDEX, TEST_X2POWER_XPOS, TEST_X2POWER_YPOS);
	UpdateRssiValue(p->y2_power, "delta Y2", TEST_Y2POWER_LEVEL_INDEX, TEST_Y2POWER_XPOS, TEST_Y2POWER_YPOS);
	UpdateRssiValue(p->z2_power, "delta Z2", TEST_Z2POWER_LEVEL_INDEX, TEST_Z2POWER_XPOS, TEST_Z2POWER_YPOS);
	UpdatePeakRssiValue(getPeakTestXYZ_2F(), "Pk delta XYZ 2", TEST_Z2POWER_XPOS, TEST_PEAK_DELTA_2_YPOS);
#endif

}


#define ZC_ONE_THRESHOLD 20
void UpdateTestZeroCrossings()
{
	char buffer[60];

#ifdef USE_STRIP_CHART
	SU_Update_ZC_StripChart(g_zero_crossings.x,g_zero_crossings.y,g_zero_crossings.z);
#endif	

	if ((g_zero_crossings.x > ZC_ONE_THRESHOLD) || (g_zero_crossings.y > ZC_ONE_THRESHOLD) || (g_zero_crossings.z > ZC_ONE_THRESHOLD))
	{
		SetForeBackGndColor(SLCD_BLACK, SLCD_YELLOW);
	}
	else 
	{
		SetDefaultBackForegnd();		
	}

		SLCD_SendCmd("f12x24");				// set font size
	#ifdef RUSSIAN
		sprintf(buffer,"t \">0 X= %2u Y= %2u Z= %2u  \" %d %d",
	#else
		sprintf(buffer,"t \"zc X= %2u Y= %2u Z= %2u  \" %d %d",
	#endif

			g_zero_crossings.x,
			g_zero_crossings.y,
			g_zero_crossings.z,
			TEST_ZC_XPOS,TEST_ZC_YPOS
			);
		SLCD_SendCmd(buffer);
}

//by Alex function for autocalibrate
void UpdateTestZeroCrossingsA(BOOL done){
	char buffer[60];

#ifdef USE_STRIP_CHART
	SU_Update_ZC_StripChart(g_zero_crossings.x,g_zero_crossings.y,g_zero_crossings.z);
#endif	

	if (done)
	{
		SetForeBackGndColor(SLCD_BLACK, SLCD_GREEN);
		
		//SLCD_SendCmd("f12x24");				// set font size
		//sprintf(buffer,"t \"      Calibrated     \" %d %d",TEST_ZC_XPOS,TEST_ZC_YPOS);
	//	SLCD_SendCmd(buffer);
	}
	else 
	{
		SetDefaultBackForegnd();
	}	
	#ifdef RUSSIAN
		sprintf(buffer,"t \">0 X= %2u Y= %2u Z= %2u  \" %d %d",
	#else
		sprintf(buffer,"t \"zc X= %2u Y= %2u Z= %2u  \" %d %d",
	#endif
			g_zero_crossings.x,
			g_zero_crossings.y,
			g_zero_crossings.z,
			TEST_ZC_XPOS,TEST_ZC_YPOS
			);
		SLCD_SendCmd(buffer);		
	}



#define PEAK_TEXT
void Draw_XYZ_Legend()
{
	char buffer[40];
	int x = 50;
	int y = 210;
	
	//SLCD_SendCmd("f24BC");	// set font size
	SLCD_SendCmd("fUARIAL14");
	SetForeBackGndColor(SLCD_RED , SLCD_BACKGROUND);
	sprintf(buffer,"t \"X\" %d %d", x, y);
	SLCD_SendCmd(buffer);

	x+= 60;
	SetForeBackGndColor(SLCD_GREEN ,SLCD_BACKGROUND);
	sprintf(buffer,"t \"Y\" %d %d", x, y);
	SLCD_SendCmd(buffer);

	x+= 60;
	SetForeBackGndColor(SLCD_YELLOW ,SLCD_BACKGROUND);
	sprintf(buffer,"t \"Z\" %d %d", x, y);
	SLCD_SendCmd(buffer);

	x+= 50;
	SetDefaultBackForegnd();
#ifdef RUSSIAN
	sprintf(buffer,"t \"\xD0\x9C\xD0\xB0\xD0\xBA\xD1\x81.\" %d %d", x, y);
#else
	sprintf(buffer,"t \"Peak\" %d %d", x, y);
#endif	
	SLCD_SendCmd(buffer);
	
}


void SU_DrawTestPage()
{
	int adder = NAVIGATION_BTN_BMP_WIDTH + 4;
	int x = SLCD_MAX_X - (adder * 5);

	SetDefaultBackForegnd();		
    SLCD_SendCmd(CLEARSCREEN);			// clear screen
#ifdef INCLUDE_MINERADIO_HEADER
	DRAW_MINERADIO_HEADER;
	DRAW_MINERADIO_FOOTER4;
#endif

	SU_DisplayStandardNavigationButtons(x+adder, NAVIGATION_BTN_YPOS, adder,TRUE);

	SetDefaultBackForegnd();
		
	SLCD_SendCmd("utf8 on");		// sets unicode
	SLCD_SendCmd("fUARIAL18");		// sets the unicode character set for the title

/////////////////////////////
////by Alex
if(g_SuSettings.enable_autocalibration)		
{

/////////Footer for autocalibration
#ifdef RUSSIAN
SLCD_SendCmd("t \"\xD0\x90\xD0\xB2\xD1\x82\xD0\xBE\xD0\xBA\xD0\xB0\xD0\xBB\xD0\xB8\xD0\xB1\xD1\x80\xD0\xBE\xD0\xB2\xD0\xBA\xD0\xB0 \" 65 50 N"); //јвтокалибровка
SLCD_WriteFooter4("\xd0\x94\xd0\xbe\xd0\xb6\xd0\xb4\xd0\xb8\xd1\x82\xd0\xb5\xd1\x81\xd1\x8c\x20\xd0\xbf\xd0\xbe\xd1\x8f\xd0\xb2\xd0\xbb\xd0\xb5\xd0\xbd\xd0\xb8\xd1\x8f",
					"\xd0\xb7\xd0\xb5\xd0\xbb\xd0\xb5\xd0\xbd\xd0\xbe\xd0\xb3\xd0\xbe\x20\xd0\xbc\xd0\xb0\xd1\x80\xd0\xba\xd0\xb5\xd1\x80\xd0\xb0\x2e.",
					"ESC = \xd0\xb2\xd1\x8b\xd1\x85\xd0\xbe\xd0\xb4",//ESC - выход
					"F1 = \xd1\x81\xd0\xb1\xd1\x80\xd0\xbe\xd1\x81\x20\xd0\xbc\xd0\xb0\xd0\xba\xd1\x81\xd0\xb8\xd0\xbc\xd1\x83\xd0\xbc\xd0\xb0"); //сброс максимума
#else
SLCD_SendCmd("t \"AutoCalibration\" 65 50 N");
SLCD_WriteFooter("Autocalibration mode. Wait green line.","Press ESC to exit","F1=Clear Peak");
#endif
}
else{
#ifdef RUSSIAN	
	SLCD_SendCmd("t \"\xd0\x9a\xd0\xb0\xd0\xbb\xd0\xb8\xd0\xb1\xd1\x80\xd0\xbe\xd0\xb2\xd0\xba\xd0\xb0\" 65 50 N");
	SLCD_WriteFooter4(
		"\xD0\x94\xD0\xBB\xD1\x8F \xD0\xB8\xD0\xB7\xD0\xBC\xD0\xB5\xD0\xBD\xD0\xB5\xD0\xBD\xD0\xB8\xD1\x8F \xD0\xB2\xD0\xBE\xD1\x81\xD0\xBF\xD0\xBE\xD0\xBB\xD1\x8C\xD0\xB7\xD1\x83\xD0\xB9\xD1\x82\xD0\xB5\xD1\x81\xD1\x8C",
		"\xD0\xBA\xD0\xBD\xD0\xBE\xD0\xBF\xD0\xBA\xD0\xB0\xD0\xBC\xD0\xB8 \xD1\x81\xD1\x82\xD1\x80\xD0\xB5\xD0\xBB\xD0\xBE\xD0\xBA \xD0\xB8\xD0\xBB\xD0\xB8 \xD1\x80\xD0\xB5\xD0\xB3\xD1\x83\xD0\xBB\xD1\x8F\xD1\x82\xD0\xBE\xD1\x80\xD0\xBE\xD0\xBC", 
		"\xD0\xBD\xD0\xB0 \xD1\x81\xD0\xB5\xD0\xBD\xD1\x81\xD0\xBE\xD1\x80\xD0\xBD\xD0\xBE\xD0\xBC \xD1\x8D\xD0\xBA\xD1\x80\xD0\xB0\xD0\xBD\xD0\xB5",
		"*=\xD0\xA1\xD0\xBE\xD1\x85\xD1\x80\xD0\xB0\xD0\xBD\xD0\xB8\xD1\x82\xD1\x8C F1=\xD0\xA1\xD0\xB1\xD1\x80\xD0\xBE\xD1\x81 \xD0\xBC\xD0\xB0\xD0\xBA\xD1\x81\xD0\xB8\xD0\xBC\xD1\x83\xD0\xBC\xD0\xB0");
#else
	SLCD_SendCmd("t \"Calibration\" 65 50 N");
	SLCD_WriteFooter("Use arrow keys to change threshold.","Press * to save","F1=Clear Peak");
#endif	
}
	SU_InitMainBatteryBar();
	SU_InitTXBatteryBar();

#ifdef USE_STRIP_CHART
	SU_Init_RSSI_StripChart();
#else
	InitRSSIBar(TEST_X1POWER_LEVEL_INDEX, TEST_X1POWERBAR_XPOS, TEST_X1POWER_YPOS+2,TEST_LEVELBAR_WIDTH,TEST_LEVELBAR_HEIGHT);
	InitRSSIBar(TEST_Y1POWER_LEVEL_INDEX, TEST_Y1POWERBAR_XPOS, TEST_Y1POWER_YPOS+2,TEST_LEVELBAR_WIDTH,TEST_LEVELBAR_HEIGHT);
	InitRSSIBar(TEST_Z1POWER_LEVEL_INDEX, TEST_Z1POWERBAR_XPOS, TEST_Z1POWER_YPOS+2,TEST_LEVELBAR_WIDTH,TEST_LEVELBAR_HEIGHT);

	InitRSSIBar(TEST_X2POWER_LEVEL_INDEX, TEST_X2POWERBAR_XPOS, TEST_X2POWER_YPOS+2,TEST_LEVELBAR_WIDTH,TEST_LEVELBAR_HEIGHT);
	InitRSSIBar(TEST_Y2POWER_LEVEL_INDEX, TEST_Y2POWERBAR_XPOS, TEST_Y2POWER_YPOS+2,TEST_LEVELBAR_WIDTH,TEST_LEVELBAR_HEIGHT);
	InitRSSIBar(TEST_Z2POWER_LEVEL_INDEX, TEST_Z2POWERBAR_XPOS, TEST_Z2POWER_YPOS+2,TEST_LEVELBAR_WIDTH,TEST_LEVELBAR_HEIGHT);
#endif

	// display threshold slider
#ifdef USE_STRIP_CHART
	SU_Init_ZC_StripChart();	
#endif
	InitThresholdSlider();
	
	Draw_XYZ_Legend();
	
	SetDefaultBackForegnd();
}



// this page listens to RSSI and Zerocrossing values on the SU_RX antenna
void SU_RunTestPage()
{
	struct button btn;
	time_t ref_second = g_seconds;
	unsigned short taskStatus;
	unsigned long long start_tick = g_tick;
	int seconds = 0;
	int level = g_SuSettings.ZC_trigger_level;
	BOOL bRedraw;
	
	SU_DataResetUnit(&g_TestModeData);	// clear data for this new session
	Reset_XYZ_Reading(&g_zero_crossings);
	ResetLastRssiReadings();
	ResetPeakRssiReadings();
	g_bTestMode = TRUE;

	set_SU_VLF_RX_Mode(SU_RX_MODE_STANDBY);	// tell the MURX module to wait until it is told to decode
	SU_DrawTestPage();
	SU_UpdateTime(SU_MAIN_TIME_XPOS,SU_MAIN_TIME_YPOS);
	SU_UpdateBatteryLevel(TRUE, FALSE);
	SU_UpdateStatus(TRUE);
	StartPingStatus();
	UpdateThresholdLevel(level);  
	
	vlf_SetMUID(0);	// make sure that all incoming messages are mapped to this id and antenna since MU packets don't always include the ID
	send_SU_RX_ZC_Trigger_Level(level+g_SuSettings.ZCx_trigger_adjust,
								level+g_SuSettings.ZCy_trigger_adjust,
								level+g_SuSettings.ZCz_trigger_adjust);
	set_SU_VLF_RX_Mode(SU_RX_MODE_CALIBRATE);	// tell the MURX module to decode regular messages until we get the response
	clear_MU_ResponseCount();				// clear the response count so we can tell if we get a response to the locate msg
    clear_SU_ZC_RX_ResponseCount();			// CLEAR THE ZC RESPONSE COUNT	
    clear_RSSI_RX_ResponseCount();			// clear the RX response count
	SLCD_SendCmd("*debounce 10");			// fast response for sliders
    
	while (1) {				// loop until we get an exit from the page
		if (ref_second != g_seconds) {
			ref_second = g_seconds;
			SU_UpdateTime(SU_MAIN_TIME_XPOS,SU_MAIN_TIME_YPOS);
			SU_UpdateBatteryLevel(FALSE, FALSE);
			SU_UpdateStatus(FALSE);
			seconds++;
			if (seconds >= 5)
			{
				SU_UpdateCOMStatus(TRUE);	// check to see if we got a response from the last ping
				StartPingStatus();		// reset and check again
//				UpdateBatteryVolts();
				seconds = 0;
			}
		}
		
		if ((g_tick - start_tick) > 330) {	// update display every 250 ms if available
			start_tick = g_tick;
			// update any new messages
			// update if we got any new messages
			if (get_MU_ResponseCount() || get_RSSI_RX_ResponseCount())	// if we got any RSSI updates from a packet or clutter response, update it
			{
				SU_DrawTestData();
				clear_MU_ResponseCount();				// clear the response count so we can tell if we get a response to the locate msg
			    clear_RSSI_RX_ResponseCount();			// clear the RX response count
				ResetLastRssiReadings();
			}
			// update if we got any zero crossings

///////////////////////////////////
//!!!!! Watch HERE!////////
/////////////////////////////////
            //UpdateTestZeroCrossings(); 			// display zero crossing data
			//UpdateThresholdLevel(level); //for debuging

			if (g_SuSettings.enable_autocalibration){
				SU_AutoCalibrate();
			}
			else
			{
			if (get_ZC_RX_ResponseCount() > 0) 
			{
				UpdateTestZeroCrossings(); 			// display zero crossing data
	    		clear_SU_ZC_RX_ResponseCount();			// CLEAR THE ZC RESPONSE COUNT	
    			Reset_XYZ_Reading(&g_zero_crossings);
			}
			Send_Clutter_request();	// request background clutter 
			Send_ZC_request();	// request background clutter 
			}

		}

		taskStatus = ProcessBackgroundTasks();

		btn = GetUserAction(taskStatus);
		if (btn.index != 0) {
			bRedraw = FALSE;
			switch (btn.index) {
				case SU_BTN_EXIT:
					set_SU_VLF_RX_Mode(SU_RX_MODE_STANDBY);	// tell the MURX module to wait until it is told to decode
					g_bTestMode = FALSE;
					SLCD_SendCmd("*debounce 100");	// return debounce speed to default for buttons
					send_SU_RX_ZC_Trigger();		// RESTORE TRIGGER LEVEL TO WHATEVER IS IN THE GLOBAL SETTINGS VARIABLE
					return;
					break;		
				case SU_BTN_UP:
					level += 4;
					bRedraw = TRUE;
					break;
				case SU_BTN_DOWN:
					level -= 4;
					bRedraw = TRUE;
					break;
				case SU_BTN_ENTER:
					g_SuSettings.ZC_trigger_level = level;	// UPDATE THE GLOBAL SETTINGS VALUE
					save_current_settings();				// save to non-volatile memory
					bRedraw = TRUE;
					break;
				case SLIDER_CONTROL:
					level = btn.state;
					bRedraw = TRUE;
					break;
				case SU_BTN_F1:
					set_SU_VLF_RX_Mode(SU_RX_MODE_CALIBRATE);	// tell the MURX module to send RSSI and zero crossings 
					clear_MU_ResponseCount();				// clear the response count so we can tell if we get a response to the locate msg
				    clear_SU_ZC_RX_ResponseCount();			// CLEAR THE ZC RESPONSE COUNT	
				    clear_RSSI_RX_ResponseCount();			// clear the RX response count
				    ResetPeakRssiReadings();	 				// clear peak power for new locate.
				    ResetLastRssiReadings();	 				// clear peak power for new locate.
					break;
					
				case SU_BTN_UPDOWN:
					SU_RunSettingsPage();
					SU_DrawTestPage();
					SU_UpdateGeneralStatus(TRUE,TRUE);
					UpdateThresholdLevel(level);
					break;
					
#if (KEYBOARD_BUTTONS == 16)
				case SU_BTN_SETTINGS:
					SU_RunSettingsPage();
					SU_DrawTestPage();
					SU_UpdateGeneralStatus(TRUE,TRUE);
					UpdateThresholdLevel(level);
					break;
				case SU_BTN_TEST:
					set_SU_VLF_RX_Mode(SU_RX_MODE_CALIBRATE);	// tell the MURX module to send RSSI and zero crossings 
					clear_MU_ResponseCount();				// clear the response count so we can tell if we get a response to the locate msg
				    clear_SU_ZC_RX_ResponseCount();			// CLEAR THE ZC RESPONSE COUNT	
				    clear_RSSI_RX_ResponseCount();			// clear the RX response count
				    clear_PeakTestXYZ();	 				// clear peak power for new locate.
					break;
#endif						
				default:
					break;
			}
			if (bRedraw == TRUE) {
				if (level > ZC_THRESHOLD_MAX) level = ZC_THRESHOLD_MAX;
				if (level < ZC_THRESHOLD_MIN) level = ZC_THRESHOLD_MIN;
				UpdateThresholdLevel(level);
				send_SU_RX_ZC_Trigger_Level(level+g_SuSettings.ZCx_trigger_adjust,
											level+g_SuSettings.ZCy_trigger_adjust,
											level+g_SuSettings.ZCz_trigger_adjust);
				
			}

		}
	}

	g_bTestMode = FALSE;

}

BOOL SU_AutoCalibrate(){
    int level = g_SuSettings.ZC_trigger_level;
	BOOL bRedraw;
    BOOL bCalibrated = FALSE;
    
    int auto_test = g_zero_crossings.x+g_zero_crossings.y+g_zero_crossings.z;
    		if (auto_test > 27) 
			{
                level += 20;             //add by Alex 8/24/18
				bRedraw = TRUE;         //add by Alex 8/24/18      
                //g_SuSettings.ZC_trigger_level = level;	// UPDATE THE GLOBAL SETTINGS VALUE
				//save_current_settings();				// save to non-volatile memory                
			}
            if (auto_test < 17)                        //add by Alex 8/24/18
            {                           //add by Alex 8/24/18
                level -= 20;             //add by Alex 8/24/18
                bRedraw = TRUE;         //add by Alex 8/24/18
                //g_SuSettings.ZC_trigger_level = level;	// UPDATE THE GLOBAL SETTINGS VALUE
				//save_current_settings();				// save to non-volatile memory
            }                           //add by Alex 8/24/18
    		if((auto_test<=40)&&(auto_test>0))			//calibration status for users
    		{
        		bCalibrated = TRUE;
    		}  
	
            UpdateTestZeroCrossingsA(bCalibrated); 	// display zero crossing data
			//UpdateTestZeroCrossings(bCalibrated); 	// display zero crossing data

	    	clear_SU_ZC_RX_ResponseCount();			// CLEAR THE ZC RESPONSE COUNT	
    		Reset_XYZ_Reading(&g_zero_crossings);
            
            if (bRedraw == TRUE) {
				if (level > ZC_THRESHOLD_MAX) level = ZC_THRESHOLD_MAX;
				if (level < ZC_THRESHOLD_MIN) level = ZC_THRESHOLD_MIN;

 				g_SuSettings.ZC_trigger_level = level;	// UPDATE THE GLOBAL SETTINGS VALUE
				save_current_settings();				// save to non-volatile memory  
				
				//UpdateThresholdLevelA(level); //for debuging
				UpdateThresholdLevel(level);
	
				send_SU_RX_ZC_Trigger_Level(level+g_SuSettings.ZCx_trigger_adjust,
											level+g_SuSettings.ZCy_trigger_adjust,
											level+g_SuSettings.ZCz_trigger_adjust);
            }
            bRedraw = FALSE;
            
			Send_Clutter_request();	// request background clutter 
			Send_ZC_request();	// request background zc 
            	           
    return bCalibrated;
}

