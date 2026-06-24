/**
 *******************************************************************************
 * @file        leddrv_al8700.c
 * @author      ABOV R&D Division
 * @brief       EXAMPLE AL8700 LED Driver Interface
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

#include "hal_pcu.h"
#include "hal_i2c.h"
#include "hal_timer1.h"

#include "debug_log.h"
#include "debug.h"

#include "leddrv_al8700.h"

#define USE_PATTERNS
//#define USE_ICON

#define LEDDRV_AL8700_SLAVE_ADDR        (0xE8 >> 1)
#define LEDDRV_AL8700_CMD_DON           0x10
#define LEDDRV_AL8700_CMD_DISPMODE      0x20
#define LEDDRV_AL8700_CMD_DIMMODE       0x30
#define LEDDRV_AL8700_CMD_ADDRSET       0x50

#define LEDDRV_AL8700_D_LINE_MAX        8


#if defined(USE_PATTERNS)
#define LEDDRV_ROWS                     8
#define LEDDRV_COLS                     14
#define LEDDRV_TICK_MS                  50
#define LEDDRV_PATTERN_HOLD_MS          2500
#define LEDDRV_SPEED_PX_PER_TICK        1
#define LEDDRV_PATTERN_HOLD_TICKS \
    ((LEDDRV_PATTERN_HOLD_MS + LEDDRV_TICK_MS - 1) / LEDDRV_TICK_MS)

typedef enum
{
    PATTERN_MARQUEE_BAR = 0,
    PATTERN_DIAGONAL,
    PATTERN_SINE_WAVE,
    PATTERN_RAIN,
    PATTERN_SPARKLE,
    PATTERN_SCANNER,
    PATTERN_CENTER_OUT,
    PATTERN_CENTER_OUT_FILL,
    PATTERN_WIPE_LR,
    PATTERN_WIPE_OUTSIDE_IN,
    PATTERN_FIRE,
    PATTERN_RIPPLE,
    PATTERN_RADAR,
    PATTERN_MAX
} PATTERN_e;

static volatile uint16_t s_tick_count = 0;
static uint8_t s_hold_tick = 0;
static PATTERN_e s_pattern = PATTERN_MARQUEE_BAR;
static int8_t s_scan_dir = 1;
static uint8_t s_scan_x = 0;
static volatile uint8_t s_led_update_req = 0;
#endif

#if defined(USE_ICON)
static const uint8_t s_un8Icon_Sun[LEDDRV_AL8700_D_LINE_MAX][2] = { { 0x11, 0x20 }, { 0x0B, 0x40 }, { 0x04, 0x80 }, { 0x38, 0x70 }, { 0x08, 0x40 }, { 0x04, 0x80 }, { 0x0B, 0x40 }, { 0x11, 0x20 }  };
static const uint8_t s_un8Icon_Cloud[LEDDRV_AL8700_D_LINE_MAX][2] = { { 0x01, 0x80 }, { 0x03, 0xC0 }, { 0x03, 0xE0 }, { 0x03, 0xE0 }, { 0x03, 0xE0 }, { 0x03, 0xE0 }, { 0x03, 0xC0 }, { 0x01, 0x80 } };
static const uint8_t s_un8Icon_Umbrella[LEDDRV_AL8700_D_LINE_MAX][2] = { { 0x00, 0xC0 }, { 0x00, 0xA0 }, { 0x00, 0x90 }, { 0x3F, 0x90 }, { 0x40, 0x90 }, { 0x20, 0x90 }, { 0x00, 0xA0 }, { 0x00, 0xC0 } };
static const uint8_t s_un8Icon_Snow[LEDDRV_AL8700_D_LINE_MAX][2] = { { 0x14, 0x00 }, { 0x08, 0x00 }, { 0x3E, 0x00 }, { 0x08, 0x50 }, { 0x14, 0x20 }, { 0x00, 0xF8 }, { 0x00, 0x20 }, { 0x00, 0x50 } };
static const uint8_t s_un8Icon_Lightning[LEDDRV_AL8700_D_LINE_MAX][2] = { { 0x01, 0x00 }, { 0x21, 0x80 }, { 0x19, 0xC0 }, { 0x0F, 0xE0 }, { 0x07, 0x30 }, { 0x03, 0x08 }, { 0x01, 0x00 }, { 0x00, 0x00 } };
#endif

static volatile bool s_bTxComplete = false;
#if defined(USE_ICON)
static uint8_t s_un8CurIconIdx = LEDDRV_ICON_MAX;
#endif

#if defined(USE_ICON)
static void PRV_LEDDRV_LedAllOnOff(bool bOnOff);
#endif

#if defined(USE_PATTERNS)
static void PRV_PatternAdvance(void);
static void PRV_RenderAndPush(void);
#endif

static void PRV_LEDDRV_I2CHandler(uint32_t un32Event, void *pContext)
{
    if(un32Event & I2C_EVENT_TX_DONE)
    {
        s_bTxComplete = true;
    }

    if(un32Event & I2C_EVENT_RX_DONE)
    {
        ;
    }
}

static HAL_ERR_e PRV_LEDDRV_Write(uint8_t *pun8Buf, uint32_t un32Len)
{
    uint32_t un32Timeout = 1000000;
    s_bTxComplete = false;

    HAL_ERR_e eErr = HAL_I2C_Transmit(I2C_ID_0, LEDDRV_AL8700_SLAVE_ADDR, pun8Buf, un32Len, false);
    if(eErr != HAL_ERR_OK)
    {
        return eErr;
    }

    while(!s_bTxComplete && un32Timeout)
    {
        un32Timeout--;
    }

    if (!un32Timeout)
    {
        return HAL_ERR_TIMEOUT;
    }

    return HAL_ERR_OK;
}

/* TIMER Handler */
static void PRV_LEDDRV_TIMERHandler(uint32_t un32Event, void *pContext)
{
#if defined(USE_ICON)
    if (s_un8CurIconIdx == LEDDRV_ICON_MAX)
    {
        PRV_LEDDRV_LedAllOnOff(false);
        s_un8CurIconIdx = s_un8CurIconIdx % LEDDRV_ICON_MAX;
    }
    else
    {
        EX_LEDDRV_DrawIcon((LEDDRV_ICON_e)s_un8CurIconIdx);
        s_un8CurIconIdx++;
    }
#endif
#if defined(USE_PATTERNS)
    (void)un32Event;
    (void)pContext;
    s_led_update_req = 1;
#endif
}

