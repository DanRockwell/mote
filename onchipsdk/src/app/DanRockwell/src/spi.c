/******************************************************************************
*	Copyright (c) 2016, Mack Informatcion Systems, In.  All rights reserved.
*
*	SPI interface control
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
#include "dn_common.h"
#include "dn_system.h"
#include "dn_gpio.h"
#include "dn_spi.h"
#include "dn_system.h"

// App includes
#include "debug.h"
#include "spi.h"
#include "gpio.h"
#include "ltc2984.h"


//=========================== definitions =====================================

#define CHANNEL_DATA_SIZE	4
#define NUMBER_OF_CHANNELS	9
#define SPI_TX_BUFFER_LENGTH	1
#define SPI_RX_BUFFER_LENGTH	4


//=========================== variables =======================================

static dn_spi_open_args_t	mSpiOpenArgs;
static dn_ioctl_spi_transfer_t	mSpiTransfer;

// Each SPI transaction is required to write into RX buffer on a word boundary,
// so force word alignment is needed for the start address of the rx buffer
#pragma data_alignment = 4
unsigned char mSpiRxBuffer[SPI_RX_BUFFER_LENGTH];
unsigned char mSpiTxBuffer[SPI_TX_BUFFER_LENGTH];


//=========================== local functions =======================================

static void             InitializeMemoryWrite( short startAddress );
static void             WriteCoefficient( long coeff );
static void	        ConfigureChipSelect( void );
static void             SpiWrite( unsigned char dataByte );
static unsigned char    SpiRead( void );
static void             AssertChipSelect( void );
static void             DeassertChipSelect( void );
static void             InitializeMemoryRead( short startAddress );
static void             GetRawResults( short baseAddress, int channelNumber, unsigned char results[4] );
static float            ConvertToSignedFloat( unsigned char results[4] );
static float            GetTemperature( float value );
static unsigned char    HighByte( short data );
static char             LowByte( short data );


//*****************************************************************************
//						SPI Initialization
//*****************************************************************************

/**
 *  SPI interface initialization.
 */
void InitSPI( void )
{
    // Initialize chip select pin, which is controlled manually
    ConfigureChipSelect();

    // Initialize SPI, open the SPI device
    mSpiOpenArgs.maxTransactionLenForCPHA_1 = 0;
    int err = dn_open( DN_SPI_DEV_ID, 
                       &mSpiOpenArgs,
                       sizeof(mSpiOpenArgs) );
      
    // Initialize spi communication parameters
    mSpiTransfer.txData		= &mSpiTxBuffer[0];
    mSpiTransfer.rxData		= &mSpiRxBuffer[0];
    mSpiTransfer.transactionLen = SPI_TX_BUFFER_LENGTH;
    mSpiTransfer.numSamples	= 1;
    mSpiTransfer.startDelay	= 0;
    mSpiTransfer.clockPolarity	= DN_SPI_CPOL_0;
    mSpiTransfer.clockPhase	= DN_SPI_CPHA_0;
    mSpiTransfer.bitOrder	= DN_SPI_MSB_FIRST;
    mSpiTransfer.slaveSelect	= DN_SPIM_SS_0n;
    mSpiTransfer.clockDivider	= DN_SPI_CLKDIV_16;
    mSpiTransfer.rxBufferLen	= SPI_RX_BUFFER_LENGTH;
}


//*****************************************************************************
//      Memory read functions
//*****************************************************************************

/**
 *  Write configuration data for selected channel
 */
void AssignChannel( int channelNumber, long channelAssignmentData )
{
    short startAddress = CHANNEL_ASSIGNMENT_BASE + (4 * channelNumber);

    InitializeMemoryWrite( startAddress );
    WriteCoefficient( channelAssignmentData );
    DeassertChipSelect();
}


/**
 *  Write single byte to selected address
 */
void WriteSingleByte( short startAddress, unsigned char singleByte )
{
    InitializeMemoryWrite( startAddress );
    SpiWrite( singleByte );
    DeassertChipSelect();
}


/**
 *	Start a write operation by asserting chip select and writting address
 */
static void InitializeMemoryWrite( short startAddress )
{
    AssertChipSelect();			// Lower chip select line
    SpiWrite( 0x02 );			// Instruction Byte Write
    SpiWrite( HighByte(startAddress) );	// Address MSB Byte
    SpiWrite( LowByte(startAddress) );	// Address LSB Byte
}


/**
 *  Support function to write multiple bytes to device
 */
static void WriteCoefficient( long coeff )
{
    int bytesPerCoeff = 4;
    unsigned char dataByte;
    int i;
    for( i = bytesPerCoeff - 1; i >= 0; i-- )
    {
	dataByte = (unsigned char)(coeff >> (i * 8));
	SpiWrite( dataByte );
    }
}


//*****************************************************************************
//      Memory read functions
//*****************************************************************************

/**
*  Read status register and return conversion status
*/
bool ConversionDone( void )
{
    InitializeMemoryRead( 0 );
    unsigned char data = SpiRead();
    DeassertChipSelect();
    return( data & 0x40 );
}


/**
 *  Assert chip select, write command and address for read	
 */
