#include <proc/p32mk1024mcf064.h>
#include <sys/attribs.h>
#include <math.h>

#include "qei.h"
#include "execution.h"
#include "SV_table.h"
#include "spi.h"
#include "can.h"
#include "control.h"
#include "status.h"
#include "FOC.h"


int Encoder_res;
int Pole_number;
int AUTO_SCALE_FLAG = 0;

long long OldPos = 0, OldPos2 = 0, NewPos = 0, NewPos2 = 0, VEL = 0, VEL2 = 0, RPM = 0, RPM2 = 0, CUR_VEL = 0, CUR_VEL2 = 0;
long long currentMotorPosition = 0, currentMotorPosition2 = 0;
long long ABS_OFFSET_ENC = 0;
long long HOME_ZERO_OFFSET = 0;
int MU_DISK_OFFSET = 0;

// MU150 & PVL
int MU_CALIBRATION_MODE = 0;    // 0: Run Mode  1: Calibration Mode
int MU_ST_MODE = 10;            // 0: Abs       2: Raw      10: Multi-Turn


int MAG_AXIS_RESET = 0;

#define MV_AVG_VEL_MAX  10

static long long vel_sav[MV_AVG_VEL_MAX] = {0,};
static long long vel_sav_cnt = 0;
static long long mv_avg_vel = 0;

static long long vel_sav2[MV_AVG_VEL_MAX] = {0,};
static long long vel_sav_cnt2 = 0;
static long long mv_avg_vel2 = 0;


int pos_old = 0;
int pos_old2 = 0;
int encoder_send_cnt = 0;



int OVER_CURRENT = 15000; // 15000mA
int OVER_CURRENT_TIME = 10; // 10ms



void InitQEI(){
    
    // QEI #1 =====================================
    ANSELGbits.ANSG6 = 0;
    ANSELGbits.ANSG7 = 0;
    ANSELGbits.ANSG8 = 0;
    TRISGbits.TRISG6 = 1;
    TRISGbits.TRISG7 = 1;
    TRISGbits.TRISG8 = 1;

        
    QEA1Rbits.QEA1R = 0b1010;   // RPG6
    QEB1Rbits.QEB1R = 0b1010;   // RPG7     -->     see TABLE 13-1 INPUT PIN SELECTION
    INDX1Rbits.INDX1R = 0b1010; // RPG8
    
    QEI1CONbits.PIMOD = 0b000;  // Index not affect position counter (reset)
    QEI1CONbits.CCM = 0;        // Quadrature encoder mode
    QEI1IOCbits.FLTREN = 1;     // Digital filter enable
    //QEI1IOCbits.QFDIV = 0b100;  // 1:16 clock divider for digital filter
    QEI1IOCbits.QFDIV = 0b010;  // 1:2 clock divider for digital filter
    QEI1CONbits.QEIEN = 1;
    
    
    // QEI #2 =====================================
    ANSELEbits.ANSE15 = 0;
    ANSELEbits.ANSE14 = 0;
    TRISEbits.TRISE15 = 1;
    TRISEbits.TRISE14 = 1;

        
    QEA2Rbits.QEA2R = 0b1000;   // RPE15
    QEB2Rbits.QEB2R = 0b1000;   // RPE14     -->     see TABLE 13-1 INPUT PIN SELECTION
    
    QEI2CONbits.PIMOD = 0b000;  // Index not affect position counter (reset)
    QEI2CONbits.CCM = 0;        // Quadrature encoder mode
    QEI2IOCbits.FLTREN = 1;     // Digital filter enable
    //QEI1IOCbits.QFDIV = 0b100;  // 1:16 clock divider for digital filter
    QEI2IOCbits.QFDIV = 0b010;  // 1:2 clock divider for digital filter
    QEI2CONbits.QEIEN = 1;
}


void ResetEncoder(){
    OldPos = 0;
    OldPos2 = 0;
    POS1CNT = 0;
    POS2CNT = 0;
    pos_old = 0;
    pos_old2 = 0;
    currentMotorPosition = 0;
    currentMotorPosition2 = 0;
    MAG_AXIS = 0;
    MAG_AXIS_TEMP = 0;
    MAG_AXIS_TEMP_INIT = 0;
}


