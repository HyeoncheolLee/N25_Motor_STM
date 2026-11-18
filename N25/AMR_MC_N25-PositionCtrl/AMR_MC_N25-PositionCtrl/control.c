#include <proc/p32mk1024mcf064.h>
#include <math.h>

#include "control.h"
#include "status.h"
#include "execution.h"
#include "qei.h"
#include "FOC.h"
#include "timer.h"
#include "SV_table.h"


int CONTROL_MODE = CONTROL_MODE_NO_ACT;

unsigned char remove_current_flag = 0;
long long REMOVE_CURRENT_REF_MUL;
long long remove_current_step = 0;
int remove_current_cnt = 0;
int remove_current_active = 0;

#define REF_RCV_TIMEOUT_CNT  25  // 500Hz * 25 = 50ms
#define BIG_ERR_SATURATED_MULTIPLIER 3

long long REF;
long long REF_MUL;
long long PREV_REF = 0;
long long PREV_ERROR = 0;

int MAX_POS1_flag, MAX_POS2_flag;
long POS_LOWER_LIM, POS_UPPER_LIM;
float ERROR_sum = 0.0;
float ERROR_sum_pushing = 0.0;
long long NewInputRefDiff, NewInputRefAbs, NewInputCurrent;
float NewInputCurrent_f;
int MDR;



static void ControlLoopDiff();
static void ControlLoopAbs();


void Handler_Control_Diff(){   
    if(STAT.b.RUN == 1){
        ControlLoopDiff();
    }
}
void Handler_Control_Abs(){
    if(STAT.b.RUN == 1){
        ControlLoopAbs();
    }
}

void Initialize_ControlParameter(){
    PREV_ERROR = 0;
}


const float ERS_max = 50000;            // 5A


#define MV_AVG_REF_ERR_MAX  5
float ref_error_sav[MV_AVG_REF_ERR_MAX];
float mv_avg_ref_error = 0.0;
int ref_error_cnt = 0;

int LAST_VELOCITY = 0;


static void ControlLoopDiff()
{
    // Diff Control
    
	float temp;
    long long temp_error;
    long long error_dot;
    int pgain, dgain, igain;
        
    
#ifdef CURRENT_REMOVE
     long long pulse_temp = NewInputRefDiff;

     if(remove_current_flag && (CONTROL_MODE == CONTROL_MODE_FOC_POS_CONTROL))
     {
         if(remove_current_flag)
         {
             if(remove_current_active == 0)
             {
                 remove_current_active = 1;
                 remove_current_cnt = 0;

                 long long delta = REF_MUL - REMOVE_CURRENT_REF_MUL;
                 remove_current_step = delta /250;
             }

             if(remove_current_cnt < 250)
             {
                 pulse_temp = remove_current_step;
                 remove_current_cnt++;
             }

             else
             {
                 remove_current_flag = 0;
                 remove_current_active =0;
                 pulse_temp = 0;
             }
         }
     }
     else
     {
         remove_current_flag = 0;

         // CAN receive timeout: stop accumulating if no CAN data for 50ms
         REF_RCV_CNT++;
         if(REF_RCV_CNT > REF_RCV_TIMEOUT_CNT)
         {
             pulse_temp = 0;
             REF_RCV_CNT = REF_RCV_TIMEOUT_CNT + 1;  // prevent overflow
         }

         REF_MUL = REF_MUL + (long long)(pulse_temp);
         REF = REF_MUL/MUL;
     }

#else
    long long pulse_temp = NewInputRefDiff;

    // CAN receive timeout: stop accumulating if no CAN data for 50ms
    REF_RCV_CNT++;
    if(REF_RCV_CNT > REF_RCV_TIMEOUT_CNT)
    {
        pulse_temp = 0;
        REF_RCV_CNT = REF_RCV_TIMEOUT_CNT + 1;  // prevent overflow
    }

    REF_MUL = REF_MUL + (long long)(pulse_temp);
//    REF = REF + (long long)(pulse_temp);
    REF = REF_MUL/MUL;

#endif



    pgain = Pgain_pos;
    dgain = Dgain_pos;
    igain = Igain_pos;



    static int idle_control_flag = 0;
    const int idle_control_current_limt = 5000;
    static int position_control_current_limit = 5000;
    static int no_ref_cnt = 0;
    static int yes_ref_cnt = 0;

    static int prev_saturated = 0;
    static unsigned int saturated_run = 0;   // 연속 포화 tick 수 (디버깅/향후 텔레메트리용)

    if(pulse_temp == 0){
        yes_ref_cnt = 0;
        no_ref_cnt++;
        if(no_ref_cnt > 200){
            // 400ms
            if(idle_control_flag == 0){
                //REF = PREV_ERROR = NewPos + ABS_OFFSET_ENC;
                idle_control_flag = 1;
            }
        }
    }else{
        no_ref_cnt = 0;
        yes_ref_cnt++;
        if(yes_ref_cnt > 50){
            // 100ms
            if(idle_control_flag == 1){
                idle_control_flag = 0;
            }
        }
    }


    int TEMP_MAX_CUR_LIMIT = OVR_LIM_Current + 1000;    // MAX_CUR_LIMIT;
    if(idle_control_flag == 1){
        position_control_current_limit -= 10;   // 1 --> 0.5A/sec  10 --> 5A/sec
        if(position_control_current_limit < idle_control_current_limt){
            position_control_current_limit = idle_control_current_limt;
        }
    }else{
        position_control_current_limit += 40;   // 1 --> 0.5A/sec  40 --> 20A/sec
        if(position_control_current_limit > OVR_LIM_Current + 1000){
            position_control_current_limit = OVR_LIM_Current + 1000;
        }
    }
    TEMP_MAX_CUR_LIMIT = position_control_current_limit;




    long long cur_ref_error = REF-PREV_REF;
    LAST_VELOCITY = cur_ref_error;
    
    // Input Error
    if((cur_ref_error > (long long)(I_ERR+1000)) || (cur_ref_error < (long long)(-I_ERR-1000)))
    {
        STAT.b.INP = 1;
        // not update reference
        REF = PREV_REF;
        cur_ref_error = REF-PREV_REF;
    }
    PREV_REF = REF;


    // ===================================

	temp_error = REF - (NewPos + ABS_OFFSET_ENC);

    // // Big Error
    // if(idle_control_flag == 1){
        
    //     if(temp_error > B_ERR/3){
    //         REF -= 1;
    //         REF_MUL -= 10;
    //     }else if(temp_error < -B_ERR/3){
    //         REF += 1;
    //         REF_MUL += 10;
    //     }
    //     temp_error = REF - (NewPos + ABS_OFFSET_ENC);
    // }else{
    //     if(TEMP_MAX_CUR_LIMIT < OVR_LIM_Current*0.8)
    //     {
    //         // if(temp_error > B_ERR || temp_error < -B_ERR){
    //         //     STAT.b.BIG = 1;
    //         // }
    //     }
    //     else
    //     {
    //         if(temp_error > B_ERR || temp_error < -B_ERR){
    //             STAT.b.BIG = 1;
    //         }
    //     }
    // }

    error_dot = temp_error-PREV_ERROR;
    PREV_ERROR = temp_error;
        
    ERROR_sum += temp_error/100.0;//*DT;
    float tempERS_max;
    
    if(igain >= 1){
        tempERS_max = ERS_max/igain;
    }else{
        tempERS_max = ERS_max;
    }    
	if(ERROR_sum > tempERS_max)
		ERROR_sum = tempERS_max;
	else if(ERROR_sum < -tempERS_max)
		ERROR_sum = -tempERS_max;

    
    temp = ((float)(temp_error*pgain + error_dot*dgain + igain*ERROR_sum)/10.0 + (float)NewInputCurrent_f);

    if(temp > TEMP_MAX_CUR_LIMIT) temp = TEMP_MAX_CUR_LIMIT;
    if(temp < -TEMP_MAX_CUR_LIMIT) temp = -TEMP_MAX_CUR_LIMIT;

    ref_current_q = (int)temp;


    // Big Error
    if(idle_control_flag == 1){
        if(temp > idle_control_current_limt-500){
            REF -= 50;
            REF_MUL -= 500;
        }else if(temp< -idle_control_current_limt+500){
            REF += 50;
            REF_MUL += 500;
        }
    }else{
        if(TEMP_MAX_CUR_LIMIT < OVR_LIM_Current*0.8)
        {
            // if(temp_error > B_ERR || temp_error < -B_ERR){
            //     STAT.b.BIG = 1;
            // }
        }
        else
        {
            if(temp_error > B_ERR || temp_error < -B_ERR){
                STAT.b.BIG = 1;
            }
        }
    }
}


	

