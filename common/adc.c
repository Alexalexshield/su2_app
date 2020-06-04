/*****************************************************************************
 *  ADC - ROUTINES TO READ THE A/D
 
*****************************************************************************/

#include "system.h"

/*****************************************************************************
To perform an A/D conversion:
1. Configure the A/D module:
	a) Configure port pins as analog inputs and/or select band gap reference inputs (AD1PCFGL<15:0> and AD1PCFGH<1:0>).
	b) Select voltage reference source to match expected range on analog inputs (AD1CON2<15:13>).
	c) Select the analog conversion clock to match desired data rate with processor clock (AD1CON3<7:0>).
	d) Select the appropriate sample/conversion sequence (AD1CON1<7:5> and AD1CON3<12:8>).
	e) Select how conversion results are presented in the buffer (AD1CON1<9:8>).
	f) Select interrupt rate (AD1CON2<5:2>).
	g) Turn on A/D module (AD1CON1<15>).
2. Configure A/D interrupt (if required):
	a) Clear the AD1IF bit.
	b) Select A/D interrupt priority.
 *****************************************************************************/
//ADC channels numbers

extern unsigned char g_TestMode;



void ADCInit(){
	AD1CON1 = 0x80E4;				//Turn on, auto sample start, auto-convert
	AD1CON2 = 0;					//AVdd, AVss, int every conversion, MUXA only
	AD1CON3 = 0x1F05;				//31 Tad auto-sample, Tad = 5*Tcy
    
	AD1CHS = BATTVOLT_ADC_CHAN;

	BATTVOLT_TRIS = INPUT;		// set the corresponding digital pins to input
	BATTVOLT_AN_PIN = 0;		//Disable digital input and set to analog input
	
#ifdef SU_RX
#if (BOARD_REV == 1)	
	X1F_TRIS = INPUT;			// set the corresponding digital pin to input
	X1F_AN_PIN = 0;				// Disable digital input and set to analog input
	X2F_TRIS = INPUT;			// set the corresponding digital pin to input
	X2F_AN_PIN = 0;				// Disable digital input and set to analog input

	Y1F_TRIS = INPUT;			// set the corresponding digital pin to input
	Y1F_AN_PIN = 0;				// Disable digital input and set to analog input
	Y2F_TRIS = INPUT;			// set the corresponding digital pin to input
	Y2F_AN_PIN = 0;				// Disable digital input and set to analog input

	Z1F_TRIS = INPUT;			// set the corresponding digital pin to input
	Z1F_AN_PIN = 0;				// Disable digital input and set to analog input
	Z2F_TRIS = INPUT;			// set the corresponding digital pin to input
	Z2F_AN_PIN = 0;				// Disable digital input and set to analog input
#endif	
#endif

	AD1CSSL = 0;				// No scanned inputs
}



unsigned int readBattMVolts()
{
	struct AD10SCAN scan;
	long value;
	scan.iScanSelect = BATTVOLT_ADC_CHAN;
	scan.iChanSelect = BATTVOLT_ADC_CHAN;
	if (ADC10ScanChannel(&scan))
	{  //scan AN2, negative input VR-
		value = scan.iAvg;
		value = (value *AVCC)/BATT_DIVISOR;		
	}
	else
		value = (HIGHBATTAVCC + LOWBATTAVCC)/2;
		
	if (value <= LOWBATTAVCC)
		g_SysError.LowBatt = 1;
	else if (value >= HIGHBATTAVCC)
		g_SysError.HighBatt = 1;
	return (unsigned int)value;
}	

int readAtoD(unsigned char channel, unsigned int usecdelay)
{
	AD1CON1 = 0x80E4;				//Turn on, auto sample start, auto-convert
	AD1CON2 = 0;					//AVdd, AVss, int every conversion, MUXA only
	AD1CON3 = 0x1F05;				//31 Tad auto-sample, Tad = 5*Tcy
	AD1CSSL = 0;					// No scanned inputs
	AD1CHS = channel;				// select Analog input channel
	AD1CON1bits.SAMP = 1; 			// Start sampling
	DelayUsecs(usecdelay);
	AD1CON1bits.DONE = 1; 			// Start the conversion
	while (!AD1CON1bits.DONE);		// wait for conversion to complete
	return ADC1BUF0;
}


