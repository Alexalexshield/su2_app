#ifndef _ADXL345_ACCEL_H
#define _ADXL345_ACCEL_H
#include "timer.h"

#define CALIBRATION_TIMES 4

/*
Table 6. Current Consumption vs. Data Rate
(TA = 25°C, VS = 2.5 V, VDD I/O = 1.8 V)
Output Data Rate (Hz) 	Bandwidth (Hz)	Rate Code	IDD (µA)
3200						1600			1111		145			
1600						800				1110		100
800							400				1101		145
400							200				1100		145
200							100				1011		145
100							50				1010		145
50							25				1001		100
25							12.5			1000		65
12.5						6.25			0111		55
6.25						3.125			0110		40
*/
#define FREQUENCY_RATE_CODE	   0x08
#define ACCEL_SAMPLE_FREQUENCY 12.5
#define ACCEL_UPDATEPERIOD ((int)(TICKSPERSECOND/ACCEL_SAMPLE_FREQUENCY))
#define ACCEL_UPDATEPERIOD_MS (ACCEL_UPDATEPERIOD)			
#define ACCEL_UPDATEPERIODS_IN_FIVE_MINUTES ((1000l*5*60l)/ACCEL_UPDATEPERIOD)


extern int g_AccelerometerEnableCountdown;
int isAccelerometerEnabled();
void SetAccelerometerEnableCountdown( int countdown);
void DecrementAccelerometerEnableCountdown(); 
float GetLastFilteredMaxAcceleration();
void ClearLastFilteredMaxAcceleration();

typedef struct ACCEL_READINGS {
	int16 x;
	int16 y;
	int16 z;
} AccelReading;

struct ACCEL_TABLE {
	AccelReading ref;
	AccelReading min;
	AccelReading max;
#ifdef _WIN32
	double CalcRefVector() { 
		double x = ref.x;
		double y = ref.y;
		double z = ref.z;
		return sqrt(x*x + y*y + z*z);
	};
	double CalcMinVector() { 
		double x = min.x;
		double y = min.y;
		double z = min.z;
		return sqrt(x*x + y*y + z*z);
	};
	double CalcMaxVector() { 
		double x = max.x;
		double y = max.y;
		double z = max.z;
		return sqrt(x*x + y*y + z*z);
	};
#endif
};

extern struct ACCEL_TABLE g_accel_readings;
void Set_adxl345_Accelerometer_Reference();
void Read_adxl345_Accelerometer_MinMax();
int GetPeak_adxl345_Accelerometer_Delta();
extern U8 GetMobility();
void UpdateMobility_MovingSquare();
float GetLastFilteredMaxAcceleration();

char adxl345_read_register(char address);
void adxl345_write_register(char address, char value);
void adxl345_defaults(void);
void adxl345_IO_setup(void);
BYTE adxl345_spi_send_byte(BYTE data_out);


#define ADXL345_CS_TRIS			_TRISB9		//Port B.9
#define ADXL345_CS				_LATB9		//Port B.9

#define ADXL345_SCK_TRIS		_TRISB11	//Port B.11
#define ADXL345_SCK				_LATB11		//Port B.11

#define ADXL345_DO_TRIS			_TRISB12	//Port B.12
#define ADXL345_DO				_LATB12		//Port B.12

#define ADXL345_DI_TRIS			_TRISB15	//Port B.15
#define ADXL345_DI				_RB15		//Port B.15

//#define ADXL345_INT1_TRIS		TRISBbits.TRISB5	//Port B.5
//#define ADXL345_INT1			(PORTBbits.RB5)		

//#define	ADXL345_INT2_TRIS		TRISGbits.TRISG9	//Port G.9
//#define	ADXL345_INT2			(PORTGbits.RG9)		

#define ADXL345_SELECT()		(ADXL345_CS = 0)
#define ADXL345_UNSELECT()		(ADXL345_CS = 1)

#define ADXL345_CLOCK_HIGH()	ADXL345_SCK = 1
#define ADXL345_CLOCK_LOW()		ADXL345_SCK = 0

#define ADXL345_SPICON1             SPI1CON1  			// The main SPI control register
#define ADXL345_SPISTAT             SPI1STAT 			// The SPI status register
#define ADXL345_SPIBUF              SPI1BUF  			// The SPI Buffer
#define ADXL345_SPISTAT_RBF         SPI1STATbits.SPIRBF	// The receive buffer full bit in the SPI status register
#define ADXL345_SPICON1bits         SPI1CON1bits		// The bitwise define for the SPI control register (i.e. _____bits)
#define ADXL345_SPISTATbits         SPI1STATbits		// The bitwise define for the SPI status register (i.e. _____bits)
#define ADXL345_SPIENABLE           SPI1STATbits.SPIEN	// The enable bit for the SPI module
#define ADXL345_SPIIF 				SPI1IF
#define ADXL345_SPIIE 				SPI1IE

