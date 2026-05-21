/**
 *******************************************************************************
 * @file        ex_str.c
 * @author      ABOV R&D Division
 * @brief       Example Common Function
 *
 * Copyright 2024 ABOV Semiconductor Co.,Ltd. All rights reserved.
 *
 * This file is licensed under terms that are found in the LICENSE file
 * located at Document directory.
 * If this file is delivered or shared without applicable license terms,
 * the terms of the BSD-3-Clause license shall be applied.
 * Reference: https://opensource.org/licenses/BSD-3-Clause
 ******************************************************************************/

#include <string.h>
#include "ex_common.h"
#include "debug.h"
#include "debug_cmd.h"
#include "debug_log.h"

#define EX_COMM_BUF_SIZE 128
#define EX_COMM_USAGE_STR "Usage : 'h opt' display specific parameters\n"
#define EX_COMM_EMPTY_USAGE_STR "Usage : \n"
#define EX_COMM_NUMBER_OF_STR "Number of"
#define EX_COMM_EVT_FIRE_STR "evt fire"

const char *pcCommStr[] = {
    EX_COMM_USAGE_STR,
    EX_COMM_EMPTY_USAGE_STR,
    EX_COMM_NUMBER_OF_STR,
    EX_COMM_EVT_FIRE_STR
};

#define EX_COMM_OPT_SRC_STR "src"
#define EX_COMM_OPT_OPS_STR "ops"
#define EX_COMM_OPT_MODE_STR "mode"
#define EX_COMM_OPT_ENA_STR "en"
#define EX_COMM_OPT_DIV_STR "div"
#define EX_COMM_OPT_MCCR_STR "mccr"
#define EX_COMM_OPT_EDGE_STR "edge"
#define EX_COMM_OPT_POL_STR "pol"
#define EX_COMM_OPT_DA_STR "da"
#define EX_COMM_OPT_DB_STR "db"
#define EX_COMM_OPT_PIN_STR "pin"
#define EX_COMM_OPT_NC_STR "nc"
#define EX_COMM_OPT_LDO_STR "ldo"
#define EX_COMM_OPT_SHIFT_STR "shift"
#define EX_COMM_OPT_CNT_STR "cnt"
#define EX_COMM_OPT_DASH_IO_STR "-io"
#define EX_COMM_OPT_RESET_STR "rst"
#define EX_COMM_OPT_LEVEL_STR "lvl"
#define EX_COMM_OPT_ARG_STR "arg"
#define EX_COMM_OPT_CLK_STR "clk"
#define EX_COMM_OPT_AON_STR "aon"
#define EX_COMM_OPT_VAL_STR "val"
#define EX_COMM_OPT_NUM_STR "num"
#define EX_COMM_OPT_MASK_STR "msk"

const char *pcOptStr[] = {
    EX_COMM_OPT_SRC_STR,
    EX_COMM_OPT_OPS_STR,
    EX_COMM_OPT_MODE_STR,
    EX_COMM_OPT_ENA_STR, 
    EX_COMM_OPT_DIV_STR,
    EX_COMM_OPT_MCCR_STR,
    EX_COMM_OPT_EDGE_STR,
    EX_COMM_OPT_POL_STR,
    EX_COMM_OPT_DA_STR,
    EX_COMM_OPT_DB_STR,
    EX_COMM_OPT_PIN_STR,
    EX_COMM_OPT_NC_STR,
    EX_COMM_OPT_LDO_STR,
    EX_COMM_OPT_SHIFT_STR,
    EX_COMM_OPT_CNT_STR,
    EX_COMM_OPT_DASH_IO_STR,
    EX_COMM_OPT_RESET_STR,
    EX_COMM_OPT_LEVEL_STR,
    EX_COMM_OPT_ARG_STR,
    EX_COMM_OPT_CLK_STR,
    EX_COMM_OPT_AON_STR,
    EX_COMM_OPT_VAL_STR,
    EX_COMM_OPT_NUM_STR,
    EX_COMM_OPT_MASK_STR
};

#define EX_COMM_CMD_INIT_STR "init"
#define EX_COMM_CMD_UNINIT_STR "uninit"
#define EX_COMM_CMD_CLK_STR "clk"
#define EX_COMM_CMD_CONFIG_STR "config"
#define EX_COMM_CMD_START_STR "start"
#define EX_COMM_CMD_STOP_STR "stop"
#define EX_COMM_CMD_LOG_STR "log"

const char *pcCmdStr[] = {
    EX_COMM_CMD_INIT_STR,
    EX_COMM_CMD_UNINIT_STR,
    EX_COMM_CMD_CLK_STR,
    EX_COMM_CMD_CONFIG_STR,
    EX_COMM_CMD_START_STR,
    EX_COMM_CMD_STOP_STR,
    EX_COMM_CMD_LOG_STR,
};

