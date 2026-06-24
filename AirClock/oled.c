/**
 *******************************************************************************
 * @file        oled.c
 * @author      ABOV R&D Division
 * @brief       EXAMPLE OLED Driver Interface
 *
 * Copyright 2025 ABOV Semiconductor Co.,Ltd. All rights reserved.
 *
 * This file is licensed under terms that are found in the LICENSE file
 * located at Document directory.
 * If this file is delivered or shared without applicable license terms,
 * the terms of the BSD-3-Clause license shall be applied.
 * Reference: https://opensource.org/licenses/BSD-3-Clause
 ******************************************************************************/

#include "abov_config.h"
#include "u8g2.h"
#include "Include/oled.h"

extern uint8_t u8x8_byte_abov_hw_spi(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
extern uint8_t u8x8_abov_gpio_and_delay(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

void EX_OLED_Init(u8g2_t *u8g2)
{
    u8g2_Setup_ssd1309_128x64_noname2_f(u8g2, U8G2_R0, u8x8_byte_abov_hw_spi, u8x8_abov_gpio_and_delay);
    u8g2_InitDisplay(u8g2);
    u8g2_SetPowerSave(u8g2, 0);
    //u8g2_ClearDisplay(u8g2);
    EX_OLED_SetFont(u8g2, FONT_MONOCHROME_NORMAL);
}

void EX_OLED_SetFont(u8g2_t *u8g2, Font_U8X8_t font)
{
    switch (font)
    {
        case FONT_MONOCHROME_NORMAL:
            u8g2_SetFont(u8g2, u8g2_font_6x10_tr);
            u8g2_SetFontPosTop(u8g2);
            break;

        case FONT_MONOCHROME_MEDIUM:
            u8g2_SetFont(u8g2, u8g2_font_8x13_tr);
            u8g2_SetFontPosTop(u8g2);
            break;

        case FONT_MONOCHROME_LARGE:
            u8g2_SetFont(u8g2, u8g2_font_10x20_tf);
            u8g2_SetFontPosTop(u8g2);
            break;

        default:
            break;
    }
}

void EX_OLED_abovlogo_display(u8g2_t *u8g2)
{
    for (uint8_t row = 0; row < 8; row++)
    {
        uint8_t *line_ptr = abov_logo_bitmap + row * 16 * 8;
        u8x8_DrawTile(&u8g2->u8x8, 0, row, 16, line_ptr);
    }
}