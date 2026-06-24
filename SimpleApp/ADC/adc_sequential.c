/**
 *******************************************************************************
 * @file        adc_sequential.c
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
 * This example demonstrates how to use ADC in sequence mode with software trigger.
 * The conversion result is printed in the interrupt handler.
 * 
 * Connection:
 * - None
 */

#include "abov_config.h"
#include "abov_simpleapp_config.h"

#include "hal_adc.h"
#include "hal_pcu.h"

#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"

#if (CONFIG_APP_ADC == 1)

#define ADC_SEQ_CNT             8
#define ADC_RBUF_SIZE           8

static ADC_Context_t s_tADCContext[CONFIG_ADC_MAX_COUNT];
static ADC_SEQ_DATA_t s_tSeqData[ADC_RBUF_SIZE];
static uint32_t s_un32RCnt = 0;

static void PRV_ADC_IRQHandler(uint32_t un32Event, void *pContext)
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    ADC_Context_t *ptContext = (ADC_Context_t *)pContext;

    if(un32Event & ADC_EVENT_BURST_CAPTURED)
    {
        for(int i = 0; i < ADC_SEQ_CNT; i++)
        {
            eErr = HAL_ADC_GetData(ptContext->eId, i, &s_tSeqData[s_un32RCnt++]);
            if(eErr != HAL_ERR_OK)
            {
                HAL_ADC_Stop(ptContext->eId);
            }
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

void ADC_INIT_Sequential(void)
{
    HAL_ERR_e eErr = HAL_ERR_OK;

    LOG("ADC sequential mode with software trigger example\n");

    /* Initialize Port */
    ADC_CLK_CFG_t tAdcClkCfg =
    {
        .eClk = ADC_CLK_PCLK,
        .un8PClkDiv = 64
    };

    ADC_CFG_t tAdcCfg =
    {
        .eMode = ADC_MODE_SEQ,
        .eBaseTrgSrc = ADC_TRG_SRC_ADST,
        .un8SeqCnt = (ADC_SEQ_CNT - 1),
        .un8SamplingTime = 0
    };

    ADC_SEQ_TRG_CFG_t tAdcSeqTrgCfg =
    {
        .eType = ADC_TRG_TYPE_INDEPENDENT,
        .eTrgSrc = ADC_TRG_SRC_ADST,
        .un8TrgNum = 0,
        .utCfg.tInd.un8SeqNum = 0,
        .utCfg.tInd.un8ChNum = 0,
    };

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

    for (int i = 0 ; i < ADC_SEQ_CNT ; i++)
    {
        tAdcSeqTrgCfg.utCfg.tInd.un8SeqNum = i;
        tAdcSeqTrgCfg.utCfg.tInd.un8ChNum = 7 * (i / 4 + 1) + i % 4;

        eErr = HAL_ADC_SetSeqConfig(ADC_ID_0, &tAdcSeqTrgCfg);
        if (eErr != HAL_ERR_OK)
        {
            LOG("HAL_ADC_SetSeqConfig() error, (%d)\n", eErr);
            return;
        }
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

    for(int i = 0 ; i < ADC_RBUF_SIZE ; i++)
    {
        LOG("[%s][%2d:%4d]\t[Ch=%d]\t[TrgInfo=%x]\n",
            s_tSeqData[i].bReadDDR == true ? "DDR" : "DRx",
            i, s_tSeqData[i].un16Result, s_tSeqData[i].un8ChInfo, s_tSeqData[i].un8TrgInfo);
    }
}
#endif