static HAL_ERR_e PRV_LEDDRV_Init(void)
{
    I2C_CFG_t tI2cCfg =
    {
        .eMode= I2C_MODE_MASTER,
        .un8OwnSlvAddr = 0,
        .un8OwnSlvAddr2 = 0,
        .bSaGcEnable = false,
        .bSa2GcEnable = false,
        .uPeriod.tFreq.un32Freq = 100000,
        .tSdht.bEnable = false,
    };
    HAL_ERR_e eErr;

    HAL_PCU_SetAltMode((PCU_ID_e)LED_DRV_SCL_PORT, (PCU_PIN_ID_e)LED_DRV_SCL_PIN_ID, (PCU_ALT_e)LED_DRV_SCL_MUX_ID);
    HAL_PCU_SetInOutMode((PCU_ID_e)LED_DRV_SCL_PORT, (PCU_PIN_ID_e)LED_DRV_SCL_PIN_ID, PCU_INOUT_OUTPUT_PUSH_PULL);

    HAL_PCU_SetAltMode((PCU_ID_e)LED_DRV_SDA_PORT, (PCU_PIN_ID_e)LED_DRV_SDA_PIN_ID, (PCU_ALT_e)LED_DRV_SDA_MUX_ID);
    HAL_PCU_SetInOutMode((PCU_ID_e)LED_DRV_SDA_PORT, (PCU_PIN_ID_e)LED_DRV_SDA_PIN_ID, PCU_INOUT_OUTPUT_PUSH_PULL);

    HAL_PCU_SetInOutMode((PCU_ID_e)LED_DRV_RESET_PORT, (PCU_PIN_ID_e)LED_DRV_RESET_PIN_ID, PCU_INOUT_OUTPUT_PUSH_PULL);
    HAL_PCU_SetPullUpDown((PCU_ID_e)LED_DRV_RESET_PORT, (PCU_PIN_ID_e)LED_DRV_RESET_PIN_ID, PCU_PUPD_UP);
    HAL_PCU_SetOutputBit((PCU_ID_e)LED_DRV_RESET_PORT, (PCU_PIN_ID_e)LED_DRV_RESET_PIN_ID, PCU_OUTPUT_BIT_CLEAR);
    SystemDelayMS(100);
    HAL_PCU_SetOutputBit((PCU_ID_e)LED_DRV_RESET_PORT, (PCU_PIN_ID_e)LED_DRV_RESET_PIN_ID, PCU_OUTPUT_BIT_SET);
    SystemDelayUS(20);

    eErr = HAL_I2C_Init(I2C_ID_0);
    if(eErr != HAL_ERR_OK)
    {
        return eErr;
    }

    eErr = HAL_I2C_SetConfig(I2C_ID_0, &tI2cCfg);
    if(eErr != HAL_ERR_OK)
    {
        return eErr;
    }

    eErr = HAL_I2C_SetIRQ(I2C_ID_0, I2C_OPS_INTR, PRV_LEDDRV_I2CHandler, NULL, 0);
    if(eErr != HAL_ERR_OK)
    {
        return eErr;
    }

    return HAL_ERR_OK;
}

