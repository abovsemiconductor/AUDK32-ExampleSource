/**
 *******************************************************************************
 * @file        timer1_capture.c
 * @author      ABOV R&D Division
 * @brief       Simple Application for TIMER1 peripheral
 *
 * Copyright 2026 ABOV Semiconductor Co.,Ltd. All rights reserved.
 *
 * This file is licensed under terms that are found in the LICENSE file
 * located at Document directory.
 * If this file is delivered or shared without applicable license terms,
 * the terms of the BSD-3-Clause license shall be applied.
 * Reference: https://opensource.org/licenses/BSD-3-Clause
 ******************************************************************************/

/*
 * Readme:
 * - This example demonstrates how to use TIMER1 in capture mode to capture the timer value on input signal edge.
 * - The captured value is printed in the interrupt handler.
 * 
 * Connection:
 * - PE0(Timer12) - PE1(Timer13)
 */

#include "abov_config.h"
#include "abov_simpleapp_config.h"

#include "hal_pcu.h"
#include "hal_timer1.h"

#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"

#if (CONFIG_APP_TIMER1 == 1)

static TIMER1_Context_t s_tTimer1Context[CONFIG_TIMER1_MAX_COUNT];

static void TIMER1_IRQHandler(uint32_t un32Event, void *pContext)
{
    TIMER1_Context_t *ptContext = (TIMER1_Context_t *)pContext;
    uint32_t un32Data;

    if(un32Event & TIMER1_EVENT_PERIODIC_MATCH)
    {
        ;
    }

    if(un32Event & TIMER1_EVENT_CAPTURE)
    {
        HAL_TIMER1_GetData(ptContext->eId, TIMER1_DATA_CAP_A, &un32Data);
        LOG("(%d) C-A (Data=%d)\n", ptContext->eId, un32Data);
    }

    if(un32Event & TIMER1_EVENT_CAPTURE_B)
    {
        HAL_TIMER1_GetData(ptContext->eId, TIMER1_DATA_CAP_B, &un32Data);
        LOG("(%d) C-B (Data=%d)\n", ptContext->eId, un32Data);
    }
}

static HAL_ERR_e PRV_TIMER1_SetOutputMode(void)
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    TIMER1_ID_e eId = TIMER1_ID_2;
    TIMER1_CLK_CFG_t tClkCfg =
    {
        .eClk = TIMER1_CLK_MCCR,
    };

    TIMER1_CFG_t tCfg =
    {
        .eMode = TIMER1_MODE_PERIODIC,
        .ePol = TIMER1_POL_HIGH,
        .bIntrEnable = true,
        .bOVFIntrEnable = false
    };

    tClkCfg.uSubClk.eMccr = TIMER1_CLK_MCCR_HSI;
    tClkCfg.un8MccrDiv = 10;
    tClkCfg.un16PreScale = 1000;

    tCfg.utData.tGRD.un16DataA = HSI_CLOCK / 10000;
    tCfg.utData.tGRD.un16DataB = HSI_CLOCK / 10000;

    eErr = HAL_PCU_SetAltMode((PCU_ID_e)TIMER12_OUT2_PORT, (PCU_PIN_ID_e)TIMER12_OUT2_PORT_ID, (PCU_ALT_e)TIMER12_OUT2_MUX_ID);
    if (eErr != HAL_ERR_OK)
    {
        return eErr;
    }

    /* Initialize instance */
    eErr = HAL_TIMER1_Init(eId);
    if (eErr != HAL_ERR_OK)
    {
        return eErr;
    }

    /* Set up the clock source */
    eErr = HAL_TIMER1_SetClkConfig(eId, &tClkCfg);
    if (eErr != HAL_ERR_OK)
    {
        return eErr;
    }

    /* Set up operation parameters */
    eErr = HAL_TIMER1_SetConfig(eId, &tCfg);
    if (eErr != HAL_ERR_OK)
    {
        return eErr;
    }

    /* Set IRQ priority and enable interrupt */
    s_tTimer1Context[eId].eId = eId;
    eErr = HAL_TIMER1_SetIRQ(eId, TIMER1_OPS_INTR, TIMER1_IRQHandler, (void *)&s_tTimer1Context[eId], 3);
    if (eErr != HAL_ERR_OK)
    {
        return eErr;
    }

    eErr = HAL_TIMER1_SetInOutPort(eId, TIMER1_PORT_OUT);
    if (eErr != HAL_ERR_OK)
    {
        return eErr;
    }

    eErr = HAL_TIMER1_Start(eId);
    if (eErr != HAL_ERR_OK)
    {
        return eErr;
    }

    return eErr;
}

static HAL_ERR_e PRV_TIMER1_SetCaptureMode(void)
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    TIMER1_ID_e eId = TIMER1_ID_3;
    TIMER1_CLK_CFG_t tClkCfg =
    {
        .eClk = TIMER1_CLK_MCCR,
    };

    TIMER1_CFG_t tCfg =
    {
        .eMode = TIMER1_MODE_CAPTURE,
        .ePol = TIMER1_POL_HIGH,
        .eCapClr = TIMER1_CAP_CLR_BOTH,
        .bIntrEnable = true,
        .bOVFIntrEnable = false
    };

    tClkCfg.uSubClk.eMccr = TIMER1_CLK_MCCR_HSI;
    tClkCfg.un8MccrDiv = 10;
    tClkCfg.un16PreScale = 1000;

    tCfg.utData.tGRD.un16DataA = 0;
    tCfg.utData.tGRD.un16DataB = 0;

    eErr = HAL_PCU_SetAltMode((PCU_ID_e)TIMER13_CAP3_PORT, (PCU_PIN_ID_e)TIMER13_CAP3_PORT_ID, (PCU_ALT_e)TIMER13_CAP3_MUX_ID);
    if (eErr != HAL_ERR_OK)
    {
        return eErr;
    }

    /* Initialize instance */
    eErr = HAL_TIMER1_Init(eId);
    if (eErr != HAL_ERR_OK)
    {
        return eErr;
    }

    /* Set up the clock source */
    eErr = HAL_TIMER1_SetClkConfig(eId, &tClkCfg);
    if (eErr != HAL_ERR_OK)
    {
        return eErr;
    }

    /* Set up operation parameters */
    eErr = HAL_TIMER1_SetConfig(eId, &tCfg);
    if (eErr != HAL_ERR_OK)
    {
        return eErr;
    }

    /* Set IRQ priority and enable interrupt */
    s_tTimer1Context[eId].eId = eId;
    eErr = HAL_TIMER1_SetIRQ(eId, TIMER1_OPS_INTR, TIMER1_IRQHandler, (void *)&s_tTimer1Context[eId], 3);
    if (eErr != HAL_ERR_OK)
    {
        return eErr;
    }

    eErr = HAL_TIMER1_Start(eId);
    if (eErr != HAL_ERR_OK)
    {
        return eErr;
    }

    return eErr;
}

void TIMER1_Capture(void)
{
    HAL_ERR_e eErr = HAL_ERR_OK;

    eErr = PRV_TIMER1_SetOutputMode();
    if(eErr != HAL_ERR_OK)
    {
        return;
    }

    eErr = PRV_TIMER1_SetCaptureMode();
    if(eErr != HAL_ERR_OK)
    {
        return;
    }
}
#endif
