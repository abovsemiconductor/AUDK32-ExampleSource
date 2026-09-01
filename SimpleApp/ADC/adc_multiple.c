/**
 *******************************************************************************
 * @file        adc_multiple.c
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
 * This example demonstrates how to use ADC in multiple mode with software trigger.
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

typedef struct
{
    bool bValid;
    bool bReadData;
    uint8_t un8ChNum;
} EX_ADC_GROUP_t;

static ADC_Context_t s_tADCContext;
static ADC_SEQ_DATA_t s_tSeqData[ADC_RBUF_SIZE];
static uint32_t s_un32RCnt = 0;

static EX_ADC_GROUP_t s_tTCAdcGroup[ADC_SEQ_CNT];

static void PRV_ADC_IRQHandler(uint32_t un32Event, void *pContext)
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    ADC_Context_t *ptContext = (ADC_Context_t *)pContext;

    if (un32Event & ADC_EVENT_SINGLE_CAPTURED)
    {
        for(int i = 0 ; i < ADC_SEQ_CNT ; i++)
        {
            if(s_tTCAdcGroup[i].bReadData)
            {
                continue;
            }

            s_tSeqData[i].bReadDDR = false;
            eErr = HAL_ADC_GetData(ptContext->eId, i, &s_tSeqData[i]);
            if (eErr != HAL_ERR_OK)
            {
                HAL_ADC_Stop(ptContext->eId);
            }
            else
            {
                if(s_tTCAdcGroup[i].bValid && (s_tTCAdcGroup[i].un8ChNum == s_tSeqData[i].un8ChInfo))
                {
                    s_tTCAdcGroup[i].bReadData = true;
                    s_un32RCnt++;
                }
            }
        }
    }

    if (s_un32RCnt < ADC_RBUF_SIZE)
    {
        ;
    }
    else
    {
        HAL_ADC_Stop(ptContext->eId);
    }
}

void ADC_INIT_Multiple(void)
{
    HAL_ERR_e eErr = HAL_ERR_OK;

    LOG("ADC multiple mode with software trigger example\n");

    /* Initialize Port */
    ADC_CLK_CFG_t tAdcClkCfg =
    {
        .eClk = ADC_CLK_PCLK,
        .un8PClkDiv = 64
    };

    ADC_CFG_t tAdcCfg =
    {
        .eMode = ADC_MODE_MULTIPLE,
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

    s_tADCContext.eId = ADC_ID_0;
    eErr = HAL_ADC_SetIRQ(ADC_ID_0, ADC_OPS_INTR, PRV_ADC_IRQHandler, &s_tADCContext, 3);
    if (eErr != HAL_ERR_OK)
    {
        LOG("HAL_ADC_SetIRQ() error, (%d)\n", eErr);
        return;
    }

    for (int i = 0 ; i < ADC_SEQ_CNT ; i++)
    {
        tAdcSeqTrgCfg.utCfg.tInd.un8SeqNum = i;
        tAdcSeqTrgCfg.utCfg.tInd.un8ChNum = i == 0 ? ADC0_IN_CHANNEL_NUM : i;

        eErr = HAL_ADC_SetSeqConfig(ADC_ID_0, &tAdcSeqTrgCfg);
        if (eErr != HAL_ERR_OK)
        {
            LOG("HAL_ADC_SetSeqConfig() error, (%d)\n", eErr);
            return;
        }

        s_tTCAdcGroup[i].bValid = true;
        s_tTCAdcGroup[i].un8ChNum = tAdcSeqTrgCfg.utCfg.tInd.un8ChNum;
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
