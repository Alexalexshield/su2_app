/*****************************************************************************
 *
 * UART Driver for PIC24.
 *
 *****************************************************************************
 * FileName:        uart2.c
 * Dependencies:    system.h
 * Processor:       PIC33
 * Compiler:       	MPLAB C30
 * Linker:          MPLAB LINK30
 * Company:         Microchip Technology Incorporated
 *
  *****************************************************************************/
#include "system.h"
#include "uart1.h"
#include "putfunctions.h"

#define TX1FIFOSIZE 1024	// to PC
#define RX1FIFOSIZE 128		// from PC
// UART1 IOs

#ifdef MU_APP
#define UART1_TX_TRIS   TRISBbits.TRISB4
#define UART1_RX_TRIS   TRISBbits.TRISB13
//#define USEINTDRIVENTX 1		// DON'T USE INTERRUPT DRIVEN TX SINCE IT IS ONLY USED FOR DEBUGGING AND WOULD OTHERWISE REQUIRE RAM

#else	
// SU + MODULES use this configuration
#define UART1_TX_TRIS   TRISFbits.TRISF3
#define UART1_RX_TRIS   TRISFbits.TRISF2
//#define USEINTDRIVENTX 1		// DON'T USE INTERRUPT DRIVEN TX SINCE IT IS ONLY USED FOR DEBUGGING AND WOULD OTHERWISE REQUIRE RAM
#endif



// comment out next line to use poll-driven RX or uncomment to use int-driven RX
#define USEINTDRIVENRX 1

#ifdef USEINTDRIVENTX
#include "queue.h"

unsigned char tx1_fifo[TX1FIFOSIZE];
struct CQUEUE txq1;
#endif

#ifdef USEINTDRIVENRX
#include "queue.h"

unsigned char rx1_fifo[RX1FIFOSIZE];
struct CQUEUE rxq1;
#endif
/*****************************************************************************
 * Function: UART1Init
 *
 * Precondition: None.
 *
 * Overview: Setup UART2 module.
 *
 * Input: None.
 *
 * Output: None.
 *
 *****************************************************************************/
void UART1Init(unsigned char brgval,unsigned char bregh)
{
	U1MODEbits.UARTEN = 0; 	// Disable UART
	U1STAbits.UTXEN = 0;	// Disable UART Tx

	UART1_TX_TRIS = 0;				// Set Tx to output
	UART1_RX_TRIS = 1;				// Set RX to input
	
	U1MODE = 0x0808;  	//TX disabled, U1RTS flow control mode,TX and Rx pins enabled,wakeup,no loopback,no autobaud
						//RX idle is 1,high speed BRG, 8 bits no parity, 1 stop

	U1MODEbits.BRGH = bregh; 	// Speed mode
	U1BRG = brgval; 			// BAUD Rate Setting 

  	IFS0bits.U1RXIF = 0;	// reset RX flag
  	IFS0bits.U1TXIF = 0;	// clear the TX flag
  	IEC0bits.U1TXIE = 0;	// Disable TX interupt for now 
  	
#ifdef USEINTDRIVENRX
	IEC0bits.U1RXIE = 1; 	// Enable UART Rx interrupt
	U1STAbits.URXISEL = 0;	// Interupt after 1 character is received
	CQueue_init(&rxq1,rx1_fifo,RX1FIFOSIZE);
#endif

#ifdef USEINTDRIVENTX
	U1STAbits.UTXISEL1=0; 	//  Interrupt after 1 character transmitted
	U1STAbits.UTXISEL0=0;
	CQueue_init(&txq1,tx1_fifo,TX1FIFOSIZE);
#endif

	U1MODEbits.UARTEN = 1; 	// Enable UART
	U1STAbits.UTXEN = 1;	// Enable UART Tx

}

/*****************************************************************************
 * Function: UART1PutCh
 *
 * Precondition: UART2Init must be called before.
 *
 * Overview: Wait for free UART transmission buffer and send a byte.
 *
 * Input: Byte to be sent.
 *
 * Output: 0 if successful else 1 for error
 *
 *****************************************************************************/
char  UART1PutCh(unsigned char Ch){
	char error = 0;
#ifdef USEINTDRIVENTX
	char i;	
	if (CQueue_isEmpty(&txq1) && (U1STAbits.UTXBF==0) && (IFS0bits.U1TXIF == 0)) { // if the TX butter is empty then send it otherwise queue it
		U1TXREG = Ch;
	}
	else {
		if (CQueue_isFull(&txq1)) {
			IEC0bits.U1TXIE = 1; // characters in queue so enable Uart 1 TX interupt
			for (i=0;i<10;i++) {
				if (CQueue_isFull(&txq1))
					DelayUsecs(50);
				else
					break;
			}
		}
		if (CQueue_isFull(&txq1)) {
			g_SysError.TX1_Overun = 1;
			error=1;
		}
		else {
			IEC0bits.U1TXIE = 0; // disable TX interupts while messing with the queue
			CQueue_enqueue(&txq1, Ch);
			IEC0bits.U1TXIE = 1; // characters in queue so enable Uart 2 TX interupt
		}
		IEC0bits.U1TXIE = 1; // characters in queue so enable Uart 1 TX interupt
	}
	IEC0bits.U1TXIE = 1; // make sure the TX interrupt is enable in case there are any characters in the queue
#else
    // wait for empty buffer  
   	while(!U1STAbits.TRMT);	//Continue while transmit shift register not empty
	U1TXREG = Ch;			//Transmit data

#endif
	return error;
}

