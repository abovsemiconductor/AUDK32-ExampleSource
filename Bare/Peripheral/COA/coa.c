/**
 *******************************************************************************
 * @file        coa.c
 * @author      ABOV R&D Division
 * @brief       Configuration Option Area (COA) Example Code
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

#if defined(COA_TC)
#include "abov_config.h"
#include "ex_configs.h"
#include "ex_common.h"

#if defined(_COA)

#if defined(_DFMC)
#define _FLASH_TYPE    2    /* CFMC & DFMC */
#elif defined(_CFMC)
#define _FLASH_TYPE    1    /* Only CFMC */
#else
#error "This chipset did not support this example."
#endif

#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "hal_scu.h"
#include "hal_scu_clk.h"
#include "hal_pcu.h"

#if (_FLASH_TYPE == 1)
#include "hal_cfmc.h"
#elif (_FLASH_TYPE == 2)
#include "hal_fmc.h"
#endif

#define EX_COA_STR "COA"
#define EX_COA_LOG_STR "COA :"
#define EX_COA_ERR_STR "[E]COA :"
#define EX_COA_MAX_NUM  0

/* WDT Defines */
#define EX_WDT_CONFIG_MAX_ARG              4
#define EX_WDT_RCOSC_WDTRCEN               0x96D
#define EX_WDT_RCOSC_WDTRC_NO_DEEPSLEEP    0x2A7
#define EX_WDT_RCOSC_WDTRC                 0xFFF

/* LVR Defines */
#define EX_LVR_CONFIG_MAX_ARG              2
#define EX_LVR_RST_OPS_LVREN               0xAA
#define EX_LVR_RST_OPS_MST                 0xFF
#define EX_LVR_VOLT_UNUSED_MIN             0x0C
#define EX_LVR_VOLT_UNUSED_MAX             0x0E

extern const char *pcCommStr[];
extern const char *pcCmdStr[];
extern const char *pcValStr[];
extern const char *pcOptStr[];

typedef struct {
    uint8_t    un8CntEn      : 1;
    uint8_t    un8RstEn      : 1;
    uint8_t    un8ClkEn      : 1;
    uint8_t    un8Reserved1  : 1;
    uint16_t   un16RCOM      : 12; 
    uint16_t   un16Reserved2 : 16;
} COA_WDT_CFG_t;

#if defined (EX_COA_LVR_WTIDKY)
typedef struct {
    uint8_t    un8Volt       : 4;
    uint8_t    un8Reserved1  : 4;
    uint8_t    un8Rst        : 8;
    uint16_t   un16WKey      : 16;
} COA_LVR_CFG_t;
#else
typedef struct {
    uint8_t    un8Volt       : 4;
    uint8_t    un8Reserved1  : 4;
    uint8_t    un8Rst        : 8;
    uint16_t   un16Reserved2 : 16;
} COA_LVR_CFG_t;
#endif

static COA_WDT_CFG_t s_tWdt;
static COA_LVR_CFG_t s_tLvr;

static enum debug_cmd_status EX_COA_Help(int32_t n32Argc, char *pn8Argv[])
{
    uint32_t un32ShowOpt = 0;
    EX_COMM_STR_OPT_e eOpt[2];

    EX_COMMON_SetShowModuleInfo(EX_COA_STR, 1);

    un32ShowOpt = EX_COMMON_GetShowOpt(pn8Argv[1]);

    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "wdtinfo", EX_COA_MAX_NUM, eOpt, 0, "display wdt config");
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "lvrinfo", EX_COA_MAX_NUM, eOpt, 0, "display lvr config");
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "wdtcfg\t", EX_COA_MAX_NUM, eOpt, 0, "[-cnt] [-rst] [-clk] [-rcosc]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "\t -cnt: m(master)/e(cnten)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "\t -rst: m(master)/e(rsten)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "\t -clk: w(wdtrc)/c(wdtclk)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "\t -rcosc: e(wdtrcen)/n(wdtrc no deepsleep)/w(wdtrc)");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "lvrcfg\t", EX_COA_MAX_NUM, eOpt, 0, "[-rst] [-volt]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "\t -rst: m(master)/l(lvren)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "\t -volt: 0~N(4bit Nibble)");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "writecfg", EX_COA_MAX_NUM, eOpt, 0, "write wdt & lvr config");

    return DEBUG_CMD_SUCCESS;
}

