#include <proc/p32mk1024mcf064.h>
#include <sys/attribs.h>
#include <math.h>
#include <string.h>

#include "FOC.h"
#include "qei.h"
#include "SV_table.h"
#include "timer.h"
#include "control.h"
#include "spi.h"
#include "pwm.h"
#include "init.h"
#include "execution.h"
#include "adc.h"
#include "can.h"
#include "status.h"


//static int my_atan2(int y, int x);

long Current_R, Current_S, Current_T, RZ, SZ, TZ;
int ADIN_RZ, ADIN_SZ, ADIN_TZ;
int AD_NULLING_CNT = 0;
long long MT_ON, MAG_AXIS;

// uiuk added
long long MAG_AXIS_TEMP, MAG_AXIS_TEMP_INIT;

int FrictionParam_LowVel = 2;
int FrictionParam_HighVel = 20;
int FrictionParam_Amp = 100;
int FrictionParam_Dead_Amp = 100;
int friction_compensation_onoff = 0;


#define MV_AVG_MEAS_Q_MAX   5
static int meas_q_sav[MV_AVG_MEAS_Q_MAX] = {0,};
static int meas_q_sav_cnt = 0;
static int mv_avg_meas_q = 0;
int measured_current_q_avg;

int reference_current_q = 0;


int ref_voltage = 0;

int ref_current_q = 0;      // mA
int ref_current_d = 0;      // mA
int measured_current_q = 0; // mA
int measured_current_d = 0;

static int Esum_d = 0;
static int Esum_q = 0;
double Esum_dd = 0.0;
double Esum_qd = 0.0;

int Pgain_d1000 = DEFAULT_FOC_PGAIN;
int Igain_d1000 = DEFAULT_FOC_IGAIN;
int Pgain_q1000 = DEFAULT_FOC_PGAIN;
int Igain_q1000 = DEFAULT_FOC_IGAIN;


int Pgain_pos = DEFAULT_POS_PGAIN;
int Igain_pos = DEFAULT_POS_IGAIN;
int Dgain_pos = DEFAULT_POS_DGAIN;


int DQ_SAVE = 0;

int send_toggle = 0;
static void SPACE_VECTOR_COMMUTATION(int Vref, int Vangle);

    


void DQ_align(int dq_save)
{
    int i, j;
    MT_ON = 0;    

    CONTROL_MODE = CONTROL_MODE_NO_ACT;
    //MOTOR_HIP_ON
    GateDriver_ON();
        
    
    if(dq_save != 2){
        CurrentNulling();
        GateDriver_ON();
    }

    
#ifdef NEW_AMP2
    PDC1 = PWM_RES-50;
    PDC2 = PWM_RES;
    PDC3 = PWM_RES;
#else
    PDC1 = 50;
    PDC2 = 0;
    PDC3 = 0;
#endif
    
        
    for(i=0; i<1000; ++i)   
        for(j=0; j<300; ++j)
            ;
        
#ifdef NEW_AMP2
    PDC1 = PWM_RES-100;
    PDC2 = PWM_RES;
    PDC3 = PWM_RES;
#else
    PDC1 = 100;
    PDC2 = 0;
    PDC3 = 0;
#endif
    
        
    for(i=0; i<1000; ++i)   
        for(j=0; j<300; ++j)
            ;
        
#ifdef NEW_AMP2
    PDC1 = PWM_RES-150;
    PDC2 = PWM_RES;
    PDC3 = PWM_RES;
#else
    PDC1 = 150;
    PDC2 = 0;
    PDC3 = 0;
#endif
    
        
    for(i=0; i<1000; ++i)   
        for(j=0; j<300; ++j)
            ;
        
#ifdef NEW_AMP2
    PDC1 = PWM_RES-200;
    PDC2 = PWM_RES;
    PDC3 = PWM_RES;
#else
    PDC1 = 200;
    PDC2 = 0;
    PDC3 = 0;
#endif
    
    for(i=0; i<1000; ++i)   
        for(j=0; j<600; ++j)
            ;
  
        
    if(dq_save == 1){
        DQ_SAVE = 1;
    }
    
    
    if(dq_save == 2){
        //MAG_AXIS_RESET = 1;
        //MAG_AXIS = 0;
        
        // uiuk modified
        MAG_AXIS_TEMP = POS1CNT - pos_old;
        //MAG_AXIS = POS1CNT - pos_old;
//        ResetEncoder();
    }else{
        ResetEncoder();
    }
    
            
    MT_ON = 1;
    
    //Ir2131_OFF();
#ifdef NEW_AMP2
    PDC1 = PWM_RES;
    PDC2 = PWM_RES;
    PDC3 = PWM_RES;
#else
    PDC1 = 0;
    PDC2 = 0;
    PDC3 = 0;
#endif
    
    
    
//    int tempMU150 = MU150_ReadSensor_MultiTurn()/8;
//
//    unsigned char canTxData[8] = {0,};
//    canTxData[0] = tempMU150&0xFF;
//    canTxData[1] = (tempMU150>>8)&0xFF;
//    canTxData[2] = (tempMU150>>16)&0xFF;
//    canTxData[3] = (tempMU150>>24)&0xFF;
//    Push_CAN_TX_Ring(705, 4, canTxData);

}



