#include "led.h"
#include "led_config.h"
#include "main.h"

/* ---- internal types ---- */
typedef struct {
    uint8_t r;      /* intensity 0~100 */
    uint8_t g;
    uint8_t b;
    uint8_t blink;  /* 0=steady, 1=slow blink(1Hz), 2=fast blink(5Hz) */
} LED_RGB;

/* ---- private state ---- */
static LED_RGB led_left  = {0, 0, 0, 0};
static LED_RGB led_right = {0, 0, 0, 0};

static volatile uint8_t  pwm_cnt          = 0;   /* software PWM step */
static volatile uint16_t blink_slow_cnt   = 0;   /* ms counter for slow blink */
static volatile uint16_t blink_fast_cnt   = 0;   /* ms counter for fast blink */
static volatile uint8_t  blink_slow_state = 1;   /* 1=on, 0=off (1Hz) */
static volatile uint8_t  blink_fast_state = 1;   /* 1=on, 0=off (5Hz) */

/* ---- clamp helper ---- */
static inline uint8_t clamp100(uint8_t v)
{
    return (v > 100) ? 100 : v;
}

/* ---- get blink-off state for a given blink mode ---- */
/* returns 1 if LED should be suppressed (off-phase), 0 if lit */
static inline uint8_t blink_is_off(uint8_t mode)
{
    if (mode == 1) return !blink_slow_state;
    if (mode == 2) return !blink_fast_state;
    return 0;   /* mode 0 = steady, always on */
}

/* ---- public API ---- */

void LED_Init(void)
{
    led_left.r = led_left.g = led_left.b = led_left.blink = 0;
    led_right.r = led_right.g = led_right.b = led_right.blink = 0;

    /* Turn off all LED pins (active-high: RESET = off) */
    HAL_GPIO_WritePin(GPIOE, RRGB_R_Pin | RRGB_G_Pin | RRGB_B_Pin | LRGB_G_Pin,
                      GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOG, LRGB_R_Pin | LRGB_B_Pin,
                      GPIO_PIN_RESET);
}

void LED_Set(uint8_t l_r, uint8_t l_g, uint8_t l_b, uint8_t l_blink,
             uint8_t r_r, uint8_t r_g, uint8_t r_b, uint8_t r_blink)
{
    led_left.r     = clamp100(l_r);
    led_left.g     = clamp100(l_g);
    led_left.b     = clamp100(l_b);
    led_left.blink = (l_blink > 2) ? 2 : l_blink;

    led_right.r     = clamp100(r_r);
    led_right.g     = clamp100(r_g);
    led_right.b     = clamp100(r_b);
    led_right.blink = (r_blink > 2) ? 2 : r_blink;
}

void LED_Tick(void)
{
    /* ---- slow blink state machine (1 Hz) ---- */
    if (++blink_slow_cnt >= LED_BLINK_SLOW_HALF) {
        blink_slow_cnt = 0;
        blink_slow_state = !blink_slow_state;
    }

    /* ---- fast blink state machine (5 Hz) ---- */
    if (++blink_fast_cnt >= LED_BLINK_FAST_HALF) {
        blink_fast_cnt = 0;
        blink_fast_state = !blink_fast_state;
    }

    /* ---- PWM counter ---- */
    if (++pwm_cnt >= LED_PWM_PERIOD) {
        pwm_cnt = 0;
    }

    /* ---- effective intensities (blink suppresses to 0 during off-phase) ---- */
    uint8_t l_r, l_g, l_b;
    uint8_t r_r, r_g, r_b;

    if (blink_is_off(led_left.blink)) {
        l_r = l_g = l_b = 0;
    } else {
        l_r = led_left.r;
        l_g = led_left.g;
        l_b = led_left.b;
    }

    if (blink_is_off(led_right.blink)) {
        r_r = r_g = r_b = 0;
    } else {
        r_r = led_right.r;
        r_g = led_right.g;
        r_b = led_right.b;
    }

    /* ---- GPIO: compute set/reset masks and write BSRR (atomic) ---- */
    uint32_t e_set = 0, e_reset = 0;
    uint32_t g_set = 0, g_reset = 0;

    /* GPIOE: Right R (PE2), Right G (PE3), Right B (PE4), Left G (PE7) */
    if (pwm_cnt < r_r) e_set   |= RRGB_R_Pin; else e_reset |= RRGB_R_Pin;
    if (pwm_cnt < r_g) e_set   |= RRGB_G_Pin; else e_reset |= RRGB_G_Pin;
    if (pwm_cnt < r_b) e_set   |= RRGB_B_Pin; else e_reset |= RRGB_B_Pin;
    if (pwm_cnt < l_g) e_set   |= LRGB_G_Pin; else e_reset |= LRGB_G_Pin;

    /* GPIOG: Left R (PG1), Left B (PG2) */
    if (pwm_cnt < l_r) g_set   |= LRGB_R_Pin; else g_reset |= LRGB_R_Pin;
    if (pwm_cnt < l_b) g_set   |= LRGB_B_Pin; else g_reset |= LRGB_B_Pin;

    /* BSRR: bits [15:0] set pins, bits [31:16] reset pins */
    GPIOE->BSRR = e_set | (e_reset << 16);
    GPIOG->BSRR = g_set | (g_reset << 16);
}
