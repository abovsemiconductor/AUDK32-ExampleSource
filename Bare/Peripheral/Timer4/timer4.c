/**
 *******************************************************************************
 * @file        timer4.c
 * @author      ABOV R&D Division
 * @brief       Timer4 Example Code
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

#if defined(TIMER4_TC)
#include "abov_config.h"
#include "ex_configs.h"
#include "ex_common.h"
#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "hal_scu.h"
#include "hal_scu_clk.h"
#include "hal_pcu.h"
#include "hal_timer4.h"

#if !defined(_TIMER4)
#error "This chipset did not support this example."
#endif

#define EX_TIMER4_STR "TIMER4"
#define EX_TIMER4_LOG_STR "TIMER4 :"
#define EX_TIMER4_ERR_STR "[E]TIMER4 :"
#define EX_TIMER4_IRQ_PRIO 3
#define EX_TIMER4_MAX_ARG  7
#define EX_TIMER4_MAX_NUM (CONFIG_TIMER4_MAX_COUNT - 1)

extern const char *pcCommStr[];
extern const char *pcCmdStr[];
extern const char *pcValStr[];
extern const char *pcOptStr[];

static TIMER4_Context_t s_tTimer4Context[CONFIG_TIMER4_MAX_COUNT];
static uint32_t un32ISRLog = 0;

static enum debug_cmd_status EX_TIMER4_Help(int32_t n32Argc, char *pn8Argv[])
{
    uint32_t un32ShowOpt = 0;
    EX_COMM_STR_OPT_e eOpt[7];

    EX_COMMON_SetShowModuleInfo(EX_TIMER4_STR, CONFIG_TIMER4_MAX_COUNT);

    un32ShowOpt = EX_COMMON_GetShowOpt(pn8Argv[1]);

    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_INIT, NULL, EX_TIMER4_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_UNINIT, NULL, EX_TIMER4_MAX_NUM, eOpt, 0, NULL);

    eOpt[0] = EX_COMM_STR_OPT_SRC; 
    eOpt[1] = EX_COMM_STR_OPT_DIV; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CLK, NULL, EX_TIMER4_MAX_NUM, eOpt, 2, NULL);
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_SRC, EX_COMM_STR_VAL_CLKPATH, "/e(ext)");
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
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CONFIG, NULL, EX_TIMER4_MAX_NUM, eOpt, 5, NULL);
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_OPS, EX_COMM_STR_VAL_OPS, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MODE, EX_COMM_STR_VAL_MAX, "p(periodic)/c(capture)/o(oneshot)/m(pwm)");
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "capture: [edge] [ch] [cnt]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_EDGE, EX_COMM_STR_VAL_EDGE, "/b(both)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "ch: 1/2/3/x(xor)"); 
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_CNT, EX_COMM_STR_VAL_N_NUM, NULL); 
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_POL, EX_COMM_STR_VAL_LOWHIGH, NULL); 
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_DA, EX_COMM_STR_VAL_N_NUM, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_DB, EX_COMM_STR_VAL_N_NUM, NULL);
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_START, NULL, EX_TIMER4_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_STOP, NULL, EX_TIMER4_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "pause", EX_TIMER4_MAX_NUM, eOpt, 0, (char *)pcValStr[EX_COMM_STR_VAL_ONOFF]);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_LOG, NULL, -1, eOpt, 0, (char *)pcValStr[EX_COMM_STR_VAL_ONOFF]);
    LOG("\n");

    return DEBUG_CMD_SUCCESS;
}

static void EX_TIMER4_IRQHandler(uint32_t un32Event, void *pContext)
{
    TIMER4_Context_t *ptContext = (TIMER4_Context_t *)pContext;
    uint32_t un32Data;

    if(un32Event & TIMER4_EVENT_PERIODIC_MATCH)
    {
        if(un32ISRLog == 1)
        {
            LOG("%s (%d) P %s\n", EX_TIMER4_LOG_STR, ptContext->eId, pcCommStr[EX_COMM_STR_EVT_FIRE]);
        }
    }

    if(un32Event & TIMER4_EVENT_PWM_DUTY)
    {
        if(un32ISRLog == 1)
        {
            LOG("%s (%d) PWM D %s\n", EX_TIMER4_LOG_STR, ptContext->eId, pcCommStr[EX_COMM_STR_EVT_FIRE]);
        }
    }

    if(un32Event & TIMER4_EVENT_PWM_PERIOD)
    {
        if(un32ISRLog == 1)
        {
            LOG("%s (%d) PWM P %s\n", EX_TIMER4_LOG_STR, ptContext->eId, pcCommStr[EX_COMM_STR_EVT_FIRE]);
        }
    }

    if(un32Event & TIMER4_EVENT_CAPTURE)
    {
        HAL_TIMER4_GetData(ptContext->eId, TIMER4_DATA_CAP_A, &un32Data);
        if(un32ISRLog == 1)
        {
            LOG("%s (%d) C-A %s(Data=%d)\n", EX_TIMER4_LOG_STR, ptContext->eId, pcCommStr[EX_COMM_STR_EVT_FIRE], un32Data);
        }
    }

    if(un32Event & TIMER4_EVENT_CAPTURE_B)
    {
        HAL_TIMER4_GetData(ptContext->eId, TIMER4_DATA_CAP_B, &un32Data);
        if(un32ISRLog == 1)
        {
            LOG("%s (%d) C-B %s(Data=%d)\n", EX_TIMER4_LOG_STR, ptContext->eId, pcCommStr[EX_COMM_STR_EVT_FIRE], un32Data);
        }
    }
}

static enum debug_cmd_status EX_TIMER4_GetClkConfig(int32_t n32Argc, char *pn8Argv[], TIMER4_CLK_CFG_t *ptClkCfg)
{
    uint8_t un8Arg = 2;

    if (pn8Argv[un8Arg][0] == 'p')
        ptClkCfg->eClk = TIMER4_CLK_PCLK;
    else if (pn8Argv[un8Arg][0] == 'm')
        ptClkCfg->eClk = TIMER4_CLK_MCCR;
    else if (pn8Argv[un8Arg][0] == 'e')
        ptClkCfg->eClk = TIMER4_CLK_EXT;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_TIMER4_ERR_STR, NULL, EX_COMM_STR_OPT_SRC);
        goto err;
    }

    if(ptClkCfg->eClk == TIMER4_CLK_MCCR)
    {
        un8Arg++;
        if (pn8Argv[un8Arg][0] == 'l')
            ptClkCfg->uSubClk.eMccr = TIMER4_CLK_MCCR_LSI;
        else if (pn8Argv[un8Arg][0] == 's')
            ptClkCfg->uSubClk.eMccr = TIMER4_CLK_MCCR_LSE;
        else if (pn8Argv[un8Arg][0] == 'm')
            ptClkCfg->uSubClk.eMccr = TIMER4_CLK_MCCR_MCLK;
        else if (pn8Argv[un8Arg][0] == 'h')
            ptClkCfg->uSubClk.eMccr = TIMER4_CLK_MCCR_HSI;
        else if (pn8Argv[un8Arg][0] == 'e')
            ptClkCfg->uSubClk.eMccr = TIMER4_CLK_MCCR_HSE;
        else if (pn8Argv[un8Arg][0] == 'p')
            ptClkCfg->uSubClk.eMccr = TIMER4_CLK_MCCR_PLL;
        else
        {
            EX_COMMON_SetShowModuleLog(EX_TIMER4_ERR_STR, NULL, EX_COMM_STR_OPT_MCCR);
            goto err;
        }
        ptClkCfg->un8MccrDiv = atoi(pn8Argv[++un8Arg]);

    }
    else if(ptClkCfg->eClk == TIMER4_CLK_EXT)
    {
        switch (pn8Argv[++un8Arg][0])
        {
            case 'f':
                ptClkCfg->uSubClk.eExtClkEdge = TIMER4_EXTCLK_EDGE_FALLING;
                break;
            case 'r':
                ptClkCfg->uSubClk.eExtClkEdge = TIMER4_EXTCLK_EDGE_RISING;
                break;
            default:
                EX_COMMON_SetShowModuleLog(EX_TIMER4_ERR_STR, NULL, EX_COMM_STR_OPT_EDGE);
                goto err;
        }
    }

    ptClkCfg->un16PreScale = atoi(pn8Argv[++un8Arg]);

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_TIMER4_GetConfig(int32_t n32Argc, char *pn8Argv[], TIMER4_ID_e eId, TIMER4_OPS_e *peOps, TIMER4_CFG_t *ptCfg)
{
    uint8_t un8Arg = 2;

    switch (pn8Argv[un8Arg++][0])
    {
        case 'p':
            *peOps = TIMER4_OPS_POLL;
            ptCfg->bIntrEnable = false;
            break;
        case 'i':
            *peOps = TIMER4_OPS_INTR;
            ptCfg->bIntrEnable = true;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_TIMER4_ERR_STR, NULL, EX_COMM_STR_OPT_OPS);
            goto err;
    }

    if (pn8Argv[un8Arg][0] == 'p')
        ptCfg->eMode = TIMER4_MODE_PERIODIC;
    else if (pn8Argv[un8Arg][0] == 'c')
        ptCfg->eMode = TIMER4_MODE_CAPTURE;
    else if (pn8Argv[un8Arg][0] == 'o')
        ptCfg->eMode = TIMER4_MODE_ONESHOT;
    else if (pn8Argv[un8Arg][0] == 'm')
        ptCfg->eMode = TIMER4_MODE_PWM;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_TIMER4_ERR_STR, NULL, EX_COMM_STR_OPT_MODE);
        goto err;
    }

    if(ptCfg->eMode == TIMER4_MODE_CAPTURE)
    {
        un8Arg++;
        if (pn8Argv[un8Arg][0] == 'r')
            ptCfg->tCapCfg.eEdge = TIMER4_CAP_EDGE_RISING;
        else if (pn8Argv[un8Arg][0] == 'f')
            ptCfg->tCapCfg.eEdge = TIMER4_CAP_EDGE_FALLING;
        else if (pn8Argv[un8Arg][0] == 'b')
            ptCfg->tCapCfg.eEdge = TIMER4_CAP_EDGE_BOTH;
        else
        {
            EX_COMMON_SetShowModuleLog(EX_TIMER4_ERR_STR, "cap", EX_COMM_STR_OPT_EDGE);
            goto err;
        }

        un8Arg++;
        if (pn8Argv[un8Arg][0] == '1')
            ptCfg->tCapCfg.eCh = TIMER4_CAP_CH_1;
        else if (pn8Argv[un8Arg][0] == '2')
            ptCfg->tCapCfg.eCh = TIMER4_CAP_CH_2;
        else if (pn8Argv[un8Arg][0] == '3')
            ptCfg->tCapCfg.eCh = TIMER4_CAP_CH_3;
        else if (pn8Argv[un8Arg][0] == 'x')
            ptCfg->tCapCfg.eCh = TIMER4_CAP_CH_XOR;
        else
        {
            EX_COMMON_SetShowModuleLog(EX_TIMER4_ERR_STR, "cap[ch]", EX_COMM_STR_OPT_MAX);
            goto err;
        }

        ptCfg->tCapCfg.un16EdgeCount = atoi(pn8Argv[++un8Arg]);
        if(ptCfg->tCapCfg.un16EdgeCount > 4095)
        {
            EX_COMMON_SetShowModuleLog(EX_TIMER4_ERR_STR, "cap[cnt]", EX_COMM_STR_OPT_MAX);
            goto err;
        }
    }

    switch (pn8Argv[++un8Arg][0])
    {
        case 'h':
            ptCfg->ePol = TIMER4_POL_HIGH;
            break;
        case 'l':
            ptCfg->ePol = TIMER4_POL_LOW;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_TIMER4_ERR_STR, NULL, EX_COMM_STR_OPT_POL);
            goto err;
    } 

    if(ptCfg->eMode == TIMER4_MODE_PWM)
    {
        ptCfg->utData.tPWM.un16Duty = atoi(pn8Argv[++un8Arg]);
        ptCfg->utData.tPWM.un16Period = atoi(pn8Argv[++un8Arg]);
    }
    else
    {
        ptCfg->utData.tGRD.un16DataA = atoi(pn8Argv[++un8Arg]);
        ptCfg->utData.tGRD.un16DataB = atoi(pn8Argv[++un8Arg]);
    }
    
    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_TIMER4_GetId(int32_t n32Argc, char *pn8Argv[], TIMER4_ID_e *peId)
{
    uint8_t un8Data = 0;
    if(n32Argc == 1)
    {
        EX_COMMON_SetShowModuleLog(EX_TIMER4_ERR_STR, (char *)pcOptStr[EX_COMM_STR_OPT_ARG], EX_COMM_STR_OPT_MAX);
        return DEBUG_CMD_INVALID;
    }

    un8Data = atoi(pn8Argv[1]); 
    if(un8Data >= CONFIG_TIMER4_MAX_COUNT)
    {
        LOG("%s Max Chan %d\n", EX_TIMER4_ERR_STR, CONFIG_TIMER4_MAX_COUNT);
        return DEBUG_CMD_INVALID;
    }

    *peId = (TIMER4_ID_e)un8Data;

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_TIMER4_Init(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    TIMER4_ID_e eId;

    eDbgStatus = EX_TIMER4_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_TIMER4_Init(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_TIMER4_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_INIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_TIMER4_Uninit(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    TIMER4_ID_e eId;

    eDbgStatus = EX_TIMER4_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_TIMER4_Uninit(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_TIMER4_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_UNINIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_TIMER4_SetClk(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    TIMER4_ID_e eId;

    TIMER4_CLK_CFG_t tClkCfg =
    {
#if (TIMER4_CLKSRC_USE_PCLK == 1)
        .eClk = TIMER4_CLK_PCLK_DIV_16,
        .uSubClk.eMccr = TIMER4_CLK_MCCR_NONE,
        .un8MccrDiv = 0,
#else
        .eClk  = TIMER4_CLK_MCCR,
        .uSubClk.eMccr = TIMER4_CLK_MCCR_HSE,
        .un8MccrDiv = DEFAULT_HSE_1MHZ_DIV,
#endif
    };

    eDbgStatus = EX_TIMER4_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eDbgStatus = EX_TIMER4_GetClkConfig(n32Argc, pn8Argv, &tClkCfg);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_TIMER4_SetClkConfig(eId, &tClkCfg);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_TIMER4_ERR_STR, "ClkCfg", EX_COMM_STR_OPT_MAX);
        }
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_TIMER4_SetConfig(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    TIMER4_ID_e eId;
    TIMER4_OPS_e eOps;
    TIMER4_CFG_t tTimer4Cfg;

    eDbgStatus = EX_TIMER4_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eDbgStatus = EX_TIMER4_GetConfig(n32Argc, pn8Argv, eId, &eOps, &tTimer4Cfg);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_TIMER4_SetConfig(eId, &tTimer4Cfg);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    if(eOps == TIMER4_OPS_INTR)
    {
        eErr = HAL_TIMER4_SetIRQ(eId, eOps, EX_TIMER4_IRQHandler, (void *)&s_tTimer4Context[(uint32_t)eId], EX_TIMER4_IRQ_PRIO);
        if(eErr != HAL_ERR_OK)
        {
            goto err;
        }
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_TIMER4_Start(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    TIMER4_ID_e eId;

    eDbgStatus = EX_TIMER4_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    s_tTimer4Context[eId].eId = eId;

    eErr = HAL_TIMER4_Start(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_TIMER4_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_START]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_TIMER4_Stop(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    TIMER4_ID_e eId;

    eDbgStatus = EX_TIMER4_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_TIMER4_Stop(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_TIMER4_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_STOP]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_TIMER4_SetPause(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    TIMER4_ID_e eId;
    bool bPause = false;

    if(n32Argc != 3)
    {
        EX_COMMON_SetShowModuleLog(EX_TIMER4_ERR_STR, (char *)pcOptStr[EX_COMM_STR_OPT_ARG], EX_COMM_STR_OPT_MAX);
        goto err;
    }

    eDbgStatus = EX_TIMER4_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    if((strncmp(pn8Argv[2],"on",2) == 0))
    {
        bPause = true;      
    }

    eErr = HAL_TIMER4_SetPause(eId, bPause);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    LOG("%s (%d) (%s)\n", EX_TIMER4_LOG_STR, eId, (bPause == true) ? "P" : "C");

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_TIMER4_SetLog(int32_t n32Argc, char *pn8Argv[])
{
    if((strncmp(pn8Argv[1],"on",2) == 0))
    {
        un32ISRLog = 1;
    }
    else
    {
        un32ISRLog = 0;
    }

    LOG("TIMER4 ISR Log %s.\n",(un32ISRLog == 1 ? "on":"off"));

    return DEBUG_CMD_SUCCESS;
}

static const struct debug_cmd s_tEX_TIMER4_CMD[] =
{
    {"TIMER4", "h", EX_TIMER4_Help, "help"},
    {"TIMER4", "init", EX_TIMER4_Init, ""},
    {"TIMER4", "uninit", EX_TIMER4_Uninit, ""},
    {"TIMER4", "clk", EX_TIMER4_SetClk, ""},
    {"TIMER4", "config",EX_TIMER4_SetConfig, ""},
    {"TIMER4", "start", EX_TIMER4_Start, ""},
    {"TIMER4", "stop", EX_TIMER4_Stop, ""},
    {"TIMER4", "pause", EX_TIMER4_SetPause, ""},
    {"TIMER4", "log", EX_TIMER4_SetLog, ""}
};

/**********************************************************************
 * @brief		Main program
 * @param[in]	None
 * @return	None
 **********************************************************************/
void EX_TIMER4(void)
{
    /* Add TC commands */
    debug_cmd_init(s_tEX_TIMER4_CMD,DEBUG_CMD_LIST_COUNT(s_tEX_TIMER4_CMD));
}

#endif
/* --------------------------------- End Of File ------------------------------ */
