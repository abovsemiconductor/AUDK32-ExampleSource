/**
 *******************************************************************************
 * @file        app_event.h
 * @author      ABOV R&D Division
 * @brief       AirClock application event queue
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
#include "hal_pcu.h"
#include "button.h"
#include "ir_receiver.h"

typedef enum {
    APP_EVT_GPIO_SHARED = 0,
    APP_EVT_BUTTON,
    APP_EVT_IR,
} APP_EVENT_ID_e;

typedef struct {
    pfnPCU_IRQ_Handler_t cb;
    uint32_t event;
    void *context;
} APP_EVENT_GPIO_t;

typedef struct {
    APP_EVENT_ID_e id;
    union {
        APP_EVENT_GPIO_t gpio;
        btn_msg_t button;
        irr_msg_t ir;
    } data;
} APP_EVENT_t;

bool AppEvent_Init(void);
bool AppEvent_SendGPIOFromISR(pfnPCU_IRQ_Handler_t cb, uint32_t event, void *context);
bool AppEvent_SendButton(const btn_msg_t *msg);
bool AppEvent_SendIRFromISR(const irr_msg_t *msg);
void AppEvent_Manager(void);

#ifdef __cplusplus
}
#endif