#define EX_COMM_VAL_ENDIS_STR "e(en)/d(dis)"
#define EX_COMM_VAL_N_NUM "0~N"
#define EX_COMM_VAL_N_255 "0~255"
#define EX_COMM_VAL_EDGE "f(fall)/r(rise)"
#define EX_COMM_VAL_BOTH "b(both)"
#define EX_COMM_VAL_ONOFF "on/off"
#define EX_COMM_VAL_LOWHIGH "l(low)/h(high)"
#define EX_COMM_VAL_CLKSRC "l(lsi)/s(lse)/h(hsi)/e(hse)/p(pll)"
#define EX_COMM_VAL_OPS "p(poll)/i(intr)"
#define EX_COMM_VAL_CLKPATH "p(pclk)/m(mccr)"
#define EX_COMM_VAL_MASK "m(mask)/n(nonmask)"

const char *pcValStr[] = {
    EX_COMM_VAL_ENDIS_STR,
    EX_COMM_VAL_N_NUM,
    EX_COMM_VAL_N_255,
    EX_COMM_VAL_EDGE,
    EX_COMM_VAL_BOTH,
    EX_COMM_VAL_ONOFF,
    EX_COMM_VAL_LOWHIGH,
    EX_COMM_VAL_CLKSRC,
    EX_COMM_VAL_OPS,
    EX_COMM_VAL_CLKPATH,
    EX_COMM_VAL_MASK
};

void EX_COMMON_SetShowModuleInfo(char *pcName, int n32Cnt)
{
    LOG("\n%s %s %d\n\n", pcCommStr[EX_COMM_STR_NUMBER_OF], pcName, n32Cnt);
}

void EX_COMMON_SetShowModuleLog(char *pcName, char *pcStr, EX_COMM_STR_OPT_e eOpt)
{
    if (eOpt != EX_COMM_STR_OPT_MAX)
    {
        LOG("%s %s[%s]\n", pcName, pcStr, pcOptStr[eOpt]);
    }
    else
    {
        LOG("%s %s\n", pcName, pcStr);
    }
}

void EX_COMMON_SetShowOptVal(int nSpace, bool bIf, EX_COMM_STR_OPT_e eOpt, EX_COMM_STR_VAL_e eVal, char *pcStr)
{
    char acStr[EX_COMM_BUF_SIZE];
    int ret = 0;
    
    ret = snprintf(&acStr[0], 3, " \t");
    memset(&acStr[2], 0x20, sizeof(acStr) - 2);

    ret += nSpace;

    if (bIf == true)
    {
        ret += snprintf(&acStr[ret], 3, "if ");
        acStr[ret-1] = 0x20;
    }

    if (eOpt != EX_COMM_STR_OPT_MAX)
    {
        ret += snprintf(&acStr[ret], 12,"%s: ", pcOptStr[eOpt]);
    }
   
    if (eVal != EX_COMM_STR_VAL_MAX)
    {
        ret += snprintf(&acStr[ret], 128 - ret,"%s", pcValStr[eVal]);
    }

    if (pcStr != NULL)
    {
        ret += snprintf(&acStr[ret], 128 - ret,"%s", pcStr);
    }

    LOG("%s\n", acStr);
}

void EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_e eCmd, char *pcCmd, int n32Cnt, EX_COMM_STR_OPT_e *eOpt, int n32OptCnt, char *pcExtStr)
{
    char acStr[EX_COMM_BUF_SIZE];
    char *pcStr;
    int ret = 0;

    if (eCmd == EX_COMM_STR_CMD_MAX)
        pcStr = pcCmd;
    else
        pcStr = (char *)pcCmdStr[eCmd];
       
    ret += snprintf(&acStr[0], EX_COMM_BUF_SIZE, " %s\t", pcStr);

    if (n32Cnt > -1)
    {
        ret += snprintf(&acStr[ret], EX_COMM_BUF_SIZE - ret, " [0~%d]", n32Cnt);
    }
    
    if (n32OptCnt > 0)
    {
        for(int i = 0; i < n32OptCnt; i++)
        {
            ret += snprintf(&acStr[ret], 12, " [%s]", pcOptStr[eOpt[i]]);
        }
    }

    if (pcExtStr != NULL)
    {
         ret = snprintf(&acStr[ret], EX_COMM_BUF_SIZE - ret, " %s", pcExtStr);
    }

    LOG("%s\n", acStr);
}

int EX_COMMON_GetShowOpt(char *pcOpt)
{
    int ret = 0;
    EX_COMM_STR_e eStr = EX_COMM_STR_USAGE;
    if((strncmp(pcOpt, "opt", 3) == 0))
    {
        eStr = EX_COMM_STR_EMPTY_USAGE;
        ret = 1;
    }
    LOG("%s", pcCommStr[eStr]);
    return ret;
}

enum debug_cmd_status EX_COMMON_GetEnable(char *chr, bool *pbEnable)
{
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;
    switch(*chr)
    {
        case 'e':
            *pbEnable = true;
            break;
        case 'd':
            *pbEnable = false;
            break;
        default:
            eDbgStatus = DEBUG_CMD_INVALID;
            break;
    }

    return eDbgStatus;
}

enum debug_cmd_status EX_COMMON_GetLevel(char *chr, bool *pbLevel)
{
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;
    switch(*chr)
    {
        case 'h':
            *pbLevel = true;
            break;
        case 'l':
            *pbLevel = false;
            break;
        default:
            eDbgStatus = DEBUG_CMD_INVALID;
            break;
    }

    return eDbgStatus;
}

/* --------------------------------- End Of File ------------------------------ */
