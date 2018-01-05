#ifndef ALARM_H
#define ALARM_H

#include "app_common.h"
#include "config.h"
#include "radio.h"
#include "ltc2984.h"


#define kMaxErrorStringLength	32
#define	kMaxNumberOfTempertures	8


//=========================== structs & enums =================================

typedef enum
{
	kAlarmId_None,
	kAlarmId_OverTemperature,
	kAlarmId_UnderTemperature,
	kAlarmId_SensorHardFailure,
	kAlarmId_AdcHardFailure,
	kAlarmId_SensorColdJunctionHardFailure,
	kAlarmId_ColdJunctionSoftFailure,
	kAlarmId_SensorOverRange,
	kAlarmId_SensorUnderRange,
	kAlarmId_AdcRangeError,
	kAlarmId_ReadFailed,
	kAlarmId_CalibrationRequired,
} AlarmIds_t;


typedef struct
{
	unsigned char error;
	unsigned char errorString[kMaxErrorStringLength];
} ErrorMap_t;


//=========================== public prototypes ================================

void InitAlarm( void );
void RunAlarmManager( void );
void GetActiveAlarms( ErrorMap_t *errors );
bool IsAlarmActive(void);
bool IsDeltaTThresholdReached(void);


#endif	// ALARM_H
