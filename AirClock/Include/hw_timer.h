/**
 *******************************************************************************
 * @file        hw_timer.h
 * @author      ABOV R&D Division
 * @brief       hardware timer
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

#include "hal_timer1.h"

typedef void (*pfnTimer6_EVT_Handler_t)(uint32_t un32Event, void *pContext);

void EX_HW_TIMER_6_Start(pfnTimer6_EVT_Handler_t cb, TIMER1_MODE_e mode, uint32_t mSec);
void EX_HW_TIMER_6_Stop(void);

#ifdef __cplusplus
}
#endif

