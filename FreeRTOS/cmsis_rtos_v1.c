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

#if defined(FREERTOS_EXAMPE_API_CMSIS_RTOS_V1)
#include "cmsis_os.h"

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

static osMessageQId s_MsgQId;
static osSemaphoreId s_SemId;
static osMailQId s_MailQId;
static EX_LED_Context_t s_tLedContext;

#if defined(FREERTOS_EXAMPE_LED_BLIKING_TASK_ONLY)
static void xLedBlinkingTask(void const *pParam)
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
static void xMsgSendTask(void const *pParam)
{
    bool bLedOnOff = false;

    LOG("LED_BLIKING_TASK_MESSAGE - SENDER\n");

    while(1)
    {
        bLedOnOff = !bLedOnOff;

        osMessagePut(s_MsgQId, (uint32_t)bLedOnOff, 0);

        osDelay(500);
    }
}

static void xMsgRecvTask(void const *pParam)
{
    uint32_t un32LedID;
    osEvent evt;

    LOG("LED_BLIKING_TASK_MESSAGE - RECEIVER\n");

    while(1)
    {
        evt = osMessageGet(s_MsgQId, osWaitForever);

        if(evt.status == osEventMessage)
        {
            LOG("Message Received.\n");

            for (un32LedID = FIRST_LED_ID; un32LedID <= LAST_LED_ID; un32LedID++)
            {
                HAL_PCU_SetOutputBit((PCU_ID_e)LED_PORT_ID, (PCU_PIN_ID_e)un32LedID, (PCU_OUTPUT_BIT_e)evt.value.v);
            }
        }
    }
}
#elif defined(FREERTOS_EXAMPE_LED_BLIKING_TASK_SEMAPHORE)
static void xSemaTask1(void const *pParam)
{
    LOG("LED_BLIKING_TASK_SEMAPHORE - FLAG TOGGLE\n");

    while(1)
    {
        if(osSemaphoreWait(s_SemId, osWaitForever) == osOK)
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

static void xSemaTask2(void const *pParam)
{
    uint32_t un32LedID;

    LOG("LED_BLIKING_TASK_SEMAPHORE - CONTROL\n");

    while(1)
    {
        if(osSemaphoreWait(s_SemId, osWaitForever) == osOK)
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
    USER_SW_IRQ_t *ptIRQ;

    ptIRQ = osMailAlloc(s_MailQId, osWaitForever);
    if (ptIRQ != NULL)
    {
        ptIRQ->un32Event = un32Event;
        ptIRQ->pContext = pContext;

        osMailPut(s_MailQId, ptIRQ);
    }
}

static void xLedOnOffTask(void const *pParam)
{
    USER_SW_IRQ_t *ptIRQ;
    uint32_t un32LedID;
    bool bLedOnOff = false;
    osEvent evt;

    LOG("LED ON/OFF by User SW\n");

    while(1)
    {
        evt = osMailGet(s_MailQId, osWaitForever);
        if(evt.status == osEventMail)
        {
            LOG("Message Received.\n");

            ptIRQ = (USER_SW_IRQ_t *)evt.value.p;
            if( (ptIRQ->un32Event >> (USER_SW1_PORT_ID * 2)) & 0x3)
            {
                bLedOnOff = !bLedOnOff;
                for (un32LedID = FIRST_LED_ID; un32LedID <= LAST_LED_ID; un32LedID++)
                {
                    HAL_PCU_SetOutputBit((PCU_ID_e)LED_PORT_ID, (PCU_PIN_ID_e)un32LedID, (PCU_OUTPUT_BIT_e)bLedOnOff);
                }
            }

            osMailFree(s_MailQId, ptIRQ);
        }
    }
}
#endif

/**********************************************************************
 * @brief		Main program
 * @param[in]	None
 * @return	None
 **********************************************************************/
void cmsis_rtos_v1(void)
{
#if defined(FREERTOS_EXAMPE_LED_BLIKING_TASK_ONLY)
    osThreadDef(LedBlinkingTask, xLedBlinkingTask, osPriorityNormal, 1, LED_BLIKING_TASK_STASK);
    osThreadCreate(osThread(LedBlinkingTask), NULL);
#elif defined(FREERTOS_EXAMPE_LED_BLIKING_TASK_MESSAGE)
    osMessageQDef(MsgQ, 5, uint32_t);
    s_MsgQId = osMessageCreate(osMessageQ(MsgQ), NULL);

    osThreadDef(MsgSendTask, xMsgSendTask, osPriorityNormal, 1, LED_BLIKING_TASK_STASK);
    osThreadDef(MsgRecvTask, xMsgRecvTask, osPriorityNormal, 1, LED_BLIKING_TASK_STASK);
    osThreadCreate(osThread(MsgSendTask), NULL);
    osThreadCreate(osThread(MsgRecvTask), NULL);
#elif defined(FREERTOS_EXAMPE_LED_BLIKING_TASK_SEMAPHORE)
    osSemaphoreDef(Semaphore);
    s_SemId = osSemaphoreCreate(osSemaphore(Semaphore), 1);
    osSemaphoreRelease(s_SemId);

    osThreadDef(SemaTask1, xSemaTask1, osPriorityNormal, 1, LED_BLIKING_TASK_STASK);
    osThreadDef(SemaTask2, xSemaTask2, osPriorityNormal, 1, LED_BLIKING_TASK_STASK);
    osThreadCreate(osThread(SemaTask1), NULL);
    osThreadCreate(osThread(SemaTask2), NULL);
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

    osMailQDef(MailQ, 5, USER_SW_IRQ_t);
    s_MailQId = osMailCreate(osMailQ(MailQ), NULL);

    osThreadDef(LedOnOffTask, xLedOnOffTask, osPriorityNormal, 1, LED_BLIKING_TASK_STASK);
    osThreadCreate(osThread(LedOnOffTask), NULL);
#endif

    osKernelStart();
}
#endif

/* --------------------------------- End Of File ------------------------------ */
