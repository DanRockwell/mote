/******************************************************************************
*	Copyright (c) 2016, Mack Informatcion Systems, In.  All rights reserved.
*
*	Application used to receive time notifactions and helpers for time reports.
*
*	Author: Dan Rockwell
*/


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
#include "dn_time.h"
#include "dn_api_local.h"
#include "dnm_local.h"

// App includes
#include "debug.h"
#include "time.h"


//=========================== variables =======================================

typedef struct 
{
   dn_time_asn_t             currentASN;
   dn_time_utc_t             currentUTC;
   long long                 sysTime;
   dn_api_loc_notif_time_t   timeNotif;
} time_app_vars_t;

static time_app_vars_t mTimeParams;


//=========================== prototypes ======================================

static dn_error_t TimeNotificationCallback(dn_api_loc_notif_time_t* timeNotif, unsigned char length);


/**
 *	Create notification to receive time callbacks
 */
void InitTime( void )
{
	DbgPrint( DBG_LEVEL_INFO, "STATUS. Initializing Time Interface\r\n" );

	// Register a callback for when receiving a time notification
	dnm_loc_registerTimeNotifCallback( TimeNotificationCallback );
}


static dn_error_t TimeNotificationCallback(dn_api_loc_notif_time_t* timeNotif, unsigned char length)
{
	// copy notification to local variables for simpler debugging
	memcpy( &mTimeParams.timeNotif, timeNotif,length ); 
	DbgPrint( DBG_LEVEL_INFO, "STATUS. System Up Time:  %d sec\r\n", htonl(mTimeParams.timeNotif.upTime) );
   
	return DN_ERR_NONE;
}


void GetTimeSlotTime( char *time )
{
	// Get Nntwork time
	dn_getNetworkTime( &mTimeParams.currentASN, &mTimeParams.currentUTC );
	sprintf( time, "%s", (char *)&mTimeParams.currentASN );
	DbgPrint(DBG_LEVEL_INFO, "STATUS. Absolute Time Slot: %d\r\n", (unsigned)mTimeParams.currentASN.asn);
}


void GetTimeUTC( long long *timeUTC )
{
	// Get system time in sec from epoch
	dn_getSystemTime( (INT64U *)&mTimeParams.sysTime );
	*timeUTC = mTimeParams.sysTime;
	DbgPrint( DBG_LEVEL_INFO, "STATUS. UTC Time: %s\r\n", mTimeParams.sysTime );
}