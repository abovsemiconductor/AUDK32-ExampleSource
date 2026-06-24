/**
 *******************************************************************************
 * @file        hsicmu.c
 * @author      ABOV R&D Division
 * @brief       HSICMU Example Code
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

#if defined(HSICMU_TC)
#include "abov_config.h"
#include "ex_common.h"
#include "ex_configs.h"
#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "hal_scu.h"
#include "hal_scu_clk.h"
#include "hal_pcu.h"
#include "hal_hsicmu.h"

#if !defined(_HSICMU)
#error "This chipset did not support this example."
#endif

#define EX_HSICMU_STR "HSICMU"
#define EX_HSICMU_LOG_STR "HSICMU :"
#define EX_HSICMU_ERR_STR "[E]HSICMU :"
#define EX_HSICMU_IRQ_PRIO      3
#define EX_HSICMU_IRQ_TIMEOUT   10000000
#define EX_HSICMU_DELAY         50
#define EX_HSICMU_DATA_LEN      16
#define EX_HSICMU_MAX_NUM  (CONFIG_HSICMU_MAX_COUNT - 1)

extern const char *pcCommStr[];
extern const char *pcCmdStr[];
extern const char *pcValStr[];
extern const char *pcOptStr[];

static HSICMU_OPS_e s_eOps = HSICMU_OPS_POLL;

static enum debug_cmd_status EX_HSICMU_Help(int32_t n32Argc, char *pn8Argv[])
{
    EX_COMMON_SetShowModuleInfo(EX_HSICMU_STR, CONFIG_HSICMU_MAX_COUNT);

    return DEBUG_CMD_SUCCESS;
}

static void EX_HSICMU_IRQHandler(uint32_t un32Event, void *pContext)
{
    HSICMU_ID_e eId = HSICMU_ID_0;

    LOG("%s (%d) (ISR)\n", EX_HSICMU_LOG_STR, eId);
}

static enum debug_cmd_status EX_HSICMU_GetId(int32_t n32Argc, char *pn8Argv[], HSICMU_ID_e *peId)
{
    uint8_t un8Data = 0;
    if(n32Argc == 1)
    {
        EX_COMMON_SetShowModuleLog(EX_HSICMU_ERR_STR, (char *)pcOptStr[EX_COMM_STR_OPT_ARG], EX_COMM_STR_OPT_MAX);
        return DEBUG_CMD_INVALID;
    }

    un8Data = atoi(pn8Argv[1]); 
    if(un8Data >= CONFIG_HSICMU_MAX_COUNT)
    {
        LOG("%s Max Chan %d\n", EX_HSICMU_ERR_STR, CONFIG_HSICMU_MAX_COUNT);
        return DEBUG_CMD_INVALID;
    }

    *peId = (HSICMU_ID_e)un8Data;

    return DEBUG_CMD_SUCCESS;
}
#if 0
static enum debug_cmd_status EX_HSICMU_GetConfig(int32_t n32Argc, char *pn8Argv[], HSICMU_CFG_t *ptCfg, HSICMU_OPS_e *peOps)
{
    uint8_t un8Arg = 2;

    *peOps = HSICMU_OPS_POLL;

    switch (pn8Argv[un8Arg][0])
    {
        case 'e':
            ptCfg->eSyncSrc = HSICMU_SRC_HSE;
            break;
        case 'l':
            ptCfg->eSyncSrc = HSICMU_SRC_LSE;
            break;
        case 'g':
            ptCfg->eSyncSrc = HSICMU_SRC_GPIO;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_HSICMU_ERR_STR, NULL, EX_COMM_STR_OPT_MODE);
            goto err;
    }

    if(ptCfg->eMode == HSICMU_MODE_VDDEXT)
    {
        ptCfg->un32StlTime = atoi(pn8Argv[++un8Arg]);
    }
    else if(ptCfg->eMode == HSICMU_MODE_INTERNAL)
    {
        ptCfg->un32StlTime = atoi(pn8Argv[++un8Arg]);
        switch (pn8Argv[++un8Arg][0])
        {
            case '0':
                ptCfg->eVoltage = HSICMU_VOLTAGE_INT20V;
                break;
            case '1':
                ptCfg->eVoltage = HSICMU_VOLTAGE_INT25V;
                break;
            default:
                EX_COMMON_SetShowModuleLog(EX_HSICMU_ERR_STR, "out[out]", EX_COMM_STR_OPT_MAX);
                goto err;
        }
    }
    else
    {
        ;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}
#endif
static enum debug_cmd_status EX_HSICMU_Init(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    HSICMU_ID_e eId = HSICMU_ID_0;

    eDbgStatus = EX_HSICMU_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_HSICMU_Init(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_HSICMU_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_INIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_HSICMU_Uninit(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    HSICMU_ID_e eId = HSICMU_ID_0;

    eDbgStatus = EX_HSICMU_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_HSICMU_Uninit(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_HSICMU_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_UNINIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_HSICMU_SetConfig(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus;
    HSICMU_ID_e eId = HSICMU_ID_0;
    HSICMU_CFG_t tCfg;

    eDbgStatus = EX_HSICMU_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    memset(&tCfg, 0x00, sizeof(HSICMU_CFG_t));
#if 0
    eDbgStatus = EX_HSICMU_GetConfig(n32Argc, pn8Argv, &tCfg, &s_eOps);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }
#endif
    tCfg.eSyncSrc = HSICMU_SRC_HSE;
    tCfg.eSyncDiv = HSICMU_DIV_128;
    tCfg.ePreDiv = HSICMU_DIV_512;
    tCfg.eSyncSrc = HSICMU_POL_RISING_EDGE;
    tCfg.un32Reload = 262144;
    tCfg.un16ErrRate = 262;

    eErr = HAL_HSICMU_SetConfig(eId, &tCfg);
    if(eErr != HAL_ERR_OK)
    {
        EX_COMMON_SetShowModuleLog(EX_HSICMU_ERR_STR, "Cfg", EX_COMM_STR_OPT_MAX);
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_HSICMU_Start(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus;
    HSICMU_ID_e eId = HSICMU_ID_0;

    eDbgStatus = EX_HSICMU_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_HSICMU_Start(eId);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    LOG("%s (%d) %s\n", EX_HSICMU_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_START]);

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_HSICMU_Stop(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus;
    HSICMU_ID_e eId = HSICMU_ID_0;

    eDbgStatus = EX_HSICMU_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_HSICMU_Stop(eId);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    LOG("%s (%d) %s\n", EX_HSICMU_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_STOP]);

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static const struct debug_cmd s_tEX_HSICMU_CMD[] =
{
    {"HSICMU", "h",EX_HSICMU_Help, "help"},
    {"HSICMU", "init",EX_HSICMU_Init, ""},
    {"HSICMU", "uninit",EX_HSICMU_Uninit, ""},
    {"HSICMU", "config",EX_HSICMU_SetConfig, ""},
    {"HSICMU", "start",EX_HSICMU_Start, ""},
    {"HSICMU", "stop",EX_HSICMU_Stop, ""},
};

/**********************************************************************
 * @brief		Main program
 * @param[in]	None
 * @return	None
 **********************************************************************/
void EX_HSICMU(void)
{
    /* Add TC commands */
    debug_cmd_init(s_tEX_HSICMU_CMD, DEBUG_CMD_LIST_COUNT(s_tEX_HSICMU_CMD));
}

#endif /* HSICMU_TC */

/* --------------------------------- End Of File ------------------------------ */
