#ifndef CALIBRATION_H
#define CALIBRATION_H


#include "app_common.h"
#include "radio.h"


//=========================== structs & enums =================================

#define kMaxCalibrationPoints 3


typedef struct
{
	float	measuredTemperature;
	float	actualTemperature;
} CalibrationPoint_t;


typedef struct
{
	GUID			guid;	// Saving GUID per channel to allow individual channel read/write over the air
	unsigned char	        channelNumber;
	unsigned char	        numberOfPoints;
	CalibrationPoint_t	data[kMaxCalibrationPoints];
} ChannelCalibration_t;


//=========================== public prototypes ================================

void  InitCalibration( void );
void  SetChannelCalibration(unsigned char channel, unsigned char *calibration);
void  GetChannelCalibration(unsigned char channel, unsigned char *calibration);
float GetTemperatureCalibrationValue(unsigned char channel, float inVal);


#endif	// CALIBRATION_H
