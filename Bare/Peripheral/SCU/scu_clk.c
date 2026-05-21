/**
 *******************************************************************************
 * @file        scu_clk.c
 * @author      ABOV R&D Division
 * @brief       SCU Clock Example Code
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
#include "hal_scu_clk.h"
#include "hal_pcu.h"

#define EX_SCU_CLK_STR "SCUCLK"
#define EX_SCU_CLK_LOG_STR "SCUCLK :"
#define EX_SCU_CLK_ERR_STR "[E]SCUCLK :"
#define EX_SCU_CLK_IRQ_PRIO 3

extern const char *pcCommStr[];
extern const char *pcCmdStr[];
extern const char *pcValStr[];
extern const char *pcOptStr[];

static enum debug_cmd_status EX_SCU_CLK_Help(int32_t n32Argc, char *pn8Argv[])
{
    uint32_t un32ShowOpt = 0;
    EX_COMM_STR_OPT_e eOpt[4];

    un32ShowOpt = EX_COMMON_GetShowOpt(pn8Argv[1]);

    eOpt[0] = EX_COMM_STR_OPT_SRC;
    eOpt[1] = EX_COMM_STR_OPT_DIV;
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "mclk", -1, eOpt, 2, "[-bdr]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_SRC, EX_COMM_STR_VAL_CLKSRC, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_DIV, EX_COMM_STR_VAL_MAX, "0,2,4,8,16,32,64,128,256,512");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-bdr: [baud] (Debug)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "baud: 1~N");
    }
    eOpt[0] = EX_COMM_STR_OPT_DIV;
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "pclk", -1, eOpt, 1, "[-bdr]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_DIV, EX_COMM_STR_VAL_MAX, "0,2,4,8,16");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-bdr: [baud] (Debug)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "baud: 1~N");
    }
    eOpt[0] = EX_COMM_STR_OPT_SRC;
    eOpt[1] = EX_COMM_STR_OPT_ENA;
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "clken", -1, eOpt, 2, NULL);
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_SRC, EX_COMM_STR_VAL_CLKSRC, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_ENA, EX_COMM_STR_VAL_ENDIS, NULL);
    }
    eOpt[0] = EX_COMM_STR_OPT_ENA;
    eOpt[1] = EX_COMM_STR_OPT_SRC;
    eOpt[2] = EX_COMM_STR_OPT_DIV;
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "pll", -1, eOpt, 3, "[prediv] [pdiv1] [pdiv2] [outdiv] [vco] [cur] [bias] [-bdr]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_ENA, EX_COMM_STR_VAL_ENDIS, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_SRC, EX_COMM_STR_VAL_MAX, "h(hsi)/e(hse)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_DIV, EX_COMM_STR_VAL_MAX, "0,2,4,8");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "prediv: 0~7");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "pdiv1: 0~255");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "pdiv2: 0~16");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "outdiv: 0~16");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "vco: s(vco)/d(vco*2)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "cur: 5,10,15,20");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "bias: 0(div_4)/1(div_2)/2(none)/3(multi_2)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-bdr: [baud] (Debug)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "baud: 1~N");
    }
    eOpt[0] = EX_COMM_STR_OPT_SRC;
    eOpt[1] = EX_COMM_STR_OPT_DIV;
    eOpt[2] = EX_COMM_STR_OPT_ENA;
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "clkout", -1, eOpt, 3, NULL);
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MCCR, EX_COMM_STR_VAL_CLKSRC, "/m(mclk)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_DIV, EX_COMM_STR_VAL_N_NUM, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_ENA, EX_COMM_STR_VAL_ENDIS, NULL);
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "exthse", -1, eOpt, 0, "[freq] [nc]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "freq: 0(1~4MHz)/1(4~8MHz)/2(8~12MHz)/3(12~16MHz)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_NC, EX_COMM_STR_VAL_ENDIS, NULL);
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "extlse", -1, eOpt, 0, "[curr] [nc]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "curr: 0(1.57uA)/1(1.79uA)/2(1.93uA)/3(2.04uA)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_NC, EX_COMM_STR_VAL_ENDIS, NULL);
    }
    eOpt[0] = EX_COMM_STR_OPT_LDO;
    eOpt[1] = EX_COMM_STR_OPT_SHIFT;
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "extlsi", -1, eOpt, 2, NULL);
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_LDO, EX_COMM_STR_VAL_ENDIS, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_SHIFT, EX_COMM_STR_VAL_ENDIS, NULL);
    }
    eOpt[0] = EX_COMM_STR_OPT_SRC;
    eOpt[1] = EX_COMM_STR_OPT_ENA;
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "mon", -1, eOpt, 2, "[prio] [-mclkrec]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_SRC, EX_COMM_STR_VAL_MAX, "m(mclk)/s(lse)/e(hse)");
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "m: [rec]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "rec: e(en)/d(dis) [auto-recovery]");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_ENA, EX_COMM_STR_VAL_ENDIS, NULL);
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "e: [msk]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "msk: m(mask)/n(non-mask)"); 
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "prio: 0~N");
    }
    LOG("\n");

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_SCU_CLK_GetClk(uint8_t un8Argv, char *pn8Argv[], SCUCLK_SRC_e *peSrc)
{
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;

    switch(pn8Argv[un8Argv][0])
    {
        case 'l':
            *peSrc = SCUCLK_SRC_LSI;
            break;
        case 'w':
            *peSrc = SCUCLK_SRC_WDT;
            break;
        case 's':
            *peSrc = SCUCLK_SRC_LSE;
            break;
        case 'h':
            *peSrc = SCUCLK_SRC_HSI;
            break;
        case 'e':
            *peSrc = SCUCLK_SRC_HSE;
            break;
        case 'p':
            *peSrc = SCUCLK_SRC_PLL;
            break;
        case 'm':
            *peSrc = SCUCLK_SRC_MCLK;
            break;
        default:
            eDbgStatus = DEBUG_CMD_INVALID;
            break;
    }

    return eDbgStatus;
}
static enum debug_cmd_status EX_SCU_CLK_GetDiv(uint8_t un8Argv, char *pn8Argv[], SCUCLK_DIV_e *peDiv)
{
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;

    switch((uint16_t)atoi(pn8Argv[un8Argv]))
    {
        case 0:
            *peDiv = SCUCLK_DIV_NONE;
            break;
        case 2:
            *peDiv = SCUCLK_DIV_2;
            break;
        case 4:
            *peDiv = SCUCLK_DIV_4;
            break;
        case 8:
            *peDiv = SCUCLK_DIV_8;
            break;
        case 16:
            *peDiv = SCUCLK_DIV_16;
            break;
        case 32:
            *peDiv = SCUCLK_DIV_32;
            break;
        case 64:
            *peDiv = SCUCLK_DIV_64;
            break;
        case 128:
            *peDiv = SCUCLK_DIV_128;
            break;
        case 256:
            *peDiv = SCUCLK_DIV_256;
            break;
        case 512:
            *peDiv = SCUCLK_DIV_512;
            break;
        default:
            eDbgStatus = DEBUG_CMD_INVALID;
            break;
    }

    return eDbgStatus;
}

void EX_SCU_CLK_GetBaud(uint8_t un8Argv, char *pn8Argv[], uint16_t *pun16BaudRate)
{
    if((strncmp(pn8Argv[un8Argv], "-bdr", 4) == 0))
    {
        un8Argv++;
        *pun16BaudRate = (uint16_t)atoi(pn8Argv[un8Argv]);
    }
}

static enum debug_cmd_status EX_SCU_CLK_GetMClk(int32_t n32Argc, char *pn8Argv[], SCUCLK_MCLK_CFG_t *ptCfg, uint16_t *pun16BaudRate)
{
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;
    uint8_t un8Arg = 1;

    eDbgStatus = EX_SCU_CLK_GetClk(un8Arg++, pn8Argv, &ptCfg->eMClk);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_CLK_ERR_STR, NULL, EX_COMM_STR_OPT_SRC);
        goto err;
    }

    eDbgStatus = EX_SCU_CLK_GetDiv(un8Arg++, pn8Argv, &ptCfg->ePreMClkDiv);
    if (eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_CLK_ERR_STR, NULL, EX_COMM_STR_OPT_DIV);
        goto err;
    }

    *pun16BaudRate = 0;
    EX_SCU_CLK_GetBaud(un8Arg++, pn8Argv, pun16BaudRate);

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_SCU_CLK_GetPLLConfig(int32_t n32Argc, char *pn8Argv[], bool *pbEnable, SCUCLK_PLL_CFG_t *ptPllCfg, uint16_t *pun16BaudRate)
{
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;
    uint8_t un8Arg = 1;
    uint16_t un16CtrlOpt = 0;

    eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], pbEnable);
    if (eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_CLK_ERR_STR, NULL, EX_COMM_STR_OPT_ENA);
        goto err;
    }

    switch (pn8Argv[un8Arg++][0])
    {
        case 'h':
            ptPllCfg->eSrc = SCUCLK_PLL_SRC_HSI;
            break;
        case 'e':
            ptPllCfg->eSrc = SCUCLK_PLL_SRC_HSE;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_SCU_CLK_ERR_STR, NULL, EX_COMM_STR_OPT_SRC);
            goto err;
    }

    eDbgStatus = EX_SCU_CLK_GetDiv(un8Arg++, pn8Argv, &ptPllCfg->eSrcDiv);
    if (eDbgStatus != DEBUG_CMD_SUCCESS || ptPllCfg->eSrcDiv > SCUCLK_DIV_32)
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_CLK_ERR_STR, NULL, EX_COMM_STR_OPT_DIV);
        goto err;
    }

    ptPllCfg->un8PreDiv = (uint8_t)atoi(pn8Argv[un8Arg++]);
    if(ptPllCfg->un8PreDiv > 16)
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_CLK_ERR_STR, "pre div max 15", EX_COMM_STR_OPT_MAX);
        goto err;
    }

    ptPllCfg->un8PostDiv1 = (uint8_t)atoi(pn8Argv[un8Arg++]);

    ptPllCfg->un8PostDiv2 = (uint8_t)atoi(pn8Argv[un8Arg++]);
    if(ptPllCfg->un8PostDiv2 > 16)
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_CLK_ERR_STR, "pdiv2 max 15", EX_COMM_STR_OPT_MAX);
        goto err;
    }

    ptPllCfg->un8OutDiv = (uint8_t)atoi(pn8Argv[un8Arg++]);
    if(ptPllCfg->un8OutDiv > 256)
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_CLK_ERR_STR, "outdiv max 255", EX_COMM_STR_OPT_MAX);
        goto err;
    }

    switch (pn8Argv[un8Arg++][0])
    {
        case 's':
            ptPllCfg->ePllMode = SCUCLK_PLL_MODE_VCO;
            break;
        case 'd':
            ptPllCfg->ePllMode = SCUCLK_PLL_MODE_VCO2X;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_SCU_CLK_ERR_STR, "[vco]", EX_COMM_STR_OPT_MAX);
            goto err;
    }

    un16CtrlOpt = (uint16_t)atoi(pn8Argv[un8Arg++]);

    if (un16CtrlOpt == 5) 
        ptPllCfg->eCurOpt = SCUCLK_PLL_CTRLOPT_5UA;
    else if (un16CtrlOpt == 10) 
        ptPllCfg->eCurOpt = SCUCLK_PLL_CTRLOPT_10UA;
    else if (un16CtrlOpt == 15) 
        ptPllCfg->eCurOpt = SCUCLK_PLL_CTRLOPT_15UA;
    else if (un16CtrlOpt == 20) 
        ptPllCfg->eCurOpt = SCUCLK_PLL_CTRLOPT_20UA;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_CLK_ERR_STR, "[cur]", EX_COMM_STR_OPT_MAX);
        goto err;
    }

    if (pn8Argv[un8Arg][0] == '0')
        ptPllCfg->eVcoBias = SCUCLK_PLL_VCOBIAS_DIV_4;
    else if (pn8Argv[un8Arg][0] == '1')
        ptPllCfg->eVcoBias = SCUCLK_PLL_VCOBIAS_DIV_2;
    else if (pn8Argv[un8Arg][0] == '2')
        ptPllCfg->eVcoBias = SCUCLK_PLL_VCOBIAS_NONE;
    else if (pn8Argv[un8Arg][0] == '3')
        ptPllCfg->eVcoBias = SCUCLK_PLL_VCOBIAS_MULTI_2;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_CLK_ERR_STR, "[bias]", EX_COMM_STR_OPT_MAX);
        goto err;
    }

    *pun16BaudRate = 0;
    un8Arg++;
    EX_SCU_CLK_GetBaud(un8Arg++, pn8Argv, pun16BaudRate);

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static void EX_SCU_CLK_IRQHandler(uint32_t un32Event, void *pContext)
{
    if(un32Event & SCUCLK_MON_EVENT_MCLKFAIL)
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_CLK_LOG_STR, "MCLK evt fire", EX_COMM_STR_OPT_MAX);
    }

    if(un32Event & SCUCLK_MON_EVENT_HSEFAIL)
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_CLK_LOG_STR, "HSE evt fire", EX_COMM_STR_OPT_MAX);
    }

    if(un32Event & SCUCLK_MON_EVENT_LSEFAIL)
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_CLK_LOG_STR, "LSE evt fire", EX_COMM_STR_OPT_MAX);
    }

}

static enum debug_cmd_status EX_SCU_CLK_SetPLL(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus;
    bool bPllEnable = false;
    uint16_t un16BaudRate = 0;

    SCUCLK_MCLK_CFG_t tMClkCfg = 
    {
        .eMClk = SCUCLK_SRC_PLL,
        .ePreMClkDiv = SCUCLK_DIV_NONE
    };   

    SCUCLK_PLL_CFG_t tPllCfg = 
    {
        .eSrc = SCUCLK_PLL_SRC_HSI,
        .eSrcDiv = SCUCLK_DIV_4,
        .un8PreDiv = 3,
        .un8PostDiv1 = 15,
        .un8PostDiv2 = 0, 
        .un8OutDiv = 0,
        .eCurOpt = SCUCLK_PLL_CTRLOPT_10UA,
        .eVcoBias = SCUCLK_PLL_VCOBIAS_NONE,
        .ePllMode = SCUCLK_PLL_MODE_VCO
    };

    eDbgStatus = EX_SCU_CLK_GetPLLConfig(n32Argc, pn8Argv, &bPllEnable, &tPllCfg, &un16BaudRate);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    if(bPllEnable == false)
    {
        tMClkCfg.eMClk = SCUCLK_SRC_LSI;
        tMClkCfg.ePreMClkDiv = SCUCLK_DIV_NONE;
        eErr = HAL_SCU_CLK_SetMClk(&tMClkCfg);
        if(eErr != HAL_ERR_OK)
        {
            EX_COMMON_SetShowModuleLog(EX_SCU_CLK_ERR_STR, "MClkCfg", EX_COMM_STR_OPT_MAX);
            goto err;
        }
    }

    tMClkCfg.eMClk = SCUCLK_SRC_LSI;
    tMClkCfg.ePreMClkDiv = SCUCLK_DIV_NONE;
    eErr = HAL_SCU_CLK_SetMClk(&tMClkCfg);
    if(eErr != HAL_ERR_OK)
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_CLK_ERR_STR, "MClkCfg", EX_COMM_STR_OPT_MAX);
        goto err;
    }

    eErr = HAL_SCU_CLK_SetPLLConfig(bPllEnable, &tPllCfg);
    if(eErr != HAL_ERR_OK)
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_CLK_ERR_STR, "PLLCfg", EX_COMM_STR_OPT_MAX);
        goto err;
    }

    if(bPllEnable == false)
    {
        tMClkCfg.eMClk = SCUCLK_SRC_HSI;
        tMClkCfg.ePreMClkDiv = SCUCLK_DIV_NONE;
    }
    else
    {
        tMClkCfg.eMClk = SCUCLK_SRC_PLL;
        tMClkCfg.ePreMClkDiv = SCUCLK_DIV_4;
    }


    eErr = HAL_SCU_CLK_SetMClk(&tMClkCfg);
    if(eErr != HAL_ERR_OK)
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_CLK_ERR_STR, "MClkCfg", EX_COMM_STR_OPT_MAX);
        goto err;
    }

#if (CONFIG_DEBUG == 1)
    Debug_Reinit(un16BaudRate);
    debug_cmd_reinit();
#endif

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_SCU_CLK_SetMClk(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus;
    uint16_t un16BaudRate = 0;
    SCUCLK_MCLK_CFG_t tMClkCfg = 
    {
        .eMClk = SCUCLK_SRC_HSI,
        .ePreMClkDiv = SCUCLK_DIV_NONE
    };   
    SCUCLK_MCLK_CFG_t tPreMClkCfg;

    eDbgStatus = EX_SCU_CLK_GetMClk(n32Argc, pn8Argv, &tMClkCfg, &un16BaudRate);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

#if defined(EX_SCU_UNSUPPORT_MCLK_SRC_LSI)
#else
    if(tMClkCfg.eMClk != SCUCLK_SRC_LSI)
    {
        tPreMClkCfg.eMClk = SCUCLK_SRC_LSI;
        tPreMClkCfg.ePreMClkDiv = SCUCLK_DIV_NONE;
        tPreMClkCfg.ePostMClkDiv = SCUCLK_DIV_NONE;
        eErr = HAL_SCU_CLK_SetMClk(&tPreMClkCfg);
        if(eErr != HAL_ERR_OK)
        {
            EX_COMMON_SetShowModuleLog(EX_SCU_CLK_ERR_STR, "MClkCfg", EX_COMM_STR_OPT_MAX);
            goto err;
        }
    }
#endif

    eErr = HAL_SCU_CLK_SetMClk(&tMClkCfg);
    if(eErr != HAL_ERR_OK)
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_CLK_ERR_STR, "MClkCfg", EX_COMM_STR_OPT_MAX);
        goto err;
    }

#if (CONFIG_DEBUG == 1)
    Debug_Reinit(un16BaudRate);
    debug_cmd_reinit();
#endif

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;

}

static enum debug_cmd_status EX_SCU_CLK_SetPClkDiv(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;
    uint16_t un16BaudRate = 0;
    uint8_t un8Arg = 1;
    SCUCLK_DIV_e eDiv = SCUCLK_DIV_NONE;

    eDbgStatus = EX_SCU_CLK_GetDiv(un8Arg++, pn8Argv, &eDiv);
    if (eDbgStatus != DEBUG_CMD_SUCCESS || eDiv > SCUCLK_DIV_16)
    {
	EX_COMMON_SetShowModuleLog(EX_SCU_CLK_ERR_STR, NULL, EX_COMM_STR_OPT_DIV);
        goto err;
    }
    
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    EX_SCU_CLK_GetBaud(un8Arg++, pn8Argv, &un16BaudRate);

    eErr = HAL_SCU_CLK_SetPClkDiv(eDiv);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }
    
#if (CONFIG_DEBUG == 1)
    Debug_Reinit(un16BaudRate);
    debug_cmd_reinit();
#endif

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;

}

static enum debug_cmd_status EX_SCU_CLK_SetSrcEnable(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    uint8_t un8Arg = 1;
    SCUCLK_SRC_e eSrc = SCUCLK_SRC_MAX;
    bool bEnable = false;

    eDbgStatus = EX_SCU_CLK_GetClk(un8Arg++, pn8Argv, &eSrc);
    if(eDbgStatus != DEBUG_CMD_SUCCESS || eSrc > SCUCLK_SRC_LSE)
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_CLK_ERR_STR, NULL, EX_COMM_STR_OPT_SRC);
        goto err;
    }

    eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], &bEnable);
    if (eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_CLK_ERR_STR, NULL, EX_COMM_STR_OPT_ENA);
        goto err;
    }

    eErr = HAL_SCU_CLK_SetSrcEnable(eSrc, bEnable);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_SCU_CLK_SetOutPort(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;
    uint8_t un8Arg = 1, un8Div = 0;
    SCUCLK_SRC_e eSrc = SCUCLK_SRC_MAX;
    bool bEnable = false;

    eDbgStatus = EX_SCU_CLK_GetClk(un8Arg++, pn8Argv, &eSrc);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_CLK_ERR_STR, NULL, EX_COMM_STR_OPT_SRC);
        return eDbgStatus;
    }

    un8Div = (uint8_t)atoi(pn8Argv[un8Arg++]);

    eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], &bEnable);
    if (eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_CLK_ERR_STR, NULL, EX_COMM_STR_OPT_ENA);
        return eDbgStatus;
    }

    eErr = HAL_SCU_CLK_SetOutput(eSrc, un8Div, bEnable);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    return eDbgStatus;
}

static enum debug_cmd_status EX_SCU_CLK_SetExtHSE(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;
    uint8_t un8Arg = 1;
    SCUCLK_HSE_FREQ_e eFreq = SCUCLK_HSE_FREQ_MAX;
    bool bEnable = false;

    if (pn8Argv[un8Arg][0] == '0')
        eFreq = SCUCLK_HSE_FREQ_4M;
    else if (pn8Argv[un8Arg][0] == '1')
        eFreq = SCUCLK_HSE_FREQ_8M;
    else if (pn8Argv[un8Arg][0] == '2')
        eFreq = SCUCLK_HSE_FREQ_12M;
    else if (pn8Argv[un8Arg][0] == '3')
        eFreq = SCUCLK_HSE_FREQ_16M;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_CLK_ERR_STR, "[freq]", EX_COMM_STR_OPT_MAX);
        goto err;
    }

    eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[++un8Arg][0], &bEnable);
    if (eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_CLK_ERR_STR, NULL, EX_COMM_STR_OPT_NC);
        goto err;
    }

    eErr = HAL_SCU_CLK_SetExtHSE(eFreq, bEnable);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_SCU_CLK_SetExtLSE(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;
    uint8_t un8Arg = 1;
    SCUCLK_LSE_CURR_e eCurr = SCUCLK_LSE_CURR_MAX;
    bool bEnable = false;

    if (pn8Argv[un8Arg][0] == '0')
        eCurr = SCUCLK_LSE_CURR_1;
    else if (pn8Argv[un8Arg][0] == '1')
        eCurr = SCUCLK_LSE_CURR_2;
    else if (pn8Argv[un8Arg][0] == '2')
        eCurr = SCUCLK_LSE_CURR_3;
    else if (pn8Argv[un8Arg][0] == '3')
        eCurr = SCUCLK_LSE_CURR_4;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_CLK_ERR_STR, "[curr]", EX_COMM_STR_OPT_MAX);
        goto err;
    }

    eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[++un8Arg][0], &bEnable);
    if (eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_CLK_ERR_STR, NULL, EX_COMM_STR_OPT_NC);
        goto err;
    }

    eErr = HAL_SCU_CLK_SetExtLSE(eCurr, bEnable);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_SCU_CLK_SetExtLSI(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;
    uint8_t un8Arg = 1;
    bool bLdoEnable = false, bShiftEnable = false;

    eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], &bLdoEnable);
    if (eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_CLK_ERR_STR, NULL, EX_COMM_STR_OPT_LDO);
        goto err;
    }

    eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], &bShiftEnable);
    if (eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_CLK_ERR_STR, NULL, EX_COMM_STR_OPT_SHIFT);
        goto err;
    }

    eErr = HAL_SCU_CLK_SetExtLSI(bLdoEnable, bShiftEnable);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_SCU_CLK_SetMonitor(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;
    uint8_t un8Arg = 1;
    SCUCLK_SRC_e eSrc = SCUCLK_SRC_MAX;
    bool bAutoRecEnable = false;
    SCUCLK_MON_CFG_t tMonCfg; 

    memset(&tMonCfg, 0x00, sizeof(SCUCLK_MON_CFG_t));

    eDbgStatus = EX_SCU_CLK_GetClk(un8Arg++, pn8Argv, &eSrc);
    if(eDbgStatus != DEBUG_CMD_SUCCESS
      || ((eSrc != SCUCLK_SRC_HSE) && (eSrc != SCUCLK_SRC_LSE) && (eSrc != SCUCLK_SRC_MCLK)))
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_CLK_ERR_STR, NULL, EX_COMM_STR_OPT_SRC);
        goto err;
    }

    if(eSrc == SCUCLK_SRC_MCLK)
    {
        eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], &bAutoRecEnable);
        if (eDbgStatus != DEBUG_CMD_SUCCESS)
        {
            EX_COMMON_SetShowModuleLog(EX_SCU_CLK_ERR_STR, "[rec]", EX_COMM_STR_OPT_MAX);
            goto err;
        }

        eErr = HAL_SCU_CLK_SetAutoRecovery(bAutoRecEnable);
        if(eErr != HAL_ERR_OK)
        {
            goto err;
        }
    }

    eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], &tMonCfg.bEnable);
    if (eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_SCU_CLK_ERR_STR, NULL, EX_COMM_STR_OPT_ENA);
        goto err;
    }

    tMonCfg.eSrc = eSrc;

    if(tMonCfg.bEnable == true)
    {
        switch (pn8Argv[un8Arg++][0])
        {
            case 'n':
                tMonCfg.bNmiEnable = true;
                break;
            case 'm':
                tMonCfg.bNmiEnable = false;
                break;
            default:
                goto err;
        }

        if(eDbgStatus != DEBUG_CMD_SUCCESS)
        {
            goto err;
        }

        tMonCfg.pfnHandler = &EX_SCU_CLK_IRQHandler;
        tMonCfg.pContext = NULL;
        tMonCfg.un32Prio = (uint8_t)atoi(pn8Argv[un8Arg++]);
    }
    else
    {
        tMonCfg.pfnHandler = NULL;
        tMonCfg.pContext = NULL;
        tMonCfg.un32Prio = 0;
    }

    eErr = HAL_SCU_CLK_SetMonitor(&tMonCfg);
    if(eErr != HAL_ERR_OK)
    {
       goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static struct debug_cmd s_tEX_SCU_CLK_CMD[] =
{
    {EX_SCU_CLK_STR, "h", EX_SCU_CLK_Help,"help"},
    {EX_SCU_CLK_STR, "mclk", EX_SCU_CLK_SetMClk,""},
    {EX_SCU_CLK_STR, "pclk", EX_SCU_CLK_SetPClkDiv,""},
    {EX_SCU_CLK_STR, "clken", EX_SCU_CLK_SetSrcEnable,""},
    {EX_SCU_CLK_STR, "pll", EX_SCU_CLK_SetPLL,""},
    {EX_SCU_CLK_STR, "clkout", EX_SCU_CLK_SetOutPort,""},
    {EX_SCU_CLK_STR, "exthse", EX_SCU_CLK_SetExtHSE,""},
    {EX_SCU_CLK_STR, "extlse", EX_SCU_CLK_SetExtLSE,""},
    {EX_SCU_CLK_STR, "extlsi", EX_SCU_CLK_SetExtLSI,""},
    {EX_SCU_CLK_STR, "mon", EX_SCU_CLK_SetMonitor,""}

};

void EX_SCU_CLK(void)
{
    /* Add TC commands */
    debug_cmd_init(s_tEX_SCU_CLK_CMD, DEBUG_CMD_LIST_COUNT(s_tEX_SCU_CLK_CMD));
}

/* --------------------------------- End Of File ------------------------------ */
