/**
 *******************************************************************************
 * @file        buzzer_bmx2705.c
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
#include "abov_config.h"
#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"

#include "hal_scu.h"
#include "hal_scu_clk.h"
#include "hal_pcu.h"
#include "hal_timer1.h"
#include "hal_timer3.h"
#include "buzzer_bmx2705.h"

#define NOTE_PLAY_50MSEC            25
#define NOTE_PLAY_QRTR_SEC          125
#define NOTE_PLAY_HALF_SEC          250
#define NOTE_PLAY_ONE_SEC           500
#define TCLK_HZ                     100000	//100 kHz

typedef struct {
    const char *name;
    float f_hz;        // target freq
    uint16_t playtime;
} NoteCfg_t;

static uint32_t s_un32NoteCnt = 0;
static uint8_t s_un8BuzzerVol = 1;
static BUZZER_t s_buzzerType = 0;
static const NoteCfg_t *bell = NULL;
static uint8_t s_bellLength = 0;

/*
    { "C", 261.63f },
    { "D", 293.66f },
    { "E", 329.63f },
    { "F", 349.23f },
    { "G", 392.00f },
    { "A", 440.00f },
    { "B", 493.88f },
    { "C", 523.25f },
*/
static const NoteCfg_t s_Bell_NABI[] = 
{
    // 1
    { "G", 392.00f, NOTE_PLAY_HALF_SEC },
    { "E", 329.63f, NOTE_PLAY_HALF_SEC },
    { "E", 329.63f, NOTE_PLAY_ONE_SEC },
    { NULL, 0, NOTE_PLAY_QRTR_SEC },
        
    { "F", 349.23f, NOTE_PLAY_HALF_SEC },
    { "D", 293.66f, NOTE_PLAY_HALF_SEC },
    { "D", 293.66f, NOTE_PLAY_ONE_SEC },
    { NULL, 0, NOTE_PLAY_QRTR_SEC },

    { "C", 261.63f, NOTE_PLAY_HALF_SEC },
    { "D", 293.66f, NOTE_PLAY_HALF_SEC },
    { "E", 329.63f, NOTE_PLAY_HALF_SEC },
    { "F", 349.23f, NOTE_PLAY_HALF_SEC },
        
    { "G", 392.00f, NOTE_PLAY_HALF_SEC },
    { "G", 392.00f, NOTE_PLAY_HALF_SEC },
    { "G", 392.00f, NOTE_PLAY_ONE_SEC },
    { NULL, 0, NOTE_PLAY_QRTR_SEC },
        
    // 2
    { "G", 392.00f, NOTE_PLAY_HALF_SEC },
    { "E", 329.63f, NOTE_PLAY_HALF_SEC },
    { "E", 329.63f, NOTE_PLAY_HALF_SEC },
    { "E", 329.63f, NOTE_PLAY_HALF_SEC },
    { NULL, 0, NOTE_PLAY_QRTR_SEC },

    { "F", 349.23f, NOTE_PLAY_HALF_SEC },
    { "D", 293.66f, NOTE_PLAY_HALF_SEC },
    { "D", 293.66f, NOTE_PLAY_ONE_SEC },
    { NULL, 0, NOTE_PLAY_QRTR_SEC },

    { "C", 261.63f, NOTE_PLAY_HALF_SEC },
    { "E", 329.63f, NOTE_PLAY_HALF_SEC },
    { "G", 392.00f, NOTE_PLAY_HALF_SEC },
    { "G", 392.00f, NOTE_PLAY_HALF_SEC },
        
    { "E", 329.63f, NOTE_PLAY_HALF_SEC },
    { "E", 329.63f, NOTE_PLAY_HALF_SEC },
    { "E", 329.63f, NOTE_PLAY_ONE_SEC },
    { NULL, 0, NOTE_PLAY_QRTR_SEC },

    // 3
    { "D", 293.66f, NOTE_PLAY_HALF_SEC },
    { "D", 293.66f, NOTE_PLAY_HALF_SEC },
    { "D", 293.66f, NOTE_PLAY_HALF_SEC },
    { "D", 293.66f, NOTE_PLAY_HALF_SEC },
    { NULL, 0, NOTE_PLAY_QRTR_SEC },

    { "D", 293.66f, NOTE_PLAY_HALF_SEC },
    { "E", 329.63f, NOTE_PLAY_HALF_SEC },
    { "F", 349.23f, NOTE_PLAY_ONE_SEC },
    { NULL, 0, NOTE_PLAY_QRTR_SEC },

    { "E", 329.63f, NOTE_PLAY_HALF_SEC },
    { "E", 329.63f, NOTE_PLAY_HALF_SEC },
    { "E", 329.63f, NOTE_PLAY_HALF_SEC },
    { "E", 329.63f, NOTE_PLAY_HALF_SEC },

    { "E", 329.63f, NOTE_PLAY_HALF_SEC },
    { "F", 349.23f, NOTE_PLAY_HALF_SEC },
    { "G", 392.00f, NOTE_PLAY_ONE_SEC },
    { NULL, 0, NOTE_PLAY_QRTR_SEC },

    // 4
    { "G", 392.00f, NOTE_PLAY_HALF_SEC },
    { "E", 329.63f, NOTE_PLAY_HALF_SEC },
    { "E", 329.63f, NOTE_PLAY_ONE_SEC },
    { NULL, 0, NOTE_PLAY_QRTR_SEC },

    { "F", 349.23f, NOTE_PLAY_HALF_SEC },
    { "D", 293.66f, NOTE_PLAY_HALF_SEC },
    { "D", 293.66f, NOTE_PLAY_ONE_SEC },
    { NULL, 0, NOTE_PLAY_QRTR_SEC },

    { "C", 261.63f, NOTE_PLAY_HALF_SEC },
    { "E", 329.63f, NOTE_PLAY_HALF_SEC },
    { "G", 392.00f, NOTE_PLAY_HALF_SEC },
    { "G", 392.00f, NOTE_PLAY_HALF_SEC },

    { "E", 329.63f, NOTE_PLAY_HALF_SEC },
    { "E", 329.63f, NOTE_PLAY_HALF_SEC },
    { "E", 329.63f, NOTE_PLAY_ONE_SEC },
};

