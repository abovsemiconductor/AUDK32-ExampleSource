/**
 *******************************************************************************
 * @file        freertos.c
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

#if defined(FREERTOS_EXAMPE_API_NATIVE)
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#define LED_BLIKING_TASK_STASK 256

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

static QueueHandle_t s_xQueue;
static SemaphoreHandle_t s_xSemaphore;
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

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
#elif defined(FREERTOS_EXAMPE_LED_BLIKING_TASK_MESSAGE)
static void xMsgSendTask(void *pParam)
{
    EX_LED_Context_t tContext = {.bLedOnOff = false};

    LOG("LED_BLIKING_TASK_MESSAGE - SENDER\n");

    while(1)
    {
        tContext.bLedOnOff = !tContext.bLedOnOff;

        xQueueSend(s_xQueue, &tContext, 0);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void xMsgRecvTask(void *pParam)
{
    uint32_t un32LedID;
    EX_LED_Context_t tContext;

    LOG("LED_BLIKING_TASK_MESSAGE - RECEIVER\n");

    while(1)
    {
        if (xQueueReceive(s_xQueue, &tContext, portMAX_DELAY) == pdPASS)
        {
            LOG("Message Received.\n");

            for (un32LedID = FIRST_LED_ID; un32LedID <= LAST_LED_ID; un32LedID++)
            {
                HAL_PCU_SetOutputBit((PCU_ID_e)LED_PORT_ID, (PCU_PIN_ID_e)un32LedID, (PCU_OUTPUT_BIT_e)tContext.bLedOnOff);
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
        if(xSemaphoreTake(s_xSemaphore, portMAX_DELAY) == pdTRUE)
        {
            if(s_tLedContext.bIsLedControl)
            {
                s_tLedContext.bLedOnOff = !s_tLedContext.bLedOnOff;
                s_tLedContext.bIsLedControl = false;
            }

            xSemaphoreGive(s_xSemaphore);

            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}

static void xSemaTask2(void *pParam)
{
    uint32_t un32LedID;

    LOG("LED_BLIKING_TASK_SEMAPHORE - CONTROL\n");

    while(1)
    {
        if(xSemaphoreTake(s_xSemaphore, portMAX_DELAY) == pdTRUE)
        {
            LOG("CONTROL Get : %s\n", s_tLedContext.bLedOnOff ? "On" : "Off");
            for (un32LedID = FIRST_LED_ID; un32LedID <= LAST_LED_ID; un32LedID++)
            {
                HAL_PCU_SetOutputBit((PCU_ID_e)LED_PORT_ID, (PCU_PIN_ID_e)un32LedID, (PCU_OUTPUT_BIT_e)s_tLedContext.bLedOnOff);
            }
            s_tLedContext.bIsLedControl = true;

            xSemaphoreGive(s_xSemaphore);
            LOG("CONTROL Release\n");

            vTaskDelay(pdMS_TO_TICKS(500));
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

    if(s_xQueue != NULL)
    {
        xQueueSendFromISR(s_xQueue, &tIRQ, 0);
    }
}

static void xLedOnOffTask(void *pParam)
{
    USER_SW_IRQ_t tIRQ;
    uint32_t un32LedID;
    bool bLedOnOff = false;

    LOG("LED ON/OFF by User SW\n");

    while(1)
    {
        if (xQueueReceive(s_xQueue, &tIRQ, portMAX_DELAY) == pdPASS)
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
extern void xPortSysTickHandler( void );

void SysTick_Handler (void)
{
    xPortSysTickHandler();
}

void freertos_native(void)
{
#if defined(FREERTOS_EXAMPE_LED_BLIKING_TASK_ONLY)
    xTaskCreate(xLedBlinkingTask, "LedBlinkingTask", LED_BLIKING_TASK_STASK, NULL, 1, NULL);
#elif defined(FREERTOS_EXAMPE_LED_BLIKING_TASK_MESSAGE)
    s_xQueue = xQueueCreate(2, sizeof(EX_LED_Context_t));
    xTaskCreate(xMsgSendTask, "MsgSendTask", LED_BLIKING_TASK_STASK, NULL, 1, NULL);
    xTaskCreate(xMsgRecvTask, "MsgRecvTask", LED_BLIKING_TASK_STASK, NULL, 2, NULL);
#elif defined(FREERTOS_EXAMPE_LED_BLIKING_TASK_SEMAPHORE)
    s_xSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(s_xSemaphore);
    xTaskCreate(xSemaTask1, "SemaTask1", LED_BLIKING_TASK_STASK, NULL, 1, NULL);
    xTaskCreate(xSemaTask2, "SemaTask2", LED_BLIKING_TASK_STASK, NULL, 2, NULL);
#else
    PCU_IRQ_CFG_t tPcuCfg = {
        .eId = (PCU_ID_e)USER_SW1_PORT,
        .eOps = PCU_OPS_INTR,
        .pfnHandler = &USER_SW_IRQHandler,
        .pContext = NULL,
        .un32IRQPrio = 5,
        .un8IntNum = USER_SW1_IRQ_NUM
    };

    HAL_PCU_SetAltMode((PCU_ID_e)USER_SW1_PORT, (PCU_PIN_ID_e)USER_SW1_PORT_ID, PCU_ALT_0);
    HAL_PCU_SetInOutMode((PCU_ID_e)USER_SW1_PORT, (PCU_PIN_ID_e)USER_SW1_PORT_ID, PCU_INOUT_OUTPUT_PUSH_PULL);
    HAL_PCU_SetPullUpDown((PCU_ID_e)USER_SW1_PORT, (PCU_PIN_ID_e)USER_SW1_PORT_ID, PCU_PUPD_DISABLED);
    HAL_PCU_SetOutputBit((PCU_ID_e)USER_SW1_PORT, (PCU_PIN_ID_e)USER_SW1_PORT_ID, PCU_OUTPUT_BIT_CLEAR);
    HAL_PCU_SetOutputBit((PCU_ID_e)USER_SW1_PORT, (PCU_PIN_ID_e)USER_SW1_PORT_ID, PCU_OUTPUT_BIT_SET);

    HAL_PCU_SetIntrPort((PCU_ID_e)USER_SW1_PORT, (PCU_PIN_ID_e)USER_SW1_PORT_ID, PCU_INTR_MODE_EDGE, PCU_INTR_TRG_LOW_FALLING, USER_SW1_IRQ_NUM);
    HAL_PCU_SetIRQ(&tPcuCfg);

    s_xQueue = xQueueCreate(3, sizeof(USER_SW_IRQ_t));
    xTaskCreate(xLedOnOffTask, "LedOnOffTask", LED_BLIKING_TASK_STASK, NULL, 1, NULL);
#endif

    vTaskStartScheduler();
}
#endif

/* --------------------------------- End Of File ------------------------------ */
