/**
 *******************************************************************************
 * @file        pga.c
 * @author      ABOV R&D Division
 * @brief       AES Example Code
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

#if defined(PGA_TC)
#include "abov_config.h"
#include "ex_common.h"
#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "hal_scu.h"
#include "hal_pcu.h"
#include "hal_pga.h"

#if !defined(_PGA)
#error "This chipset did not support this example."
#endif

#define EX_PGA_STR "PGA"
#define EX_PGA_LOG_STR "PGA :"
#define EX_PGA_ERR_STR "[E]PGA :"
#define EX_PGA_MAX_NUM  (CONFIG_PGA_MAX_COUNT - 1)

extern const char *pcCommStr[];
extern const char *pcCmdStr[];
extern const char *pcValStr[];
extern const char *pcOptStr[];

static enum debug_cmd_status EX_PGA_Help(int32_t n32Argc, char *pn8Argv[])
{
    uint32_t un32ShowOpt = 0;
    EX_COMM_STR_OPT_e eOpt[2];

    EX_COMMON_SetShowModuleInfo(EX_PGA_STR, CONFIG_PGA_MAX_COUNT);

    un32ShowOpt = EX_COMMON_GetShowOpt(pn8Argv[1]);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_INIT, NULL, EX_PGA_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_UNINIT, NULL, EX_PGA_MAX_NUM, eOpt, 0, NULL);

    eOpt[0] = EX_COMM_STR_OPT_ENA; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "amp", EX_PGA_MAX_NUM, eOpt, 1, "[cur]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_ENA, EX_COMM_STR_VAL_ENDIS, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "cur: 0~N(decimal) (output driving current)");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "gen", EX_PGA_MAX_NUM, eOpt, 0, "[gain] [-flw]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "gain: 0~N(decimal) (gain select)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-flw: [en] (follow config mode)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_ENA, EX_COMM_STR_VAL_ENDIS, NULL);
    }
    LOG("\n");

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_PGA_GetId(int32_t n32Argc, char *pn8Argv[], PGA_ID_e *peId)
{
    uint8_t un8Data = 0;
    if(n32Argc == 1)
    {
        EX_COMMON_SetShowModuleLog(EX_PGA_ERR_STR, (char *)pcOptStr[EX_COMM_STR_OPT_ARG], EX_COMM_STR_OPT_MAX);
        return DEBUG_CMD_INVALID;
    }

    un8Data = atoi(pn8Argv[1]); 
    if(un8Data >= CONFIG_PGA_MAX_COUNT)
    {
        LOG("%s Max Chan %d\n", EX_PGA_ERR_STR, CONFIG_PGA_MAX_COUNT);
        return DEBUG_CMD_INVALID;
    }

    *peId = (PGA_ID_e)un8Data;

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_PGA_Init(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    PGA_ID_e eId = PGA_ID_0;

    eDbgStatus = EX_PGA_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_PGA_Init(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_PGA_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_INIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_PGA_Uninit(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    PGA_ID_e eId = PGA_ID_0;

    eDbgStatus = EX_PGA_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_PGA_Uninit(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_PGA_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_UNINIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_PGA_SetAmp(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus;
    PGA_ID_e eId = PGA_ID_0;
    PGA_OUT_CURR_e eCurr = PGA_OUT_CURR_0;
    uint8_t un8Arg = 2;
    bool bEnable = false;
   
    eDbgStatus = EX_PGA_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], &bEnable);
    if (eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eCurr = (PGA_OUT_CURR_e)atoi(pn8Argv[un8Arg]); 

    if(eCurr >= PGA_OUT_CURR_MAX)
    {
        goto err;
    }

    eErr = HAL_PGA_SetAmp(eId, eCurr, bEnable);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_PGA_SetGain(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus;
    PGA_ID_e eId = PGA_ID_0;
    PGA_GAIN_e eGain = PGA_GAIN_0;
    uint8_t un8Arg = 2;
    bool bEnable = false;
   
    eDbgStatus = EX_PGA_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eGain = (PGA_GAIN_e)atoi(pn8Argv[un8Arg++]); 

    if(eGain >= PGA_GAIN_MAX)
    {
        goto err;
    }

    if (strncmp(pn8Argv[un8Arg], "-flw", 4) == 0)
    {
        un8Arg++;
        eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], &bEnable);
        if (eDbgStatus != DEBUG_CMD_SUCCESS)
        {
            goto err;
        }
    }

    eErr = HAL_PGA_SetGain(eId, eGain, bEnable);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static const struct debug_cmd s_tEX_PGA_CMD[] =
{
    {"PGA", "h", EX_PGA_Help, "help"},
    {"PGA", "init", EX_PGA_Init, ""},
    {"PGA", "uninit", EX_PGA_Uninit, ""},
    {"PGA", "amp", EX_PGA_SetAmp, ""},
    {"PGA", "gain", EX_PGA_SetGain, ""},
};

/**********************************************************************
 * @brief		Main program
 * @param[in]	None
 * @return	None
 **********************************************************************/
void EX_PGA(void)
{
    /* Add TC commands */
    debug_cmd_init(s_tEX_PGA_CMD, DEBUG_CMD_LIST_COUNT(s_tEX_PGA_CMD));
}

#endif /* PGA_TC */

/* --------------------------------- End Of File ------------------------------ */