extern float temp_old;

void FOC_Init_CurrentControl(){
    Esum_d = 0;
    Esum_q = 0;
    Esum_dd = 0.0;
    Esum_qd = 0.0;
    
    IOCON1CLR = 0x0300;
    IOCON2CLR = 0x0300;
    IOCON3CLR = 0x0300;
}

int can_toggle = 0;
extern long enc_sav;
extern long enc_sav_old;

//void FOC_CurrentControl(){
//    int SINA1000, COSA1000;
//    int I_d, I_q;
//    int E_d, E_q;
//    int Vd, Vq;
//    int Valpha, Vbeta;
//    int ref_current_q_modi = 0;
//    
//    if(MT_ON == 0)
//        return;
//    
//    if(friction_compensation_onoff == 1 && CONTROL_MODE == CONTROL_MODE_FOC_CUR_CONTROL){
//        int fric_amp = FrictionCompensation();
//        ref_current_q_modi = ref_current_q + fric_amp;
//    }else{
//        ref_current_q_modi = ref_current_q;
//    }
//    
//    if(MDR == 1){
//        ref_current_q_modi = -ref_current_q_modi;
//    }
//    
//    
//    
//    
//    Current_R = ADCDATA0 - RZ;
//    Current_S = ADCDATA1 - SZ;
//    Current_T = -Current_R-Current_S; 
//    
//    
//    // Clarke Transformation ----- (a, b, c --> alpha, beta)
//#ifdef NEW_CURRENT_SENSING
//    // I * R * G * (R1/R2+R1) = V   --->   I = V/R/G/(R1/R2+R1)
//    // (3.3/4096) / 0.01 / 20 / (2.1/3.1) = 5.94657 (mA/digit)
//    // (3.3/4096) / 0.01 / 20  = 4.02832 (mA/digit)
//    // (3.3/4096) / 0.005 / 20  = 8.05664 (mA/digit)
//    // (3.3/4096) / 0.003 / 20  = 13.42773 (mA/digit)
//    // (3.3/4096) / 0.002 / 20 = 20.1416 (mA/digit)
//#ifdef BOARD_TYPE_25
////    float alpha = 1343 * Current_R/100;
////    float beta = 1343 * 577 * (Current_S - Current_T)/1000/100;
//    float alpha = 2014 * Current_R/100;
//    float beta = 2014 * 577 * (Current_S - Current_T)/1000/100;
//#endif
//#ifdef BOARD_TYPE_14
//    float alpha = 403 * Current_R/100;
//    float beta = 403 * 577 * (Current_S - Current_T)/1000/100;
//#endif
//#else
//    // 3.7/2.7   *   3.3/4096   *   10   *   1000  =  11.04058 (mA/digit)
//    // v-divider     volt/adc   sensitivity   mA
//    int alpha = 11 * Current_R;
//    int beta = 11 * 577 * (Current_S - Current_T)/1000;
//#endif
//    
//    // get Rotor Flux Position
////    int I_theta = (long)(((MAG_AXIS%ENC_RES)*3600/(ENC_POLE_VAL)))%3600;      // Maxon -MAG_AXIS --> MAG_AXIS
////    if(I_theta < 0) I_theta += 3600;
//    float I_theta = fmod(((MAG_AXIS%ENC_RES)*360.0/(ENC_POLE_VAL)), 360.0);
//    if(I_theta < 0) I_theta += 360.0;
//    
//    // Park Transformation ----- (alpha, beta --> d, q)
//    SINA1000 = SIN_T1000[(int)(I_theta*10)];
//    COSA1000 = COS_T1000[(int)(I_theta*10)];
////    SINA1000 = SIN_T1000[I_theta];
////    COSA1000 = COS_T1000[I_theta];
//    I_d = (COSA1000*alpha + SINA1000*beta)/1000;    // mA
//    I_q = (-SINA1000*alpha + COSA1000*beta)/1000;   // mA
//    measured_current_q = I_q;
//    
//    // PI current control -----
//    E_d = ref_current_d - I_d;  // mA
//    E_q = ref_current_q_modi - I_q;  // mA
//    Esum_d += E_d;
//    Esum_q += E_q;
//    
//    
//    Vq = (Pgain_q1000*E_q + Igain_q1000*Esum_q)/1000;// + Vq_ff;   // mV
//    Vd = (Pgain_d1000*E_d + Igain_d1000*Esum_d)/1000;// + Vd_ff;   // mV
//    int tempMag = (int)(sqrt(Vq/10*Vq/10+Vd/10*Vd/10)*10);
//    if(tempMag > 48000){
//        int prev_Vq = Vq;
//        int prev_Vd = Vd;
//        Vq = Vq*480/tempMag*100;
//        Vd = Vd*480/tempMag*100;
//        Esum_q += (Vq - prev_Vq)*Igain_q1000/Pgain_q1000;
//        Esum_d += (Vd - prev_Vd)*Igain_d1000/Pgain_d1000;
//    }
//        
//    // Inverse Park Transformation ----- (d, q --> alpha, beta)
//    Valpha = (Vd*COSA1000 - Vq*SINA1000)/1000;      // mV
//    Vbeta = (Vd*SINA1000 + Vq*COSA1000)/1000;       // mV
//    
//    
//    int Vref = (int)(sqrt(Valpha/10*Valpha/10 + Vbeta/10*Vbeta/10)*10);    // mV
//    int v_angle = my_atan2(Vbeta, Valpha);   
//    if(v_angle < 0) v_angle = 3600 + v_angle;
//        
//    SPACE_VECTOR_COMMUTATION(Vref, v_angle);
//}