static void PRV_LEDDRV_SetTimer(void)
{
    TIMER1_CLK_CFG_t tTimerClkCfg = {
        .eClk = TIMER1_CLK_PCLK,
        .uSubClk.ePClkDiv = TIMER1_PCLK_DIV_64,
        .un16PreScale = 499
    };

    TIMER1_CFG_t tTimerCfg = {
        .eMode = TIMER1_MODE_PERIODIC,
        .bIntrEnable = true,
#if defined(USE_ICON)        
        .utData.tGRD.un16DataA = 1000,
        .utData.tGRD.un16DataB = 1000
#endif
#if defined(USE_PATTERNS)
        .utData.tGRD.un16DataA = LEDDRV_TICK_MS,
        .utData.tGRD.un16DataB = LEDDRV_TICK_MS
#endif
    };

    HAL_TIMER1_Init(TIMER1_ID_7);
    HAL_TIMER1_SetClkConfig(TIMER1_ID_7, &tTimerClkCfg);
    HAL_TIMER1_SetConfig(TIMER1_ID_7, &tTimerCfg);
    HAL_TIMER1_SetIRQ(TIMER1_ID_7, TIMER1_OPS_INTR, PRV_LEDDRV_TIMERHandler, NULL, 3);
    HAL_TIMER1_Start(TIMER1_ID_7);
}

#if defined(USE_ICON)
static void PRV_LEDDRV_LedAllOnOff(bool bOnOff)
{
    uint8_t un8Write[4];
    uint8_t un8Seg;

    un8Seg = bOnOff ? 0xFF : 0x00;

    for (int i = 0; i < 8; i++)
    {
        un8Write[0] = LEDDRV_AL8700_CMD_ADDRSET | i;
        un8Write[1] = un8Seg;
        un8Write[2] = un8Seg & 0xFC;
        un8Write[3] = un8Write[0] ^ un8Write[1] ^ un8Write[2];

        PRV_LEDDRV_Write(un8Write, 4);
    }
}
#endif


/* PATTERN */
#if defined(USE_PATTERNS)
static void PRV_LEDDRV_WriteRow14(uint8_t y, uint16_t row14)
{
    uint8_t w[4];

    uint8_t b0 = (uint8_t)((row14 >> 6) & 0xFF);
    uint8_t b1 = (uint8_t)((row14 & 0x3F) << 2);

    w[0] = (uint8_t)(LEDDRV_AL8700_CMD_ADDRSET | (y & 0x0F));
    w[1] = b0;
    w[2] = (uint8_t)(b1 & 0xFC);
    w[3] = (uint8_t)(w[0] ^ w[1] ^ w[2]);

    (void)PRV_LEDDRV_Write(w, 4);
}

