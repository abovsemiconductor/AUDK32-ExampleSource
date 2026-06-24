/**
 *******************************************************************************
 * @file        adc_comparison.c
 * @author      ABOV R&D Division
 * @brief       Simple Application for ADC peripheral
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
 * This example demonstrates how to use ADC in comparison mode with Timer1 trigger.
 * The conversion result is printed in the interrupt handler.
 * 
 * Connection:
 * - PE2(ADC16) - PE6(Timer10)
 */

#include "abov_config.h"
#include "abov_simpleapp_config.h"

#include "hal_adc.h"
#include "hal_timer1.h"
#include "hal_pcu.h"

#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"

#if (CONFIG_APP_ADC == 1)

#define ADC_SEQ_CNT             8
#define ADC_RBUF_SIZE           8

static ADC_Context_t s_tADCContext[CONFIG_ADC_MAX_COUNT];
static ADC_SEQ_DATA_t s_tResult[ADC_RBUF_SIZE];
static uint32_t s_un32RCnt = 0;

static void PRV_ADC_IRQHandler(uint32_t un32Event, void *pContext)
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    ADC_Context_t *ptContext = (ADC_Context_t *)pContext;

    if (un32Event & ADC_EVENT_SINGLE_CAPTURED)
    {
        eErr = HAL_ADC_GetData(ptContext->eId, 0, &s_tResult[s_un32RCnt++]);
        if(eErr != HAL_ERR_OK)
        {
            HAL_ADC_Stop(ptContext->eId);
        }
    }

    if(s_un32RCnt < ADC_RBUF_SIZE - 1)
    {
        HAL_ADC_Start(ptContext->eId);
    }
    else
    {
        HAL_ADC_Stop(ptContext->eId);
    }
}

static HAL_ERR_e PRV_ADC_SetTimerSource(void)
{
    HAL_ERR_e eErr = HAL_ERR_OK;

    TIMER1_CLK_CFG_t tTimerClkCfg = 
    {
        .eClk = TIMER1_CLK_MCCR,
        .uSubClk.eMccr = TIMER1_CLK_MCCR_HSI,
        .un8MccrDiv = 10,
        .un16PreScale = 1000
    };

    TIMER1_CFG_t tTimer1Cfg = 
    {
        .eMode = TIMER1_MODE_PERIODIC,
        .ePol = TIMER1_POL_HIGH,
        .utData.tGRD.un16DataA = 1600,
        .utData.tGRD.un16DataB = 1600
    };

    /* Initialize Port */
    HAL_PCU_SetAltMode((PCU_ID_e)TIMER10_OUT0_PORT, (PCU_PIN_ID_e)TIMER10_OUT0_PORT_ID, (PCU_ALT_e)TIMER10_OUT0_MUX_ID);

    /* Set Timer1 */
    eErr = HAL_TIMER1_Init(TIMER1_ID_0);
    if (eErr != HAL_ERR_OK)
    {
        LOG("HAL_TIMER1_Init() error, (%d)\n", eErr);
        return eErr;
    }

    eErr = HAL_TIMER1_SetClkConfig(TIMER1_ID_0, &tTimerClkCfg);
    if (eErr != HAL_ERR_OK)
    {
        LOG("HAL_TIMER1_SetClkConfig() error, (%d)\n", eErr);
        return eErr;
    }

    eErr = HAL_TIMER1_SetConfig(TIMER1_ID_0, &tTimer1Cfg);
    if (eErr != HAL_ERR_OK)
    {
        LOG("HAL_TIMER1_SetConfig() error, (%d)\n", eErr);
        return eErr;
    }

    eErr = HAL_TIMER1_SetIRQ(TIMER1_ID_0, TIMER1_OPS_INTR, NULL, NULL, 3);
    if (eErr != HAL_ERR_OK)
    {
        LOG("HAL_TIMER1_SetIRQ() error, (%d)\n", eErr);
        return eErr;
    }

    eErr = HAL_TIMER1_SetInOutPort(TIMER1_ID_0, TIMER1_PORT_OUT);
    if (eErr != HAL_ERR_OK)
    {
        LOG("HAL_TIMER1_SetInOutPort() error, (%d)\n", eErr);
        return eErr;
    }

    eErr = HAL_TIMER1_Start(TIMER1_ID_0);
    if (eErr != HAL_ERR_OK)
    {
        LOG("HAL_TIMER1_Start() error, (%d)\n", eErr);
        return eErr;
    }

    return eErr;
}

