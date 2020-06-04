#ifndef __CONFIG_H
#define __CONFIG_H
	
// The following number is used to check if the struct is already programmed - change it
// whenever the structure format changes
#include "su_struct.h"
#include <math.h> 

#define CONFIG_SOURCE_RAM 1
#define CONFIG_SOURCE_FLASH 0
#define CONFIG_EEPROM_VERSION 6


extern Config g_Config;

extern char load_non_volotile_config();
extern char save_current_config();
extern char load_config(Config* pConfig);
extern char save_config(Config* pConfig);
extern void default_config(Config* pConfig);

#endif // __CONFIG_H
