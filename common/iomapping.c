/*****************************************************************************
 *
 *****************************************************************************
 * FileName:        iomapping.c
  *****************************************************************************/

#include  "iomapping.h"


/*****************************************************************************
 * Function: lockIO
 *
 * Preconditions: None.
 *
 * Overview: This executes the necessary process to set the IOLOCK bit to lock
 * I/O mapping from being modified.
 *
 * Input: None.
 *
 * Output: None.
 *
 *****************************************************************************/
void lockIO(){

asm volatile ("mov #OSCCON,w1 \n"
				"mov #0x46, w2 \n"
				"mov #0x57, w3 \n"
				"mov.b w2,[w1] \n"
				"mov.b w3,[w1] \n"
				"bset OSCCON, #6");


}

/*****************************************************************************
 * Function: unlockIO
 *
 * Preconditions: None.
 *
 * Overview: This executes the necessary process to clear the IOLOCK bit to 
 * allow I/O mapping to be modified.
 *
 * Input: None.
 *
 * Output: None.
 *
 *****************************************************************************/
void unlockIO(){

asm volatile ("mov #OSCCON,w1 \n"
				"mov #0x46, w2 \n"
				"mov #0x57, w3 \n"
				"mov.b w2,[w1] \n"
				"mov.b w3,[w1] \n"
				"bclr OSCCON, #6");

}

