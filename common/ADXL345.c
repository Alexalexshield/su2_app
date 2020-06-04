#include "system.h"
#include "pc_cmds.h"
#include "ADXL345.h"
#include <stdlib.h>
#include <stdio.h>
#include "delays.h"

#if PROCESSOR_MIPS == 16	
#define DELAY1USEC Nop(); Nop(); Nop(); Nop(); Nop(); Nop(); Nop(); Nop(); Nop(); Nop();
#else
#define DELAY1USEC Nop(); 
#endif

struct ACCEL_TABLE g_accel_readings;
static float filtered_max_acceleration = 0.0;
static int filtered_max_acceleration_count = 0;
int g_AccelerometerEnableCountdown = 0;




int isAccelerometerEnabled() { return g_AccelerometerEnableCountdown; };

void SetAccelerometerEnableCountdown( int countdown)
{
	if (g_AccelerometerEnableCountdown == 0)  {
	//	Set_adxl345_Accelerometer_Reference();		// take a reference sample
	//	filtered_max_acceleration_count = 0;		// reset filter if not already running
	}
	
	ADXL345_UNSELECT();
	adxl345_defaults();

	g_AccelerometerEnableCountdown = 	((1000l*countdown)/ACCEL_UPDATEPERIOD_MS);
}


void DecrementAccelerometerEnableCountdown() 
{ 	
	if (g_AccelerometerEnableCountdown) {
		g_AccelerometerEnableCountdown--;
		if (g_AccelerometerEnableCountdown == 0) {
			filtered_max_acceleration_count = 0;
//			Set_adxl345_Accelerometer_Reference();
			adxl345_write_register(ADXL345_POWER_CTL, ~ADXL345_MEASURE);		//Put the ADXL345 into standby mode
			ADXL345_UNSELECT();
		}
	}
}


//#define USE_SPI 1

void test_adx1345();
void adxl345_read_xyz(unsigned int data[3]);

/*For SPI, either 3- or 4-wire configuration is possible. 
Clearing the SPI bit in the DATA_FORMAT register (Address 0x31) selects 4-wire mode, 
whereas setting the SPI bit selects 3-wire mode. 
The maximum SPI clock speed is 5 MHz with 100 pF maximum loading, 
and the timing scheme follows clock polarity (CPOL) = 1 and clock phase (CPHA) = 1.
CS is the serial port enable line and is controlled by the SPI master. 
This line must go low at the start of a transmission and high at the end of a transmission, 
SCLK is the serial port clock and is supplied by the SPI master. 
It is stopped high when CS is high during a period of no transmission. 
SDI and SDO are the serial data input and output, respectively.
*/


