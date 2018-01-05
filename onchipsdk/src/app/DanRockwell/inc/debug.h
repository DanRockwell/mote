#ifndef DEBUG_H
#define DEBUG_H


//=========================== defines ==========================================
// Debug level determines how many messages are printed
#define DBG_LEVEL_ALL		0		// Detailed debugging down to the data level
#define DBG_LEVEL_DEBUG		1		// Detailed debugging down to message level
#define DBG_LEVEL_DEBUG1	2		// Detailed Debugging Flow 
#define DBG_LEVEL_INFO		3		// General Flow and informational
#define DBG_LEVEL_WARN		4		// Out of the ordinary events that should be flagged
#define DBG_LEVEL_ERROR  	7		// Error events that should always be displayed
#define DBG_LEVEL_NONE  	255		// Turn off all debug messages


//=========================== public prototypes ================================
void InitDebug();
void DbgPrint(int level, const char *fmtstring, ...);


#endif	// DEBUG_H
