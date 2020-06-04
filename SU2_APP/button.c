#include "system.h"
#include "button.h"

#define BUTTON_COLUMNS 4

#define COL0_TRIS		(TRISEbits.TRISE0)
#define	COL0_IO			(PORTEbits.RE0)

#define COL1_TRIS		(TRISEbits.TRISE1)
#define	COL1_IO			(PORTEbits.RE1)

#define COL2_TRIS		(TRISEbits.TRISE2)
#define	COL2_IO			(PORTEbits.RE2)

#define COL3_TRIS		(TRISEbits.TRISE3)
#define	COL3_IO			(PORTEbits.RE3)

#define BUTTON_ROWS 3
#define ROW0_TRIS		(TRISEbits.TRISE4)
#define	ROW0_IO			(PORTEbits.RE4)
#define	ROW0_LATCH		(_LATE4)

#define ROW1_TRIS		(TRISEbits.TRISE5)
#define	ROW1_IO			(PORTEbits.RE5)
#define	ROW1_LATCH		(_LATE5)

#define ROW2_TRIS		(TRISEbits.TRISE6)
#define	ROW2_IO			(PORTEbits.RE6)
#define	ROW2_LATCH		(_LATE6)

// local prototypes
void read_button_col(char state, int row, int col);
void setbuttonstate(char start, unsigned short shifter);

// local global

unsigned short buttonstate;

unsigned short GetButtonState() { return buttonstate; };

#define setbuttonstate(STATE, SHIFT) if (STATE == 0) 	buttonstate |= SHIFT;	else buttonstate &= ~SHIFT;


void ButtonInit()
{
	
	// clear all buttons
	buttonstate = 0;
		
	COL0_TRIS = INPUT;	
	COL1_TRIS = INPUT;
	COL2_TRIS = INPUT;
	COL3_TRIS = INPUT;
	ROW0_LATCH = 0;		// ALWAYS SELECT ROW 0 IN CASE WE WANT TO ADD INTERRUPTS ON THIS ROW
	ROW0_TRIS = OUTPUT;
	ROW1_TRIS = INPUT;
	ROW2_TRIS = INPUT;
}

// THIS FUNCTION ASSUMES IT IS BEING CALLED EVERY n MILLISECONDS - FOR THE DEBOUNCE TO WORK
// returns with the current button state - multiple buttons may be pressed at once
unsigned short ProcessButtonEvents()
{
	ROW0_LATCH = 0;
	ROW0_TRIS = OUTPUT;
	ROW1_TRIS = INPUT;
	ROW2_TRIS = INPUT;
	DelayMsecs(1);
	setbuttonstate(COL0_IO, 0x0001);
	setbuttonstate(COL1_IO, 0x0002);
	setbuttonstate(COL2_IO, 0x0004);
	setbuttonstate(COL3_IO, 0x0008);
	
	ROW0_TRIS = INPUT;
	ROW1_LATCH = 0;
	ROW1_TRIS = OUTPUT;
	ROW2_TRIS = INPUT;
	DelayMsecs(1);
	setbuttonstate(COL0_IO, 0x0010);
	setbuttonstate(COL1_IO, 0x0020);
	setbuttonstate(COL2_IO, 0x0040);
	setbuttonstate(COL3_IO, 0x0080);

	ROW0_TRIS = INPUT;
	ROW1_TRIS = INPUT;
	ROW2_LATCH = 0;
	ROW2_TRIS = OUTPUT;
	DelayMsecs(1);
	setbuttonstate(COL0_IO, 0x0100);
	setbuttonstate(COL1_IO, 0x0200);
	setbuttonstate(COL2_IO, 0x0400);
	setbuttonstate(COL3_IO, 0x0800);
	
	ROW0_LATCH = 0;
	ROW0_TRIS = OUTPUT;
	ROW1_TRIS = INPUT;
	ROW2_TRIS = INPUT;
	return buttonstate;
}