static uint16_t PRV_Row14_SetPixel(uint16_t row14, uint8_t x, uint8_t on)
{
    if (x >= LEDDRV_COLS)
        return row14;

    uint16_t mask = (uint16_t)(1u << (LEDDRV_COLS - 1u - x));
    if (on)
        row14 |= mask;
    else
        row14 &= (uint16_t)~mask;
    
    return row14;
}

static uint8_t PRV_Hash8(uint8_t x, uint8_t y, uint8_t t)
{
    uint8_t v = (uint8_t)(x * 29u + y * 97u + t * 53u);
    v ^= (uint8_t)(v >> 3);
    v ^= (uint8_t)(v << 5);
    return v;
}

/* ---- PATTERN_MARQUEE_BAR ---- */
static uint16_t PRV_PatternRow_MarqueeBar(uint8_t y, uint32_t t_ms)
{
    uint32_t tick = t_ms / LEDDRV_TICK_MS;
    uint32_t shift = tick * (uint32_t)LEDDRV_SPEED_PX_PER_TICK;
    uint16_t row = 0;
    uint8_t x;

    (void)y;

    for (x = 0; x < LEDDRV_COLS; x++)
    {
        uint8_t phase = (uint8_t)((x + (shift % 5u)) % 5u);
        uint8_t on = (phase < 3u) ? 1u : 0u;

        if (y == 0 || y == 7)
            on = 0;
        row = PRV_Row14_SetPixel(row, x, on);
    }
    return row;
}

/* ---- PATTERN_DIAGONAL ---- */
static uint16_t PRV_PatternRow_Diagonal(uint8_t y, uint32_t t_ms)
{
    uint32_t tick = t_ms / LEDDRV_TICK_MS;
    uint8_t shift = (uint8_t)(tick % (LEDDRV_COLS + LEDDRV_ROWS));
    uint16_t row = 0;
    uint8_t x;

    for (x = 0; x < LEDDRV_COLS; x++)
    {
        uint8_t on = 0;

        if ((uint8_t)(x + y) == shift) on = 1u;
        if ((uint8_t)(x + y) == (uint8_t)(shift + 1u)) on = 1u;
        if ((uint8_t)(x + y) == (uint8_t)(shift + 2u)) on = 1u;

        row = PRV_Row14_SetPixel(row, x, on);
    }
    return row;
}

/* ---- PATTERN_SINE_WAVE ---- */
static uint8_t PRV_SineY_Approx(uint8_t x, uint8_t phase)
{
    uint8_t p = (uint8_t)((x + phase) & 0x0F);

    if (p <= 2)  return 2;
    if (p <= 4)  return 3;
    if (p <= 6)  return 4;
    if (p <= 8)  return 5;
    if (p <= 10) return 4;
    if (p <= 12) return 3;
    if (p <= 14) return 2;
    return 1;
}

static uint16_t PRV_PatternRow_SineWave(uint8_t y, uint32_t t_ms)
{
    uint32_t tick = t_ms / LEDDRV_TICK_MS;
    uint8_t phase = (uint8_t)(tick & 0x0F);
    uint16_t row = 0;
    uint8_t x;

    for (x = 0; x < LEDDRV_COLS; x++)
    {
        uint8_t wy = PRV_SineY_Approx(x, phase);
        uint8_t on = (y == wy || y == (uint8_t)(wy + 1u)) ? 1u : 0u;
        row = PRV_Row14_SetPixel(row, x, on);
    }
    return row;
}

