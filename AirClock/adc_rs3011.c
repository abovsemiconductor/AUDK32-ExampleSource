/**
 *******************************************************************************
 * @file        adc_rs3011.c
 * @author      ABOV R&D Division
 * @brief       EXAMPLE RS3011 ADC Driver Interface
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

#include "hal_pcu.h"
#include "hal_adc.h"
#include "hal_timer1.h"

#include "adc_rs3011.h"
#include "buzzer_bmx2705.h"

static uint16_t s_un16AdcData = 0;
static pfnADC_EVT_Handler_t s_pfnAdcEvt = NULL;

static void PRV_EX_ADC_IRQHandler(uint32_t un32Event, void *pContext)
{
    ADC_SEQ_DATA_t tData;
    uint16_t un16Data;
	
    tData.bReadDDR = false;
    HAL_ADC_GetData(ADC_ID_0, 0, &tData);
    //un16Data = tData.un16Result / 41;
	un16Data = (tData.un16Result / 102);
    if (un16Data > 40) un16Data = 40;
    
    //LOG("%d\n", un16Data);
    if (s_un16AdcData != un16Data)
    {
        s_un16AdcData = un16Data;
        if (s_pfnAdcEvt != NULL)
        {
            s_pfnAdcEvt(0, &un16Data);
        }
    }
}

void EX_ADC_GetData(uint16_t *pun16Data)
{
    *pun16Data = s_un16AdcData;   
}

void EX_ADC_Init(pfnADC_EVT_Handler_t pfnAdcHandler)
{
    ADC_CLK_CFG_t tAdcClkCfg = {
        .eClk = ADC_CLK_PCLK,
        .un8PClkDiv = 63                      /* (un8PClkDiv + 1) 32MHz / 64 = 500KHz */
    };

    ADC_CFG_t tAdcCfg = {
        .eMode = ADC_MODE_SINGLE,             /* Only Single Mode */
        .eBaseTrgSrc = ADC_TRG_SRC_TIMER1,    /* Timer1 Event for sampling */
        .eRef = ADC_REF_INT,                  /* Internal Reference */
        .un8SeqCnt = 0,                       /* Only Single buffer  */
        .bAutoRestart = true,
    };

    ADC_SEQ_TRG_CFG_t tAdcSeqCfg = {
        .eType = ADC_TRG_TYPE_INDEPENDENT,
        .eTrgSrc = ADC_TRG_SRC_TIMER1,        /* Timer1 Event for sampling */
        .un8TrgNum = TIMER1_ID_9,             /* Channel Number 9 of Timer1 */

        .utCfg.tInd.un8ChNum = 17,            /* Input Analog Channel 17 */
        .utCfg.tInd.un8SeqNum = 0,            /* Sequence Number 0 (single buffer) */
    };

    /*
    PCLK : 32MHz
    CKSEL : 64
    PRS : 999
    TMCLK = PCLK / CKSEL = 500 Hz
    TCLK = TMCLK / (PRS + 1) = 0.5 Hz
    */
    TIMER1_CLK_CFG_t tTimerClkCfg = {
        .eClk = TIMER1_CLK_PCLK,
        .uSubClk.ePClkDiv = TIMER1_PCLK_DIV_64,
        .un16PreScale = 999
    };

    TIMER1_CFG_t tTimerCfg = {
        .eMode = TIMER1_MODE_PERIODIC,
        .bIntrEnable = true,
        .utData.tGRD.un16DataA = 150, //300ms
        .utData.tGRD.un16DataB = 150 //300ms
    };
		
    TIMER1_ADCTRG_CFG_t tTimerADCCfg = {
        .bEnable = true,
    };

    if (pfnAdcHandler != NULL)
    {
        s_pfnAdcEvt = pfnAdcHandler;
    }

    HAL_PCU_SetAltMode(PCU_ID_E, PCU_PIN_ID_3, PCU_ALT_7); /* Analog Input Channel 17 */
    HAL_PCU_SetInOutMode(PCU_ID_E, PCU_PIN_ID_3, PCU_INOUT_ANG_INPUT);

    if (HAL_ADC_Uninit(ADC_ID_0) != HAL_ERR_OK)
        LOG("HAL_ADC_Uninit error.\n");

    if (HAL_ADC_Init(ADC_ID_0) != HAL_ERR_OK)
        LOG("HAL_ADC_Init error.\n");

    if (HAL_ADC_SetClkConfig(ADC_ID_0, &tAdcClkCfg) != HAL_ERR_OK)
        LOG("HAL_ADC_SetClkConfig error.\n");

    if (HAL_ADC_SetConfig(ADC_ID_0, &tAdcCfg) != HAL_ERR_OK)
        LOG("HAL_ADC_SetConfig error.\n");

    if (HAL_ADC_SetSeqConfig(ADC_ID_0, &tAdcSeqCfg) != HAL_ERR_OK)
        LOG("HAL_ADC_SetSeqConfig error.\n");

    if (HAL_ADC_SetIRQ(ADC_ID_0, ADC_OPS_INTR, PRV_EX_ADC_IRQHandler, NULL, 3) != HAL_ERR_OK)
        LOG("HAL_ADC_SetIRQ error.\n");    

    if (HAL_ADC_Start(ADC_ID_0) != HAL_ERR_OK)
        LOG("HAL_ADC_Start error.\n");


    /* Timer1 channel 9 for ADC Trigger */
    if (HAL_TIMER1_Init(TIMER1_ID_9) != HAL_ERR_OK)
        LOG("HAL_TIMER1_Init error.\n");

    if (HAL_TIMER1_SetClkConfig(TIMER1_ID_9, &tTimerClkCfg) != HAL_ERR_OK)
        LOG("HAL_TIMER1_SetClkConfig error.\n");

    if (HAL_TIMER1_SetConfig(TIMER1_ID_9, &tTimerCfg) != HAL_ERR_OK)
        LOG("HAL_TIMER1_SetConfig error.\n");

    if (HAL_TIMER1_SetAdcTrgConfig(TIMER1_ID_9, &tTimerADCCfg) != HAL_ERR_OK)
        LOG("HAL_TIMER1_SetAdcTrgConfig error.\n");

    if (HAL_TIMER1_SetIRQ(TIMER1_ID_9, TIMER1_OPS_INTR, NULL, NULL, 3) != HAL_ERR_OK)
        LOG("HAL_TIMER1_SetIRQ error.\n");

    if (HAL_TIMER1_Start(TIMER1_ID_9) != HAL_ERR_OK)
        LOG("HAL_TIMER1_Start error.\n");

    LOG("EX_ADC_Init (%d)\n", HAL_ADC_GetMaxResolution());
}
