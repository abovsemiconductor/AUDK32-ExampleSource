/**
 *******************************************************************************
 * @file        scu_lvd_indicator.c
 * @author      ABOV R&D Division
 * @brief       Simple Application for SCU LVD Indicator
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
 * - This example demonstrates how to use SCU LVD and LVI.
 * - The LVD and LVI events are triggered when the voltage drops below the configured threshold.
 * 
 * Connection:
 * - None
 */

#include "abov_config.h"
#include "abov_simpleapp_config.h"

#include "hal_pcu.h"
#include "hal_scu_lvd.h"

#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"

#if (CONFIG_APP_SCU == 1)

static void SCU_LVD_IRQHandler(uint32_t un32Event, void *pContext)
{
    if(un32Event & SCULVD_IND_EVENT_MASK)
    {
        LOG("LVD Event Fire. (Masked)\n");
    }
}

void SCU_LVD_LowVoltageIndicator(void)
{
    HAL_ERR_e eErr = HAL_ERR_OK;

    LOG("SCU LVD Indicator example.\n");

    LOG("Please use power supply to drop the voltage to trigger level 1(3.01V)\n");
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
}

#endif
