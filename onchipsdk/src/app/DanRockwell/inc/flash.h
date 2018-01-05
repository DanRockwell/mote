#ifndef FLASH_H
#define FLASH_H

#include <stdbool.h>


//=========================== public prototypes ================================

void InitFlash( void );
bool WriteConfigurationToFlash( unsigned char *data, unsigned length );
bool ReadConfigurationFromFlash( unsigned char *data, unsigned length );

bool WriteCalibrationToFlash( unsigned char *data, unsigned length );
bool ReadCalibrationFromFlash( unsigned char *data, unsigned length );

bool WriteSerialNumberToFlash( unsigned char *data, unsigned length );
bool ReadSerialNumberFromFlash( unsigned char *data, unsigned length );


#endif	// FLASH_H
