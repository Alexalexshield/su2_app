#include "system.h"
#include "DEE Emulation 16-bit.h"

Config g_Config;

char load_non_volotile_config()
{
	return load_config(&g_Config);
}


char load_config(Config* pConfig)
{
	char error = 0;
	unsigned char*  p = (unsigned char*)pConfig;
	int count;
	unsigned int DEEaddr = CONFIG_EEPROM_OFFSET;

    dataEEFlags.val = 0;		// CLEAR STATUS FLAGS

    char c = DataEERead(DEEaddr++);

    // When a record is saved, first byte is written as version marker to indicate
    // that a valid record was saved.  This change has been made to so old EEPROM contents 
	// will get overwritten.  
    if(c == CONFIG_EEPROM_VERSION)
    {
        for ( count = 0; count < sizeof(Config); count++ ) {
            *p++ = DataEERead(DEEaddr++);
            if (dataEEFlags.val) {
	            error = 1;
	            break;
	        }
	    }  
    }
    else
    	error = 1;
    	
    return error;
}	



char save_current_config_settings()
{
	return save_config(&g_Config);
}


// saves the config settings to flash eeprom  
char save_config(Config* pConfig)
{
	int c;
	int len = sizeof(Config);
    unsigned char* p = (unsigned char*)pConfig;
 	unsigned int DEEaddr = CONFIG_EEPROM_OFFSET;
   
    dataEEFlags.val = 0;		// CLEAR STATUS FLAGS

    DataEEWrite((unsigned char)CONFIG_EEPROM_VERSION,DEEaddr++);

	if (dataEEFlags.val == 0 ) {
	    for ( c = 0; c < len; c++ )
    	{
        	DataEEWrite(*p++,DEEaddr++);
        	if (dataEEFlags.val)
        		break;
    	}
 	}
    
	return dataEEFlags.val;
}



void default_config(Config* pConfig)
{
	pConfig->id = DEFAULT_ID;
}	

	
