/**
 *******************************************************************************
 * @file        timer1.c
 * @author      ABOV R&D Division
 * @brief       Timer1 Example Code
 *
 * Copyright 2022 ABOV Semiconductor Co.,Ltd. All rights reserved.
 *
 * This file is licensed under terms that are found in the LICENSE file
 * located at Document directory.
 * If this file is delivered or shared without applicable license terms,
 * the terms of the BSD-3-Clause license shall be applied.
 * Reference: https://opensource.org/licenses/BSD-3-Clause
 ******************************************************************************/

#include "abov_example_config.h"

#if defined(TIMER1_TC)
#include "abov_config.h"
#include "ex_configs.h"
#include "ex_common.h"
#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "hal_scu.h"
#include "hal_scu_clk.h"
#include "hal_pcu.h"
#include "hal_timer1.h"

#if !defined(_TIMER1)
#error EX_COMM_MODULE_ERR_STR
#endif

#define EX_TIMER1_STR "TIMER1"
#define EX_TIMER1_LOG_STR "TIMER1 :"
#define EX_TIMER1_ERR_STR "[E]TIMER1 :"
#define EX_TIMER1_IRQ_PRIO 3
#define EX_TIMER1_MAX_ARG  7
#define EX_TIMER1_MAX_NUM  (CONFIG_TIMER1_MAX_COUNT - 1)

extern const char *pcCommStr[];
extern const char *pcCmdStr[];
extern const char *pcValStr[];
extern const char *pcOptStr[];

static TIMER1_Context_t s_tTimer1Context[CONFIG_TIMER1_MAX_COUNT];
static uint32_t un32ISRLog = 0;

static enum debug_cmd_status EX_TIMER1_Help(int32_t n32Argc, char *pn8Argv[])
{
    uint32_t un32ShowOpt = 0;
    EX_COMM_STR_OPT_e eOpt[7];

    EX_COMMON_SetShowModuleInfo(EX_TIMER1_STR, CONFIG_TIMER1_MAX_COUNT);

    un32ShowOpt = EX_COMMON_GetShowOpt(pn8Argv[1]);

    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_INIT, NULL, EX_TIMER1_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_UNINIT, NULL, EX_TIMER1_MAX_NUM, eOpt, 0, NULL);

    eOpt[0] = EX_COMM_STR_OPT_SRC; 
    eOpt[1] = EX_COMM_STR_OPT_DIV; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CLK, NULL, EX_TIMER1_MAX_NUM, eOpt, 2, NULL);
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_SRC, EX_COMM_STR_VAL_CLKPATH, "/e(ext)");
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "pclk: [div]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_DIV, EX_COMM_STR_VAL_MAX, "0,2,4,16,64");
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MCCR, EX_COMM_STR_VAL_MAX, "[mccr] [div]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MCCR, EX_COMM_STR_VAL_CLKSRC, "/m(mclk)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_DIV, EX_COMM_STR_VAL_N_255, NULL);
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "ext: [edge]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_EDGE, EX_COMM_STR_VAL_EDGE, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_DIV, EX_COMM_STR_VAL_N_NUM, NULL);
    }

    eOpt[0] = EX_COMM_STR_OPT_OPS; 
    eOpt[1] = EX_COMM_STR_OPT_MODE; 
    eOpt[2] = EX_COMM_STR_OPT_POL; 
    eOpt[3] = EX_COMM_STR_OPT_DA;
    eOpt[4] = EX_COMM_STR_OPT_DB;
    eOpt[5] = EX_COMM_STR_OPT_DASH_IO;
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CONFIG, NULL, EX_TIMER1_MAX_NUM, eOpt, 6, NULL);
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_OPS, EX_COMM_STR_VAL_OPS, "/n(nmi)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MODE, EX_COMM_STR_VAL_MAX, "p(periodic)/c(capture)/o(oneshot)/m(pwm)");
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "capture: [edge]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_EDGE, EX_COMM_STR_VAL_EDGE, "/b(both)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_POL, EX_COMM_STR_VAL_LOWHIGH, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_DA, EX_COMM_STR_VAL_N_NUM, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_DB, EX_COMM_STR_VAL_N_NUM, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_DASH_IO, EX_COMM_STR_VAL_MAX, "i(in)/o(out)");
    }

    eOpt[0] = EX_COMM_STR_OPT_ENA; 
    eOpt[1] = EX_COMM_STR_OPT_MODE; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "adctrg", EX_TIMER1_MAX_NUM, eOpt, 2, NULL);
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_ENA, EX_COMM_STR_VAL_ENDIS, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MODE, EX_COMM_STR_VAL_MAX, "n(normal)/p(trig)/b(both)");
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "p or b: [val]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "val: 0~N(16bit dec) - trg cnt");
    }

    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_START, NULL, EX_TIMER1_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_STOP, NULL, EX_TIMER1_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "pause", EX_TIMER1_MAX_NUM, eOpt, 0, (char *)pcValStr[EX_COMM_STR_VAL_ONOFF]);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_LOG, NULL, -1, eOpt, 0, (char *)pcValStr[EX_COMM_STR_VAL_ONOFF]);
    LOG("\n");

    return DEBUG_CMD_SUCCESS;
}

