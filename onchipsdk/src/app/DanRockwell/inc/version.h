#ifndef VERSION_H
#define VERSION_H


//=========================== public prototypes ================================

#define MAJOR_VERSION   1

#define MINOR_VERSION   0

#define PATCH_VERSION   0

#define BUILD_VERSION   1


const unsigned char FirmwareVersion[4] = { MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION, BUILD_VERSION };


#endif	// VERSION_H
