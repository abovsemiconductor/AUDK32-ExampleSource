/**
 *******************************************************************************
 * @file        wdt.c
 * @author      ABOV R&D Division
 * @brief       WDT Example Code
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

#if defined(WDT_TC)
#include "abov_config.h"
#include "ex_common.h"
#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "hal_scu.h"
#include "hal_scu_clk.h"
#include "hal_pcu.h"
#include "hal_wdt.h"

#if !defined(_WDT)
#error "This chipset did not support this example."
#endif

#define EX_WDT_STR "WDT"
#define EX_WDT_LOG_STR "WDT :"
#define EX_WDT_ERR_STR "[E]WDT :"
#define EX_WDT_IRQ_PRIO 3
#define EX_WDT_STOP_CNT 10
#define EX_WDT_MAX_NUM  (CONFIG_WDT_MAX_COUNT - 1)

extern const char *pcCommStr[];
extern const char *pcCmdStr[];
extern const char *pcValStr[];
extern const char *pcOptStr[];

extern uint32_t SystemCoreClock;
static WDT_Context_t s_tWdtContext[CONFIG_WDT_MAX_COUNT];
static WDT_OPS_e s_eOps = WDT_OPS_POLL;
static uint8_t s_un8Mode = 0;
static uint32_t s_un32MatchCnt = 0, s_un32InitCnt = 0;

static enum debug_cmd_status EX_WDT_Help(int32_t n32Argc, char *pn8Argv[])
{
    uint32_t un32ShowOpt = 0;
    EX_COMM_STR_OPT_e eOpt[2];

    EX_COMMON_SetShowModuleInfo(EX_WDT_STR, CONFIG_WDT_MAX_COUNT);

    un32ShowOpt = EX_COMMON_GetShowOpt(pn8Argv[1]);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_INIT, NULL, EX_WDT_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_UNINIT, NULL, EX_WDT_MAX_NUM, eOpt, 0, NULL);

    eOpt[0] = EX_COMM_STR_OPT_SRC; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CLK, NULL, EX_WDT_MAX_NUM, eOpt, 1, "[prescl]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_SRC, EX_COMM_STR_VAL_MAX, "m(mccr)/l(lse)/w(wdtrc)");
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MCCR, EX_COMM_STR_VAL_MAX, "[mccr] [div]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MCCR, EX_COMM_STR_VAL_CLKSRC, "/m(mclk)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_DIV, EX_COMM_STR_VAL_N_255, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "prescl: 1,4,8,16,32,64,128,256");
    }

    eOpt[0] = EX_COMM_STR_OPT_OPS; 
    eOpt[1] = EX_COMM_STR_OPT_MODE; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CONFIG, NULL, EX_WDT_MAX_NUM, eOpt, 2, "[init] [match]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_OPS, EX_COMM_STR_VAL_OPS, NULL);
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "i: [msk]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MASK, EX_COMM_STR_VAL_MASK, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MODE, EX_COMM_STR_VAL_MAX, "c(match)/r(reset)/u(underflow)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "init: 0~N");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "match: 0~N");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_START, NULL, EX_WDT_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_STOP, NULL, EX_WDT_MAX_NUM, eOpt, 0, NULL);
    LOG("\n");

    return DEBUG_CMD_SUCCESS;
}

static void EX_WDT_IRQHandler(uint32_t un32Event, void *pContext)
{
    WDT_Context_t *ptContext = (WDT_Context_t *)pContext;

    if(un32Event & WDT_EVENT_MATCH)
    {
        LOG("%s (%d) M evt fired\n", EX_WDT_LOG_STR, ptContext->eId);
    }

    if(un32Event & WDT_EVENT_UNDERFLOW)
    {
        LOG("%s (%d) U evt fired\n", EX_WDT_LOG_STR, ptContext->eId);
    }

    if(!(ptContext->un8Mode & WDT_MODE_RST) && !(ptContext->un8Mode & WDT_MODE_UNDERFLOW))
    {
        HAL_WDT_SetReload(ptContext->eId, s_un32InitCnt);
    }
}

static enum debug_cmd_status EX_WDT_GetId(int32_t n32Argc, char *pn8Argv[], WDT_ID_e *peId)
{
    uint8_t un8Data = 0;
    if(n32Argc == 1)
    {
        EX_COMMON_SetShowModuleLog(EX_WDT_ERR_STR, (char *)pcOptStr[EX_COMM_STR_OPT_ARG], EX_COMM_STR_OPT_MAX);
        return DEBUG_CMD_INVALID;
    }

    un8Data = atoi(pn8Argv[1]); 
    if(un8Data >= CONFIG_WDT_MAX_COUNT)
    {
        LOG("%s Max Chan %d\n", EX_WDT_ERR_STR, CONFIG_WDT_MAX_COUNT);
        return DEBUG_CMD_INVALID;
    }

    *peId = (WDT_ID_e)un8Data;

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_WDT_GetClkConfig(int32_t n32Argc, char *pn8Argv[], WDT_CLK_CFG_t *ptClkCfg)
{
    uint8_t un8Arg = 2;
    uint16_t un16Data = 0;

    if (pn8Argv[un8Arg][0] == 'p')
        ptClkCfg->eClk = WDT_CLK_PCLK;
    else if (pn8Argv[un8Arg][0] == 'm')
        ptClkCfg->eClk = WDT_CLK_MCCR;
    else if (pn8Argv[un8Arg][0] == 'w')
        ptClkCfg->eClk = WDT_CLK_WDTRC;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_WDT_ERR_STR, NULL, EX_COMM_STR_OPT_SRC);
        goto err;
    }

    un8Arg++;
    if(ptClkCfg->eClk == WDT_CLK_MCCR)
    {
        if (pn8Argv[un8Arg][0] == 'l')
            ptClkCfg->eMccr = WDT_CLK_MCCR_LSI;
        else if (pn8Argv[un8Arg][0] == 's')
            ptClkCfg->eMccr = WDT_CLK_MCCR_LSE;
        else if (pn8Argv[un8Arg][0] == 'm')
            ptClkCfg->eMccr = WDT_CLK_MCCR_MCLK;
        else if (pn8Argv[un8Arg][0] == 'h')
            ptClkCfg->eMccr = WDT_CLK_MCCR_HSI;
        else if (pn8Argv[un8Arg][0] == 'e')
            ptClkCfg->eMccr = WDT_CLK_MCCR_HSE;
        else if (pn8Argv[un8Arg][0] == 'p')
            ptClkCfg->eMccr = WDT_CLK_MCCR_PLL;
        else
        {
            EX_COMMON_SetShowModuleLog(EX_WDT_ERR_STR, "src", EX_COMM_STR_OPT_MCCR);
            goto err;
        }

        un8Arg++;
        ptClkCfg->un8MccrDiv = atoi(pn8Argv[un8Arg++]);

    }
    
    un16Data = ((uint16_t)atoi(pn8Argv[un8Arg++]));
    if (un16Data == 1)
        ptClkCfg->ePreDiv = WDT_CLK_PREDIV_1;
    else if (un16Data == 4)
        ptClkCfg->ePreDiv = WDT_CLK_PREDIV_4;
    else if (un16Data == 8)
        ptClkCfg->ePreDiv = WDT_CLK_PREDIV_8;
    else if (un16Data == 16)
        ptClkCfg->ePreDiv = WDT_CLK_PREDIV_16;
    else if (un16Data == 32)
        ptClkCfg->ePreDiv = WDT_CLK_PREDIV_32;
    else if (un16Data == 64)
        ptClkCfg->ePreDiv = WDT_CLK_PREDIV_64;
    else if (un16Data == 128)
        ptClkCfg->ePreDiv = WDT_CLK_PREDIV_128;
    else if (un16Data == 256)
        ptClkCfg->ePreDiv = WDT_CLK_PREDIV_256;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_WDT_ERR_STR, "[prescl]", EX_COMM_STR_OPT_MAX);
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;

}

static enum debug_cmd_status EX_WDT_GetConfig(int32_t n32Argc, char *pn8Argv[], WDT_CFG_t *ptCfg, WDT_OPS_e *peOps, bool *pbNonMask)
{

    uint8_t un8Arg = 2;
    WDT_OPS_e eOps;

    switch (pn8Argv[un8Arg++][0])
    {
        case 'p':
            eOps = WDT_OPS_POLL;
            break;
        case 'i':
            eOps = WDT_OPS_INTR;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_WDT_ERR_STR, NULL, EX_COMM_STR_OPT_OPS);
            goto err;
    }

    *peOps = eOps;

    if(eOps == WDT_OPS_INTR)
    {
        switch (pn8Argv[un8Arg++][0])
        {
            case 'n':
                *pbNonMask = true;
                break;
            case 'm':
                *pbNonMask = false;
                break;
            default:
                EX_COMMON_SetShowModuleLog(EX_WDT_ERR_STR, "ops", EX_COMM_STR_OPT_MASK);
                goto err;
        }
    }

    switch (pn8Argv[un8Arg++][0])
    {
        case 'c':
            ptCfg->un8Mode = WDT_MODE_CNT;
            break;
        case 'r':
            ptCfg->un8Mode = WDT_MODE_CNT | WDT_MODE_RST;
            break;
        case 'u':
            ptCfg->un8Mode = WDT_MODE_CNT | WDT_MODE_UNDERFLOW;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_WDT_ERR_STR, NULL, EX_COMM_STR_OPT_MODE);
            goto err;
    }

    if (eOps == WDT_OPS_INTR)
    {
        if(ptCfg->un8Mode & WDT_MODE_UNDERFLOW)
        {
            ptCfg->un8IntrEnable = WDT_INTR_UNDERFLOW;
        }
        else
        {
            ptCfg->un8IntrEnable = WDT_INTR_MATCH;
        }
    }

    ptCfg->un32InitCnt = (uint32_t)atoi(pn8Argv[un8Arg++]);
    ptCfg->un32MatchCnt = (uint32_t)atoi(pn8Argv[un8Arg++]);

    s_un32InitCnt = ptCfg->un32InitCnt;
    if(eOps == WDT_OPS_POLL)
    {
        s_un8Mode = ptCfg->un8Mode;
        s_un32MatchCnt = ptCfg->un32MatchCnt; 
    }
    else
    {
        s_un8Mode = 0;
        s_un32MatchCnt = 0;
    } 

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_WDT_Init(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    WDT_ID_e eId = WDT_ID_0;

    eDbgStatus = EX_WDT_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_WDT_Init(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_WDT_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_INIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_WDT_Uninit(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    WDT_ID_e eId = WDT_ID_0;

    eDbgStatus = EX_WDT_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_WDT_Uninit(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_WDT_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_UNINIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_WDT_SetClk(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    WDT_ID_e eId;
    WDT_CLK_CFG_t tClkCfg; 

    eDbgStatus = EX_WDT_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    memset(&tClkCfg, 0x00, sizeof(WDT_CLK_CFG_t));

    eDbgStatus = EX_WDT_GetClkConfig(n32Argc, pn8Argv, &tClkCfg);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_WDT_SetClkConfig(eId, &tClkCfg);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_WDT_ERR_STR, "ClkCfg", EX_COMM_STR_OPT_MAX);
        }
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_WDT_SetConfig(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    WDT_ID_e eId = WDT_ID_0;
    bool bNonMask = false;
    WDT_CFG_t tCfg;
   
    eDbgStatus = EX_WDT_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    memset(&tCfg, 0x00, sizeof(WDT_CFG_t));

    eDbgStatus = EX_WDT_GetConfig(n32Argc, pn8Argv, &tCfg, &s_eOps, &bNonMask);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_WDT_SetConfig(eId, &tCfg);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_WDT_ERR_STR, "Cfg", EX_COMM_STR_OPT_MAX);
        }
        goto err;
    }

    eErr = HAL_WDT_SetIRQ(eId, s_eOps, EX_WDT_IRQHandler, (void *)&s_tWdtContext[(uint32_t)eId], EX_WDT_IRQ_PRIO, bNonMask);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_WDT_Start(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    WDT_ID_e eId;
    uint32_t un32Cnt = 0, un32StopCnt = EX_WDT_STOP_CNT;
    

    eDbgStatus = EX_WDT_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    s_tWdtContext[eId].eId = eId;

    eErr = HAL_WDT_Start(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_WDT_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_START]);

    if(s_eOps == WDT_OPS_POLL)
    {
        while(1)
        {
            SystemDelayMS(1);
            eErr = HAL_WDT_GetCount(eId, &un32Cnt);
            if(eErr != HAL_ERR_OK)
            {
                break;
            }
   
            if(un32Cnt < s_un32MatchCnt)
            {
                if(s_un8Mode == (WDT_MODE_RST | WDT_MODE_CNT))
                {
                    if(un32StopCnt != 0)
                    {
                        LOG("%s (%d) cnt value under match and re-load(%d)\n", EX_WDT_LOG_STR, eId, un32StopCnt);
                        (void)HAL_WDT_SetReload(eId, s_un32InitCnt);
                        un32StopCnt--;
                    }
                }
                else
                {
                    LOG("%s (%d) cnt value under match value\n", EX_WDT_LOG_STR, eId);
                    (void)HAL_WDT_Stop(eId);
                    break;
                }
            }
        }
    }

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_WDT_Stop(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    WDT_ID_e eId;

    eDbgStatus = EX_WDT_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_WDT_Stop(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_WDT_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_STOP]);

    return DEBUG_CMD_SUCCESS;
}

static const struct debug_cmd s_tEX_WDT_CMD[] =
{
    {"WDT", "h", EX_WDT_Help,"help"},
    {"WDT", "init", EX_WDT_Init, ""},
    {"WDT", "uninit", EX_WDT_Uninit, ""},
    {"WDT", "clk", EX_WDT_SetClk, ""},
    {"WDT", "config", EX_WDT_SetConfig, ""},
    {"WDT", "start", EX_WDT_Start, ""},
    {"WDT", "stop", EX_WDT_Stop, ""}
};

/**********************************************************************
 * @brief		Main program
 * @param[in]	None
 * @return	None
 **********************************************************************/
void EX_WDT(void)
{
    /* Add TC commands */
    debug_cmd_init(s_tEX_WDT_CMD, DEBUG_CMD_LIST_COUNT(s_tEX_WDT_CMD));
}

#endif /* WDT_TC */

/* --------------------------------- End Of File ------------------------------ */
