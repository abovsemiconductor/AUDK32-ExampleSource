/**
 *******************************************************************************
 * @file        cmsisos_main.c
 * @author      ABOV R&D Division
 * @brief       Main Example
 *
 * Copyright 2022 ABOV Semiconductor Co.,Ltd. All rights reserved.
 *
 * This file is licensed under terms that are found in the LICENSE file
 * located at Document directory.
 * If this file is delivered or shared without applicable license terms,
 * the terms of the BSD-3-Clause license shall be applied.
 * Reference: https://opensource.org/licenses/BSD-3-Clause
 ******************************************************************************/

#include "abov_config.h"
#include "freertos_example.h"

#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"

#include "hal_pcu.h"

#if defined(FREERTOS_EXAMPE_API_CMSIS_RTOS_V2)
#include "cmsis_os2.h"

#define LED_BLIKING_TASK_STASK 1024

typedef struct
{
    bool bLedOnOff;
    bool bIsLedControl;
} EX_LED_Context_t;

typedef struct
{
    uint32_t un32Event;
    void *pContext;
} USER_SW_IRQ_t;

static osMessageQueueId_t s_MsgQId;
static osSemaphoreId_t s_SemId;
static EX_LED_Context_t s_tLedContext;

#if defined(FREERTOS_EXAMPE_LED_BLIKING_TASK_ONLY)
static void xLedBlinkingTask(void *pParam)
{
    uint32_t un32LedID;
    bool bLedOnOff = false;

    LOG("LED_BLIKING_TASK_ONLY\n");

    while(1)
    {
        bLedOnOff = !bLedOnOff;

        for (un32LedID = FIRST_LED_ID; un32LedID <= LAST_LED_ID; un32LedID++)
        {
            HAL_PCU_SetOutputBit((PCU_ID_e)LED_PORT_ID, (PCU_PIN_ID_e)un32LedID, (PCU_OUTPUT_BIT_e)bLedOnOff);
        }

        osDelay(500);
    }
}
#elif defined(FREERTOS_EXAMPE_LED_BLIKING_TASK_MESSAGE)
static void xMsgSendTask(void *pParam)
{
    bool bLedOnOff = false;

    LOG("LED_BLIKING_TASK_MESSAGE - SENDER\n");

    while(1)
    {
        bLedOnOff = !bLedOnOff;

        osMessageQueuePut(s_MsgQId, &bLedOnOff, 0, 0);

        osDelay(500);
    }
}

static void xMsgRecvTask(void *pParam)
{
    uint32_t un32LedID;
    bool bLedOnOff;

    LOG("LED_BLIKING_TASK_MESSAGE - RECEIVER\n");

    while(1)
    {
        if(osMessageQueueGet(s_MsgQId, &bLedOnOff, NULL, osWaitForever) == osOK)
        {
            LOG("Message Received.\n");

            for (un32LedID = FIRST_LED_ID; un32LedID <= LAST_LED_ID; un32LedID++)
            {
                HAL_PCU_SetOutputBit((PCU_ID_e)LED_PORT_ID, (PCU_PIN_ID_e)un32LedID, (PCU_OUTPUT_BIT_e)bLedOnOff);
            }
        }
    }
}
#elif defined(FREERTOS_EXAMPE_LED_BLIKING_TASK_SEMAPHORE)
static void xSemaTask1(void *pParam)
{
    LOG("LED_BLIKING_TASK_SEMAPHORE - FLAG TOGGLE\n");

    while(1)
    {
        if(osSemaphoreAcquire(s_SemId, osWaitForever) == osOK)
        {
            if(s_tLedContext.bIsLedControl)
            {
                s_tLedContext.bLedOnOff = !s_tLedContext.bLedOnOff;
                s_tLedContext.bIsLedControl = false;
                LOG("CONTROL Update : %s\n", s_tLedContext.bLedOnOff ? "On" : "Off");
            }

            osSemaphoreRelease(s_SemId);

            osDelay(50);
        }
    }
}

static void xSemaTask2(void *pParam)
{
    uint32_t un32LedID;

    LOG("LED_BLIKING_TASK_SEMAPHORE - CONTROL\n");

    while(1)
    {
        if(osSemaphoreAcquire(s_SemId, osWaitForever) == osOK)
        {
            LOG("CONTROL Get : %s\n", s_tLedContext.bLedOnOff ? "On" : "Off");
            for (un32LedID = FIRST_LED_ID; un32LedID <= LAST_LED_ID; un32LedID++)
            {
                HAL_PCU_SetOutputBit((PCU_ID_e)LED_PORT_ID, (PCU_PIN_ID_e)un32LedID, (PCU_OUTPUT_BIT_e)s_tLedContext.bLedOnOff);
            }
            s_tLedContext.bIsLedControl = true;

            osSemaphoreRelease(s_SemId);
            LOG("CONTROL Release\n");

            osDelay(500);
        }
    }
}
#else
static void USER_SW_IRQHandler(uint32_t un32Event, void *pContext)
{
    USER_SW_IRQ_t tIRQ = {
        .un32Event = un32Event,
        .pContext = pContext
    };

    osMessageQueuePut(s_MsgQId, &tIRQ, 0, 0);
}

