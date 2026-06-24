/**
 *******************************************************************************
 * @file        leddrv_al8700.h
 * @author      ABOV R&D Division
 * @brief       LED Driver Interface
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

typedef enum
{
    LEDDRV_MODE_4_14,
    LEDDRV_MODE_5_14,
    LEDDRV_MODE_6_14,
    LEDDRV_MODE_7_14,
    LEDDRV_MODE_8_14,
    LEDDRV_MODE_MAX,
}LEDDRV_MODE_e;

typedef enum
{
    LEDDRV_DIMMING_0,
    LEDDRV_DIMMING_1,
    LEDDRV_DIMMING_2,
    LEDDRV_DIMMING_3,
    LEDDRV_DIMMING_4,
    LEDDRV_DIMMING_5,
    LEDDRV_DIMMING_6,
    LEDDRV_DIMMING_7,
    LEDDRV_DIMMING_MAX,
}LEDDRV_DIMMING_e;

typedef enum
{
    LEDDRV_ICON_SUN,
    LEDDRV_ICON_CLOUD,
    LEDDRV_ICON_UMBRELLA,
    LEDDRV_ICON_SNOW,
    LEDDRV_ICON_LIGHTNING,
    LEDDRV_ICON_MAX,
}LEDDRV_ICON_e;

void EX_LEDDRV_Init(void);
void EX_LEDDRV_SetDisplayMode(LEDDRV_MODE_e eMode);
void EX_LEDDRV_SetOnOff(bool bOnOff);
void EX_LEDDRV_SetDimming(LEDDRV_DIMMING_e eDimming);
void EX_LEDDRV_DrawIcon(LEDDRV_ICON_e eIcon);
void EX_LEDDRV_Manager(void);

#ifdef __cplusplus
}
#endif

