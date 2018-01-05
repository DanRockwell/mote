#ifndef RADIO_H
#define RADIO_H

#include "app_common.h"
#include "config.h"
#include "calibration.h"


//=========================== structs & enums =================================

#define kMessageIdChannelTemperature	0x0A
#define kMessageIdDigitIOEvent		0x0B
#define kMessageIdChannelConfiguration	0x0C
#define kMessageIdSerialNumber		0x0B
#define kGetDigitalIOReport		0x0D
#define kSetChannelConfiguration	0x0E
#define kGetChannelConfiguration	0x0F
#define kSetChannelCalibration		0x10
#define kGetChannelCalibration		0x11
#define kSetMoteSerialNumber		0x12
#define kGetMoteSerialNumber		0x13
#define kGetMoteFirmwareVersion		0x14
#define kGetUncalibratedTemperatures	0x15
#define kPerformPressToTest		0x16

#define kMessageIdFirmwareVersion	0x05
#define kMessageIdStackVersion		0x06
#define kMessageIdChannelCalibration	0x07

#define kSerialNumberLength		16
#define kMacAddressLength		6
#define	kIpv6AddressLength		8


typedef struct
{
	unsigned char   messageId;
	unsigned char   messageSize;
	unsigned char	channelNumber;
	unsigned char	alarmId;
	float		temperature;
} ReportDataMessage_t;


typedef struct
{
    unsigned char   messageId;
    unsigned char   messageSize;
    unsigned char   event1;
    unsigned char   event2;
    unsigned char   relayState;
} ReportEventMessage_t;


typedef struct
{
	GUID		guid;	// Saving GUID per channel to allow individual channel read/write over the air
	unsigned char	channelNumber;
	unsigned char	channelEnable;
	unsigned char	sensorType;
	float		highTemperatureLimit_C;
	float		lowTemperatureLimit_C;
	unsigned char	alarmTriggerTime_sec;
} ChannelConfiguration_t;


typedef struct
{
	unsigned char	messageId;		// Message id = 0x0F
	unsigned char	messageSize;		// sizeof(ChannelConfiguration)
	unsigned char	channelNumber;		// Mote’s sensor channel number {0..8}
} GetChannelConfiguration_t;


typedef struct
{
	unsigned char   messageId;
	unsigned char   messageSize;
	ChannelConfiguration_t config;
} ChannelConfigurationMessage_t;


typedef struct
{
	unsigned char   messageId;
	unsigned char   messageSize;
	unsigned char	serialNumber[kSerialNumberLength];
} SerialNumberMessage_t;


typedef struct
{
	unsigned char   messageId;
	unsigned char   messageSize;
	unsigned char	majorVerionsNumber;
	unsigned char	minorVersionNumber;
	unsigned char	patchVersionNumber;
	unsigned short	buildVersionNumber;
	dn_api_swver_t apiVersion;
} FirmwareVersionMessage_t;


typedef struct
{
	unsigned char   messageId;
	unsigned char   messageSize;
	ChannelCalibration_t calibration;
} GetChannelCalibration_t;


typedef struct
{
	unsigned char   messageId;
	unsigned char   messageSize;
	unsigned char    channelNumber;
} GetChannelMessage_t;


typedef struct
{
	unsigned char	messageId;		// Message id = 0x11
	unsigned char	messageSize;	// sizeof(ChannelConfiguration_t)
	ChannelCalibration_t calibration;
} ChannelCalibrationMessage_t;


typedef struct
{
	unsigned char  messageId;		// Message id = 0x15
	unsigned char  messageSize;		// sizeof(GetUncalibratedTemperatures _t)
	unsigned char  channelConfig;	// Bit mask of configured channels
	float  temperatures[8];	// Uncalibrated temperature reading in °C
} GetUncalibratedTemperatures_t;


typedef struct
{
	unsigned char	messageId;
	unsigned char	messageSize;
	float			measured;
	float			expected;
	float			threshold;
	int				passFail;	
} PressToTestResult_t;


//=========================== public prototypes ================================

void InitRadio( void );
void SendTemperatures(unsigned char channel, float temperature);
void SendAlarm(unsigned char channel, unsigned char error, float temperature);
void SendDigitalIOReport(unsigned char event1, unsigned char event2, unsigned char relayState);
void SendChannelConfig(unsigned char channel);
void SendSerialNumber( void );
void SendFirmwareVersion( void );
void SendChannelCalibration(unsigned char channel);
void SendBatteryHealthReport( int batteryLife, float batterImpedance );


#endif	// RADIO_H
