/*
 * line_follow.c — 循迹运动控制（状态驱动 + 转弯减速）
 *
 * 传感器布局（左→右）：X2  X1  X3  X4
 *
 * 逻辑：
 *   X1+X3 在线上 → 居中 → 全速直行
 *   仅 X1        → 线偏左 → 减速右旋（小转弯）
 *   仅 X3        → 线偏右 → 减速左旋（小转弯）
 *   X2 在线上    → 线大偏左 → 减速右旋（大转弯）
 *   X4 在线上    → 线大偏右 → 减速左旋（大转弯）
 *   全白         → 惯性旋转找回
 *   全黑         → 全速直行（十字路口）
 */

#include "line_follow.h"
#include "line_follow_config.h"
#include "irtracking.h"
#include "motor.h"
#include "motor_config.h"
#include "tim.h"

/* ---- 状态 ---- */
static int8_t  last_dir = 0;     /* 上次转向: -1=右旋, +1=左旋, 0=直行 */

/* ========== 初始化 ========== */

void LineFollow_Init(void)
{
    Motor_Init();
    IRTracking_Init();
    last_dir = 0;
}

/* ========== 运行 ========== */

void LineFollow_Run(int16_t base_speed)
{
    uint8_t state = IRTracking_ReadAll();   /* bit0=X1..bit3=X4, 0=黑线, 1=白地 */
    int16_t turn_speed = LINE_FOLLOW_TURN_SPEED;

    /* 提取各路：0=在线上(黑线), 1=白地 */
    uint8_t x1 = (state & 0x01) ? 0 : 1;
    uint8_t x2 = (state & 0x02) ? 0 : 1;
    uint8_t x3 = (state & 0x04) ? 0 : 1;
    uint8_t x4 = (state & 0x08) ? 0 : 1;

    uint8_t on_line = x1 + x2 + x3 + x4;

    if (on_line == 0) {
        /* ---- 全白：惯性旋转找回（减速） ---- */
        if (last_dir > 0)       pwm_car_rotate_left(turn_speed);
        else if (last_dir < 0)  pwm_car_rotate_right(turn_speed);
        else                    pwm_car_forward(turn_speed);

    } else if (on_line == 4) {
        /* ---- 全黑：十字路口 → 全速直行 ---- */
        pwm_car_forward(base_speed);
        last_dir = 0;

    } else if (x1 && x3) {
        /* ---- X1+X3 居中 → 全速直行 ---- */
        pwm_car_forward(base_speed);
        last_dir = 0;

    } else if (x4) {
        /* ---- X4 在线上（最右）→ 线大偏右 → 减速左旋 ---- */
        pwm_car_rotate_left(turn_speed);
        last_dir = +1;

    } else if (x2) {
        /* ---- X2 在线上（最左）→ 线大偏左 → 减速右旋 ---- */
        pwm_car_rotate_right(turn_speed);
        last_dir = -1;

    } else if (x3) {
        /* ---- 仅 X3（右中）→ 线偏右 → 减速左转差速 ---- */
        pwm_motor1_forward(MOTOR1_PWM(LINE_FOLLOW_TURN_SLOW));
        pwm_motor2_forward(MOTOR2_PWM(LINE_FOLLOW_TURN_SLOW));
        pwm_motor3_forward(MOTOR3_PWM(turn_speed));
        pwm_motor4_forward(MOTOR4_PWM(turn_speed));
        last_dir = +1;

    } else if (x1) {
        /* ---- 仅 X1（左中）→ 线偏左 → 减速右转差速 ---- */
        pwm_motor1_forward(MOTOR1_PWM(turn_speed));
        pwm_motor2_forward(MOTOR2_PWM(turn_speed));
        pwm_motor3_forward(MOTOR3_PWM(LINE_FOLLOW_TURN_SLOW));
        pwm_motor4_forward(MOTOR4_PWM(LINE_FOLLOW_TURN_SLOW));
        last_dir = -1;

    } else {
        /* ---- 兜底：全速直行 ---- */
        pwm_car_forward(base_speed);
        last_dir = 0;
    }
}

/* ========== 调试接口 ========== */

int8_t LineFollow_GetError(void)
{
    return last_dir;
}