/* ---- PATTERN_RAIN ---- */
static uint16_t PRV_PatternRow_Rain(uint8_t y, uint32_t t_ms)
{
    uint32_t tick = t_ms / LEDDRV_TICK_MS;
    uint8_t fall = (uint8_t)(tick & 0x07);
    uint16_t row = 0;
    uint8_t x;

    for (x = 0; x < LEDDRV_COLS; x++)
    {
        uint8_t h = PRV_Hash8(x, 0, (uint8_t)(tick >> 2));
        uint8_t spawn = (uint8_t)(h & 0x07);
        uint8_t dropY = (uint8_t)((spawn + fall) & 0x07);
        uint8_t on = (y == dropY) ? 1u : 0u;

        if (y == (uint8_t)((dropY + 7u) & 0x07) && (h & 0x10))
            on = 1u;
        row = PRV_Row14_SetPixel(row, x, on);
    }
    return row;
}

/* ---- PATTERN_SPARKLE ---- */
static uint16_t PRV_PatternRow_Sparkle(uint8_t y, uint32_t t_ms)
{
    uint32_t tick = t_ms / LEDDRV_TICK_MS;
    uint8_t t = (uint8_t)tick;
    uint16_t row = 0;
    uint8_t x;

    for (x = 0; x < LEDDRV_COLS; x++)
    {
        uint8_t h = PRV_Hash8(x, y, t);
        uint8_t on = ((h & 0x1F) == 0u) ? 1u : 0u;

        if (((h & 0x3F) == 1u) && (x + 1u < LEDDRV_COLS))
            row = PRV_Row14_SetPixel(row, (uint8_t)(x + 1u), 1u);

        row = PRV_Row14_SetPixel(row, x, on);
    }
    return row;
}

/* ---- PATTERN_SCANNER ---- */
static uint16_t PRV_PatternRow_Scanner(uint8_t y)
{
    uint16_t row = 0;

    if (y < 2 || y > 5)
        return 0;

    row = PRV_Row14_SetPixel(row, s_scan_x, 1u);

    if (s_scan_x >= 1u)
        row = PRV_Row14_SetPixel(row, (uint8_t)(s_scan_x - 1u), 1u);
    if (s_scan_x + 1u < LEDDRV_COLS)
        row = PRV_Row14_SetPixel(row, (uint8_t)(s_scan_x + 1u), 1u);
    if (s_scan_x >= 2u)
        row = PRV_Row14_SetPixel(row, (uint8_t)(s_scan_x - 2u), 1u);
    if (s_scan_x + 2u < LEDDRV_COLS)
        row = PRV_Row14_SetPixel(row, (uint8_t)(s_scan_x + 2u), 1u);

    return row;
}
/* ---- PATTERN_CENTER_OUT ---- */
static uint16_t PRV_PatternRow_CenterOut(uint8_t y, uint32_t t_ms)
{
    uint32_t tick = t_ms / LEDDRV_TICK_MS;
    uint8_t phase = (uint8_t)(tick % 8u);   /* 0..7 */

    uint16_t row = 0;

    uint8_t thickness = (y >= 2u && y <= 5u) ? 1u : 0u;
    int8_t left  = 6 - (int8_t)phase;
    int8_t right = 7 + (int8_t)phase;

    for (uint8_t x = 0; x < LEDDRV_COLS; x++)
    {
        uint8_t on = 0;

        if ((int8_t)x == left || (int8_t)x == right)
            on = 1u;

        if ((int8_t)x == (left + 1) || (int8_t)x == (right - 1))
            on = 1u;

        if (thickness)
        {
            if ((int8_t)x == (left - 1) || (int8_t)x == (right + 1))
                on = 1u;
        }

        row = PRV_Row14_SetPixel(row, x, on);
    }

    return row;
}

/* ---- PATTERN_CENTER_OUT_FILL ---- */
static uint16_t PRV_PatternRow_CenterOut_FILL(uint8_t y, uint32_t t_ms)
{
    uint32_t tick = t_ms / LEDDRV_TICK_MS;
    uint8_t phase = (uint8_t)(tick % 12u);

    if (phase > 6u)
        phase = 6u;

    int8_t left  = 6 - (int8_t)phase;
    int8_t right = 7 + (int8_t)phase;

    uint16_t row = 0;

    for (uint8_t x = 0; x < LEDDRV_COLS; x++)
    {
        uint8_t on = ((int8_t)x >= left && (int8_t)x <= right) ? 1u : 0u;
        row = PRV_Row14_SetPixel(row, x, on);
    }

    if ((y == 0 || y == 7) && phase == 0u)
        row = 0;

    return row;
}

