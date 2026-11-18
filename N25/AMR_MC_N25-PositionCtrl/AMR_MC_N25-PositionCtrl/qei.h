#ifndef __QEI_H__
#define __QEI_H__

#include "board_definition.h"

#define AUTO_SCALE_RES  6000        //4000
#define AUTO_SCALE_RES_DIV_BY_100   60

extern void InitQEI();
extern void ResetEncoder();
extern void ReadEncoder();
extern void UpdateEncoder();
extern void EncoderReset_from_MU150();

extern int encoder_send_cnt;

extern int Encoder_res;
extern int Pole_number;
extern int AUTO_SCALE_FLAG;

extern long long OldPos, OldPos2, NewPos, NewPos2, VEL, VEL2, RPM, RPM2, CUR_VEL, CUR_VEL2;
extern long long currentMotorPosition, currentMotorPosition2;
extern long long ABS_OFFSET_ENC;
extern long long HOME_ZERO_OFFSET;
extern int MU_DISK_OFFSET;


extern int cur_motor_vel;
extern int prev_motor_vel;


// MU150 & PVL
extern int MU_CALIBRATION_MODE;
extern int MU_ST_MODE;


extern int MAG_AXIS_RESET;
extern int pos_old;
extern int pos_old2;


extern int OVER_CURRENT;
extern int OVER_CURRENT_TIME;
extern int CURRENT_LIMIT;


#endif