void adxl345_IO_setup(void)
{

	ADXL345_UNSELECT();
	ADXL345_CS_TRIS = OUTPUT;	
	ADXL345_SCK_TRIS = OUTPUT;		
	ADXL345_DI_TRIS	= INPUT;
	ADXL345_DO_TRIS = OUTPUT;
//	ADXL345_INT1_TRIS = INPUT;
//	ADXL345_INT2_TRIS = INPUT;

#ifdef USE_SPI	
//	ADXL345_INT1_TRIS = INPUT;
//	ADXL345_INT2_TRIS = INPUT;
	ADXL345_SPISTAT = 0x0000;       // power on state 
	ADXL345_SPICON1 = 0;			// SET TO RESET STATE
    ADXL345_SPICON1bits.CKP = 1;	// clock polarity: 1 = Idle state for clock is a high level; active state is a low level
    ADXL345_SPICON1bits.CKE = 1;	// SPIx Clock Edge Select bit 1 = Idle state for clock is a high level; active state is a low level
    ADXL345_SPICON1bits.MSTEN = 1;	// This spi is the master
	ADXL345_SPICON1bits.MODE16 = 0;	// communication is 8 bits wide
	ADXL345_SPICON1bits.SMP = 0;	// 1 = Input data sampled at end of data output time

/*
bit 4-2 SPRE2:SPRE0: Secondary Prescale bits (Master mode)
111 = Secondary prescale 1:1
110 = Secondary prescale 2:1
101 = 3:1
100 = 4:1
011 = 5:1
010 = 6:1
001 = 7:1
000 = Secondary prescale 8:1
bit 1-0 PPRE1:PPRE0: Primary Prescale bits (Master mode)
11 = Primary prescale 1:1
10 = Primary prescale 4:1
01 = Primary prescale 16:1
00 = Primary prescale 64:1
*/

// Set up the accelerometer for 2 MHZ (running at 16 MIPS)
	ADXL345_SPICON1bits.SPRE = 6;	// 2:1
	ADXL345_SPICON1bits.PPRE = 2;	// 4:1 
	
	
/*In Enhanced Master mode, the SRMPT bit
(SPIxSTAT<7>) may erroneously become set for
several clock cycles in the middle of a FIFO transfer,
indicating that the shift register is empty when it is
not. This happens when both SPI clock prescalers
are set to values other than their maximum
(SPIxCON<4:2> ? 000 and SPIxCON<1:0> ? 00).
Work around
Set SISEL2:SISEL0 (SPIxSTAT<4:2>) to ‘101’.
This configures the module to generate an SPI
event interrupt whenever the last bit is shifted out
of the shift register. When the SPIxIF flag becomes
set, the shift register is empty.
*/	
	ADXL345_SPISTATbits.SISEL = 0b101;
    ADXL345_SPIENABLE = 1;          // enable synchronous serial port
    DelayUsecs(100);				//todo - make sure this is needed - copied from example code

#else	// manual mode
	ADXL345_CLOCK_HIGH();		// make sure clock is high by default
	ADXL345_UNSELECT();			// chip select is deactivated by default
#endif // USE_SPI
}


void adxl345_defaults(void)
{
	unsigned char byte=0;
	unsigned char wbyte;
	int i;
	char buffer[40];
		
	ADXL345_UNSELECT();
	adxl345_IO_setup();	//Setup the AVR I/O for data logging


	if (g_UnitFlags.DebugAccel) {
		PortPutCRStr("adxl345_defaults()",PC_PORT, 1);
	}
	
//	adxl345_write_register(ADXL345_DATA_FORMAT, ADXL345_INT_INVERT);	//Invert the interrupt bit (0 on active interrupt)
//	adxl345_write_register(ADXL345_INT_ENABLE, ADXL345_DATA_READY);	//Activate the 'Data Ready' Interrupt
//	adxl345_write_register(ADXL345_INT_MAP, 0<<7);					//Set the Data Ready Interrupt to the INT1 pin
	adxl345_write_register(ADXL345_BW_RATE, 0x08);					//Set Output Rate to 12.5 Hz
//	adxl345_write_register(ADXL345_BW_RATE, 0x0A);					//Set Output Rate to 100 Hz
//	adxl345_write_register(ADXL345_BW_RATE, 0x0f);					//Set Output Rate to 1600 Hz
//	adxl345_write_register(ADXL345_POWER_CTL, ADXL345_MEASURE);		//Put the Accelerometer into measurement mode	
	//waits for data format register to respond correctly
	wbyte = ADXL345_FULL_RES;
//	wbyte = 0;
	i=0;
	do
	{
		adxl345_write_register(ADXL345_DATA_FORMAT, wbyte);
		DelayMsecs(1);
		byte = adxl345_read_register(ADXL345_DATA_FORMAT);
	}
	while ((byte != wbyte) && (i++ < 200));
	if (g_UnitFlags.DebugAccel) {
		sprintf(buffer, "Format is: %x %s", byte, (byte == wbyte)?"ok":"incorrect");
		PortPutCRStr(buffer,PC_PORT,1);
	}
	
	//waits for power register to respond correctly
//	wbyte = ADXL345_LINK | ADXL345_MEASURE;
	wbyte = ADXL345_MEASURE;
	adxl345_write_register(ADXL345_POWER_CTL, wbyte);
	byte = adxl345_read_register(ADXL345_POWER_CTL);
	DelayMsecs(1);

	if (g_UnitFlags.DebugAccel) {
		sprintf(buffer, "Power is: %x %s", byte, (byte == wbyte)?"ok":"incorrect");
		PortPutCRStr(buffer,PC_PORT,1);
	}

	
	wbyte = 0;
	adxl345_write_register(ADXL345_FIFO_CTL, wbyte);
	DelayMsecs(1);
	byte = adxl345_read_register(ADXL345_FIFO_CTL);
	if (g_UnitFlags.DebugAccel) {
		sprintf(buffer, "FIFO is: %x %s", byte, (byte == wbyte)?"ok":"incorrect");
		PortPutCRStr(buffer,PC_PORT,1);
	}
	
	DelayMsecs(10);		// wait for 1.1 ms + sample rate delay = 11.1 ms at 10 hz


}