/* ---- PATTERN_WIPE_LR ---- */
static uint16_t PRV_PatternRow_WipeLR(uint8_t y, uint32_t t_ms)
{
    uint32_t tick = t_ms / LEDDRV_TICK_MS;
    uint8_t phase = (uint8_t)(tick % (LEDDRV_COLS + 4u));

    uint16_t row = 0;
    uint8_t x;

    (void)y;

    for (x = 0; x < LEDDRV_COLS; x++)
    {
        uint8_t on = (x <= phase) ? 1u : 0u;
        row = PRV_Row14_SetPixel(row, x, on);
    }

    return row;
}

/* ---- PATTERN_WIPE_OUTSIDE_IN ---- */
static uint16_t PRV_PatternRow_WipeOutsideIn(uint8_t y, uint32_t t_ms)
{
    uint32_t tick = t_ms / LEDDRV_TICK_MS;
    uint8_t phase = (uint8_t)(tick % 8u);

    int8_t left_end  = (int8_t)phase;
    int8_t right_end = (int8_t)(LEDDRV_COLS - 1u - phase);

    uint16_t row = 0;
    uint8_t x;

    (void)y;

    for (x = 0; x < LEDDRV_COLS; x++)
    {
        uint8_t on = 0;

        if ((int8_t)x <= left_end)
            on = 1u;
        if ((int8_t)x >= right_end)
            on = 1u;

        row = PRV_Row14_SetPixel(row, x, on);
    }

    return row;
}

/* ---- PATTERN_FIRE ---- */
static uint16_t PRV_PatternRow_Fire(uint8_t y, uint32_t t_ms)
{
    uint32_t tick = t_ms / LEDDRV_TICK_MS;
    uint8_t t = (uint8_t)tick;
    uint16_t row = 0;
    uint8_t x;

    uint8_t base_th;
    if (y >= 6u)      base_th = 180u;
    else if (y >= 4u) base_th = 120u;
    else if (y >= 2u) base_th = 70u;
    else              base_th = 35u;

    for (x = 0; x < LEDDRV_COLS; x++)
    {
        uint8_t h1 = PRV_Hash8(x, y, t);
        uint8_t h2 = PRV_Hash8((uint8_t)(x + 3u), (uint8_t)(7u - y), (uint8_t)(t << 1));
        uint16_t energy = (uint16_t)h1 + (uint16_t)(h2 >> 1);
        
        if (x >= 4u && x <= 9u)
            energy = (uint16_t)(energy + 25u);

        if (energy > base_th)
            row = PRV_Row14_SetPixel(row, x, 1u);
    }

    return row;
}

/* ---- PATTERN_RIPPLE ---- */
static uint16_t PRV_PatternRow_Ripple(uint8_t y, uint32_t t_ms)
{
    uint32_t tick = t_ms / LEDDRV_TICK_MS;
    uint8_t phase = (uint8_t)(tick % 12u);

    uint16_t row = 0;
    uint8_t x;

    for (x = 0; x < LEDDRV_COLS; x++)
    {
        int8_t dx = (int8_t)x - 6;
        int8_t dy = (int8_t)y - 3;

        uint8_t dist;
        if (dx < 0) dx = (int8_t)(-dx);
        if (dy < 0) dy = (int8_t)(-dy);
        
        dist = (uint8_t)(dx + dy);

        if (dist == phase || dist == (uint8_t)(phase + 1u) || dist == (uint8_t)(phase + 6u))
            row = PRV_Row14_SetPixel(row, x, 1u);
    }

    return row;
}

