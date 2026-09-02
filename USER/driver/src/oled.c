/*
 * oled.c — SSD1306 128×64 OLED driver via hardware I2C1 (HAL)
 *
 * PB6 = I2C1_SCL, PB7 = I2C1_SDA
 * I2C 7-bit address: 0x3C
 *
 * Uses HAL_I2C_Mem_Write() for efficient command/data transfer.
 * Cursor-based drawing with FontDef_t multi-size font system.
 * Internal 1024-byte framebuffer; call OLED_Update() to push to display.
 */

#include "oled.h"
#include "i2c.h"       /* hi2c1 from CubeMX */
#include <string.h>

/* ========== I2C helpers ========== */

#define OLED_I2C_ADDR    (0x3C << 1)   /* HAL expects address left-shifted */
#define OLED_CTRL_CMD    0x00          /* Control byte: command */
#define OLED_CTRL_DATA   0x40          /* Control byte: data */
#define OLED_I2C_TIMEOUT 100           /* ms timeout for HAL I2C calls */

/**
 * @brief  Write a single command byte to SSD1306.
 */
static void OLED_WriteCmd(uint8_t cmd)
{
    HAL_I2C_Mem_Write(&hi2c1, OLED_I2C_ADDR, OLED_CTRL_CMD,
                      I2C_MEMADD_SIZE_8BIT, &cmd, 1, OLED_I2C_TIMEOUT);
}

/**
 * @brief  Write a data burst to SSD1306.
 */
static void OLED_WriteData(const uint8_t *data, uint16_t len)
{
    HAL_I2C_Mem_Write(&hi2c1, OLED_I2C_ADDR, OLED_CTRL_DATA,
                      I2C_MEMADD_SIZE_8BIT, (uint8_t *)data, len, OLED_I2C_TIMEOUT);
}

/* ========== Framebuffer ========== */

static uint8_t SSD1306_Buffer[OLED_WIDTH * OLED_PAGES];  /* 1024 bytes */

/* ========== Cursor state ========== */

typedef struct {
    uint16_t CurrentX;
    uint16_t CurrentY;
} SSD1306_Cursor_t;

static SSD1306_Cursor_t Cursor;

/* ========== Public API ========== */

void OLED_Init(void)
{
    /*
     * Note: MX_I2C1_Init() is called before OLED_Init() in main.c.
     * The HAL_Delay gives the OLED controller time to settle after I2C bus init.
     */
    HAL_Delay(100);

    /* SSD1306 initialization sequence for 128×32 */
    OLED_WriteCmd(0xAE);       /* Display OFF */
    OLED_WriteCmd(0xD5);       /* Set display clock divide ratio */
    OLED_WriteCmd(0x80);       /* Suggested ratio 0x80 */
    OLED_WriteCmd(0xA8);       /* Set multiplex ratio */
    OLED_WriteCmd(0x1F);       /* 1/32 duty (32 lines) */
    OLED_WriteCmd(0xD3);       /* Set display offset */
    OLED_WriteCmd(0x00);       /* No offset */
    OLED_WriteCmd(0x40);       /* Set start line = 0 */
    OLED_WriteCmd(0x8D);       /* Charge pump setting */
    OLED_WriteCmd(0x14);       /* Enable charge pump (0x14=enable, 0x10=disable) */
    OLED_WriteCmd(0x20);       /* Set memory addressing mode */
    OLED_WriteCmd(0x02);       /* Page addressing mode */
    OLED_WriteCmd(0xA1);       /* Set segment remap (column 128→1) */
    OLED_WriteCmd(0xC8);       /* Set COM output scan direction (remapped) */
    OLED_WriteCmd(0xDA);       /* Set COM pins hardware config */
    OLED_WriteCmd(0x02);       /* 128×32: sequential COM pin config */
    OLED_WriteCmd(0x81);       /* Set contrast control */
    OLED_WriteCmd(0xCF);       /* Contrast level */
    OLED_WriteCmd(0xD9);       /* Set pre-charge period */
    OLED_WriteCmd(0xF1);       /* Pre-charge: 15 clocks, discharge: 1 clock */
    OLED_WriteCmd(0xDB);       /* Set VCOMH deselect level */
    OLED_WriteCmd(0x40);       /* VCOMH = 0.77 × Vcc */
    OLED_WriteCmd(0x2E);       /* Disable scroll */
    OLED_WriteCmd(0xA4);       /* Entire display ON: follow RAM content */
    OLED_WriteCmd(0xA6);       /* Set normal display (not inverted) */
    OLED_WriteCmd(0xAF);       /* Display ON */

    /* Clear screen and push to display */
    OLED_Clear();
    OLED_Update();

    /* Reset cursor */
    Cursor.CurrentX = 0;
    Cursor.CurrentY = 0;
}

