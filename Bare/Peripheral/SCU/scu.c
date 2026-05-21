/**
 *******************************************************************************
 * @file        scu.c
 * @author      ABOV R&D Division
 * @brief       SCU Example Code
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

#include "abov_config.h"
#include "ex_configs.h"
#include "ex_common.h"
#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "hal_scu.h"

#define EX_SCU_STR "SCU"
#define EX_SCU_LOG_STR "SCU :"
#define EX_SCU_ERR_STR "[E]SCU :"

extern const char *pcCommStr[];
extern const char *pcCmdStr[];
extern const char *pcValStr[];
extern const char *pcOptStr[];

typedef enum {
    EX_SCU_CMD_RESET,
    EX_SCU_CMD_BPIN,
    EX_SCU_CMD_RSTEN,
    EX_SCU_CMD_RPINDB,
    EX_SCU_CMD_RMEM,
    EX_SCU_CMD_MAX
}EX_SCU_CMD_e;

static enum debug_cmd_status EX_SCU_Help(int32_t n32Argc, char *pn8Argv[])
{
    uint32_t un32ShowOpt = 0;
    EX_COMM_STR_OPT_e eOpt[2];

    un32ShowOpt = EX_COMMON_GetShowOpt(pn8Argv[1]);
 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "reset", -1, NULL, 0, "sw reset");
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "bpin", -1, NULL, 0, "boot pin level");

    eOpt[0] = EX_COMM_STR_OPT_RESET;
    eOpt[1] = EX_COMM_STR_OPT_ENA;
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "rsten", -1, eOpt, 2, NULL);
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_RESET, EX_COMM_STR_VAL_MAX, "s(sw)/c(cpu)/e(ext-pin)/l(cpu lock-up)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_ENA, EX_COMM_STR_VAL_ENDIS, NULL);
    }

    eOpt[0] = EX_COMM_STR_OPT_ENA;
    eOpt[1] = EX_COMM_STR_OPT_CNT;
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "rpindb", -1, eOpt, 2, NULL);
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_ENA, EX_COMM_STR_VAL_ENDIS, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_CNT, EX_COMM_STR_VAL_N_NUM, NULL);
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "rmem", -1, eOpt, 0, "[addr] [len]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "addr: 0~0xN(32bit hexa)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "len: 0~N(32bit dec)");
    }

    return DEBUG_CMD_SUCCESS;
}


static enum debug_cmd_status EX_SCU_SetSWReset(int32_t n32Argc, char *pn8Argv[])
{
    SystemDelayMS(1);

    HAL_SCU_SetSWReset();

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_SCU_GetBootPinLevel(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    bool bLevel = false;

    eErr = HAL_SCU_GetBootPinLevel(&bLevel);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }
    
    LOG("%s level=%s\n", EX_SCU_LOG_STR, (bLevel == false ? "L" : "H"));

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_SCU_SetResetEnable(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;
    uint8_t un8Arg = 1;
    bool bEnable = false;
    SCU_RST_e eRst = SCU_RST_MAX;

    if (pn8Argv[un8Arg][0] == 's')
        eRst = SCU_RST_SW;
    else if (pn8Argv[un8Arg][0] == 'c')
        eRst = SCU_RST_CPU;
    else if (pn8Argv[un8Arg][0] == 'e')
        eRst = SCU_RST_EXT_PIN;
    else if (pn8Argv[un8Arg][0] == 'l')
        eRst = SCU_RST_CPU_LOCKUP;
    else
    {
        goto err;
    }
    
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_ERR_STR, NULL, EX_COMM_STR_OPT_RESET);
        goto err;
    }

    eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[++un8Arg][0], &bEnable);
    if (eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_ERR_STR, NULL, EX_COMM_STR_OPT_ENA);
        goto err;
    }

    eErr = HAL_SCU_SetReset(eRst, bEnable);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }
    
    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_SCU_SetResetPinDeb(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;
    uint8_t un8Arg = 1, un8Count = 0;
    bool bEnable = false;

    eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], &bEnable);
    if (eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_ERR_STR, NULL, EX_COMM_STR_OPT_ENA);
        goto err;
    }

    un8Count = (uint8_t)atoi(pn8Argv[un8Arg]); 

    eErr = HAL_SCU_SetResetPinDebounce(un8Count, bEnable);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }
    
    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_SCU_GetData(int32_t n32Argc, char *pn8Argv[])
{
    uint8_t un8Arg = 1;
    uint32_t un32Mem = 0, un32Len = 1;
   
    sscanf(pn8Argv[un8Arg++], "%X", &un32Mem);

    if (n32Argc > 2)
    {
        un32Len = (uint32_t)atoi(pn8Argv[un8Arg]);
    }

    for(int i = 0; i < un32Len; i++)
    {
        LOG("%s A=0x%08X, D=0x%08X\n", EX_SCU_LOG_STR, (un32Mem + i*4), *(uint32_t *)(un32Mem + i*4));
    }

    return DEBUG_CMD_SUCCESS;
}

static const struct debug_cmd s_tEX_SCU_CMD[] =
{
    {EX_SCU_STR, "h", EX_SCU_Help, "help"},
    {EX_SCU_STR, "reset", EX_SCU_SetSWReset,""},
    {EX_SCU_STR, "bpin", EX_SCU_GetBootPinLevel,""},
    {EX_SCU_STR, "rsten", EX_SCU_SetResetEnable,""},
    {EX_SCU_STR, "rpindb", EX_SCU_SetResetPinDeb,""},
    {EX_SCU_STR, "rmem", EX_SCU_GetData,""},
};

void EX_SCU(void)
{
    /* Add TC commands */
    debug_cmd_init(s_tEX_SCU_CMD, DEBUG_CMD_LIST_COUNT(s_tEX_SCU_CMD));
}

/* --------------------------------- End Of File ------------------------------ */
