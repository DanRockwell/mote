/******************************************************************************
*	Copyright (c) 2016, Mack Informatcion Systems, In.  All rights reserved.
*
*	Radio interface for packet send and receive services
*
*	Author: Dan Rockwell
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include "os_cfg.h"
#include "os_cpu.h"

#include "dn_common.h"
#include "dn_exe_hdr.h"
#include "loc_task.h"
#include "dnm_local.h"
#include "debug.h"
#include "radio.h"
#include "version.h"
#include "config.h"
#include "alarm.h"
#include "flash.h"
#include "gpio.h"


//=========================== defines =========================================

#define MAX_NUMBER_OF_MESSAGES		10
#define TASK_APP_RADIO_STK_SIZE		256
#define TASK_APP_RADIO_PRIORITY		53
#define TASK_APP_RADIO_NAME			"TASK_RADIO"


//=========================== variables =======================================

typedef struct 
{
	OS_EVENT*	joinedSem;
	OS_STK		radioTaskStack[TASK_APP_RADIO_STK_SIZE];
} join_app_vars_t;

join_app_vars_t		mRadioVars;
static OS_EVENT		*mDataLock;

static unsigned char mQueuedMsgCount;
static unsigned char mEnqueueIndex;
static unsigned char mDequeueIndex;
static unsigned char mMessageQueue[256];


//=========================== prototypes ======================================

static void		SendTask( void* unused );
static bool		DataToSend( void );
static void             EnqueuePacket(unsigned char *packet, int length);
static void		DequeuePacket(unsigned char *packet, unsigned char *length);
static void		LockData(void);
static void		UnlockData(void);
static void		GetTemperatureAlarm(unsigned char *data, unsigned char length);
static void		GetDigitalEventReport(unsigned char *data, unsigned char length);
static void		SetChannelConfig(unsigned char *data, unsigned char length);
static void		GetChannelConfig(unsigned char *data, unsigned char length);
static void		SetChannelCal(unsigned char *data, unsigned char length);
static void		GetCalibration(unsigned char *data, unsigned char length);
static void		SetSerialNumber(unsigned char *data, unsigned char length);
static void		GetSerialNumber(unsigned char *data, unsigned char length);
static void		GetFirmwareVersion(unsigned char *data, unsigned char length);
static void		GetUncalibratedTemperatures(unsigned char *data, unsigned char length);
dn_error_t		RxNotifyCallback(dn_api_loc_notif_received_t* rxFrame, unsigned char length);
static void             ProcessIncomingMessage(unsigned char *data, unsigned length);
static void             PerformPressToTest(unsigned char *data, unsigned char length);


//=========================== initialization ==================================

void InitRadio( void )
{
	DbgPrint( DBG_LEVEL_INFO, "STATUS. Initializing Radio Manager\r\n" );

	unsigned char  osErr;
   
	// Create semaphore for loc_task to indicate when joined
	mRadioVars.joinedSem = OSSemCreate(0);
   
	// Create shared data lock
	mDataLock = OSSemCreate(1);

	// Initialize helper tasks
	loc_task_init(	JOIN_YES,				// fJoin
					NULL,					// netId
					60000,					// udpPort
					mRadioVars.joinedSem,	// joinedSem
					BANDWIDTH_NONE,			// bandwidth
					NULL );					// serviceSem
   
	// Register a callback for when receiving a packet
	dnm_loc_registerRxNotifCallback( RxNotifyCallback );
   

	// Initialize sendTask
	osErr = OSTaskCreateExt(  SendTask,
							  (void *) 0,
							  (OS_STK *)(&mRadioVars.radioTaskStack[TASK_APP_RADIO_STK_SIZE - 1]),
							  TASK_APP_RADIO_PRIORITY,
							  TASK_APP_RADIO_PRIORITY,
							  (OS_STK *)mRadioVars.radioTaskStack,
							  TASK_APP_RADIO_STK_SIZE,
							  (void *)0,
							  OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR );
   
	OSTaskNameSet(TASK_APP_RADIO_PRIORITY, (unsigned char *)TASK_APP_RADIO_NAME, &osErr);
}


static void SendTask( void* unused ) 
{
	unsigned char	osErr;
	unsigned char	pkBuf[sizeof(loc_sendtoNW_t) + 64];
	loc_sendtoNW_t	*pkToSend;
	unsigned char	rc;

	// Wait for the loc_task to finish joining the network
	DbgPrint( DBG_LEVEL_INFO, "STATUS. Waiting for network join\r\n" );
	OSSemPend( mRadioVars.joinedSem, 0, &osErr );
   
	DbgPrint( DBG_LEVEL_INFO, "STATUS. Radio joined network\r\n" );

	while( 1 ) 
	{     
		// Wait for queued packets
		if (false == DataToSend())
		{
			OSTimeDly(  (10 * OS_TICKS_PER_SEC) );
			continue;
		}

		// prepare packet to send
		memset( pkBuf, 0, sizeof(pkBuf) );
		pkToSend = (loc_sendtoNW_t *)pkBuf;
		pkToSend->locSendTo.socketId	= loc_getSocketId();
		pkToSend->locSendTo.destAddr    = DN_MGR_IPV6_MULTICAST_ADDR;
		pkToSend->locSendTo.destPort    = 60000;
		pkToSend->locSendTo.serviceType = DN_API_SERVICE_TYPE_BW;   
		pkToSend->locSendTo.priority    = DN_API_PRIORITY_MED;   
		pkToSend->locSendTo.packetId    = 0xFFFF;

		unsigned char length = 0;
		DequeuePacket((unsigned char *)&pkToSend->locSendTo.payload[0], &length);
      
		// Send packet
		unsigned char dnErr = dnm_loc_sendtoCmd( pkToSend, length, &rc );
		if( rc == DN_API_RC_OK ) 
		{
			DbgPrint(DBG_LEVEL_INFO, "STATUS. Radio Manager Sent packet\r\n");
		} 
		else 
		{
			DbgPrint(DBG_LEVEL_INFO, "ERROR. Radio Manager Send Error, 0x%02x\r\n", rc );
		}
	}
}


dn_error_t RxNotifyCallback(dn_api_loc_notif_received_t* rxFrame, unsigned char length)
{
	unsigned char i;
   
	DbgPrint(DBG_LEVEL_INFO, "packet received:\r\r\n");
	DbgPrint(DBG_LEVEL_INFO, " - sourceAddr: ");
	for( i = 0; i < sizeof(dn_ipv6_addr_t); i++ ) 
	{
		DbgPrint(DBG_LEVEL_INFO, "%02x", ((unsigned char *)&(rxFrame->sourceAddr))[i]);
	}
	DbgPrint(DBG_LEVEL_INFO, "\r\r\n");
	DbgPrint(DBG_LEVEL_INFO, " - sourcePort: %d\r\r\n", rxFrame->sourcePort);
	DbgPrint(DBG_LEVEL_INFO, " - data:       (%d bytes) ", (length - sizeof(dn_api_loc_notif_received_t)));
	for( i = 0; i < (length - sizeof(dn_api_loc_notif_received_t)); i++ ) 
	{
		DbgPrint(DBG_LEVEL_INFO, "%02x", rxFrame->data[i]);
	}
	DbgPrint(DBG_LEVEL_INFO, "\r\r\n");
	ProcessIncomingMessage((unsigned char *)&rxFrame->data[0], (unsigned char)(length - sizeof(dn_api_loc_notif_received_t)));

	return DN_ERR_NONE;
}


void SendTemperatures(unsigned char channel, float temperature)
{
	DbgPrint(DBG_LEVEL_INFO, "Sending new temperature readings\r\r\n");
	ReportDataMessage_t report;
	report.messageId	= kMessageIdChannelTemperature;
	report.messageSize	= sizeof(report);
	report.channelNumber	= channel;
	report.alarmId		= kAlarmId_None;
	report.temperature	= temperature;
	EnqueuePacket((unsigned char *)&report, sizeof(report));
}


void SendAlarm(unsigned char channel, unsigned char error, float temperature)
{
	DbgPrint(DBG_LEVEL_INFO, "Sending new alarm readings\r\r\n");
	ReportDataMessage_t report;
	report.messageId		= kMessageIdChannelTemperature;
	report.messageSize		= sizeof(report);
	report.channelNumber	= channel;
	report.alarmId			= error;
	report.temperature		= temperature;
	EnqueuePacket((unsigned char *)&report, sizeof(report));
}


void SendDigitalIOReport( unsigned char event1, unsigned char event2, unsigned char relayState )
{
	DbgPrint(DBG_LEVEL_INFO, "Sending digital-io readings\r\r\n");
	ReportEventMessage_t report;
	report.messageId	= kMessageIdDigitIOEvent;
	report.messageSize	= sizeof(report);
	report.event1		= event1;
	report.event2		= event2;
	report.relayState	= relayState;
	EnqueuePacket((unsigned char *)&report, sizeof(report));
}


void SendChannelConfig(unsigned char channel)
{
	DbgPrint(DBG_LEVEL_INFO, "Sending channel configuration\r\r\n");
	ChannelConfigurationMessage_t message;
	message.messageId		= kMessageIdChannelConfiguration;
	message.messageSize		= sizeof(message);
	GetChannelCalibration(channel, (unsigned char *)&message.config);
	EnqueuePacket((unsigned char *)&message, sizeof(message));
}


void SendSerialNumber( void )
{
	DbgPrint(DBG_LEVEL_INFO, "Sending serial numner\r\r\n");
	SerialNumberMessage_t message;
	message.messageId   = kGetMoteSerialNumber;
	message.messageSize = sizeof(message);
	ReadSerialNumberFromFlash((unsigned char *)&message.serialNumber[0], kSerialNumberLength);
	EnqueuePacket((unsigned char *)&message, sizeof(message));
}


void SendFirmwareVersion( void )
{
	DbgPrint(DBG_LEVEL_INFO, "Sending version\r\r\n");
	FirmwareVersionMessage_t message;
	message.messageId			= kMessageIdFirmwareVersion;
	message.messageSize			= sizeof(message);
	message.majorVerionsNumber	= FirmwareVersion[0];
	message.minorVersionNumber	= FirmwareVersion[1];
	message.patchVersionNumber	= FirmwareVersion[2];
	message.buildVersionNumber	= FirmwareVersion[3];
	EnqueuePacket((unsigned char *)&message, sizeof(message));
}


void SendChannelCalibration(unsigned char channel)
{
	DbgPrint(DBG_LEVEL_INFO, "Sending channel calibration\r\r\n");
	GetChannelCalibration_t message;
	message.messageId       = kMessageIdChannelCalibration;
	message.messageSize	= sizeof(message);
	GetChannelCalibration(channel, (unsigned char *)&message.calibration);
	EnqueuePacket((unsigned char *)&message, sizeof(message));
}


static bool DataToSend( void )
{
	LockData();
	bool dataReady = (0 < mQueuedMsgCount) ? true : false;
	UnlockData();
	return( dataReady );
}


static void EnqueuePacket(unsigned char *packet, int length)
{
	LockData();

	int i;
	for( i = 0; i < length; i++ )
	{
		mMessageQueue[mEnqueueIndex] = *packet;
		mEnqueueIndex = (mEnqueueIndex + 1) % sizeof(mMessageQueue);
		packet++;
	}
	mQueuedMsgCount++;

	UnlockData();
}


static void DequeuePacket(unsigned char *packet, unsigned char *length)
{
	LockData();

	// First bytes is always message Id
	*packet       = mMessageQueue[mDequeueIndex];
	mDequeueIndex = (mDequeueIndex + 1) % sizeof(mMessageQueue);
	packet++;

	// Second byte is always message length
	*length		  = mMessageQueue[mDequeueIndex];
	*packet		  = mMessageQueue[mDequeueIndex];
	mDequeueIndex = (mDequeueIndex + 1) % sizeof(mMessageQueue);
	packet++;

	// Read remainder of packet
	int i;
	for( i = 0; i < (*length - 2); i++ )
	{
		*packet       = mMessageQueue[mDequeueIndex];
		mDequeueIndex = (mDequeueIndex + 1) % sizeof(mMessageQueue);
		packet++;
	}
	mQueuedMsgCount--;

	UnlockData();
}


static void LockData(void)
{
	unsigned char osErr;
	OSSemPend(mDataLock, 0, &osErr);
}


static void UnlockData(void)
{
	OSSemPost(mDataLock);
}


static void ProcessIncomingMessage(unsigned char *data, unsigned length)
{
	unsigned char msgId = *data;

	switch( msgId )
	{
		case kMessageIdChannelConfiguration:
			GetTemperatureAlarm(data, length);
			break;

		case kGetDigitalIOReport:
			GetDigitalEventReport(data, length);
			break;

		case kSetChannelConfiguration:
			SetChannelConfig(data, length);
			break;

		case kGetChannelConfiguration:
			GetChannelConfig(data, length);
			break;

		case kSetChannelCalibration:
			SetChannelCal(data, length);
			break;

		case kGetChannelCalibration:
			GetCalibration(data, length);
			break;

		case kSetMoteSerialNumber:
			SetSerialNumber(data, length);
			break;

		case kGetMoteSerialNumber:
			GetSerialNumber(data, length);
			break;

		case kGetMoteFirmwareVersion:
			GetFirmwareVersion(data, length);
			break;

		case kGetUncalibratedTemperatures:
			GetUncalibratedTemperatures(data, length);
			break;

		case kPerformPressToTest:
			PerformPressToTest(data, length);
			break;

		default:
			break;
	}
}


static void GetTemperatureAlarm(unsigned char *data, unsigned char length)
{
	DbgPrint(DBG_LEVEL_INFO, "Getting channel temperature reading\r\r\n");
	GetChannelMessage_t *request = (GetChannelMessage_t *)data;
	AdcReading_t response;
	memset(&response, 0, sizeof(response));
	GetChannelReading(request->channelNumber, &response);

	ReportDataMessage_t report;
	report.messageId	= kMessageIdChannelConfiguration;
	report.messageSize	= sizeof(report);
	report.channelNumber    = request->channelNumber;
	report.alarmId		= response.status;
	report.temperature      = response.temperature;
	EnqueuePacket((unsigned char *)&report, sizeof(report));
}


static void GetDigitalEventReport(unsigned char *data, unsigned char length)
{
	DbgPrint(DBG_LEVEL_INFO, "Getting event report\r\r\n");
	unsigned char event1;
	unsigned char event2;
	unsigned char relayState;
	GetEventData( &event1, &event2, &relayState );
	
	ReportEventMessage_t report;
	report.messageId	= kGetDigitalIOReport;
	report.messageSize	= sizeof(report);
	report.event1		= event1;
	report.event2		= event2;
	report.relayState	= relayState;
	EnqueuePacket((unsigned char *)&report, sizeof(report));
}


static void SetChannelConfig(unsigned char *data, unsigned char length)
{
	DbgPrint(DBG_LEVEL_INFO, "Updating channel configuration\r\r\n");
	ChannelConfigurationMessage_t *config = (ChannelConfigurationMessage_t *)data;
	SetChannelConfiguration(config->config.channelNumber, (unsigned char *)&config->config);
}


static void GetChannelConfig(unsigned char *data, unsigned char length)
{
	DbgPrint(DBG_LEVEL_INFO, "Getting channel configuration\r\r\n");
	GetChannelConfiguration_t *request = (GetChannelConfiguration_t *)data;
	ChannelConfigurationMessage_t config;
	config.messageId        = kGetChannelConfiguration;
	config.messageSize      = sizeof(ChannelConfigurationMessage_t);
	GetChannelConfiguration(request->channelNumber, (unsigned char *)&config.config);
	EnqueuePacket((unsigned char *)&config, sizeof(config));
}


static void SetChannelCal(unsigned char *data, unsigned char length)
{
	DbgPrint(DBG_LEVEL_INFO, "Updating channel calibration\r\r\n");
	ChannelCalibrationMessage_t *cal = (ChannelCalibrationMessage_t *)data;
	SetChannelCalibration( cal->calibration.channelNumber, (unsigned char *)&cal->calibration.data[cal->calibration.channelNumber] );
}


static void GetCalibration(unsigned char *data, unsigned char length)
{
	GetChannelCalibration_t *request = (GetChannelCalibration_t *)data;
	ChannelCalibrationMessage_t cal;
	cal.messageId   = kGetChannelCalibration;
	cal.messageSize = sizeof(ChannelCalibrationMessage_t);
	GetChannelCalibration(request->calibration.channelNumber, (unsigned char *)&cal.calibration);
	EnqueuePacket((unsigned char *)&cal, sizeof(cal));
}


static void SetSerialNumber(unsigned char *data, unsigned char length)
{
	SerialNumberMessage_t *serialNum = (SerialNumberMessage_t *)data;
	WriteSerialNumberToFlash((unsigned char *)&serialNum->serialNumber[0], kSerialNumberLength);
}


static void GetSerialNumber(unsigned char *data, unsigned char length)
{
	SendSerialNumber();
}


static void GetFirmwareVersion(unsigned char *data, unsigned char length)
{
	SendFirmwareVersion();
}


static void GetUncalibratedTemperatures(unsigned char *data, unsigned char length)
{
	GetUncalibratedTemperatures_t uncalData;
	uncalData.messageId = kGetUncalibratedTemperatures;
	uncalData.messageSize = sizeof(GetUncalibratedTemperatures_t);
	GetRawTemperatureReadings( &uncalData.channelConfig, &uncalData.temperatures[0] );
	EnqueuePacket((unsigned char *)&uncalData, sizeof(uncalData));
}


static void PerformPressToTest(unsigned char *data, unsigned char length)
{
	// Check the press to test channel
	float measured;
	float expected;
	float threshold;
	CheckTemperatureTestChannel( &measured, &expected, &threshold );

	PressToTestResult_t result;
	result.messageId   = kPerformPressToTest;
	result.messageSize = sizeof(PressToTestResult_t);
	result.measured    = measured;
	result.expected    = expected;
	result.threshold   = threshold;
	result.passFail    = (threshold > fabs(measured - expected)) ? 0 : -1;
	EnqueuePacket((unsigned char *)&result, sizeof(result));
}


void SendBatteryHealthReport( int batteryLife, float batterImpedance )
{
    // currently not implemented
}

