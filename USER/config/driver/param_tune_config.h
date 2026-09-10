#ifndef __PARAM_TUNE_CONFIG_H
#define __PARAM_TUNE_CONFIG_H

/*
 * 按键调参驱动参数配置
 *
 * 长按 K1 进入调参模式，短按 K1 切换参数，K2+/K3-，再长按 K1 退出
 */

#define PT_LONG_PRESS_MS    1000    /* 长按阈值 ms */
#define PT_MAX_PARAMS       8       /* 最多注册参数数 */
#define PT_DISPLAY_MS       100     /* 调参模式 OLED 刷新间隔 ms */

#endif /* __PARAM_TUNE_CONFIG_H */
