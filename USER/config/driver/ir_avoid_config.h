#ifndef __IR_AVOID_CONFIG_H
#define __IR_AVOID_CONFIG_H

/*
 * 红外避障参数配置
 *
 * 非对称去抖滤波：
 *   触发(检测到障碍): 立即
 *   释放(障碍消失):  需连续 N 次清零读数
 *   右侧接收管释放慢，需更大的 RELEASE 值
 */

#define IRAVOID_L_RELEASE        1       /* 左侧: 基准, 立即释放 */
#define IRAVOID_R_RELEASE        10      /* 右侧: 需10次连续清零才释放 (10ms) */

#endif /* __IR_AVOID_CONFIG_H */
