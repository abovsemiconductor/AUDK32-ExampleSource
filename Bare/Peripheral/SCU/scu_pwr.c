/**
 *******************************************************************************
 * @file        scu_pwr.c
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

#if defined(SCU_PWR_TC)
#include "abov_config.h"
#include "ex_configs.h"
#include "ex_common.h"
#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "hal_scu.h"
#include "hal_scu_pwr.h"
#include "hal_scu_clk.h"

#if defined (EX_COMMON_ENABLE_CUSTOM_SSCANF)
#define sscanf(str, fmt, out) EX_COMMON_ParseByFormat((str), (fmt)[1], (uint32_t *)(out))
#endif

#define EX_SCU_PWR_STR "SCUPWR"
#define EX_SCU_PWR_LOG_STR "SCUPWR :"
#define EX_SCU_PWR_ERR_STR "[E]SCUPWR :"
#define EX_SCU_PWR_BACKUP_DATA_CNT 4

static enum debug_cmd_status EX_SCU_PWR_Help(int32_t n32Argc, char *pn8Argv[])
{
    uint32_t un32ShowOpt = 0;
    EX_COMM_STR_OPT_e eOpt[4];

    un32ShowOpt = EX_COMMON_GetShowOpt(pn8Argv[1]);

    eOpt[0] = EX_COMM_STR_OPT_MODE;
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "pwr", -1, eOpt, 1, NULL);
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MODE, EX_COMM_STR_VAL_MAX, "s(sleep)/d(deepsleep1)/t(deepsleep2)");
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "d or t");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "smclk: [clk] [div] (mclk before entering deepsleep 1 or 2");
        EX_COMMON_SetShowOptVal(3, false, EX_COMM_STR_OPT_CLK, EX_COMM_STR_VAL_CLKSRC, "/w(wdt)");
        EX_COMMON_SetShowOptVal(3, false, EX_COMM_STR_OPT_DIV, EX_COMM_STR_VAL_N_NUM, NULL);
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "wmclk: [clk] [div] (mclk after wake-up deepsleep 1 or 2");
        EX_COMMON_SetShowOptVal(3, false, EX_COMM_STR_OPT_CLK, EX_COMM_STR_VAL_CLKSRC, "/w(wdt)");
        EX_COMMON_SetShowOptVal(3, false, EX_COMM_STR_OPT_DIV, EX_COMM_STR_VAL_N_NUM, NULL);
#if defined (EXTRN_HAS_DEEP_SLEEP_SUB_MODE)
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "dsm: [mode] [pin]");
        EX_COMMON_SetShowOptVal(3, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "mode: 0/1/2/3");
        EX_COMMON_SetShowOptVal(3, true, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "1: [nmi]");
        EX_COMMON_SetShowOptVal(4, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "nmi: e(en)/d(dis)");
        EX_COMMON_SetShowOptVal(3, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "pin: 0x0~0xN(hexa)");
#endif
    }

#if defined (EXTRN_HAS_DEEP_SLEEP_SUB_MODE)
#else
    eOpt[0] = EX_COMM_STR_OPT_AON;
    eOpt[1] = EX_COMM_STR_OPT_ENA;
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "aon", -1, eOpt, 2, NULL);
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_AON, EX_COMM_STR_VAL_CLKSRC, "/v(vdc)/b(bgr)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_ENA, EX_COMM_STR_VAL_ENDIS, NULL);
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "vdc", -1, eOpt, 0, "[delay]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "delay : 0 ~ N");
    }
#endif
    eOpt[0] = EX_COMM_STR_OPT_ENA;
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "bak", -1, eOpt, 1, NULL);
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_ENA, EX_COMM_STR_VAL_ENDIS, NULL);
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "bakdw", -1, eOpt, 0, "[num] [val] ...");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_NUM, EX_COMM_STR_VAL_N_NUM, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_VAL, EX_COMM_STR_VAL_MAX, "0x0~0xN(hexa)");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "bakdr", -1, eOpt, 0, "[num] ...");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_NUM, EX_COMM_STR_VAL_N_NUM, NULL);
    }
    LOG("\n");

    return DEBUG_CMD_SUCCESS;
}

#if defined (EXTRN_HAS_DEEP_SLEEP_SUB_MODE)
#if defined (EX_SCU_PWR_NO_IRQ)
#else
static void EX_SCU_PWR_WkUpIRQHandler(uint32_t un32Event, void *pContext)
{
    if(un32Event == SCUPWR_DS_WKUP_REASON_1)
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_PWR_LOG_STR, "wkup ds 1 evt fired", EX_COMM_STR_OPT_MAX);
    }
}
#endif
#endif

static enum debug_cmd_status EX_SCU_PWR_GetMClk(uint8_t un8Argc, char *pn8Argv[], SCUCLK_MCLK_CFG_t *ptCfg)
{
    uint8_t un8Arg = un8Argc;
    uint16_t un16Data = 0;

    if (pn8Argv[un8Arg][0] == 'l')
        ptCfg->eMClk = SCUCLK_SRC_LSI;
    else if (pn8Argv[un8Arg][0] == 's')
        ptCfg->eMClk = SCUCLK_SRC_LSE;
    else if (pn8Argv[un8Arg][0] == 'h')
        ptCfg->eMClk = SCUCLK_SRC_HSI;
    else if (pn8Argv[un8Arg][0] == 'e')
        ptCfg->eMClk = SCUCLK_SRC_HSE;
    else if (pn8Argv[un8Arg][0] == 'p')
        ptCfg->eMClk = SCUCLK_SRC_PLL;
    else if (pn8Argv[un8Arg][0] == 'w')
        ptCfg->eMClk = SCUCLK_SRC_WDT;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_PWR_ERR_STR, "mclk", EX_COMM_STR_OPT_CLK);
        goto err;
    }

    un8Arg++;
    un16Data = ((uint16_t)atoi(pn8Argv[un8Arg]));
    if (un16Data == 0) 
        ptCfg->ePreMClkDiv = SCUCLK_DIV_NONE;
    else if (un16Data == 2) 
        ptCfg->ePreMClkDiv = SCUCLK_DIV_2;
    else if (un16Data == 4) 
        ptCfg->ePreMClkDiv = SCUCLK_DIV_4;
    else if (un16Data == 8) 
        ptCfg->ePreMClkDiv = SCUCLK_DIV_8;
    else if (un16Data == 16) 
        ptCfg->ePreMClkDiv = SCUCLK_DIV_16;
    else if (un16Data == 32) 
        ptCfg->ePreMClkDiv = SCUCLK_DIV_32;
    else if (un16Data == 64) 
        ptCfg->ePreMClkDiv = SCUCLK_DIV_64;
    else if (un16Data == 128) 
        ptCfg->ePreMClkDiv = SCUCLK_DIV_128;
    else if (un16Data == 256) 
        ptCfg->ePreMClkDiv = SCUCLK_DIV_256;
    else if (un16Data == 512) 
        ptCfg->ePreMClkDiv = SCUCLK_DIV_512;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_PWR_ERR_STR, "mclk", EX_COMM_STR_OPT_DIV);
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

#if defined (EXTRN_HAS_DEEP_SLEEP_SUB_MODE)
static enum debug_cmd_status EX_SCU_PWR_GetDeepSleepMode(uint8_t un8Argc, char *pn8Argv[], SCUPWR_DS_CFG_t *ptCfg, bool *pbIntrEnable)
{
    uint8_t un8Arg = un8Argc;
    uint32_t un32Data = 0;

    if (pn8Argv[un8Arg][0] == '0')
        ptCfg->eDsMode = SCUPWR_DS_MODE_0;
    else if (pn8Argv[un8Arg][0] == '1')
        ptCfg->eDsMode = SCUPWR_DS_MODE_1;
    else if (pn8Argv[un8Arg][0] == '2')
        ptCfg->eDsMode = SCUPWR_DS_MODE_2;
    else if (pn8Argv[un8Arg][0] == '3')
        ptCfg->eDsMode = SCUPWR_DS_MODE_3;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_PWR_ERR_STR, "mode", EX_COMM_STR_OPT_CLK);
        goto err;
    }

    if (ptCfg->eDsMode == SCUPWR_DS_MODE_1)
    {
        un8Arg++;
        EX_COMMON_GetEnable(&pn8Argv[un8Arg][0], pbIntrEnable);
    }

    un8Arg++;
    sscanf(pn8Argv[un8Arg], "%x", &un32Data);
    ptCfg->un8WkUpPinEnable = (uint8_t)un32Data;

    return DEBUG_CMD_SUCCESS;
err:
    return DEBUG_CMD_INVALID;

}
#endif

static enum debug_cmd_status EX_SCU_PWR_SetMode(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;
    uint8_t un8Arg = 1;
    SCUPWR_MODE_e eMode = SCUPWR_MODE_MAX;
    SCUCLK_MCLK_CFG_t tMClkCfg = 
    {
        .eMClk = SCUCLK_SRC_PLL,
        .ePreMClkDiv = SCUCLK_DIV_4
    };   
#if defined (EXTRN_HAS_DEEP_SLEEP_SUB_MODE)
    bool bNmiEnable = false;
#if defined (EX_SCU_PWR_NO_IRQ)
#else
    SCUPWR_WKUP_INTR_e eWkUpIntrMode = SCUPWR_WKUP_INTR_MASK;
#endif
    SCUPWR_DS_CFG_t tDsCfg = 
    {
        .eDsMode = SCUPWR_DS_MODE_0,
        .un8WkUpPinEnable = 0
    };
#endif

    if (pn8Argv[un8Arg][0] == 's')
        eMode = SCUPWR_MODE_SLEEP;
    else if (pn8Argv[un8Arg][0] == 'd')
        eMode = SCUPWR_MODE_DEEPSLEEP;
    else if (pn8Argv[un8Arg][0] == 't')
        eMode = SCUPWR_MODE_DEEPSLEEP2;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_PWR_ERR_STR, NULL, EX_COMM_STR_OPT_MODE);
        goto err;
    }

    un8Arg++;
    if(eMode == SCUPWR_MODE_DEEPSLEEP || eMode == SCUPWR_MODE_DEEPSLEEP2)
    { 
        eDbgStatus = EX_SCU_PWR_GetMClk(un8Arg, pn8Argv, &tMClkCfg);
        if(eDbgStatus != DEBUG_CMD_SUCCESS)
        {
            goto err;
        }
 
        eErr = HAL_SCU_CLK_SetMClk(&tMClkCfg);
        if(eErr != HAL_ERR_OK)
        {
            EX_COMMON_SetShowModuleLog(EX_SCU_PWR_ERR_STR, "Mode", EX_COMM_STR_OPT_MAX);
            goto err;
        }

        un8Arg += 2;
        eDbgStatus = EX_SCU_PWR_GetMClk(un8Arg, pn8Argv, &tMClkCfg);
        if(eDbgStatus != DEBUG_CMD_SUCCESS)
        {
            goto err;
        }
    }

#if defined (EXTRN_HAS_DEEP_SLEEP_SUB_MODE)
    un8Arg += 2;
    eDbgStatus = EX_SCU_PWR_GetDeepSleepMode(un8Arg, pn8Argv, &tDsCfg, &bNmiEnable);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

#if defined (EX_SCU_PWR_NO_IRQ)
#else
    if (tDsCfg.eDsMode == SCUPWR_DS_MODE_1)
    {
        if (bNmiEnable == true)
        {
            eWkUpIntrMode = SCUPWR_WKUP_INTR_NON_MASK;
        }

        eErr = HAL_SCU_PWR_SetIRQWakeUp(&EX_SCU_PWR_WkUpIRQHandler, NULL, 0, eWkUpIntrMode);
        if(eErr != HAL_ERR_OK)
        {
            EX_COMMON_SetShowModuleLog(EX_SCU_PWR_ERR_STR, "wkup1 IRQ", EX_COMM_STR_OPT_MAX);
            goto err;
        }
    }
#endif

    eErr = HAL_SCU_PWR_SetDeepSleepMode(&tDsCfg);
    if(eErr != HAL_ERR_OK)
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_PWR_ERR_STR, "dsm", EX_COMM_STR_OPT_MAX);
        goto err;
    }
#endif

    eErr = HAL_SCU_PWR_SetMode(eMode);
    if(eErr != HAL_ERR_OK)
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_PWR_ERR_STR, "Mode", EX_COMM_STR_OPT_MAX);
        goto err;
    }

    if(eMode == SCUPWR_MODE_DEEPSLEEP || eMode == SCUPWR_MODE_DEEPSLEEP2)
    { 
        eErr = HAL_SCU_CLK_SetMClk(&tMClkCfg);
        if(eErr != HAL_ERR_OK)
        {
            EX_COMMON_SetShowModuleLog(EX_SCU_PWR_ERR_STR, "Mode", EX_COMM_STR_OPT_MAX);
            goto err;
        }
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_SCU_PWR_SetAon(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;
    uint8_t un8Arg = 1;
    int8_t n8ArgCnt = 0;
    uint32_t un32Aon = 0, un32AonSelect = 0, un32AonEnable = 0;

    n8ArgCnt = n32Argc;

    while(--n8ArgCnt > 0)
    {
        switch (pn8Argv[un8Arg][0])
        {
            case 'v':
                un32Aon |= SCUPWR_AON_VDC;
                un32AonSelect = SCUPWR_AON_VDC;
                break;
            case 'b':
                un32Aon |= SCUPWR_AON_BGR;
                un32AonSelect = SCUPWR_AON_BGR;
                break;
            case 'l':
                un32Aon |= SCUPWR_AON_LSI;
                un32AonSelect = SCUPWR_AON_LSI;
                break;
            case 's':
                un32Aon |= SCUPWR_AON_LSE;
                un32AonSelect = SCUPWR_AON_LSE;
                break;
            case 'h':
                un32Aon |= SCUPWR_AON_HSI;
                un32AonSelect = SCUPWR_AON_HSI;
                break;
            case 'e':
                un32Aon |= SCUPWR_AON_HSE;
                un32AonSelect = SCUPWR_AON_HSE;
                break;
            case 'p':
                un32Aon |= SCUPWR_AON_PLL;
                un32AonSelect = SCUPWR_AON_PLL;
                break;
            default:
                eDbgStatus = DEBUG_CMD_INVALID;
                EX_COMMON_SetShowModuleLog(EX_SCU_PWR_ERR_STR, NULL, EX_COMM_STR_OPT_AON);
                break;
        }

        switch (pn8Argv[un8Arg++][2])
        {
            case 'e':
                un32AonEnable |= un32AonSelect;
                break;
            case 'd':
                un32AonEnable &= ~un32AonSelect;
                break;
            default:
                eDbgStatus = DEBUG_CMD_INVALID;
                EX_COMMON_SetShowModuleLog(EX_SCU_PWR_ERR_STR, NULL, EX_COMM_STR_OPT_ENA);
                break;
        }
        
        if(eDbgStatus != DEBUG_CMD_SUCCESS)
        {
            return DEBUG_CMD_INVALID;
        }
    }

    eErr = HAL_SCU_PWR_SetAlwaysOn(un32Aon, un32AonEnable);
    if(eErr != HAL_ERR_OK)
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_PWR_ERR_STR, "AlwaysOn", EX_COMM_STR_OPT_MAX);
        return DEBUG_CMD_INVALID;
    }

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_SCU_PWR_SetVdcDelay(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    uint8_t un8Delay = 0;

    un8Delay = (uint8_t)atoi(pn8Argv[1]);

    eErr = HAL_SCU_PWR_SetVdcDelay(un8Delay);
    if(eErr != HAL_ERR_OK)
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_PWR_ERR_STR, "VdcDelay", EX_COMM_STR_OPT_MAX);
        return DEBUG_CMD_INVALID;
    }

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_SCU_PWR_SetBackupMode(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;
    uint8_t un8Arg = 1;
    bool bEnable = false;

    eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], &bEnable);
    if (eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_PWR_ERR_STR, NULL, EX_COMM_STR_OPT_ENA);
        goto err;
    }
        
    eErr = HAL_SCU_PWR_SetBackupMode(bEnable);
    if(eErr != HAL_ERR_OK)
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_PWR_ERR_STR, "BackupMode", EX_COMM_STR_OPT_MAX);
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_SCU_PWR_SetBackupData(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    SCUPWR_BAK_DATA_t tBakData[EX_SCU_PWR_BACKUP_DATA_CNT];
    uint8_t un8Idx = 1, un8InCnt = 0;
    uint32_t un32Data = 0;

    un8InCnt = (n32Argc - 1) / 2;

    if(un8InCnt > EX_SCU_PWR_BACKUP_DATA_CNT)
    {
        goto err;
    }

    while(un8Idx <= un8InCnt)
    {
        tBakData[un8Idx - 1].un8Num = (uint8_t)atoi(pn8Argv[(un8Idx*2) - 1]);
        sscanf(pn8Argv[(un8Idx*2)], "%x", &un32Data);
        tBakData[un8Idx - 1].un32Data = un32Data;
        un8Idx++;
    }

    eErr = HAL_SCU_PWR_SetBackupData(tBakData, un8InCnt);
    if(eErr != HAL_ERR_OK)
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_PWR_ERR_STR, "BackupData", EX_COMM_STR_OPT_MAX);
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_SCU_PWR_GetBackupData(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    SCUPWR_BAK_DATA_t tBakData[EX_SCU_PWR_BACKUP_DATA_CNT];
    uint8_t un8Idx = 1, un8InCnt = 0;

    un8InCnt = (n32Argc - 1);

    if(un8InCnt > EX_SCU_PWR_BACKUP_DATA_CNT)
    {
        goto err;
    }

    while(un8Idx <= un8InCnt)
    {
        tBakData[un8Idx - 1].un8Num = (uint8_t)atoi(pn8Argv[un8Idx]);
        un8Idx++;
    }

    eErr = HAL_SCU_PWR_GetBackupData(tBakData, un8InCnt);
    if(eErr != HAL_ERR_OK)
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_PWR_ERR_STR, "BackupData", EX_COMM_STR_OPT_MAX);
        goto err;
    }

    for(int i = 0; i < un8InCnt; i++)
    {
        LOG("%s [%d] : [0x%8x]\n", EX_SCU_PWR_LOG_STR, tBakData[i].un8Num, tBakData[i].un32Data);
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static const struct debug_cmd s_tEX_SCU_PWR_CMD[] =
{
    {"SCUPWR", "h", EX_SCU_PWR_Help, "help"},
    {"SCUPWR", "pwr", EX_SCU_PWR_SetMode, ""},
    {"SCUPWR", "aon", EX_SCU_PWR_SetAon, ""},
    {"SCUPWR", "vdc", EX_SCU_PWR_SetVdcDelay, ""},
    {"SCUPWR", "bak", EX_SCU_PWR_SetBackupMode, ""},
    {"SCUPWR", "bakdw", EX_SCU_PWR_SetBackupData, ""},
    {"SCUPWR", "bakdr", EX_SCU_PWR_GetBackupData, ""}
};

void EX_SCU_PWR(void)
{
    /* Add TC commands */
    debug_cmd_init(s_tEX_SCU_PWR_CMD, DEBUG_CMD_LIST_COUNT(s_tEX_SCU_PWR_CMD));
#if defined (EXTRN_HAS_DEEP_SLEEP_SUB_MODE)
    SCUPWR_DS_WKUP_REASON_e eReason;
    HAL_SCU_PWR_GetDsWkUpReason(&eReason);
    if (eReason == SCUPWR_DS_WKUP_REASON_2)
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_PWR_LOG_STR, "wkup ds 2 reason", EX_COMM_STR_OPT_MAX);
    } 
    else if (eReason == SCUPWR_DS_WKUP_REASON_3)
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_PWR_LOG_STR, "wkup ds 3 reason", EX_COMM_STR_OPT_MAX);
    }
#endif
}

#endif /* SCU_PWR_TC */

/* --------------------------------- End Of File ------------------------------ */
