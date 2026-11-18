#ifndef __ADC_H__
#define __ADC_H__


#define SET_ADC_TRIGGER     ADCCON3bits.GSWTRG = 1;

extern int temperature_board_celsius_x10;
extern int temperature_motor_celsius_x10;
extern int temperature_chip_celsius_x10;
extern int temperature_board_celsius_x1;
extern int temperature_motor_celsius_x1;
extern int temperature_chip_celsius_x1;

extern void InitADC();


#endif