#ifndef __CONTROL_H__
#define __CONTROL_H__

#include "board_definition.h"


extern int CONTROL_MODE;


extern long long REF;
extern long long REF_MUL;
extern long long PREV_REF;


extern int MAX_POS1_flag, MAX_POS2_flag;
extern long POS_LOWER_LIM, POS_UPPER_LIM;
extern float ERROR_sum;
extern float ERROR_sum_pushing;
extern long long NewInputRefDiff, NewInputRefAbs, NewInputCurrent;
extern float NewInputCurrent_f;
extern int MDR;
extern long long PREV_ERROR;
extern int LAST_VELOCITY;

extern float DT;


extern void Initialize_ControlParameter();
extern void Initialize_PWMParameter();

extern void Handler_Control_Diff();
extern void Handler_Control_Abs();


#endif