static void EX_TIMER1_IRQHandler(uint32_t un32Event, void *pContext)
{
    TIMER1_Context_t *ptContext = (TIMER1_Context_t *)pContext;
    uint32_t un32Data;

    if(un32Event & TIMER1_EVENT_OVERFLOW)
    {
        if(un32ISRLog == 1)
        {
            LOG("%s (%d) OVF %s\n", EX_TIMER1_LOG_STR, ptContext->eId, pcCommStr[EX_COMM_STR_EVT_FIRE]);
        }
    }

    if(un32Event & TIMER1_EVENT_PERIODIC_MATCH)
    {
        if(un32ISRLog == 1)
        {
            LOG("%s (%d) P %s\n", EX_TIMER1_LOG_STR, ptContext->eId, pcCommStr[EX_COMM_STR_EVT_FIRE]);
        }
    }

    if(un32Event & TIMER1_EVENT_PWM_DUTY)
    {
        if(un32ISRLog == 1)
        {
            LOG("%s (%d) PWM D %s\n", EX_TIMER1_LOG_STR, ptContext->eId, pcCommStr[EX_COMM_STR_EVT_FIRE]);
        }
    }

    if(un32Event & TIMER1_EVENT_PWM_PERIOD)
    {
        if(un32ISRLog == 1)
        {
            LOG("%s (%d) PWM P %s\n", EX_TIMER1_LOG_STR, ptContext->eId, pcCommStr[EX_COMM_STR_EVT_FIRE]);
        }
    }

    if(un32Event & TIMER1_EVENT_CAPTURE)
    {
        HAL_TIMER1_GetData(ptContext->eId, TIMER1_DATA_CAP_A, &un32Data);
        if(un32ISRLog == 1)
        {
            LOG("%s (%d) C-A %s(Data=%d)\n", EX_TIMER1_LOG_STR, ptContext->eId, pcCommStr[EX_COMM_STR_EVT_FIRE], un32Data);
        }
    }

    if(un32Event & TIMER1_EVENT_CAPTURE_B)
    {
        HAL_TIMER1_GetData(ptContext->eId, TIMER1_DATA_CAP_B, &un32Data);
        if(un32ISRLog == 1)
        {
            LOG("%s (%d) C-B %s(Data=%d)\n", EX_TIMER1_LOG_STR, ptContext->eId, pcCommStr[EX_COMM_STR_EVT_FIRE], un32Data);
        }
    }
}

static enum debug_cmd_status EX_TIMER1_GetClkConfig(int32_t n32Argc, char *pn8Argv[], TIMER1_CLK_CFG_t *ptClkCfg)
{
    uint8_t un8Arg = 2, un8Data = 0;

