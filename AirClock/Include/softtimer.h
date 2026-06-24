/**
 *******************************************************************************
 * @file        softtimer.h
 * @author      ABOV R&D Division
 * @brief       EXAMPLE Software Timer Implementation
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

#include "hal_common.h"

#define SOFTTIMER_MAX  12

typedef void (*softtimer_cb_t)(void* user);
void EX_SoftTimer_Init(void);
void EX_SoftTimer_TickIsr(void);
void EX_SoftTimer_Manager(void);
bool EX_SoftTimer_StartOneshot(uint8_t id, uint32_t delay_ms, softtimer_cb_t cb, void* user);
void EX_SoftTimer_Cancel(uint8_t id);


#ifdef __cplusplus
}
#endif

