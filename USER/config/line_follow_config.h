#ifndef __LINE_FOLLOW_CONFIG_H
#define __LINE_FOLLOW_CONFIG_H

/*
 * 循迹运动参数配置 — 加权误差 + Kp 比例差速
 *
 * 算法：
 *   mid = X1 ∨ X3  (两中间传感器合并为等效中心)
 *   error = (-3)*X2 + (+3)*X4
 *   filtered_error = EMA(error)       ← 低通滤波，平滑抖动
 *   left_speed  = base_speed + Kp * filtered_error
 *   right_speed = base_speed - Kp * filtered_error
 *
 * 调参：
 *   Kp 越大 → 转向越灵敏，但过大会振荡
 *   Kp 越小 → 转向越平缓，但弯道可能跟不上
 *   EMA_ALPHA 越大 → 滤波越强（越平滑），但响应越慢
 *
 * ⚠ 电机 PWM < 2100 无法克服静摩擦，车轮不转
 *   但差速慢侧允许 0（停转），靠快侧单边驱动转向
 */

/* ---- 速度 (0~3599) ---- */
#define LINE_FOLLOW_BASE_SPEED     2500    /* 直行基准速度 */
#define LINE_FOLLOW_MIN_SPEED      2100    /* 电机最低启动速度（快侧下限） */
#define LINE_FOLLOW_MAX_SPEED      3599    /* PWM 最大值 */

/* ---- 比例增益 ---- */
#define LINE_FOLLOW_KP             500     /* Kp: PWM 差速/误差单位 */
/* Kp=500 典型: error=1→差速±500, error=3→差速±1500 */

/* ---- 误差 EMA 滤波 ---- */
#define LF_EMA_ALPHA               2       /* EMA 系数: α = 1/N, N=2 → α=0.5 */
/* 定点累加器 ×256, 衰减: N=2 时 3 ticks 从 ±3 → 0 */

/* ---- 脱线旋转 ---- */
#define LINE_FOLLOW_ROTATE_SPEED   2500    /* 脱线找回旋转速度 */

/* ---- 传感器滤波 ---- */
#define IRTRACK_RELEASE            5       /* 连续 N 次 HIGH 才释放（5ms 去抖） */

/* ---- OLED 显示 ---- */
#define LINE_FOLLOW_DISPLAY_MS     50      /* OLED 刷新间隔 ms */

#endif /* __LINE_FOLLOW_CONFIG_H */
