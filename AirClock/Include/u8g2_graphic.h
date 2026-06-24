/**
 *******************************************************************************
 * @file        u8g2_graphic.h
 * @author      ABOV R&D Division
 * @brief       u8g2_graphic
 *
 * Copyright 2025 ABOV Semiconductor Co.,Ltd. All rights reserved.
 *
 * This file is licensed under terms that are found in the LICENSE file
 * located at Document directory.
 * If this file is delivered or shared without applicable license terms,
 * the terms of the BSD-3-Clause license shall be applied.
 * Reference: https://opensource.org/licenses/BSD-3-Clause
 ******************************************************************************/

#pragma once
#ifdef __cplusplus
extern "C"
{
#endif

#include "u8g2.h"

#define SCREEN_WIDTH        128                                 // pixel count
#define SCREEN_HEIGHT       64                                  // pixel count

void fb_flush_dirty(u8x8_t* u8x8);
void fb_draw_line(int x0, int y0, int x1, int y1, bool on);
void fb_draw_string8x8_tile(int tx, int ty, const char* s);
void fb_hline(int x, int y, int w, bool on);
void fb_hline(int x, int y, int w, bool on);

void DrawCenteredString(u8g2_t* u8g2, int y, const char* s);
void DrawBox(u8g2_t* u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t w, u8g2_uint_t h);
void DrawProgressBar(u8g2_t* u8, int x, int y, int w, int h, int percent);

#ifdef __cplusplus
}
#endif