void ADC_INIT_Comparison(void)
{
    HAL_ERR_e eErr = HAL_ERR_OK;

    LOG("ADC Comparison Mode Example Start\n");

    ADC_CLK_CFG_t tAdcClkCfg =
    {
        .eClk = ADC_CLK_PCLK,
        .un8PClkDiv = 63
    };

    ADC_CFG_t tAdcCfg =
    {
        .eMode = ADC_MODE_SINGLE,
        .eBaseTrgSrc = ADC_TRG_SRC_ADST,
        .un8SeqCnt = 0,
        .un8SamplingTime = 0
    };

    ADC_SEQ_TRG_CFG_t tAdcSeqTrgCfg =
    {
        .eType = ADC_TRG_TYPE_INDEPENDENT,
        .eTrgSrc = ADC_TRG_SRC_ADST,
        .utCfg.tInd.un8SeqNum = 0,
        .utCfg.tInd.un8ChNum = ADC0_IN_CHANNEL_NUM,
    };

    ADC_CMP_CFG_t tCmpCfg =
    {
        .bEnable = true,
        .un8ChNum = 16,
        .un16Data = 1024,
        .bIntrEnable = true,
        .bIntrTrg = false
    };

    HAL_PCU_SetAltMode((PCU_ID_e)ADC0_IN_PORT, (PCU_PIN_ID_e)ADC0_IN_PORT_ID, (PCU_ALT_e)ADC0_IN_MUX_ID);

    eErr = PRV_ADC_SetTimerSource();
    if (eErr != HAL_ERR_OK)
    {
        LOG("PRV_ADC_SetTimerSource() error, (%d)\n", eErr);
        return;
    }

    eErr = HAL_ADC_Init(ADC_ID_0);
    if (eErr != HAL_ERR_OK)
    {
        LOG("HAL_ADC_Init() error, (%d)\n", eErr);
        return;
    }

    eErr = HAL_ADC_SetClkConfig(ADC_ID_0, &tAdcClkCfg);
    if (eErr != HAL_ERR_OK)
    {
        LOG("HAL_ADC_SetClkConfig() error, (%d)\n", eErr);
        return;
    }

    eErr = HAL_ADC_SetConfig(ADC_ID_0, &tAdcCfg);
    if (eErr != HAL_ERR_OK)
    {
        LOG("HAL_ADC_SetConfig() error, (%d)\n", eErr);
        return;
    }

    eErr = HAL_ADC_SetIRQ(ADC_ID_0, ADC_OPS_INTR, PRV_ADC_IRQHandler, &s_tADCContext[ADC_ID_0], 3);
    if (eErr != HAL_ERR_OK)
    {
        LOG("HAL_ADC_SetIRQ() error, (%d)\n", eErr);
        return;
    }

    eErr = HAL_ADC_SetSeqConfig(ADC_ID_0, &tAdcSeqTrgCfg);
    if (eErr != HAL_ERR_OK)
    {
        LOG("HAL_ADC_SetSeqConfig() error, (%d)\n", eErr);
        return;
    }

    eErr = HAL_ADC_SetCmpConfig(ADC_ID_0, &tCmpCfg);
    if(eErr != HAL_ERR_OK)
    {
        LOG("HAL_ADC_SetCmpConfig() error, (%d)\n", eErr);
        return;
    }

    eErr = HAL_ADC_Start(ADC_ID_0);
    if (eErr != HAL_ERR_OK)
    {
        LOG("HAL_ADC_Start() error, (%d)\n", eErr);
        return;
    }

    SystemDelayMS(3000);

    eErr = HAL_ADC_Stop(ADC_ID_0);
    if (eErr != HAL_ERR_OK)
    {
        LOG("HAL_ADC_Stop() error, (%d)\n", eErr);
        return;
    }

    eErr = HAL_TIMER1_Stop(TIMER1_ID_0);
    if (eErr != HAL_ERR_OK)
    {
        LOG("HAL_TIMER1_Stop() error, (%d)\n", eErr);
        return;
    }

    for(int i = 0; i < ADC_RBUF_SIZE; i++)
    {
        LOG("[%s][%02d:%8d]\t[Ch=%d]\t[TrgInfo=%x]\n", 
            s_tResult[i].bReadDDR == true ? "DDR" : "DRx",
            i, s_tResult[i].un16Result, s_tResult[i].un8ChInfo, s_tResult[i].un8TrgInfo);
    }
}
#endif
