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

#include "hal_frt.h"

#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"

#if (CONFIG_APP_FRT == 1) && (FRT_APP == FRT_APP_MODE_MATCH)

static FRT_Context_t s_tFrtContext[CONFIG_FRT_MAX_COUNT];

static void PRV_FRT_IRQHandler(uint32_t un32Event, void *pContext)
{
    FRT_Context_t *ptContext = (FRT_Context_t *)pContext;

    if(un32Event & FRT_EVENT_MATCH)
    {
        LOG("[FRT] (%d) M evt fired\n", ptContext->eId);
    }
}

void FRT_Match(void)
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    FRT_ID_e eId = FRT_ID_0;

    FRT_CLK_CFG_t tClkCfg = {
        .eClk = FRT_CLK_MCCR,
        .eMccr = FRT_CLK_MCCR_HSI,
        .un8MccrDiv = 200,
        .ePreDiv = FRT_CLK_PREDIV_1,
    };

    FRT_CFG_t tCfg = {
        .eMode = FRT_MODE_MATCH,
        .eIntr = FRT_INTR_MATCH,
        .un32MatchCnt = 320000,
    };

    LOG("[FRT] Match interrupt\n");

    eErr = HAL_FRT_Init(eId);
    if (eErr != HAL_ERR_OK)
    {
        return;
    }

    eErr = HAL_FRT_SetClkConfig(eId, &tClkCfg);
    if (eErr != HAL_ERR_OK)
    {
        return;
    }

    eErr = HAL_FRT_SetConfig(eId, &tCfg);
    if (eErr != HAL_ERR_OK)
    {
        return;
    }

    s_tFrtContext[eId].eId = eId;
    eErr = HAL_FRT_SetIRQ(eId, FRT_OPS_INTR, PRV_FRT_IRQHandler, &s_tFrtContext[eId], 3);
    if (eErr != HAL_ERR_OK)
    {
        return;
    }

    eErr = HAL_FRT_Start(eId, false);
    if (eErr != HAL_ERR_OK)
    {
        return;
    }

    LOG("[FRT] Id (%d) Start\n", eId);
}
#endif
