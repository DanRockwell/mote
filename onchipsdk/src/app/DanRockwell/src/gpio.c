/******************************************************************************
*       
*       Copyright (c) 2017, Mack Informatcion Systems, In.  All rights reserved.
*
*	Application used to manage alarms.
*
*	Author: Dan Rockwell
*/


// C includes
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// SDK includes
#include "dn_common.h"
#include "cli_task.h"
#include "loc_task.h"
#include "dn_gpio.h"
#include "dn_system.h"
#include "dn_exe_hdr.h"

// App includes
#include "gpio.h"
#include "debug.h"
#include "radio.h"


//=========================== defines and enums ===============================

#define TASK_APP_DIGITALIO_STK_SIZE	256
#define TASK_APP_DIGITALIO_PRIORITY	53
#define TASK_APP_DIGITALIO_NAME		"TASK_DIGITALIO"

typedef struct
{
	OS_STK          gpioTaskStack[TASK_APP_DIGITALIO_STK_SIZE];
	OS_EVENT*       gpioSemaphore;
} TaskParams_t;


//=========================== module parameters ===============================

static TaskParams_t	mAppVars;
static unsigned char mEvent1 = 0;
static unsigned char mEvent2 = 0;
static unsigned char mRelayState = 0;


//=========================== prototypes ======================================

static void ConfigureDigitialIO(void);
static void ReadGPIOTask(void *unused);


//=========================== initialization ==================================

void InitGPIO( void )
{
    DbgPrint(DBG_LEVEL_INFO, "STATUS. GPIO Initializing\r\n");

    ConfigureDigitialIO();

    // Create semaphore for task control
    mAppVars.gpioSemaphore = OSSemCreate(1);
  
    // Create reader task
    DbgPrint(DBG_LEVEL_INFO, "STATUS. Digital-IO Creating Task\r\n");
    unsigned char osErr = OSTaskCreateExt(ReadGPIOTask,
					(void *)0,
					(OS_STK *)(&mAppVars.gpioTaskStack[TASK_APP_DIGITALIO_STK_SIZE - 1]),
					TASK_APP_DIGITALIO_PRIORITY,
					TASK_APP_DIGITALIO_PRIORITY,
					(OS_STK *)mAppVars.gpioTaskStack,
					TASK_APP_DIGITALIO_STK_SIZE,
					(void *)0,
					OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR );

    OSTaskNameSet( TASK_APP_DIGITALIO_PRIORITY,
                  (unsigned char *)TASK_APP_DIGITALIO_NAME,
                  &osErr );
}


static void ReadGPIOTask(void *unused)
{
	unsigned char	event1[3];
	unsigned char	event2[3];
	unsigned char	sum1 = 0;
	unsigned char	sum2 = 0;
	bool			stateChanged = false;

	// Look for state changes after debouncing for 300ms
	while (1)
	{
 		for (int i = 0; i < 3; i++)
		{
			event1[i] = GpioRead(PIN_EVENT1_IN);
			event2[i] = GpioRead(PIN_EVENT2_IN);
			OSTimeDly(100);
			sum1 += event1[i];
			sum2 += event2[i];
		}

		// Look for state change on event1
		if( ((3 == sum1) && (0 == mEvent1))
		 || ((0 == sum1) && (1 == mEvent1)) )
		{
			mEvent1 = (0 == sum1) ? 0 : 1;
			stateChanged = true;
		}

		// Look for state change on event2
		if (((3 == sum2) && (0 == mEvent2))
			|| ((0 == sum2) && (1 == mEvent2)))
		{
			mEvent2 = (0 == sum2) ? 0 : 1;
			stateChanged = true;
		}

		if (true == stateChanged)
		{
			stateChanged = false;
			SendDigitalIOReport( mEvent1, mEvent2, mRelayState );
		}
	}
}


void GpioConfigPin( dn_device_t pin, unsigned char mode, unsigned char config )
{
	dn_gpio_ioctl_cfg_in_t    gpioInCfg;
	dn_gpio_ioctl_cfg_out_t   gpioOutCfg;

	// open the GPIO
	DbgPrint( DBG_LEVEL_INFO, "STATUS. Configuring GPIO pin: %d\r\n", pin );

	dn_open( pin, NULL, 0 );

	// set the mode
	switch( mode )
	{
		case DN_IOCTL_GPIO_CFG_INPUT:
			// config is pullMode
			DbgPrint(DBG_LEVEL_INFO, "STATUS. GPIO input pin: %d, pull-up: %d\r\n", pin, config );
			gpioInCfg.pullMode = config;
			dn_ioctl( pin,
                                  DN_IOCTL_GPIO_CFG_INPUT,
				  &gpioInCfg,
				  sizeof(gpioInCfg) );
			break;

		case DN_IOCTL_GPIO_CFG_OUTPUT:
			// config is initialLevel      
			DbgPrint(DBG_LEVEL_INFO, "STATUS. GPIO output pin: %d, initial state: %d\r\n", pin, config);
			gpioOutCfg.initialLevel = config;
			dn_ioctl( pin,
				  DN_IOCTL_GPIO_CFG_OUTPUT,
				  &gpioOutCfg,
				  sizeof(gpioOutCfg) );
			break;

		default:
			DbgPrint(DBG_LEVEL_INFO, "ERROR. GPIO invalid pin mode\r\n" );
	}
}


void GpioWrite( dn_device_t pin, const unsigned char value )
{
   dn_write( pin,                      // device
             (const char *)&value,     // buf
             sizeof(value) );	       // len
}


unsigned char GpioRead( dn_device_t pin )
{
    unsigned char   returnVal;

    dn_read( pin,		        // device
             (char *)&returnVal,        // buf
             sizeof(returnVal) );	// len
 
    return( returnVal );
}


static void ConfigureDigitialIO( void )
{
	mEvent1 = 0;
	mEvent2 = 0;
	mRelayState = 0;

	GpioConfigPin(PIN_EVENT1_IN, DN_IOCTL_GPIO_CFG_INPUT, PIN_HIGH);
	GpioConfigPin(PIN_EVENT2_IN, DN_IOCTL_GPIO_CFG_INPUT, PIN_HIGH);

	GpioConfigPin(PIN_RELAY_SET, DN_IOCTL_GPIO_CFG_OUTPUT, PIN_LOW);
	GpioConfigPin(PIN_RELAY_RESET, DN_IOCTL_GPIO_CFG_OUTPUT, PIN_LOW);
}


void  SetAlarmOut(void)
{
	mRelayState = 1;
	GpioWrite(PIN_RELAY_SET, PIN_HIGH);
	OSTimeDly(100);
	GpioWrite(PIN_RELAY_SET, PIN_LOW);
}


void  ClearAlarmOut(void)
{
	mRelayState = 0;
	GpioWrite(PIN_RELAY_RESET, PIN_HIGH);
	OSTimeDly(100);
	GpioWrite(PIN_RELAY_RESET, PIN_LOW);
}


void  GetEventData(unsigned char *event1, unsigned char *event2, unsigned char *relayState)
{
	*event1		= mEvent1;
	*event2		= mEvent2;
	*relayState = mRelayState;
}