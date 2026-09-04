#ifndef __LINE_FOLLOW_CONFIG_H
#define __LINE_FOLLOW_CONFIG_H

/*
 * 循迹运动参数配置 — 状态驱动 + 转弯减速
 *
 * 直行用 BASE_SPEED，转弯降到 TURN_SPEED
 * 减速降低惯性，转向更灵活
 *
 * 调参：
 *   TURN_SPEED 越低 → 转弯越灵活，但过弯速度慢
 *   TURN_SLOW  越低 → 小转弯差速越猛
 */

/* ---- 速度 ---- */
#define LINE_FOLLOW_BASE_SPEED     2500    /* 直行速度 0~3599 */
#define LINE_FOLLOW_TURN_SPEED     1800    /* 转弯速度（旋转/差速时用） */

/* ---- 小转弯慢侧速度 ---- */
#define LINE_FOLLOW_TURN_SLOW       200    /* 小转弯慢侧，接近停转 */

/* ---- 传感器滤波 ---- */
#define IRTRACK_RELEASE            1       /* 即时释放，无延迟 */

/* ---- OLED 显示 ---- */
#define LINE_FOLLOW_DISPLAY_MS     50      /* OLED 刷新间隔 ms */

#endif /* __LINE_FOLLOW_CONFIG_H */