/////////////////////////////////// SPI ROUTINES //////////////////////////////////
#ifdef USE_SPI	

/*void adxl345_defaults(void)
{

//	adxl345_write_register(ADXL345_DATA_FORMAT, ADXL345_SPI);			//set SPI to 3-WIRE MODE)
	adxl345_write_register(ADXL345_DATA_FORMAT, ADXL345_INT_INVERT);	//Invert the interrupt bit (0 on active interrupt)
	adxl345_write_register(ADXL345_INT_ENABLE, ADXL345_DATA_READY);	//Activate the 'Data Ready' Interrupt
	adxl345_write_register(ADXL345_INT_MAP, 0<<7);					//Set the Data Ready Interrupt to the INT1 pin
	adxl345_write_register(ADXL345_BW_RATE, 0x0A);					//Set Output Rate to 100 Hz
	adxl345_write_register(ADXL345_POWER_CTL, ADXL345_MEASURE);		//Put the Accelerometer into measurement mode	
}
*/

BYTE adxl345_spi_send_byte(BYTE data_out)
{
    ADXL345_SPIBUF = data_out;          // write byte to SPIBUF register 
    while(!ADXL345_SPISTAT_RBF);        //Wait until cycle complete
    return ADXL345_SPIBUF;    			//Return with byte read
}


char adxl345_read_register(char register_address){
	char value;
	ADXL345_SELECT();	//Lower CS pin.
	value = adxl345_spi_send_byte(register_address | 0x80);
	ADXL345_UNSELECT();
	return value;
}

void adxl345_write_register(char register_address, char register_value)
{
	ADXL345_SELECT();	//Lower CS pin.
	adxl345_spi_send_byte(register_address);
	adxl345_spi_send_byte(register_value);
	ADXL345_UNSELECT();
}


// To read or write multiple bytes in a single transmission, the multiple-byte bit, 
//located after the R/W bit in the first byte transfer (MB in to ), must be set. 
// After the register addressing and the first byte of data, 
//each subsequent set of clock pulses (eight clock pulses) 
//causes the ADXL345 to point to the next register for a read or write. 
//This shifting continues until the clock pulses cease and Figure 5Figure 7CS is deasserted. 
//To perform reads or writes on different, nonsequential registers, 
//CS must be deasserted between transmissions and the new register must be addressed separately.
void adxl345_read_xyz(unsigned int data[3])
{
	int i;
	
	ADXL345_SELECT();	//Lower CS pin.
	adxl345_spi_send_byte(0xF2);

	for (i=0;i<3;i++) 
	{
		data[i] = (adxl345_spi_send_byte(0)) << 8;
		data[i] += adxl345_spi_send_byte(0);
	}
	ADXL345_UNSELECT();
}


#else		///////////////////////////// MANUAL METHOD /////////////////////////////////////