/* ---- PATTERN_RADAR ---- */
static uint16_t PRV_PatternRow_Radar(uint8_t y, uint32_t t_ms)
{
    uint32_t tick = t_ms / LEDDRV_TICK_MS;
    uint8_t beam = (uint8_t)(tick % LEDDRV_COLS);

    uint16_t row = 0;
    uint8_t x;

    for (x = 0; x < LEDDRV_COLS; x++)
    {
        uint8_t on = 0;

        if (x == beam)
            on = 1u;
        else if (x + 1u == beam || x == beam + 1u)
            on = (y >= 1u && y <= 6u) ? 1u : 0u;
        else if (x + 2u == beam || x == beam + 2u)
            on = (y >= 2u && y <= 5u) ? 1u : 0u;

        if (!on)
        {
            uint8_t h = PRV_Hash8(x, y, (uint8_t)(tick >> 1));
            if ((h & 0x3F) == 0u)
                on = 1u;
        }

        row = PRV_Row14_SetPixel(row, x, on);
    }

    return row;
}

static uint16_t PRV_RenderRow(uint8_t y, uint32_t t_ms)
{
    switch (s_pattern)
    {
        case PATTERN_MARQUEE_BAR:
            return PRV_PatternRow_MarqueeBar(y, t_ms);

        case PATTERN_DIAGONAL:
            return PRV_PatternRow_Diagonal(y, t_ms);

        case PATTERN_SINE_WAVE:
            return PRV_PatternRow_SineWave(y, t_ms);

        case PATTERN_RAIN:
            return PRV_PatternRow_Rain(y, t_ms);

        case PATTERN_SPARKLE:
            return PRV_PatternRow_Sparkle(y, t_ms);

        case PATTERN_SCANNER:
            return PRV_PatternRow_Scanner(y);

        case PATTERN_CENTER_OUT:
            return PRV_PatternRow_CenterOut(y, t_ms);

        case PATTERN_CENTER_OUT_FILL:
            return PRV_PatternRow_CenterOut_FILL(y, t_ms);
        
        case PATTERN_WIPE_LR:
            return PRV_PatternRow_WipeLR(y, t_ms);

        case PATTERN_WIPE_OUTSIDE_IN:
            return PRV_PatternRow_WipeOutsideIn(y, t_ms);

        case PATTERN_FIRE:
            return PRV_PatternRow_Fire(y, t_ms);

        case PATTERN_RIPPLE:
            return PRV_PatternRow_Ripple(y, t_ms);

        case PATTERN_RADAR:
            return PRV_PatternRow_Radar(y, t_ms);
            
        default:
            return 0;
    }
}

static void PRV_PatternAdvance(void)
{
    s_tick_count++;

    if (s_pattern == PATTERN_SCANNER)
    {
        int16_t nx = (int16_t)s_scan_x + (int16_t)s_scan_dir;

        if (nx <= 0)
        {
            nx = 0;
            s_scan_dir = 1;
        }
        else if (nx >= (LEDDRV_COLS - 1))
        {
            nx = (LEDDRV_COLS - 1);
            s_scan_dir = -1;
        }

        s_scan_x = (uint8_t)nx;
    }

    s_hold_tick++;
    if (s_hold_tick >= LEDDRV_PATTERN_HOLD_TICKS)
    {
        s_hold_tick = 0;
        s_pattern = (PATTERN_e)(((uint8_t)s_pattern + 1u) % (uint8_t)PATTERN_MAX);

        if (s_pattern == PATTERN_SCANNER)
        {
            s_scan_x = 0;
            s_scan_dir = 1;
        }
    }
}

static void PRV_RenderAndPush(void)
{
    uint32_t t = (uint32_t)s_tick_count * (uint32_t)LEDDRV_TICK_MS;
    uint8_t y;

    for (y = 0; y < LEDDRV_ROWS; y++)
    {
        uint16_t row14 = PRV_RenderRow(y, t);
        PRV_LEDDRV_WriteRow14(y, row14);
    }
}
#endif

