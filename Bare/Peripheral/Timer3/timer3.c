/**
 *******************************************************************************
 * @file        timer3.c
 * @author      ABOV R&D Division
 * @brief       Timer3 Example Code
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

#if defined(TIMER3_TC)
#include "abov_config.h"
#include "ex_common.h"
#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "hal_scu.h"
#include "hal_scu_clk.h"
#include "hal_pcu.h"
#include "hal_timer3.h"

#if !defined(_TIMER3)
#error "This chipset did not support this example."
#endif

#define EX_TIMER3_STR "TIMER3"
#define EX_TIMER3_LOG_STR "TIMER3 :"
#define EX_TIMER3_ERR_STR "[E]TIMER3 :"
#define EX_TIMER3_IRQ_PRIO 3
#define EX_TIMER3_MAX_ARG  7
#define EX_TIMER3_MAX_NUM (CONFIG_TIMER3_MAX_COUNT - 1)

extern const char *pcCommStr[];
extern const char *pcCmdStr[];
extern const char *pcValStr[];
extern const char *pcOptStr[];

static TIMER3_Context_t s_tTimer3Context[CONFIG_TIMER3_MAX_COUNT];
static uint32_t un32ISRLog = 0;

static enum debug_cmd_status EX_TIMER3_Help(int32_t n32Argc, char *pn8Argv[])
{
    uint32_t un32ShowOpt = 0;
    EX_COMM_STR_OPT_e eOpt[10];

    EX_COMMON_SetShowModuleInfo(EX_TIMER3_STR, CONFIG_TIMER3_MAX_COUNT);

    un32ShowOpt = EX_COMMON_GetShowOpt(pn8Argv[1]);

    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_INIT, NULL, EX_TIMER3_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_UNINIT, NULL, EX_TIMER3_MAX_NUM, eOpt, 0, NULL);

    eOpt[0] = EX_COMM_STR_OPT_SRC; 
    eOpt[1] = EX_COMM_STR_OPT_DIV; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CLK, NULL, EX_TIMER3_MAX_NUM, eOpt, 2, NULL);
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_SRC, EX_COMM_STR_VAL_CLKPATH, "/e(ext)");
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MCCR, EX_COMM_STR_VAL_MAX, "[mccr] [div]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MCCR, EX_COMM_STR_VAL_CLKSRC, "/m(mclk)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_DIV, EX_COMM_STR_VAL_N_255, NULL);
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "ext: [edge]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_EDGE, EX_COMM_STR_VAL_EDGE, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_DIV, EX_COMM_STR_VAL_N_NUM, NULL);
    }
    eOpt[0] = EX_COMM_STR_OPT_OPS; 
    eOpt[1] = EX_COMM_STR_OPT_MODE; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CONFIG, NULL, EX_TIMER3_MAX_NUM, eOpt, 2, "[out] [-pol] [-hiz] [-dly] [-updt] [-pmoc] [dp] [da] [db] [dc]");

    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_OPS, EX_COMM_STR_VAL_OPS, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MODE, EX_COMM_STR_VAL_MAX, "i(interval)/c(capture)/b(backtoback)");
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "capture: [edge]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_EDGE, EX_COMM_STR_VAL_EDGE, "/b(both)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "out: s(6-ch)/a(a-ch)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-pol: [xa] [xb]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "xa: l(low)/h(high)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "xb: l(low)/h(high)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-hiz: [edge] [src] [msk]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_EDGE, EX_COMM_STR_VAL_EDGE, NULL);
        //LOG("\t\t  edge : f(fall)/r(rise)\n"); 
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_SRC, EX_COMM_STR_VAL_MAX, "e(ext:blank)/t(timer40)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "msk: m(mask)/n(non-mask)"); 
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-dly: [pos] [val]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "pos: f(front xa / back xb)/b(front xb and back xa)"); 
	EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "val: 0~N"); 
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-updt: [reload]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "reload: i(instant)/p(period)/b(bottom)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-pmoc: [cnt]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_CNT, EX_COMM_STR_VAL_MAX, "1(every),2,3,4,5,6,7,8");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_CNT, EX_COMM_STR_VAL_MAX, "dp: 0~N(period)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_DA, EX_COMM_STR_VAL_N_NUM, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_DB, EX_COMM_STR_VAL_N_NUM, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "dc: 0~N");
    }
    eOpt[0] = EX_COMM_STR_OPT_ENA; 
    eOpt[1] = EX_COMM_STR_OPT_LEVEL; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "port", EX_TIMER3_MAX_NUM, eOpt, 2, NULL);
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_ENA, EX_COMM_STR_VAL_MAX, "hexa(mask port num ex.0x3f)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_LEVEL, EX_COMM_STR_VAL_MAX, "hexa(port no enable ex.0x77)");
    }
    eOpt[0] = EX_COMM_STR_OPT_ENA; 
    eOpt[1] = EX_COMM_STR_OPT_CNT; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "adctrg", EX_TIMER3_MAX_NUM, eOpt, 2, NULL);
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_ENA, EX_COMM_STR_VAL_MAX, "hexa(mask adctrg num ex.0x1f)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_CNT, EX_COMM_STR_VAL_N_NUM, NULL);
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "t1sync", EX_TIMER3_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_START, NULL, EX_TIMER3_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_STOP, NULL, EX_TIMER3_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_LOG, NULL, -1, eOpt, 0, (char *)pcValStr[EX_COMM_STR_VAL_ONOFF]);
    LOG("\n");

    return DEBUG_CMD_SUCCESS;
}

static void EX_TIMER3_IRQHandler(uint32_t un32Event, void *pContext)
{
    TIMER3_Context_t *ptContext = (TIMER3_Context_t *)pContext;
    uint32_t un32Data;

    if(un32Event & TIMER3_INTR_PERIOD_MATCH)
    {
        if(un32ISRLog == 1)
        {
            LOG("%s (%d) P evt fire\n", EX_TIMER3_LOG_STR, ptContext->eId);
        }
    }

    if(un32Event & TIMER3_INTR_MATCH_CH_A)
    {
        if(un32ISRLog == 1)
        {
            LOG("%s (%d) M evt A fire\n", EX_TIMER3_LOG_STR, ptContext->eId);
        }
    }

    if(un32Event & TIMER3_INTR_MATCH_CH_B)
    {
        if(un32ISRLog == 1)
        {
            LOG("%s (%d) M evt B fire\n", EX_TIMER3_LOG_STR, ptContext->eId);
        }
    }

    if(un32Event & TIMER3_INTR_MATCH_CH_C)
    {
        if(un32ISRLog == 1)
        {
            LOG("%s (%d) M evt C fire\n", EX_TIMER3_LOG_STR, ptContext->eId);
        }
    }

    if(un32Event & TIMER3_INTR_BOTTOM)
    {
        if(un32ISRLog == 1)
        {
            LOG("%s (%d) B evt fire\n", EX_TIMER3_LOG_STR, ptContext->eId);
        }
    }

    if(un32Event & TIMER3_INTR_CAPTURE)
    {
        HAL_TIMER3_GetData(ptContext->eId, TIMER3_DATA_CAP, &un32Data);
        if(un32ISRLog == 1)
        {
            LOG("%s (%d) C evt fire (Data=%d)\n", EX_TIMER3_LOG_STR, ptContext->eId, un32Data);
        }
    }

    if(un32Event & TIMER3_INTR_HIZ)
    {
        if(un32ISRLog == 1)
        {
            LOG("%s (%d) H evt fire\n", EX_TIMER3_LOG_STR, ptContext->eId);
        }
    }
}

static enum debug_cmd_status EX_TIMER3_GetClkConfig(int32_t n32Argc, char *pn8Argv[], TIMER3_CLK_CFG_t *ptClkCfg)
{
    uint8_t un8Arg = 2;

    if (pn8Argv[un8Arg][0] == 'p')
        ptClkCfg->eClk = TIMER3_CLK_PCLK;
    else if (pn8Argv[un8Arg][0] == 'm')
        ptClkCfg->eClk = TIMER3_CLK_MCCR;
    else if (pn8Argv[un8Arg][0] == 'e')
        ptClkCfg->eClk = TIMER3_CLK_EXT;
    else
    {
        goto err;
    }
    
    if(ptClkCfg->eClk == TIMER3_CLK_MCCR)
    {
        un8Arg++;
        if(pn8Argv[un8Arg][0] == 'l')
            ptClkCfg->uSubClk.eMccr = TIMER3_CLK_MCCR_LSI;
        else if(pn8Argv[un8Arg][0] == 's')
            ptClkCfg->uSubClk.eMccr = TIMER3_CLK_MCCR_LSE;
        else if(pn8Argv[un8Arg][0] == 'm')
            ptClkCfg->uSubClk.eMccr = TIMER3_CLK_MCCR_MCLK;
        else if(pn8Argv[un8Arg][0] == 'h')
            ptClkCfg->uSubClk.eMccr = TIMER3_CLK_MCCR_HSI;
        else if(pn8Argv[un8Arg][0] == 'e')
            ptClkCfg->uSubClk.eMccr = TIMER3_CLK_MCCR_HSE;
        else if(pn8Argv[un8Arg][0] == 'p')
            ptClkCfg->uSubClk.eMccr = TIMER3_CLK_MCCR_PLL;
        else 
        {
            EX_COMMON_SetShowModuleLog(EX_TIMER3_ERR_STR, NULL, EX_COMM_STR_OPT_MCCR);
            goto err;
        }  

        un8Arg++;
        ptClkCfg->un8MccrDiv = atoi(pn8Argv[un8Arg]);
    }
    else if(ptClkCfg->eClk == TIMER3_CLK_EXT)
    {
        un8Arg++;
        if(pn8Argv[un8Arg][0] == 'f')
            ptClkCfg->uSubClk.eExtClkEdge = TIMER3_EXTCLK_EDGE_FALLING;
        else if(pn8Argv[un8Arg][0] == 'r')
            ptClkCfg->uSubClk.eExtClkEdge = TIMER3_EXTCLK_EDGE_RISING;
        else 
        {
            EX_COMMON_SetShowModuleLog(EX_TIMER3_ERR_STR, NULL, EX_COMM_STR_OPT_EDGE);
            goto err;
        }
    }

    un8Arg++;
    ptClkCfg->un16PreScale = atoi(pn8Argv[un8Arg]);

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_TIMER3_GetConfig(int32_t n32Argc, char *pn8Argv[], TIMER3_OPS_e *peOps, TIMER3_CFG_t *ptCfg, bool *pbBlnkNonMask)
{
    uint8_t un8Arg = 2;

    switch (pn8Argv[un8Arg++][0])
    {
        case 'p':
            *peOps = TIMER3_OPS_POLL;
            break;
        case 'i':
            *peOps = TIMER3_OPS_INTR;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_TIMER3_ERR_STR, NULL, EX_COMM_STR_OPT_EDGE);
            goto err;
    }

    if ((pn8Argv[un8Arg][0]) == 'i')
    {
        ptCfg->eMode = TIMER3_MODE_INTERVAL;
        ptCfg->un8IntrEnable = TIMER3_INTR_MATCH_CH_C | TIMER3_INTR_MATCH_CH_B | TIMER3_INTR_MATCH_CH_A | TIMER3_INTR_PERIOD_MATCH;
    }
    else if ((pn8Argv[un8Arg][0]) == 'c') 
    {
        ptCfg->eMode = TIMER3_MODE_CAPTURE;
        ptCfg->un8IntrEnable = TIMER3_INTR_CAPTURE;
    }
    else if ((pn8Argv[un8Arg][0]) == 'b') 
    {
        ptCfg->eMode = TIMER3_MODE_BACK2BACK;
        ptCfg->un8IntrEnable = TIMER3_INTR_MATCH_CH_C | TIMER3_INTR_MATCH_CH_B | TIMER3_INTR_MATCH_CH_A | TIMER3_INTR_PERIOD_MATCH | TIMER3_INTR_BOTTOM;
    }
    else
    {
        EX_COMMON_SetShowModuleLog(EX_TIMER3_ERR_STR, NULL, EX_COMM_STR_OPT_MODE);
        goto err;
    }

    if(ptCfg->eMode == TIMER3_MODE_CAPTURE)
    {
        un8Arg++;
        if ((pn8Argv[un8Arg][0] == 'r'))
            ptCfg->eCapClr = TIMER3_CAP_CLR_RISING;
        else if ((pn8Argv[un8Arg][0] == 'f'))
            ptCfg->eCapClr = TIMER3_CAP_CLR_FALLING;
        else if ((pn8Argv[un8Arg][0] == 'b'))
            ptCfg->eCapClr = TIMER3_CAP_CLR_BOTH;
        else
        {
            EX_COMMON_SetShowModuleLog(EX_TIMER3_ERR_STR, NULL, EX_COMM_STR_OPT_EDGE);
            goto err;
        }
    }

    un8Arg++;
    switch (pn8Argv[un8Arg++][0])
    {
        case 's':
            ptCfg->tOutputCfg.eOutputMode = TIMER3_OUT_MODE_6CH;
            break;
        case 'a':
            ptCfg->tOutputCfg.eOutputMode = TIMER3_OUT_MODE_ACH;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_TIMER3_ERR_STR, "[out]", EX_COMM_STR_OPT_MAX);
            goto err;
    }

    if(strncmp(pn8Argv[un8Arg],"-pol",4) == 0)
    {
        un8Arg++;
        for(int i = 0; i < 2; i++)
        {
            switch (pn8Argv[un8Arg++][0])
            {
                case 'h':
                    if(i == 0)
                    {
                        ptCfg->tOutputCfg.eXAPol = TIMER3_POL_HIGH;
                    }
                    else
                    {
                        ptCfg->tOutputCfg.eXBPol = TIMER3_POL_HIGH;
                    }
                    break;
                case 'l':
                    if(i == 0)
                    {
                        ptCfg->tOutputCfg.eXAPol = TIMER3_POL_LOW;
                    }
                    else
                    {
                        ptCfg->tOutputCfg.eXBPol = TIMER3_POL_LOW;
                    }
                    break;
                default:
                    EX_COMMON_SetShowModuleLog(EX_TIMER3_ERR_STR, (i == 0) ? "xa" : "xb", EX_COMM_STR_OPT_MAX);
                    goto err;
            }
        }
    } 

    if(strncmp(pn8Argv[un8Arg],"-hiz",4) == 0)
    {
        un8Arg++;
        ptCfg->tOutputCfg.bHizEnable = true;
        ptCfg->un8IntrEnable |= TIMER3_INTR_HIZ;

        switch (pn8Argv[un8Arg++][0])
        {
            case 'r':
                ptCfg->tOutputCfg.eHizEdge = TIMER3_HIZ_EDGE_RISING;
                break;
            case 'f':
                ptCfg->tOutputCfg.eHizEdge = TIMER3_HIZ_EDGE_FALLING;
                break;
            default:
                EX_COMMON_SetShowModuleLog(EX_TIMER3_ERR_STR, "hiz", EX_COMM_STR_OPT_EDGE);
                goto err;
        }

        switch (pn8Argv[un8Arg++][0])
        {
            case 'e':
                ptCfg->tOutputCfg.eHizSrc = TIMER3_HIZ_SRC_EXT;
                break;
            case 't':
                ptCfg->tOutputCfg.eHizSrc = TIMER3_HIZ_SRC_TIMER40;
                break;
            default:
                EX_COMMON_SetShowModuleLog(EX_TIMER3_ERR_STR, "hiz", EX_COMM_STR_OPT_SRC);
                goto err;
        }

        switch (pn8Argv[un8Arg++][0])
        {
            case 'n':
                *pbBlnkNonMask = true;
                break;
            case 'm':
                *pbBlnkNonMask = false;
                break;
            default:
                EX_COMMON_SetShowModuleLog(EX_TIMER3_ERR_STR, "hiz[msk]", EX_COMM_STR_OPT_MAX);
                goto err;
        }
    } 
    else
    {
        ptCfg->tOutputCfg.bHizEnable = false;
        *pbBlnkNonMask = false;
    }

    if(strncmp(pn8Argv[un8Arg],"-dly",4) == 0)
    {
        un8Arg++;
        ptCfg->tDlyCfg.bEnable = true;
        switch (pn8Argv[un8Arg++][0])
        {
            case 'f':
                ptCfg->tDlyCfg.ePos = TIMER3_DLY_POS_XAXB;
                break;
            case 'b':
                ptCfg->tDlyCfg.ePos = TIMER3_DLY_POS_XBXA;
                break;
            default:
                EX_COMMON_SetShowModuleLog(EX_TIMER3_ERR_STR, "-dly[msk]", EX_COMM_STR_OPT_MAX);
                goto err;
        }
        ptCfg->tDlyCfg.un16Value = (uint16_t)atoi(pn8Argv[un8Arg++]);
    }

    if(strncmp(pn8Argv[un8Arg],"-updt",5) == 0)
    {
        un8Arg++;
        if (pn8Argv[un8Arg][0] == 'i')
            ptCfg->eReload = TIMER3_RELOAD_INSTANT;
        else if (pn8Argv[un8Arg][0] == 'p')
            ptCfg->eReload = TIMER3_RELOAD_PERIOD_MATCH;
        else if (pn8Argv[un8Arg][0] == 'b')
            ptCfg->eReload = TIMER3_RELOAD_BOTTOM;
        else 
        {
            EX_COMMON_SetShowModuleLog(EX_TIMER3_ERR_STR, "updt[reload]", EX_COMM_STR_OPT_MAX);
            goto err;
        }
        un8Arg++;
    }

    if(strncmp(pn8Argv[un8Arg],"-pmoc",5) == 0)
    {
        un8Arg++;
        ptCfg->un8PeriodMatchCnt = (uint8_t)atoi(pn8Argv[un8Arg++]);   
        if(ptCfg->un8PeriodMatchCnt == 0 || ptCfg->un8PeriodMatchCnt > 8)
        {
            EX_COMMON_SetShowModuleLog(EX_TIMER3_ERR_STR, "pmoc", EX_COMM_STR_OPT_CNT);
            goto err;
        }
    }

    ptCfg->utData.tGRD.un16DataP = (uint16_t)atoi(pn8Argv[un8Arg++]);
    ptCfg->utData.tGRD.un16DataA = (uint16_t)atoi(pn8Argv[un8Arg++]);
    ptCfg->utData.tGRD.un16DataB = (uint16_t)atoi(pn8Argv[un8Arg++]);
    ptCfg->utData.tGRD.un16DataC = (uint16_t)atoi(pn8Argv[un8Arg++]);

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_TIMER3_GetId(int32_t n32Argc, char *pn8Argv[], TIMER3_ID_e *peId)
{
    uint8_t un8Data = 0;
    if(n32Argc == 1)
    {
        EX_COMMON_SetShowModuleLog(EX_TIMER3_ERR_STR, "arg", EX_COMM_STR_OPT_MAX);
        return DEBUG_CMD_INVALID;
    }

    un8Data = atoi(pn8Argv[1]); 
    if(un8Data >= CONFIG_TIMER3_MAX_COUNT)
    {
        LOG("%s Max Chan %d\n", EX_TIMER3_ERR_STR, CONFIG_TIMER3_MAX_COUNT);
        return DEBUG_CMD_INVALID;
    }

    *peId = (TIMER3_ID_e)un8Data;

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_TIMER3_Init(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    TIMER3_ID_e eId;

    eDbgStatus = EX_TIMER3_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_TIMER3_Init(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_TIMER3_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_INIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_TIMER3_Uninit(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    TIMER3_ID_e eId;

    eDbgStatus = EX_TIMER3_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_TIMER3_Uninit(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_TIMER3_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_UNINIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_TIMER3_SetClk(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    TIMER3_ID_e eId;

    TIMER3_CLK_CFG_t tClkCfg =
    {
#if (TIMER3_CLKSRC_USE_PCLK == 1)
        .eClk = TIMER3_CLK_PCLK_DIV_16,
        .uSubClk.eMccr = TIMER3_CLK_MCCR_NONE,
        .un8MccrDiv = 0,
#else
        .eClk  = TIMER3_CLK_MCCR,
        .uSubClk.eMccr = TIMER3_CLK_MCCR_HSE,
        .un8MccrDiv = DEFAULT_HSE_1MHZ_DIV,
#endif
    };

    eDbgStatus = EX_TIMER3_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eDbgStatus = EX_TIMER3_GetClkConfig(n32Argc, pn8Argv, &tClkCfg);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_TIMER3_SetClkConfig(eId, &tClkCfg);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_TIMER3_SetConfig(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    TIMER3_ID_e eId;
    TIMER3_OPS_e eOps;
    TIMER3_CFG_t tTimer3Cfg;
    bool bBlnkNonMask = false;

    eDbgStatus = EX_TIMER3_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    memset(&tTimer3Cfg, 0x00, sizeof(TIMER3_CFG_t));

    eDbgStatus = EX_TIMER3_GetConfig(n32Argc, pn8Argv, &eOps, &tTimer3Cfg, &bBlnkNonMask);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_TIMER3_SetConfig(eId, &tTimer3Cfg);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    if(eOps == TIMER3_OPS_INTR)
    {
        eErr = HAL_TIMER3_SetIRQ(eId, eOps, EX_TIMER3_IRQHandler, (void *)&s_tTimer3Context[(uint32_t)eId], EX_TIMER3_IRQ_PRIO, bBlnkNonMask);
        if(eErr != HAL_ERR_OK)
        {
            return DEBUG_CMD_INVALID;
        }
    }

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_TIMER3_SetPort(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    TIMER3_ID_e eId;
    uint32_t un32Enable = 0, un32Level;

    if(n32Argc != 4)
    {
        return DEBUG_CMD_INVALID;
    }

    eDbgStatus = EX_TIMER3_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }
    
    sscanf(pn8Argv[2], "%X", &un32Enable);
    sscanf(pn8Argv[3], "%X", &un32Level);

    eErr = HAL_TIMER3_SetOutputPort(eId, (uint8_t)un32Enable, (uint8_t)un32Level);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_TIMER3_SetAdcTrg(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    TIMER3_ID_e eId;
    uint32_t un32AdcTrgEnable = 0;
    TIMER3_ADCTRG_CFG_t tCfg;

    if(n32Argc != 4)
    {
        return DEBUG_CMD_INVALID;
    }

    eDbgStatus = EX_TIMER3_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    memset(&tCfg, 0x00, sizeof(TIMER3_ADCTRG_CFG_t));
    
    sscanf(pn8Argv[2], "%X", &un32AdcTrgEnable);

    tCfg.un8Enable = (uint8_t)un32AdcTrgEnable;
    tCfg.un16GenData = (uint16_t)atoi(pn8Argv[3]);

    eErr = HAL_TIMER3_SetAdcTrgConfig(eId, &tCfg);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_TIMER3_Start(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    TIMER3_ID_e eId;

    eDbgStatus = EX_TIMER3_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    s_tTimer3Context[eId].eId = eId;

    eErr = HAL_TIMER3_Start(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_TIMER3_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_START]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_TIMER3_Stop(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    TIMER3_ID_e eId;

    eDbgStatus = EX_TIMER3_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_TIMER3_Stop(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_TIMER3_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_STOP]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_TIMER3_SetLog(int32_t n32Argc, char *pn8Argv[])
{
    if((strncmp(pn8Argv[1],"on",2) == 0))
    {
        un32ISRLog = 1;
    }
    else
    {
        un32ISRLog = 0;
    }

    LOG("%s ISR Log %s.\n", EX_TIMER3_LOG_STR, (un32ISRLog == 1 ? "on":"off"));

    return DEBUG_CMD_SUCCESS;
}

static struct debug_cmd s_tEX_TIMER3_CMD[] =
{
    {EX_TIMER3_STR, "h", EX_TIMER3_Help, "help"},
    {EX_TIMER3_STR, "init", EX_TIMER3_Init,""},
    {EX_TIMER3_STR, "uninit", EX_TIMER3_Uninit,""},
    {EX_TIMER3_STR, "clk", EX_TIMER3_SetClk,""},
    {EX_TIMER3_STR, "config",EX_TIMER3_SetConfig,""},
    {EX_TIMER3_STR, "port", EX_TIMER3_SetPort,""},
    {EX_TIMER3_STR, "adctrg", EX_TIMER3_SetAdcTrg,""},
    {EX_TIMER3_STR, "start", EX_TIMER3_Start,""},
    {EX_TIMER3_STR, "stop", EX_TIMER3_Stop,""},
    {EX_TIMER3_STR, "log", EX_TIMER3_SetLog,""}
};

/**********************************************************************
 * @brief		Main program
 * @param[in]	None
 * @return	None
 **********************************************************************/
void EX_TIMER3(void)
{
    /* Add EX commands */
    debug_cmd_init(s_tEX_TIMER3_CMD,DEBUG_CMD_LIST_COUNT(s_tEX_TIMER3_CMD));
}

#endif
/* --------------------------------- End Of File ------------------------------ */
