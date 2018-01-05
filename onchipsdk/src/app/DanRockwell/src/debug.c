/******************************************************************************
 *	Copyright (c) 2016, Mack Informatcion Systems, In.  All rights reserved.
 *
 *	Debug interface using the CLI0 port.
 *
 *	Author: Dan Rockwell
*/

//=========================== includes =====================================

// C includes
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>

// SDK includes
#include "cli_task.h"
#include "loc_task.h"
#include "dn_common.h"
#include "dn_system.h"
#include "dn_uart_raw.h"
#include "dn_exe_hdr.h"
#include "Ver.h"
#include "well_known_ports.h"
#include "dnm_ucli.h"

// App includes
#include "alarm.h"


//=========================== CLI handlers ====================================

//dn_error_t SetDebugLevel( const char *arg, unsigned len );
//dn_error_t GetDebugLevel( const char *arg, unsigned len);


//=========================== variables =======================================

static int mDebugLevel = DBG_LEVEL_ALL;
/*
const dnm_ucli_cmdDef_t cliCmdDefs[] = 
{
    { &SetDebugLevel, "set-level", "", DN_CLI_ACCESS_LOGIN },
    { &GetDebugLevel, "get-level", "", DN_CLI_ACCESS_LOGIN },
    { NULL, NULL, NULL, DN_CLI_ACCESS_NONE },
};
*/

/******************************************************************************
*
*  Function    : void InitDebug()
*  Description : Initialize interface for printing debugging statements, and 
*				 accepting debug commands.
*
******************************************************************************/

void InitDebug()
{
    cli_task_init( "Mote_App", NULL );
}


/******************************************************************************
*
*  Function    : void SetDebugLevel(int level)
*  Description : Set debug level used for filtering debug output
*
******************************************************************************/
/*
dn_error_t SetDebugLevel(const char* arg, unsigned len)
{
    int level;

    // Read parameter
    int result = sscanf( arg, "%d", &level );
    if( 1 > result )
    {
	return DN_ERR_INVALID;
    }

    mDebugLevel = level;
    dnm_ucli_printf( "Debug level set: %d", mDebugLevel );

    return DN_ERR_NONE;
}
*/

/******************************************************************************
*
*  Function    : int GetDebugLevel(void)
*  Description : Print current level of debug setting
*
******************************************************************************/
/*
dn_error_t GetDebugLevel(const char *arg, unsigned len)
{
    dnm_ucli_printf( "Current debug level: %d", mDebugLevel );
    return DN_ERR_NONE;
}
*/

/******************************************************************************
*
*  Function    : void debugPrint(int level, char * fmtstring, ...)
*  Description : For printing debugging statements
*
******************************************************************************/

void DbgPrint(int level, const char *fmtstring, ...)
{
    if( level >= mDebugLevel )
    {   
        /*
        // Build the time stamp string
        time_t now = time( NULL );
        struct tm *tp = localtime( &now );
        char timeStamp[32];
	memset( (void *)&timeStamp, 0, sizeof(timeStamp) );
	snprintf( (char *)&timeStamp[0],
                  (sizeof(timeStamp) - 1),
                  "[%02d%02d%04d %02d:%02d:%02d.%03d] ",
                  tp->tm_mon + 1, tp->tm_mday, tp->tm_year + 1900, tp->tm_hour, tp->tm_min, tp->tm_sec, tb->millitm);
        */
	// Build the formatted print string
	char data[256];
	memset( (void *)&data, 0, sizeof(data) );
	va_list args;
	va_start( args, fmtstring );
	vsnprintf( (char *)&data[0],
		   (sizeof(data) - 1),
		   fmtstring,
		   args );
	va_end( args );

	// Print to terminal based on level setting
	// dnm_ucli_printf( "%s", (const char *)&timeStamp[0] );
	dnm_ucli_printf( "%s", (const char *)&data[0] );
    }
}