void FOC_Init_OpenLoopControl(){
    IOCON1CLR = 0x0300;
    IOCON2CLR = 0x0300;
    IOCON3CLR = 0x0300;
}

void FOC_OpenLoopControl(){
    static int openloop_not_moving_cnt = 0;
    int temp_ref_voltage = 0;
    
    if(MT_ON == 0)
        return;

    // get Rotor Flux Position
    int I_theta = (long)(((MAG_AXIS%ENC_RES)*3600/(ENC_POLE_VAL)))%3600;      // Maxon -MAG_AXIS --> MAG_AXIS
    if(I_theta < 0) I_theta += 3600;
    int v_angle;
    
    if(MDR == 1)
        temp_ref_voltage = -ref_voltage;
    else
        temp_ref_voltage = ref_voltage;
    
    if(temp_ref_voltage>=0)
        v_angle = (I_theta+900)%3600;
    else
        v_angle = (I_theta-900)%3600;
    if(v_angle < 0) 
        v_angle = 3600+v_angle;
    
    
    if(CONTROL_MODE == CONTROL_MODE_FOC_OPENLOOP_CONTROL){
        if(abs(temp_ref_voltage) > 1920){   // 48000 * 4% = 1920
            if(abs(VEL) < 1){
                openloop_not_moving_cnt++;
                if(openloop_not_moving_cnt > 4000){ // 200ms at 20K control
                    STAT.b.BIG = 1;
                    openloop_not_moving_cnt = 0;
                }
            }else{
                openloop_not_moving_cnt = 0;
            }
        }else{
            openloop_not_moving_cnt = 0;
        }
    }
    
    SPACE_VECTOR_COMMUTATION(abs(temp_ref_voltage), v_angle);
}