void OLED_Update(void)
{
    for (uint8_t page = 0; page < OLED_PAGES; page++) {
        OLED_WriteCmd(0xB0 + page);   /* Set page address */
        OLED_WriteCmd(0x00);           /* Set lower column = 0 */
        OLED_WriteCmd(0x10);           /* Set higher column = 0 */

        /* Send entire page (128 bytes) in one I2C transaction */
        OLED_WriteData(&SSD1306_Buffer[page * OLED_WIDTH], OLED_WIDTH);
    }
}

void OLED_Clear(void)
{
    memset(SSD1306_Buffer, 0x00, sizeof(SSD1306_Buffer));
}

void OLED_Fill(OLED_Color_t color)
{
    memset(SSD1306_Buffer, (color == OLED_COLOR_BLACK) ? 0x00 : 0xFF,
           sizeof(SSD1306_Buffer));
}

void OLED_GotoXY(uint16_t x, uint16_t y)
{
    Cursor.CurrentX = x;
    Cursor.CurrentY = y;
}

void OLED_DrawPixel(uint16_t x, uint16_t y, OLED_Color_t color)
{
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT) return;

    if (color == OLED_COLOR_WHITE) {
        SSD1306_Buffer[x + (y / 8) * OLED_WIDTH] |=  (1 << (y % 8));
    } else {
        SSD1306_Buffer[x + (y / 8) * OLED_WIDTH] &= ~(1 << (y % 8));
    }
}

char OLED_Putc(char ch, const FontDef_t *Font, OLED_Color_t color)
{
    uint32_t i, b, j;

    /* Check bounds */
    if (OLED_WIDTH  <= (Cursor.CurrentX + Font->FontWidth) ||
        OLED_HEIGHT <= (Cursor.CurrentY + Font->FontHeight)) {
        return 0;
    }

    /* Draw character pixel by pixel */
    for (i = 0; i < Font->FontHeight; i++) {
        b = Font->data[(ch - 32) * Font->FontHeight + i];
        for (j = 0; j < Font->FontWidth; j++) {
            if ((b << j) & 0x8000) {
                OLED_DrawPixel(Cursor.CurrentX + j, Cursor.CurrentY + i, color);
            } else {
                OLED_DrawPixel(Cursor.CurrentX + j, Cursor.CurrentY + i,
                               (OLED_Color_t)!color);
            }
        }
    }

    /* Advance cursor */
    Cursor.CurrentX += Font->FontWidth;

    return ch;
}

char OLED_Puts(const char *str, const FontDef_t *Font, OLED_Color_t color)
{
    while (*str) {
        if (OLED_Putc(*str, Font, color) != *str) {
            return *str;  /* Error: out of bounds */
        }
        str++;
    }
    return *str;  /* Success: returns '\0' */
}

/* ========== Convenience wrappers ========== */

void OLED_DrawString(const char *str, uint16_t x, uint16_t y,
                     const FontDef_t *Font, uint8_t clear, uint8_t update)
{
    if (clear)  OLED_Clear();
    OLED_GotoXY(x, y);
    OLED_Puts(str, Font, OLED_COLOR_WHITE);
    if (update) OLED_Update();
}

void OLED_DrawInt(int32_t num, uint16_t x, uint16_t y,
                  const FontDef_t *Font, OLED_Color_t color)
{
    char buf[12];
    int pos = 0;

    if (num < 0) {
        buf[pos++] = '-';
        num = -num;
    } else if (num == 0) {
        buf[pos++] = '0';
    }

    /* Build digits in reverse */
    char rev[10];
    int rpos = 0;
    while (num > 0) {
        rev[rpos++] = '0' + (num % 10);
        num /= 10;
    }
    while (rpos > 0) {
        buf[pos++] = rev[--rpos];
    }
    buf[pos] = '\0';

    OLED_GotoXY(x, y);
    OLED_Puts(buf, Font, color);
}

/* ========== Power control ========== */

void OLED_On(void)
{
    OLED_WriteCmd(0x8D);
    OLED_WriteCmd(0x14);   /* Enable charge pump */
    OLED_WriteCmd(0xAF);   /* Display ON */
}

void OLED_Off(void)
{
    OLED_WriteCmd(0x8D);
    OLED_WriteCmd(0x10);   /* Disable charge pump */
    OLED_WriteCmd(0xAE);   /* Display OFF */
}
