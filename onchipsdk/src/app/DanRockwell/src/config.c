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
#include <stdbool.h>

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
#include "ltc2984.h"


//=========================== module parameters ======================================

static ChannelConfiguration_t mChannelConfiguration[TOTAL_NUMBER_OF_CHANNELS];
static OS_EVENT	*mDataLock;
static bool	mConfigValid = false;


//=========================== prototypes ======================================

static void	LockData(void);
static void UnlockData(void);


/**
 *	Initialize the system configurator
 */
void InitConfiguration( void )
{
	DbgPrint( DBG_LEVEL_INFO, "STATUS. Initializing Configuration Manager\r\n" );

	// Create shared data lock
	mDataLock = OSSemCreate(1);

	memset( &mChannelConfiguration[0], 0, sizeof(mChannelConfiguration) );
	// Read configuration from FLASH
	if (ReadConfigurationFromFlash((unsigned char *)&mChannelConfiguration[0],
					(sizeof(ChannelConfiguration_t) * TOTAL_NUMBER_OF_CHANNELS)) )
	{
		// Flag ADC interface for new configuration
		mConfigValid = true;
		UpdateADCConfiguration();
	}
	else
	{
		DbgPrint( DBG_LEVEL_INFO, "ERROR. Failed to read Configuration from FLASH\r\n" );
	}
}


/**
 *  Configuration for all channels
 *	channel number, enable, sensor type, high limit, low limit, alarm trigger time
 */
bool GetConfiguration( unsigned char *config )
{
    if( !mConfigValid )
    {
	DbgPrint( DBG_LEVEL_INFO, "ERROR. LTC2984 not configurated\r\n" );
	return false;
    }

    LockData();
    memcpy( config, 
            &mChannelConfiguration[0], 
            (sizeof(ChannelConfiguration_t) * TOTAL_NUMBER_OF_CHANNELS) );
    UnlockData();
    return true;
}


void GetChannelConfiguration(unsigned char channel, unsigned char *config)
{
	LockData();
	memcpy(config, (unsigned char *)&mChannelConfiguration[channel], sizeof(ChannelConfiguration_t));
	UnlockData();
}


void SetChannelConfiguration(unsigned char channel, unsigned char *config)
{
	LockData();
	memcpy((unsigned char *)&mChannelConfiguration[channel], config, sizeof(ChannelConfiguration_t));

	// Write new configuration to FLASH
	if (!WriteConfigurationToFlash((unsigned char *)&mChannelConfiguration[0],
		(sizeof(ChannelConfiguration_t) * TOTAL_NUMBER_OF_CHANNELS)))
	{
		DbgPrint(DBG_LEVEL_INFO, "ERROR. Failed to write Configuration to FLASH\r\n");
	}

	// Update ADC configuration
	UpdateADCConfiguration();
	mConfigValid = true;

	UnlockData();
}


static void LockData(void)
{
	unsigned char osErr;
	OSSemPend( mDataLock, 0, &osErr );
}


static void UnlockData(void)
{
	OSSemPost( mDataLock );
}