static void InitializeMemoryRead( short startAddress )
{
    AssertChipSelect();			// Lower chip select line
    SpiWrite( 0x03 );			// instruction Byte read
    SpiWrite( HighByte(startAddress) );	// Address MSB Byte
    SpiWrite( LowByte(startAddress) );	// Address LSB Byte
}


/**
 *  Read temperature results in SI units along with convesion status
 */
void ReadTemperatureResult( int channelNumber, AdcReading_t *data )
{
    unsigned char results[4];
    GetRawResults( READ_CHANNEL_BASE, channelNumber, results );
    float signedFloat = ConvertToSignedFloat( results );
    data->temperature = GetTemperature( signedFloat );
    data->status      = results[3];
}

/**
 *  Read 4 byte conversion result for selected channel
 */
static void GetRawResults( short baseAddress, int channelNumber, unsigned char results[4] )
{
    short address = baseAddress + (4 * channelNumber);
    InitializeMemoryRead( address );

    results[3] = SpiRead();     // fault data
    results[2] = SpiRead();     // MSB result byte
    results[1] = SpiRead();     // 2nd result byte
    results[0] = SpiRead();     // LSB result byte
    DeassertChipSelect();
}


/**
 *  Get the last 24 bits of the results (the first 8 bits are status bits)
 */
static float ConvertToSignedFloat( unsigned char results[4] )
{
    long value = 0L;
    value = value 
          | ((unsigned long)results[2] << 16)
          | ((unsigned long)results[1] << 8)
	  | ((unsigned long)results[0]);

    // Convert a 24-bit two's complement number into a 32-bit two's complement number
    if( (results[2] & 0x80) == 128 )
    {
        // Convert to negative
	value = value | 0xFF000000;
    }

    return( (float)value );
}


/**
 *  The temperature format is (14, 10) so we divide by 2^10
 */
static float GetTemperature( float value )
{
    return( value / 1024 );
}


// Translate the fault byte into usable fault data and print it out
void PrintFaultData( unsigned char faultByte )
{
    if ((faultByte & SENSOR_HARD_FAILURE) > 0)	DbgPrint(DBG_LEVEL_ERROR, "ERROR. LTC2984: SENSOR HARD FALURE\r\n");
    if ((faultByte & ADC_HARD_FAILURE) > 0)	DbgPrint(DBG_LEVEL_ERROR, "ERROR. LTC2984: ADC_HARD_FAILURE\r\n");
    if ((faultByte & CJ_HARD_FAILURE) > 0)	DbgPrint(DBG_LEVEL_ERROR, "ERROR. LTC2984: CJ_HARD_FAILURE\r\n");
    if ((faultByte & CJ_SOFT_FAILURE) > 0)	DbgPrint(DBG_LEVEL_ERROR, "ERROR. LTC2984: CJ_SOFT_FAILURE\r\n");
    if ((faultByte & SENSOR_ABOVE) > 0)		DbgPrint(DBG_LEVEL_ERROR, "ERROR. LTC2984: SENSOR_ABOVE\r\n");
    if ((faultByte & SENSOR_BELOW) > 0)		DbgPrint(DBG_LEVEL_ERROR, "ERROR. LTC2984: SENSOR_BELOW\r\n");
    if ((faultByte & ADC_RANGE_ERROR) > 0)	DbgPrint(DBG_LEVEL_ERROR, "ERROR. LTC2984: ADC RANGE ERROR\r\n");
    if ((faultByte & VALID) != 1)		DbgPrint(DBG_LEVEL_ERROR, "ERROR. LTC2984: INVALID READING\r\n");
    if (faultByte == 0xff)			DbgPrint(DBG_LEVEL_ERROR, "ERROR. LTC2984: CONFIGURATION ERROR\r\n");
}


/**
 *  Write bytes to SPI interface
 */
static void SpiWrite( unsigned char dataByte )
{
    mSpiTxBuffer[0] = dataByte;
    dn_ioctl( DN_SPI_DEV_ID,
              DN_IOCTL_SPI_TRANSFER,
              &mSpiTransfer,
              sizeof(mSpiTransfer) );
}


/**
 *  Returns the data byte read
 */
static unsigned char SpiRead( void ) //!The data byte to be written
{
    mSpiTxBuffer[0] = 0;
    dn_ioctl( DN_SPI_DEV_ID,
              DN_IOCTL_SPI_TRANSFER,
              &mSpiTransfer,
              sizeof(mSpiTransfer) );
	
    return( (unsigned char)mSpiRxBuffer[0] );
}


/**
 *  The micros SPI SS only supports 8 and 16 bit transfers.
 *  A manually control chip select is being used to support the 
 *  LTC2984 multi-byte transfer requirements.
 */
static void ConfigureChipSelect( void )
{
    GpioConfigPin( PIN_CS, DN_IOCTL_GPIO_CFG_OUTPUT, PIN_HIGH );
}


/**
 *  Assert chip select by writing 0 to pin
 */
static void AssertChipSelect( void )
{
    GpioWrite( PIN_CS, PIN_LOW );
}


/**
 *  Deassert chip select by writing 1 to pin
 */
static void DeassertChipSelect( void )
{
    GpioWrite( PIN_CS, PIN_HIGH );
}


static unsigned char HighByte( short data )
{
  return( (unsigned char)(data >> 8) );
}


static char LowByte( short data )
{
    return( (unsigned char)(data && 0xff) );
}
