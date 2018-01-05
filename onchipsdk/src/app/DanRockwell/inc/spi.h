#ifndef SPI_H
#define SPI_H

#include <stdbool.h>

#include "dn_common.h"
#include "dn_system.h"

#include "ltc2984.h"


//**********************************************
// -- ADDRESSES --
//**********************************************
#define READ_CHANNEL_BASE	0x010
#define CHANNEL_ASSIGNMENT_BASE	0x200

//**********************************************
// -- EEPROM --
//**********************************************
#define EEPROM_START_ADDRESS 0x0B0
#define CONNECT_TO_EEPROM    0xA53C0F5A
#define WRITE_TO_EEPROM      0x95
#define READ_FROM_EEPROM     0x96


//=========================== prototypes ======================================

void	InitSPI( void );
void	AssignChannel( int channelNumber, long channelAssignmentData );
void	WriteSingleByte( short startAddress, unsigned char singleByte );
bool	ConversionDone();
void	ReadTemperatureResult( int channelNumber, AdcReading_t *data );
void	PrintFaultData( unsigned char faultByte );


#endif	// SPI_H