void SPACE_VECTOR_COMMUTATION_Double(float Vref, float Vangle){
    int sector_num = (int)Vangle/600 + 1;
    
    if(Vref>48000)
        Vref = 48000;
    
    int T1 = 0.866025*PWM_RES*Vref/48000.0*SIN_T1000[600*sector_num-(int)Vangle]/1000.0;
    int T2 = 0.866025*PWM_RES*Vref/48000.0*SIN_T1000[(int)Vangle - 600*(sector_num-1)]/1000.0;
    int T0 = PWM_RES-T1-T2;
    
    int pdc1 = 0, pdc2 = 0, pdc3 = 0;
    switch(sector_num){
    case 1:
        pdc1 = T1+T2+T0/2;
        pdc2 = T2+T0/2;
        pdc3 = T0/2;
        break;
    case 2:
        pdc1 = T1+T0/2;
        pdc2 = T1+T2+T0/2;
        pdc3 = T0/2;
        break;
    case 3:
        pdc1 = T0/2;
        pdc2 = T1+T2+T0/2;
        pdc3 = T2+T0/2;
        break;
    case 4:
        pdc1 = T0/2;
        pdc2 = T1+T0/2;
        pdc3 = T1+T2+T0/2;
        break;
    case 5:
        pdc1 = T2+T0/2;
        pdc2 = T0/2;
        pdc3 = T1+T2+T0/2;
        break;
    case 6:
        pdc1 = T1+T2+T0/2;
        pdc2 = T0/2;
        pdc3 = T1+T0/2;
        break;
    }                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    
//    if(pdc1 > 2995)
//        pdc1 = 2995;
//    if(pdc2 > 2995)
//        pdc2 = 2995;
//    if(pdc3 > 2995)
//        pdc3 = 2995;
    
    
//    int PWM_SLOPE_LIMIT = 200;
//    
//    if((pdc1 - (int)PDC1) > PWM_SLOPE_LIMIT)
//        pdc1 = PDC1 + PWM_SLOPE_LIMIT;
//    else if(((int)PDC1 - pdc1) > PWM_SLOPE_LIMIT)
//        pdc1 = PDC1 - PWM_SLOPE_LIMIT;
//    
//    if((pdc2 - (int)PDC2) > PWM_SLOPE_LIMIT)
//        pdc2 = PDC2 + PWM_SLOPE_LIMIT;
//    else if(((int)PDC2 - pdc2) > PWM_SLOPE_LIMIT)
//        pdc2 = PDC2 - PWM_SLOPE_LIMIT;
//    
//    if((pdc3 - (int)PDC3) > PWM_SLOPE_LIMIT)
//        pdc3 = PDC3 + PWM_SLOPE_LIMIT;
//    else if(((int)PDC3 - pdc3) > PWM_SLOPE_LIMIT)
//        pdc3 = PDC3 - PWM_SLOPE_LIMIT;
    
#ifdef NEW_AMP2
    PDC1 = PWM_RES-pdc1;
    PDC2 = PWM_RES-pdc2;
    PDC3 = PWM_RES-pdc3;
#else
    PDC1 = pdc1;
    PDC2 = pdc2;
    PDC3 = pdc3;
#endif
}




