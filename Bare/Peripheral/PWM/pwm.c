/**
 *******************************************************************************
 * @file        pwm.c
 * @author      ABOV R&D Division
 * @brief       PWM Example Code
 *
 * Copyright 2023 ABOV Semiconductor Co.,Ltd. All rights reserved.
 *
 * This file is licensed under terms that are found in the LICENSE file
 * located at Document directory.
 * If this file is delivered or shared without applicable license terms,
 * the terms of the BSD-3-Clause license shall be applied.
 * Reference: https://opensource.org/licenses/BSD-3-Clause
 ******************************************************************************/

#include "abov_example_config.h"

#if defined(PWM_TC)
#include "abov_config.h"
#include "ex_configs.h"
#include "ex_common.h"
#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "hal_scu.h"
#include "hal_scu_clk.h"
#include "hal_pcu.h"
#include "hal_pwm.h"

#if !defined(_PWM)
#error "This chipset did not support this example."
#endif

#define EX_PWM_STR "PWM"
#define EX_PWM_LOG_STR "PWM :"
#define EX_PWM_ERR_STR "[E]PWM :"
#define EX_PWM_IRQ_PRIO 3
#define EX_PWM_MAX_ARG  7
#define EX_PWM_MAX_NUM  (CONFIG_PWM_MAX_COUNT - 1)

extern const char *pcCommStr[];
extern const char *pcCmdStr[];
extern const char *pcValStr[];
extern const char *pcOptStr[];

static PWM_Context_t s_tPwmContext[CONFIG_PWM_MAX_COUNT];
static uint32_t un32ISRLog = 0;

static enum debug_cmd_status EX_PWM_Help(int32_t n32Argc, char *pn8Argv[])
{
    uint32_t un32ShowOpt = 0;
    EX_COMM_STR_OPT_e eOpt[3];

    EX_COMMON_SetShowModuleInfo(EX_PWM_STR, CONFIG_PWM_MAX_COUNT);

    un32ShowOpt = EX_COMMON_GetShowOpt(pn8Argv[1]);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_INIT, NULL, EX_PWM_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_UNINIT, NULL, EX_PWM_MAX_NUM, eOpt, 0, NULL);

    eOpt[0] = EX_COMM_STR_OPT_SRC; 
    eOpt[1] = EX_COMM_STR_OPT_ENA; 
    eOpt[2] = EX_COMM_STR_OPT_DIV; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CLK, NULL, EX_PWM_MAX_NUM, eOpt, 3, NULL);
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_SRC, EX_COMM_STR_VAL_MAX, "p(pclk)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_ENA, EX_COMM_STR_VAL_ENDIS, " - pre-scaler");
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "e: [scl]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "scl: 0~N");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_DIV, EX_COMM_STR_VAL_MAX, "0,2,4,16");
    }
    eOpt[0] = EX_COMM_STR_OPT_OPS; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CONFIG, NULL, EX_PWM_MAX_NUM, eOpt, 1, "[inv] [sync] [peri] [duty]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_OPS, EX_COMM_STR_VAL_OPS, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "inv: n(not invert)/i(invert) - port A");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "sync: e(en)/d(dis) - sync with other chans");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "peri: 0~N(16bit)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "duty: 0~N(16bit)");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_START, NULL, EX_PWM_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_STOP, NULL, EX_PWM_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_LOG, NULL, -1, eOpt, 0, (char *)pcValStr[EX_COMM_STR_VAL_ONOFF]);
    LOG("\n");

    return DEBUG_CMD_SUCCESS;
}

static void EX_PWM_IRQHandler(uint32_t un32Event, void *pContext)
{
    PWM_Context_t *ptContext = (PWM_Context_t *)pContext;

    if(un32Event & PWM_EVENT_MATCH)
    {
        if(un32ISRLog == 1)
        {
            LOG("%s (%d) M evt fired\n", EX_PWM_LOG_STR, ptContext->eId);
        }
    }

}

static enum debug_cmd_status EX_PWM_GetClkConfig(int32_t n32Argc, char *pn8Argv[], PWM_CLK_CFG_t *ptClkCfg)
{
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;
    uint8_t un8Arg = 2, un8Data = 0;

