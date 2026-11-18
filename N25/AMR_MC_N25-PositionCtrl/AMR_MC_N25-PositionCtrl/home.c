#include <math.h>
#include "home.h"
#include "qei.h"


HOME_ST home_param;
GOTO_ST goto_param;
int HomeDir = 0;

void set_home_start_pos(long long st_pos){
    home_param.start_pos = st_pos;
}

void set_home_target(long long target, int time_cnt){
    if(home_param.move_flag == 1)
        return;
    
    //home_param.start_pos = currentMotorPosition + ABS_OFFSET_ENC;
    home_param.target_pos = target;
    home_param.delta_pos = home_param.target_pos - home_param.start_pos;
    home_param.goal_cnt = time_cnt;
    home_param.cur_cnt = 0;
    home_param.move_flag = 1;
}
void move_home_target(){
    if(home_param.move_flag == 1){
        home_param.cur_cnt++;
        
        if(home_param.cur_cnt >= home_param.goal_cnt){
            home_param.cur_cnt = home_param.goal_cnt = 0;
            home_param.cur_pos = home_param.target_pos;
            home_param.move_flag = 0;
        }else{
            home_param.cur_pos = home_param.start_pos + home_param.delta_pos * 0.5 * (1.0-cos(PI * ((float)(home_param.cur_cnt)) / ((float)(home_param.goal_cnt))));
        }
    }
}


void set_goto_start_pos(float st_pos){
    goto_param.start_pos = st_pos;
}

void set_goto_target(float target, float vel){  // pulse, pulse/tick  a = pulse/tick^2
    if(goto_param.move_flag != 0)
        return;
    
    if(vel < 5.0){
        vel = 5.0;
    }
    goto_param.target_pos = target;
    goto_param.delta_pos = goto_param.target_pos - goto_param.start_pos;
    
    if(vel*vel/goto_param.fixed_a < fabs(goto_param.delta_pos)){
        // normal
        goto_param.mode = 0;
        goto_param.t1 = vel/goto_param.fixed_a;
        goto_param.t2 = fabs(goto_param.delta_pos)/vel;
        goto_param.t3 = goto_param.t1+goto_param.t2;
    }else{
        // triangle
        goto_param.mode = 1;
        goto_param.t1 = sqrt(fabs(goto_param.delta_pos)/goto_param.fixed_a);
        goto_param.t2 = goto_param.t1*2;
    }
    goto_param.cur_cnt = 0;
    goto_param.cur_vel = 0.0;
    goto_param.move_flag = 1;
}

void move_goto_target(){
    float prev_pos = goto_param.cur_pos;
    
    if(goto_param.move_flag == 1){
        goto_param.cur_cnt = goto_param.cur_cnt + 1.0;
    
        float dir = 1.0;
        if(goto_param.delta_pos > 0){
            dir = 1.0;
        }else{
            dir = -1.0;
        }    
                
        if(goto_param.mode == 0){
            // normal
            float vel = goto_param.fixed_a * goto_param.t1;
            if(goto_param.cur_cnt < goto_param.t1){
                goto_param.cur_pos = goto_param.start_pos + dir*(0.5*goto_param.fixed_a*goto_param.cur_cnt*goto_param.cur_cnt);
            }else if(goto_param.cur_cnt < goto_param.t2){
                float t = goto_param.cur_cnt - goto_param.t1;
                goto_param.cur_pos = goto_param.start_pos + dir*(0.5*goto_param.fixed_a*goto_param.t1*goto_param.t1 + vel*t);
            }else if(goto_param.cur_cnt < goto_param.t3){
                float t = goto_param.cur_cnt - goto_param.t2;
                goto_param.cur_pos = goto_param.start_pos + dir*(0.5*goto_param.fixed_a*goto_param.t1*goto_param.t1 + vel*(goto_param.t2-goto_param.t1) + vel*t - 0.5*goto_param.fixed_a*t*t);
            }else{
                goto_param.cur_pos = goto_param.target_pos;
                goto_param.cur_cnt = 0;
                goto_param.move_flag = 0;
            }
        }else{
            // triangle
            float vel = goto_param.fixed_a * goto_param.t1;
            if(goto_param.cur_cnt < goto_param.t1){
                goto_param.cur_pos = goto_param.start_pos + dir*(0.5*goto_param.fixed_a*goto_param.cur_cnt*goto_param.cur_cnt);
            }else if(goto_param.cur_cnt < goto_param.t2){
                float t = goto_param.cur_cnt - goto_param.t1;
                goto_param.cur_pos = goto_param.start_pos + dir*(0.5*goto_param.fixed_a*goto_param.t1*goto_param.t1 + vel*t - 0.5*goto_param.fixed_a*t*t);
            }else{
                goto_param.cur_pos = goto_param.target_pos;
                goto_param.cur_cnt = 0;
                goto_param.move_flag = 0;
            }
        }
        
        goto_param.cur_vel = goto_param.cur_pos - prev_pos;
        
    }else if(goto_param.move_flag == 2){
        // slow down and stop
        if(fabs(goto_param.cur_vel) < 0.01){
            // stopped --> finish
            goto_param.cur_cnt = 0;
            goto_param.move_flag = 0;
        }else{
            if(goto_param.cur_vel > 0){
                float now_vel = goto_param.cur_vel - goto_param.fixed_a;
                if(now_vel < 0){
                    now_vel = 0.0;
                }
                goto_param.cur_pos = goto_param.cur_pos + now_vel;
            }else{
                float now_vel = goto_param.cur_vel + goto_param.fixed_a;
                if(now_vel > 0){
                    now_vel = 0.0;
                }
                goto_param.cur_pos = goto_param.cur_pos + now_vel;
            }
        }
        
        goto_param.cur_vel = goto_param.cur_pos - prev_pos;
    }
}