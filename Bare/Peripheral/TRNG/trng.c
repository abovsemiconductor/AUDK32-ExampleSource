/**
 *******************************************************************************
 * @file        trng.c
 * @author      ABOV R&D Division
 * @brief       RNG Example Code
 *
 * Copyright 2024 ABOV Semiconductor Co.,Ltd. All rights reserved.
 *
 * This file is licensed under terms that are found in the LICENSE file
 * located at Document directory.
 * If this file is delivered or shared without applicable license terms,
 * the terms of the BSD-3-Clause license shall be applied.
 * Reference: https://opensource.org/licenses/BSD-3-Clause
 ******************************************************************************/

#include "abov_example_config.h"

#if defined(TRNG_TC)
#include "abov_config.h"
#include "ex_configs.h"
#include "ex_common.h"
#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "hal_scu.h"
#include "hal_pcu.h"
#include "hal_trng.h"

#if !defined(_TRNG)
#error "This chipset did not support this example."
#endif

#define EX_TRNG_STR "TRNG"
#define EX_TRNG_LOG_STR "TRNG :"
#define EX_TRNG_ERR_STR "[E]TRNG :"
#define EX_TRNG_IRQ_PRIO      3
#define EX_TRNG_MAX_NUM  (CONFIG_TRNG_MAX_COUNT - 1)

extern const char *pcCommStr[];
extern const char *pcCmdStr[];
extern const char *pcValStr[];
extern const char *pcOptStr[];

static TRNG_OPS_e s_eOps = TRNG_OPS_POLL;
static bool s_bISRLog = false;
static uint32_t s_un32LogDispCnt = 0;
static uint32_t s_un32LogCurCnt = 0;

static enum debug_cmd_status EX_TRNG_Help(int32_t n32Argc, char *pn8Argv[])
{
    uint32_t un32ShowOpt = 0;
    EX_COMM_STR_OPT_e eOpt[2];

    EX_COMMON_SetShowModuleInfo(EX_TRNG_STR, CONFIG_TRNG_MAX_COUNT);

    un32ShowOpt = EX_COMMON_GetShowOpt(pn8Argv[1]);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_INIT, NULL, EX_TRNG_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_UNINIT, NULL, EX_TRNG_MAX_NUM, eOpt, 0, NULL);

    eOpt[0] = EX_COMM_STR_OPT_OPS; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CONFIG, NULL, EX_TRNG_MAX_NUM, eOpt, 1, "");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_OPS, EX_COMM_STR_VAL_OPS, "/n(nmi)");
    }
    eOpt[0] = EX_COMM_STR_OPT_ENA; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "gen", EX_TRNG_MAX_NUM, eOpt, 1, NULL);
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_ENA, EX_COMM_STR_VAL_ENDIS, NULL);
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_LOG, NULL, -1, eOpt, 0, "on [cnt] / off");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_CNT, EX_COMM_STR_VAL_N_NUM, NULL);
    }
    LOG("\n");

    return DEBUG_CMD_SUCCESS;
}

static void EX_TRNG_IRQHandler(uint32_t un32Event, void *pContext)
{
    TRNG_ID_e eId = TRNG_ID_0;
    uint32_t un32Result;

    if(un32Event & TRNG_EVENT_READY)
    {
        (void)HAL_TRNG_GetResult(eId, &un32Result);
        
        if(s_bISRLog == true && s_un32LogDispCnt == s_un32LogCurCnt)
        {
            LOG("%s (%d) result 0x%08x (ISR)\n", EX_TRNG_LOG_STR, eId, un32Result);
            s_un32LogCurCnt = 0;
        }
    }
 
    if(un32Event & TRNG_EVENT_ERROR)
    {
        LOG("%s (%d) (ISR)\n", EX_TRNG_ERR_STR, eId);
    }

    s_un32LogCurCnt++;
    if(s_un32LogCurCnt > s_un32LogDispCnt)
    {
        s_un32LogCurCnt = 0;
    }
}

static enum debug_cmd_status EX_TRNG_GetId(int32_t n32Argc, char *pn8Argv[], TRNG_ID_e *peId)
{
    uint8_t un8Data = 0;
    if(n32Argc == 1)
    {
        EX_COMMON_SetShowModuleLog(EX_TRNG_ERR_STR, (char *)pcOptStr[EX_COMM_STR_OPT_ARG], EX_COMM_STR_OPT_MAX);
        return DEBUG_CMD_INVALID;
    }

    un8Data = atoi(pn8Argv[1]); 
    if(un8Data >= CONFIG_TRNG_MAX_COUNT)
    {
        LOG("%s Max Chan %d\n", EX_TRNG_ERR_STR, CONFIG_TRNG_MAX_COUNT);
        return DEBUG_CMD_INVALID;
    }