void EX_LEDDRV_Init(void)
{
    HAL_ERR_e eErr;

    eErr = PRV_LEDDRV_Init();
    if(eErr == HAL_ERR_OK)
    {
#if defined(USE_ICON)
        EX_LEDDRV_SetDisplayMode(LEDDRV_MODE_8_14);
        EX_LEDDRV_SetOnOff(true);
        EX_LEDDRV_SetDimming(LEDDRV_DIMMING_7);

        PRV_LEDDRV_LedAllOnOff(true);

        PRV_LEDDRV_SetTimer();
#endif

#if defined(USE_PATTERNS)
        EX_LEDDRV_SetDisplayMode(LEDDRV_MODE_8_14);
        EX_LEDDRV_SetOnOff(true);
        EX_LEDDRV_SetDimming(LEDDRV_DIMMING_0);

        s_tick_count = 0;
        s_hold_tick = 0;
        s_pattern = PATTERN_MARQUEE_BAR;
        s_scan_x = 0;
        s_scan_dir = 1;

        PRV_LEDDRV_SetTimer();
#endif
    }
    else
    {
        LOG("PRV_LEDDRV_Init() error (%d)\n", eErr);
    }
}

void EX_LEDDRV_SetOnOff(bool bOnOff)
{
    uint8_t un8Write[4];

    un8Write[0] = LEDDRV_AL8700_CMD_DON | ((uint8_t)bOnOff);
    un8Write[1] = un8Write[0];
    PRV_LEDDRV_Write(un8Write, 2);
}

void EX_LEDDRV_SetDimming(LEDDRV_DIMMING_e eDimming)
{
    uint8_t un8Write[4];

    if (eDimming == LEDDRV_DIMMING_MAX)
    {
        return;
    }

    un8Write[0] = LEDDRV_AL8700_CMD_DIMMODE | (uint8_t)eDimming;
    un8Write[1] = un8Write[0];
    PRV_LEDDRV_Write(un8Write, 2);
}

void EX_LEDDRV_SetDisplayMode(LEDDRV_MODE_e eMode)
{
    uint8_t un8Write[4];

    if (eMode == LEDDRV_MODE_MAX)
    {
        return;
    }

    un8Write[0] = LEDDRV_AL8700_CMD_DISPMODE | (uint8_t)eMode;
    un8Write[1] = un8Write[0];
    PRV_LEDDRV_Write(un8Write, 2);
}

#if defined(USE_ICON)
void EX_LEDDRV_DrawIcon(LEDDRV_ICON_e eIcon)
{
    const uint8_t (*ppun8Icon)[2];
    uint8_t un8Write[4];

    if (eIcon == LEDDRV_ICON_SUN)
    {
        ppun8Icon = s_un8Icon_Sun;
    }
    else if (eIcon == LEDDRV_ICON_CLOUD)
    {
        ppun8Icon = s_un8Icon_Cloud;
    }
    else if (eIcon == LEDDRV_ICON_UMBRELLA)
    {
        ppun8Icon = s_un8Icon_Umbrella;
    }
    else if (eIcon == LEDDRV_ICON_SNOW)
    {
        ppun8Icon = s_un8Icon_Snow;
    }
    else if (eIcon == LEDDRV_ICON_LIGHTNING)
    {
        ppun8Icon = s_un8Icon_Lightning;
    }
    else
    {
        return;
    }

    for (int i = 0; i < LEDDRV_AL8700_D_LINE_MAX; i++)
    {
        un8Write[0] = LEDDRV_AL8700_CMD_ADDRSET | i;
        un8Write[1] = ppun8Icon[i][0];
        un8Write[2] = ppun8Icon[i][1] & 0xFC;
        un8Write[3] = un8Write[0] ^ un8Write[1] ^ un8Write[2];

        PRV_LEDDRV_Write(un8Write, 4);
    }
}
#endif

#if defined(USE_PATTERNS)
void EX_LEDDRV_Manager(void)
{
    if (!s_led_update_req)
        return;

    s_led_update_req = 0;

    PRV_PatternAdvance();
    PRV_RenderAndPush();
}
#else
void EX_LEDDRV_Manager(void)
{
    return;
}
#endif