//////////////////ADC scan (16x)/////////////////////////////
//first variable sets channel to be scanned, second sets mux to that channel
int ADC10ScanChannel(struct AD10SCAN* p)
{
	static char b_inRoutine = FALSE;
	if (b_inRoutine)
		return FALSE;
	b_inRoutine = TRUE;
	//AD1CON1 = 0x00E4;		//ADC off,unsigned integer output, autoconvert trigger, sample after last convert
	AD1CON1bits.ADON = 0;	// Turn off A/D
	AD1CON1bits.ADSIDL = 0;	// Continue module operation in idle mode
	AD1CON1bits.FORM = 0;	// 00 = Integer (0000 00dd dddd dddd)
	AD1CON1bits.SSRC = 7;	// Internal counter ends sampling and starts conversion (auto-convert)	
	AD1CON1bits.ASAM = 1;	// Sampling begins immediately after last conversion completes = SAMP is autoset
	
	//AD1CON2 = 0x043C;		//AVDD & AVSS references,scan mode for ch0,interupt after 16converts,16 buffers, use MUXA
	AD1CON2bits.VCFG = 0;	// AVDD & AVSS references
	AD1CON2bits.CSCNA = 1;	// Scan inputs for CH0 + s/h input for mux a
	AD1CON2bits.SMPI = 0x0f;	//interupt after 16 conversions
	AD1CON2bits.BUFM = 0;	// READ AS 16 bit word
	AD1CON2bits.ALTS = 0; 	// Always use MUX A input multiplexer settings
	
	//AD1CON3 = 0x111E;		//AD clock from system, 17 AD sample time, 16 samples/msec for FCY = 4 MHz
	AD1CON3bits.ADRC = 0;	// Clock derived from system clock
//	AD1CON3bits.SAMC = 0x11;	// 17 Auto-Sample Time bits
//	AD1CON3bits.ADCS = 0x1E;	// A/D Conversion Clock Select = 30 
	AD1CON3bits.SAMC = 0x1;	// 17 Auto-Sample Time bits (sample window size)
	AD1CON3bits.ADCS = 0x0;	// A/D Conversion Clock Select = 30 (lower is less time between samples)

	AD1CHSbits.CH0NA = 0;		// 0 = Channel 0 negative input is VRbit
	AD1CSSL	= 1<<p->iScanSelect;	//the channels to be included for sequential scanning - set x bit for ANx 
	AD1CHSbits.CH0SA = p->iChanSelect;	//selects the input channels to be connected to the S/H amplifier- input is VR-,CH0+ input is ANxx - 
	
	AD1CON1bits.ADON = 1; 	//start conversion (16 consecutive readings)
	IFS0bits.AD1IF = 0;
	DelayUsecs(10);			// delay for 10 usec before sampling
 	AD1CON1bits.ASAM = 1;	// auto start sampling for 31Tad
	
	while(!IFS0bits.AD1IF);	//wait for ADC block conversion complete
	AD1CON1bits.ASAM = 0;	// stop sampling
	
	AD1CON1bits.ADON = 0;   // Turn off the A/D converter
	AD1CON1bits.SAMP = 0;   // Hold sample	
	
	IFS0bits.AD1IF = 0;		//reset flag

	p->iAvg = ADCBlockAvg();		//average block of readings 
	p->iMax = ADCBlockMax();		//average block of readings 
	p->iMin = ADCBlockMin();		//average block of readings 
	b_inRoutine = FALSE;
	return TRUE;
}


//////////////////Average ADC 16 samples////////////////////////////
int ADCBlockAvg()
{
	// skip the first read (ADC1BUF0) and use the fourth one twice
	long temp = 	(
			ADC1BUF1 + ADC1BUF3 + ADC1BUF2 + ADC1BUF3 + 
			ADC1BUF4 + ADC1BUF5 + ADC1BUF6 + ADC1BUF7 + 
			ADC1BUF8 + ADC1BUF9 + ADC1BUFA + ADC1BUFB + 
			ADC1BUFC + ADC1BUFD + ADC1BUFE + ADC1BUFF);
	return (int)(temp >> 4);
}

int ADCBlockMax()
{
	int peak;	// skip over 1st sample ADC1BUF0
	peak = ADC1BUF1;
	peak = max(peak, ADC1BUF2);
	peak = max(peak, ADC1BUF3);
	peak = max(peak, ADC1BUF4);
	peak = max(peak, ADC1BUF5);
	peak = max(peak, ADC1BUF6);
	peak = max(peak, ADC1BUF7);
	peak = max(peak, ADC1BUF8);
	peak = max(peak, ADC1BUF9);
	peak = max(peak, ADC1BUFA);
	peak = max(peak, ADC1BUFB);
	peak = max(peak, ADC1BUFC);
	peak = max(peak, ADC1BUFD);
	peak = max(peak, ADC1BUFE);
	peak = max(peak, ADC1BUFF);
	return peak;
}

int ADCBlockMin()
{
	int iMin;	// skip over 1st sample ADC1BUF0
	iMin = ADC1BUF1;
	iMin = min(iMin, ADC1BUF2);
	iMin = min(iMin, ADC1BUF3);
	iMin = min(iMin, ADC1BUF4);
	iMin = min(iMin, ADC1BUF5);
	iMin = min(iMin, ADC1BUF6);
	iMin = min(iMin, ADC1BUF7);
	iMin = min(iMin, ADC1BUF8);
	iMin = min(iMin, ADC1BUF9);
	iMin = min(iMin, ADC1BUFA);
	iMin = min(iMin, ADC1BUFB);
	iMin = min(iMin, ADC1BUFC);
	iMin = min(iMin, ADC1BUFD);
	iMin = min(iMin, ADC1BUFE);
	iMin = min(iMin, ADC1BUFF);
	return iMin;
}



