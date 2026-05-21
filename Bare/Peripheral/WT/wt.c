/**
 *******************************************************************************
 * @file        wt.c
 * @author      ABOV R&D Division
 * @brief       WT Example Code
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

#if defined(WT_TC)
#include "abov_config.h"
#include "ex_configs.h"
#include "ex_common.h"
#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "hal_scu.h"
#include "hal_scu_clk.h"
#include "hal_pcu.h"
#include "hal_wt.h"

#if !defined(_WT)
#error "This chipset did not support this example."
#endif

#define EX_WT_STR "WT"
#define EX_WT_LOG_STR "WT :"
#define EX_WT_ERR_STR "[E]WT :"
#define EX_WT_IRQ_PRIO 3
#define EX_WT_STOP_CNT 10
#define EX_WT_MAX_NUM  (CONFIG_WT_MAX_COUNT - 1)

extern const char *pcCommStr[];
extern const char *pcCmdStr[];
extern const char *pcValStr[];
extern const char *pcOptStr[];

extern uint32_t SystemCoreClock;
static WT_Context_t s_tWdtContext[CONFIG_WT_MAX_COUNT];
static WT_OPS_e s_eOps = WT_OPS_POLL;

static enum debug_cmd_status EX_WT_Help(int32_t n32Argc, char *pn8Argv[])
{
    uint32_t un32ShowOpt = 0;
    EX_COMM_STR_OPT_e eOpt[2];

    EX_COMMON_SetShowModuleInfo(EX_WT_STR, CONFIG_WT_MAX_COUNT);

    un32ShowOpt = EX_COMMON_GetShowOpt(pn8Argv[1]);

    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_INIT, NULL, EX_WT_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_UNINIT, NULL, EX_WT_MAX_NUM, eOpt, 0, NULL);

    eOpt[0] = EX_COMM_STR_OPT_SRC; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CLK, NULL, EX_WT_MAX_NUM, eOpt, 1, NULL);
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_SRC, EX_COMM_STR_VAL_MAX, "m(mccr)/l(lse)/w(wdtrc)");
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MCCR, EX_COMM_STR_VAL_MAX, "[mccr] [div]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MCCR, EX_COMM_STR_VAL_CLKSRC, "/m(mclk)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_DIV, EX_COMM_STR_VAL_N_255, NULL);
    }

    eOpt[0] = EX_COMM_STR_OPT_OPS; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CONFIG, NULL, EX_WT_MAX_NUM, eOpt, 0, "[intv] [da]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_OPS, EX_COMM_STR_VAL_OPS, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "intv: s(7)/t(13)/f(14)/w(14xDR)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_DA, EX_COMM_STR_VAL_N_NUM, NULL);
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_START, NULL, EX_WT_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_STOP, NULL, EX_WT_MAX_NUM, eOpt, 0, NULL);
    LOG("\n");

    return DEBUG_CMD_SUCCESS;
}

static void EX_WT_IRQHandler(uint32_t un32Event, void *pContext)
{
    if(un32Event & WT_EVENT_MATCH)
    {
        LOG("%s (%d) M evt fired\n", EX_WT_LOG_STR, ((WT_Context_t *)pContext)->eId);
    }
}

static enum debug_cmd_status EX_WT_GetId(int32_t n32Argc, char *pn8Argv[], WT_ID_e *peId)
{
    uint8_t un8Data = 0;
    if(n32Argc == 1)
    {
        EX_COMMON_SetShowModuleLog(EX_WT_ERR_STR, (char *)pcOptStr[EX_COMM_STR_OPT_ARG], EX_COMM_STR_OPT_MAX);
        goto err;
    }

    un8Data = atoi(pn8Argv[1]); 
    if(un8Data >= CONFIG_WT_MAX_COUNT)
    {
        LOG("%s Max Chan %d\n", EX_WT_ERR_STR, CONFIG_WT_MAX_COUNT);
        goto err;
    }

    *peId = (WT_ID_e)un8Data;

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_WT_GetClkConfig(int32_t n32Argc, char *pn8Argv[], WT_CLK_CFG_t *ptClkCfg)
{
    uint8_t un8Arg = 2;

    if (pn8Argv[un8Arg][0] == 'l')
        ptClkCfg->eClk = WT_CLK_LSE;
    else if (pn8Argv[un8Arg][0] == 'm')
        ptClkCfg->eClk = WT_CLK_MCCR;
    else if (pn8Argv[un8Arg][0] == 'w')
        ptClkCfg->eClk = WT_CLK_WDTRC;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_WT_ERR_STR, NULL, EX_COMM_STR_OPT_SRC);
        goto err;
    }

    if(ptClkCfg->eClk == WT_CLK_MCCR)
    {
        un8Arg++;
        if (pn8Argv[un8Arg][0] == 'l')
            ptClkCfg->eMccr = WT_CLK_MCCR_LSI;
        else if (pn8Argv[un8Arg][0] == 's')
            ptClkCfg->eMccr = WT_CLK_MCCR_LSE;
        else if (pn8Argv[un8Arg][0] == 'm')
            ptClkCfg->eMccr = WT_CLK_MCCR_MCLK;
        else if (pn8Argv[un8Arg][0] == 'h')
            ptClkCfg->eMccr = WT_CLK_MCCR_HSI;
        else if (pn8Argv[un8Arg][0] == 'e')
            ptClkCfg->eMccr = WT_CLK_MCCR_HSE;
        else if (pn8Argv[un8Arg][0] == 'p')
            ptClkCfg->eMccr = WT_CLK_MCCR_PLL;
        else
        {
            EX_COMMON_SetShowModuleLog(EX_WT_ERR_STR, "src", EX_COMM_STR_OPT_MCCR);
            goto err;
        }

        ptClkCfg->un8MccrDiv = atoi(pn8Argv[un8Arg++]);
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_WT_GetConfig(int32_t n32Argc, char *pn8Argv[], WT_CFG_t *ptCfg, WT_OPS_e *peOps)
{
    uint8_t un8Arg = 2;
    WT_OPS_e eOps;

    switch (pn8Argv[un8Arg++][0])
    {
        case 'p':
            eOps = WT_OPS_POLL;
            ptCfg->bIntrEnable = false;
            break;
        case 'i':
            eOps = WT_OPS_INTR;
            ptCfg->bIntrEnable = true;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_WT_ERR_STR, NULL, EX_COMM_STR_OPT_OPS);
            goto err;
    }

    *peOps = eOps;

    if (pn8Argv[un8Arg][0] == 's')
        ptCfg->eIntv = WT_INTV_2_7;
    else if (pn8Argv[un8Arg][0] == 't')
        ptCfg->eIntv = WT_INTV_2_13;
    else if (pn8Argv[un8Arg][0] == 'f')
        ptCfg->eIntv = WT_INTV_2_14;
    else if (pn8Argv[un8Arg][0] == 'w')
        ptCfg->eIntv = WT_INTV_2_14_MULY_DATA_REG;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_WT_ERR_STR, "[prescl]", EX_COMM_STR_OPT_MAX);
        goto err;
    }

    if(ptCfg->eIntv == WT_INTV_2_14_MULY_DATA_REG)
    {
        ptCfg->un16MatchCnt = (uint32_t)atoi(pn8Argv[++un8Arg]);
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_WT_Init(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    WT_ID_e eId = WT_ID_0;

    eDbgStatus = EX_WT_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_WT_Init(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_WT_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_INIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_WT_Uninit(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    WT_ID_e eId = WT_ID_0;

    eDbgStatus = EX_WT_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_WT_Uninit(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_WT_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_UNINIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_WT_SetClk(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    WT_ID_e eId;
    WT_CLK_CFG_t tClkCfg; 

    eDbgStatus = EX_WT_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    memset(&tClkCfg, 0x00, sizeof(WT_CLK_CFG_t));

    eDbgStatus = EX_WT_GetClkConfig(n32Argc, pn8Argv, &tClkCfg);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_WT_SetClkConfig(eId, &tClkCfg);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_WT_ERR_STR, "ClkCfg", EX_COMM_STR_OPT_MAX);
        }
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_WT_SetConfig(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    WT_ID_e eId = WT_ID_0;
    WT_CFG_t tCfg;
   
    eDbgStatus = EX_WT_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    memset(&tCfg, 0x00, sizeof(WT_CFG_t));

    eDbgStatus = EX_WT_GetConfig(n32Argc, pn8Argv, &tCfg, &s_eOps);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_WT_SetConfig(eId, &tCfg);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_WT_ERR_STR, "Cfg", EX_COMM_STR_OPT_MAX);
        }

        goto err;
    }

    eErr = HAL_WT_SetIRQ(eId, s_eOps, EX_WT_IRQHandler, (void *)&s_tWdtContext[(uint32_t)eId], EX_WT_IRQ_PRIO);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_WT_Start(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    WT_ID_e eId;
    uint32_t un32StopCnt = EX_WT_STOP_CNT;
    bool bMatched = false;

    eDbgStatus = EX_WT_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    s_tWdtContext[eId].eId = eId;

    eErr = HAL_WT_Start(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_WT_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_START]);

    if(s_eOps == WT_OPS_POLL)
    {
        while(1)
        {
            eErr = HAL_WT_GetEvent(eId, &bMatched);
            if(eErr != HAL_ERR_OK)
            {
                LOG("%s (%d) GetEvent\n",EX_WT_ERR_STR, eId);
                break;
            }
            
            if(bMatched == true)
            {
                LOG("%s (%d) cnt value matched(%d)\n",EX_WT_LOG_STR, eId, un32StopCnt);
                un32StopCnt--; 
                if(un32StopCnt == 0)
                {
                    (void)HAL_WT_Stop(eId);
                    break;
                }
            }
        }
    }
    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_WT_Stop(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    WT_ID_e eId;

    eDbgStatus = EX_WT_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_WT_Stop(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_WT_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_STOP]);

    return DEBUG_CMD_SUCCESS;
}

static const struct debug_cmd s_tEX_WT_CMD[] =
{
    {"WT", "h", EX_WT_Help,"help"},
    {"WT", "init", EX_WT_Init, ""},
    {"WT", "uninit", EX_WT_Uninit, ""},
    {"WT", "clk", EX_WT_SetClk, ""},
    {"WT", "config", EX_WT_SetConfig, ""},
    {"WT", "start", EX_WT_Start, ""},
    {"WT", "stop", EX_WT_Stop, ""}
};

void EX_WT(void)
{
    /* Add TC commands */
    debug_cmd_init(s_tEX_WT_CMD, DEBUG_CMD_LIST_COUNT(s_tEX_WT_CMD));
}

#endif /* WT_TC */

/* --------------------------------- End Of File ------------------------------ */
