/**
 *	Copyright (c) 2016, Mack Informatcion Systems, In.  All rights reserved.
 *
 *	OS entry and task creation.
 *
 *	Author: Dan Rockwell
 */

#include "dn_common.h"
#include "dn_system.h"
#include "dn_fs.h"
#include "dn_gpio.h"
#include "dn_adc.h"
#include "dn_api_param.h" 
#include "dn_exe_hdr.h" 

// application configuration
#include "Ver.h"

// App includes
#include "debug.h"
#include "flash.h"
#include "config.h"
#include "calibration.h"
#include "gpio.h"
#include "spi.h"
#include "ltc2984.h"
#include "battery.h"
#include "time.h"
#include "alarm.h"
#include "report.h"
//#include "security.h"
#include "radio.h"
//#include "fwupdate.h"
//#include "display.h"
#include "moteapp.h"


//=========================== prototypes ======================================

void InhibitRadio();
void EnableRadio();


/**
 *	Entry point for the application.
 */
int p2_init( void )
{
	InitDebug();			// done

	InitFlash();			// done

	InitConfiguration();	        // done

	InitCalibration();		// done

	InitGPIO();			// done

	InitSPI();			// done

	InitLTC2984();			// done

	InitBattery();			// done

	InitTime();			// done

	InitAlarm();			// done

	InitReport();			// done

	//InitSecurity();		// later
	InitRadio();

	//InitFwUpdate();		// later

	//InitDisplay();		// later

	RunMoteApp();

	return 0;
}


void InhibitRadio() 
{
	// this is a stub
}


void EnableRadio() 
{
	// this is a stub
}


/**
 *	A kernel header is a set of bytes prepended to the actual binary image of this
 *	application. Thus header is needed for your application to start running.
 */
DN_CREATE_EXE_HDR( DN_VENDOR_ID_NOT_SET,
				   DN_APP_ID_NOT_SET,
				   VER_MAJOR,
				   VER_MINOR,
				   VER_PATCH,
				   VER_BUILD );

