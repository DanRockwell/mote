#ifndef GPIO_H
#define GPIO_H


#include "dn_system.h"


// Pin States
#define PIN_HIGH			0x01
#define PIN_LOW				0x00

// Digital I/O Pins
#define PIN_EVENT1_IN		DN_GPIO_PIN_23_DEV_ID
#define PIN_EVENT2_IN		DN_GPIO_PIN_17_DEV_ID
#define PIN_RELAY_SET		DN_GPIO_PIN_22_DEV_ID
#define PIN_RELAY_RESET		DN_GPIO_PIN_21_DEV_ID


//=========================== public prototypes ================================

void  InitGPIO( void );
void  GpioConfigPin( dn_device_t pin, unsigned char mode, unsigned char config );
void  GpioWrite( dn_device_t pin, const unsigned char value );
unsigned char GpioRead( dn_device_t pin );
void  GetEventData( unsigned char *event1, unsigned char *event2, unsigned char *relayState );
void  SetAlarmOut( void );
void  ClearAlarmOut( void );


#endif	// GPIO_H
