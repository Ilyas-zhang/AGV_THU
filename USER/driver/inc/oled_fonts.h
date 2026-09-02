/*
 * oled_fonts.h — Font definitions for SSD1306 OLED driver
 *
 * Based on font library by Nima Mohammadi (GPL v3)
 * Adapted for STM32 HAL + FontDef_t structure
 */

#ifndef __OLED_FONTS_H
#define __OLED_FONTS_H

#include <stdint.h>

/**
 * @brief  Font definition structure
 */
typedef struct {
    uint8_t  FontWidth;    /*!< Font width in pixels */
    uint8_t  FontHeight;   /*!< Font height in pixels */
    const uint16_t *data;  /*!< Pointer to font data array */
} FontDef_t;

/**
 * @brief  7×10 pixels font (compact, fits ~18 chars per line on 128px)
 */
extern FontDef_t Font_7x10;

/**
 * @brief  11×18 pixels font (medium, fits ~11 chars per line)
 */
extern FontDef_t Font_11x18;

/**
 * @brief  16×26 pixels font (large, fits ~8 chars per line)
 */
extern FontDef_t Font_16x26;

#endif /* __OLED_FONTS_H */
