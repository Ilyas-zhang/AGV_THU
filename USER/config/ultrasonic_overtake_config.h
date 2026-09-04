#ifndef __ULTRASONIC_OVERTAKE_CONFIG_H
#define __ULTRASONIC_OVERTAKE_CONFIG_H

/*
 * 超声超车避障参数配置
 *
 * 超车动作参数在 overtake_config.h 中统一管理。
 * 此文件仅包含超声检测相关的参数。
 */

/* ---- 距离阈值 (mm) ---- */
#define UO_STOP_DIST_MM          150     /* 停车阈值 15cm */

/* ---- 超声触发 ---- */
#define UO_TRIGGER_MS            60      /* 触发间隔 ms */

/* ---- 直行保护期 ---- */
#define UO_FORWARD_LOCK_MS       200     /* 超车完成后前200ms不检测距离 */

#endif /* __ULTRASONIC_OVERTAKE_CONFIG_H */
