/**
 *******************************************************************************
 * @file        tempsens.c
 * @author      ABOV R&D Division
 * @brief       TempSens Example Code
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

#if defined(TEMPSENS_TC)
#include "abov_config.h"
#include "ex_configs.h"
#include "ex_common.h"
#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "hal_scu.h"
#include "hal_scu_clk.h"
#include "hal_pcu.h"
#include "hal_tempsens.h"

#if !defined(_TEMPSENS)
#error "This chipset did not support this example."
#endif

#define EX_TEMPSENS_STR "TEMPSENS"
#define EX_TEMPSENS_LOG_STR "TEMPSENS :"
#define EX_TEMPSENS_ERR_STR "[E]TEMPSENS :"
#define EX_TEMPSENS_IRQ_PRIO     3
#define EX_TEMPSENS_MAX_NUM  (CONFIG_TEMPSENS_MAX_COUNT - 1)

extern const char *pcCommStr[];
extern const char *pcCmdStr[];
extern const char *pcValStr[];
extern const char *pcOptStr[];

extern uint32_t SystemCoreClock;
static TEMPSENS_Context_t s_tTempSensContext[CONFIG_TEMPSENS_MAX_COUNT];
static TEMPSENS_OPS_e s_eOps = TEMPSENS_OPS_POLL;

static enum debug_cmd_status EX_TEMPSENS_Help(int32_t n32Argc, char *pn8Argv[])
{
    uint32_t un32ShowOpt = 0;
    EX_COMM_STR_OPT_e eOpt[2];

    EX_COMMON_SetShowModuleInfo(EX_TEMPSENS_STR, CONFIG_TEMPSENS_MAX_COUNT);

    un32ShowOpt = EX_COMMON_GetShowOpt(pn8Argv[1]);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_INIT, NULL, EX_TEMPSENS_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_UNINIT, NULL, EX_TEMPSENS_MAX_NUM, eOpt, 0, NULL);

    eOpt[0] = EX_COMM_STR_OPT_OPS; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CONFIG, NULL, EX_TEMPSENS_MAX_NUM, eOpt, 1, "[refclk] [senseclk]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_OPS, EX_COMM_STR_VAL_OPS, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "refclk: h(hsi)/m(mclk)/e(hse)/s(lse)/p(pclk)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "senseclk: t(lsits)");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_START, NULL, EX_TEMPSENS_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_STOP, NULL, EX_TEMPSENS_MAX_NUM, eOpt, 0, NULL);
    LOG("\n");

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_TEMPSENS_GetId(int32_t n32Argc, char *pn8Argv[], TEMPSENS_ID_e *peId)
{
    uint8_t un8Data = 0;
    if(n32Argc == 1)
    {
        EX_COMMON_SetShowModuleLog(EX_TEMPSENS_ERR_STR, (char *)pcOptStr[EX_COMM_STR_OPT_ARG], EX_COMM_STR_OPT_MAX);
        return DEBUG_CMD_INVALID;
    }

    un8Data = atoi(pn8Argv[1]); 
    if(un8Data >= CONFIG_TEMPSENS_MAX_COUNT)
    {
        LOG("%s Max Chan %d\n", EX_TEMPSENS_ERR_STR, CONFIG_TEMPSENS_MAX_COUNT);
        return DEBUG_CMD_INVALID;
    }

    *peId = (TEMPSENS_ID_e)un8Data;

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_TEMPSENS_GetConfig(int32_t n32Argc, char *pn8Argv[], TEMPSENS_CFG_t *ptCfg, TEMPSENS_OPS_e *peOps)
{
    uint8_t un8Arg = 2;

    switch (pn8Argv[un8Arg++][0])
    {
        case 'p':
            *peOps = TEMPSENS_OPS_POLL;
            break;
        case 'i':
            *peOps = TEMPSENS_OPS_INTR;
            break;
        default:
	    EX_COMMON_SetShowModuleLog(EX_TEMPSENS_ERR_STR, NULL, EX_COMM_STR_OPT_OPS);
            goto err;
    }

    if (pn8Argv[un8Arg][0] == 'h')
        ptCfg->eRefClk = TEMPSENS_REF_CLK_HSI;
    else if (pn8Argv[un8Arg][0] == 'm')
        ptCfg->eRefClk = TEMPSENS_REF_CLK_MCLK;
    else if (pn8Argv[un8Arg][0] == 'e')
        ptCfg->eRefClk = TEMPSENS_REF_CLK_HSE;
    else if (pn8Argv[un8Arg][0] == 's')
        ptCfg->eRefClk = TEMPSENS_REF_CLK_LSE;
    else if (pn8Argv[un8Arg][0] == 'p')
        ptCfg->eRefClk = TEMPSENS_REF_CLK_PCLK;
    else
    {
	EX_COMMON_SetShowModuleLog(EX_TEMPSENS_ERR_STR, "[refclk]", EX_COMM_STR_OPT_MAX);
        goto err;
    }

    un8Arg++;
    if (pn8Argv[un8Arg][0] == 't')
        ptCfg->eSenClk = TEMPSENS_SEN_CLK_LSITS;
    else
    {
	EX_COMMON_SetShowModuleLog(EX_TEMPSENS_ERR_STR, "[senseclk]", EX_COMM_STR_OPT_MAX);
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static void EX_TEMPSENS_IRQHandler(uint32_t un32Event, void *pContext)
{

    HAL_ERR_e eErr = HAL_ERR_OK;
    TEMPSENS_ID_e eId = TEMPSENS_ID_0;
    TEMPSENS_Context_t *ptContext = (TEMPSENS_Context_t *)pContext;
    uint32_t un32Data;
    int8_t n8Temp = 0;

    eId = ptContext->eId;

    if(un32Event & TEMPSENS_EVENT_MATCHED)
    {
        eErr = HAL_TEMPSENS_GetData(eId, &un32Data);
        if(eErr != HAL_ERR_OK)
        {
            LOG("%s GetData\n", EX_TEMPSENS_ERR_STR);
        }

        eErr = HAL_TEMPSENS_GetTemp(eId, &n8Temp);
        if(eErr != HAL_ERR_OK)
        {
            LOG("%s Get Temperature\n", EX_TEMPSENS_ERR_STR);
        }

        eErr = HAL_TEMPSENS_Stop(eId);
        if(eErr != HAL_ERR_OK)
        {
            LOG("%s stop\n", EX_TEMPSENS_ERR_STR);
        }
        LOG("%s intr mode stop. (Count=%d,Temp=%d)\n", EX_TEMPSENS_LOG_STR, un32Data, n8Temp);
    }
}

static enum debug_cmd_status EX_TEMPSENS_Init(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    TEMPSENS_ID_e eId = TEMPSENS_ID_0;

    eDbgStatus = EX_TEMPSENS_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_TEMPSENS_Init(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_TEMPSENS_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_INIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_TEMPSENS_Uninit(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    TEMPSENS_ID_e eId = TEMPSENS_ID_0;

    eDbgStatus = EX_TEMPSENS_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_TEMPSENS_Uninit(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_TEMPSENS_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_UNINIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_TEMPSENS_SetConfig(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus;
    TEMPSENS_ID_e eId = TEMPSENS_ID_0;
    TEMPSENS_CFG_t tCfg;

    eDbgStatus = EX_TEMPSENS_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    memset(&tCfg, 0x00, sizeof(TEMPSENS_CFG_t));

    eDbgStatus = EX_TEMPSENS_GetConfig(n32Argc, pn8Argv, &tCfg, &s_eOps);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_TEMPSENS_SetConfig(eId, &tCfg);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_TEMPSENS_ERR_STR, "Cfg", EX_COMM_STR_OPT_MAX);
        }
        goto err;
    }

    s_tTempSensContext[eId].eId = eId;

    eErr = HAL_TEMPSENS_SetIRQ(eId, s_eOps, EX_TEMPSENS_IRQHandler, &s_tTempSensContext[eId], EX_TEMPSENS_IRQ_PRIO);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_TEMPSENS_ERR_STR, "IRQ", EX_COMM_STR_OPT_MAX);
        }
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;

}

static enum debug_cmd_status EX_TEMPSENS_Start(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    TEMPSENS_ID_e eId = TEMPSENS_ID_0;
    uint32_t un32Data=0;
    int8_t n8Temp;

    eDbgStatus = EX_TEMPSENS_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    LOG("%s (%d) %s\n", EX_TEMPSENS_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_START]);

    if(s_eOps == TEMPSENS_OPS_POLL)
    {
        eErr = HAL_TEMPSENS_Start(eId);
        if(eErr != HAL_ERR_OK)
        {
            goto err;
        }

        eErr = HAL_TEMPSENS_SetWaitComplete(eId, 20000000);
        if(eErr != HAL_ERR_OK)
        {
            LOG("%s timeout\n", EX_TEMPSENS_ERR_STR);
        }
        else
        {
            eErr = HAL_TEMPSENS_GetData(eId, &un32Data);
            if(eErr != HAL_ERR_OK)
            {
                LOG("%s GetData\n", EX_TEMPSENS_ERR_STR);
            }

            eErr = HAL_TEMPSENS_GetTemp(eId, &n8Temp);
            if(eErr != HAL_ERR_OK)
            {
                LOG("%s Get Temperature\n", EX_TEMPSENS_ERR_STR);
            }

            LOG("%s polling mode stop. (Count=%d,Temp=%d)\n", EX_TEMPSENS_LOG_STR, un32Data, n8Temp);
        }

        eErr = HAL_TEMPSENS_Stop(eId);
        if(eErr != HAL_ERR_OK)
        {
            goto err;
        }
    }
    else
    {
        eErr = HAL_TEMPSENS_Start(eId);
        if(eErr != HAL_ERR_OK)
        {
            goto err;
        }
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_TEMPSENS_Stop(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    TEMPSENS_ID_e eId = TEMPSENS_ID_0;

    eDbgStatus = EX_TEMPSENS_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_TEMPSENS_Stop(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_TEMPSENS_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_STOP]);

    return DEBUG_CMD_SUCCESS;
}

static const struct debug_cmd s_tEX_TEMPSENS_CMD[] =
{
    {"TEMPSENS", "h", EX_TEMPSENS_Help,"help"},
    {"TEMPSENS", "init", EX_TEMPSENS_Init, ""},
    {"TEMPSENS", "uninit", EX_TEMPSENS_Uninit, ""},
    {"TEMPSENS", "config", EX_TEMPSENS_SetConfig, ""},
    {"TEMPSENS", "start", EX_TEMPSENS_Start, ""},
    {"TEMPSENS", "stop", EX_TEMPSENS_Stop, ""}
};

void EX_TEMPSENS(void)
{
    /* Add TC commands */
    debug_cmd_init(s_tEX_TEMPSENS_CMD, DEBUG_CMD_LIST_COUNT(s_tEX_TEMPSENS_CMD));
}

#endif /* TEMPSENS_TC */

/* --------------------------------- End Of File ------------------------------ */
