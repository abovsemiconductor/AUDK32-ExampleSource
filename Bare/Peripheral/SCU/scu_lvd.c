/**
 *******************************************************************************
 * @file        scu_lvd.c
 * @author      ABOV R&D Division
 * @brief       SCU LVD Example Code
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

#if defined(SCU_LVD_TC)
#include "abov_config.h"
#include "ex_configs.h"
#include "ex_common.h"
#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "hal_scu_lvd.h"

#define EX_SCU_LVD_STR "SCULVD"
#define EX_SCU_LVD_LOG_STR "SCULVD :"
#define EX_SCU_LVD_ERR_STR "[E]SCULVD :"
#define EX_SCU_LVD_IRQ_PRIO 3
#define EX_SCU_LVD_DISP_COUNT 100000

static SCULVD_Context_t s_tSCULVDContext;
static bool s_bISRLog = true;
static uint32_t s_un32LogDispCnt = EX_SCU_LVD_DISP_COUNT;
static uint32_t s_un32LogCurCnt = 0;

static enum debug_cmd_status EX_SCU_LVD_Help(int32_t n32Argc, char *pn8Argv[])
{
    uint32_t un32ShowOpt = 0;
    EX_COMM_STR_OPT_e eOpt[4];

    un32ShowOpt = EX_COMMON_GetShowOpt(pn8Argv[1]);

    eOpt[0] = EX_COMM_STR_OPT_ENA;
    eOpt[1] = EX_COMM_STR_OPT_LEVEL;
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "lvi", -1, eOpt, 2, "[-wake]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_ENA, EX_COMM_STR_VAL_ENDIS, "/n(non-mask)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_LEVEL, EX_COMM_STR_VAL_MAX, "0~15 Level");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-wake: [en]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_ENA, EX_COMM_STR_VAL_ENDIS, NULL);
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "lvr", -1, eOpt, 2, "[-auto]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_ENA, EX_COMM_STR_VAL_ENDIS, "/n(non-mask)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_LEVEL, EX_COMM_STR_VAL_MAX, "0~15 Level");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-auto: [en]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_ENA, EX_COMM_STR_VAL_ENDIS, NULL);
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "lvre", -1, eOpt, 0, "reset from lvr");
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_LOG, NULL, -1, eOpt, 0, "on:opt / off");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "opt: [cnt]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_CNT, EX_COMM_STR_VAL_N_NUM, NULL);
    }
    LOG("\n");

    return DEBUG_CMD_SUCCESS;
}

static void EX_SCU_LVD_IRQHandler(uint32_t un32Event, void *pContext)
{
    if(un32Event & SCULVD_IND_EVENT_MASK)
    {
        if(s_bISRLog == true && s_un32LogDispCnt == s_un32LogCurCnt)
        {
            EX_COMMON_SetShowModuleLog(EX_SCU_LVD_LOG_STR, "lvi mask evt fired", EX_COMM_STR_OPT_MAX);
        }
    }

    if(un32Event & SCULVD_IND_EVENT_NON_MASK)
    {
        if(s_bISRLog == true && s_un32LogDispCnt == s_un32LogCurCnt)
        {
            EX_COMMON_SetShowModuleLog(EX_SCU_LVD_LOG_STR, "lvi non-mask evt fired", EX_COMM_STR_OPT_MAX);
        }
    }

    s_un32LogCurCnt++;
    if(s_un32LogCurCnt > s_un32LogDispCnt)
    {
        s_un32LogCurCnt = 0;
    }
}

static enum debug_cmd_status EX_SCU_LVD_SetLVI(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;
    uint8_t un8Arg = 1;
    SCULVD_IND_LVL_e eLvl = SCULVD_IND_LVL_MAX;
    SCULVD_IND_INTR_e eIntr = SCULVD_IND_INTR_MAX;
    bool bEnable = false;

    switch (pn8Argv[un8Arg++][0])
    {
        case 'e':
            eIntr = SCULVD_IND_INTR_MASK;
            bEnable = true;
            break;
        case 'n':
            eIntr = SCULVD_IND_INTR_NON_MASK;
            bEnable = true;
            break;
        case 'd':
            eIntr = SCULVD_IND_INTR_NONE;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_SCU_LVD_ERR_STR, NULL, EX_COMM_STR_OPT_ENA);
            goto err;
    }

    eLvl = (SCULVD_IND_LVL_e)atoi(pn8Argv[un8Arg++]);

    if(eLvl >= SCULVD_IND_LVL_MAX || eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_SCU_LVD_SetIndicatorEnable(eLvl, bEnable);
    if(eErr != HAL_ERR_OK)
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_LVD_ERR_STR, "IndicatorEnable", EX_COMM_STR_OPT_ENA);
        goto err;
    }

    eErr = HAL_SCU_LVD_SetIRQIndicator(EX_SCU_LVD_IRQHandler, &s_tSCULVDContext, EX_SCU_LVD_IRQ_PRIO, eIntr);
    if(eErr != HAL_ERR_OK)
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_LVD_ERR_STR, "IndicatorEnable", EX_COMM_STR_OPT_ENA);
        goto err;
    }

    if(strncmp(pn8Argv[un8Arg], "-wake", 5) == 0)
    {
        un8Arg++;
        eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], &bEnable);
        if (eDbgStatus != DEBUG_CMD_SUCCESS)
        {
            EX_COMMON_SetShowModuleLog(EX_SCU_LVD_ERR_STR, "-wake", EX_COMM_STR_OPT_ENA);
            goto err;
        }

        eErr = HAL_SCU_LVD_SetWakeupSrc(bEnable);
        if(eErr != HAL_ERR_OK)
        {
            EX_COMMON_SetShowModuleLog(EX_SCU_LVD_ERR_STR, "WakeupSrc", EX_COMM_STR_OPT_MAX);
            goto err;
        }
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_SCU_LVD_SetLVR(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;
    uint8_t un8Arg = 1;
    SCULVD_RST_LVL_e eLvl = SCULVD_RST_LVL_MAX;
    bool bEnable = false, bAuto = false;

    eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], &bEnable);
    if (eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_LVD_ERR_STR, NULL, EX_COMM_STR_OPT_ENA);
        goto err;
    }

    eLvl = (SCULVD_RST_LVL_e)atoi(pn8Argv[un8Arg++]);

    if(eLvl >= SCULVD_RST_LVL_MAX || eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    if(strncmp(pn8Argv[un8Arg], "-auto", 5) == 0)
    {
        un8Arg++;
        eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], &bEnable);
        if (eDbgStatus != DEBUG_CMD_SUCCESS)
        {
            EX_COMMON_SetShowModuleLog(EX_SCU_LVD_ERR_STR, "-auto", EX_COMM_STR_OPT_ENA);
            goto err;
        }
    }

    eErr = HAL_SCU_LVD_SetResetEnable(eLvl, bEnable, bAuto);
    if(eErr != HAL_ERR_OK)
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_LVD_ERR_STR, "ResetEnable", EX_COMM_STR_OPT_MAX);
        goto err;
    }

    eErr = HAL_SCU_LVD_SetResetSrc(bEnable);
    if(eErr != HAL_ERR_OK)
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_LVD_ERR_STR, "ResetSrc", EX_COMM_STR_OPT_MAX);
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;

}

static enum debug_cmd_status EX_SCU_LVD_GetLVR(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    bool bEvent = false;

    eErr = HAL_SCU_LVD_GetResetSrcEvent(&bEvent);
    if(eErr != HAL_ERR_OK)
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_LVD_ERR_STR, "ResetEnable", EX_COMM_STR_OPT_MAX);
        return DEBUG_CMD_INVALID;
    }

    LOG("%s Reset %s from LVR.\n", EX_SCU_LVD_ERR_STR, (bEvent == true ? "is" : "is not"));

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_SCU_LVD_SetLog(int32_t n32Argc, char *pn8Argv[])
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

    LOG("%s ISR Log %s.\n", EX_SCU_LVD_LOG_STR, (s_bISRLog == true ? "on":"off"));

    return DEBUG_CMD_SUCCESS;
}

static const struct debug_cmd s_tEX_SCU_LVD_CMD[] =
{
    {"SCULVD", "h", EX_SCU_LVD_Help, "help"},
    {"SCULVD", "lvi", EX_SCU_LVD_SetLVI, ""},
    {"SCULVD", "lvr", EX_SCU_LVD_SetLVR, ""},
    {"SCULVD", "lvre", EX_SCU_LVD_GetLVR, ""},
    {"SCULVD", "log", EX_SCU_LVD_SetLog, ""}
};

void EX_SCU_LVD(void)
{
    /* Add TC commands */
    debug_cmd_init(s_tEX_SCU_LVD_CMD, DEBUG_CMD_LIST_COUNT(s_tEX_SCU_LVD_CMD));
}

#endif /* SCU_LVD_TC */

/* --------------------------------- End Of File ------------------------------ */
