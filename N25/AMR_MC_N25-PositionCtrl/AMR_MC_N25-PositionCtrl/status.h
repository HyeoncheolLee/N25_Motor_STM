#ifndef __STATUS_H__
#define __STATUS_H__

#include "board_definition.h"

// Status Error Definition
#define STAT_ERR_OK         0x00
#define STAT_ERR_JAM        0x01
#define STAT_ERR_INP        0x02
#define STAT_ERR_PWM        0x03
#define STAT_ERR_BIG        0x04
#define STAT_ERR_FLG        0x05
#define STAT_ERR_CUR        0x06
#define STAT_ERR_ENC        0x07
#define STAT_ERR_TMP        0x08


// Board Type Definition
#define BTY_2CH_SYNC        0x01
#define BTY_2CH_INDI        0x02
#define BTY_1CH             0x04


#define SET_STATUS_FLAG     STATUS_FLAG = 1;

#define INIT_BOARD_MODE_OK              0
#define INIT_BOARD_MODE_BNO_CHANGED     1
#define INIT_BOARD_MODE_RESET_BTN       2
#define INIT_BOARD_MODE_RESTORE         3

extern int      BOARD_TYPE;
extern int      JMC;    // board id
extern int      INIT_BOARD_MODE;
extern int      NOC;    // number of channel
extern int      NOM;    // number of motor
extern char     BoardName[13];

extern mSTAT    STAT;
extern int      STATUS_FLAG;

// error --------------------
extern int T_ERR;
extern long long I_ERR, B_ERR;
extern unsigned long long JAM_cnt, OVR_cnt, CUR_BIG_cnt, R_cnt, S_cnt, T_cnt;
extern unsigned long long JAM_LIM_Current, OVR_LIM_Current, JAM_LIM_Time, OVR_LIM_Time, CUR_BIG_LIM, CUR_BIG_Time;





extern void Handler_Status(unsigned long cnt);
extern void Handler_Error(unsigned long cnt);



#endif