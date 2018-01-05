/******************************************************************************
*	Copyright (c) 2016, Mack Informatcion Systems, In.  All rights reserved.
*
*	Application used to configure channels and GPIO.
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
#include "config.h"
#include "flash.h"
#include "calibration.h"
#include "spline.h"
#include "ltc2984.h"
#include "radio.h"


//=========================== module parameters ======================================

static ChannelCalibration_t mChannelCalibration[TOTAL_NUMBER_OF_CHANNELS];
static OS_EVENT	*mDataLock;
static Spline_t *mCalibrationSplines[TOTAL_NUMBER_OF_CHANNELS];


//=========================== prototypes ======================================

static void LockData( void );
static void UnlockData( void );
static void BuildSplines( void );

/**
 *	Initialize the system configurator
 */
void InitCalibration( void )
{
	DbgPrint( DBG_LEVEL_INFO, "STATUS. Initializing Calirbation Manager\r\n" );

	// Create shared data lock
	mDataLock = OSSemCreate(1);

	// Read calibration from FLASH
	memset( &mChannelCalibration[0], 0, sizeof(mChannelCalibration) );
	if (ReadCalibrationFromFlash((unsigned char *)&mChannelCalibration[0], sizeof(mChannelCalibration)))
	{
		DbgPrint( DBG_LEVEL_INFO, "STATUS. Read calibraiton, building splines\r\n" );
		BuildSplines();
	}
	else
	{
		DbgPrint( DBG_LEVEL_INFO, "ERROR. Failed to read Configuration from FLASH\r\n" );
	}
}


/**
 *	Update the configuration for selected channel and reconfiguration LTC2984
 */
void SetChannelCalibration(unsigned char channel, unsigned char *calibration)
{
	DbgPrint( DBG_LEVEL_INFO, "STATUS. Updating calibration\r\n" );

	LockData();

	memcpy( (void *)&mChannelCalibration[channel], 
                (const void *)calibration,
		sizeof(ChannelConfiguration_t) );

	// Write new configuration to FLASH
	if( WriteCalibrationToFlash( (unsigned char *)&mChannelCalibration[0],
                                     (unsigned)(sizeof(ChannelCalibration_t) * TOTAL_NUMBER_OF_CHANNELS) ) )
	{
		DbgPrint( DBG_LEVEL_INFO, "ERROR. Failed to write calibration to FLASH\r\n" );
	}

	// Rebuild splines with new calibraiton
	BuildSplines();

	UnlockData();
}


void GetChannelCalibration(unsigned char channel, unsigned char *calibration)
{
	LockData();
	memcpy( (void *)calibration, (const void *)&mChannelCalibration[channel], sizeof(ChannelCalibration_t) );
	UnlockData();
}


static void LockData( void )
{
	unsigned char osErr;
	OSSemPend( mDataLock, 0, &osErr );
}


static void UnlockData(void)
{
	OSSemPost( mDataLock );
}


static void BuildSplines( void )
{
	LockData();
	for (int i = 0; i < TOTAL_NUMBER_OF_CHANNELS; i++)
	{
		// Delete old spline if one exists
		if( NULL != mCalibrationSplines[i] )
		{
			free( (void *)mCalibrationSplines[i] );
			mCalibrationSplines[i] = NULL;
		}

		// Create spline if data is available
		if (0 < mChannelCalibration[i].numberOfPoints)
		{
			int numberOfPoints = 1;
			float offsets[kMaxCalibrationPoints + 2];
			float axis[kMaxCalibrationPoints + 2];
			memset((void *)&offsets[0], 0, (sizeof(float) * (kMaxCalibrationPoints + 2)));
			memset((void *)&axis[0], 0, (sizeof(float) * (kMaxCalibrationPoints + 2)));
			if( 1 == mChannelCalibration[i].numberOfPoints )
			{
				offsets[0] = mChannelCalibration[i].data[0].measuredTemperature - mChannelCalibration[i].data[0].actualTemperature;
				axis[0] = mChannelCalibration[i].data[0].measuredTemperature;
			}
			else
			{
				offsets[0] = mChannelCalibration[i].data[0].measuredTemperature - mChannelCalibration[i].data[0].actualTemperature;
				axis[0] = -273.0f;
				for (int j = 0; j < kMaxCalibrationPoints; j++)
				{
					offsets[j + 1] = mChannelCalibration[i].data[j].measuredTemperature - mChannelCalibration[i].data[j].actualTemperature;
					axis[j + 1] = mChannelCalibration[i].data[j].measuredTemperature;
				}
				offsets[mChannelCalibration[i].numberOfPoints + 1] = offsets[mChannelCalibration[i].numberOfPoints];
				axis[mChannelCalibration[i].numberOfPoints + 1] = 100.0f;
				numberOfPoints = mChannelCalibration[i].numberOfPoints + 2;
			}
			Spline_Create( (Spline_t *)&mCalibrationSplines[i], mChannelCalibration[i].numberOfPoints );
			Spline_Setup( (Spline_t *)&mCalibrationSplines[i], numberOfPoints, &axis[0], &offsets[0] );
		}
	}
	UnlockData();
}


float GetTemperatureCalibrationValue(unsigned char channel, float inVal)
{
	float calValue = 0.0f;

	LockData();
	if( NULL != mCalibrationSplines[channel] )
	{
		calValue = Spline_Calc( (Spline_t *)&mCalibrationSplines[channel], inVal );
	}
	UnlockData();

	return( calValue );
}