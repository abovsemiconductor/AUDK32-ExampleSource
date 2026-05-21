/**
 *******************************************************************************
 * @file        frt.c
 * @author      ABOV R&D Division
 * @brief       FRT Example Code
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

#if defined(FRT_TC)
#include "abov_config.h"
#include "ex_common.h"
#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "hal_scu.h"
#include "hal_scu_clk.h"
#include "hal_frt.h"

#if !defined(_FRT)
#error "This chipset did not support this example."
#endif

#define EX_FRT_STR "FRT"
#define EX_FRT_LOG_STR "FRT :"
#define EX_FRT_ERR_STR "[E]FRT :"
#define EX_FRT_IRQ_PRIO 3
#define EX_FRT_DEFAULT_TIMEOUT 1000000
#define EX_FRT_MAX_NUM  (CONFIG_FRT_MAX_COUNT - 1)

extern const char *pcCommStr[];
extern const char *pcCmdStr[];
extern const char *pcValStr[];
extern const char *pcOptStr[];

static FRT_Context_t s_tFrtContext[CONFIG_FRT_MAX_COUNT];
static FRT_OPS_e s_eOps = FRT_OPS_POLL;

static enum debug_cmd_status EX_FRT_Help(int32_t n32Argc, char *pn8Argv[])
{
    uint32_t un32ShowOpt = 0;
    EX_COMM_STR_OPT_e eOpt[7];

    EX_COMMON_SetShowModuleInfo(EX_FRT_STR, CONFIG_FRT_MAX_COUNT);

    un32ShowOpt = EX_COMMON_GetShowOpt(pn8Argv[1]);

    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_INIT, NULL, EX_FRT_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_UNINIT, NULL, EX_FRT_MAX_NUM, eOpt, 0, NULL);

    eOpt[0] = EX_COMM_STR_OPT_SRC; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CLK, NULL, EX_FRT_MAX_NUM, eOpt, 1, "[prescl]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_SRC, EX_COMM_STR_VAL_CLKPATH, NULL);
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MCCR, EX_COMM_STR_VAL_MAX, "[mccr] [div]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MCCR, EX_COMM_STR_VAL_CLKSRC, "/m(mclk)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_DIV, EX_COMM_STR_VAL_N_255, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "prescl: 1,4,8,16,32,64,128,256");
    }
    eOpt[0] = EX_COMM_STR_OPT_OPS; 
    eOpt[1] = EX_COMM_STR_OPT_MODE; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CONFIG, NULL, EX_FRT_MAX_NUM, eOpt, 2, NULL);
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_OPS, EX_COMM_STR_VAL_OPS, "/n(nmi)");
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "m: [match]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "match: 0~N(32bit hexa)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MODE, EX_COMM_STR_VAL_MAX, "f(freerun)/m(match)");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_START, NULL, EX_FRT_MAX_NUM, eOpt, 0, "[-res]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-res: resume(no clear cnt value)");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_STOP, NULL, EX_FRT_MAX_NUM, eOpt, 0, NULL);
    LOG("\n");

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_FRT_GetId(int32_t n32Argc, char *pn8Argv[], FRT_ID_e *peId)
{
    uint8_t un8Data = 0;
    if(n32Argc == 1)
    {
        EX_COMMON_SetShowModuleLog(EX_FRT_ERR_STR, (char *)pcOptStr[EX_COMM_STR_OPT_ARG], EX_COMM_STR_OPT_MAX);
        return DEBUG_CMD_INVALID;
    }

    un8Data = atoi(pn8Argv[1]); 
    if(un8Data >= CONFIG_FRT_MAX_COUNT)
    {
        LOG("%s Max Chan %d\n", EX_FRT_ERR_STR, CONFIG_FRT_MAX_COUNT);
        return DEBUG_CMD_INVALID;
    }

    *peId = (FRT_ID_e)un8Data;

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_FRT_GetClkConfig(int32_t n32Argc, char *pn8Argv[], FRT_CLK_CFG_t *ptClkCfg)
{
    uint8_t un8Arg = 2;
    uint16_t un16Data = 0;

    switch (pn8Argv[un8Arg++][0])
    {
        case 'm':
            ptClkCfg->eClk = FRT_CLK_MCCR;
            break;
        case 'p':
            ptClkCfg->eClk = FRT_CLK_PCLK;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_FRT_ERR_STR, NULL, EX_COMM_STR_OPT_SRC);
            goto err;
    }

    if(ptClkCfg->eClk == FRT_CLK_MCCR)
    {
        if (pn8Argv[un8Arg][0] == 'l')
            ptClkCfg->eMccr = FRT_CLK_MCCR_LSI;
        else if (pn8Argv[un8Arg][0] == 's')
            ptClkCfg->eMccr = FRT_CLK_MCCR_LSE;
        else if (pn8Argv[un8Arg][0] == 'm')
            ptClkCfg->eMccr = FRT_CLK_MCCR_MCLK;
        else if (pn8Argv[un8Arg][0] == 'h')
            ptClkCfg->eMccr = FRT_CLK_MCCR_HSI;
        else if (pn8Argv[un8Arg][0] == 'e')
            ptClkCfg->eMccr = FRT_CLK_MCCR_HSE;
        else if (pn8Argv[un8Arg][0] == 'p')
            ptClkCfg->eMccr = FRT_CLK_MCCR_PLL;
        else
        {
            EX_COMMON_SetShowModuleLog(EX_FRT_ERR_STR, NULL, EX_COMM_STR_OPT_MCCR);
            goto err;
        }

        ptClkCfg->un8MccrDiv = atoi(pn8Argv[++un8Arg]);
        un8Arg++;
    }

    un16Data = ((uint16_t)atoi(pn8Argv[un8Arg++]));
    if (un16Data == 1)
        ptClkCfg->ePreDiv = FRT_CLK_PREDIV_1;
    else if (un16Data == 4)
        ptClkCfg->ePreDiv = FRT_CLK_PREDIV_4;
    else if (un16Data == 8)
        ptClkCfg->ePreDiv = FRT_CLK_PREDIV_8;
    else if (un16Data == 16)
        ptClkCfg->ePreDiv = FRT_CLK_PREDIV_16;
    else if (un16Data == 32)
        ptClkCfg->ePreDiv = FRT_CLK_PREDIV_32;
    else if (un16Data == 64)
        ptClkCfg->ePreDiv = FRT_CLK_PREDIV_64;
    else if (un16Data == 128)
        ptClkCfg->ePreDiv = FRT_CLK_PREDIV_128;
    else if (un16Data == 256)
        ptClkCfg->ePreDiv = FRT_CLK_PREDIV_256;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_FRT_ERR_STR, "[prescl]", EX_COMM_STR_OPT_MAX);
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_FRT_GetConfig(int32_t n32Argc, char *pn8Argv[], FRT_CFG_t *ptCfg, FRT_OPS_e *peOps)
{
    uint8_t un8Arg = 2;
    uint32_t un32Data = 0;

    if (pn8Argv[un8Arg][0] == 'p')
        *peOps = FRT_OPS_POLL;
    else if (pn8Argv[un8Arg][0] == 'i')
        *peOps = FRT_OPS_INTR;
    else if (pn8Argv[un8Arg][0] == 'n')
        *peOps = FRT_OPS_NMI;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_FRT_ERR_STR, NULL, EX_COMM_STR_OPT_OPS);
        goto err;
    }

    switch (pn8Argv[++un8Arg][0])
    {
        case 'f':
            ptCfg->eMode = FRT_MODE_FREERUN;
            ptCfg->eIntr = FRT_INTR_OVERFLOW;
            break;
        case 'm':
            ptCfg->eMode = FRT_MODE_MATCH;
            ptCfg->eIntr = FRT_INTR_MATCH;
            sscanf(pn8Argv[++un8Arg], "%X", &un32Data);
            ptCfg->un32MatchCnt = un32Data;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_FRT_ERR_STR, NULL, EX_COMM_STR_OPT_MODE);
            goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static void EX_FRT_IRQHandler(uint32_t un32Event, void *pContext)
{
    FRT_Context_t *ptContext = (FRT_Context_t *)pContext;

    if(un32Event & FRT_EVENT_MATCH)
    {
        LOG("%s (%d) M evt fired\n", EX_FRT_LOG_STR, ptContext->eId);
        HAL_FRT_Stop(ptContext->eId);
    }

    if(un32Event & FRT_EVENT_OVERFLOW)
    {
        LOG("%s (%d) O(free) evt fired\n", EX_FRT_LOG_STR, ptContext->eId);
        HAL_FRT_Stop(ptContext->eId);
    }
}

static enum debug_cmd_status EX_FRT_Init(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    FRT_ID_e eId = FRT_ID_0;

    eDbgStatus = EX_FRT_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_FRT_Init(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_FRT_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_INIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_FRT_Uninit(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    FRT_ID_e eId = FRT_ID_0;

    eDbgStatus = EX_FRT_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_FRT_Uninit(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_FRT_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_UNINIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_FRT_SetClk(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    FRT_ID_e eId;
    FRT_CLK_CFG_t tClkCfg; 

    eDbgStatus = EX_FRT_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    memset(&tClkCfg, 0x00, sizeof(FRT_CLK_CFG_t));

    eDbgStatus = EX_FRT_GetClkConfig(n32Argc, pn8Argv, &tClkCfg);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_FRT_SetClkConfig(eId, &tClkCfg);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_FRT_ERR_STR, "ClkCfg", EX_COMM_STR_OPT_MAX);
        }

        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_FRT_SetConfig(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    FRT_ID_e eId = FRT_ID_0;
    FRT_CFG_t tCfg;
   
    eDbgStatus = EX_FRT_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    memset(&tCfg, 0x00, sizeof(FRT_CFG_t));

    eDbgStatus = EX_FRT_GetConfig(n32Argc, pn8Argv, &tCfg, &s_eOps);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_FRT_SetConfig(eId, &tCfg);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_FRT_ERR_STR, "Cfg", EX_COMM_STR_OPT_MAX);
        }

        goto err;
    }

    eErr = HAL_FRT_SetIRQ(eId, s_eOps, EX_FRT_IRQHandler, (void *)&s_tFrtContext[(uint32_t)eId], EX_FRT_IRQ_PRIO);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_FRT_Start(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    FRT_ID_e eId;
    uint8_t un8Event = 0;
    bool bResume = false;

    eDbgStatus = EX_FRT_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    if(strncmp(pn8Argv[2], "-res", 4) == 0)
    {
        bResume = true;
    }

    s_tFrtContext[eId].eId = eId;

    eErr = HAL_FRT_Start(eId, bResume);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    LOG("%s (%d) %s\n", EX_FRT_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_START]);

    if(s_eOps == FRT_OPS_POLL)
    {
        while(1)
        {
            eErr = HAL_FRT_SetWaitComplete(eId, EX_FRT_DEFAULT_TIMEOUT, &un8Event);
            if(eErr != HAL_ERR_OK)
            {
                goto err;
            }

            if(un8Event & FRT_EVENT_MATCH)
            {
                LOG("%s (%d) M evt fired\n",EX_FRT_ERR_STR, eId);
            }

            if(un8Event & FRT_EVENT_OVERFLOW)
            {
                LOG("%s (%d) O(free) evt fired\n",EX_FRT_ERR_STR, eId);
            }
        }
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_FRT_Stop(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    FRT_ID_e eId;

    eDbgStatus = EX_FRT_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_FRT_Stop(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_FRT_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_STOP]);

    return DEBUG_CMD_SUCCESS;
}

static const struct debug_cmd s_tEX_FRT_CMD[] =
{
    {"FRT", "h", EX_FRT_Help,"help"},
    {"FRT", "init", EX_FRT_Init, ""},
    {"FRT", "uninit", EX_FRT_Uninit, ""},
    {"FRT", "clk", EX_FRT_SetClk, ""},
    {"FRT", "config", EX_FRT_SetConfig, ""},
    {"FRT", "start", EX_FRT_Start, ""},
    {"FRT", "stop", EX_FRT_Stop, ""}
};

void EX_FRT(void)
{
    /* Add TC commands */
    debug_cmd_init(s_tEX_FRT_CMD, DEBUG_CMD_LIST_COUNT(s_tEX_FRT_CMD));
}

#endif /* FRT_TC */
/* --------------------------------- End Of File ------------------------------ */
