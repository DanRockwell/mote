/******************************************************************************
*	Copyright (c) 2016, Mack Informatcion Systems, In.  All rights reserved.
*
*	Application used to manage temperature reporting.
*
*	Author: Dan Rockwell
*/

//=========================== includes =====================================

// C includes
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// SDK includes
#include "dn_system.h"
#include "dn_fs.h"
#include "dn_api_param.h"
#include "dn_exe_hdr.h"
#include "dn_common.h"
#include "dn_system.h"

// App includes
#include "debug.h"
#include "alarm.h"
#include "ltc2984.h"
#include "config.h"
#include "report.h"


//=========================== defines and enums ===============================

#define TASK_APP_REPORT_STK_SIZE	256
#define TASK_APP_REPORT_PRIORITY	53
#define TASK_APP_REPORT_NAME		"TASK_REPORT"


//=========================== module parameters ======================================

static LTC2984Data_t	mTemperatureReadings;
static ErrorMap_t	mErrorMap[kMaxNumberOfTempertures];
static OS_EVENT		*mDataLock;


//=========================== prototypes ======================================

static void LockData(void);
static void UnlockData(void);


/**
*	Initialize the alarm manager
*/
void InitReport( void )
{
	DbgPrint( DBG_LEVEL_INFO, "STATUS. Initializing Report Manager\r\n" );

	// Create shared data lock
	mDataLock = OSSemCreate( 1 );
}


/**
*	Run alarm manager to process any alarms and temperatures
*/
void RunReportManager( void )
{
	DbgPrint( DBG_LEVEL_INFO, "STATUS. Running Report Manager\r\n" );
	 
	LockData();

	// Get temperature readings
	GetCurrentTemperatures( &mTemperatureReadings );

	// Get active alarms
	GetActiveAlarms( &mErrorMap[0] );

	// Send reports
	for( int channel = 0; channel < kMaxNumberOfTempertures; channel++ )
	{
            // Get configuration
            ChannelConfiguration_t channelConfig;
            GetChannelConfiguration( channel, (unsigned char *)&channelConfig );

            if( 1 == channelConfig.channelEnable )
            {
                if( kAlarmId_None == mErrorMap[channel].error )
		{
                    SendTemperatures( channel, mTemperatureReadings.channel[channel].temperature );
		}
		else
		{
                    SendAlarm( channel, mErrorMap[channel].error, mTemperatureReadings.channel[channel].temperature );
		}
            }
	}

	UnlockData();
}


static void LockData( void )
{
    unsigned char osErr;
    OSSemPend(mDataLock, 0, &osErr);
}


static void UnlockData( void )
{
    OSSemPost(mDataLock);
}
