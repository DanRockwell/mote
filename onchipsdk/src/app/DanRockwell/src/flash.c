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

// SDK includes
#include "dn_system.h"
#include "dn_fs.h"
#include "dn_api_param.h"
#include "dn_exe_hdr.h"
#include "dn_common.h"
#include "dn_system.h"
#include "dn_flash_drv.h"
#include "dn_flash_info.h"

// App includes
#include "debug.h"
#include "flash.h"


/**
*	Initialize the alarm manager
*/
void InitFlash( void )
{
    DbgPrint( DBG_LEVEL_INFO, "STATUS. Initializing Flash Interface\r\n" );
}


/**
 *	Save configuration to FLASH
 */
bool WriteConfigurationToFlash(unsigned char *data, unsigned length)
{
	dn_flash_par_id_t parId = DN_FLASH_PAR_USER1;
	dn_flash_par_t	  *par  = NULL;
	dn_error_t	  error;
	bool		  result = true;

	error = dn_flash_open( parId, par );
	if( DN_ERR_NONE != error )
	{
		result = false;
		DbgPrint( DBG_LEVEL_ERROR, "ERROR. Failed to open FLASH access to configuration\r\n" );
	}
	else
	{
		error = dn_flash_pageErase( *par, 0 );
		if (DN_ERR_NONE == error)
		{
			error = dn_flash_write( *par, 0, data, length );
			if (DN_ERR_NONE != error)
			{
				result = false;
				DbgPrint( DBG_LEVEL_ERROR, "ERROR. Failed to write configuration to FLASH\r\n" );
			}
		}
		else
		{
			result = false;
			DbgPrint( DBG_LEVEL_ERROR, "ERROR. Failed to erase configuration in FLASH\r\n" );
		}
	}

	return( result );
}


/**
 *	Recall configuration from FLASH
 */
bool ReadConfigurationFromFlash(unsigned char *data, unsigned length)
{
	dn_flash_par_id_t 	parId = DN_FLASH_PAR_USER1;
	dn_flash_par_t		*par = NULL;
	dn_error_t			error;
	bool				result = true;

	error = dn_flash_open( parId, par );
	if( DN_ERR_NONE != error )
	{
		result = false;
		DbgPrint( DBG_LEVEL_ERROR, "ERROR. Failed to open FLASH access to configuration\r\n" );
	}
	else
	{
		error = dn_flash_read( *par, 0, data, length );
		if( DN_ERR_NONE != error )
		{
			result = false;
			DbgPrint( DBG_LEVEL_ERROR, "ERROR. Failed to read configuration from FLASH\r\n" );
		}
	}

	return( result );
}


bool WriteCalibrationToFlash(unsigned char *data, unsigned length)
{
	dn_flash_par_id_t 	parId = DN_FLASH_PAR_USER2;
	dn_flash_par_t		*par = NULL;
	dn_error_t		error;
	bool			result = true;

	error = dn_flash_open( parId,  par );
	if (DN_ERR_NONE != error)
	{
		result = false;
		DbgPrint(DBG_LEVEL_ERROR, "ERROR. Failed to open FLASH access to calibration\r\n");
	}
	else
	{
		error = dn_flash_pageErase( *par, 0 );
		if (DN_ERR_NONE == error)
		{
			error = dn_flash_write( *par, 0, data, length );
			if (DN_ERR_NONE != error)
			{
				result = false;
				DbgPrint(DBG_LEVEL_ERROR, "ERROR. Failed to write calibration to FLASH\r\n");
			}
		}
		else
		{
			result = false;
			DbgPrint(DBG_LEVEL_ERROR, "ERROR. Failed to erase calibration in FLASH\r\n");
		}
	}

	return(result);
}

bool ReadCalibrationFromFlash(unsigned char *data, unsigned length)
{
	dn_flash_par_id_t 	parId = DN_FLASH_PAR_USER2;
	dn_flash_par_t		*par = NULL;
	dn_error_t			error;
	bool				result = true;

	error = dn_flash_open( parId,  par );
	if (DN_ERR_NONE != error)
	{
		result = false;
		DbgPrint(DBG_LEVEL_ERROR, "ERROR. Failed to open FLASH access to calibration\r\n");
	}
	else
	{
		error = dn_flash_read( *par, 0, data, length );
		if (DN_ERR_NONE != error)
		{
			result = false;
			DbgPrint(DBG_LEVEL_ERROR, "ERROR. Failed to read calibration from FLASH\r\n");
		}
	}

	return(result);
}


bool WriteSerialNumberToFlash(unsigned char *data, unsigned length)
{
	dn_flash_par_id_t 	parId = DN_FLASH_PAR_USER3;
	dn_flash_par_t		*par = NULL;
	dn_error_t			error;
	bool				result = true;

	error = dn_flash_open( parId,  par );
	if (DN_ERR_NONE != error)
	{
		result = false;
		DbgPrint(DBG_LEVEL_ERROR, "ERROR. Failed to open FLASH access to serial number\r\n");
	}
	else
	{
		error = dn_flash_pageErase( *par, 0 );
		if (DN_ERR_NONE == error)
		{
			error = dn_flash_write( *par, 0, data, length );
			if (DN_ERR_NONE != error)
			{
				result = false;
				DbgPrint(DBG_LEVEL_ERROR, "ERROR. Failed to write serial number to FLASH\r\n");
			}
		}
		else
		{
			result = false;
			DbgPrint(DBG_LEVEL_ERROR, "ERROR. Failed to erase serial number in FLASH\r\n");
		}
	}

	return(result);
}


bool ReadSerialNumberFromFlash(unsigned char *data, unsigned length)
{
	dn_flash_par_id_t 	parId = DN_FLASH_PAR_USER3;
	dn_flash_par_t		*par = NULL;
	dn_error_t			error;
	bool				result = true;

	error = dn_flash_open( parId,  par );
	if (DN_ERR_NONE != error)
	{
		result = false;
		DbgPrint(DBG_LEVEL_ERROR, "ERROR. Failed to open FLASH access to serial number\r\n");
	}
	else
	{
		error = dn_flash_read( *par, 0, data, length );
		if (DN_ERR_NONE != error)
		{
			result = false;
			DbgPrint(DBG_LEVEL_ERROR, "ERROR. Failed to read serial number from FLASH\r\n");
		}
	}

	return(result);
}
