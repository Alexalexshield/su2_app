#ifndef __DELAY_H
#define __DELAY_H

#include "system.h"

extern void DelaySecs(unsigned int secs);
extern void DelayMsecs(unsigned int msecs); 
extern void DelayUsecs(unsigned int usecs); 
extern void DelayulUSecs(unsigned long usecs);
// warning - value must be less than 4000 to not overflow when runing at 16 MIPS
//void DelayTMR4Usecs(unsigned int usecs);
//void Delay_LowPower_TMR4Usecs(unsigned int usecs);

#endif // __DELAY_H
