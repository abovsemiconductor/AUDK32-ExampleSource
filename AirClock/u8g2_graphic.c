/**
 *******************************************************************************
 * @file        u8g2_graphic.c
 * @author      ABOV R&D Division
 * @brief       u8g2 Graphic Functions with Framebuffer
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
#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "hal_common.h"
#include "u8g2_graphic.h"

//!!!!!!!!!!!!!!!!!
// x is not available if it is greater than 125

#define SCREEN_WIDTH        128                                 // pixel count
#define SCREEN_HEIGHT       64                                  // pixel count
#define TILE_SIZE           8                                   // byte
#define TILE_X_COUNT        (SCREEN_WIDTH / TILE_SIZE)          // 16
#define TILE_Y_COUNT        (SCREEN_HEIGHT / TILE_SIZE)         // 8   


static uint8_t fb[(TILE_X_COUNT * TILE_Y_COUNT) * TILE_SIZE];   // (16x8)x8 = 1024 byte
static uint8_t dirty_tile[TILE_X_COUNT * TILE_Y_COUNT];         // 16 x 8 byte = 128 byte

static inline void fb_clear(void)
{
    memset(fb, 0, sizeof(fb));
    memset(dirty_tile, 0, sizeof(dirty_tile));
}

// set dirty tile
static inline void fb_set_pixel(int x, int y, bool on) 
{
    if ((unsigned)x >= 128 || (unsigned)y >= 64) 
        return;
    uint16_t idx  = (y >> 3) * 128 + x;     // page * width + x, (y/8) x 128 + x
    uint8_t  mask = 1 << (y & 7);          // 1 << (y%8)
    if (on) 
        fb[idx] |=  mask; 
    else 
        fb[idx] &= ~mask;
    dirty_tile[(y >> 3) * 16 + (x >> 3)] = 1; 
}

void fb_flush_dirty(u8x8_t *u8x8) 
{
    uint8_t tile[8];
    for (int ty = 0; ty < 8; ++ty) 
    {
        for (int tx = 0; tx < 16; ++tx) 
        {
            int di = ty * 16 + tx;
            if (!dirty_tile[di]) 
                continue;
            dirty_tile[di] = 0;

            int x0 = tx * 8;
            for (int k = 0; k < 8; ++k)
                tile[k] = fb[ty * 128 + (x0 + k)];

            u8x8_DrawTile(u8x8, tx, ty, 1, tile);
        }
    }
}

void fb_draw_line(int x0, int y0, int x1, int y1, bool on) 
{
    int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int sx = (x0 < x1) ? 1 : -1;
    int dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    int sy = (y0 < y1) ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2, e2;

    for (;;) 
    {
        fb_set_pixel(x0, y0, on);
        if (x0 == x1 && y0 == y1) 
            break;
        e2 = err;
        if (e2 > -dx) 
        { 
            err -= dy; x0 += sx; 
        }
        if (e2 <  dy) 
        { 
            err += dx; y0 += sy; 
        }
    }
}

void fb_hline(int x, int y, int w, bool on) 
{
    int x1 = x + w - 1;
    if (w <= 0 ) 
        return;
    if (y < 0 || y >= SCREEN_HEIGHT) 
        return;
    if (x < 0) 
        x = 0;
    if (x1 >= SCREEN_WIDTH) 
        x1 = SCREEN_WIDTH - 1;
    for (int xx = x; xx <= x1; xx++) 
        fb_set_pixel(xx, y, on);
}

void fb_vline(int x, int y, int h, bool on) 
{
    int y1 = y + h - 1;
    if (h <= 0) 
        return;
    if ( x < 0 || x >= SCREEN_WIDTH)
        return;
    if (y < 0) 
        y=0;
    if (y1 >= SCREEN_HEIGHT) 
        y1 = SCREEN_HEIGHT - 1;
    for (int yy = y; yy <= y1; yy++)
        fb_set_pixel(x, yy, on);
}


static inline void fb_draw_char8x8_tile(int tx, int ty, char c)
{
    if ((unsigned)tx >= TILE_X_COUNT || (unsigned)ty >= TILE_Y_COUNT)
        return;
    int x0 = tx * 8;
    uint16_t base = ty * SCREEN_WIDTH + x0;

    const uint8_t *g = &u8x8_font_chroma48medium8_r[(unsigned char)c];
    for (int k=0; k<8; ++k) 
    {
        fb[base + k] |= g[k];
    }
    dirty_tile[ty*TILE_X_COUNT + tx] = 1;
}

void fb_draw_string8x8_tile(int tx, int ty, const char* s) 
{
    for (; *s; ++s, ++tx) 
    {
        if ((unsigned)tx >= TILE_X_COUNT) 
            break;
        fb_draw_char8x8_tile(tx, ty, *s);
    }
}

void DrawCenteredString(u8g2_t* u8g2, int y, const char* s)
{
    int w = u8g2_GetDisplayWidth(u8g2);
    w = w - 2;
    int sw = u8g2_GetStrWidth(u8g2, s);
    u8g2_DrawStr(u8g2, (w - sw)/2, y, s);
}
void DrawBox(u8g2_t* u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t w, u8g2_uint_t h)
{
    if (w == 0 || h == 0)
        return;

    if (w > h)
    {
        for (u8g2_uint_t dy = 0; dy < h; ++dy)
        {
            u8g2_DrawHVLine(u8g2, x, y + dy, w, 0 /*H*/);
        }
    }
    else
    {
        for (u8g2_uint_t dx = 0; dx < w; ++dx)
        {
            u8g2_DrawHVLine(u8g2, x + dx, y, h, 1 /*V*/);
        }
    }
}
void DrawProgressBar(u8g2_t* u8, int x, int y, int w, int h, int percent)
{
    if (percent < 0)
        percent = 0;
    if (percent > 100)
        percent = 100;

    int fill = (w - 2) * percent / 100;
    if (fill > 0)
        DrawBox(u8, x + 1, y + 1, fill, h - 2);
}