void SPACE_VECTOR_COMMUTATION(int Vref, int Vangle){
    int sector_num = Vangle/600 + 1;
    
    if(Vref>48000)
        Vref = 48000;
//    int T1 = 866*3000/1000*Vref/1000*SIN_T1000[600*sector_num-Vangle]/48000;
//    int T2 = 866*3000/1000*Vref/1000*SIN_T1000[Vangle - 600*(sector_num-1)]/48000;
//    int T0 = 3000-T1-T2;
    int T1 = 866*PWM_RES/1000*Vref/1000*SIN_T1000[600*sector_num-Vangle]/48000;
    int T2 = 866*PWM_RES/1000*Vref/1000*SIN_T1000[Vangle - 600*(sector_num-1)]/48000;
    int T0 = PWM_RES-T1-T2;
        
    int pdc1 = 0, pdc2 = 0, pdc3 = 0;
    switch(sector_num){
    case 1:
        pdc1 = T1+T2+T0/2;
        pdc2 = T2+T0/2;
        pdc3 = T0/2;
        break;
    case 2:
        pdc1 = T1+T0/2;
        pdc2 = T1+T2+T0/2;
        pdc3 = T0/2;
        break;
    case 3:
        pdc1 = T0/2;
        pdc2 = T1+T2+T0/2;
        pdc3 = T2+T0/2;
        break;
    case 4:
        pdc1 = T0/2;
        pdc2 = T1+T0/2;
        pdc3 = T1+T2+T0/2;
        break;
    case 5:
        pdc1 = T2+T0/2;
        pdc2 = T0/2;
        pdc3 = T1+T2+T0/2;
        break;
    case 6:
        pdc1 = T1+T2+T0/2;
        pdc2 = T0/2;
        pdc3 = T1+T0/2;
        break;
    }                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    
//    if(pdc1 > 2995)
//        pdc1 = 2995;
//    if(pdc2 > 2995)
//        pdc2 = 2995;
//    if(pdc3 > 2995)
//        pdc3 = 2995;
    
//    int PWM_SLOPE_LIMIT = 200;
//    
//    if((pdc1 - (int)PDC1) > PWM_SLOPE_LIMIT)
//        pdc1 = PDC1 + PWM_SLOPE_LIMIT;
//    else if(((int)PDC1 - pdc1) > PWM_SLOPE_LIMIT)
//        pdc1 = PDC1 - PWM_SLOPE_LIMIT;
//    
//    if((pdc2 - (int)PDC2) > PWM_SLOPE_LIMIT)
//        pdc2 = PDC2 + PWM_SLOPE_LIMIT;
//    else if(((int)PDC2 - pdc2) > PWM_SLOPE_LIMIT)
//        pdc2 = PDC2 - PWM_SLOPE_LIMIT;
//    
//    if((pdc3 - (int)PDC3) > PWM_SLOPE_LIMIT)
//        pdc3 = PDC3 + PWM_SLOPE_LIMIT;
//    else if(((int)PDC3 - pdc3) > PWM_SLOPE_LIMIT)
//        pdc3 = PDC3 - PWM_SLOPE_LIMIT;


    
#ifdef NEW_AMP2
    PDC1 = PWM_RES-pdc1;
    PDC2 = PWM_RES-pdc2;
    PDC3 = PWM_RES-pdc3;
#else
    PDC1 = pdc1;
    PDC2 = pdc2;
    PDC3 = pdc3;
#endif
}



//int my_atan2(int y, int x){
//    int tempYX100;
//    if(x == 0){
//        if(y == 0){
//            return 0;
//        }else if(y < 0){
//            return -890;
//        }else{
//            return 890;
//        }
//    }
//    
//    
//    tempYX100 = y*100/x;
//    
//    if(x > 0){
//        if(tempYX100 < 0){
//            if(tempYX100 < -4999){
//                return -890;
//            }else{
//                return -ATAN_T100[-tempYX100];
//            }
//        }else{
//            if(tempYX100 > 4999){
//                return 890;
//            }else{
//                return ATAN_T100[tempYX100];
//            }
//        }
//    }
//    else{
//        if(y > 0){
//            if(tempYX100 < -4999){
//                return 1800-890;
//            }else{
//                return 1800-ATAN_T100[-tempYX100];
//            }   
//        }else{
//            if(tempYX100 > 4999){
//                return -1800+890;
//            }else{
//                return -1800+ATAN_T100[tempYX100];
//            }
//        }
//    }
//}






void MV_AVG_Q(){
    mv_avg_meas_q += measured_current_q - meas_q_sav[meas_q_sav_cnt];
    meas_q_sav[meas_q_sav_cnt] = measured_current_q;
    meas_q_sav_cnt++;
    if(meas_q_sav_cnt >= MV_AVG_MEAS_Q_MAX)
        meas_q_sav_cnt = 0;
    measured_current_q_avg = mv_avg_meas_q/MV_AVG_MEAS_Q_MAX;
}


