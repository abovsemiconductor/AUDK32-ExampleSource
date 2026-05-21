/**
 *******************************************************************************
 * @file        rng.c
 * @author      ABOV R&D Division
 * @brief       RNG Example Code
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

#if defined(RNG_TC)
#include "abov_config.h"
#include "ex_configs.h"
#include "ex_common.h"
#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "hal_scu.h"
#include "hal_pcu.h"
#include "hal_rng.h"

#if !defined(_RNG)
#error "This chipset did not support this example."
#endif

#define EX_RNG_STR "RNG"
#define EX_RNG_LOG_STR "RNG :"
#define EX_RNG_ERR_STR "[E]RNG :"
#define EX_RNG_IRQ_PRIO      3
#define EX_RNG_MAX_NUM  (CONFIG_RNG_MAX_COUNT - 1)

extern const char *pcCommStr[];
extern const char *pcCmdStr[];
extern const char *pcValStr[];
extern const char *pcOptStr[];

static RNG_OPS_e s_eOps = RNG_OPS_POLL;
static bool s_bISRLog = false;
static uint32_t s_un32LogDispCnt = 0;
static uint32_t s_un32LogCurCnt = 0;

static enum debug_cmd_status EX_RNG_Help(int32_t n32Argc, char *pn8Argv[])
{
    uint32_t un32ShowOpt = 0;
    EX_COMM_STR_OPT_e eOpt[2];

    EX_COMMON_SetShowModuleInfo(EX_RNG_STR, CONFIG_RNG_MAX_COUNT);

    un32ShowOpt = EX_COMMON_GetShowOpt(pn8Argv[1]);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_INIT, NULL, EX_RNG_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_UNINIT, NULL, EX_RNG_MAX_NUM, eOpt, 0, NULL);

    eOpt[0] = EX_COMM_STR_OPT_OPS; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CONFIG, NULL, EX_RNG_MAX_NUM, eOpt, 1, "[lfsc] [casc] [val]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_OPS, EX_COMM_STR_VAL_OPS, "/n(nmi)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "lfsc: o(osc)/l(lsi) clock");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "casc: o(osc)/h(hsi) clock");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "val: 0~N generate time");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "seed", EX_RNG_MAX_NUM, eOpt, 0, "[val]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "val: 0~N (hexa)");
    }
    eOpt[0] = EX_COMM_STR_OPT_ENA; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "gen", EX_RNG_MAX_NUM, eOpt, 1, NULL);
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

static void EX_RNG_IRQHandler(uint32_t un32Event, void *pContext)
{
    RNG_ID_e eId = RNG_ID_0;
    uint32_t un32Result;

    if(un32Event & RNG_EVENT_READY)
    {
        (void)HAL_RNG_GetResult(eId, &un32Result);
        
        if(s_bISRLog == true && s_un32LogDispCnt == s_un32LogCurCnt)
        {
            LOG("%s (%d) result 0x%08x (ISR)\n", EX_RNG_LOG_STR, eId, un32Result);
            s_un32LogCurCnt = 0;
        }
    }
 
    if(un32Event & RNG_EVENT_ERROR)
    {
        LOG("%s (%d) (ISR)\n", EX_RNG_ERR_STR, eId);
    }

    s_un32LogCurCnt++;
    if(s_un32LogCurCnt > s_un32LogDispCnt)
    {
        s_un32LogCurCnt = 0;
    }
}

static enum debug_cmd_status EX_RNG_GetId(int32_t n32Argc, char *pn8Argv[], RNG_ID_e *peId)
{
    uint8_t un8Data = 0;
    if(n32Argc == 1)
    {
        EX_COMMON_SetShowModuleLog(EX_RNG_ERR_STR, (char *)pcOptStr[EX_COMM_STR_OPT_ARG], EX_COMM_STR_OPT_MAX);
        return DEBUG_CMD_INVALID;
    }

    un8Data = atoi(pn8Argv[1]); 
    if(un8Data >= CONFIG_RNG_MAX_COUNT)
    {
        LOG("%s Max Chan %d\n", EX_RNG_ERR_STR, CONFIG_RNG_MAX_COUNT);
        return DEBUG_CMD_INVALID;
    }

    *peId = (RNG_ID_e)un8Data;

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_RNG_GetConfig(int32_t n32Argc, char *pn8Argv[], RNG_CFG_t *ptCfg, RNG_OPS_e *peOps)
{
    uint8_t un8Arg = 2;

    switch (pn8Argv[un8Arg++][0])
    {
        case 'p':
            *peOps = RNG_OPS_POLL;
            ptCfg->un8IntrEnable = 0;
            break;
        case 'i':
            *peOps = RNG_OPS_INTR;
            ptCfg->un8IntrEnable = RNG_INTR_READY | RNG_INTR_ERROR;
            break;
        case 'n':
            *peOps = RNG_OPS_NMI;
            ptCfg->un8IntrEnable = RNG_INTR_READY | RNG_INTR_ERROR;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_RNG_ERR_STR, NULL, EX_COMM_STR_OPT_OPS);
            goto err;
    }

    switch (pn8Argv[un8Arg++][0])
    {
        case 'o':
            ptCfg->eClkLFS = RNG_CLK_LFS_OSC;
            break;
        case 'l':
            ptCfg->eClkLFS = RNG_CLK_LFS_LSI;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_RNG_ERR_STR, "[lfs]", EX_COMM_STR_OPT_MAX);
            goto err;
    }

    switch (pn8Argv[un8Arg++][0])
    {
        case 'o':
            ptCfg->eClkCAS = RNG_CLK_CAS_OSC;
            break;
        case 'h':
            ptCfg->eClkCAS = RNG_CLK_CAS_HSI;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_RNG_ERR_STR, "[casc]", EX_COMM_STR_OPT_MAX);
            goto err;
    }

    ptCfg->un16GenTime = (uint16_t)atoi(pn8Argv[un8Arg++]);

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_RNG_Init(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    RNG_ID_e eId = RNG_ID_0;

    eDbgStatus = EX_RNG_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_RNG_Init(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_RNG_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_INIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_RNG_Uninit(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    RNG_ID_e eId = RNG_ID_0;

    eDbgStatus = EX_RNG_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_RNG_Uninit(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_RNG_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_UNINIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_RNG_SetConfig(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus;
    RNG_ID_e eId = RNG_ID_0;
    RNG_CFG_t tCfg;
   
    eDbgStatus = EX_RNG_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    memset(&tCfg, 0x00, sizeof(RNG_CFG_t));

    eDbgStatus = EX_RNG_GetConfig(n32Argc, pn8Argv, &tCfg, &s_eOps);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_RNG_SetConfig(eId, &tCfg);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_RNG_ERR_STR, "Cfg", EX_COMM_STR_OPT_MAX);
        }

        goto err;
    }

    if((s_eOps == RNG_OPS_INTR) || (s_eOps == RNG_OPS_NMI))
    {
        eErr = HAL_RNG_SetIRQ(eId, s_eOps, EX_RNG_IRQHandler, NULL, EX_RNG_IRQ_PRIO);
        if(eErr != HAL_ERR_OK)
        {
            goto err;
        }
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_RNG_SetSeed(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    RNG_ID_e eId = RNG_ID_0;
    uint8_t un8Arg = 2;
    uint32_t un32Data = 0;

    eDbgStatus = EX_RNG_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    sscanf(pn8Argv[un8Arg++], "%X", &un32Data);

    eErr = HAL_RNG_SetSeed(eId, un32Data);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    return DEBUG_CMD_SUCCESS;
}


static enum debug_cmd_status EX_RNG_SetGenerate(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    RNG_ID_e eId = RNG_ID_0;
    uint8_t un8Arg = 2;
    uint32_t un32Result = 0;
    bool bEnable = false;

    eDbgStatus = EX_RNG_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], &bEnable);
    if (eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_RNG_ERR_STR, NULL, EX_COMM_STR_OPT_ENA);
        goto err;
    }

    eErr = HAL_RNG_SetGenerate(eId, bEnable);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    if(s_eOps == RNG_OPS_POLL && bEnable == true)
    {
        eErr = HAL_RNG_SetWaitComplete(eId, 2000000);
        if(eErr == HAL_ERR_OK)
        {
            eErr = HAL_RNG_GetResult(eId, &un32Result);
            if(eErr == HAL_ERR_OK)
            {
                LOG("%s (%d) result 0x%08x\n", EX_RNG_LOG_STR, eId, un32Result);
            }
        }
        else
        {
            if(eErr == HAL_ERR_HW)
            {
                LOG("%s (%d) HW err\n", EX_RNG_ERR_STR, eId);
            }
            goto err;
        }

        eErr = HAL_RNG_SetGenerate(eId, false);
        if(eErr != HAL_ERR_OK)
        {
            goto err;
        }
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_RNG_SetLog(int32_t n32Argc, char *pn8Argv[])
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

    LOG("RNG ISR Log %s.\n",(s_bISRLog == true ? "on":"off"));

    return DEBUG_CMD_SUCCESS;
}

static const struct debug_cmd s_tEX_RNG_CMD[] =
{
    {"RNG", "h",EX_RNG_Help, "help"},
    {"RNG", "init",EX_RNG_Init, ""},
    {"RNG", "uninit",EX_RNG_Uninit, ""},
    {"RNG", "config",EX_RNG_SetConfig, ""},
    {"RNG", "seed", EX_RNG_SetSeed, ""},
    {"RNG", "gen",EX_RNG_SetGenerate, ""},
    {"RNG", "log",EX_RNG_SetLog, ""}
};

/**********************************************************************
 * @brief		Main program
 * @param[in]	None
 * @return	None
 **********************************************************************/
void EX_RNG(void)
{
    /* Add TC commands */
    debug_cmd_init(s_tEX_RNG_CMD, DEBUG_CMD_LIST_COUNT(s_tEX_RNG_CMD));
}

#endif /* RNG_TC */

/* --------------------------------- End Of File ------------------------------ */