static void PRV_EX_COA_PrintWDT(COA_WDT_CFG_t *ptWdt)
{
    LOG("\n");
    LOG("[ WDT Config ]\n");
    LOG("- Counter\t: %s [0x%x]\n",(ptWdt->un8CntEn == false ? "Master" : "CNTEN"), ptWdt->un8CntEn);
    LOG("- Reset\t\t: %s [0x%x]\n",(ptWdt->un8RstEn == false ? "Master" : "RSTEN"), ptWdt->un8RstEn);
    LOG("- Clock\t\t: %s [0x%x]\n",(ptWdt->un8ClkEn == true ? "WDTRC" : "WDTCLK"), ptWdt->un8ClkEn);
    LOG("- RC OSC\t: %s [0x%x]\n",(ptWdt->un16RCOM == EX_WDT_RCOSC_WDTRCEN ? "WDTRCEN" 
                                   : (ptWdt->un16RCOM == EX_WDT_RCOSC_WDTRC_NO_DEEPSLEEP ? "Master no deepsleep" 
                                   : "Master")), ptWdt->un16RCOM);
    LOG("\n");
}

static void PRV_EX_COA_PrintLVR(COA_LVR_CFG_t *ptLvr)
{
    LOG("\n");
    LOG("[ LVR Config ]\n");
#if defined (EX_COA_LVR_WTIDKY)
    LOG("- WKey\t\t: 0x%x\n", ptLvr->un16WKey);
#endif
    LOG("- Reset\t\t: %s [0x%x]\n",(ptLvr->un8Rst == EX_LVR_RST_OPS_LVREN ? "LVREN" : "Master"), ptLvr->un8Rst);
    LOG("- Voltage\t: 0x%x\n", ptLvr->un8Volt);
    LOG("\n");
}

#if (_FLASH_TYPE == 1)
static enum debug_cmd_status PRV_EX_COA_SetCFMC(void)
{
    HAL_ERR_e eRet = HAL_ERR_OK;
    CFMC_RD_PROTECT_CONFIG_t tRdProtectConfig;

    eRet = HAL_CFMC_Erase(CONFIG_COA_PAGE1_ADDR, CFMC_PAGE_ERASE_MODE);
    if(eRet != HAL_ERR_OK)
    {
        goto err;
    }

    tRdProtectConfig.eRdProtectLevel = CFMC_RD_PROTECT_LEVEL0;

    eRet = HAL_CFMC_SetReadProtect(tRdProtectConfig);
    if(eRet != HAL_ERR_OK)
    {
        goto err;
    }

    /* Write WDT Configuration */
    eRet = HAL_CFMC_Write(CONFIG_COA_WDT_ADDR, (uint8_t *)&s_tWdt, sizeof(COA_WDT_CFG_t));
    if(eRet != HAL_ERR_OK)
    {
        goto err;
    }

    /* Write LVR Configuration */
    eRet = HAL_CFMC_Write(CONFIG_COA_LVR_ADDR, (uint8_t *)&s_tLvr, sizeof(COA_LVR_CFG_t));
    if(eRet != HAL_ERR_OK)
    {
        goto err;
    }

    LOG("%s In order for changes to take effect, must do Reboot\n", EX_COA_LOG_STR);

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}
#endif

#if (_FLASH_TYPE == 2)
static enum debug_cmd_status PRV_EX_COA_SetFMC(void)
{
    HAL_ERR_e eRet = HAL_ERR_OK;
    FMC_RD_PROTECT_CONFIG_t tRdProtectConfig;

    
    eRet = HAL_FMC_Init();
    if(eRet != HAL_ERR_OK)
    {
        goto err;
    }

    eRet = HAL_FMC_Erase(FMC_ID_CFMC, CONFIG_COA_PAGE1_ADDR, FMC_PAGE_ERASE_MODE);
    if(eRet != HAL_ERR_OK)
    {
        goto err;
    }

    tRdProtectConfig.eRdProtectLevel = FMC_RD_PROTECT_LEVEL0;

