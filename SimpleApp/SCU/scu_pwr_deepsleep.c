/**
 *******************************************************************************
 * @file        scu_pwr_deepsleep.c
 * @author      ABOV R&D Division
 * @brief       Simple Application for SCU Power Deep Sleep
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
 * - This example demonstrates how to use SCU Power Deep Sleep mode.
 * - The system enters Deep Sleep mode and can be woken up by configured wakeup sources.
 * 
 * Connection:
 * - None
 */

#include "abov_config.h"
#include "abov_simpleapp_config.h"

#include "hal_pcu.h"
#include "hal_scu_clk.h"
#include "hal_scu_lvd.h"
#include "hal_scu_pwr.h"

#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"

#if (CONFIG_APP_SCU == 1)

static void SCU_LVD_IRQHandler(uint32_t un32Event, void *pContext)
{
    if(un32Event & SCULVD_IND_EVENT_MASK)
    {
        LOG("LVD WakeUp Event Fire. (Masked)\n");
    }
}

void SCU_PWR_DeepSleep(void)
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    uint32_t un32Aon = 0;
    uint32_t un32AonEnable = 0;

    SCUCLK_MCLK_CFG_t tMClkCfg = 
    {
        .eMClk = SCUCLK_SRC_LSI,
        .ePreMClkDiv = SCUCLK_DIV_NONE
    };

    LOG("SCU Power Deep Sleep example.\n");

    eErr = HAL_SCU_LVD_SetIndicatorEnable(SCULVD_IND_LVL_1, true);
    if(eErr != HAL_ERR_OK)
    {
        LOG("HAL_SCU_LVD_SetIndicatorEnable() error, (%d)\n", eErr);
        return;
    }

    eErr = HAL_SCU_LVD_SetIRQIndicator(SCU_LVD_IRQHandler, NULL, 3, SCULVD_IND_INTR_MASK);
    if(eErr != HAL_ERR_OK)
    {
        LOG("HAL_SCU_LVD_SetIRQIndicator() error, (%d)\n", eErr);
        return;
    }

    eErr = HAL_SCU_LVD_SetWakeupSrc(true);
    if (eErr != HAL_ERR_OK)
    {
        LOG("HAL_SCU_LVD_SetWakeupSrc() error, (%d)\n", eErr);
        return;
    }

    un32Aon |= SCUPWR_AON_VDC | SCUPWR_AON_BGR | SCUPWR_AON_LSI;
    un32AonEnable = SCUPWR_AON_VDC | SCUPWR_AON_BGR | SCUPWR_AON_LSI;
    eErr = HAL_SCU_PWR_SetAlwaysOn(un32Aon, un32AonEnable);
    if (eErr != HAL_ERR_OK)
    {
        LOG("HAL_SCU_PWR_SetAlwaysOn() error, (%d)\n", eErr);
        return;
    }

    eErr = HAL_SCU_CLK_SetMClk(&tMClkCfg);
    if (eErr != HAL_ERR_OK)
    {
        LOG("HAL_SCU_CLK_SetMClk() error, (%d)\n", eErr);
        return;
    }

    tMClkCfg.eMClk = SCUCLK_SRC_HSI;

    eErr = HAL_SCU_PWR_SetMode(SCUPWR_MODE_DEEPSLEEP);
    if(eErr != HAL_ERR_OK)
    {
        LOG("HAL_SCU_PWR_SetMode() error, (%d)\n", eErr);
        return;
    }
    eErr = HAL_SCU_CLK_SetMClk(&tMClkCfg);
    if(eErr != HAL_ERR_OK)
    {
        LOG("HAL_SCU_CLK_SetMClk() error, (%d)\n", eErr);
        return;
    }
}

#endif
