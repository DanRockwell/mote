/******************************************************************************
*	Copyright (c) 2016, Mack Informatcion Systems, In.  All rights reserved.
*
*	Application used to configure and control the LTC2984 ADC.
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
#include "dn_gpio.h"

// App includes
#include "debug.h"
#include "ltc2984.h"
#include "ltc2984_cfg.h"
#include "spi.h"
#include "gpio.h"
#include "config.h"
#include "moteapp.h"


//=========================== defines and enums ===============================

#define TASK_APP_LTC2984_STK_SIZE	256
#define TASK_APP_LTC2984_PRIORITY	53
#define TASK_APP_LTC2984_NAME		"TASK_LTC2984"


//=========================== structs & enums =================================

// Read task
typedef struct
{
	OS_STK          readerTaskStack[TASK_APP_LTC2984_STK_SIZE];
	OS_EVENT*       readerSemaphore;
} TaskParams_t;


// 3-wire PT-100 RTD, 50uA excitiation on Channel 2, using American standard
const unsigned kRTDChannelConfig = SENSOR_TYPE__RTD_PT_100
				 | RTD_RSENSE_CHANNEL__2
				 | RTD_NUM_WIRES__3_WIRE
				 | RTD_EXCITATION_CURRENT__50UA
				 | RTD_STANDARD__AMERICAN;

// Sense resistor 10K
const unsigned kSenseResistorChannelConfig = SENSOR_TYPE__SENSE_RESISTOR | 0x9c4000;

const float kTemperatureRecalibrationThreshold  = 0.25;
const float kExpectedReferenceTemperature       = 0.0;
const int   kReferenceTestChannel		= 8;


//=========================== module parameters ======================================

static TaskParams_t		mAppVars;
static TemperatureData_t	mTemperatureReadings;
static OS_EVENT			*mDataLock;
static bool		   	mUpdateConfiguration = false;
static float		        mRawDataReading[TOTAL_NUMBER_OF_CHANNELS];


//=========================== prototypes ======================================

// Read task
static void		ReadLTC2984Task( void *unused );
static void		ConfigureLTC2984( void );
static bool		EnableLTC2984( void );
static void		DisableLTC2984( void );
static void		InitiateReadSequence( void );
static bool		ReadConversionComplete( void );
static void		ReadTemperatureValues( void );
static void		ComputeAverageTemperature( void );
const  char		*ChannelFault(unsigned char status);
static void		LockData( void );
static void		UnlockData( void );


/**
 *	Initialization of LTC2984 task
 */
void InitLTC2984( void )
{
	DbgPrint( DBG_LEVEL_INFO, "STATUS. LTC2984 Initializing Interface\r\n" );

	// Create semaphore for task control
	mAppVars.readerSemaphore = OSSemCreate(1);

	// Create shared data lock
	mDataLock = OSSemCreate(1);

	// Create reader task
	DbgPrint( DBG_LEVEL_INFO, "STATUS. LTC2984 Creating Task\r\n" );
	unsigned char osErr = OSTaskCreateExt(ReadLTC2984Task,
								   (void *)0,
								   (OS_STK *)(&mAppVars.readerTaskStack[TASK_APP_LTC2984_STK_SIZE - 1]),
								   TASK_APP_LTC2984_PRIORITY,
								   TASK_APP_LTC2984_PRIORITY,
								   (OS_STK *)mAppVars.readerTaskStack,
								   TASK_APP_LTC2984_STK_SIZE,
								   (void *)0,
								   OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR );
   
	OSTaskNameSet( TASK_APP_LTC2984_PRIORITY,
				   (unsigned char *)TASK_APP_LTC2984_NAME,
				   &osErr );
}


/**
 *	Signal semaphore to run temperature acquisition task.
 */
void AcquireTemperatureReadings( void )
{
	OSSemPost( mAppVars.readerSemaphore );
}


/**
 *	Get current temperature and channel status
 */
void GetCurrentTemperatures( LTC2984Data_t *pReading )
{
	LockData();
	int channel;
	for( channel = 0; channel < (TOTAL_NUMBER_OF_CHANNELS - 1); channel++ )
	{
		pReading->channel[channel].status      = mTemperatureReadings.currentReading[channel].status;
		pReading->channel[channel].temperature = mTemperatureReadings.temperatureAverage[channel];
	}
	UnlockData();
}


