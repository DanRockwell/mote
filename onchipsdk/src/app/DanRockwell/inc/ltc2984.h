#ifndef LTC2984_H
#define LTC2984_H


#include "ltc2984_cfg.h"


//=========================== structs & enums =================================

#define	TOTAL_NUMBER_OF_CHANNELS	(8 + 1)
#define SAMPLE_AVERAGE_SIZE		5
#define	LAST_LTC2984_CHANNEL		20

// SPI
#define PIN_MISO			DN_GPIO_PIN_11_DEV_ID
#define PIN_MOSI			DN_GPIO_PIN_10_DEV_ID
#define PIN_SCK				DN_GPIO_PIN_9_DEV_ID
#define PIN_SS				DN_GPIO_PIN_12_DEV_ID	// Not used (only supports 8 and 16 bit transfer)
#define PIN_CS				DN_GPIO_PIN_19_DEV_ID	// Custom/bit bang chip select for variable length transfers

// Control Pins
#define PIN_RESET			DN_GPIO_PIN_20_DEV_ID
#define PIN_INTERRUPT		        DN_GPIO_PIN_0_DEV_ID


typedef struct
{
	unsigned char	status;
	float		temperature;
} AdcReading_t;


// 8 - data channels and 1 test channel
typedef struct
{
    AdcReading_t channel[TOTAL_NUMBER_OF_CHANNELS];
} LTC2984Data_t;


typedef struct
{
    AdcReading_t        currentReading[TOTAL_NUMBER_OF_CHANNELS];
    float               averagingQueue[TOTAL_NUMBER_OF_CHANNELS][SAMPLE_AVERAGE_SIZE];
    float               temperatureAverage[TOTAL_NUMBER_OF_CHANNELS];
    int		        initialized[TOTAL_NUMBER_OF_CHANNELS];
    int		        readIndex;
} TemperatureData_t;


//=========================== public  prototypes ======================================

void InitLTC2984( void );
void AcquireTemperatureReadings( void );
void GetCurrentTemperatures( LTC2984Data_t *temperatureReadings );
void UpdateADCConfiguration( void );
void GetChannelReading(unsigned char channelNumber, AdcReading_t *reading);
void GetRawTemperatureReadings(unsigned char *channelConfig, float *temperatures);
void CheckTemperatureTestChannel(float *measured, float *expected, float *threshold );
const char *ChannelFault( unsigned char status );


#endif	// LTC2984_H
