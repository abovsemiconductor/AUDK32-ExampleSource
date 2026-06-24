/**
 *******************************************************************************
 * @file        timer1_pwm.c
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
 * - This example demonstrates how to use TIMER1 in PWM mode to generate a PWM signal.
 * - The interrupt handler prints a message when the PWM period or duty event occurs.
 * 
 * Connection:
 * - None 
 */

#include "abov_config.h"
#include "abov_simpleapp_config.h"

#include "hal_timer1.h"

#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"

#if (CONFIG_APP_TIMER1 == 1)

static void Timer1_IRQHandler(uint32_t un32Event, void *pContext)
{
    if(un32Event & TIMER1_EVENT_PWM_PERIOD)
    {
        LOG("Timer1 PERIOD Event Fire.\n");
    }

    if(un32Event & TIMER1_EVENT_PWM_DUTY)
    {
        LOG("Timer1 DUTY Event Fire.\n");
    }
}

void TIMER1_Pwm(void)
{
    HAL_ERR_e eErr = HAL_ERR_OK;

    TIMER1_CLK_CFG_t tClkCfg =
    {
        .eClk = TIMER1_CLK_MCCR,
    };

    TIMER1_CFG_t tCfg =
    {
        .eMode = TIMER1_MODE_PWM,
        .ePol = TIMER1_POL_HIGH,
        .bIntrEnable = true,
        .bOVFIntrEnable = false
    };

    tClkCfg.uSubClk.eMccr = TIMER1_CLK_MCCR_HSI;
    tClkCfg.un8MccrDiv = 10;
    tClkCfg.un16PreScale = 1000;

    tCfg.utData.tGRD.un16DataA = 3200;
    tCfg.utData.tGRD.un16DataB = 6400;

    /* Initialize instance */
    eErr = HAL_TIMER1_Init(TIMER1_ID_0);
    if (eErr != HAL_ERR_OK)
    {
        return;
    }

    /* Set up the clock source */
    eErr = HAL_TIMER1_SetClkConfig(TIMER1_ID_0, &tClkCfg);
    if (eErr != HAL_ERR_OK)
    {
        return;
    }

    /* Set up operation parameters */
    eErr = HAL_TIMER1_SetConfig(TIMER1_ID_0, &tCfg);
    if (eErr != HAL_ERR_OK)
    {
        return;
    }

    /* Set IRQ priority and enable interrupt */
    eErr = HAL_TIMER1_SetIRQ(TIMER1_ID_0, TIMER1_OPS_INTR, Timer1_IRQHandler, NULL, 3);
    if (eErr != HAL_ERR_OK)
    {
        return;
    }

    eErr = HAL_TIMER1_Start(TIMER1_ID_0);
    if (eErr != HAL_ERR_OK)
    {
        return;
    }
}
#endif