static const NoteCfg_t s_Bell_BEEP_ONCE[] = 
{
    { "G", 392.00f, NOTE_PLAY_50MSEC },
};

static const NoteCfg_t s_Bell_BEEP_TWICE[] = 
{
    { "C", 523.25f, NOTE_PLAY_50MSEC },
    { NULL, 0, NOTE_PLAY_50MSEC },
    { "B", 493.88f, NOTE_PLAY_50MSEC },
};

#define BELL_NABI_TOTAL sizeof(s_Bell_NABI) / sizeof(NoteCfg_t)
#define BEEP_ONCE_TOTAL sizeof(s_Bell_BEEP_ONCE) / sizeof(NoteCfg_t)
#define BEEP_TWICE_TOTAL sizeof(s_Bell_BEEP_TWICE) / sizeof(NoteCfg_t)

static void PRV_BUZZER_WAIT_IRQHandler(uint32_t un32Event, void *pContext)
{
    HAL_TIMER1_Stop(TIMER1_ID_4);
    HAL_TIMER1_Stop(TIMER1_ID_5);

    s_un32NoteCnt++;
    if (s_un32NoteCnt < s_bellLength)
    {
        if (bell[s_un32NoteCnt].name != NULL)
        {
            uint32_t grb = TCLK_HZ / bell[s_un32NoteCnt].f_hz;

            //LOG("playing %s=%dHz, vol=%d, playtime=%d\n", bell[s_un32NoteCnt].name, grb, 1, bell[s_un32NoteCnt].playtime);
            HAL_TIMER1_SetData(TIMER1_ID_4, TIMER1_DATA_A, s_un8BuzzerVol);
            HAL_TIMER1_SetData(TIMER1_ID_4, TIMER1_DATA_B, grb);
            HAL_TIMER1_Start(TIMER1_ID_4);
        }

        HAL_TIMER1_SetData(TIMER1_ID_5, TIMER1_DATA_A, bell[s_un32NoteCnt].playtime);
        HAL_TIMER1_SetData(TIMER1_ID_5, TIMER1_DATA_B, bell[s_un32NoteCnt].playtime);
        HAL_TIMER1_Start(TIMER1_ID_5);
    }
}