/*****************************************************************************
 * Function: UART1ChReady
 *
 * Precondition: UART2Init must be called before.
 *
 * Overview: Check if there's a new byte in UART reception buffer.
 *
 * Input: None.
 *
 * Output: Zero if there's no new data received.
 *
 *****************************************************************************/
char UART1ChReady()
{
#ifdef USEINTDRIVENRX
	if (CQueue_isEmpty(&rxq1))
		return 0;
	else
		return 1;
#else
    if(IFS0bits.U1RXIF == 1)
        return 1;
    else
	    return 0;
#endif    
}

/*****************************************************************************
 * Function: UART1GetCh
 *
 * Precondition: UART2Init must be called before.
 *
 * Overview: Wait for a byte.
 *
 * Input: None.
 *
 * Output: Byte received.
 *
 *****************************************************************************/
unsigned char UART1GetCh(){
	unsigned char temp = 0xff;
#ifdef USEINTDRIVENRX
	IEC0bits.U1RXIE = 0; // Disable UART Rx interrupt while fetching character
	if (!CQueue_isEmpty(&rxq1)) {
		CQueue_dequeue(&rxq1,&temp);
	}
	IEC0bits.U1RXIE = 1; // Re-Enable UART Rx interrupt
#else // polled mode
    while(IFS0bits.U1RXIF == 0);
    temp = U1RXREG;
    IFS0bits.U1RXIF = 0;
#endif	
    return temp;
}

// returns the remaining size in the Tx buffer if using a buffer otherwise a very large integer size
extern unsigned int UART1GetTxBufSize()
{
#ifdef USEINTDRIVENTX
	return	CQueue_freesize(&txq1);
#else
	return 30000;
#endif
}	

unsigned char c1;
#ifdef USEINTDRIVENTX
void __attribute__((__interrupt__, no_auto_psv)) _U1TXInterrupt(void)
{
	if (CQueue_isEmpty(&txq1))
		IEC0bits.U1TXIE = 0;	// nothing in the queue so shut off the interupt
	else {
		CQueue_dequeue(&txq1,&c1); // if there is a character in the queue then put it into the TX buffer
		U1TXREG = c1;
	}	
	IFS0bits.U1TXIF = 0; // clear the TX flag
}
#endif

#ifdef USEINTDRIVENRX
void __attribute__((__interrupt__, no_auto_psv)) _U1RXInterrupt(void)
{
	unsigned char rbyte; 

	
	while (U1STAbits.URXDA == 1) {	// there can be multiple characters in the buffer
		if (U1STAbits.OERR) {
			g_SysError.U1_OverunError |= U1STAbits.OERR;
			U1STAbits.OERR = 0;	// clear any overrun errors
		}	
		rbyte = U1RXREG;
		if (!CQueue_isFull(&rxq1)) {
			g_SysError.U1_ParityError |= U1STAbits.PERR;
		  	g_SysError.U1_FramingError |= U1STAbits.FERR;
	  		
			CQueue_enqueue(&rxq1,rbyte);					
		}
		else {
			g_SysError.RX1_Overun = 1;
		}
	}
	IFS0bits.U1RXIF = 0; // clear the RX flag

}
#endif


char UART1Rx_push(unsigned char ch)
{
	char error = 0;
#ifdef USEINTDRIVENRX
	IEC0bits.U1RXIE = 0; // Disable UART Rx interrupt while fetching character
	if (!CQueue_isFull(&rxq1)) {
		CQueue_enqueue(&rxq1,ch);
	}
	else {
		g_SysError.RX1_Overun = 1;
		error = 1;
	}	
	IEC0bits.U1RXIE = 1; // Re-Enable UART Rx interrupt
#endif		
	return error;
}	

int GetUART1RXpCent() 
{
#ifdef USEINTDRIVENRX
	if (rxq1.len==0)
		return 0;
	else
		return ((rxq1.peakhold * 100)/rxq1.len);
#endif		
}
int GetUART1TXpCent()
{
#ifdef USEINTDRIVENTX
	if (txq1.len==0)
		return 0;
	else
		return ((txq1.peakhold * 100)/txq1.len);
#else
	return 0;
#endif		
}

void ResetUART1peaks()
{
#ifdef USEINTDRIVENRX
	rxq1.peakhold=0;
#endif
	
#ifdef USEINTDRIVENTX
	txq1.peakhold=0;
#endif	
}

/*****************************************************************************
 * EOF
 *****************************************************************************/
