#ifndef __LINE_FOLLOW_CONFIG_H
#define __LINE_FOLLOW_CONFIG_H

/*
 * 四路循迹参数配置 — 高速平滑版
 *
 * 当前 AGV_THU 实车约定：
 *   1) 黑线 = 低电平，白地 = 高电平；
 *   2) 传感器物理从左到右为 X2, X1, X3, X4；
 *   3) 当前项目按实车验证使用：
 *      Motor3/4 = 物理左侧，Motor1/2 = 物理右侧。
 *
 * 控制目标：
 *   - 圆弧：轻反转内侧 + 高速前转外侧，减少“转一下/直一下”的抽搐；
 *   - 直角：一旦外侧探头触发或转弯中丢线，自动升级为强差速；
 *   - 转弯方向带锁定/滞回，短暂反向传感器噪声不会立刻反打；
 *   - 中心连续稳定后才恢复直行，保持“先转正，再直行”。
 */

/* ---- 传感器/电机物理映射 ---- */
#define LINE_FOLLOW_SWAP_X1_X2        1   /* 1: 左→右 = X2 X1 X3 X4；0: X1 X2 X3 X4 */
#define LINE_FOLLOW_SWAP_MOTOR_SIDES  1   /* 1: M3/M4=左侧, M1/M2=右侧；0: M1/M2=左侧 */

/* ---- 高速直行（PWM ARR = 3599） ---- */
#define LINE_FOLLOW_BASE_SPEED              2850

/* ---- 转弯参数：外侧前推速度 + 旋转比率 (0~100) ----
 * inner_reverse = outer_forward × ratio / 100（编译期计算）
 *
 * ratio 含义：
 *   0   = 纯直行（无内侧反转，不转弯）
 *   50  = 内侧反转 = 外侧一半 → 温和弧线
 *   100 = 内侧 = 外侧 → 原地旋转
 *
 * 原值(转弯不足): ARC 850/3150=27%  CORNER 1900/3350=57%
 *                 SETTLE 650/2650=25%  SEARCH 1350/3100=44%
 */

/* 圆弧：常规偏移修正 */
#define LINE_FOLLOW_ARC_OUTER_FORWARD       3000
#define LINE_FOLLOW_ARC_TURN_RATIO          50

/* 直角/严重偏离：大拐弯 */
#define LINE_FOLLOW_CORNER_OUTER_FORWARD    3200
#define LINE_FOLLOW_CORNER_TURN_RATIO       81

/* 收尾：即将转正，减小力度防过冲 */
#define LINE_FOLLOW_SETTLE_OUTER_FORWARD    2400
#define LINE_FOLLOW_SETTLE_TURN_RATIO       42

/* 丢线搜索：找线旋转 */
#define LINE_FOLLOW_SEARCH_OUTER_FORWARD    2900
#define LINE_FOLLOW_SEARCH_TURN_RATIO       69

/* ---- 派生：内侧反转速度（由比率自动计算，无需手动改） ---- */
#define LINE_FOLLOW_ARC_INNER_REVERSE \
    ((int16_t)((int32_t)LINE_FOLLOW_ARC_OUTER_FORWARD * LINE_FOLLOW_ARC_TURN_RATIO / 100))
#define LINE_FOLLOW_CORNER_INNER_REVERSE \
    ((int16_t)((int32_t)LINE_FOLLOW_CORNER_OUTER_FORWARD * LINE_FOLLOW_CORNER_TURN_RATIO / 100))
#define LINE_FOLLOW_SETTLE_INNER_REVERSE \
    ((int16_t)((int32_t)LINE_FOLLOW_SETTLE_OUTER_FORWARD * LINE_FOLLOW_SETTLE_TURN_RATIO / 100))
#define LINE_FOLLOW_SEARCH_INNER_REVERSE \
    ((int16_t)((int32_t)LINE_FOLLOW_SEARCH_OUTER_FORWARD * LINE_FOLLOW_SEARCH_TURN_RATIO / 100))

/* ---- 原地差速转弯（已注释，保留回退） ----
 * 内外侧等速反转，纯旋转无平移。
 *
#define LINE_FOLLOW_ARC_SPIN_SPEED          2800
#define LINE_FOLLOW_CORNER_SPIN_SPEED       3200
#define LINE_FOLLOW_SETTLE_SPIN_SPEED       2000
#define LINE_FOLLOW_SEARCH_SPIN_SPEED       2600
*/

/* ---- PWM 斜坡 ----
 * 同向变化保持较快；发生正/反换向时先快速制到 0，再较慢进入反向。
 * 这样既不会拖慢直角响应，也不会瞬间反接造成机械抽搐。
 */
#define LINE_FOLLOW_SLEW_STEP                 420
#define LINE_FOLLOW_REVERSE_BRAKE_STEP        650
#define LINE_FOLLOW_REVERSE_ACCEL_STEP        320

/* ---- 状态滞回/确认（控制周期 1 ms） ---- */
#define LINE_FOLLOW_TURN_MIN_HOLD_MS            7  /* 进入转弯后至少保持，过滤瞬时“回中” */
#define LINE_FOLLOW_CENTER_CONFIRM_MS            6  /* 中心连续稳定后才恢复直行 */
#define LINE_FOLLOW_OPPOSITE_CONFIRM_MS          5  /* 反方向连续出现才允许反打 */
#define LINE_FOLLOW_LOST_GRACE_MS                2  /* 直行时短暂全白不立刻甩头 */
#define LINE_FOLLOW_LOST_TO_CORNER_MS            2  /* 转弯中持续丢线则升级直角力度 */

/* ---- 传感器滤波：入黑线立即响应，离开黑线稍慢 ---- */
#define IRTRACK_RELEASE                          3

/* ---- OLED 调试 ---- */
#define LINE_FOLLOW_OLED_ENABLE                  0
#define LINE_FOLLOW_DISPLAY_MS                 100

#endif /* __LINE_FOLLOW_CONFIG_H */
