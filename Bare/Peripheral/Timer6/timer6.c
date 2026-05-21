/**
 *******************************************************************************
 * @file        timer6.c
 * @author      ABOV R&D Division
 * @brief       Timer6 Example Code
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

#if defined(TIMER6_TC)
#include "abov_config.h"
#include "ex_common.h"
#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "hal_scu.h"
#include "hal_scu_clk.h"
#include "hal_pcu.h"
#include "hal_timer6.h"

#if !defined(_TIMER6)
#error EX_COMM_MODULE_ERR_STR
#endif

#define EX_TIMER6_STR "TIMER6"
#define EX_TIMER6_LOG_STR "TIMER6 :"
#define EX_TIMER6_ERR_STR "[E]TIMER6 :"
#define EX_TIMER6_IRQ_PRIO 3
#define EX_TIMER6_MAX_ARG  7
#define EX_TIMER6_MAX_NUM  (CONFIG_TIMER6_MAX_COUNT - 1)

extern const char *pcCommStr[];
extern const char *pcCmdStr[];
extern const char *pcValStr[];
extern const char *pcOptStr[];

static TIMER6_Context_t s_tTimer6Context[CONFIG_TIMER6_MAX_COUNT];
static uint32_t un32ISRLog = 0;

static enum debug_cmd_status EX_TIMER6_Help(int32_t n32Argc, char *pn8Argv[])
{
    uint32_t un32ShowOpt = 0;
    EX_COMM_STR_OPT_e eOpt[7];

    EX_COMMON_SetShowModuleInfo(EX_TIMER6_STR, CONFIG_TIMER6_MAX_COUNT);

    un32ShowOpt = EX_COMMON_GetShowOpt(pn8Argv[1]);

    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_INIT, NULL, EX_TIMER6_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_UNINIT, NULL, EX_TIMER6_MAX_NUM, eOpt, 0, NULL);

    eOpt[0] = EX_COMM_STR_OPT_OPS; 
    eOpt[1] = EX_COMM_STR_OPT_DA;
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CONFIG, NULL, EX_TIMER6_MAX_NUM, eOpt, 2, NULL);
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_OPS, EX_COMM_STR_VAL_OPS, "/n(nmi)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_DA, EX_COMM_STR_VAL_N_NUM, NULL);
    }

    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_START, NULL, EX_TIMER6_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_STOP, NULL, EX_TIMER6_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_LOG, NULL, -1, eOpt, 0, (char *)pcValStr[EX_COMM_STR_VAL_ONOFF]);
    LOG("\n");

    return DEBUG_CMD_SUCCESS;
}

static void EX_TIMER6_IRQHandler(uint32_t un32Event, void *pContext)
{
    TIMER6_Context_t *ptContext = (TIMER6_Context_t *)pContext;

    if(un32Event & TIMER6_EVENT_PERIODIC_MATCH)
    {
        if(un32ISRLog == 1)
        {
            LOG("%s (%d) P %s\n", EX_TIMER6_LOG_STR, ptContext->eId, pcCommStr[EX_COMM_STR_EVT_FIRE]);
        }
    }
}

static enum debug_cmd_status EX_TIMER6_GetConfig(int32_t n32Argc, char *pn8Argv[], TIMER6_ID_e eId, TIMER6_OPS_e *peOps, TIMER6_CFG_t *ptCfg)
{
    uint8_t un8Arg = 2;

    if (pn8Argv[un8Arg][0] == 'p')
    {
        *peOps = TIMER6_OPS_POLL;
        ptCfg->bIntrEnable = false;
    }
    else if (pn8Argv[un8Arg][0] == 'i')
    {
        *peOps = TIMER6_OPS_INTR;
        ptCfg->bIntrEnable = true;
    }
    else if (pn8Argv[un8Arg][0] == 'n')
    {
        *peOps = TIMER6_OPS_NMI;
        ptCfg->bIntrEnable = true;
    }
    else
    {
        EX_COMMON_SetShowModuleLog(EX_TIMER6_ERR_STR, NULL, EX_COMM_STR_OPT_OPS);
        goto err;
    }

    un8Arg++;
    ptCfg->un16Data = (uint16_t)atoi(pn8Argv[un8Arg++]);

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_TIMER6_GetId(int32_t n32Argc, char *pn8Argv[], TIMER6_ID_e *peId)
{
    uint8_t un8Data = 0;
    if(n32Argc == 1)
    {
        EX_COMMON_SetShowModuleLog(EX_TIMER6_ERR_STR, (char *)pcOptStr[EX_COMM_STR_OPT_ARG], EX_COMM_STR_OPT_MAX);
        return DEBUG_CMD_INVALID;
    }

    un8Data = atoi(pn8Argv[1]); 
    if(un8Data >= CONFIG_TIMER6_MAX_COUNT)
    {
        LOG("%s Max Chan %d\n", EX_TIMER6_ERR_STR, CONFIG_TIMER6_MAX_COUNT);
        return DEBUG_CMD_INVALID;
    }

    *peId = (TIMER6_ID_e)un8Data;

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_TIMER6_Init(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    TIMER6_ID_e eId;

    eDbgStatus = EX_TIMER6_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_TIMER6_Init(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_TIMER6_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_INIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_TIMER6_Uninit(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    TIMER6_ID_e eId;

    eDbgStatus = EX_TIMER6_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_TIMER6_Uninit(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_TIMER6_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_UNINIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_TIMER6_SetConfig(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    TIMER6_ID_e eId;
    TIMER6_OPS_e eOps;
    TIMER6_CFG_t tTimer6Cfg;

    eDbgStatus = EX_TIMER6_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eDbgStatus = EX_TIMER6_GetConfig(n32Argc, pn8Argv, eId, &eOps, &tTimer6Cfg);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_TIMER6_SetConfig(eId, &tTimer6Cfg);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    if(eOps == TIMER6_OPS_INTR || eOps == TIMER6_OPS_NMI)
    {
        eErr = HAL_TIMER6_SetIRQ(eId, eOps, EX_TIMER6_IRQHandler, (void *)&s_tTimer6Context[(uint32_t)eId], EX_TIMER6_IRQ_PRIO);
        if(eErr != HAL_ERR_OK)
        {
            goto err;
        }
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_TIMER6_Start(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    TIMER6_ID_e eId;

    eDbgStatus = EX_TIMER6_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    s_tTimer6Context[eId].eId = eId;

    eErr = HAL_TIMER6_Start(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_TIMER6_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_START]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_TIMER6_Stop(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    TIMER6_ID_e eId;

    eDbgStatus = EX_TIMER6_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_TIMER6_Stop(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_TIMER6_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_STOP]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_TIMER6_SetLog(int32_t n32Argc, char *pn8Argv[])
{
    if((strncmp(pn8Argv[1],"on",2) == 0))
    {
        un32ISRLog = 1;
    }
    else
    {
        un32ISRLog = 0;
    }

    LOG("%s ISR Log %s.\n", EX_TIMER6_LOG_STR, (un32ISRLog == 1 ? "on":"off"));

    return DEBUG_CMD_SUCCESS;
}

static struct debug_cmd s_tEX_TIMER6_CMD[] =
{
    {EX_TIMER6_STR, "h", EX_TIMER6_Help, "help"},
    {EX_TIMER6_STR, "init", EX_TIMER6_Init,""},
    {EX_TIMER6_STR, "uninit", EX_TIMER6_Uninit,""},
    {EX_TIMER6_STR, "config", EX_TIMER6_SetConfig,""},
    {EX_TIMER6_STR, "start", EX_TIMER6_Start,""},
    {EX_TIMER6_STR, "stop", EX_TIMER6_Stop,""},
    {EX_TIMER6_STR, "log", EX_TIMER6_SetLog,""}
};

/**********************************************************************
 * @brief		Main program
 * @param[in]	None
 * @return	None
 **********************************************************************/
void EX_TIMER6(void)
{
    /* Add EX commands */
    debug_cmd_init(s_tEX_TIMER6_CMD,DEBUG_CMD_LIST_COUNT(s_tEX_TIMER6_CMD));
}

#endif
/* --------------------------------- End Of File ------------------------------ */
