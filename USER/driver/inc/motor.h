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

/* ---- 整车：差速转弯 + 能耗制动 ---- */

/**
 * @brief  Signed differential drive: left/right sides get independent speeds.
 *         Positive = forward, negative = backward, magnitude 0~3599.
 *         Allows inner wheel reversal for in-place rotation (P-control output).
 *         Physical layout: M3/M4 = left side, M1/M2 = right side.
 */
void car_diff_turn(int16_t left_speed, int16_t right_speed);

/**
 * @brief  Energy braking: set all 8 PWM channels to max output.
 *         H-bridge A=B=1 → motor terminals shorted → fast deceleration.
 *         Much faster than pwm_car_stop() (free-wheeling).
 *         CCR=3600 > ARR=3599 → PWM1 mode outputs constant high.
 */
void car_brake(void);

#endif /* __MOTOR_H */