/**
 *	Get results for a specific channel
 */
void GetChannelReading(unsigned char channelNumber, AdcReading_t *reading)
{
	LockData();
	memcpy((void *)reading, (void const *)&mTemperatureReadings.currentReading[channelNumber], sizeof(AdcReading_t));
	UnlockData();
}


/**
 *	Return raw data and channel enable/disable
 */
void GetRawTemperatureReadings(unsigned char *channelConfig, float *temperatures)
{
	LockData();
	ChannelConfiguration_t config[TOTAL_NUMBER_OF_CHANNELS];
	GetConfiguration( (unsigned char *)&config[0] );

	*channelConfig = 0;
	for( int channel = 0; channel < (TOTAL_NUMBER_OF_CHANNELS - 1); channel++ )
	{
		temperatures[channel] = mRawDataReading[channel];
		if (1 == config[channel].channelEnable)
		{
			*channelConfig |= 1 << channel;
		}
	}	
	UnlockData();
}


/**
 *	The configuration changed, then LTC2984 configuration needs to be updated
 *	Set parameter used by task to force reconfiguration.
 */
void UpdateADCConfiguration( void )
{
	mUpdateConfiguration = true;
}


/**
 *	This task waits for the mote to join, then periodically sends temperature/ADC values.
 *	It is assumed that the gpio and spi are initialized prior to calling this module.
 */
static void ReadLTC2984Task( void *unused )
{
	unsigned char osErr;
	bool validRead = false;

	DbgPrint( DBG_LEVEL_INFO, "STATUS. LTC2984 Starting Task\r\n" );

	// Interrupt  input used to indicate read state of LTC2984
	GpioConfigPin( PIN_INTERRUPT, DN_IOCTL_GPIO_CFG_INPUT, DN_GPIO_PULL_NONE );

	// Reset output used to place LTC2984 in low power mode
	GpioConfigPin( PIN_RESET, DN_IOCTL_GPIO_CFG_OUTPUT, PIN_HIGH );

	DbgPrint( DBG_LEVEL_INFO, "STATUS. LTC2984 Enabling Device\r\n" );
	EnableLTC2984();

	// Configure LTC2984
	DbgPrint( DBG_LEVEL_INFO, "STATUS. LTC2984 Configuring Device\r\n" );
	ConfigureLTC2984();
                 
	while( 1 )
	{           
		DbgPrint( DBG_LEVEL_INFO, "STATUS. LTC2984 Disabling Device\r\n" );
		DisableLTC2984();

		// Wait on acquisition start signal
		DbgPrint( DBG_LEVEL_INFO, "STATUS. LTC2984 Waiting for ADC Acquisition Start\r\n" );
		OSSemPend( mAppVars.readerSemaphore, 0, &osErr );
		validRead = false;

		if( mUpdateConfiguration )
		{
			// Configuration required
			ConfigureLTC2984();
		}
		
		do
		{
			// Do not read ADC until it is configured
			if( !mUpdateConfiguration )
			{
				break;
			}

			// Enable LTC2984 from low power mode
			DbgPrint( DBG_LEVEL_INFO, "STATUS. LTC2984 Enabling\r\n" );
			if( !EnableLTC2984() )
			{
				DbgPrint( DBG_LEVEL_ERROR, "ERROR. LTC2984 Failed to enable device\r\n" );
				break;
			}

			// Initiate a read of all channels
			DbgPrint( DBG_LEVEL_INFO, "STATUS. LTC2984 Acquisition Start\r\n" );
			InitiateReadSequence();

			// Wait for read complete
			DbgPrint( DBG_LEVEL_INFO, "STATUS. LTC2984 Acquisition Complete\r\n" );
			if( !ReadConversionComplete() )
			{
				DbgPrint( DBG_LEVEL_ERROR, "ERROR. LTC2984 Failed to receive conversion complete\r\n" );
				break;
			}

			// Read all channels (8 data channels and 1 test channel)
			DbgPrint( DBG_LEVEL_INFO, "STATUS. LTC2984 Reading Temperatures\r\n" );
			ReadTemperatureValues();

			// Compute Moving Average of Readings
			DbgPrint( DBG_LEVEL_INFO, "STATUS. LTC2984 Computing Average Temperature\r\n" );
			ComputeAverageTemperature();

			// Signal conversion complete
			TemperatureReadingsAvailable();
			validRead = true;

		} while(0);

		// Re-configure SPI and device on any read error
		if( !validRead )
		{
			DbgPrint( DBG_LEVEL_INFO, "STATUS. LTC2984 Enabling Device\r\n" );
			EnableLTC2984();

			// Initialize SPI interface
			DbgPrint( DBG_LEVEL_INFO, "STATUS. LTC2984 Initializing SPI Interface\r\n" );
			InitSPI();
			
			// Configure LTC2984
			DbgPrint( DBG_LEVEL_INFO, "STATUS. LTC2984 Configuring Device\r\n" );
			ConfigureLTC2984();
		}
	}
}


