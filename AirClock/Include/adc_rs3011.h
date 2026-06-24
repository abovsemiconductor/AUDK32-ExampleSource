/**
 *******************************************************************************
 * @file        adc_rs3011.h
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

#ifndef _ADC_H_
#define _ADC_H_

#ifdef __cplusplus
extern "C"
{
#endif

/**
 *******************************************************************************
 * @brief       ADC Event Callback Function Type.
 * @param[in]   un32Event : zero.
 * @param[in]   *pContext : converted data.
 * @return      void : None
 ******************************************************************************/
typedef void (*pfnADC_EVT_Handler_t)(uint32_t un32Event, void *pContext);

/**
 *******************************************************************************
 * @brief       Get ADC data.
 * @param[out]  *pun16Data : the current ADC value.
 * @return      void : None
 ******************************************************************************/
void EX_ADC_GetData(uint16_t *pun16Data);

/**
 *******************************************************************************
 * @brief       Initialize ADC.
 * param[in]    pfnAdcHandler : Callback Function to receive event. 
 * @return      void : None
 ******************************************************************************/
void EX_ADC_Init(pfnADC_EVT_Handler_t pfnAdcHandler);

#ifdef __cplusplus
}
#endif

#endif /* _ADC_H_ */
