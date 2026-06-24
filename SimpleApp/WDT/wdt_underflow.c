/**
 *******************************************************************************
 * @file        wdt_underflow.c
 * @author      ABOV R&D Division
 * @brief       Simple Application for WDT underflow mode
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
 * This example demonstrates how to use WDT in underflow mode.
 * 
 * Connection:
 * - None
 */

#include "abov_config.h"
#include "abov_simpleapp_config.h"

#include "hal_wdt.h"

#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"

#if (CONFIG_APP_WDT == 1)

static void WDT_IRQHandler(uint32_t un32Event, void *pContext)
{
    if(un32Event & WDT_EVENT_UNDERFLOW)
    {
        LOG("WDT Underflow Event Fire.\n");
    }
}

void WDT_Underflow(void)
{
    HAL_ERR_e eErr = HAL_ERR_OK;

    LOG("WDT underflow mode example.\n");

    WDT_CLK_CFG_t tClkCfg =
    {
        .eClk = WDT_CLK_MCCR,
        .eMccr = WDT_CLK_MCCR_HSI,
        .un8MccrDiv = 15,
        .ePreDiv = WDT_CLK_PREDIV_256
    };

    WDT_CFG_t tCfg =
    {
        .un8Mode = WDT_MODE_CNT | WDT_MODE_UNDERFLOW,
        .un8IntrEnable = WDT_INTR_UNDERFLOW,
        .un32InitCnt = 16666,
        .un32MatchCnt = 8333
    };

    /* Initialize instance */
    eErr = HAL_WDT_Init(WDT_ID_0);
    if (eErr != HAL_ERR_OK)
    {
        LOG("HAL_WDT_Init() error, (%d)\n", eErr);
        return;
    }

    /* Set clock source as HSI and set divider to 15, 256 */
    eErr = HAL_WDT_SetClkConfig(WDT_ID_0, &tClkCfg);
    if (eErr != HAL_ERR_OK)
    {
        LOG("HAL_WDT_SetClkConfig() error, (%d)\n", eErr);
        return;
    }

    /* Set configuration as underflow mode, enable underflow interrupt, set initial count and match count */ 
    eErr = HAL_WDT_SetConfig(WDT_ID_0, &tCfg);
    if (eErr != HAL_ERR_OK)
    {
        LOG("HAL_WDT_Config() error, (%d)\n", eErr);
        return;
    }

    /* Set underflow interrupt */
    eErr = HAL_WDT_SetIRQ(WDT_ID_0, WDT_OPS_INTR, WDT_IRQHandler, NULL, 3, false);
    if (eErr != HAL_ERR_OK)
    {
        LOG("HAL_WDT_SetIRQ() error, (%d)\n", eErr);
        return;
    }

    /* Start WDT */
    eErr = HAL_WDT_Start(WDT_ID_0);
    if (eErr != HAL_ERR_OK)
    {
        LOG("HAL_WDT_Start() error, (%d)\n", eErr);
        return;
    }

}
#endif
