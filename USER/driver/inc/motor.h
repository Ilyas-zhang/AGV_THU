#ifndef __MOTOR_H
#define __MOTOR_H

#include <stdint.h>

/* ---- 初始化 ---- */
void Motor_Init(void);   /* 启动所有 PWM 通道，电机初始停止 */

/* ---- 单电机：非 PWM（全速开/关，用 CCR=0/ARR 等效） ---- */
void motor1_forward(void);
void motor1_backward(void);
void motor2_forward(void);
void motor2_backward(void);
void motor3_forward(void);
void motor3_backward(void);
void motor4_forward(void);
void motor4_backward(void);
void motor_stop(void);

/* ---- 整车：非 PWM ---- */
void car_forward(void);
void car_backward(void);
void car_turn_left(void);
void car_turn_right(void);
void car_rotate_left(void);
void car_rotate_right(void);
void car_stop(void);

/* ---- 单电机：PWM 调速（speed: 0~3599） ---- */
void pwm_motor1_forward(int16_t speed);
void pwm_motor1_backward(int16_t speed);
void pwm_motor2_forward(int16_t speed);
void pwm_motor2_backward(int16_t speed);
void pwm_motor3_forward(int16_t speed);
void pwm_motor3_backward(int16_t speed);
void pwm_motor4_forward(int16_t speed);
void pwm_motor4_backward(int16_t speed);
void pwm_motor_stop(void);

/* ---- 整车：PWM 调速 ---- */
void pwm_car_forward(int16_t speed);
void pwm_car_backward(int16_t speed);
void pwm_car_turn_left(int16_t speed);
void pwm_car_turn_right(int16_t speed);
void pwm_car_rotate_left(int16_t speed);
void pwm_car_rotate_right(int16_t speed);
void pwm_car_stop(void);

#endif /* __MOTOR_H */
