/**
 *******************************************************************************
 * @file        pcu_gpio_isr.c
 * @author      ABOV R&D Division
 * @brief       Simple Application for SCU Clock Output
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
 * - This example demonstrates how to output MCLK on CLKO pin.
 * 
 * Connection:
 * - None
 */

#include "abov_config.h"
#include "abov_simpleapp_config.h"

#include "hal_pcu.h"
#include "hal_scu_clk.h"

#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"

#if (CONFIG_APP_SCU == 1)

void SCU_CLK_ClockOutput()
{
    HAL_ERR_e eErr = HAL_ERR_OK;

    LOG("SCU Clock Output example.\n");

    SCUCLK_MCLK_CFG_t tMClkCfg = 
    {
        .eMClk = SCUCLK_SRC_HSI,
        .ePreMClkDiv = SCUCLK_DIV_NONE
    };

    /* Initialize CLKO Port*/
    HAL_PCU_SetAltMode((PCU_ID_e)CLKO_PORT,(PCU_PIN_ID_e)CLKO_PORT_ID,(PCU_ALT_e)CLKO_MUX_ID);

    eErr = HAL_SCU_CLK_SetOutput(SCUCLK_SRC_MCLK, 2, true);
    if (eErr != HAL_ERR_OK)
    {
        LOG("HAL_SCU_CLK_SetOutput() error, (%d)\n", eErr);
        return;
    }

    eErr = HAL_SCU_CLK_SetMClk(&tMClkCfg);
    if (eErr != HAL_ERR_OK)
    {
        LOG("HAL_SCU_CLK_SetMClk() error, (%d)\n", eErr);
        return;
    }
    LOG("Output 8 MHz MCLK from HSI.\n");
}

#endif
