/******************************************************************************
*	Copyright (c) 2017, Mack Informatcion Systems, In.  All rights reserved.
*
*	Main mote application control loop
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
#include "ltc2984.h"
#include "alarm.h"
#include "battery.h"
#include "moteapp.h"
#include "report.h"


//=========================== defines and enums ===============================
#define TASK_APP_MOTE_STK_SIZE	512
#define TASK_APP_MOTE_PRIORITY	53
#define TASK_APP_MOTE_NAME	"TASK_MOTE_TIC"


//=========================== structs  ========================================

typedef struct
{
    OS_STK      moteTaskStack[TASK_APP_MOTE_STK_SIZE];
    OS_EVENT*	moteSemaphore;
} MoteTaskParams_t;


//=========================== module parameters ======================================

static MoteTaskParams_t	mAppVars;
static OS_EVENT		*mDataLock;
static const int	kReportTimerCount = (OS_TICKS_PER_SEC * 60 * 10);	// 60 sec * 10 = 10 minutes
static int		mTenMinuteTimer   = (OS_TICKS_PER_SEC * 60 * 10);
static bool		mReadingAvailable = false;


//=========================== prototypes ======================================

static void RunMoteTimerTask(void *unused);
static void LockData(void);
static void UnlockData(void);
static void ClearReadingAvailable(void);
static bool IsReadingAvailable(void);


/**
 *	Process:
 *	1. ADC reading is taken every 30 seconds
 *	2. Temperatures are transmitted every 10 minutes
 *	3. If an alarm is active, temperatures are transmitted every 1 minute
 *	4. If delta-T between readings exceeds 0.25C, temperature is transmitted every 1 minute
 */
void RunMoteApp(void)
{
    DbgPrint(DBG_LEVEL_INFO, "STATUS. Mote Control App\r\n");

    // Create semaphore for task control
    mAppVars.moteSemaphore = OSSemCreate(1);

    // Create shared data lock
    mDataLock = OSSemCreate(1);

    // Create reader task
    DbgPrint(DBG_LEVEL_INFO, "STATUS. Mote Timer Task\r\n");
    unsigned char osErr = OSTaskCreateExt(  RunMoteTimerTask,
                                            (void *)0,
                                            (OS_STK *)(&mAppVars.moteTaskStack[TASK_APP_MOTE_STK_SIZE - 1]),
                                            TASK_APP_MOTE_PRIORITY,
                                            TASK_APP_MOTE_PRIORITY,
                                            (OS_STK *)mAppVars.moteTaskStack,
                                            TASK_APP_MOTE_STK_SIZE,
                                            (void *)0,
                                            OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR );

    OSTaskNameSet( TASK_APP_MOTE_PRIORITY,
                  (unsigned char *)TASK_APP_MOTE_NAME,
                  &osErr );
}


/**
 *	Take that fires semaphore every 30 seconds to read adc
 */
static void RunMoteTimerTask(void *unused)
{
    static int dataSendPhase = 1;

    while( 1 )
    {
	// Sleep for 30seconds
	OSTimeDly( OS_TICKS_PER_SEC * 30 );

	// Trigger new reading (it takes about 3 seconds)
	ClearReadingAvailable();
	AcquireTemperatureReadings();

	// Wait for up to 7 seconds for readings to complete
	int maxAdcTimeOut = 7;
	while (!IsReadingAvailable() )
	{
            OSTimeDly( OS_TICKS_PER_SEC );
          
            if( 0 >= maxAdcTimeOut-- )
            {           
                OSTimeDly( OS_TICKS_PER_SEC );
		DbgPrint(DBG_LEVEL_ERROR, "ERROR. Failed to read ADC\r\n");
                SendAlarm(0, kAlarmId_ReadFailed, 0 );
            }
        }

        // Check for any alarm or delta-T exceeding 0.25C
	RunAlarmManager();

	// Data is read every 30 seconds and sent at 60 seconds on alarm
	dataSendPhase++;
	if( 1 < dataSendPhase ) dataSendPhase = 0;

	// Check for alarm
	if( IsAlarmActive() )
	{
            if( 0 == dataSendPhase )
            {
		// Send temperature and alarms for all channels
		RunReportManager();
		// Reset 10 minute temperature post tiemr
		mTenMinuteTimer = kReportTimerCount;
            }
	}
	else if( IsDeltaTThresholdReached() )
	{
            if( 0 == dataSendPhase )
            {
                // Send temperature and alarms for all channels
		RunReportManager();
		// Reset 10 minute temperature post tiemr
		mTenMinuteTimer = kReportTimerCount;
            }
	}
	else
	{
            if( 0 >= mTenMinuteTimer-- )
            {
                mTenMinuteTimer = kReportTimerCount;
		RunReportManager();
		CheckBatteryHealth();
            }
        }
    }
}


/**
 *  Function called when LTC2984 completes conversions
 */
void TemperatureReadingsAvailable( void )
{
    LockData();
    mReadingAvailable = true;
    UnlockData();
}


static void ClearReadingAvailable( void )
{
    LockData();
    mReadingAvailable = false;
    UnlockData();
}


static bool IsReadingAvailable( void )
{
    LockData();
    bool available = mReadingAvailable;
    UnlockData();
    return( available );
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
