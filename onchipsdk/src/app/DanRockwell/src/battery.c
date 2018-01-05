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

// SDK includes
#include "dn_common.h"
#include "dn_system.h"
#include "dn_adc.h"
#include "dn_exe_hdr.h"
#include "dn_gpio.h"
#include "cli_task.h"
#include "loc_task.h"
#include "app_task_cfg.h"

// App includes
#include "debug.h"
#include "gpio.h"
#include "battery.h"
#include "radio.h"


#define TASK_APP_BATTERY_STK_SIZE	256
#define TASK_APP_BATTERY_PRIORITY	53
//#define TASK_APP_BATTERY_NAME		"TASK_BATTERY"

#define BATTERY_MONITOR_ENABLE		DN_GPIO_PIN_1_DEV_ID

#define kBatteryImpedanceThreshold	1


//=========================== variables =======================================

typedef struct 
{
   OS_STK	batteryTaskStack[TASK_APP_BATTERY_STK_SIZE];
} BatteryParams_t;

static BatteryParams_t  mBatteryParams;
static OS_EVENT	        *mBatterMonitorSemaphore;


//=========================== prototypes ======================================

static void BatteryTask( void* unused );


/**
 *	Initialize battery health monitor task
 */
void InitBattery( void )
{
  // Create the battery task
	DbgPrint(DBG_LEVEL_INFO, "STATUS. Initializing Battery Monitor Task\r\n");

	// Create semaphore for task control
	mBatterMonitorSemaphore = OSSemCreate(1);

	unsigned char osErr = OSTaskCreateExt(BatteryTask,
						(void *)0,
								  (OS_STK *)(&mBatteryParams.batteryTaskStack[TASK_APP_BATTERY_STK_SIZE - 1]),
								  TASK_APP_BATTERY_PRIORITY,
								  TASK_APP_BATTERY_PRIORITY,
								  (OS_STK *)mBatteryParams.batteryTaskStack,
								  TASK_APP_BATTERY_STK_SIZE,
								  (void *)0,
								  OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR );

   OSTaskNameSet(TASK_APP_BATTERY_PRIORITY, (unsigned char *)TASK_APP_BATTERY_NAME, &osErr);
}


/**
*	Signal semaphore to run battery monitor task.
*/
void CheckBatteryHealth( void )
{
	DbgPrint(DBG_LEVEL_INFO, "STATUS. Signaling battery health check task\r\n");
	unsigned char osErr = OSSemPost(mBatterMonitorSemaphore);
}


/**
 *	Battery monitor task
 */
static void BatteryTask( void *unused )
{
	unsigned char	          osErr;
	dn_adc_drv_open_args_t    openArgs;
      
	DbgPrint( DBG_LEVEL_INFO, "STATUS. Running Battery Monitor Task\r\n" );

	// Configure battery monitor enable interface
	GpioConfigPin(BATTERY_MONITOR_ENABLE, DN_IOCTL_GPIO_CFG_OUTPUT, PIN_LOW);

	while( 1 ) 
	{
		/////////////////////////////////
		// Read the open circuit voltage
		GpioWrite( BATTERY_MONITOR_ENABLE, PIN_LOW );
		OSTimeDly( 10 );

		// Wait on start signal
		DbgPrint(DBG_LEVEL_INFO, "STATUS. LTC2984 Waiting for battery monitor Start\r\n");
		OSSemPend(mBatterMonitorSemaphore, 0, &osErr );

		// Open battery sensor with no load 
		memset(&openArgs, 0, sizeof(dn_adc_drv_open_args_t));
		openArgs.loadBattery = DN_ADC_LOAD_BATT_NONE;
		dn_open( DN_ADC_AI_0_DEV_ID,	
			&openArgs,				
			sizeof(openArgs) );  

		// Read battery value with no load
		unsigned short openCircuitVoltage;
		dn_read( DN_ADC_AI_0_DEV_ID,				
			(char*)&openCircuitVoltage,		
			sizeof(openCircuitVoltage) );	

		dn_close(DN_ADC_AI_0_DEV_ID);

		/////////////////////////////////
		// Read the Load circuit voltage
		GpioWrite( BATTERY_MONITOR_ENABLE, PIN_HIGH );
		OSTimeDly(10);

		// Open battery sensor with 500 Ohm load 
		memset( &openArgs, 0, sizeof(dn_adc_drv_open_args_t) );
		openArgs.loadBattery = DN_ADC_LOAD_BATT_500_OHM;
		dn_open( DN_ADC_AI_0_DEV_ID,
						 &openArgs,
						 sizeof(openArgs) );

		// Read battery value with 500 Ohm load
		unsigned short loadCircuitVoltage;
		dn_read( DN_ADC_AI_0_DEV_ID,				
			(char*)&loadCircuitVoltage,		
			sizeof(loadCircuitVoltage) );	 

		/////////////////////////////////////////
		// Close device and remove load resistor
		dn_close( DN_ADC_AI_0_DEV_ID );
		GpioWrite( BATTERY_MONITOR_ENABLE, PIN_LOW );

		//////////////////////////////////////////////
		// Battery Impedance = R (500 Ohms) / Voltage
		float batterImpedance = 500.0 * ((float)openCircuitVoltage / (float)loadCircuitVoltage - 1.0);

		// Return to the open circuit voltage
		GpioWrite( BATTERY_MONITOR_ENABLE, PIN_LOW );

		int batteryLife = (kBatteryImpedanceThreshold > batterImpedance) ? 1:0;
		DbgPrint( DBG_LEVEL_INFO, "STATUS. Battery V1: %f, V2: %f, Status: %d\r\n",
				  openCircuitVoltage, loadCircuitVoltage, batteryLife );

		// Report results
		SendBatteryHealthReport( batteryLife, batterImpedance );
	}
}
