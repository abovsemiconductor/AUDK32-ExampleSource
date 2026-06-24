/**
 *******************************************************************************
 * @file        button.c
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
#include "abov_config.h"

#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"

#include "hal_common.h"
#include "hal_pcu.h"

#include "button.h"
#include "softtimer.h"
#include "buzzer_bmx2705.h"
#include "virtual_key.h"
#include "gpio_shared_irq.h"
#include "app_event.h"

#include "FreeRTOS.h"

#define PCU_IRQ_PRIO   configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY
#define PCU_INTR_NUM   12

#define BUTTON_REPEAT_CHECK_TIME_MSEC 30

typedef struct {
    uint8_t down; 
    uint8_t tmr_on; 
} btn_state_t;

static btn_state_t s_btn[PCU_PIN_ID_MAX];					// user switch

static PCU_Context_t s_tPCUContext[CONFIG_PCU_MAX_COUNT];
static pfnKeyCallback_t s_pfnKey = NULL;

static void PRV_BUTTON_IRQHandler(uint32_t un32Event, void *pContext)
{
    // send for user switch 7
    int pin = PCU_PIN_ID_7;
    uint32_t v = (un32Event >> (pin * 2)) & 0x3;
    if (v != 0 && (v & 0x1)) 
    { 
        btn_msg_t m = { pin, BTN_EVT_PRESSED };
        (void)AppEvent_SendButton(&m);
    }
    if (v != 0 && (v & 0x2)) 
    { 
        btn_msg_t m = { pin, BTN_EVT_RELEASED };
        (void)AppEvent_SendButton(&m);
    }

    // send for user switch 8
    pin = PCU_PIN_ID_8;
    v = (un32Event >> (pin * 2)) & 0x3;
    if (v != 0 && (v & 0x1)) 
    { 
        btn_msg_t m = { pin, BTN_EVT_PRESSED };
        (void)AppEvent_SendButton(&m);
    }
    if (v != 0 && (v & 0x2))
    { 
        btn_msg_t m = { pin, BTN_EVT_RELEASED };
        (void)AppEvent_SendButton(&m);
    }
}

static void PRV_Timer_OneshotCallback(void* user)
{
    uint8_t pin = (uint8_t)(uint32_t)user;

    // still pressed?
    PCU_PORT_e v;
    HAL_PCU_GetInputValue(PCU_ID_C, pin, &v);
    bool pressed = (v == PCU_PORT_LOW) ? true : false;
    if (!pressed) 
    {
        return;
    }

    // then send BTN_EVT_REPEAT
    btn_msg_t m = { pin, BTN_EVT_REPEAT };
    (void)AppEvent_SendButton(&m);

    EX_SoftTimer_StartOneshot(pin, BUTTON_REPEAT_CHECK_TIME_MSEC, PRV_Timer_OneshotCallback, (void*)(uint32_t)pin);
}

bool EX_BUTTON_Init(pfnKeyCallback_t handler)
{
    // 0. init
    memset(s_btn, 0x00, sizeof(btn_state_t) * PCU_PIN_ID_MAX);

    if (handler != NULL)
    {
        s_pfnKey = handler;
    }

    // user switch 2
    PCU_ID_e portNameS2             = PCU_ID_C;
    PCU_PIN_ID_e portNumberS2       = PCU_PIN_ID_7;
    PCU_INOUT_e portInOutModeS2     = PCU_INOUT_INPUT;
    PCU_INTR_MODE_e intrModeS2      = PCU_INTR_MODE_EDGE;
    PCU_INTR_TRG_e intrTriggerS2    = PCU_INTR_TRG_BOTH_LEVEL_EDGE;

    // user switch 3
    PCU_ID_e portNameS3             = portNameS2;
    PCU_PIN_ID_e portNumberS3       = PCU_PIN_ID_8;
    PCU_INOUT_e portInOutModeS3     = PCU_INOUT_INPUT;
    PCU_INTR_MODE_e intrModeS3      = PCU_INTR_MODE_EDGE;
    PCU_INTR_TRG_e intrTriggerS3    = PCU_INTR_TRG_BOTH_LEVEL_EDGE;

    // 1. set port
    if (HAL_PCU_SetIntrPort(portNameS2, portNumberS2, intrModeS2, intrTriggerS2, PCU_INTR_NUM) != HAL_ERR_OK)
    {
        LOG("HAL_PCU_SetIntrPort() fail.\n");
        return false;
    }
    if (HAL_PCU_SetIntrPort(portNameS3, portNumberS3, intrModeS3, intrTriggerS3, PCU_INTR_NUM) != HAL_ERR_OK)
    {
        LOG("HAL_PCU_SetIntrPort() fail.\n");
        return false;
    }

    // 2. set IRQ
    PCU_IRQ_CFG_t tIRQCfg;
    memset(&tIRQCfg, 0x00, sizeof(PCU_IRQ_CFG_t));
    memset(&s_tPCUContext[0], 0x00, sizeof(PCU_Context_t) * CONFIG_PCU_MAX_COUNT);

    s_tPCUContext[portNameS2].eId = portNameS2;
    tIRQCfg.eId = portNameS2;
    tIRQCfg.eOps = PCU_OPS_INTR;
    tIRQCfg.pfnHandler = &PRV_BUTTON_IRQHandler;
    tIRQCfg.pContext = &s_tPCUContext[portNameS2];
    tIRQCfg.un32IRQPrio = PCU_IRQ_PRIO;
    tIRQCfg.un8IntNum = PCU_INTR_NUM;

    uint32_t PINS = ((1u << PCU_PIN_ID_7) | (1u << PCU_PIN_ID_8));

    // not use HAL_PCU_SetIRQ(&tIRQCfg)
    if (!EX_SharedIRQ_Register(&tIRQCfg, PINS, SHARED_GPIO_LOW_QUEUE))
    {
        LOG("EX_SharedIRQ_Register(7) fail.\n");
        return false;
    }

    return true;
}

void EX_BUTTON_Manager(void)
{
    return;
}

void EX_BUTTON_ProcessEvent(const btn_msg_t *msg)
{
    if (msg == NULL)
    {
        return;
    }

    {
        uint8_t pin = msg->pin;
        if (pin >= PCU_PIN_ID_MAX)
        {
            return;
        }

        switch (msg->evt) 
        {
            case BTN_EVT_PRESSED:
                if (!s_btn[pin].down) 
                {
                    s_btn[pin].down = 1;
                    LOG("PRESSED pin%d\n", pin);

                    if (!s_btn[pin].tmr_on)
                    {
                        s_btn[pin].tmr_on = 1;
                        EX_SoftTimer_StartOneshot(pin, BUTTON_REPEAT_CHECK_TIME_MSEC, PRV_Timer_OneshotCallback, (void*)(uint32_t)pin);
                    }
                }
                break;

            case BTN_EVT_RELEASED:
                if (s_btn[pin].down)
                    LOG("RELEASED pin%d\n", pin);

                // send key
                if (s_pfnKey != NULL)
                {
                    if (pin == 7)
                        s_pfnKey(KEY_DOWN);
                        //s_pfnKey(KEY_OK);
                    else if (pin == 8)
                        s_pfnKey(KEY_OK);
                        //s_pfnKey(KEY_DOWN);
                }
                s_btn[pin].down = 0;
                s_btn[pin].tmr_on = 0;
                EX_SoftTimer_Cancel(pin);
                break;

            case BTN_EVT_REPEAT:
                LOG("REPEAT pin%d\n", pin);
                // send key
                #if 0
                if (s_pfnKey != NULL)
                {
                    s_pfnKey(KEY_OK);
                }
                #endif
                break;
        }
    }
}