void CurrentNulling(){
    int i;
    
    CONTROL_MODE = CONTROL_MODE_NO_ACT;
#ifdef NEW_AMP2
    PDC1 = PWM_RES;
    PDC2 = PWM_RES;
    PDC3 = PWM_RES;
#else
    PDC1 = 0;
    PDC2 = 0;
    PDC3 = 0;
#endif
    
    //MOTOR_HIP_ON
    GateDriver_ON();
//    Ir2131_OFF();
    

    ADIN_RZ = ADIN_SZ = ADIN_TZ = 0;
    for(i=0;i<1000;++i){
        while(!ADCDSTAT1bits.ARDY0)
            ;
        while(!ADCDSTAT1bits.ARDY1)
            ;
        while(!ADCDSTAT1bits.ARDY2)
            ;
        
        ADIN_RZ += ADCDATA0;
        ADIN_SZ += ADCDATA1;
        ADIN_TZ += ADCDATA2;
    }
    ADIN_RZ /= 1000;
    ADIN_SZ /= 1000;
    ADIN_TZ /= 1000;
    
    RZ = (int)ADIN_RZ;
    SZ = (int)ADIN_SZ;
    TZ = (int)ADIN_TZ;
}


float Vd = 0.0, Vq = 0.0;
float alp = 0.0;
int speedLow = 1;
void FOC_CurrentControl_Double()
{
    int SINA1000, COSA1000;
    float I_d, I_q;
    float E_d, E_q;
    float E_d1 = 0.0, E_d2 = 0.0, E_q1 = 0.0, E_q2 = 0.0;   
    
    float Valpha, Vbeta;
    int ref_current_q_modi = 0;
    
    if(MT_ON == 0)
        return;
    
    ref_current_q_modi = ref_current_q;
    if(MDR == 1){
        ref_current_q_modi = -ref_current_q_modi;
    }
    reference_current_q = ref_current_q_modi;
    
#ifdef NEW_AMP
    Current_R = ADCDATA0 - RZ;
    Current_S = ADCDATA1 - SZ;
    Current_T = ADCDATA2 - TZ; 
#else
    Current_R = ADCDATA0 - RZ;
    Current_S = ADCDATA1 - SZ;
    Current_T = -Current_R-Current_S; 
#endif
    
    // Clarke Transformation ----- (a, b, c --> alpha, beta)
    float alpha = CURRENT_CONSTANT * Current_R;
    float beta = CURRENT_CONSTANT * 0.57735 * (Current_S - Current_T);
    
    // get Rotor Flux Position
    float I_theta = fmod(((MAG_AXIS%ENC_RES)*360.0/(ENC_POLE_VAL)), 360.0);
    if(I_theta < 0) I_theta += 360.0;
    
    
    // Park Transformation ----- (alpha, beta --> d, q)
    SINA1000 = SIN_T1000[(int)(I_theta*10)];
    COSA1000 = COS_T1000[(int)(I_theta*10)];
    I_d = (COSA1000*alpha + SINA1000*beta)/1000;    // mA
    I_q = (-SINA1000*alpha + COSA1000*beta)/1000;   // mA
    measured_current_q = I_q;
    measured_current_d = I_d;

    // PI current control -----
    E_d = ref_current_d - I_d;          // mA
    E_q = ref_current_q_modi - I_q;     // mA
  
    
    Esum_dd += E_d;
    Esum_qd += E_q;
    
#ifdef FF_VOLTAGE
    Vq = (Pgain_q1000*E_q + Igain_q1000*Esum_qd)/2000 + MOTOR_R*ref_current_q_modi;   // mV
    Vd = (Pgain_d1000*E_d + Igain_d1000*Esum_dd)/1000;   // mV
#else
    Vq = (Pgain_q1000*E_q + Igain_q1000*Esum_qd)/1000;// + Vq_ff;   // mV
    Vd = (Pgain_d1000*E_d + Igain_d1000*Esum_dd)/1000;// + Vd_ff;   // mV
#endif
   
    float tempMag = sqrt(Vq*Vq+Vd*Vd);
    if(tempMag > 48000.0){
        //LATAbits.LATA4 = 1;
        float prev_Vq = Vq;
        float prev_Vd = Vd;
        Vq = Vq*48000.0/tempMag;
        Vd = Vd*48000.0/tempMag;
        Esum_qd += (Vq - prev_Vq)*Igain_q1000/Pgain_q1000;
        Esum_dd += (Vd - prev_Vd)*Igain_d1000/Pgain_d1000;
        //LATAbits.LATA4 = 0;
    }
    if(Igain_q1000 != 0){
        if(Esum_qd > 44000000/Igain_q1000){
            Esum_qd = 44000000/Igain_q1000;
        }else if(Esum_qd < -44000000/Igain_q1000){
            Esum_qd = -44000000/Igain_q1000;
        }
    }
    if(Igain_d1000 != 0){
        if(Esum_dd > 44000000/Igain_d1000){
            Esum_dd = 44000000/Igain_d1000;
        }else if(Esum_dd < -44000000/Igain_d1000){
            Esum_dd = -44000000/Igain_d1000;
        }
    }

    
    
    // Inverse Park Transformation ----- (d, q --> alpha, beta)
    Valpha = (Vd*COSA1000 - Vq*SINA1000)/1000;      // mV
    Vbeta = (Vd*SINA1000 + Vq*COSA1000)/1000;       // mV
    float Vref = sqrt(Valpha*Valpha + Vbeta*Vbeta);    // mV
        
    int v_angle = atan2(Vbeta, Valpha)*R2D*10.0;
    if(v_angle < 0) v_angle = 3600 + v_angle;
    
    
    SPACE_VECTOR_COMMUTATION_Double(Vref, v_angle);
}



