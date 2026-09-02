#include "motor.h"
#include "motor_config.h"
#include "main.h"
#include "tim.h"

/* ---- 常量 ---- */
#define PWM_ARR  3599   /* ARR = 3599 → 72MHz / 3600 = 20KHz */

/*
 * 电机布局（4轮差速小车）：
 *   Motor1, Motor2 = 左侧（前左 + 后左）
 *   Motor3, Motor4 = 右侧（前右 + 后右）
 *
 * 电机驱动逻辑（AT8236 H 桥）：
 *   Forward:  IN1 = PWM(speed), IN2 = 0
 *   Backward: IN1 = 0,           IN2 = PWM(speed)
 *   Stop:     IN1 = 0,           IN2 = 0
 *   Brake:    IN1 = ARR,         IN2 = ARR
 *
 * TIM1_CH1~4 → Motor1_IN1, Motor1_IN2, Motor2_IN1, Motor2_IN2
 * TIM8_CH1~4 → Motor3_IN1, Motor3_IN2, Motor4_IN1, Motor4_IN2
 */

/* ========== 初始化 ========== */

void Motor_Init(void)
{
    /* 启动所有 PWM 通道 */
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);

    HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_4);

    /* 高级定时器必须使能 MOE，否则 PWM 不会输出到引脚 */
    __HAL_TIM_MOE_ENABLE(&htim1);
    __HAL_TIM_MOE_ENABLE(&htim8);

    /* 初始占空比 = 0，所有电机停止 */
    motor_stop();
}

/* ========== 单电机：非 PWM（全速） ========== */

void motor1_forward(void)  { pwm_motor1_forward(PWM_ARR); }
void motor1_backward(void) { pwm_motor1_backward(PWM_ARR); }
void motor2_forward(void)  { pwm_motor2_forward(PWM_ARR); }
void motor2_backward(void) { pwm_motor2_backward(PWM_ARR); }
void motor3_forward(void)  { pwm_motor3_forward(PWM_ARR); }
void motor3_backward(void) { pwm_motor3_backward(PWM_ARR); }
void motor4_forward(void)  { pwm_motor4_forward(PWM_ARR); }
void motor4_backward(void) { pwm_motor4_backward(PWM_ARR); }

void motor_stop(void)
{
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 0);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, 0);
    __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_2, 0);
    __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_3, 0);
    __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_4, 0);
}

/* ========== 整车：非 PWM ========== */

void car_forward(void)
{
    motor1_forward(); motor2_forward();
    motor3_forward(); motor4_forward();
}

void car_backward(void)
{
    motor1_backward(); motor2_backward();
    motor3_backward(); motor4_backward();
}

void car_turn_left(void)
{
    /* 右侧前进，左侧停止 → 左转 */
    motor1_forward(); motor2_forward();
    motor3_forward(); motor4_forward();
    /* 差速：左慢右快 — 非PWM版用半速近似 */
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, PWM_ARR / 4);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, PWM_ARR / 4);
}

void car_turn_right(void)
{
    /* 左侧快，右侧慢 → 右转 */
    motor1_forward(); motor2_forward();
    motor3_forward(); motor4_forward();
    __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_1, PWM_ARR / 4);
    __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_3, PWM_ARR / 4);
}

void car_rotate_left(void)
{
    /* 左侧后退，右侧前进 → 原地左旋 */
    motor1_backward(); motor2_backward();
    motor3_forward();  motor4_forward();
}

void car_rotate_right(void)
{
    /* 左侧前进，右侧后退 → 原地右旋 */
    motor1_forward();  motor2_forward();
    motor3_backward(); motor4_backward();
}

void car_stop(void) { motor_stop(); }

/* ========== 单电机：PWM 调速 ========== */
/* MOTORx_REVERSE: 1 = 逻辑反转（forward 实际输出 backward），0 = 正常 */

void pwm_motor1_forward(int16_t speed)
{
#if MOTOR1_REVERSE
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, (uint32_t)speed);
#else
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, (uint32_t)speed);
#endif
}

