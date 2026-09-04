#ifndef __MOTOR_CONFIG_H
#define __MOTOR_CONFIG_H

/*
 * 电机 PWM 参数配置
 *
 * 由于硬件差异，四个电机在相同 PWM 下转速不同。
 * 每个电机设有独立的校正系数（百分比，100 = 无校正），
 * 实际 PWM = base_speed * MOTORx_CORR / 100。
 *
 * 使用方法：
 *   pwm_motor1_forward(MOTOR1_PWM(1800));  // 替代 pwm_motor1_forward(1800)
 *   pwm_car_forward(MOTOR_BASE_SPEED);     // 内部已应用各电机校正
 *
 * 调车方法：
 *   1. 先将所有 CORR 设为 100
 *   2. 令车直行，观察偏移方向
 *   3. 降低偏快侧电机的 CORR（如 95, 90...）
 *   4. 反复测试直到走直
 */

/* ---- 基础速度 ---- */
#define MOTOR_BASE_SPEED     2300   /* 默认基础速度 0~3599 (~64% 占空比) */
#define MOTOR_MAX_SPEED      3599   /* PWM ARR 最大值 */

/* ---- 各电机校正系数（百分比，100 = 无校正） ---- */
#define MOTOR1_CORR          100    /* 左前电机 */
#define MOTOR2_CORR          100    /* 左后电机 */
#define MOTOR3_CORR          100    /* 右前电机 */
#define MOTOR4_CORR          100    /* 右后电机 */

/* ---- 各电机反转标志（1 = 反转，0 = 正常） ---- */
/* 如果某电机正转时车轮方向反了，设为 1 即可逻辑反转，无需改接线 */
#define MOTOR1_REVERSE       0     /* 左前电机 */
#define MOTOR2_REVERSE       0     /* 左后电机 */
#define MOTOR3_REVERSE       1     /* 右前电机 */
#define MOTOR4_REVERSE       1     /* 右后电机 */

/* ---- 校正后 PWM 计算宏 ---- */
#define MOTOR1_PWM(speed)    ((int16_t)((speed) * MOTOR1_CORR / 100))
#define MOTOR2_PWM(speed)    ((int16_t)((speed) * MOTOR2_CORR / 100))
#define MOTOR3_PWM(speed)    ((int16_t)((speed) * MOTOR3_CORR / 100))
#define MOTOR4_PWM(speed)    ((int16_t)((speed) * MOTOR4_CORR / 100))

/* ---- 差速转向比例 ---- */
#define TURN_SLOW_RATIO      4      /* 转弯慢侧 = speed / 4 */
#define ROTATE_SLOW_RATIO    4      /* 原地旋转慢侧 = speed / 4 */

#endif /* __MOTOR_CONFIG_H */