#define ADXL345_SPIINTEN 			IEC0 
#define ADXL345_SPIINTFLG 			IFS0
#define ADXL345_SPIINTENbits 		IEC0bits
#define ADXL345_SPIINTFLGbits 		IFS0bits


//*******************************************************
//						ADXL345 Definitions
//*******************************************************
#define ADXL345_READ	0x80

//ADXL Register Map
#define	ADXL345_DEVID			0x00	//Device ID Register
#define ADXL345_THRESH_TAP		0x1D	//Tap Threshold
#define	ADXL345_OFSX			0x1E	//X-axis offset
#define	ADXL345_OFSY			0x1F	//Y-axis offset
#define	ADXL345_OFSZ			0x20	//Z-axis offset
#define	ADXL345_DUR				0x21	//Tap Duration
#define	ADXL345_Latent			0x22	//Tap latency
#define	ADXL345_Window			0x23	//Tap window
#define	ADXL345_THRESH_ACT		0x24	//Activity Threshold
#define	ADXL345_THRESH_INACT	0x25	//Inactivity Threshold
#define	ADXL345_TIME_INACT		0x26	//Inactivity Time
#define	ADXL345_ACT_INACT_CTL	0x27	//Axis enable control for activity and inactivity detection
#define	ADXL345_THRESH_FF		0x28	//free-fall threshold
#define	ADXL345_TIME_FF			0x29	//Free-Fall Time
#define	ADXL345_TAP_AXES		0x2A	//Axis control for tap/double tap
#define ADXL345_ACT_TAP_STATUS	0x2B	//Source of tap/double tap
#define	ADXL345_BW_RATE			0x2C	//Data rate and power mode control
#define ADXL345_POWER_CTL		0x2D	//Power Control Register
#define	ADXL345_INT_ENABLE		0x2E	//Interrupt Enable Control
#define	ADXL345_INT_MAP			0x2F	//Interrupt Mapping Control
#define	ADXL345_INT_SOURCE		0x30	//Source of interrupts
#define	ADXL345_DATA_FORMAT		0x31	//Data format control
#define ADXL345_DATAX0			0x32	//X-Axis Data 0
#define ADXL345_DATAX1			0x33	//X-Axis Data 1
#define ADXL345_DATAY0			0x34	//Y-Axis Data 0
#define ADXL345_DATAY1			0x35	//Y-Axis Data 1
#define ADXL345_DATAZ0			0x36	//Z-Axis Data 0
#define ADXL345_DATAZ1			0x37	//Z-Axis Data 1
#define	ADXL345_FIFO_CTL		0x38	//FIFO control
#define	ADXL345_FIFO_STATUS		0x39	//FIFO status

//Power Control Register Bits
#define ADXL345_WU_0		(1<<0)	//Wake Up Mode - Bit 0
#define	ADXL345_WU_1		(1<<1)	//Wake Up mode - Bit 1
#define ADXL345_SLEEP		(1<<2)	//Sleep Mode
#define	ADXL345_MEASURE		(1<<3)	//Measurement Mode
#define ADXL345_AUTO_SLP	(1<<4)	//Auto Sleep Mode bit
#define ADXL345_LINK		(1<<5)	//Link bit

//Interrupt Enable/Interrupt Map/Interrupt Source Register Bits
#define	ADXL345_OVERRUN		(1<<0)
#define	ADXL345_WATERMARK	(1<<1)
#define ADXL345_FREE_FALL	(1<<2)
#define	ADXL345_INACTIVITY	(1<<3)
#define	ADXL345_ACTIVITY	(1<<4)
#define ADXL345_DOUBLE_TAP	(1<<5)
#define	ADXL345_SINGLE_TAP	(1<<6)
#define	ADXL345_DATA_READY	(1<<7)

//Data Format Bits
#define ADXL345_RANGE_0		(1<<0)
#define	ADXL345_RANGE_1		(1<<1)
#define ADXL345_JUSTIFY		(1<<2)
#define	ADXL345_FULL_RES	(1<<3)

#define	ADXL345_INT_INVERT	(1<<5)
#define	ADXL345_SPI			(1<<6)
#define	ADXL345_SELF_TEST	(1<<7)


#endif	//#ifndef _ADXL345_ACCEL_H