static void xLedOnOffTask(void *pParam)
{
    USER_SW_IRQ_t tIRQ;
    uint32_t un32LedID;
    bool bLedOnOff = false;

    LOG("LED ON/OFF by User SW\n");

    while(1)
    {
        if(osMessageQueueGet(s_MsgQId, &tIRQ, NULL, osWaitForever) == osOK)
        {
            LOG("Message Received.\n");

            if( (tIRQ.un32Event >> (USER_SW1_PORT_ID * 2)) & 0x3)
            {
                bLedOnOff = !bLedOnOff;
                for (un32LedID = FIRST_LED_ID; un32LedID <= LAST_LED_ID; un32LedID++)
                {
                    HAL_PCU_SetOutputBit((PCU_ID_e)LED_PORT_ID, (PCU_PIN_ID_e)un32LedID, (PCU_OUTPUT_BIT_e)bLedOnOff);
                }
            }
        }
    }
}
#endif

/**********************************************************************
 * @brief		Main program
 * @param[in]	None
 * @return	None
 **********************************************************************/
void cmsis_rtos_v2(void)
{
    osKernelInitialize();

#if defined(FREERTOS_EXAMPE_LED_BLIKING_TASK_ONLY)
    osThreadAttr_t osThreadAttr = {
        .name = "LedBlinkingTask",
        .stack_size = LED_BLIKING_TASK_STASK,
        .priority = osPriorityNormal
    };

    osThreadNew(xLedBlinkingTask, NULL, &osThreadAttr);
#elif defined(FREERTOS_EXAMPE_LED_BLIKING_TASK_MESSAGE)
    osThreadAttr_t osThread1Attr = {
        .name = "MsgSendTask",
        .stack_size = LED_BLIKING_TASK_STASK,
        .priority = osPriorityNormal
    };

    osThreadAttr_t osThread2Attr = {
        .name = "MsgRecvTask",
        .stack_size = LED_BLIKING_TASK_STASK,
        .priority = osPriorityNormal
    };

    s_MsgQId = osMessageQueueNew(5, sizeof(bool), NULL);
    osThreadNew(xMsgSendTask, NULL, &osThread1Attr);
    osThreadNew(xMsgRecvTask, NULL, &osThread2Attr);
#elif defined(FREERTOS_EXAMPE_LED_BLIKING_TASK_SEMAPHORE)
    osThreadAttr_t osThread1Attr = {
        .name = "SemaTask1",
        .stack_size = LED_BLIKING_TASK_STASK,
        .priority = osPriorityNormal
    };

    osThreadAttr_t osThread2Attr = {
        .name = "SemaTask2",
        .stack_size = LED_BLIKING_TASK_STASK,
        .priority = osPriorityNormal
    };

    s_SemId = osSemaphoreNew(1, 1, NULL);
    osThreadNew(xSemaTask1, NULL, &osThread1Attr);
    osThreadNew(xSemaTask2, NULL, &osThread2Attr);
#else
    PCU_IRQ_CFG_t tPcuCfg = {
        .eId = (PCU_ID_e)USER_SW1_PORT,
        .eOps = PCU_OPS_INTR,
        .pfnHandler = &USER_SW_IRQHandler,
        .pContext = NULL,
        .un32IRQPrio = 5,
        .un8IntNum = USER_SW1_IRQ_NUM
    };

    osThreadAttr_t osThreadAttr = {
        .name = "LedOnOffTask",
        .stack_size = LED_BLIKING_TASK_STASK,
        .priority = osPriorityNormal
    };

    HAL_PCU_SetAltMode((PCU_ID_e)USER_SW1_PORT, (PCU_PIN_ID_e)USER_SW1_PORT_ID, PCU_ALT_0);
    HAL_PCU_SetInOutMode((PCU_ID_e)USER_SW1_PORT, (PCU_PIN_ID_e)USER_SW1_PORT_ID, PCU_INOUT_OUTPUT_PUSH_PULL);
    HAL_PCU_SetPullUpDown((PCU_ID_e)USER_SW1_PORT, (PCU_PIN_ID_e)USER_SW1_PORT_ID, PCU_PUPD_DISABLED);
    HAL_PCU_SetOutputBit((PCU_ID_e)USER_SW1_PORT, (PCU_PIN_ID_e)USER_SW1_PORT_ID, PCU_OUTPUT_BIT_CLEAR);
    HAL_PCU_SetOutputBit((PCU_ID_e)USER_SW1_PORT, (PCU_PIN_ID_e)USER_SW1_PORT_ID, PCU_OUTPUT_BIT_SET);

    HAL_PCU_SetIntrPort((PCU_ID_e)USER_SW1_PORT, (PCU_PIN_ID_e)USER_SW1_PORT_ID, PCU_INTR_MODE_EDGE, PCU_INTR_TRG_LOW_FALLING, USER_SW1_IRQ_NUM);
    HAL_PCU_SetIRQ(&tPcuCfg);

    s_MsgQId = osMessageQueueNew(5, sizeof(USER_SW_IRQ_t), NULL);
    osThreadNew(xLedOnOffTask, NULL, &osThreadAttr);
#endif

    osKernelStart();
}
#endif

/* --------------------------------- End Of File ------------------------------ */