void pwm_motor1_backward(int16_t speed)
{
#if MOTOR1_REVERSE
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, (uint32_t)speed);
#else
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, (uint32_t)speed);
#endif
}

void pwm_motor2_forward(int16_t speed)
{
#if MOTOR2_REVERSE
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 0);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, (uint32_t)speed);
#else
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, 0);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, (uint32_t)speed);
#endif
}

void pwm_motor2_backward(int16_t speed)
{
#if MOTOR2_REVERSE
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, 0);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, (uint32_t)speed);
#else
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 0);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, (uint32_t)speed);
#endif
}

void pwm_motor3_forward(int16_t speed)
{
#if MOTOR3_REVERSE
    __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_2, (uint32_t)speed);
#else
    __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_2, 0);
    __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_1, (uint32_t)speed);
#endif
}

void pwm_motor3_backward(int16_t speed)
{
#if MOTOR3_REVERSE
    __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_2, 0);
    __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_1, (uint32_t)speed);
#else
    __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_2, (uint32_t)speed);
#endif
}

void pwm_motor4_forward(int16_t speed)
{
#if MOTOR4_REVERSE
    __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_3, 0);
    __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_4, (uint32_t)speed);
#else
    __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_4, 0);
    __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_3, (uint32_t)speed);
#endif
}

void pwm_motor4_backward(int16_t speed)
{
#if MOTOR4_REVERSE
    __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_4, 0);
    __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_3, (uint32_t)speed);
#else
    __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_3, 0);
    __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_4, (uint32_t)speed);
#endif
}

void pwm_motor_stop(void) { motor_stop(); }

/* ========== 整车：PWM 调速 ========== */

void pwm_car_forward(int16_t speed)
{
    pwm_motor1_forward(MOTOR1_PWM(speed)); pwm_motor2_forward(MOTOR2_PWM(speed));
    pwm_motor3_forward(MOTOR3_PWM(speed)); pwm_motor4_forward(MOTOR4_PWM(speed));
}

void pwm_car_backward(int16_t speed)
{
    pwm_motor1_backward(MOTOR1_PWM(speed)); pwm_motor2_backward(MOTOR2_PWM(speed));
    pwm_motor3_backward(MOTOR3_PWM(speed)); pwm_motor4_backward(MOTOR4_PWM(speed));
}

void pwm_car_turn_left(int16_t speed)
{
    /* 左侧慢，右侧全速 → 左转 */
    pwm_motor1_forward(MOTOR1_PWM(speed / TURN_SLOW_RATIO)); pwm_motor2_forward(MOTOR2_PWM(speed / TURN_SLOW_RATIO));
    pwm_motor3_forward(MOTOR3_PWM(speed));                   pwm_motor4_forward(MOTOR4_PWM(speed));
}

void pwm_car_turn_right(int16_t speed)
{
    /* 左侧全速，右侧慢 → 右转 */
    pwm_motor1_forward(MOTOR1_PWM(speed));                   pwm_motor2_forward(MOTOR2_PWM(speed));
    pwm_motor3_forward(MOTOR3_PWM(speed / TURN_SLOW_RATIO)); pwm_motor4_forward(MOTOR4_PWM(speed / TURN_SLOW_RATIO));
}

void pwm_car_rotate_left(int16_t speed)
{
    pwm_motor1_backward(MOTOR1_PWM(speed)); pwm_motor2_backward(MOTOR2_PWM(speed));
    pwm_motor3_forward(MOTOR3_PWM(speed));  pwm_motor4_forward(MOTOR4_PWM(speed));
}

void pwm_car_rotate_right(int16_t speed)
{
    pwm_motor1_forward(MOTOR1_PWM(speed));  pwm_motor2_forward(MOTOR2_PWM(speed));
    pwm_motor3_backward(MOTOR3_PWM(speed)); pwm_motor4_backward(MOTOR4_PWM(speed));
}

void pwm_car_stop(void) { motor_stop(); }
