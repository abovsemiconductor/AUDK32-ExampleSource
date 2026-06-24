/**
 *******************************************************************************
 * @file        hw_timer.c
 * @author      ABOV R&D Division
 * @brief       hardware timer
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

#include "hal_scu.h"
#include "hal_scu_clk.h"
#include "hal_pcu.h"
#include "hal_timer1.h"
#include "hw_timer.h"

void EX_HW_TIMER_6_Stop(void)
{
    HAL_TIMER1_Stop(TIMER1_ID_6);
    LOG("EX_HW_TIMER_6_Stop\n");
}

void EX_HW_TIMER_6_Start(pfnTimer6_EVT_Handler_t cb, TIMER1_MODE_e mode, uint32_t mSec)
{
    /*
        to set playtime.
        PCLK : 32MHz
        CKSEL : 64
        PRS : 499
        TMCLK = PCLK / CKSEL = 500 Hz
        TCLK = TMCLK / (PRS + 1) = 1 Hz
    */
    TIMER1_CLK_CFG_t tTimeClkCfg = {
        .eClk = TIMER1_CLK_PCLK,
        .uSubClk.ePClkDiv = TIMER1_PCLK_DIV_64,
        .un16PreScale = 499
    };

    TIMER1_CFG_t tTimeCfg = {
        .eMode = mode,
        .bIntrEnable = true,
        .utData.tGRD.un16DataA = mSec,	// 1000 = 1 sec
        .utData.tGRD.un16DataB = mSec 	// 1000 = 1 sec
    };

    if (cb != NULL)
    {
        /* Timer1 channel 6 for clock */
        HAL_TIMER1_Init(TIMER1_ID_6);
        HAL_TIMER1_SetClkConfig(TIMER1_ID_6, &tTimeClkCfg);
        HAL_TIMER1_SetConfig(TIMER1_ID_6, &tTimeCfg);
        HAL_TIMER1_SetIRQ(TIMER1_ID_6, TIMER1_OPS_INTR, cb, NULL, 3);
        HAL_TIMER1_Start(TIMER1_ID_6);
        LOG("EX_HW_TIMER_6_Start\n");
    }
}
