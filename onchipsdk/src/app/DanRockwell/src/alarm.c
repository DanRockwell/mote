/******************************************************************************
*	Copyright (c) 2016, Mack Informatcion Systems, In.  All rights reserved.
*
*	Application used to manage alarms.
*
*	Author: Dan Rockwell
*/

//=========================== includes =====================================

// C includes
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

// SDK includes
#include "dn_system.h"
#include "dn_fs.h"
#include "dn_api_param.h"
#include "dn_exe_hdr.h"
#include "dn_common.h"
#include "dn_system.h"

// App includes
#include "app_common.h"
#include "debug.h"
#include "alarm.h"
#include "ltc2984.h"
#include "ltc2984_cfg.h"
#include "config.h"


//=========================== defines and enums ===============================

const char *OverTemperatureError  = "Alarm Over Temperature";
const char *UnderTemperatureError = "Alarm Under Temperature";


//=========================== structs =================================

typedef struct
{
    int errorCount[kMaxNumberOfTempertures];
    LTC2984Data_t temperatureReadings;
    AdcReading_t previousTemperatureReadings[kMaxNumberOfTempertures];
} AlaramData_t;


//=========================== module parameters ======================================

static ErrorMap_t       mErrorMap[kMaxNumberOfTempertures];
static AlaramData_t     mAlarmData;
static OS_EVENT		*mDataLock;
static const float	mDeltaTThreshold = 0.25;	
static bool             mDeltaTThresholdReached = false;


//=========================== prototypes ======================================

static void SetError(unsigned char channel, const char *errorString);
static void ClearError(unsigned char channel);
static void LockData(void);
static void UnlockData(void);


/**
 *	Initialize the alarm manager
 */
void InitAlarm( void )
{
	DbgPrint( DBG_LEVEL_INFO, "STATUS. Initializing Alarm Manager\r\n" );

	// Create shared data lock
	mDataLock = OSSemCreate(1);
}


/**
 *	Run alarm manager to process any alarms
 */
void RunAlarmManager( void )
{
	DbgPrint( DBG_LEVEL_INFO, "STATUS. Running Alarm Manager\r\n" );

	// Get configuration
	ChannelConfiguration_t channelConfig[kMaxNumberOfTempertures];
	GetConfiguration( (unsigned char *)&channelConfig[0] );

	LockData();

	// Get current temperatures
	GetCurrentTemperatures( (LTC2984Data_t *)&mAlarmData.temperatureReadings.channel[0] );

	// Reset delta-T error check
	mDeltaTThresholdReached = false;

	// Check temperature errors
	int channel;
	for( channel = 0; channel < kMaxNumberOfTempertures; channel++ )
	{
		if( 1 == channelConfig[channel].channelEnable )
		{
			// Check for temperature above limit
			if( channelConfig[channel].highTemperatureLimit_C < mAlarmData.temperatureReadings.channel[channel].temperature )
			{
				// Check read status as cause of error
				mAlarmData.errorCount[channel]++;
				// Check time at bad read state
				if( channelConfig[channel].alarmTriggerTime_sec < mAlarmData.errorCount[channel] )
				{
					if( VALID != mAlarmData.temperatureReadings.channel[channel].status )
					{
						// Signal error by bad sensor
						DbgPrint( DBG_LEVEL_ERROR, "ERROR. Channel %d, Cause: %s\r\n", 
							  channel, ChannelFault(mAlarmData.temperatureReadings.channel[channel].status) );
						SetError( channel, ChannelFault(mAlarmData.temperatureReadings.channel[channel].status) );
					}
					else
					{
						// Signal error
						DbgPrint( DBG_LEVEL_ERROR, "ERROR. Channel %d, Over temperature: %fC, limit: %fC\r\n",
								  channel, channelConfig[channel].highTemperatureLimit_C );
						SetError( channel, OverTemperatureError );
					}
				}	
			}

			// Check for temperature below limit
			else if( channelConfig[channel].lowTemperatureLimit_C > mAlarmData.temperatureReadings.channel[channel].temperature )
			{
				// Check read status as cause of error
				mAlarmData.errorCount[channel]++;
				// Check time at bad read state
				if( channelConfig[channel].alarmTriggerTime_sec < mAlarmData.errorCount[channel] )
				{
					if( VALID != mAlarmData.temperatureReadings.channel[channel].status )
					{
						// Signal error by bad sensor
						DbgPrint( DBG_LEVEL_ERROR, "ERROR. Channel %d, Cause: %s\r\n",
							  channel, ChannelFault(mAlarmData.temperatureReadings.channel[channel].status) );
						SetError( channel, ChannelFault(mAlarmData.temperatureReadings.channel[channel].status) );
					}
					else
					{
						// Signal error
						DbgPrint( DBG_LEVEL_ERROR, "ERROR. Channel %d, Under temperature: %fC, limit: %fC\r\n",
							  channel, channelConfig[channel].lowTemperatureLimit_C );
						SetError( channel, UnderTemperatureError );
					}
				}
			}
			else
			{
				// Reset error counter and clear channel error
				DbgPrint( DBG_LEVEL_INFO, "STATUS. No alarm on channel %d\r\n", channel );
				mAlarmData.errorCount[channel] = 0;
				ClearError( channel );
			}

			// Check for delta-T greater than 0.25C (delta-T = current - previous)
			float deltaT = mAlarmData.temperatureReadings.channel[channel].temperature
						 - mAlarmData.previousTemperatureReadings[channel].temperature; 
			if( mDeltaTThreshold < fabs(deltaT) )
			{
				mDeltaTThresholdReached = true;
			}
		}
	}

	// Update previous temperature readings
	memcpy( &mAlarmData.previousTemperatureReadings[0], 
		&mAlarmData.temperatureReadings.channel[0], 
		(sizeof(AdcReading_t) * kMaxNumberOfTempertures) );

	UnlockData();
}


static void SetError( unsigned char channel, const char *errorString )
{
	memset( (void *)&mErrorMap[channel].errorString[0], 0, kMaxErrorStringLength );
	strncpy( (char *)&mErrorMap[channel].errorString[0], errorString, kMaxNumberOfTempertures );
	mErrorMap[channel].error = 1;
}


static void ClearError(unsigned char channel)
{
	memset( &mErrorMap[channel].errorString[0], 0, kMaxErrorStringLength );
	mErrorMap[channel].error = 0;
}


void GetActiveAlarms( ErrorMap_t *errors )
{
	LockData();
	memcpy( errors, &mErrorMap[0], sizeof(ErrorMap_t) );
	UnlockData();
}


bool IsAlarmActive( void )
{
	bool alarm = false;
	for( int i = 0; i < kMaxNumberOfTempertures; i++ )
	{
		if( 0 != mErrorMap[i].error )
		{
			alarm = true;
			break;
		}
	}

	return( alarm );
}


bool IsDeltaTThresholdReached( void )
{
	return( mDeltaTThresholdReached );
}


static void LockData( void )
{
	unsigned char osErr;
	OSSemPend( mDataLock, 0, &osErr );
}


static void UnlockData( void )
{
	OSSemPost( mDataLock );
}
