/**
 *******************************************************************************
 * @file        timer5.c
 * @author      ABOV R&D Division
 * @brief       Timer5 Example Code
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

#if defined(TIMER5_TC)
#include "abov_config.h"
#include "ex_configs.h"
#include "ex_common.h"
#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "hal_scu.h"
#include "hal_scu_clk.h"
#include "hal_pcu.h"
#include "hal_timer5.h"

#if !defined(_TIMER5)
#error EX_COMM_MODULE_ERR_STR
#endif

#define EX_TIMER5_STR "TIMER5"
#define EX_TIMER5_LOG_STR "TIMER5 :"
#define EX_TIMER5_ERR_STR "[E]TIMER5 :"
#define EX_TIMER5_IRQ_PRIO 3
#define EX_TIMER5_MAX_ARG  7
#define EX_TIMER5_MAX_NUM  (CONFIG_TIMER5_MAX_COUNT - 1)

extern const char *pcCommStr[];
extern const char *pcCmdStr[];
extern const char *pcValStr[];
extern const char *pcOptStr[];

static TIMER5_Context_t s_tTimer5Context[CONFIG_TIMER5_MAX_COUNT];
static uint32_t un32ISRLog = 0;

static enum debug_cmd_status EX_TIMER5_Help(int32_t n32Argc, char *pn8Argv[])
{
    uint32_t un32ShowOpt = 0;
    EX_COMM_STR_OPT_e eOpt[10];

    EX_COMMON_SetShowModuleInfo(EX_TIMER5_STR, CONFIG_TIMER5_MAX_COUNT);

    un32ShowOpt = EX_COMMON_GetShowOpt(pn8Argv[1]);

    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_INIT, NULL, EX_TIMER5_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_UNINIT, NULL, EX_TIMER5_MAX_NUM, eOpt, 0, NULL);

    eOpt[0] = EX_COMM_STR_OPT_SRC; 
    eOpt[1] = EX_COMM_STR_OPT_DIV; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CLK, NULL, EX_TIMER5_MAX_NUM, eOpt, 2, NULL);
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_SRC, EX_COMM_STR_VAL_CLKPATH, "/e(ext)/w(wdtrc)");
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MCCR, EX_COMM_STR_VAL_MAX, "[mccr]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MCCR, EX_COMM_STR_VAL_MAX, "/s(lse)/h(hsi)");
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "ext: [edge]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_EDGE, EX_COMM_STR_VAL_EDGE, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_DIV, EX_COMM_STR_VAL_N_NUM, NULL);
    }

    eOpt[0] = EX_COMM_STR_OPT_OPS; 
    eOpt[1] = EX_COMM_STR_OPT_MODE; 
    eOpt[2] = EX_COMM_STR_OPT_POL; 
    eOpt[3] = EX_COMM_STR_OPT_DA;
    eOpt[4] = EX_COMM_STR_OPT_DB;
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CONFIG, NULL, EX_TIMER5_MAX_NUM, eOpt, 5, NULL);
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_OPS, EX_COMM_STR_VAL_OPS, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MODE, EX_COMM_STR_VAL_MAX, "p(periodic)/c(capture)/o(oneshot)/m(pwm)");
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "capture: [edge] [sig] [keep]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_EDGE, EX_COMM_STR_VAL_EDGE, "/b(both)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX,"sig: e(ext)/l(lse)/w(wdtrc)"); 
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX,"keep: e(en)/d(dis)"); 
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_POL, EX_COMM_STR_VAL_LOWHIGH, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_DA, EX_COMM_STR_VAL_N_NUM, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_DB, EX_COMM_STR_VAL_N_NUM, NULL);
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_START, NULL, EX_TIMER5_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_STOP, NULL, EX_TIMER5_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "pause", EX_TIMER5_MAX_NUM, eOpt, 0, (char *)pcValStr[EX_COMM_STR_VAL_ONOFF]);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_LOG, NULL, -1, eOpt, 0, (char *)pcValStr[EX_COMM_STR_VAL_ONOFF]);
    LOG("\n");

    return DEBUG_CMD_SUCCESS;
}

static void EX_TIMER5_IRQHandler(uint32_t un32Event, void *pContext)
{
    TIMER5_Context_t *ptContext = (TIMER5_Context_t *)pContext;
    uint32_t un32Data;

    if(un32Event & TIMER5_EVENT_PERIODIC_MATCH)
    {
        if(un32ISRLog == 1)
        {
            LOG("%s (%d) P %s\n", EX_TIMER5_LOG_STR, ptContext->eId, pcCommStr[EX_COMM_STR_EVT_FIRE]);
        }
    }

    if(un32Event & TIMER5_EVENT_PWM_DUTY)
    {
        if(un32ISRLog == 1)
        {
            LOG("%s (%d) PWM D %s\n", EX_TIMER5_LOG_STR, ptContext->eId, pcCommStr[EX_COMM_STR_EVT_FIRE]);
        }
    }

    if(un32Event & TIMER5_EVENT_PWM_PERIOD)
    {
        if(un32ISRLog == 1)
        {
            LOG("%s (%d) PWM P %s\n", EX_TIMER5_LOG_STR, ptContext->eId, pcCommStr[EX_COMM_STR_EVT_FIRE]);
        }
    }

    if(un32Event & TIMER5_EVENT_CAPTURE)
    {
        HAL_TIMER5_GetData(ptContext->eId, TIMER5_DATA_CAP_A, &un32Data);
        if(un32ISRLog == 1)
        {
            LOG("%s (%d) C-A %s(Data=%d)\n", EX_TIMER5_LOG_STR, ptContext->eId, pcCommStr[EX_COMM_STR_EVT_FIRE], un32Data);
        }
    }

    if(un32Event & TIMER5_EVENT_CAPTURE_B)
    {
        HAL_TIMER5_GetData(ptContext->eId, TIMER5_DATA_CAP_B, &un32Data);
        if(un32ISRLog == 1)
        {
            LOG("%s (%d) C-B %s(Data=%d)\n", EX_TIMER5_LOG_STR, ptContext->eId, pcCommStr[EX_COMM_STR_EVT_FIRE], un32Data);
        }
    }
}

static enum debug_cmd_status EX_TIMER5_GetClkConfig(int32_t n32Argc, char *pn8Argv[], TIMER5_CLK_CFG_t *ptClkCfg)
{
    uint8_t un8Arg = 2;

    if (pn8Argv[un8Arg][0] == 'p')
        ptClkCfg->eClk = TIMER5_CLK_PCLK;
    else if (pn8Argv[un8Arg][0] == 'm')
        ptClkCfg->eClk = TIMER5_CLK_MCCR;
    else if (pn8Argv[un8Arg][0] == 'e')
        ptClkCfg->eClk = TIMER5_CLK_EXT;
    else if (pn8Argv[un8Arg][0] == 'w')
        ptClkCfg->eClk = TIMER5_CLK_WDTRC;
    else 
    {
        EX_COMMON_SetShowModuleLog(EX_TIMER5_ERR_STR, NULL, EX_COMM_STR_OPT_SRC);
        goto err;
    }

    un8Arg++;
    if(ptClkCfg->eClk == TIMER5_CLK_MCCR)
    {
        if (pn8Argv[un8Arg][0] == 's')
            ptClkCfg->uSubClk.eMccr = TIMER5_CLK_MCCR_LSE;
        else if (pn8Argv[un8Arg][0] == 'h')
            ptClkCfg->uSubClk.eMccr = TIMER5_CLK_MCCR_HSI;
        else
        {
            EX_COMMON_SetShowModuleLog(EX_TIMER5_ERR_STR, NULL, EX_COMM_STR_OPT_MCCR);
            goto err;
        }
        un8Arg++;
    }
    else if(ptClkCfg->eClk == TIMER5_CLK_EXT)
    {
        switch (pn8Argv[un8Arg++][0])
        {
            case 'f':
                ptClkCfg->uSubClk.eExtClkEdge = TIMER5_EXTCLK_EDGE_FALLING;
                break;
            case 'r':
                ptClkCfg->uSubClk.eExtClkEdge = TIMER5_EXTCLK_EDGE_RISING;
                break;
            default:
                EX_COMMON_SetShowModuleLog(EX_TIMER5_ERR_STR, NULL, EX_COMM_STR_OPT_EDGE);
                goto err;
        }
    }

    if (ptClkCfg->eClk != TIMER5_CLK_EXT)
    {
        ptClkCfg->un16PreScale = atoi(pn8Argv[un8Arg++]);
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_TIMER5_GetConfig(int32_t n32Argc, char *pn8Argv[], TIMER5_ID_e eId, TIMER5_OPS_e *peOps, TIMER5_CFG_t *ptCfg)
{
    enum debug_cmd_status eDbgStatus;
    uint8_t un8Arg = 2;

    switch (pn8Argv[un8Arg++][0])
    {
        case 'p':
            *peOps = TIMER5_OPS_POLL;
            ptCfg->bIntrEnable = false;
            break;
        case 'i':
            *peOps = TIMER5_OPS_INTR;
            ptCfg->bIntrEnable = true;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_TIMER5_ERR_STR, NULL, EX_COMM_STR_OPT_OPS);
            goto err;
    }

    if (pn8Argv[un8Arg][0] == 'p')
        ptCfg->eMode = TIMER5_MODE_PERIODIC;
    else if (pn8Argv[un8Arg][0] == 'c')
        ptCfg->eMode = TIMER5_MODE_CAPTURE;
    else if (pn8Argv[un8Arg][0] == 'o')
        ptCfg->eMode = TIMER5_MODE_ONESHOT;
    else if (pn8Argv[un8Arg][0] == 'm')
        ptCfg->eMode = TIMER5_MODE_PWM;
    else 
    {
        EX_COMMON_SetShowModuleLog(EX_TIMER5_ERR_STR, NULL, EX_COMM_STR_OPT_MODE);
        goto err;
    }

    un8Arg++;
    if(ptCfg->eMode == TIMER5_MODE_CAPTURE)
    {
        if (pn8Argv[un8Arg][0] == 'r')
            ptCfg->tCapCfg.eEdge = TIMER5_CAP_EDGE_RISING;
        else if (pn8Argv[un8Arg][0] == 'f')
            ptCfg->tCapCfg.eEdge = TIMER5_CAP_EDGE_FALLING;
        else if (pn8Argv[un8Arg][0] == 'b')
            ptCfg->tCapCfg.eEdge = TIMER5_CAP_EDGE_BOTH;
        else
        {
            EX_COMMON_SetShowModuleLog(EX_TIMER5_ERR_STR, NULL, EX_COMM_STR_OPT_EDGE);
            goto err;
        }

        un8Arg++;
        if (pn8Argv[un8Arg][0] == 'e')
            ptCfg->tCapCfg.eSig = TIMER5_CAP_SIG_EXT;
        else if (pn8Argv[un8Arg][0] == 'l')
            ptCfg->tCapCfg.eSig = TIMER5_CAP_SIG_LSE;
        else if (pn8Argv[un8Arg][0] == 'w')
            ptCfg->tCapCfg.eSig = TIMER5_CAP_SIG_WDTRC;
        else
        {
            EX_COMMON_SetShowModuleLog(EX_TIMER5_ERR_STR, "[sig]", EX_COMM_STR_OPT_MAX);
            goto err;
        }

        un8Arg++;
        eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], &ptCfg->tCapCfg.bKeepCount);
        if (eDbgStatus != DEBUG_CMD_SUCCESS)
        {
            EX_COMMON_SetShowModuleLog(EX_TIMER5_ERR_STR, "[keep]", EX_COMM_STR_OPT_MAX);
            goto err;
        }
    }

    switch (pn8Argv[un8Arg++][0])
    {
        case 'h':
            ptCfg->ePol = TIMER5_POL_HIGH;
            break;
        case 'l':
            ptCfg->ePol = TIMER5_POL_LOW;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_TIMER5_ERR_STR, NULL, EX_COMM_STR_OPT_POL);
            goto err;
    } 

    if(ptCfg->eMode == TIMER5_MODE_PWM)
    {
        ptCfg->utData.tPWM.un16Duty = atoi(pn8Argv[un8Arg++]);
        ptCfg->utData.tPWM.un16Period = atoi(pn8Argv[un8Arg++]);
    }
    else
    {
        ptCfg->utData.tGRD.un16DataA = atoi(pn8Argv[un8Arg++]);
        ptCfg->utData.tGRD.un16DataB = atoi(pn8Argv[un8Arg++]);
    }
    
    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_TIMER5_GetId(int32_t n32Argc, char *pn8Argv[], TIMER5_ID_e *peId)
{
    uint8_t un8Data = 0;
    if(n32Argc == 1)
    {
        EX_COMMON_SetShowModuleLog(EX_TIMER5_ERR_STR, (char *)pcOptStr[EX_COMM_STR_OPT_ARG], EX_COMM_STR_OPT_MAX);
        return DEBUG_CMD_INVALID;
    }

    un8Data = atoi(pn8Argv[1]); 
    if(un8Data >= CONFIG_TIMER5_MAX_COUNT)
    {
        LOG("%s Max Chan %d\n", EX_TIMER5_ERR_STR, CONFIG_TIMER5_MAX_COUNT);
        return DEBUG_CMD_INVALID;
    }

    *peId = (TIMER5_ID_e)un8Data;

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_TIMER5_Init(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    TIMER5_ID_e eId;

    eDbgStatus = EX_TIMER5_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_TIMER5_Init(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_TIMER5_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_INIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_TIMER5_Uninit(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    TIMER5_ID_e eId;

    eDbgStatus = EX_TIMER5_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_TIMER5_Uninit(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_TIMER5_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_UNINIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_TIMER5_SetClk(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    TIMER5_ID_e eId;

    TIMER5_CLK_CFG_t tClkCfg =
    {
#if (TIMER5_CLKSRC_USE_PCLK == 1)
        .eClk = TIMER5_CLK_PCLK_DIV_16,
        .uSubClk.eMccr = TIMER5_CLK_MCCR_NONE,
#else
        .eClk  = TIMER5_CLK_MCCR,
        .uSubClk.eMccr = TIMER5_CLK_MCCR_HSE,
#endif
    };

    eDbgStatus = EX_TIMER5_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eDbgStatus = EX_TIMER5_GetClkConfig(n32Argc, pn8Argv, &tClkCfg);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_TIMER5_SetClkConfig(eId, &tClkCfg);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_TIMER5_ERR_STR, "ClkCfg", EX_COMM_STR_OPT_MAX);
        }
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_TIMER5_SetConfig(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    TIMER5_ID_e eId;
    TIMER5_OPS_e eOps;
    TIMER5_CFG_t tTimer5Cfg;

    eDbgStatus = EX_TIMER5_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eDbgStatus = EX_TIMER5_GetConfig(n32Argc, pn8Argv, eId, &eOps, &tTimer5Cfg);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_TIMER5_SetConfig(eId, &tTimer5Cfg);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    if(eOps == TIMER5_OPS_INTR)
    {
        eErr = HAL_TIMER5_SetIRQ(eId, eOps, EX_TIMER5_IRQHandler, (void *)&s_tTimer5Context[(uint32_t)eId], EX_TIMER5_IRQ_PRIO);
        if(eErr != HAL_ERR_OK)
        {
            goto err;
        }
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_TIMER5_Start(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    TIMER5_ID_e eId;

    eDbgStatus = EX_TIMER5_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    s_tTimer5Context[eId].eId = eId;

    eErr = HAL_TIMER5_Start(eId);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    LOG("%s (%d) %s\n", EX_TIMER5_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_START]);

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_TIMER5_Stop(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    TIMER5_ID_e eId;

    eDbgStatus = EX_TIMER5_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_TIMER5_Stop(eId);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    LOG("%s (%d) %s\n", EX_TIMER5_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_STOP]);

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_TIMER5_SetPause(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    TIMER5_ID_e eId;
    bool bPause = false;

    if(n32Argc != 3)
    {
        EX_COMMON_SetShowModuleLog(EX_TIMER5_ERR_STR, (char *)pcOptStr[EX_COMM_STR_OPT_ARG], EX_COMM_STR_OPT_MAX);
        goto err;
    }

    eDbgStatus = EX_TIMER5_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    if((strncmp(pn8Argv[2],"on",2) == 0))
    {
        bPause = true;      
    }

    eErr = HAL_TIMER5_SetPause(eId, bPause);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    LOG("%s (%d) (%s)\n", EX_TIMER5_LOG_STR, eId, (bPause == true) ? "P" : "C");

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_TIMER5_SetLog(int32_t n32Argc, char *pn8Argv[])
{
    if((strncmp(pn8Argv[1],"on",2) == 0))
    {
        un32ISRLog = 1;
    }
    else
    {
        un32ISRLog = 0;
    }

    LOG("%s ISR Log %s\n", EX_TIMER5_LOG_STR, (un32ISRLog == 1 ? "on":"off"));

    return DEBUG_CMD_SUCCESS;
}

static struct debug_cmd s_tEX_TIMER5_CMD[] =
{
    {EX_TIMER5_STR, "h", EX_TIMER5_Help, "help"},
    {EX_TIMER5_STR, "init", EX_TIMER5_Init,""},
    {EX_TIMER5_STR, "uninit", EX_TIMER5_Uninit,""},
    {EX_TIMER5_STR, "clk", EX_TIMER5_SetClk,""},
    {EX_TIMER5_STR, "config",EX_TIMER5_SetConfig,""},
    {EX_TIMER5_STR, "start", EX_TIMER5_Start,""},
    {EX_TIMER5_STR, "stop", EX_TIMER5_Stop,""},
    {EX_TIMER5_STR, "pause", EX_TIMER5_SetPause,""},
    {EX_TIMER5_STR, "log", EX_TIMER5_SetLog,""}
};

/**********************************************************************
 * @brief		Main program
 * @param[in]	None
 * @return	None
 **********************************************************************/
void EX_TIMER5(void)
{
    /* Add TC commands */
    debug_cmd_init(s_tEX_TIMER5_CMD,DEBUG_CMD_LIST_COUNT(s_tEX_TIMER5_CMD));
}

#endif
/* --------------------------------- End Of File ------------------------------ */
