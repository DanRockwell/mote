#ifndef APP_COMMON_H
#define APP_COMMON_H

#include <stdbool.h>
#include "debug.h"


//=========================== structs & enums =================================

#define	kGuidByteLength 16

typedef struct
{
    unsigned char data[kGuidByteLength];
} GUID;



#endif	// APP_COMMON_H