char adxl345_read_register(char register_address){
	char read_address=0x80 | register_address;
	char register_value=0;
	int bit_pos;
	
	ADXL345_CLOCK_HIGH();
	ADXL345_SELECT();	//Lower CS pin.
	
	for(bit_pos=7; bit_pos>=0; bit_pos--){
		ADXL345_CLOCK_LOW();
		
		if((read_address & (1<<bit_pos))==(1<<bit_pos))
			ADXL345_DO = 1;
		else 
			ADXL345_DO = 0;
		DELAY1USEC;
		
		ADXL345_CLOCK_HIGH();
		DELAY1USEC;
	}
	
	for(bit_pos=7; bit_pos>=0; bit_pos--){
		ADXL345_CLOCK_LOW();
		DELAY1USEC;
		
		ADXL345_CLOCK_HIGH();
		DELAY1USEC;
		
		if(ADXL345_DI)
			register_value |= (1<<bit_pos);
		else 
			register_value &= ~(1<<bit_pos);		
	}
	
	ADXL345_UNSELECT();
	
	return register_value;
}

void adxl345_write_register(char register_address, char register_value)
{
	ADXL345_CLOCK_HIGH();
	ADXL345_SELECT();	//Lower CS pin.
	int bit_pos;
	
	for(bit_pos=7; bit_pos>=0; bit_pos--){
		ADXL345_CLOCK_LOW();
		
		if (register_address & (1<<bit_pos))
			ADXL345_DO = 1;
		else 
			ADXL345_DO = 0;
		DELAY1USEC;
		
		ADXL345_CLOCK_HIGH();
		DELAY1USEC;
	}
	
	for (bit_pos=7; bit_pos>=0; bit_pos--) {
		ADXL345_CLOCK_LOW();
		
		if (register_value & (1<<bit_pos))
			ADXL345_DO = 1;
		else 
			ADXL345_DO = 0;
		DELAY1USEC;
		
		ADXL345_CLOCK_HIGH();
		DELAY1USEC;
	}
	
	ADXL345_UNSELECT();
}

// To read or write multiple bytes in a single transmission, the multiple-byte bit, 
//located after the R/W bit in the first byte transfer (MB in to ), must be set. 
// After the register addressing and the first byte of data, 
//each subsequent set of clock pulses (eight clock pulses) 
//causes the ADXL345 to point to the next register for a read or write. 
//This shifting continues until the clock pulses cease and Figure 5Figure 7CS is deasserted. 
//To perform reads or writes on different, nonsequential registers, 
//CS must be deasserted between transmissions and the new register must be addressed separately.
void adxl345_read_xyz(unsigned int data[3])
{
//	char read_address=0x80 | ADXL345_DATAX0 | 0x40;
	char read_address=0xf2;	// 2ND HIGHEST BIT SET PUTS IT INTO A MULTI-BYTE READ MODE
	int bit_pos;
	int i;
	unsigned int first,second;
	
	ADXL345_CLOCK_HIGH();
	ADXL345_SELECT();	//Lower CS pin.
	
	for(bit_pos=7; bit_pos>=0; bit_pos--){
		ADXL345_CLOCK_LOW();
		
		if((read_address & (1<<bit_pos))==(1<<bit_pos))
			ADXL345_DO = 1;
		else 
			ADXL345_DO = 0;
		DELAY1USEC;
		
		ADXL345_CLOCK_HIGH();
		DELAY1USEC;
	}
	
	for (i=0;i<3;i++) {
		first = 0;
		for(bit_pos=7; bit_pos>=0; bit_pos--){
			ADXL345_CLOCK_LOW();
			DELAY1USEC;
			
			ADXL345_CLOCK_HIGH();
			DELAY1USEC;
			
			if(ADXL345_DI)
				first |= (1<<bit_pos);
			else 
				first &= ~(1<<bit_pos);		
		}
		
		second = 0;
		for(bit_pos=7; bit_pos>=0; bit_pos--){
			ADXL345_CLOCK_LOW();
			DELAY1USEC;
			
			ADXL345_CLOCK_HIGH();
			DELAY1USEC;			
			if(ADXL345_DI)
				second |= (1<<bit_pos);
			else 
				second &= ~(1<<bit_pos);		
		}
		data[i] = (second<<8) | first;
	}
	
	ADXL345_UNSELECT();
	

}

