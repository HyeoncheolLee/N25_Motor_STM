#ifndef __INIT_H__
#define __INIT_H__

#include "board_definition.h"

#define DEFAULT_TEMP_ERROR          60

#define DEFAULT_ENC_RES             16384


#define DEFAULT_POS_LOWER_LIMIT     -4194304
#define DEFAULT_POS_UPPER_LIMIT     4194304

#define DEFAULT_CURRENT_NULL_RZ     2048
#define DEFAULT_CURRENT_NULL_SZ     2048
#define DEFAULT_CURRENT_NULL_TZ     2048




extern void Handler_Init();
extern void InitBoardIdentification();
extern void InitPeripheral();

extern void InitROM(int mode);


#endif