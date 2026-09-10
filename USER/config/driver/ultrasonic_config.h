#ifndef __ULTRASONIC_CONFIG_H
#define __ULTRASONIC_CONFIG_H

/*
 * 超声测距参数配置
 *
 * HC-SR04 最大量程约 4m，对应 echo 约 23ms
 * MAX_ECHO_US 设为 35000 覆盖略超 4m 的场景
 */

/* ---- Echo 超时 ---- */
#define ULTRASONIC_MAX_ECHO_US   35000   /* Echo 超时 μs (~6m 上限) */

/* ---- 速度估算 ---- */
#define ULTRASONIC_SPEED_ALPHA   25      /* EMA α = 25/100 = 0.25 */
#define ULTRASONIC_SPEED_MIN_DT  1000    /* 最小 Δt (DWT cycles, ~14μs @72MHz) */

#endif /* __ULTRASONIC_CONFIG_H */
