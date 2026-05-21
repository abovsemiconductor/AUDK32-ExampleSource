/**
 *******************************************************************************
 * @file        ex_common.h
 * @author      ABOV R&D Division
 * @brief       Example Common
 *
 * Copyright 2024 ABOV Semiconductor Co.,Ltd. All rights reserved.
 *
 * This file is licensed under terms that are found in the LICENSE file
 * located at Document directory.
 * If this file is delivered or shared without applicable license terms,
 * the terms of the BSD-3-Clause license shall be applied.
 * Reference: https://opensource.org/licenses/BSD-3-Clause
 ******************************************************************************/

#ifndef _EXAMPLE_COMMON_H_
#define _EXAMPLE_COMMON_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>

#define EX_COMM_MODULE_ERR_STR "This chipset did not support this example."

typedef enum {
    EX_COMM_STR_USAGE,
    EX_COMM_STR_EMPTY_USAGE,
    EX_COMM_STR_NUMBER_OF,
    EX_COMM_STR_EVT_FIRE,
    EX_COMM_STR_MAX = 255
} EX_COMM_STR_e;

typedef enum {
    EX_COMM_STR_CMD_INIT,
    EX_COMM_STR_CMD_UNINIT,
    EX_COMM_STR_CMD_CLK,
    EX_COMM_STR_CMD_CONFIG,
    EX_COMM_STR_CMD_START,
    EX_COMM_STR_CMD_STOP,
    EX_COMM_STR_CMD_LOG,
    EX_COMM_STR_CMD_MAX
} EX_COMM_STR_CMD_e;

typedef enum {
    EX_COMM_STR_VAL_ENDIS,
    EX_COMM_STR_VAL_N_NUM,
    EX_COMM_STR_VAL_N_255,
    EX_COMM_STR_VAL_EDGE,
    EX_COMM_STR_VAL_BOTH,
    EX_COMM_STR_VAL_ONOFF,
    EX_COMM_STR_VAL_LOWHIGH,
    EX_COMM_STR_VAL_CLKSRC,
    EX_COMM_STR_VAL_OPS,
    EX_COMM_STR_VAL_CLKPATH,
    EX_COMM_STR_VAL_MASK,
    EX_COMM_STR_VAL_MAX
} EX_COMM_STR_VAL_e;

typedef enum {
    EX_COMM_STR_OPT_SRC,
    EX_COMM_STR_OPT_OPS,
    EX_COMM_STR_OPT_MODE,
    EX_COMM_STR_OPT_ENA,
    EX_COMM_STR_OPT_DIV,
    EX_COMM_STR_OPT_MCCR,
    EX_COMM_STR_OPT_EDGE,
    EX_COMM_STR_OPT_POL,
    EX_COMM_STR_OPT_DA,
    EX_COMM_STR_OPT_DB,
    EX_COMM_STR_OPT_PIN,
    EX_COMM_STR_OPT_NC,
    EX_COMM_STR_OPT_LDO,
    EX_COMM_STR_OPT_SHIFT,
    EX_COMM_STR_OPT_CNT,
    EX_COMM_STR_OPT_DASH_IO,
    EX_COMM_STR_OPT_RESET,
    EX_COMM_STR_OPT_LEVEL,
    EX_COMM_STR_OPT_ARG,
    EX_COMM_STR_OPT_CLK,
    EX_COMM_STR_OPT_AON,
    EX_COMM_STR_OPT_VAL,
    EX_COMM_STR_OPT_NUM,
    EX_COMM_STR_OPT_MASK,
    EX_COMM_STR_OPT_MAX
} EX_COMM_STR_OPT_e;

void EX_COMMON_SetShowModuleInfo(char *pcName, int n32Cnt);
int EX_COMMON_GetShowOpt(char *pcOpt);
void EX_COMMON_SetShowModuleLog(char *pcName, char *pcStr, EX_COMM_STR_OPT_e eOpt);
#if defined (EX_COMMON_UNSUPPORT_OPTION_MSG)
#define EX_COMMON_SetShowOptVal(nSpace, bIf, eOpt, eVal, pcStr)     (void)0
#else
void EX_COMMON_SetShowOptVal(int nSpace, bool bIf, EX_COMM_STR_OPT_e eOpt, EX_COMM_STR_VAL_e eVal, char *pcStr);
#endif
void EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_e eCmd, char *pcCmd, int n32Cnt, EX_COMM_STR_OPT_e *eOpt, int n32OptCnt, char *pcExtStr);

enum debug_cmd_status EX_COMMON_GetEnable(char *chr, bool *pbEnable);
enum debug_cmd_status EX_COMMON_GetLevel(char *chr, bool *pbLevel);

#ifdef __cplusplus
}
#endif

#endif /* _EXAMPLE_COMMON_H_ */