void EX_BUZZER_Start(BUZZER_t buzzer)
{
    //LOG("EX_BUZZER_Start\n");
    if (buzzer >= BUZZER_MAX)
    {
        return;
    }
    s_buzzerType = buzzer;
    switch(s_buzzerType)
    {
        case BUZZER_NABI:
            bell = s_Bell_NABI;
            s_bellLength = BELL_NABI_TOTAL;
            break;
        case BUZZER_BEEP_ONCE:
            bell = s_Bell_BEEP_ONCE;
            s_bellLength = BEEP_ONCE_TOTAL;
            break;
        case BUZZER_BEEP_TWICE:
            bell = s_Bell_BEEP_TWICE;
            s_bellLength = BEEP_TWICE_TOTAL;
            break;
        default:
            return;
    }

    s_un32NoteCnt = 0;
    uint32_t grb = TCLK_HZ / bell[s_un32NoteCnt].f_hz;

    //LOG("playing %s=%dHz, vol=%d, playtime=%d\n", bell[s_un32NoteCnt].name, grb, 1, bell[s_un32NoteCnt].playtime);
    HAL_TIMER1_SetData(TIMER1_ID_4, TIMER1_DATA_A, s_un8BuzzerVol);
    HAL_TIMER1_SetData(TIMER1_ID_4, TIMER1_DATA_B, grb);

    HAL_TIMER1_SetData(TIMER1_ID_5, TIMER1_DATA_A, bell[s_un32NoteCnt].playtime);
    HAL_TIMER1_SetData(TIMER1_ID_5, TIMER1_DATA_B, bell[s_un32NoteCnt].playtime);

    HAL_TIMER1_Start(TIMER1_ID_4);
    HAL_TIMER1_Start(TIMER1_ID_5);
}

void EX_BUZZER_Stop(void)
{
    //LOG("EX_BUZZER_Stop\n");
    HAL_TIMER1_Stop(TIMER1_ID_4);
    HAL_TIMER1_Stop(TIMER1_ID_5);
    s_un32NoteCnt = 0;
}

void EX_BUZZER_SetVolume(uint8_t un8Vol)
{
    s_un8BuzzerVol = un8Vol;
}

void EX_BUZZER_Init(void)
{
    /*
        to play note.
        PCLK : 32MHz
        CKSEL : 16
        PRS : 19
        TMCLK = PCLK / CKSEL = 2 MHz
        TCLK = TMCLK / (PRS + 1) = 100 kHz
    */
    TIMER1_CLK_CFG_t tBuzzerClkCfg = {
        .eClk = TIMER1_CLK_PCLK,
        .uSubClk.ePClkDiv = TIMER1_PCLK_DIV_16,
        .un16PreScale = 19
    };

    TIMER1_CFG_t tBuzzerCfg = {
        .eMode = TIMER1_MODE_PWM,
        .bIntrEnable = true,
        .utData.tGRD.un16DataA = s_un8BuzzerVol,
        .utData.tGRD.un16DataB = (TCLK_HZ / bell[0].f_hz)
    };

    /*
        to set playtime.
        PCLK : 32MHz
        CKSEL : 64
        PRS : 999
        TMCLK = PCLK / CKSEL = 500 Hz
        TCLK = TMCLK / (PRS + 1) = 0.5 Hz
    */
    TIMER1_CLK_CFG_t tWaitClkCfg = {
        .eClk = TIMER1_CLK_PCLK,
        .uSubClk.ePClkDiv = TIMER1_PCLK_DIV_64,
        .un16PreScale = 999
    };

    TIMER1_CFG_t tWaitCfg = {
        .eMode = TIMER1_MODE_PERIODIC,
        .bIntrEnable = true,
        .utData.tGRD.un16DataA = 500,
        .utData.tGRD.un16DataB = 500 
    };
    
    HAL_PCU_SetAltMode(PCU_ID_E, PCU_PIN_ID_5, PCU_ALT_2);

    /* Timer1 channel 4 for buzzer */
    HAL_TIMER1_Init(TIMER1_ID_4);
    HAL_TIMER1_SetClkConfig(TIMER1_ID_4, &tBuzzerClkCfg);
    HAL_TIMER1_SetConfig(TIMER1_ID_4, &tBuzzerCfg);

    /* Timer1 channel 5 for wait */
    HAL_TIMER1_Init(TIMER1_ID_5);
    HAL_TIMER1_SetClkConfig(TIMER1_ID_5, &tWaitClkCfg);
    HAL_TIMER1_SetConfig(TIMER1_ID_5, &tWaitCfg);
    HAL_TIMER1_SetIRQ(TIMER1_ID_5, TIMER1_OPS_INTR, PRV_BUZZER_WAIT_IRQHandler, NULL, 3);

    LOG("EX_BUZZER_Init\n");
}
