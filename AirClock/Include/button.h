/**
 *******************************************************************************
 * @file        button_ts1102s.h
 * @author      ABOV R&D Division
 * @brief       EXAMPLE TS1102S Button Driver Interface
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
#include "virtual_key.h"

typedef enum { 
	BUTTON_EVT_NONE = 0, 
	BUTTON_EVT_PUSH = 1, 
	BUTTON_EVT_PULL = 2, 
	BUTTON_EVT_BOTH = 3 
} pin_evt_t;

typedef enum { 
	BTN_EVT_PRESSED, 
	BTN_EVT_RELEASED, 
	BTN_EVT_REPEAT 
} btn_evt_t;

typedef struct {
	uint8_t pin;
	btn_evt_t evt;
} btn_msg_t;

bool EX_BUTTON_Init(pfnKeyCallback_t handler);
void EX_BUTTON_Manager(void);

// todo, FreeRTOS
void EX_BUTTON_ProcessEvent(const btn_msg_t *msg);

#ifdef __cplusplus
}
#endif