/**
 *	An initial power on takes about 200ms.
 *	Following power on, reset to interrupt high is about 100ms
 */
static bool EnableLTC2984( void )
{
	// De-assert the reset line
	GpioWrite( PIN_RESET, PIN_HIGH );

	// Wait for interrupt line to go high
	unsigned char interruptPin;
	unsigned char tics = 0;
	do
	{
		OSTimeDly(10);
		interruptPin = GpioRead( PIN_INTERRUPT );
	} while( (0 == interruptPin) && (25 > ++tics) );

	return(0 == interruptPin);
}


/**
 *	Assert reset line to enter low power mode
 *	Wait for interrupt pin to indicate 
 */
static void DisableLTC2984( void )
{
	// De-assert the reset line
	GpioWrite( PIN_RESET, PIN_LOW );
}


/**
 *	Configure the 8 RTD channels and 9th RTD test channel
 *
 *	LTC2984 has channels 1 & 2 used for RTD current source and sense resistor.
 *	Each data channel is constructed from consecutive channel pairs.
 *	Configuration is performed on CH(n) and CH(n-1) is its differential pair.
 *	Channels are number 1..20, but address are zero based. So 0 & 1 are Rsense,
 *  and first data channel is 3 with channel 2 as it differential pair.
 */
static void ConfigureLTC2984( void )
{
	ChannelConfiguration_t config[TOTAL_NUMBER_OF_CHANNELS];
	if( !GetConfiguration( (unsigned char *)&config[0] ) )
	{
		// Signal to try again for a valid configuration
		mUpdateConfiguration = true;
		return;
	}

	// Configuring is now valid
	mUpdateConfiguration = false;
	// Configure the current source and Rsense for RTDs
	// Channel 1 = Rsense, channel 0 = Rsense - 1
	AssignChannel( 1, kSenseResistorChannelConfig );

	// Configure the RTD channel pairs
	int i;
	int channel;
	for( channel = 3; channel < LAST_LTC2984_CHANNEL; (channel += 2), i++ )
	{
		if( 1 == config[i].channelEnable )
		{
			AssignChannel( channel, kRTDChannelConfig );
		}
	}
}


/**
 *	Write start to address 0x000 to read all 9 temperature readings
 */
static void InitiateReadSequence( void )
{
	WriteSingleByte( READ_START_ADDRESSS, READ_MULTIPLE_CHANNELS );
}

/**
 *	Read interrupt pin until it goes high, and then check the status reagister
 *	to confirm conversion is complete. Each conversion takes about 251ms.
 *  Total Time = 9 * 251ms ==> 2.259 seconds
 */
static bool ReadConversionComplete( void )
{
	// Wait for interrupt line to go high, 3 second timeout
        unsigned char interruptPin;
        int tics = 0;
	do
	{
		OSTimeDly(10);
		interruptPin = GpioRead(PIN_INTERRUPT);
	} while ((0 == interruptPin) && (300 > ++tics));

	// Read conversion results via status register
	bool status = ConversionDone();

	return( status );
}


/**
 *	Read all temperature channels (temperature and status)
 *  First data channel is channel 4, having a channel address of 3 using zero based numbering.
 */
