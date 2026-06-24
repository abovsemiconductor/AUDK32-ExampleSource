/**
 *******************************************************************************
 * @file        app_event.c
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

#include "abov_config.h"
#include "debug_log.h"

#include "FreeRTOS.h"
#include "queue.h"

#include "app_event.h"

#define APP_EVENT_QUEUE_CAPACITY 16

static QueueHandle_t s_appEventQ;
static StaticQueue_t s_appEventQCtrl;
static uint8_t s_appEventQStorage[APP_EVENT_QUEUE_CAPACITY * sizeof(APP_EVENT_t)];

bool AppEvent_Init(void)
{
    s_appEventQ = xQueueCreateStatic(APP_EVENT_QUEUE_CAPACITY,
                                     sizeof(APP_EVENT_t),
                                     s_appEventQStorage,
                                     &s_appEventQCtrl);
    if (s_appEventQ == NULL)
    {
        LOG("xQueueCreateStatic(app_event) fail.\n");
        return false;
    }

    return true;
}

bool AppEvent_SendGPIOFromISR(pfnPCU_IRQ_Handler_t cb, uint32_t event, void *context)
{
    APP_EVENT_t appEvent = {
        .id = APP_EVT_GPIO_SHARED,
        .data.gpio = { cb, event, context }
    };

    if (s_appEventQ == NULL)
    {
        return false;
    }

    return (xQueueSendFromISR(s_appEventQ, &appEvent, NULL) == pdTRUE);
}

bool AppEvent_SendButton(const btn_msg_t *msg)
{
    APP_EVENT_t appEvent = {
        .id = APP_EVT_BUTTON,
    };

    if (msg == NULL)
    {
        return false;
    }

    if (s_appEventQ == NULL)
    {
        return false;
    }

    appEvent.data.button = *msg;
    return (xQueueSend(s_appEventQ, &appEvent, 0) == pdTRUE);
}

bool AppEvent_SendIRFromISR(const irr_msg_t *msg)
{
    APP_EVENT_t appEvent = {
        .id = APP_EVT_IR,
    };

    if (msg == NULL)
    {
        return false;
    }

    if (s_appEventQ == NULL)
    {
        return false;
    }

    appEvent.data.ir = *msg;
    return (xQueueSendFromISR(s_appEventQ, &appEvent, NULL) == pdTRUE);
}

void AppEvent_Manager(void)
{
    APP_EVENT_t event;

    if (s_appEventQ == NULL)
    {
        return;
    }

    while (xQueueReceive(s_appEventQ, &event, 0) == pdTRUE)
    {
        switch (event.id)
        {
            case APP_EVT_GPIO_SHARED:
                if (event.data.gpio.cb != NULL)
                {
                    event.data.gpio.cb(event.data.gpio.event, event.data.gpio.context);
                }
                break;

            case APP_EVT_BUTTON:
                EX_BUTTON_ProcessEvent(&event.data.button);
                break;

            case APP_EVT_IR:
                EX_IR_ProcessEvent(&event.data.ir);
                break;

            default:
                break;
        }
    }
}