void ReadEncoder(){    
    // calculate velocity
    long long cur_vel = NewPos - OldPos;
    long long cur_vel2 = NewPos2 - OldPos2;
    
    mv_avg_vel += cur_vel - vel_sav[vel_sav_cnt];
    vel_sav[vel_sav_cnt] = cur_vel;
    vel_sav_cnt++;
    if(vel_sav_cnt >= MV_AVG_VEL_MAX)
        vel_sav_cnt = 0;
    VEL = mv_avg_vel/MV_AVG_VEL_MAX;
    CUR_VEL = cur_vel;
    RPM = VEL*1000*60/ENC_RES;
    
    mv_avg_vel2 += cur_vel2 - vel_sav2[vel_sav_cnt2];
    vel_sav2[vel_sav_cnt2] = cur_vel2;
    vel_sav_cnt2++;
    if(vel_sav_cnt2 >= MV_AVG_VEL_MAX)
        vel_sav_cnt2 = 0;
    VEL2 = mv_avg_vel2/MV_AVG_VEL_MAX;
    CUR_VEL2 = cur_vel2;
    RPM2 = VEL2*1000*60/ENC_RES;
    
    
    OldPos = NewPos;
    OldPos2 = NewPos2;
    
    
    // JAM Error Detection
    if(abs(mv_avg_vel) < 10 && abs(measured_current_q_avg) > JAM_LIM_Current){// 190801 avg
        JAM_cnt++;
        if(JAM_cnt > JAM_LIM_Time*100){
            STAT.b.JAM = 1;
        }
    }else{
        JAM_cnt = 0;
    }
    
    // Over Current Error Detection
    if(abs(measured_current_q_avg) > OVR_LIM_Current){// 190801 avg
        OVR_cnt++;
        if(OVR_cnt > OVR_LIM_Time*100){
            STAT.b.CUR = 1;
        }
    }else{
        OVR_cnt = 0;
    }
    
    
    
#ifndef NO_NEW_FUNCTION
    // Over Current Detect -- 1ms    
    if(fabs(Current_R*CURRENT_CONSTANT) > OVER_CURRENT){
        R_cnt++;
        if(R_cnt > OVER_CURRENT_TIME){
            STAT.b.CUR = 1;
        }
    }else{
        R_cnt = 0;
    }
    if(fabs(Current_S*CURRENT_CONSTANT) > OVER_CURRENT){
        S_cnt++;
        if(S_cnt > OVER_CURRENT_TIME){
            STAT.b.CUR = 1;
        }
    }else{
        S_cnt = 0;
    }
    if(fabs(Current_T*CURRENT_CONSTANT) > OVER_CURRENT){
        T_cnt++;
    }else{
        T_cnt = 0;
        if(T_cnt > OVER_CURRENT_TIME){
            STAT.b.CUR = 1;
        }
    }

    
    // Current Big Error Detection
    if(abs(measured_current_q_avg-reference_current_q) > CUR_BIG_LIM){
        CUR_BIG_cnt++;
        if(CUR_BIG_cnt > CUR_BIG_Time*100){
            STAT.b.CUR_BIG = 1;
        }
    }else{
        CUR_BIG_cnt = 0;
        //STAT.b.CUR_BIG = 0;
    }
#endif
}


static int prev_motorPosition = 0;
void UpdateEncoder(){
    //uiuk modified
    int pos_temp;
    int pos_temp2;
    int temp_motor_vel;
    int temp_motor_vel2;
    
    pos_temp = POS1CNT;
    pos_temp2 = POS2CNT;
    
    if(MDR == 0){
        temp_motor_vel = -(pos_temp - pos_old);
        temp_motor_vel2 = -(pos_temp2 - pos_old2);
    }else{
        temp_motor_vel = pos_temp - pos_old;
        temp_motor_vel2 = pos_temp2 - pos_old2;
    }
    currentMotorPosition = currentMotorPosition + (long long)(temp_motor_vel);
    currentMotorPosition2 = currentMotorPosition2 + (long long)(temp_motor_vel2);
    
    MAG_AXIS_TEMP -= (pos_temp - pos_old);
    MAG_AXIS = MAG_AXIS_TEMP;
    
    
    if(MDR == 0){
        NewPos = currentMotorPosition;
        NewPos2 = currentMotorPosition2;
    }else {
        NewPos = currentMotorPosition;
        NewPos2 = currentMotorPosition2;
    }
    
    pos_old = pos_temp;
    pos_old2 = pos_temp2;
}


void EncoderReset_from_MU150(){
    POS1CNT = 0;
    POS2CNT = 0;
    pos_old = 0;
    pos_old2 = 0;
    
//    int tempMU150_1 = MU150_ReadSensor_MultiTurn()/8;
//    int tempMU150_2 = MU150_ReadSensor_MultiTurn()/8;
//    int tempMU150 = (tempMU150_1+tempMU150_2)/2;
    
    // uiuk
    
    //int rawMU150 = MU150_ReadSensor_MultiTurn();
    int rawMU150 = MU150_ReadSensor();
    int tempMU150 = rawMU150/8;
        
    long long modiMU150 = (long long)rawMU150;// + MU_DISK_OFFSET;
//    int multi_turn_modi = 0;
//    if(modiMU150 > 131072){
//        modiMU150 -= 131072;
//        multi_turn_modi++;
//    }
//    if(modiMU150 < 0){
//        modiMU150 += 131072;
//        multi_turn_modi--;
//    }
    modiMU150 = modiMU150/8;
    
    MAG_AXIS_TEMP = (tempMU150-InitPos_DQ_MU);
    MAG_AXIS_TEMP_INIT = MAG_AXIS_TEMP;
    
    
    if(MDR == 0){
        currentMotorPosition = modiMU150;
        currentMotorPosition2 = currentMotorPosition;
    }else{
        modiMU150 = 16383 - modiMU150;
        currentMotorPosition = modiMU150;
        currentMotorPosition2 = currentMotorPosition;
    }
}