    switch (pn8Argv[un8Arg++][0])
    {
        case 'p':
            ptClkCfg->eClk = PWM_CLK_PCLK;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_PWM_ERR_STR, NULL, EX_COMM_STR_OPT_SRC);
            goto err;
    }

    eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], &ptClkCfg->bPreScaleEn);
    if (eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_PWM_ERR_STR, NULL, EX_COMM_STR_OPT_ENA);
        goto err;
    }

    if(ptClkCfg->bPreScaleEn == true)
    {
        ptClkCfg->un8PreScale = (uint8_t)atoi(pn8Argv[un8Arg++]);
    }

    un8Data = (uint8_t)atoi(pn8Argv[un8Arg++]);
    if (un8Data == 2)
        ptClkCfg->eClkDiv = PWM_CLK_DIV_2;
    else if (un8Data == 4)
        ptClkCfg->eClkDiv = PWM_CLK_DIV_4;
    else if (un8Data == 8)
        ptClkCfg->eClkDiv = PWM_CLK_DIV_8;
    else if (un8Data == 16)
        ptClkCfg->eClkDiv = PWM_CLK_DIV_16;
    else
        ptClkCfg->eClkDiv = PWM_CLK_DIV_DISABLE;

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_PWM_GetConfig(int32_t n32Argc, char *pn8Argv[], PWM_ID_e eId, PWM_OPS_e *peOps, PWM_CFG_t *ptCfg)
{
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;
    uint8_t un8Arg = 2;

    switch (pn8Argv[un8Arg++][0])
    {
        case 'p':
            *peOps = PWM_OPS_POLL;
            break;
        case 'i':
            *peOps = PWM_OPS_INTR;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_PWM_ERR_STR, NULL, EX_COMM_STR_OPT_OPS);
            goto err;
    }

    switch (pn8Argv[un8Arg++][0])
    {
        case 'n':
            ptCfg->ePAInv = PWM_PORTA_INV_OFF;
            break;
        case 'i':
            ptCfg->ePAInv = PWM_PORTA_INV_ON;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_PWM_ERR_STR, "[inv]", EX_COMM_STR_OPT_MAX);
            goto err;
    } 

    eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], &ptCfg->bSyncEn);
    if (eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_PWM_ERR_STR, NULL, EX_COMM_STR_OPT_ENA);
        goto err;
    }

    ptCfg->un16Period = (uint16_t)atoi(pn8Argv[un8Arg++]);
    ptCfg->un16Duty = (uint16_t)atoi(pn8Argv[un8Arg++]);
    
    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_PWM_GetId(int32_t n32Argc, char *pn8Argv[], PWM_ID_e *peId)
{
    uint8_t un8Data = 0;
    if(n32Argc == 1)
    {
        EX_COMMON_SetShowModuleLog(EX_PWM_ERR_STR, (char *)pcOptStr[EX_COMM_STR_OPT_ARG], EX_COMM_STR_OPT_MAX);
        return DEBUG_CMD_INVALID;
    }

    un8Data = atoi(pn8Argv[1]); 
    if(un8Data >= CONFIG_PWM_MAX_COUNT)
    {
        LOG("%s Max Chan %d\n", EX_PWM_ERR_STR, CONFIG_PWM_MAX_COUNT);
        return DEBUG_CMD_INVALID;
    }

    *peId = (PWM_ID_e)un8Data;

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_PWM_Init(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    PWM_ID_e eId;

    eDbgStatus = EX_PWM_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_PWM_Init(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_PWM_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_INIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_PWM_Uninit(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    PWM_ID_e eId;

    eDbgStatus = EX_PWM_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_PWM_Uninit(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_PWM_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_UNINIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_PWM_SetClk(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    PWM_ID_e eId;
    PWM_CLK_CFG_t tClkCfg;

    eDbgStatus = EX_PWM_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eDbgStatus = EX_PWM_GetClkConfig(n32Argc, pn8Argv, &tClkCfg);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_PWM_SetClkConfig(eId, &tClkCfg);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_PWM_SetConfig(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    PWM_ID_e eId;
    PWM_OPS_e eOps;
    PWM_CFG_t tPwmCfg;

    eDbgStatus = EX_PWM_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eDbgStatus = EX_PWM_GetConfig(n32Argc, pn8Argv, eId, &eOps, &tPwmCfg);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_PWM_SetConfig(eId, &tPwmCfg);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    if(eOps == PWM_OPS_INTR)
    {
        eErr = HAL_PWM_SetIRQ(eId, eOps, EX_PWM_IRQHandler, (void *)&s_tPwmContext[(uint32_t)eId], EX_PWM_IRQ_PRIO);
        if(eErr != HAL_ERR_OK)
        {
            goto err;
        }
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_PWM_Start(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    PWM_ID_e eId;

    eDbgStatus = EX_PWM_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    s_tPwmContext[eId].eId = eId;

    eErr = HAL_PWM_Start(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_PWM_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_START]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_PWM_Stop(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    PWM_ID_e eId;

    eDbgStatus = EX_PWM_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_PWM_Stop(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_PWM_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_STOP]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_PWM_SetLog(int32_t n32Argc, char *pn8Argv[])
{
    if((strncmp(pn8Argv[1],"on",2) == 0))
    {
        un32ISRLog = 1;
    }
    else
    {
        un32ISRLog = 0;
    }

    LOG("%s ISR Log %s.\n", EX_PWM_LOG_STR, (un32ISRLog == 1 ? "on":"off"));

    return DEBUG_CMD_SUCCESS;
}

static const struct debug_cmd s_tEX_PWM_CMD[] =
{
    {"PWM", "h", EX_PWM_Help, "help"},
    {"PWM", "init", EX_PWM_Init, ""},
    {"PWM", "uninit", EX_PWM_Uninit, ""},
    {"PWM", "clk", EX_PWM_SetClk, ""},
    {"PWM", "config", EX_PWM_SetConfig, ""},
    {"PWM", "start", EX_PWM_Start, ""},
    {"PWM", "stop", EX_PWM_Stop, ""},
    {"PWM", "log", EX_PWM_SetLog, ""}
};

/**********************************************************************
 * @brief		Main program
 * @param[in]	None
 * @return	None
 **********************************************************************/
void EX_PWM(void)
{
    /* Add EX commands */
    debug_cmd_init(s_tEX_PWM_CMD,DEBUG_CMD_LIST_COUNT(s_tEX_PWM_CMD));
}

#endif
/* --------------------------------- End Of File ------------------------------ */
