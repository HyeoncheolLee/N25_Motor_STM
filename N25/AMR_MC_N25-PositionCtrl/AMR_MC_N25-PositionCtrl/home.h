
#ifndef __HOME_H__
#define __HOME_H__



typedef struct _HOME_ST_{
    float start_pos;
    float target_pos;
    float delta_pos;
    float cur_pos;
    
    int cur_cnt;
    int goal_cnt;
    
    int move_flag;
}HOME_ST;


typedef struct _GOTO_ST_{
    float start_pos;
    float target_pos;
    float delta_pos;
    float cur_pos;
    
    float fixed_a;
    
    int mode;
    float cur_cnt;
    float t1;
    float t2;
    float t3;
    
    float cur_vel;
    
    int move_flag;
}GOTO_ST;


extern HOME_ST home_param;
extern GOTO_ST goto_param;

extern int HomeDir;



extern void set_home_start_pos(long long st_pos);
extern void set_home_target(long long target, int time_cnt);
extern void move_home_target();

extern void set_goto_start_pos(float st_pos);
extern void set_goto_target(float target, float vel);
extern void move_goto_target();


#endif