static void ControlLoopAbs(){
    // Abs Control

	float temp;
    long long temp_error;
    long long error_dot;
    int pgain, dgain, igain;
        
    REF = NewInputRefAbs;
    
    pgain = Pgain_pos;
    dgain = Dgain_pos;
    igain = Igain_pos;
    
    long long cur_ref_error = REF-PREV_REF;
    LAST_VELOCITY = cur_ref_error;
    
    // Input Error
    if((cur_ref_error > (long long)(I_ERR+1000)) || (cur_ref_error < (long long)(-I_ERR-1000))){
        STAT.b.INP = 1;
        // not update reference
        REF = PREV_REF;
        cur_ref_error = REF-PREV_REF;
    }
    PREV_REF = REF;
    REF_MUL = REF * REF_MUL;

    // ===================================
            
	temp_error = REF - (NewPos + ABS_OFFSET_ENC);
        
    // Big Error
    if(temp_error > B_ERR || temp_error < -B_ERR){
        STAT.b.BIG = 1;
    }
    
    error_dot = temp_error-PREV_ERROR;
    PREV_ERROR = temp_error;
        
    ERROR_sum += temp_error/100.0;//*DT;
    float tempERS_max;
    
    if(igain >= 1){
        tempERS_max = ERS_max/igain;
    }else{
        tempERS_max = ERS_max;
    }    
	if(ERROR_sum > tempERS_max)
		ERROR_sum = tempERS_max;
	else if(ERROR_sum < -tempERS_max)
		ERROR_sum = -tempERS_max;

    
    temp = ((float)(temp_error*pgain + error_dot*dgain + igain*ERROR_sum)/10.0);
  
    
    int TEMP_MAX_CUR_LIMIT = OVR_LIM_Current + 1000;    // MAX_CUR_LIMIT
    if(temp > TEMP_MAX_CUR_LIMIT) temp = TEMP_MAX_CUR_LIMIT;
    if(temp < -TEMP_MAX_CUR_LIMIT) temp = -TEMP_MAX_CUR_LIMIT;
        
    ref_current_q = (int)temp;
}


