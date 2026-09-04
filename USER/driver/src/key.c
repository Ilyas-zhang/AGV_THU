#include "key.h"
#include "key_config.h"
#include "main.h"

/* ---- per-key state (active-low: pressed = pin LOW) ---- */
typedef struct {
    uint8_t stable;          /* 0 = released, 1 = pressed (debounced) */
    uint8_t debounce_cnt;    /* consecutive readings counter */
    uint8_t rising_edge;    /* set for one tick on release→press transition */
} KeyState;

static KeyState keys[3];    /* index 0 = KEY1, 1 = KEY2, 2 = KEY3 */

/* ---- helper: read raw pin (active-low) ---- */
static uint8_t read_key_raw(uint8_t idx)
{
    switch (idx) {
    case 0: return (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_RESET) ? 1 : 0;
    case 1: return (HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_RESET) ? 1 : 0;
    case 2: return (HAL_GPIO_ReadPin(KEY3_GPIO_Port, KEY3_Pin) == GPIO_PIN_RESET) ? 1 : 0;
    default: return 0;
    }
}

/* ---- public API ---- */

void Key_Init(void)
{
    for (int i = 0; i < 3; i++) {
        keys[i].stable        = 0;
        keys[i].debounce_cnt  = 0;
        keys[i].rising_edge   = 0;
    }
}

void Key_Tick(void)
{
    for (int i = 0; i < 3; i++) {
        uint8_t now = read_key_raw(i);
        keys[i].rising_edge = 0;   /* clear edge flag each tick */

        if (now != keys[i].stable) {
            if (++keys[i].debounce_cnt >= KEY_DEBOUNCE_MS) {
                keys[i].stable = now;
                keys[i].debounce_cnt = 0;
                /* Rising edge: just pressed */
                if (keys[i].stable) {
                    keys[i].rising_edge = 1;
                }
            }
        } else {
            keys[i].debounce_cnt = 0;
        }
    }
}

uint8_t Key_Pressed(uint8_t key_id)
{
    if (key_id < 1 || key_id > 3) return 0;
    return keys[key_id - 1].stable;
}

uint8_t Key_RisingEdge(uint8_t key_id)
{
    if (key_id < 1 || key_id > 3) return 0;
    return keys[key_id - 1].rising_edge;
}