void FOC_MeasureCurrent()
{
    int SINA1000, COSA1000;
    float I_q, I_d;
    
    
    if(MT_ON == 0)
        return;
        
#ifdef NEW_AMP
    Current_R = ADCDATA0 - RZ;
    Current_S = ADCDATA1 - SZ;
    Current_T = ADCDATA2 - TZ;
#else
    Current_R = ADCDATA0 - RZ;
    Current_S = ADCDATA1 - SZ;
    Current_T = -Current_R-Current_S; 
#endif
    
    float alpha = CURRENT_CONSTANT * Current_R;
    float beta = CURRENT_CONSTANT * 0.57735 * (Current_S - Current_T);
    
    
    float I_theta = fmod(((MAG_AXIS%ENC_RES)*360.0/(ENC_POLE_VAL)), 360.0);
    if(I_theta < 0) I_theta += 360.0;
    
    SINA1000 = SIN_T1000[(int)(I_theta*10)];
    COSA1000 = COS_T1000[(int)(I_theta*10)];
    I_d = (COSA1000*alpha + SINA1000*beta)/1000;    // mA
    I_q = (-SINA1000*alpha + COSA1000*beta)/1000;   // mA
    measured_current_q = I_q;
    measured_current_d = I_d;
}



int get_tan(int input){
    int st = 64;
    int next = 64;
    int i;
    for(i=0; i<6; i++){
        next = next/2;
        if(input < TAN_T100[st]){
            st = st - next;
        }else{
            st = st + next;
        }
    }
    return st;
}


//int my_atan22(int y, int x){
//    int tempYX100;
//    if(x == 0){
//        if(y == 0){
//            return 0;
//        }else if(y < 0){
//            return -900;
//        }else{
//            return 900;
//        }
//    }
//    
//    tempYX100 = y*100/x;
//    
//    if(x > 0){
//        if(tempYX100 < 0){
//            if(tempYX100 < -8148){
//                return -900;
//            }else{
//                return -get_tan(-tempYX100)*900/128;//-ATAN_T100[-tempYX100];
//            }
//        }else{
//            if(tempYX100 > 8148){
//                return 900;
//            }else{
//                return get_tan(tempYX100)*900/128;//ATAN_T100[tempYX100];
//            }
//        }
//    }
//    else{
//        if(y > 0){
//            if(tempYX100 < -8148){
//                return 1800-900;
//            }else{
//                return 1800-get_tan(-tempYX100)*900/128;//ATAN_T100[-tempYX100];
//            }   
//        }else{
//            if(tempYX100 > 8148){
//                return -1800+900;
//            }else{
//                return -1800+get_tan(tempYX100)*900/128;//ATAN_T100[tempYX100];
//            }
//        }
//    }
//}