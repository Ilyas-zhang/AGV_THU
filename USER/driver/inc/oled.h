/*
 * oled.h — SSD1306 128×64 OLED driver via hardware I2C1 (HAL)
 *
 * PB6 = I2C1_SCL, PB7 = I2C1_SDA
 * I2C device address: 0x3C (7-bit)
 *
 * Cursor-based drawing with multi-size font support (Font_7x10, Font_11x18, Font_16x26).
 * All drawing operates on an internal framebuffer; call OLED_Update() to push to display.
 */

#ifndef __OLED_H
#define __OLED_H

#include "oled_fonts.h"

/* ========== Display constants ========== */

#define OLED_WIDTH   128
#define OLED_HEIGHT  32
#define OLED_PAGES   (OLED_HEIGHT / 8)   /* 4 pages */

/* ========== Color enum ========== */

typedef enum {
    OLED_COLOR_BLACK = 0,
    OLED_COLOR_WHITE = 1
} OLED_Color_t;

/* ========== Initialization ========== */

/**
 * @brief  Initialize OLED display.
 *         Assumes MX_I2C1_Init() has already been called by CubeMX generated code.
 *         Sends SSD1306 init commands, clears screen, turns display ON.
 */
void OLED_Init(void);

/* ========== Screen update ========== */

/**
 * @brief  Push entire framebuffer to display. Call after drawing operations.
 */
void OLED_Update(void);

/* ========== Clear / Fill ========== */

/**
 * @brief  Clear entire framebuffer to black. Does NOT update display.
 */
void OLED_Clear(void);

/**
 * @brief  Fill entire framebuffer with a color. Does NOT update display.
 */
void OLED_Fill(OLED_Color_t color);

/* ========== Cursor ========== */

/**
 * @brief  Set cursor position for Putc/Puts.
 * @param  x  Column 0~127
 * @param  y  Row    0~63 (pixel-based, not page-based)
 */
void OLED_GotoXY(uint16_t x, uint16_t y);

/* ========== Character / String drawing ========== */

/**
 * @brief  Draw a character at cursor position using the specified font.
 *         Cursor advances by FontWidth after drawing.
 * @param  ch     ASCII character (32~127)
 * @param  Font   Pointer to font definition
 * @param  color  Foreground color
 * @return The character written, or 0 if out of bounds
 */
char OLED_Putc(char ch, const FontDef_t *Font, OLED_Color_t color);

/**
 * @brief  Draw a string at cursor position using the specified font.
 *         Cursor advances for each character.
 * @param  str    Null-terminated string
 * @param  Font  & Pointer to font definition
 * @param  color  Foreground color
 * @return Last character written, or 0 on error
 */
char OLED_Puts(const char *str, const FontDef_t *Font, OLED_Color_t color);

/* ========== Pixel drawing ========== */

/**
 * @brief  Set a single pixel in framebuffer.
 * @param  x      Column 0~127
 * @param  y      Row    0~63
 * @param  color  Pixel color
 */
void OLED_DrawPixel(uint16_t x, uint16_t y, OLED_Color_t color);

/* ========== Convenience wrappers ========== */

/**
 * @brief  Draw string at (x, y) with optional clear and update.
 * @param  str     Null-terminated string
 * @param  x       Column
 * @param  y       Row (pixel)
 * @param  Font    Font definition
 * @param  clear   If true, clear screen first
 * @param  update  If true, update display after drawing
 */
void OLED_DrawString(const char *str, uint16_t x, uint16_t y,
                     const FontDef_t *Font, uint8_t clear, uint8_t update);

/**
 * @brief  Draw a signed integer at (x, y) with specified font.
 */
void OLED_DrawInt(int32_t num, uint16_t x, uint16_t y,
                  const FontDef_t *Font, OLED_Color_t color);

/*8 ========== Power control ========== */

/**
 * @brief  Turn OLED display ON (exit sleep mode).
 */
void OLED_On(void);

/**
 * @brief  Turn OLED display OFF (enter sleep mode, < 10µA).
 */
void OLED_Off(void);

#endif /* __OLED_H */