    *peId = (TRNG_ID_e)un8Data;

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_TRNG_GetConfig(int32_t n32Argc, char *pn8Argv[], TRNG_CFG_t *ptCfg, TRNG_OPS_e *peOps)
{
    uint8_t un8Arg = 2;

    switch (pn8Argv[un8Arg++][0])
    {
        case 'p':
            *peOps = TRNG_OPS_POLL;
            ptCfg->un8IntrEnable = 0;
            break;
        case 'i':
            *peOps = TRNG_OPS_INTR;
            ptCfg->un8IntrEnable = TRNG_INTR_READY | TRNG_INTR_ERROR;
            break;
        case 'n':
            *peOps = TRNG_OPS_NMI;
            ptCfg->un8IntrEnable = TRNG_INTR_READY | TRNG_INTR_ERROR;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_TRNG_ERR_STR, NULL, EX_COMM_STR_OPT_OPS);
            goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_TRNG_Init(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    TRNG_ID_e eId = TRNG_ID_0;

    eDbgStatus = EX_TRNG_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_TRNG_Init(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_TRNG_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_INIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_TRNG_Uninit(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    TRNG_ID_e eId = TRNG_ID_0;

    eDbgStatus = EX_TRNG_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_TRNG_Uninit(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_TRNG_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_UNINIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_TRNG_SetConfig(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus;
    TRNG_ID_e eId = TRNG_ID_0;
    TRNG_CFG_t tCfg;
   
    eDbgStatus = EX_TRNG_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    memset(&tCfg, 0x00, sizeof(TRNG_CFG_t));

    eDbgStatus = EX_TRNG_GetConfig(n32Argc, pn8Argv, &tCfg, &s_eOps);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_TRNG_SetConfig(eId, &tCfg);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_TRNG_ERR_STR, "Cfg", EX_COMM_STR_OPT_MAX);
        }

        goto err;
    }

    if((s_eOps == TRNG_OPS_INTR) || (s_eOps == TRNG_OPS_NMI))
    {
        eErr = HAL_TRNG_SetIRQ(eId, s_eOps, EX_TRNG_IRQHandler, NULL, EX_TRNG_IRQ_PRIO);
        if(eErr != HAL_ERR_OK)
        {
            goto err;
        }
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_TRNG_SetGenerate(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    TRNG_ID_e eId = TRNG_ID_0;
    uint8_t un8Arg = 2;
    uint32_t un32Result = 0;
    bool bEnable = false;

    eDbgStatus = EX_TRNG_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], &bEnable);
    if (eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_TRNG_ERR_STR, NULL, EX_COMM_STR_OPT_ENA);
        goto err;
    }

    eErr = HAL_TRNG_SetGenerate(eId, bEnable);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    if(s_eOps == TRNG_OPS_POLL && bEnable == true)
    {
        eErr = HAL_TRNG_SetWaitComplete(eId, 2000000);
        if(eErr == HAL_ERR_OK)
        {
            eErr = HAL_TRNG_GetResult(eId, &un32Result);
            if(eErr == HAL_ERR_OK)
            {
                LOG("%s (%d) result 0x%08x\n", EX_TRNG_LOG_STR, eId, un32Result);
            }
        }
        else
        {
            if(eErr == HAL_ERR_HW)
            {
                LOG("%s (%d) HW err\n", EX_TRNG_ERR_STR, eId);
            }
            goto err;
        }

        eErr = HAL_TRNG_SetGenerate(eId, false);
        if(eErr != HAL_ERR_OK)
        {
            goto err;
        }
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_TRNG_SetLog(int32_t n32Argc, char *pn8Argv[])
{
    if((strncmp(pn8Argv[1],"on",2) == 0))
    {
        s_bISRLog = true;
        if(n32Argc == 3)
        {
            s_un32LogDispCnt = (uint32_t)atoi(pn8Argv[2]);
        }
    }
    else
    {
        s_bISRLog = false;
    }

    LOG("TRNG ISR Log %s.\n",(s_bISRLog == true ? "on":"off"));

    return DEBUG_CMD_SUCCESS;
}

static const struct debug_cmd s_tEX_TRNG_CMD[] =
{
    {"TRNG", "h",EX_TRNG_Help, "help"},
    {"TRNG", "init",EX_TRNG_Init, ""},
    {"TRNG", "uninit",EX_TRNG_Uninit, ""},
    {"TRNG", "config",EX_TRNG_SetConfig, ""},
    {"TRNG", "gen",EX_TRNG_SetGenerate, ""},
    {"TRNG", "log",EX_TRNG_SetLog, ""}
};

/**********************************************************************
 * @brief		Main program
 * @param[in]	None
 * @return	None
 **********************************************************************/
void EX_TRNG(void)
{
    /* Add TC commands */
    debug_cmd_init(s_tEX_TRNG_CMD, DEBUG_CMD_LIST_COUNT(s_tEX_TRNG_CMD));
}

#endif /* TRNG_TC */

/* --------------------------------- End Of File ------------------------------ */