    if (pn8Argv[un8Arg][0] == 'p')
        ptClkCfg->eClk = TIMER1_CLK_PCLK;
    else if (pn8Argv[un8Arg][0] == 'm')
        ptClkCfg->eClk = TIMER1_CLK_MCCR;
    else if (pn8Argv[un8Arg][0] == 'e')
        ptClkCfg->eClk = TIMER1_CLK_EXT;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_TIMER1_ERR_STR, NULL, EX_COMM_STR_OPT_SRC);
        goto err;
    }

    un8Arg++;
    if(ptClkCfg->eClk == TIMER1_CLK_PCLK)
    {
        un8Data = atoi(pn8Argv[un8Arg++]);

        if (un8Data == 0)
            ptClkCfg->uSubClk.ePClkDiv = TIMER1_PCLK_DIV_MAX;
        else if (un8Data == 2)
            ptClkCfg->uSubClk.ePClkDiv = TIMER1_PCLK_DIV_2;
        else if (un8Data == 4)
            ptClkCfg->uSubClk.ePClkDiv = TIMER1_PCLK_DIV_4;
        else if (un8Data == 16)
            ptClkCfg->uSubClk.ePClkDiv = TIMER1_PCLK_DIV_16;
        else if (un8Data == 64)
            ptClkCfg->uSubClk.ePClkDiv = TIMER1_PCLK_DIV_64;
        else
        {
            EX_COMMON_SetShowModuleLog(EX_TIMER1_ERR_STR, NULL, EX_COMM_STR_OPT_DIV);
            goto err;
        }
    }
    else if(ptClkCfg->eClk == TIMER1_CLK_MCCR)
    {
        if (pn8Argv[un8Arg][0] == 'l')
            ptClkCfg->uSubClk.eMccr = TIMER1_CLK_MCCR_LSI;
        else if (pn8Argv[un8Arg][0] == 's')
            ptClkCfg->uSubClk.eMccr = TIMER1_CLK_MCCR_LSE;
        else if (pn8Argv[un8Arg][0] == 'm')
            ptClkCfg->uSubClk.eMccr = TIMER1_CLK_MCCR_MCLK;
        else if (pn8Argv[un8Arg][0] == 'h')
            ptClkCfg->uSubClk.eMccr = TIMER1_CLK_MCCR_HSI;
        else if (pn8Argv[un8Arg][0] == 'e')
            ptClkCfg->uSubClk.eMccr = TIMER1_CLK_MCCR_HSE;
        else if (pn8Argv[un8Arg][0] == 'p')
            ptClkCfg->uSubClk.eMccr = TIMER1_CLK_MCCR_PLL;
        else
        {
            EX_COMMON_SetShowModuleLog(EX_TIMER1_ERR_STR, NULL, EX_COMM_STR_OPT_MCCR);
            goto err;
        }

        ptClkCfg->un8MccrDiv = atoi(pn8Argv[++un8Arg]);
        un8Arg++;
    }
    else if(ptClkCfg->eClk == TIMER1_CLK_EXT)
    {
        switch (pn8Argv[un8Arg++][0])
        {
            case 'f':
                ptClkCfg->uSubClk.eExtClkEdge = TIMER1_EXTCLK_EDGE_FALLING;
                break;
            case 'r':
                ptClkCfg->uSubClk.eExtClkEdge = TIMER1_EXTCLK_EDGE_RISING;
                break;
            default:
                EX_COMMON_SetShowModuleLog(EX_TIMER1_ERR_STR, NULL, EX_COMM_STR_OPT_EDGE);
                goto err;
        }
    }

    ptClkCfg->un16PreScale = (uint16_t)atoi(pn8Argv[un8Arg++]);

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_TIMER1_GetConfig(int32_t n32Argc, char *pn8Argv[], TIMER1_ID_e eId, TIMER1_OPS_e *peOps, TIMER1_CFG_t *ptCfg, TIMER1_PORT_e *peIo)
{
    uint8_t un8Arg = 2;

    if (pn8Argv[un8Arg][0] == 'p')
    {
        *peOps = TIMER1_OPS_POLL;
            ptCfg->bIntrEnable = false;
    }
    else if (pn8Argv[un8Arg][0] == 'i')
    {
            *peOps = TIMER1_OPS_INTR;
            ptCfg->bIntrEnable = true;
    }
    else if (pn8Argv[un8Arg][0] == 'n')
    {
            *peOps = TIMER1_OPS_NMI;
            ptCfg->bIntrEnable = true;
    }
    else
    {
        EX_COMMON_SetShowModuleLog(EX_TIMER1_ERR_STR, NULL, EX_COMM_STR_OPT_OPS);
        goto err;
    }

    un8Arg++;
    if (pn8Argv[un8Arg][0] == 'p')
        ptCfg->eMode = TIMER1_MODE_PERIODIC;
    else if (pn8Argv[un8Arg][0] == 'c')
        ptCfg->eMode = TIMER1_MODE_CAPTURE;
    else if (pn8Argv[un8Arg][0] == 'o')
        ptCfg->eMode = TIMER1_MODE_ONESHOT;
    else if (pn8Argv[un8Arg][0] == 'm')
        ptCfg->eMode = TIMER1_MODE_PWM;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_TIMER1_ERR_STR, NULL, EX_COMM_STR_OPT_MODE);
        goto err;
    }

    if(ptCfg->eMode == TIMER1_MODE_CAPTURE)
    {
        un8Arg++;
        if (pn8Argv[un8Arg][0] == 'r')
            ptCfg->eCapClr = TIMER1_CAP_CLR_RISING;
        else if (pn8Argv[un8Arg][0] == 'f')
            ptCfg->eCapClr = TIMER1_CAP_CLR_FALLING;
        else if (pn8Argv[un8Arg][0] == 'b')
            ptCfg->eCapClr = TIMER1_CAP_CLR_BOTH;
        else
        {
            EX_COMMON_SetShowModuleLog(EX_TIMER1_ERR_STR, NULL, EX_COMM_STR_OPT_EDGE);
            goto err;
        }
    }

    un8Arg++;
    switch (pn8Argv[un8Arg++][0])
    {
        case 'h':
            ptCfg->ePol = TIMER1_POL_HIGH;
            break;
        case 'l':
            ptCfg->ePol = TIMER1_POL_LOW;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_TIMER1_ERR_STR, NULL, EX_COMM_STR_OPT_POL);
            goto err;
    } 

    if(ptCfg->eMode == TIMER1_MODE_PWM)
    {
        ptCfg->utData.tPWM.un16Duty = atoi(pn8Argv[un8Arg++]);
        ptCfg->utData.tPWM.un16Period = atoi(pn8Argv[un8Arg++]);
    }
    else
    {
        ptCfg->utData.tGRD.un16DataA = atoi(pn8Argv[un8Arg++]);
        ptCfg->utData.tGRD.un16DataB = atoi(pn8Argv[un8Arg++]);
    }

    if(strncmp(pn8Argv[un8Arg],"-io",3) == 0)
    {
        un8Arg++;
        switch(pn8Argv[un8Arg++][0])
        {
            case 'i':
                *peIo = TIMER1_PORT_IN;
                break;
            case 'o':
                *peIo = TIMER1_PORT_OUT;
                break;
            default:
                *peIo = TIMER1_PORT_OUT;
                break;
        }
    }
    
    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_TIMER1_GetAdcTrgConfig(int32_t n32Argc, char *pn8Argv[], TIMER1_ADCTRG_CFG_t *ptCfg)
{
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;
    uint8_t un8Arg = 2;

    eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], &ptCfg->bEnable);
    if (eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_TIMER1_ERR_STR, NULL, EX_COMM_STR_OPT_ENA);
        goto err;
    }

    if (pn8Argv[un8Arg][0] == 'n')
        ptCfg->eMode = TIMER1_ADCTRG_MODE_NORMAL;
    else if (pn8Argv[un8Arg][0] == 'p')
        ptCfg->eMode = TIMER1_ADCTRG_MODE_POINT;
    else if (pn8Argv[un8Arg][0] == 'b')
        ptCfg->eMode = TIMER1_ADCTRG_MODE_BOTH;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_TIMER1_ERR_STR, NULL, EX_COMM_STR_OPT_MODE);
        goto err;
    }

    if(ptCfg->eMode != TIMER1_ADCTRG_MODE_NORMAL)
    {
        ptCfg->un16TrgPnt = (uint16_t)atoi(pn8Argv[++un8Arg]);
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_TIMER1_GetId(int32_t n32Argc, char *pn8Argv[], TIMER1_ID_e *peId)
{
    uint8_t un8Data = 0;
    if(n32Argc == 1)
    {
        EX_COMMON_SetShowModuleLog(EX_TIMER1_ERR_STR, (char *)pcOptStr[EX_COMM_STR_OPT_ARG], EX_COMM_STR_OPT_MAX);
        return DEBUG_CMD_INVALID;
    }

    un8Data = atoi(pn8Argv[1]); 
    if(un8Data >= CONFIG_TIMER1_MAX_COUNT)
    {
        LOG("%s Max Chan %d\n", EX_TIMER1_ERR_STR, CONFIG_TIMER1_MAX_COUNT);
        return DEBUG_CMD_INVALID;
    }

    *peId = (TIMER1_ID_e)un8Data;

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_TIMER1_Init(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    TIMER1_ID_e eId;

    eDbgStatus = EX_TIMER1_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_TIMER1_Init(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_TIMER1_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_INIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_TIMER1_Uninit(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    TIMER1_ID_e eId;

    eDbgStatus = EX_TIMER1_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_TIMER1_Uninit(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_TIMER1_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_UNINIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_TIMER1_SetClk(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    TIMER1_ID_e eId;

    TIMER1_CLK_CFG_t tClkCfg =
    {
#if (TIMER1_CLKSRC_USE_PCLK == 1)
        .eClk = TIMER1_CLK_PCLK_DIV_16,
        .uSubClk.eMccr = TIMER1_CLK_MCCR_NONE,
        .un8MccrDiv = 0,
#else
        .eClk  = TIMER1_CLK_MCCR,
        .uSubClk.eMccr = TIMER1_CLK_MCCR_HSE,
        .un8MccrDiv = DEFAULT_HSE_1MHZ_DIV,
#endif
    };

    eDbgStatus = EX_TIMER1_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eDbgStatus = EX_TIMER1_GetClkConfig(n32Argc, pn8Argv, &tClkCfg);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_TIMER1_SetClkConfig(eId, &tClkCfg);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_TIMER1_SetConfig(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    TIMER1_ID_e eId;
    TIMER1_OPS_e eOps;
    TIMER1_CFG_t tTimer1Cfg;
    TIMER1_PORT_e eIo = TIMER1_PORT_OUT;

    eDbgStatus = EX_TIMER1_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eDbgStatus = EX_TIMER1_GetConfig(n32Argc, pn8Argv, eId, &eOps, &tTimer1Cfg, &eIo);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_TIMER1_SetConfig(eId, &tTimer1Cfg);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    if(eOps == TIMER1_OPS_INTR || eOps == TIMER1_OPS_NMI)
    {
        eErr = HAL_TIMER1_SetIRQ(eId, eOps, EX_TIMER1_IRQHandler, (void *)&s_tTimer1Context[(uint32_t)eId], EX_TIMER1_IRQ_PRIO);
        if(eErr != HAL_ERR_OK)
        {
            goto err;
        }
    }

    eErr = HAL_TIMER1_SetInOutPort(eId, eIo);
    if(eErr != HAL_ERR_OK)
    {
       EX_COMMON_SetShowModuleLog(EX_TIMER1_LOG_STR, "def port:out", EX_COMM_STR_OPT_MAX);
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_TIMER1_SetAdcTrgConfig(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    TIMER1_ID_e eId;
    TIMER1_ADCTRG_CFG_t tCfg;

    eDbgStatus = EX_TIMER1_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eDbgStatus = EX_TIMER1_GetAdcTrgConfig(n32Argc, pn8Argv, &tCfg);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_TIMER1_SetAdcTrgConfig(eId, &tCfg);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    return DEBUG_CMD_SUCCESS;
}


static enum debug_cmd_status EX_TIMER1_Start(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    TIMER1_ID_e eId;

    eDbgStatus = EX_TIMER1_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    s_tTimer1Context[eId].eId = eId;

    eErr = HAL_TIMER1_Start(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_TIMER1_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_START]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_TIMER1_Stop(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    TIMER1_ID_e eId;

    eDbgStatus = EX_TIMER1_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_TIMER1_Stop(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_TIMER1_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_STOP]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_TIMER1_SetPause(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    TIMER1_ID_e eId;
    bool bPause = false;

    if(n32Argc != 3)
    {
        EX_COMMON_SetShowModuleLog(EX_TIMER1_ERR_STR, (char *)pcOptStr[EX_COMM_STR_OPT_ARG], EX_COMM_STR_OPT_MAX);
        return DEBUG_CMD_INVALID;
    }

    eDbgStatus = EX_TIMER1_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    if((strncmp(pn8Argv[2],"on",2) == 0))
    {
        bPause = true;      
    }

    eErr = HAL_TIMER1_SetPause(eId, bPause);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", eId, (bPause == true) ? "P" : "C");

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_TIMER1_SetLog(int32_t n32Argc, char *pn8Argv[])
{
    if((strncmp(pn8Argv[1],"on",2) == 0))
    {
        un32ISRLog = 1;
    }
    else
    {
        un32ISRLog = 0;
    }

    LOG("%s ISR Log %s.\n", EX_TIMER1_LOG_STR, (un32ISRLog == 1 ? "on":"off"));

    return DEBUG_CMD_SUCCESS;
}

static struct debug_cmd s_tEX_TIMER1_CMD[] =
{
    {EX_TIMER1_STR, "h", EX_TIMER1_Help, "help"},
    {EX_TIMER1_STR, "init", EX_TIMER1_Init,""},
    {EX_TIMER1_STR, "uninit", EX_TIMER1_Uninit,""},
    {EX_TIMER1_STR, "clk", EX_TIMER1_SetClk,""},
    {EX_TIMER1_STR, "config", EX_TIMER1_SetConfig,""},
    {EX_TIMER1_STR, "adctrg", EX_TIMER1_SetAdcTrgConfig,""},
    {EX_TIMER1_STR, "start", EX_TIMER1_Start,""},
    {EX_TIMER1_STR, "stop", EX_TIMER1_Stop,""},
    {EX_TIMER1_STR, "pause", EX_TIMER1_SetPause,""},
    {EX_TIMER1_STR, "log", EX_TIMER1_SetLog,""}
};

/**********************************************************************
 * @brief		Main program
 * @param[in]	None
 * @return	None
 **********************************************************************/
void EX_TIMER1(void)
{
    /* Add EX commands */
    debug_cmd_init(s_tEX_TIMER1_CMD,DEBUG_CMD_LIST_COUNT(s_tEX_TIMER1_CMD));
}

#endif
/* --------------------------------- End Of File ------------------------------ */