    eRet = HAL_FMC_SetReadProtect(FMC_ID_CFMC, tRdProtectConfig);
    if(eRet != HAL_ERR_OK)
    {
        goto err;
    }

    /* Write WDT Configuration */
    eRet = HAL_FMC_Write(FMC_ID_CFMC, CONFIG_COA_WDT_ADDR, (uint8_t *)&s_tWdt, sizeof(COA_WDT_CFG_t));
    if(eRet != HAL_ERR_OK)
    {
        goto err;
    }

    /* Write LVR Configuration */
    eRet = HAL_FMC_Write(FMC_ID_CFMC, CONFIG_COA_LVR_ADDR, (uint8_t *)&s_tLvr, sizeof(COA_LVR_CFG_t));
    if(eRet != HAL_ERR_OK)
    {
        goto err;
    }

    LOG("%s In order for changes to take effect, must do Reboot\n", EX_COA_LOG_STR);

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}
#endif

static enum debug_cmd_status EX_COA_GetWDTInfo(int32_t n32Argc, char *pn8Argv[])
{
    COA_WDT_CFG_t *ptWdt;

    ptWdt = (COA_WDT_CFG_t *)CONFIG_COA_WDT_ADDR;
    PRV_EX_COA_PrintWDT(ptWdt);
     
    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_COA_GetLVRInfo(int32_t n32Argc, char *pn8Argv[])
{
    COA_LVR_CFG_t *ptLvr;

    ptLvr = (COA_LVR_CFG_t *)CONFIG_COA_LVR_ADDR;
    PRV_EX_COA_PrintLVR(ptLvr); 

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_COA_SetWDT(int32_t n32Argc, char *pn8Argv[])
{
    uint8_t un8Arg = 1;
    uint8_t un8Cnt = (n32Argc - 1) / 2;

    if(un8Cnt > EX_WDT_CONFIG_MAX_ARG)
    {
        EX_COMMON_SetShowModuleLog(EX_COA_ERR_STR, (char *)pcOptStr[EX_COMM_STR_OPT_ARG], EX_COMM_STR_OPT_MAX);
        return DEBUG_CMD_INVALID;
    }

    for(uint8_t i = 0; i < un8Cnt; i++)
    { 
        if(strncmp(pn8Argv[un8Arg], "-cnt", 4) == 0)
        {
            un8Arg++;
            switch (pn8Argv[un8Arg++][0])
            {
                case 'm':
                    s_tWdt.un8CntEn = false;
                    break;
                case 'e':
                    s_tWdt.un8CntEn = true;
                    break;
                default:
                    EX_COMMON_SetShowModuleLog(EX_COA_ERR_STR, "[-cnt]", EX_COMM_STR_OPT_MAX);
                    goto err;
            }
        } 

        if(strncmp(pn8Argv[un8Arg], "-rst", 4) == 0)
        {
            un8Arg++;
            switch (pn8Argv[un8Arg++][0])
            {
                case 'm':
                    s_tWdt.un8RstEn = false;
                    break;
                case 'e':
                    s_tWdt.un8RstEn = true;
                    break;
                default:
                    EX_COMMON_SetShowModuleLog(EX_COA_ERR_STR, "[-rst]", EX_COMM_STR_OPT_MAX);
                    goto err;
            }
        } 

        if(strncmp(pn8Argv[un8Arg], "-clk", 4) == 0)
        {
            un8Arg++;
            switch (pn8Argv[un8Arg++][0])
            {
                case 'c':
                    s_tWdt.un8ClkEn = false;
                    break;
                case 'w':
                    s_tWdt.un8ClkEn = true;
                    break;
                default:
                    EX_COMMON_SetShowModuleLog(EX_COA_ERR_STR, "[-clk]", EX_COMM_STR_OPT_MAX);
                    goto err;
            }
        } 

        if(strncmp(pn8Argv[un8Arg], "-rcosc", 6) == 0)
        {
            un8Arg++;
            switch (pn8Argv[un8Arg++][0])
            {
                case 'e':
                    s_tWdt.un16RCOM = EX_WDT_RCOSC_WDTRCEN;
                    break;
                case 'n':
                    s_tWdt.un16RCOM = EX_WDT_RCOSC_WDTRC_NO_DEEPSLEEP;
                    break;
                case 'w':
                    s_tWdt.un16RCOM = EX_WDT_RCOSC_WDTRC;
                    break;
                default:
                    EX_COMMON_SetShowModuleLog(EX_COA_ERR_STR, "[-rcosc]", EX_COMM_STR_OPT_MAX);
                    goto err;
            }
        } 
    }

    PRV_EX_COA_PrintWDT(&s_tWdt);
    
    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_COA_SetLVR(int32_t n32Argc, char *pn8Argv[])
{
    uint8_t un8Arg = 1;
    uint8_t un8Cnt = (n32Argc - 1) / 2;

    if(un8Cnt > EX_LVR_CONFIG_MAX_ARG)
    {
        EX_COMMON_SetShowModuleLog(EX_COA_ERR_STR, (char *)pcOptStr[EX_COMM_STR_OPT_ARG], EX_COMM_STR_OPT_MAX);
        goto err;
    }

    for(uint8_t i = 0; i < un8Cnt; i++)
    { 
        if(strncmp(pn8Argv[un8Arg], "-rst", 4) == 0)
        {
            un8Arg++;
            switch (pn8Argv[un8Arg++][0])
            {
                case 'm':
                    s_tLvr.un8Rst = EX_LVR_RST_OPS_MST;
                    break;
                case 'l':
                    s_tLvr.un8Rst = EX_LVR_RST_OPS_LVREN;
                    break;
                default:
                    EX_COMMON_SetShowModuleLog(EX_COA_ERR_STR, "[-rst]", EX_COMM_STR_OPT_MAX);
                    goto err;
            }
        } 

        if(strncmp(pn8Argv[un8Arg], "-volt", 5) == 0)
        {
            un8Arg++;
            s_tLvr.un8Volt = ((uint8_t)atoi(pn8Argv[un8Arg++]) & 0x0F);
#if defined (EX_COA_LVR_WTIDKY)
            s_tLvr.un16WKey = EX_COA_LVR_WTIDKY_VALUE;
#endif
            if((s_tLvr.un8Volt >= EX_LVR_VOLT_UNUSED_MIN) && (s_tLvr.un8Volt <= EX_LVR_VOLT_UNUSED_MAX))
            {
                EX_COMMON_SetShowModuleLog(EX_COA_ERR_STR, "[-volt]", EX_COMM_STR_OPT_MAX);
                goto err;
            }
        } 
    }

    PRV_EX_COA_PrintLVR(&s_tLvr); 

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_COA_SetWriteCfg(int32_t n32Argc, char *pn8Argv[])
{
#if (_FLASH_TYPE == 1)
    return PRV_EX_COA_SetCFMC();
#elif (_FLASH_TYPE == 2)
    return PRV_EX_COA_SetFMC();
#else
    return DEBUG_CMD_INVALID;
#endif

}

static const struct debug_cmd s_tEX_COA_CMD[] =
{
    {"COA", "h", EX_COA_Help, "help"},
    {"COA", "wdtinfo", EX_COA_GetWDTInfo, ""},
    {"COA", "lvrinfo", EX_COA_GetLVRInfo, ""},
    {"COA", "wdtcfg", EX_COA_SetWDT, ""},
    {"COA", "lvrcfg", EX_COA_SetLVR, ""},
    {"COA", "writecfg", EX_COA_SetWriteCfg, ""},
};

void EX_COA(void)
{
    /* Initialization */
#if (_FLASH_TYPE == 1)
    HAL_CFMC_Init();
#elif (_FLASH_TYPE == 2)
    /* Not Available */
#endif

    /* Update WDT / LVR Config from Flash */
    memcpy(&s_tWdt, (COA_WDT_CFG_t *)CONFIG_COA_WDT_ADDR, sizeof(COA_WDT_CFG_t));
    memcpy(&s_tLvr, (COA_LVR_CFG_t *)CONFIG_COA_LVR_ADDR, sizeof(COA_LVR_CFG_t));

    /* Add TC commands */
    debug_cmd_init(s_tEX_COA_CMD, DEBUG_CMD_LIST_COUNT(s_tEX_COA_CMD));
}

#endif
#endif /* COA_TC */
/* --------------------------------- End Of File ------------------------------ */