#endif	//#ifdef MANUAL METHOD



void Set_adxl345_Accelerometer_Reference_donotuse()
{
	long x,y,z;
//	char low_byte=0;
//	char high_byte=0;
//	char enabled_interrupts=0, interrupt_map=0, interrupt_source=0;
	int count;
	unsigned int data[3];
	int interrupt_source;
//test_adx1345();

//	ADXL345_UNSELECT();
//	adxl345_defaults();

	
	x=0;
	y=0;
	z=0;
	for (count = 0;count < CALIBRATION_TIMES; count++) 
	{
		do {
			//Read the interrupt source register to clear any interrupts
			interrupt_source=adxl345_read_register(ADXL345_INT_SOURCE);
		}
		while ((interrupt_source & ADXL345_DATA_READY) !=ADXL345_DATA_READY);

//		while (ADXL345_INT1) 
		{
			//Read the interrupt source register to clear any interrupts
//			interrupt_source=adxl345_read_register(ADXL345_INT_SOURCE);
//			if((interrupt_source & ADXL345_DATA_READY)==ADXL345_DATA_READY)
			{
				adxl345_read_xyz(data);
				x += (int)data[0];
				y += (int)data[1];
				z += (int)data[2];
//				high_byte = adxl345_read_register(ADXL345_DATAX1);
//				low_byte = adxl345_read_register(ADXL345_DATAX0);
//				x += ((high_byte << 8) | low_byte);
				
//				high_byte = adxl345_read_register(ADXL345_DATAY1);
//				low_byte = adxl345_read_register(ADXL345_DATAY0);
//				y += ((high_byte << 8) | low_byte);
	
//				high_byte = adxl345_read_register(ADXL345_DATAZ1);
//				low_byte = adxl345_read_register(ADXL345_DATAZ0);
//				z += ((high_byte << 8) | low_byte);
				DelayMsecs(1);
			}
		}
	}
		//Put the ADXL345 into standby mode
//	adxl345_write_register(ADXL345_POWER_CTL, ~ADXL345_MEASURE);
//  ADXL345_UNSELECT();
	
	x = x/CALIBRATION_TIMES;
	y = y/CALIBRATION_TIMES;
	z = z/CALIBRATION_TIMES;
	
	g_accel_readings.min.x = x;
	g_accel_readings.max.x = x;
	g_accel_readings.ref.x = x;
	
	g_accel_readings.min.y = y;
	g_accel_readings.max.y = y;
	g_accel_readings.ref.y = y;
	
	g_accel_readings.min.z = z;
	g_accel_readings.max.z = z;
	g_accel_readings.ref.z = z;
}	

#define ONE_SECOND_SAMPLES 10
void Read_adxl345_Accelerometer_MinMax()
{
	int x,y,z;
	unsigned int data[3];
	int interrupt_source;
	static int count = 0;

//	ADXL345_UNSELECT();
//	adxl345_defaults();


//	do {
///		//Read the interrupt source register to clear any interrupts
		interrupt_source=adxl345_read_register(ADXL345_INT_SOURCE);
//	}
//	while ((interrupt_source & ADXL345_DATA_READY) !=ADXL345_DATA_READY);
	if ((interrupt_source & ADXL345_DATA_READY) ==ADXL345_DATA_READY) { 

		adxl345_read_xyz(data);
		x = (int)data[0];
		y = (int)data[1];
		z = (int)data[2];
			
		if (count == 0) {			// set a reference every 1 second
			g_accel_readings.min.x = x;
			g_accel_readings.max.x = x;
			g_accel_readings.ref.x = x;
			
			g_accel_readings.min.y = y;
			g_accel_readings.max.y = y;
			g_accel_readings.ref.y = y;
			
			g_accel_readings.min.z = z;
			g_accel_readings.max.z = z;
			g_accel_readings.ref.z = z;	
			count = ONE_SECOND_SAMPLES;	
		} 
		else {
			g_accel_readings.min.x = min(g_accel_readings.min.x , x);
			g_accel_readings.min.y = min(g_accel_readings.min.y , y);
			g_accel_readings.min.z = min(g_accel_readings.min.z , z);
			g_accel_readings.max.x = max(g_accel_readings.max.x , x);
			g_accel_readings.max.y = max(g_accel_readings.max.y , y);
			g_accel_readings.max.z = max(g_accel_readings.max.z , z);
			
			UpdateMobility_MovingSquare();
			count--;
		}	
	}
//	adxl345_write_register(ADXL345_POWER_CTL, ~ADXL345_MEASURE);		//Put the ADXL345 into standby mode
//	ADXL345_UNSELECT();

}


