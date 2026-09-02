#include "line_follow.h"
#include "irtracking.h"
#include "motor.h"
#include "motor_config.h"

/*
 * 循迹运动逻辑（从小车前进方向看）：
 *
 *   X1  X2  X3  X4  →  动作
 *    0   1   1   0  →  前进（中间两路在黑线上）
 *    1   1   1   0  →  小幅右转（车偏左）
 *    0   0   1   0  →  小幅左转（车偏右）
 *    1   0   1   0  →  大幅右转
 *    0   1   0   0  →  大幅左转
 *    1   1   1   1  →  全白：继续前进
 *    0   0   0   0  →  全黑：十字路口，直行
 */

void LineFollow_Init(void)
{
    IRTracking_Init();
}

void LineFollow_Run(int16_t base_speed)
{
    uint8_t state = IRTracking_ReadAll();

    int16_t slow  = base_speed / 2;
    int16_t vslow = base_speed / 4;

    switch (state) {
    /* ---- 直行（中间两路在黑线上） ---- */
    case 0x06:  /* X1=0,X2=1,X3=1,X4=0 */
        pwm_car_forward(base_speed);
        break;

    /* ---- 小幅偏移 ---- */
    case 0x0E:  /* X1=1,X2=1,X3=1,X4=0 → 车偏左，右转 */
        pwm_motor1_forward(MOTOR1_PWM(base_speed)); pwm_motor2_forward(MOTOR2_PWM(base_speed));
        pwm_motor3_forward(MOTOR3_PWM(slow));       pwm_motor4_forward(MOTOR4_PWM(slow));
        break;

    case 0x02:  /* X1=0,X2=1,X3=0,X4=0 → 车偏右，左转 */
        pwm_motor1_forward(MOTOR1_PWM(slow));       pwm_motor2_forward(MOTOR2_PWM(slow));
        pwm_motor3_forward(MOTOR3_PWM(base_speed)); pwm_motor4_forward(MOTOR4_PWM(base_speed));
        break;

    /* ---- 大幅偏移 ---- */
    case 0x0A:  /* X1=1,X2=0,X3=1,X4=0 → 大幅偏左，大右转 */
        pwm_motor1_forward(MOTOR1_PWM(base_speed)); pwm_motor2_forward(MOTOR2_PWM(base_speed));
        pwm_motor3_forward(MOTOR3_PWM(vslow));      pwm_motor4_forward(MOTOR4_PWM(vslow));
        break;

    case 0x04:  /* X1=0,X2=0,X3=1,X4=0 → 大幅偏右，大左转 */
        pwm_motor1_forward(MOTOR1_PWM(vslow));      pwm_motor2_forward(MOTOR2_PWM(vslow));
        pwm_motor3_forward(MOTOR3_PWM(base_speed)); pwm_motor4_forward(MOTOR4_PWM(base_speed));
        break;

    /* ---- 极端偏移（仅外侧检测到黑线） ---- */
    case 0x08:  /* 仅 X4=0 → 严重偏左，原地右旋 */
        pwm_car_rotate_right(vslow);
        break;

    case 0x01:  /* 仅 X1=0 → 严重偏右，原地左旋 */
        pwm_car_rotate_left(vslow);
        break;

    /* ---- 全黑（十字路口）→ 直行 ---- */
    case 0x00:
        pwm_car_forward(base_speed);
        break;

    /* ---- 全白或其他（脱线）→ 保持直行 ---- */
    default:
        pwm_car_forward(base_speed);
        break;
    }
}