static void ReadTemperatureValues( void )
{
	LockData();
	int readIndex = 0;
	int channel;
	for( channel = 3; channel < LAST_LTC2984_CHANNEL; (channel += 2) )
	{
		// Get reading and apply temperature calibration
		ReadTemperatureResult( channel, &mTemperatureReadings.currentReading[readIndex] );
		mRawDataReading[readIndex] = mTemperatureReadings.currentReading[readIndex].temperature;
		float calibration = GetTemperatureCalibrationValue( readIndex, mRawDataReading[readIndex] );
		mTemperatureReadings.currentReading[readIndex].temperature = mRawDataReading[readIndex] - calibration;
		if( VALID != mTemperatureReadings.currentReading[readIndex].status )
		{
			DbgPrint( DBG_LEVEL_ERROR, "ERROR. Error reading channel %d, Temperature: %f, Status: %d\r\n",
					  (readIndex + 1), 
					  mTemperatureReadings.currentReading[readIndex].temperature,
					  mTemperatureReadings.currentReading[readIndex].status );

			PrintFaultData( mTemperatureReadings.currentReading[readIndex].status );
		}
		else
		{
			DbgPrint( DBG_LEVEL_INFO, "STATUS. Read Channel %d, Temperature: %f, Status: %d\r\n",
					 (readIndex + 1),
					 mTemperatureReadings.currentReading[readIndex].temperature,
					 mTemperatureReadings.currentReading[readIndex].status );
		}
		readIndex++;
	}
	UnlockData();
}


/**
 *	Add temperture measurement to average
 */
static void ComputeAverageTemperature( void )
{
	int point;
	float sum = 0;
	int channel;

	LockData();
	for (channel = 0; channel < (TOTAL_NUMBER_OF_CHANNELS - 1); channel++)
	{
		if( VALID == mTemperatureReadings.currentReading[channel].status )
		{
			// If data queue is not initialized, fill it with current temperature
			if( 0 == mTemperatureReadings.initialized )
			{
				for( point = 0; point < SAMPLE_AVERAGE_SIZE; point++ )
				{
					mTemperatureReadings.averagingQueue[channel][point] = mTemperatureReadings.currentReading[channel].temperature;
				}
				mTemperatureReadings.initialized[channel] = 1;
			}
			mTemperatureReadings.averagingQueue[channel][mTemperatureReadings.readIndex] = mTemperatureReadings.currentReading[channel].temperature;
			mTemperatureReadings.readIndex = (mTemperatureReadings.readIndex + 1) % SAMPLE_AVERAGE_SIZE;
			sum = 0;
			for( point = 0; point < SAMPLE_AVERAGE_SIZE; point++ )
			{
				sum += mTemperatureReadings.averagingQueue[channel][point];
			}
			mTemperatureReadings.temperatureAverage[channel] = sum / (float)SAMPLE_AVERAGE_SIZE;
		}
	}
	UnlockData();
}


const char *ErrorStrings[] =
{
	"SENSOR HARD FAILURE",
	"ADC HARD FAILURE",
	"COLD JUNCTION HARD FAILURE",
	"COLD JUNCTION SOFT FAILURE",
	"SENSORCOLD JUNCTIONABOVE",
	"COLD JUNCTION",
	"ADC RANGE ERROR",
	"VALID",
};


const char *ChannelFault( unsigned char status )
{
    if (status & SENSOR_HARD_FAILURE)   return(ErrorStrings[0]);
    if (status & ADC_HARD_FAILURE)      return(ErrorStrings[1]);
    if (status & CJ_HARD_FAILURE)       return(ErrorStrings[2]);
    if (status & CJ_SOFT_FAILURE)       return(ErrorStrings[3]);
    if (status & SENSOR_ABOVE)	        return(ErrorStrings[4]);
    if (status & SENSOR_BELOW)	        return(ErrorStrings[5]);
    if (status & ADC_RANGE_ERROR)       return(ErrorStrings[6]);
    if (status & VALID)		        return(ErrorStrings[7]);
    return( NULL );
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


/**
 *      Run test on reference channel to see if it remains within expected range.
 *	100 Ohm precision resistor should read 0C
 */
void CheckTemperatureTestChannel( float *measured, float *expected, float *threshold )
{
    *measured  = mRawDataReading[kReferenceTestChannel];
    *expected  = kExpectedReferenceTemperature;
    *threshold = kTemperatureRecalibrationThreshold;
}
