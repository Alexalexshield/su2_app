#ifndef __ADC_H
#define __ADC_H

#include "system.h"

struct AD10SCAN 
{
	int iScanSelect;
	int iChanSelect;
	int iAvg;
	int iMin;
	int iMax;	
};

#define BATTVOLT_AN_PIN  	AD1PCFGbits.PCFG12			//voltage input on AN12
#define BATTVOLT_ADC_CHAN	12
#define BATTVOLT_TRIS		TRISBbits.TRISB12			



#define ADMAX				1024
#define AVCC                6600
//#define ADBITS_POWER		10
#define LOWBATTAVCC         5000
#define HIGHBATTAVCC        6700
//#define BATTERY_PERCENTAGE(mv)  ((long)(mv*100l)/AVCC)

#if (BOARD_REV == 2)
#define BATT_DIVISOR (ADMAX/2)
#else
#define BATT_DIVISOR ADMAX
#endif

#ifdef SU_RX
#define X1F_TRIS			_TRISB9			
#define X1F_AN_PIN			AD1PCFGbits.PCFG9			//voltage input on AN9
#define X1F_ADC_CHAN		9

#define X2F_TRIS			_TRISB8			
#define X2F_AN_PIN		  	AD1PCFGbits.PCFG8			//voltage input on AN8
#define X2F_ADC_CHAN		8

#define Y1F_TRIS			_TRISB1			
#define Y1F_AN_PIN		  	AD1PCFGbits.PCFG1			//voltage input on AN1
#define Y1F_ADC_CHAN		1

#define Y2F_TRIS			_TRISB0			
#define Y2F_AN_PIN		  	AD1PCFGbits.PCFG0			//voltage input on AN0
#define Y2F_ADC_CHAN		0

#define Z1F_TRIS			_TRISB10			
#define Z1F_AN_PIN		  	AD1PCFGbits.PCFG10			//voltage input on AN10
#define Z1F_ADC_CHAN		10

#define Z2F_TRIS			_TRISB4			
#define Z2F_AN_PIN		  	AD1PCFGbits.PCFG4			//voltage input on AN4
#define Z2F_ADC_CHAN		4

#endif

extern void ADCInit();
extern unsigned int readBattMVolts();
int ADC10ScanChannel(struct AD10SCAN* p); //ADC scan

int ADCBlockMin();
int ADCBlockMax();
int ADCBlockAvg();


int readAtoD(unsigned char channel, unsigned int usecdelay);


#endif // __ADC_H
