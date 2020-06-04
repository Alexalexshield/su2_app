/************************************************************************
*  definititions for iomap functions                                    *

************************************************************************/
#ifndef IOMAPPING_H	
#define IOMAPPING_H

#include <p24fxxxx.h>

//PPS Outputs
#define NULL_IO		0
#define C1OUT_IO	1
#define C2OUT_IO	2
#define U1TX_IO		3
#define U1RTS_IO	4
#define U2TX_IO		5
#define U2RTS_IO	6
#define SDO1_IO		7
#define SCK1OUT_IO	8
#define SS1OUT_IO	9
#define SDO2_IO		10
#define SCK2OUT_IO	11
#define SS2OUT_IO	12
#define OC1_IO		18
#define OC2_IO		19
#define OC3_IO		20
#define OC4_IO		21
#define OC5_IO		22
#define U3TX_IO		28
#define U4TX_IO		30
#define SD03_IO		32
#define SCK3OUT_IO 	33
#define SS3OUT		34
#define C3OUT_IO    36


extern void lockIO();
extern void unlockIO();

#endif