int GetPeak_adxl345_Accelerometer_Delta()
{
	int maxg = 0;
	int peak;
	
	maxg = max(abs(g_accel_readings.ref.x - g_accel_readings.min.x),abs(g_accel_readings.ref.x - g_accel_readings.max.x));
	peak = max(abs(g_accel_readings.ref.y - g_accel_readings.min.y),abs(g_accel_readings.ref.y - g_accel_readings.max.y));
	if (peak > maxg) maxg = peak;
	peak = max(abs(g_accel_readings.ref.z - g_accel_readings.min.z),abs(g_accel_readings.ref.z - g_accel_readings.max.z));
	if (peak > maxg) maxg = peak;

	return maxg;
}	


void UpdateMobility_MovingSquare()
{
	int filtlen = ACCEL_UPDATEPERIODS_IN_FIVE_MINUTES;
	float dlast = filtered_max_acceleration;
	float dval = GetPeak_adxl345_Accelerometer_Delta();
	dval = dval*dval;	// square the acceleration to give emphasis to the strongest signals
	
	// perform a peakhold on the value;
	if (filtered_max_acceleration_count == 0) {	// if first time then just use this average
		filtered_max_acceleration = dval;
		filtered_max_acceleration_count++;
	}
	else if (dval > filtered_max_acceleration) {
		filtered_max_acceleration = dval;
	}
	else {	
		if (filtered_max_acceleration_count < filtlen) {
			filtered_max_acceleration = ((dlast * (filtered_max_acceleration_count-1)) + dval)/filtered_max_acceleration_count;
			filtered_max_acceleration_count++;
		}
		else {
			filtered_max_acceleration = ((dlast * (filtlen-1)) + dval)/filtlen;
		}
	}
}


#define MOBILITY_RANGE 300
#define MOBILITY_STEPS 15
#define MOBILITY_NOISE_FLOOR 25
float GetLastFilteredMaxAcceleration() { return filtered_max_acceleration; };
void ClearLastFilteredMaxAcceleration() { filtered_max_acceleration = 0.0; };

U8 GetMobility()
{
	U8 value;
	
	if (filtered_max_acceleration > (MOBILITY_RANGE+MOBILITY_NOISE_FLOOR))
		value = 15;
	else if (filtered_max_acceleration < MOBILITY_NOISE_FLOOR)
		value = 0;
	else
		value = (U8)(floor(((filtered_max_acceleration-MOBILITY_NOISE_FLOOR)*MOBILITY_STEPS)/MOBILITY_RANGE));	

	return value;
}

/*
void test_adx1345()
{
	unsigned int data[3];
	int i;
	char buffer[40];
		
	ADXL345_UNSELECT();
	adxl345_defaults();

	while(1)
	{
	
		adxl345_read_xyz(data);
		
		for(i=0; i<3; i++)
		{
			sprintf(buffer, "%04x ", data[i]);
			PortPutStr(buffer,PC_PORT,1);
		}
		
		for(i=0; i<3; i++)
		{
			sprintf(buffer, "%5d ", (int)data[i]);
			PortPutStr(buffer,PC_PORT,1);
		}
		
		PortPutStr("\r\n",PC_PORT,1);
		DelayMsecs(500);
	}
	
}
*/


