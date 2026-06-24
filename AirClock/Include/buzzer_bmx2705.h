/**
 *******************************************************************************
 * @file        buzzer_bmx2705.h
 * @author      ABOV R&D Division
 * @brief       EXAMPLE BMX2705 Buzzer Driver Interface
 *
 * Copyright 2025 ABOV Semiconductor Co.,Ltd. All rights reserved.
 *
 * This file is licensed under terms that are found in the LICENSE file
 * located at Document directory.
 * If this file is delivered or shared without applicable license terms,
 * the terms of the BSD-3-Clause license shall be applied.
 * Reference: https://opensource.org/licenses/BSD-3-Clause
 ******************************************************************************/

#ifndef _BMX2705_BUZZER_H_
#define _BMX2705_BUZZER_H_

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum { 
    BUZZER_NABI = 0,
    BUZZER_BEEP_ONCE,
    BUZZER_BEEP_TWICE,
    BUZZER_MAX,
} BUZZER_t;

/**
 *******************************************************************************
 * @brief       Initialize Buzzer.
 * @return      void : None
 ******************************************************************************/
void EX_BUZZER_Init(void);

/**
 *******************************************************************************
 * @brief       Start Buzzer.
 * @return      void : None
 ******************************************************************************/
void EX_BUZZER_Start(BUZZER_t buzzer);

/**
 *******************************************************************************
 * @brief       Stop Buzzer.
 * @return      void : None
 ******************************************************************************/
void EX_BUZZER_Stop(void);

/**
 *******************************************************************************
 * @brief       Set Buzzer volume.
 * @param[in]   un8Vol : Buzzer volume (1 ~ 40)
 * @return      void : None
 ******************************************************************************/
void EX_BUZZER_SetVolume(uint8_t un8Vol);

#ifdef __cplusplus
}
#endif

#endif /* _BMX2703_BUZZER_H_ */
