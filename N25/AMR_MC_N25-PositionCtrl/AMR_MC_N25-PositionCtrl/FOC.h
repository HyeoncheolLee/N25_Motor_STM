#ifndef __FOC_H__
#define __FOC_H__

#include "board_definition.h"


extern double Esum_dd;
extern double Esum_qd;


extern long Current_R, Current_S, Current_T, RZ, SZ, TZ;
extern int ADIN_RZ, ADIN_SZ, ADIN_TZ;
extern int AD_NULLING_CNT;


extern int FrictionParam_LowVel;
extern int FrictionParam_HighVel;
extern int FrictionParam_Amp;
extern int FrictionParam_Dead_Amp;
extern int friction_compensation_onoff;


extern int measured_current_q_avg;


extern int AutoTune_CurOffset;
extern int AutoTune_InputVq;
extern int AutoTune_PosOffset;
extern int AutoTune_InputCurrent;

extern int Pgain_d1000;
extern int Igain_d1000;
extern int Pgain_q1000;
extern int Igain_q1000;

extern int Pgain_pos;
extern int Igain_pos;
extern int Dgain_pos;



extern int ref_voltage;
extern int ref_current_q;
extern int ref_current_d;
extern int measured_current_q;
extern int measured_current_d;
extern int measured_vangle;

extern int reference_current_q;


extern int DQ_SAVE;

extern int sweep_amplitude;
extern int sweep_total_time;
extern float sweep_frequency;





extern void FOC_Init_AutoTuning_Current();
extern void FOC_AutoTuning_Current();

extern void FOC_Init_CurrentControl();
//extern void FOC_CurrentControl();

extern void FOC_Init_OpenLoopControl();
extern void FOC_OpenLoopControl();

extern void DQ_align(int dq_save);
extern void CurrentNulling();

extern void FOC_CurrentControl_Double();
extern void FOC_MeasureCurrent();


#endif