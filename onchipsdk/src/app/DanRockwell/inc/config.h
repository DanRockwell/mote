#ifndef CONFIG_H
#define CONFIG_H


#include "radio.h"


//=========================== public prototypes ================================

void InitConfiguration( void );
bool GetConfiguration( unsigned char *config );
void GetChannelConfiguration( unsigned char channel, unsigned char *config );
void SetChannelConfiguration( unsigned char channel, unsigned char *config );


#endif	// CONFIG_H
