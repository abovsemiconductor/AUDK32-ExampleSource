/**
 *******************************************************************************
 * @file        timer4e.c
 * @author      ABOV R&D Division
 * @brief       Timer4e Example Code
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

#if defined(TIMER4E_TC)
#include "abov_config.h"
#include "ex_common.h"
#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "hal_scu.h"
#include "hal_scu_clk.h"
#include "hal_pcu.h"
#include "hal_timer4e.h"

#if !defined(_TIMER4E)
#error "This chipset did not support this example."
#endif

#define EX_TIMER4E_STR "TIMER4E"
#define EX_TIMER4E_LOG_STR "TIMER4E :"
#define EX_TIMER4E_ERR_STR "[E]TIMER4E :"
#define EX_TIMER4E_IRQ_PRIO 3
#define EX_TIMER4E_MAX_ARG  7
#define EX_TIMER4E_MAX_NUM (CONFIG_TIMER4E_MAX_COUNT - 1)

extern const char *pcCommStr[];
extern const char *pcCmdStr[];
extern const char *pcValStr[];
extern const char *pcOptStr[];

static TIMER4E_Context_t s_tTimer4eContext[CONFIG_TIMER4E_MAX_COUNT];
static uint32_t un32ISRLog = 0;

static enum debug_cmd_status EX_TIMER4E_Help(int32_t n32Argc, char *pn8Argv[])
{
    uint32_t un32ShowOpt = 0;
    EX_COMM_STR_OPT_e eOpt[10];

    EX_COMMON_SetShowModuleInfo(EX_TIMER4E_STR, CONFIG_TIMER4E_MAX_COUNT);

    un32ShowOpt = EX_COMMON_GetShowOpt(pn8Argv[1]);

    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_INIT, NULL, EX_TIMER4E_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_UNINIT, NULL, EX_TIMER4E_MAX_NUM, eOpt, 0, NULL);

    eOpt[0] = EX_COMM_STR_OPT_SRC; 
    eOpt[1] = EX_COMM_STR_OPT_DIV; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CLK, NULL, EX_TIMER4E_MAX_NUM, eOpt, 2, NULL);
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "p(pclk)/e(ext)");
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "pclk: [div]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_DIV, EX_COMM_STR_VAL_N_NUM, NULL);
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "ext: [edge]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_EDGE, EX_COMM_STR_VAL_EDGE, NULL);
    }
    eOpt[0] = EX_COMM_STR_OPT_OPS; 
    eOpt[1] = EX_COMM_STR_OPT_MODE; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CONFIG, NULL, EX_TIMER4E_MAX_NUM, eOpt, 2, "[-port] [-dly] [-updt] [dp] [da] [db]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_OPS, EX_COMM_STR_VAL_OPS, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MODE, EX_COMM_STR_VAL_MAX, "i(interval)/c(capture)/b(backtoback)/o(oneshot)");
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "capture: [edge]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_EDGE, EX_COMM_STR_VAL_EDGE, "/b(both)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "msk: m(mask)/n(non-mask)"); 
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-port: [-pa] [-pb]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-pa: [en]");
        EX_COMMON_SetShowOptVal(3, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "en: [slvl]");
        EX_COMMON_SetShowOptVal(4, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "slvl: l(low)/h(high)");
        EX_COMMON_SetShowOptVal(3, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "dis: [dlvl]");
        EX_COMMON_SetShowOptVal(4, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "dlvl: l(low)/h(high)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-pb: [en]");
        EX_COMMON_SetShowOptVal(3, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "en: [slvl]");
        EX_COMMON_SetShowOptVal(4, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "slvl: l(low)/h(high)");
        EX_COMMON_SetShowOptVal(3, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "dis: [dlvl]");
        EX_COMMON_SetShowOptVal(4, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "dlvl: l(low)/h(high)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-dly: [pos] [val]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "pos: f(front xa / back xb)/b(front xb and back xa)"); 
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "val: 0~N"); 
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-updt: [reload]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "reload: i(instant)/p(period)/b(bottom)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_CNT, EX_COMM_STR_VAL_MAX, "dp: 0~N(period)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_DA, EX_COMM_STR_VAL_N_NUM, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_DB, EX_COMM_STR_VAL_N_NUM, NULL);
    }
    eOpt[0] = EX_COMM_STR_OPT_ENA; 
    eOpt[1] = EX_COMM_STR_OPT_LEVEL; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "port", EX_TIMER4E_MAX_NUM, eOpt, 2, NULL);
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_ENA, EX_COMM_STR_VAL_MAX, "hexa(mask port num ex.0x3f)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_LEVEL, EX_COMM_STR_VAL_MAX, "hexa(port no enable ex.0x77)");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "adctrg", EX_TIMER4E_MAX_NUM, eOpt, 0, NULL);
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_ENA, EX_COMM_STR_VAL_MAX, "hexa(adctrg mask bit ex.0x1f)");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_START, NULL, EX_TIMER4E_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_STOP, NULL, EX_TIMER4E_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_LOG, NULL, -1, eOpt, 0, (char *)pcValStr[EX_COMM_STR_VAL_ONOFF]);
    LOG("\n");

    return DEBUG_CMD_SUCCESS;
}

static void EX_TIMER4E_IRQHandler(uint32_t un32Event, void *pContext)
{
    TIMER4E_Context_t *ptContext = (TIMER4E_Context_t *)pContext;
    uint32_t un32Data;

    if(un32Event & TIMER4E_EVENT_PERIOD_MATCH)
    {
        if(un32ISRLog == 1)
        {
            LOG("%s (%d) P evt fire\n", EX_TIMER4E_LOG_STR, ptContext->eId);
        }
    }

    if(un32Event & TIMER4E_EVENT_MATCH_CH_A)
    {
        if(un32ISRLog == 1)
        {
            LOG("%s (%d) M evt A fire\n", EX_TIMER4E_LOG_STR, ptContext->eId);
        }
    }

    if(un32Event & TIMER4E_EVENT_MATCH_CH_B)
    {
        if(un32ISRLog == 1)
        {
            LOG("%s (%d) U/D evt B fire\n", EX_TIMER4E_LOG_STR, ptContext->eId);
        }
    }

    if(un32Event & TIMER4E_EVENT_BOTTOM)
    {
        if(un32ISRLog == 1)
        {
            LOG("%s (%d) B evt fire\n", EX_TIMER4E_LOG_STR, ptContext->eId);
        }
    }

    if(un32Event & TIMER4E_EVENT_OUTPUT_FORCE)
    {
        if(un32ISRLog == 1)
        {
            LOG("%s (%d) OF evt fire\n", EX_TIMER4E_LOG_STR, ptContext->eId);
        }
    }

    if(un32Event & TIMER4E_EVENT_CAPTURE)
    {
        HAL_TIMER4E_GetData(ptContext->eId, TIMER4E_DATA_CAP, &un32Data);
        if(un32ISRLog == 1)
        {
            LOG("%s (%d) C evt fire (Data=%d)\n", EX_TIMER4E_LOG_STR, ptContext->eId, un32Data);
        }
    }
}

static enum debug_cmd_status EX_TIMER4E_GetClkConfig(int32_t n32Argc, char *pn8Argv[], TIMER4E_CLK_CFG_t *ptClkCfg)
{
    uint8_t un8Arg = 2;

    if (pn8Argv[un8Arg][0] == 'p')
        ptClkCfg->eClk = TIMER4E_CLK_PCLK;
    else if (pn8Argv[un8Arg][0] == 'e')
        ptClkCfg->eClk = TIMER4E_CLK_EXT;
    else
    {
        goto err;
    }
    
    un8Arg++;
    if (ptClkCfg->eClk == TIMER4E_CLK_PCLK)
    {
        ptClkCfg->un16PreScale = atoi(pn8Argv[un8Arg]);
    }
    else if(ptClkCfg->eClk == TIMER4E_CLK_EXT)
    {
        if(pn8Argv[un8Arg][0] == 'f')
            ptClkCfg->uSubClk.eExtClkEdge = TIMER4E_EXTCLK_EDGE_FALLING;
        else if(pn8Argv[un8Arg][0] == 'r')
            ptClkCfg->uSubClk.eExtClkEdge = TIMER4E_EXTCLK_EDGE_RISING;
        else 
        {
            EX_COMMON_SetShowModuleLog(EX_TIMER4E_ERR_STR, NULL, EX_COMM_STR_OPT_EDGE);
            goto err;
        }
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_TIMER4E_GetPortConfig(int32_t n32Argc, char *pn8Argv[], TIMER4E_OUTPUT_CFG_t *ptCfg)
{
    enum debug_cmd_status eDbgStatus;
    uint8_t un8Arg = 0;
    bool bLevel = false;
    TIMER4E_POL_e ePol = TIMER4E_POL_LOW;

    eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], &ptCfg->bEnable);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_TIMER4E_ERR_STR, NULL, EX_COMM_STR_OPT_ENA);
        goto err;
    }

    eDbgStatus = EX_COMMON_GetLevel(&pn8Argv[un8Arg++][0], &bLevel);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_TIMER4E_ERR_STR, NULL, EX_COMM_STR_OPT_POL);
        goto err;
    }

    ePol = (bLevel == true) ? TIMER4E_POL_HIGH : TIMER4E_POL_LOW;

    if (ptCfg->bEnable == true)
    {
        ptCfg->eEnPol = ePol;
    }
    else
    {
        ptCfg->eDisPol = ePol;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_TIMER4E_GetConfig(int32_t n32Argc, char *pn8Argv[], TIMER4E_OPS_e *peOps, TIMER4E_CFG_t *ptCfg, bool *pbBlnkNonMask)
{
    uint8_t un8Arg = 2;
    enum debug_cmd_status eDbgStatus;

    switch (pn8Argv[un8Arg++][0])
    {
        case 'p':
            *peOps = TIMER4E_OPS_POLL;
            break;
        case 'i':
            *peOps = TIMER4E_OPS_INTR;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_TIMER4E_ERR_STR, NULL, EX_COMM_STR_OPT_EDGE);
            goto err;
    }

    if ((pn8Argv[un8Arg][0]) == 'i')
    {
        ptCfg->eMode = TIMER4E_MODE_INTERVAL;
        ptCfg->un16IntrEnable = TIMER4E_INTR_MATCH_CH_B_UP_DOWN | TIMER4E_INTR_MATCH_CH_A_UP_DOWN | TIMER4E_INTR_PERIOD_MATCH | TIMER4E_INTR_OUTPUT_FORCE;
    }
    else if ((pn8Argv[un8Arg][0]) == 'c') 
    {
        ptCfg->eMode = TIMER4E_MODE_CAPTURE;
        ptCfg->un16IntrEnable = TIMER4E_INTR_CAPTURE;
    }
    else if ((pn8Argv[un8Arg][0]) == 'b') 
    {
        ptCfg->eMode = TIMER4E_MODE_BACK2BACK;
        ptCfg->un16IntrEnable = TIMER4E_INTR_MATCH_CH_B_UP_DOWN | TIMER4E_INTR_MATCH_CH_A_UP_DOWN | TIMER4E_INTR_PERIOD_MATCH | TIMER4E_INTR_BOTTOM | TIMER4E_INTR_OUTPUT_FORCE;
    }
    else if ((pn8Argv[un8Arg][0]) == 'o') 
    {
        ptCfg->eMode = TIMER4E_MODE_ONESHOT_INTERVAL;
        ptCfg->un16IntrEnable = TIMER4E_INTR_MATCH_CH_B_UP_DOWN | TIMER4E_INTR_MATCH_CH_A_UP_DOWN | TIMER4E_INTR_PERIOD_MATCH | TIMER4E_INTR_BOTTOM | TIMER4E_INTR_OUTPUT_FORCE;
    }
    else
    {
        EX_COMMON_SetShowModuleLog(EX_TIMER4E_ERR_STR, NULL, EX_COMM_STR_OPT_MODE);
        goto err;
    }

    if(ptCfg->eMode == TIMER4E_MODE_CAPTURE)
    {
        un8Arg++;
        if ((pn8Argv[un8Arg][0] == 'r'))
            ptCfg->eCapClr = TIMER4E_CAP_CLR_RISING;
        else if ((pn8Argv[un8Arg][0] == 'f'))
            ptCfg->eCapClr = TIMER4E_CAP_CLR_FALLING;
        else if ((pn8Argv[un8Arg][0] == 'b'))
            ptCfg->eCapClr = TIMER4E_CAP_CLR_BOTH;
        else
        {
            EX_COMMON_SetShowModuleLog(EX_TIMER4E_ERR_STR, NULL, EX_COMM_STR_OPT_EDGE);
            goto err;
        }
        un8Arg++;
    }
    else
    {
        un8Arg++;
    }

    if(strncmp(pn8Argv[un8Arg],"-port",5) == 0)
    {
        un8Arg++;
        if(strncmp(pn8Argv[un8Arg],"-pa",3) == 0)
        {
            un8Arg++;
            eDbgStatus = EX_TIMER4E_GetPortConfig(un8Arg, &pn8Argv[un8Arg], &ptCfg->tOutputsCfg.tAPortCfg);
            if(eDbgStatus != DEBUG_CMD_SUCCESS)
            {
                EX_COMMON_SetShowModuleLog(EX_TIMER4E_ERR_STR, "-pa", EX_COMM_STR_OPT_MAX);
                goto err;
            }
            un8Arg += 2;
        }

        if(strncmp(pn8Argv[un8Arg],"-pb",3) == 0)
        {
            un8Arg++;
            eDbgStatus = EX_TIMER4E_GetPortConfig(un8Arg, &pn8Argv[un8Arg], &ptCfg->tOutputsCfg.tBPortCfg);
            if(eDbgStatus != DEBUG_CMD_SUCCESS)
            {
                EX_COMMON_SetShowModuleLog(EX_TIMER4E_ERR_STR, "-pb", EX_COMM_STR_OPT_MAX);
                goto err;
            }
            un8Arg += 2;
        }
    }

    if(strncmp(pn8Argv[un8Arg],"-dly",4) == 0)
    {
        un8Arg++;
        ptCfg->tDlyCfg.bEnable = true;
        switch (pn8Argv[un8Arg++][0])
        {
            case 'f':
                ptCfg->tDlyCfg.ePos = TIMER4E_DLY_POS_AB;
                break;
            case 'b':
                ptCfg->tDlyCfg.ePos = TIMER4E_DLY_POS_BA;
                break;
            default:
                EX_COMMON_SetShowModuleLog(EX_TIMER4E_ERR_STR, "-dly[msk]", EX_COMM_STR_OPT_MAX);
                goto err;
        }
        ptCfg->tDlyCfg.un16Value = (uint16_t)atoi(pn8Argv[un8Arg++]);
    }

    if(strncmp(pn8Argv[un8Arg],"-updt",5) == 0)
    {
        un8Arg++;
        if (pn8Argv[un8Arg][0] == 'i')
            ptCfg->eReload = TIMER4E_RELOAD_INSTANT;
        else if (pn8Argv[un8Arg][0] == 'p')
            ptCfg->eReload = TIMER4E_RELOAD_PERIOD_MATCH;
        else if (pn8Argv[un8Arg][0] == 'b')
            ptCfg->eReload = TIMER4E_RELOAD_BOTTOM;
        else 
        {
            EX_COMMON_SetShowModuleLog(EX_TIMER4E_ERR_STR, "updt[reload]", EX_COMM_STR_OPT_MAX);
            goto err;
        }
        un8Arg++;
    }

    ptCfg->utData.tGRD.un16DataP = (uint16_t)atoi(pn8Argv[un8Arg++]);
    ptCfg->utData.tGRD.un16DataA = (uint16_t)atoi(pn8Argv[un8Arg++]);
    ptCfg->utData.tGRD.un16DataB = (uint16_t)atoi(pn8Argv[un8Arg++]);

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_TIMER4E_GetId(int32_t n32Argc, char *pn8Argv[], TIMER4E_ID_e *peId)
{
    uint8_t un8Data = 0;
    if(n32Argc == 1)
    {
        EX_COMMON_SetShowModuleLog(EX_TIMER4E_ERR_STR, "arg", EX_COMM_STR_OPT_MAX);
        return DEBUG_CMD_INVALID;
    }

    un8Data = atoi(pn8Argv[1]); 
    if(un8Data >= CONFIG_TIMER4E_MAX_COUNT)
    {
        LOG("%s Max Chan %d\n", EX_TIMER4E_ERR_STR, CONFIG_TIMER4E_MAX_COUNT);
        return DEBUG_CMD_INVALID;
    }

    *peId = (TIMER4E_ID_e)un8Data;

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_TIMER4E_Init(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    TIMER4E_ID_e eId;

    eDbgStatus = EX_TIMER4E_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_TIMER4E_Init(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_TIMER4E_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_INIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_TIMER4E_Uninit(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    TIMER4E_ID_e eId;

    eDbgStatus = EX_TIMER4E_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_TIMER4E_Uninit(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_TIMER4E_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_UNINIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_TIMER4E_SetClk(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    TIMER4E_ID_e eId;

    TIMER4E_CLK_CFG_t tClkCfg =
    {
        .eClk = TIMER4E_CLK_PCLK,
        .un16PreScale = 0,
    };

    eDbgStatus = EX_TIMER4E_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eDbgStatus = EX_TIMER4E_GetClkConfig(n32Argc, pn8Argv, &tClkCfg);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_TIMER4E_SetClkConfig(eId, &tClkCfg);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_TIMER4E_SetConfig(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    TIMER4E_ID_e eId;
    TIMER4E_OPS_e eOps;
    TIMER4E_CFG_t tTimer4eCfg;
    bool bBlnkNonMask = false;

    eDbgStatus = EX_TIMER4E_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    memset(&tTimer4eCfg, 0x00, sizeof(TIMER4E_CFG_t));

    eDbgStatus = EX_TIMER4E_GetConfig(n32Argc, pn8Argv, &eOps, &tTimer4eCfg, &bBlnkNonMask);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_TIMER4E_SetConfig(eId, &tTimer4eCfg);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    if(eOps == TIMER4E_OPS_INTR)
    {
        eErr = HAL_TIMER4E_SetIRQ(eId, eOps, EX_TIMER4E_IRQHandler, (void *)&s_tTimer4eContext[(uint32_t)eId], EX_TIMER4E_IRQ_PRIO, bBlnkNonMask);
        if(eErr != HAL_ERR_OK)
        {
            return DEBUG_CMD_INVALID;
        }
    }

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_TIMER4E_SetPort(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    TIMER4E_ID_e eId;
    TIMER4E_OUTPUTS_CFG_t tCfg;
    
    if(n32Argc != 4)
    {
        return DEBUG_CMD_INVALID;
    }

    eDbgStatus = EX_TIMER4E_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }
    
    eErr = HAL_TIMER4E_SetOutputPort(eId, &tCfg);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_TIMER4E_SetAdcTrg(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    TIMER4E_ID_e eId;
    uint32_t un32AdcTrgEnable = 0;
    TIMER4E_ADCTRG_CFG_t tCfg;

    if(n32Argc != 3)
    {
        return DEBUG_CMD_INVALID;
    }

    eDbgStatus = EX_TIMER4E_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    memset(&tCfg, 0x00, sizeof(TIMER4E_ADCTRG_CFG_t));
    
    sscanf(pn8Argv[2], "%X", &un32AdcTrgEnable);

    tCfg.un16Enable = (uint16_t)un32AdcTrgEnable;

    eErr = HAL_TIMER4E_SetAdcTrgConfig(eId, &tCfg);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_TIMER4E_Start(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    TIMER4E_ID_e eId;

    eDbgStatus = EX_TIMER4E_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    s_tTimer4eContext[eId].eId = eId;

    eErr = HAL_TIMER4E_Start(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_TIMER4E_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_START]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_TIMER4E_Stop(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    TIMER4E_ID_e eId;

    eDbgStatus = EX_TIMER4E_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_TIMER4E_Stop(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_TIMER4E_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_STOP]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_TIMER4E_SetLog(int32_t n32Argc, char *pn8Argv[])
{
    if((strncmp(pn8Argv[1],"on",2) == 0))
    {
        un32ISRLog = 1;
    }
    else
    {
        un32ISRLog = 0;
    }

    LOG("%s ISR Log %s.\n", EX_TIMER4E_LOG_STR, (un32ISRLog == 1 ? "on":"off"));

    return DEBUG_CMD_SUCCESS;
}

static struct debug_cmd s_tEX_TIMER4E_CMD[] =
{
    {EX_TIMER4E_STR, "h", EX_TIMER4E_Help, "help"},
    {EX_TIMER4E_STR, "init", EX_TIMER4E_Init,""},
    {EX_TIMER4E_STR, "uninit", EX_TIMER4E_Uninit,""},
    {EX_TIMER4E_STR, "clk", EX_TIMER4E_SetClk,""},
    {EX_TIMER4E_STR, "config",EX_TIMER4E_SetConfig,""},
    {EX_TIMER4E_STR, "port", EX_TIMER4E_SetPort,""},
    {EX_TIMER4E_STR, "adctrg", EX_TIMER4E_SetAdcTrg,""},
    {EX_TIMER4E_STR, "start", EX_TIMER4E_Start,""},
    {EX_TIMER4E_STR, "stop", EX_TIMER4E_Stop,""},
    {EX_TIMER4E_STR, "log", EX_TIMER4E_SetLog,""}
};

/**********************************************************************
 * @brief		Main program
 * @param[in]	None
 * @return	None
 **********************************************************************/
void EX_TIMER4E(void)
{
    /* Add EX commands */
    debug_cmd_init(s_tEX_TIMER4E_CMD,DEBUG_CMD_LIST_COUNT(s_tEX_TIMER4E_CMD));
}

#endif
/* --------------------------------- End Of File ------------------------------ */
