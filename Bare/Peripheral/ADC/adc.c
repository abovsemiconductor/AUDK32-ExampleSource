/**
 *******************************************************************************
 * @file        adc.c
 * @author      ABOV R&D Division
 * @brief       ADC Example Code
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

#if defined(ADC_TC)
#include "abov_config.h"
#include "ex_configs.h"
#include "ex_common.h"
#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "hal_scu.h"
#include "hal_scu_clk.h"
#include "hal_pcu.h"
#include "hal_adc.h"
#if defined(_TIMER1)
#include "hal_timer1.h"
#endif

#if !defined(_ADC)
#error "This chipset did not support this example."
#endif

#define EX_ADC_STR "ADC"
#define EX_ADC_LOG_STR "ADC :"
#define EX_ADC_ERR_STR "[E]ADC :"
#define EX_ADC_IRQ_PRIO 3
#define EX_ADC_LOOP 1000
#define EX_ADC_DELAY 1
#if defined (EXTRN_SINGLE_ADC)
#define EX_ADC_RBUF_SIZE 20
#define EX_ADC_SEQ_CNT 1
#else
#define EX_ADC_RBUF_SIZE 80
#define EX_ADC_SEQ_CNT 8
#endif
#define EX_ADC_MAX_NUM  (CONFIG_ADC_MAX_COUNT - 1)

extern const char *pcCommStr[];
extern const char *pcCmdStr[];
extern const char *pcValStr[];
extern const char *pcOptStr[];

extern uint32_t SystemCoreClock;
static ADC_OPS_e s_eOps = ADC_OPS_POLL;
static ADC_MODE_e s_eMode = ADC_MODE_SINGLE;
static ADC_Context_t s_tADCContext[CONFIG_ADC_MAX_COUNT];

static uint16_t s_aun16RBuf[EX_ADC_RBUF_SIZE];
static uint32_t s_un32RCnt = 0;
static ADC_SEQ_DATA_t s_tResult[EX_ADC_RBUF_SIZE];
static ADC_TRG_TYPE_e s_eType = ADC_TRG_TYPE_MAX;

#if defined (EX_ADC_TRG_TYPE_INDEPENDENT)
static uint8_t s_un8SeqCnt = 0;
static bool s_bEnableCmp = false;
#endif
#if defined (EX_ADC_TRG_TYPE_SHARE)
static uint8_t s_un8ChCnt = 0;
#endif

typedef struct
{
    ADC_TRG_SRC_e    eTrgSrc;
    uint8_t          un8TrgNum;
    uint8_t          un8TrgMap;
    uint8_t          un8LastSeqNum;
    bool             bValid;
} EX_ADC_GROUP_t;

typedef struct
{
    ADC_TRG_SRC_e    eTrgSrc;
    uint8_t          un8TrgNum;
} EX_ADC_TRG_INFO_t;

static EX_ADC_GROUP_t s_tTCAdcGroup[EX_ADC_SEQ_CNT];
static EX_ADC_TRG_INFO_t s_tTCAdcTrgInfo[EX_ADC_RBUF_SIZE];
static uint32_t un32ISRLog = 0;

static void PRV_EX_ADC_Print(uint32_t un32Cnt)
{
    LOG("%s [%s][%d\t:%d]\t[Ch=%d]\t[TrgInfo=%x]", EX_ADC_LOG_STR, s_tResult[un32Cnt].bReadDDR == true ? "DDR" : "DRx",
         un32Cnt, s_tResult[un32Cnt].un16Result, s_tResult[un32Cnt].un8ChInfo, s_tResult[un32Cnt].un8TrgInfo);
    if(s_eMode == ADC_MODE_MULTIPLE)
    {
        LOG("\t[TrgSrc=%d]\t[TrgSrcSubNum=%d]\n", s_tTCAdcTrgInfo[un32Cnt].eTrgSrc, s_tTCAdcTrgInfo[un32Cnt].un8TrgNum);
    }
    else
    {
        LOG("\n");
    }
}

static enum debug_cmd_status EX_ADC_Help(int32_t n32Argc, char *pn8Argv[])
{
    uint32_t un32ShowOpt = 0;
    EX_COMM_STR_OPT_e eOpt[2];

    EX_COMMON_SetShowModuleInfo(EX_ADC_STR, CONFIG_ADC_MAX_COUNT);

    un32ShowOpt = EX_COMMON_GetShowOpt(pn8Argv[1]);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_INIT, NULL, EX_ADC_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_UNINIT, NULL, EX_ADC_MAX_NUM, eOpt, 0, NULL);

    eOpt[0] = EX_COMM_STR_OPT_SRC; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CLK, NULL, EX_ADC_MAX_NUM, eOpt, 1, NULL);
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_SRC, EX_COMM_STR_VAL_CLKPATH, NULL);
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MCCR, EX_COMM_STR_VAL_MAX, "[mccr] [div]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MCCR, EX_COMM_STR_VAL_CLKSRC, "/m(mclk)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_DIV, EX_COMM_STR_VAL_N_255, NULL);
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "pclk: [div]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_DIV, EX_COMM_STR_VAL_N_NUM, NULL);
    }
#if defined (EX_ADC_TRG_TYPE_SINGLE)
    eOpt[0] = EX_COMM_STR_OPT_OPS; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CONFIG, NULL, EX_ADC_MAX_NUM, eOpt, 1, "[ref]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_OPS, EX_COMM_STR_VAL_OPS, "/d(intr dma)/n(nmi)/m(nmi dma)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "ref: i(VDD)/e(ext)");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "seq", EX_ADC_MAX_NUM, eOpt, 0, "[trg] [trgnum] [ch]");
    if (un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "trg: a(adst)/1(timer1)/2(timer2)/3(timer3)/4(timer4)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "trgnum: 0~N");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "ch: 0~N");
    }
#elif defined (EX_ADC_TRG_TYPE_SHARE)
    eOpt[0] = EX_COMM_STR_OPT_OPS; 
    eOpt[1] = EX_COMM_STR_OPT_MODE; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CONFIG, NULL, EX_ADC_MAX_NUM, eOpt, 2, "");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_OPS, EX_COMM_STR_VAL_OPS, "/d(intr dma)/n(nmi)/m(nmi dma)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MODE, EX_COMM_STR_VAL_MAX, "s(single)/q(seq)/c(conti)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "smptime: 0~N");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "seq", EX_ADC_MAX_NUM, eOpt, 0, "[trg] [ch]");
    if (un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "trg: a(adst)/1(timer1)/2(timer2)/3(timer3)/4(timer4)/5(edge)");
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "1,2,3,4");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "trgnum: 0~N");
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "5(edge)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "edge: f(fall)/r(rise)/b(both)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "ch: 0x00~0xff");
    }
#elif defined (EX_ADC_TRG_TYPE_INDEPENDENT)
    eOpt[0] = EX_COMM_STR_OPT_OPS; 
    eOpt[1] = EX_COMM_STR_OPT_MODE; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CONFIG, NULL, EX_ADC_MAX_NUM, eOpt, 2, "[btrg] [seqcnt] [smptime]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_OPS, EX_COMM_STR_VAL_OPS, "/d(intr dma)/n(nmi)/m(nmi dma)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MODE, EX_COMM_STR_VAL_MAX, "s(single)/q(seq)/b(burst)/m(multi)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "btrg: a(adst)/1(timer1)/2(timer2)/3(timer3)/4(timer4)/5(mpwm0)/6(mpwm1)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "seqcnt: 1~N");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "smptime: 0~N");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "seq", EX_ADC_MAX_NUM, eOpt, 0, "[itrg] [trgnum] [seq] [ch]");
    if (un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "itrg: a(adst)/1(timer1)/2(timer2)/3(timer3)/4(timer4)/5(mpwm0)/6(mpwm1)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "trgnum: 0~N");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "seq: 0~N");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "ch: 0~N");
    }
#else
#error "There is no trigger type define."
#endif

#if defined (EX_ADC_TRG_TYPE_INDEPENDENT)
    eOpt[0] = EX_COMM_STR_OPT_ENA; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "cmp", EX_ADC_MAX_NUM, eOpt, 0, "[ch] [val] [-intr]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_ENA, EX_COMM_STR_VAL_ENDIS, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "ch: 0~N");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "val: 0~N");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-intr: [en] [trg]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_ENA, EX_COMM_STR_VAL_ENDIS, NULL);
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "trg: u(adc>cval)/d(adc<=cval)");
         
    }
#endif

#if defined (EX_ADC_OVERSAMPLING)
    eOpt[0] = EX_COMM_STR_OPT_ENA; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "ovs", EX_ADC_MAX_NUM, eOpt, 0, "[ratio] [shft]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_ENA, EX_COMM_STR_VAL_ENDIS, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "ratio: 0~N");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "shft: 0~N");
    }
#endif
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_START, NULL, EX_ADC_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_STOP, NULL, EX_ADC_MAX_NUM, eOpt, 0, NULL);
    LOG("\n");

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_ADC_GetId(int32_t n32Argc, char *pn8Argv[], ADC_ID_e *peId)
{
    uint8_t un8Data = 0;
    if(n32Argc == 1)
    {
        EX_COMMON_SetShowModuleLog(EX_ADC_ERR_STR, (char *)pcOptStr[EX_COMM_STR_OPT_ARG], EX_COMM_STR_OPT_MAX);
        return DEBUG_CMD_INVALID;
    }

    un8Data = atoi(pn8Argv[1]); 
    if(un8Data >= CONFIG_ADC_MAX_COUNT)
    {
        LOG("%s Max Chan %d\n", EX_ADC_ERR_STR, CONFIG_ADC_MAX_COUNT);
        return DEBUG_CMD_INVALID;
    }

    *peId = (ADC_ID_e)un8Data;

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_ADC_GetClkConfig(int32_t n32Argc, char *pn8Argv[], ADC_CLK_CFG_t *ptClkCfg)
{
    uint8_t un8Arg = 2;

    switch (pn8Argv[un8Arg++][0])
    {
        case 'p':
            ptClkCfg->eClk = ADC_CLK_PCLK;
            break;
        case 'm':
            ptClkCfg->eClk = ADC_CLK_MCCR;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_ADC_ERR_STR, NULL, EX_COMM_STR_OPT_SRC);
            goto err;
    }

    if(ptClkCfg->eClk == ADC_CLK_PCLK)
    {
        ptClkCfg->un8PClkDiv = atoi(pn8Argv[un8Arg++]);
    }

    if(ptClkCfg->eClk == ADC_CLK_MCCR)
    {
        if (pn8Argv[un8Arg][0] == 'l')
            ptClkCfg->eMccr = ADC_CLK_MCCR_LSI;
        else if (pn8Argv[un8Arg][0] == 's')
            ptClkCfg->eMccr = ADC_CLK_MCCR_LSE;
        else if (pn8Argv[un8Arg][0] == 'm')
            ptClkCfg->eMccr = ADC_CLK_MCCR_MCLK;
        else if (pn8Argv[un8Arg][0] == 'h')
            ptClkCfg->eMccr = ADC_CLK_MCCR_HSI;
        else if (pn8Argv[un8Arg][0] == 'e')
            ptClkCfg->eMccr = ADC_CLK_MCCR_HSE;
        else if (pn8Argv[un8Arg][0] == 'p')
            ptClkCfg->eMccr = ADC_CLK_MCCR_PLL;
        else
        {
            EX_COMMON_SetShowModuleLog(EX_ADC_ERR_STR, NULL, EX_COMM_STR_OPT_MCCR);
            goto err;
        }

        un8Arg++;
        ptClkCfg->un8MccrDiv = atoi(pn8Argv[un8Arg]);
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_ADC_GetConfig(int32_t n32Argc, char *pn8Argv[], ADC_CFG_t *ptCfg, ADC_OPS_e *peOps)
{

    uint8_t un8Arg = 2;

    if (pn8Argv[un8Arg][0] == 'p')
        *peOps = ADC_OPS_POLL;
    else if (pn8Argv[un8Arg][0] == 'i')
        *peOps = ADC_OPS_INTR;
    else if (pn8Argv[un8Arg][0] == 'd')
        *peOps = ADC_OPS_INTR_DMA;
    else if (pn8Argv[un8Arg][0] == 'n')
        *peOps = ADC_OPS_NMI;
    else if (pn8Argv[un8Arg][0] == 'm')
        *peOps = ADC_OPS_NMI_DMA;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_ADC_ERR_STR, NULL, EX_COMM_STR_OPT_OPS);
        goto err;
    }

#if defined (EX_ADC_TRG_TYPE_SINGLE)
    ptCfg->eMode = ADC_MODE_SINGLE;
#elif defined (EX_ADC_TRG_TYPE_SHARE)
    un8Arg++;
    if (pn8Argv[un8Arg][0] == 's')
        ptCfg->eMode = ADC_MODE_SINGLE;
    else if (pn8Argv[un8Arg][0] == 'q')
        ptCfg->eMode = ADC_MODE_SEQ;
    else if (pn8Argv[un8Arg][0] == 'c')
        ptCfg->eMode = ADC_MODE_CONTINUOUS;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_ADC_ERR_STR, NULL, EX_COMM_STR_OPT_MODE);
        goto err;
    }
#elif defined (EX_ADC_TRG_TYPE_INDEPENDENT)
    un8Arg++;
    if (pn8Argv[un8Arg][0] == 's')
        ptCfg->eMode = ADC_MODE_SINGLE;
    else if (pn8Argv[un8Arg][0] == 'q')
        ptCfg->eMode = ADC_MODE_SEQ;
    else if (pn8Argv[un8Arg][0] == 'b')
        ptCfg->eMode = ADC_MODE_BURST;
    else if (pn8Argv[un8Arg][0] == 'm')
        ptCfg->eMode = ADC_MODE_MULTIPLE;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_ADC_ERR_STR, NULL, EX_COMM_STR_OPT_MODE);
        goto err;
    }
#else
#error "There is no trigger type define."
#endif

#if defined (EX_ADC_TRG_TYPE_INDEPENDENT)
    un8Arg++;
    if (pn8Argv[un8Arg][0] == 'a')
        ptCfg->eBaseTrgSrc = ADC_TRG_SRC_ADST;
    else if (pn8Argv[un8Arg][0] == '1')
        ptCfg->eBaseTrgSrc = ADC_TRG_SRC_TIMER1;
    else if (pn8Argv[un8Arg][0] == '2')
        ptCfg->eBaseTrgSrc = ADC_TRG_SRC_TIMER2;
    else if (pn8Argv[un8Arg][0] == '3')
        ptCfg->eBaseTrgSrc = ADC_TRG_SRC_TIMER3;
    else if (pn8Argv[un8Arg][0] == '4')
        ptCfg->eBaseTrgSrc = ADC_TRG_SRC_TIMER4;
    else if (pn8Argv[un8Arg][0] == '5')
        ptCfg->eBaseTrgSrc = ADC_TRG_SRC_MPWM0;
    else if (pn8Argv[un8Arg][0] == '6')
        ptCfg->eBaseTrgSrc = ADC_TRG_SRC_MPWM1;
    else if (pn8Argv[un8Arg][0] == 'n')
        ptCfg->eBaseTrgSrc = ADC_TRG_SRC_NONE;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_ADC_ERR_STR, "[trg]", EX_COMM_STR_OPT_MAX);
        goto err;
    }
#endif
    un8Arg++;
#if defined (EX_ADC_TRG_TYPE_SINGLE)
    switch (pn8Argv[un8Arg++][0])
    {
        case 'i':
            ptCfg->eRef = ADC_REF_INT;
            break;
        case 'e':
            ptCfg->eRef = ADC_REF_EXT;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_ADC_ERR_STR, "[ref]", EX_COMM_STR_OPT_MAX);
            goto err;
    }
#endif

#if defined (EX_ADC_TRG_TYPE_SHARE)
    ptCfg->un8SamplingTime = atoi(pn8Argv[un8Arg++]);
#endif
#if defined (EX_ADC_TRG_TYPE_INDEPENDENT)
    s_un8SeqCnt = (uint8_t)atoi(pn8Argv[un8Arg++]);
    if(s_un8SeqCnt == 0)
    {
        ptCfg->un8SeqCnt = 1;
    }
    else
    {
        ptCfg->un8SeqCnt = s_un8SeqCnt - 1;
    }

    ptCfg->un8SamplingTime = atoi(pn8Argv[un8Arg++]);
    ptCfg->bChInfo = true;
    ptCfg->bTrgInfo = true;
    ptCfg->bAutoRestart = false;
#endif

    s_eMode = ptCfg->eMode;

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_ADC_GetSeqConfig(int32_t n32Argc, char *pn8Argv[], ADC_SEQ_TRG_CFG_t *ptCfg)
{
    uint8_t un8Arg = 2;
#if defined (EX_ADC_TRG_TYPE_SHARE)
    uint32_t un32ChInBit = 0;
#endif

#if defined (EX_ADC_TRG_TYPE_SINGLE)
    s_eType = ADC_TRG_TYPE_SINGLE;
#elif defined (EX_ADC_TRG_TYPE_SHARE)
    s_eType = ADC_TRG_TYPE_SHARE;
#elif defined (EX_ADC_TRG_TYPE_INDEPENDENT)
    s_eType = ADC_TRG_TYPE_INDEPENDENT;
#else
#error "There is no trigger type define."
#endif
    ptCfg->eType = s_eType;
 
    if (pn8Argv[un8Arg][0] == 'a')
        ptCfg->eTrgSrc = ADC_TRG_SRC_ADST;
    else if (pn8Argv[un8Arg][0] == '1')
        ptCfg->eTrgSrc = ADC_TRG_SRC_TIMER1;
    else if (pn8Argv[un8Arg][0] == '2')
        ptCfg->eTrgSrc = ADC_TRG_SRC_TIMER2;
    else if (pn8Argv[un8Arg][0] == '3')
        ptCfg->eTrgSrc = ADC_TRG_SRC_TIMER3;
    else if (pn8Argv[un8Arg][0] == '4')
        ptCfg->eTrgSrc = ADC_TRG_SRC_TIMER4;
#if defined (EX_ADC_TRG_TYPE_SHARE)
    else if (pn8Argv[un8Arg][0] == '5')
        ptCfg->eTrgSrc = ADC_TRG_SRC_EXT;
#elif defined (EX_ADC_TRG_TYPE_INDEPENDENT)
    else if (pn8Argv[un8Arg][0] == '5')
        ptCfg->eTrgSrc = ADC_TRG_SRC_MPWM0;
    else if (pn8Argv[un8Arg][0] == '6')
        ptCfg->eTrgSrc = ADC_TRG_SRC_MPWM1;
#endif
    else if (pn8Argv[un8Arg][0] == 'n')
        ptCfg->eTrgSrc = ADC_TRG_SRC_NONE;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_ADC_ERR_STR, "[trg]", EX_COMM_STR_OPT_MAX);
        goto err;
    }

    un8Arg++;
    if (ptCfg->eTrgSrc != ADC_TRG_SRC_ADST)
    {
#if defined (EX_ADC_TRG_TYPE_SHARE)
        if (ptCfg->eTrgSrc == ADC_TRG_SRC_EXT)
        {
            if (pn8Argv[un8Arg++][0] == 'f')
                ptCfg->utCfg.tShe.eExtTrg = ADC_EXT_TRG_EDGE_FALL;
            else if (pn8Argv[un8Arg++][0] == 'r')
                ptCfg->utCfg.tShe.eExtTrg = ADC_EXT_TRG_EDGE_RISE;
            else if (pn8Argv[un8Arg++][0] == 'b')
                ptCfg->utCfg.tShe.eExtTrg = ADC_EXT_TRG_EDGE_BOTH;
            else
                ptCfg->utCfg.tShe.eExtTrg = ADC_EXT_TRG_EDGE_NONE;
        }
        else
#endif
        {
            ptCfg->un8TrgNum = atoi(pn8Argv[un8Arg++]);
        }
    }
    else
    {
        ptCfg->un8TrgNum = 0;
    }

#if defined (EX_ADC_TRG_TYPE_SINGLE)
    ptCfg->utCfg.tSgl.un8ChNum = atoi(pn8Argv[un8Arg++]);
#elif defined (EX_ADC_TRG_TYPE_SHARE)
    sscanf(pn8Argv[un8Arg++], "%x", &un32ChInBit); 
    ptCfg->utCfg.tShe.un32ChNum = un32ChInBit;
    s_un8ChCnt = 0;
    for (uint8_t i = 0; i < 32; i++)
    {
        if(0x1 & un32ChInBit >> i)
            s_un8ChCnt++;
    }
#elif defined (EX_ADC_TRG_TYPE_INDEPENDENT)
    ptCfg->utCfg.tInd.un8SeqNum = atoi(pn8Argv[un8Arg++]);
    ptCfg->utCfg.tInd.un8ChNum = atoi(pn8Argv[un8Arg++]);
  #if defined(EX_ADC_SAMPLING_TIME_INDEPENDENT)
    ptCfg->utCfg.tInd.un8SamplingTime = atoi(pn8Argv[un8Arg++]);
  #endif
#else
#error "There is no trigger type define."
#endif

#if defined (EX_ADC_TRG_TYPE_INDEPENDENT)
    for(int i = 0; i < EX_ADC_SEQ_CNT; i++)
    {
        if(ptCfg->eTrgSrc == ADC_TRG_SRC_NONE)
        {
            s_tTCAdcGroup[i].un8TrgMap &= ~(0x1UL << ptCfg->utCfg.tInd.un8SeqNum);
            if(s_tTCAdcGroup[i].un8TrgMap == 0)
            {
                memset(&s_tTCAdcGroup[i], 0x00, sizeof(EX_ADC_GROUP_t));
            }
        }
        else if((s_tTCAdcGroup[i].eTrgSrc == ptCfg->eTrgSrc)
            && (s_tTCAdcGroup[i].un8TrgNum == ptCfg->un8TrgNum)
            && (s_tTCAdcGroup[i].bValid == true))
        {
            s_tTCAdcGroup[i].un8TrgMap |= (0x1UL << ptCfg->utCfg.tInd.un8SeqNum);
            if(s_tTCAdcGroup[i].un8LastSeqNum < ptCfg->utCfg.tInd.un8SeqNum)
            {
                s_tTCAdcGroup[i].un8LastSeqNum = ptCfg->utCfg.tInd.un8SeqNum;
            }
            break;
        }
        else
        {
            if(s_tTCAdcGroup[i].bValid == false)
            {
                memset(&s_tTCAdcGroup[i], 0x00, sizeof(EX_ADC_GROUP_t));
                s_tTCAdcGroup[i].eTrgSrc = ptCfg->eTrgSrc;
                s_tTCAdcGroup[i].un8TrgNum = ptCfg->un8TrgNum;
                s_tTCAdcGroup[i].un8TrgMap |= (0x1UL << ptCfg->utCfg.tInd.un8SeqNum);
                s_tTCAdcGroup[i].un8LastSeqNum = ptCfg->utCfg.tInd.un8SeqNum;
                s_tTCAdcGroup[i].bValid = true;
                break;
            }
        }
    }
#endif

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

#if defined (EX_ADC_TRG_TYPE_INDEPENDENT)
static enum debug_cmd_status EX_ADC_GetCmpConfig(int32_t n32Argc, char *pn8Argv[], ADC_CMP_CFG_t *ptCmpCfg)
{
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;
    uint8_t un8Arg = 2;

    eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], &ptCmpCfg->bEnable);
    if (eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_ADC_ERR_STR, NULL, EX_COMM_STR_OPT_ENA);
        goto err;
    }

    if(ptCmpCfg->bEnable == true)
    {
        ptCmpCfg->un8ChNum = (uint8_t)atoi(pn8Argv[un8Arg++]);
        ptCmpCfg->un16Data = (uint16_t)atoi(pn8Argv[un8Arg++]);

        if(strncmp(pn8Argv[un8Arg], "-intr", 5) == 0)
        {
            un8Arg++;
            eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], &ptCmpCfg->bIntrEnable);
            if (eDbgStatus != DEBUG_CMD_SUCCESS)
            {
                EX_COMMON_SetShowModuleLog(EX_ADC_ERR_STR, "-intr", EX_COMM_STR_OPT_ENA);
                goto err;
            }

            if(ptCmpCfg->bIntrEnable == true)
            {
                switch (pn8Argv[un8Arg++][0])
                {
                    case 'u':
                        ptCmpCfg->bIntrTrg = false;
                        break;
                    case 'd':
                        ptCmpCfg->bIntrTrg = true;
                        break;
                    default:
                        EX_COMMON_SetShowModuleLog(EX_ADC_ERR_STR, "-intr[trg]", EX_COMM_STR_OPT_MAX);
                        goto err;
                }
            }
        }
    }

    if(eDbgStatus == DEBUG_CMD_SUCCESS)
    {
        s_bEnableCmp = ptCmpCfg->bEnable;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}
#endif

#if defined (EX_ADC_TRG_TYPE_SINGLE)
static void EX_ADC_IRQHandler(uint32_t un32Event, void *pContext)
{

    HAL_ERR_e eErr = HAL_ERR_OK;
    ADC_ID_e eId = ADC_ID_0;
    ADC_Context_t *ptContext = (ADC_Context_t *)pContext;

    eId = ptContext->eId;

    if(s_eOps == ADC_OPS_INTR || s_eOps == ADC_OPS_NMI)
    {
        switch(s_eMode)
        {
            case ADC_MODE_SINGLE:
                if(un32Event & ADC_EVENT_SINGLE_CAPTURED)
                {
                    eErr = HAL_ADC_GetData(eId, 0, &s_tResult[s_un32RCnt++]);
                    if(eErr != HAL_ERR_OK)
                    {
                        s_un32RCnt = 0;
                        HAL_ADC_Stop(eId);
                    }

                    if(un32ISRLog == 1)
                    {
                        PRV_EX_ADC_Print((s_un32RCnt-1));
                    }
                }
                break;
            default:
                break;
        }
    }

    if(s_un32RCnt < EX_ADC_RBUF_SIZE - 1)
    {
        HAL_ADC_Start(eId);
    }
    else
    {
        HAL_ADC_Stop(eId);
        LOG("%s intr stop\n", EX_ADC_LOG_STR);
    }
}
#elif defined (EX_ADC_TRG_TYPE_SHARE)
static void EX_ADC_IRQHandler(uint32_t un32Event, void *pContext)
{

    HAL_ERR_e eErr = HAL_ERR_OK;
    ADC_ID_e eId = ADC_ID_0;
    ADC_Context_t *ptContext = (ADC_Context_t *)pContext;

    eId = ptContext->eId;

    if(s_eOps == ADC_OPS_INTR || s_eOps == ADC_OPS_NMI)
    {
        switch(s_eMode)
        {
            case ADC_MODE_SINGLE:
            case ADC_MODE_SEQ:
            case ADC_MODE_CONTINUOUS:
                if(un32Event & ADC_EVENT_SINGLE_CAPTURED)
                {
                    eErr = HAL_ADC_GetData(eId, 0, &s_tResult[s_un32RCnt++]);
                    if(eErr != HAL_ERR_OK)
                    {
                        s_un32RCnt = 0;
                        HAL_ADC_Stop(eId);
                    }

                    if(un32ISRLog == 1)
                    {
                        PRV_EX_ADC_Print((s_un32RCnt-1));
                    }
                }
                break;
            default:
                break;
        }
    }
    else
    {
        s_un32RCnt++;
    }

    if(s_un32RCnt >= EX_ADC_RBUF_SIZE)
    {
        HAL_ADC_Stop(eId);
        LOG("%s intr stop\n", EX_ADC_LOG_STR);
    }
    else
    {
        if (s_eMode != ADC_MODE_CONTINUOUS)
        {
            HAL_ADC_Start(eId);
        }
    }
}
#elif defined (EX_ADC_TRG_TYPE_INDEPENDENT)
static void EX_ADC_IRQHandler(uint32_t un32Event, void *pContext)
{

    HAL_ERR_e eErr = HAL_ERR_OK;
    ADC_ID_e eId = ADC_ID_0;
    ADC_Context_t *ptContext = (ADC_Context_t *)pContext;
    uint8_t un8TrgInfo = 0, un8TrgInfoMsked = 0;
    ADC_SEQ_DATA_t tSeqData;
    bool bSkip = false;

    eId = ptContext->eId;

    if(s_eOps == ADC_OPS_INTR || s_eOps == ADC_OPS_NMI)
    {
        if(s_bEnableCmp == true && !(un32Event & ADC_EVENT_COMPARE_MATCHED))
        {
            bSkip = true;
        }

        switch(s_eMode)
        {
            case ADC_MODE_SEQ:
            case ADC_MODE_BURST:
                if((un32Event & ADC_EVENT_BURST_CAPTURED)
                   && (bSkip == false))
             
                {
                    for(int i = 0; i < s_un8SeqCnt; i++)
                    {
                        eErr = HAL_ADC_GetData(eId, i, &s_tResult[s_un32RCnt++]);
                        if(eErr != HAL_ERR_OK)
                        {
                            s_un32RCnt = 0;
                            HAL_ADC_Stop(eId);
                        }
                        if(un32ISRLog == 1)
                        {
                            PRV_EX_ADC_Print((s_un32RCnt-1));
                        }
                    }
  
                }
                break;
            case ADC_MODE_MULTIPLE: 
                if((un32Event & ADC_EVENT_SINGLE_CAPTURED)
                   && (bSkip == false))
                {
#if DUMP_DDR_REGISTER
                    s_tResult[s_un32RCnt].bReadDDR = true;
                    eErr = HAL_ADC_GetData(eId, 0, &s_tResult[s_un32RCnt]);
#else
                    tSeqData.bReadDDR = true;
                    eErr = HAL_ADC_GetData(eId, 0, &tSeqData);
#endif
                    if(eErr != HAL_ERR_OK)
                    {
                        s_un32RCnt = 0;
                        HAL_ADC_Stop(eId);
                        break;
                    }

#if DUMP_DDR_REGISTER
                    un8TrgInfo = s_tResult[s_un32RCnt].un8TrgInfo; 
                    s_un32RCnt++;
#else
                    un8TrgInfo = tSeqData.un8TrgInfo; 
#endif
                    for(int i = 0; i < EX_ADC_SEQ_CNT; i++)
                    {
                        un8TrgInfoMsked = (un8TrgInfo & s_tTCAdcGroup[i].un8TrgMap);
                        if((s_tTCAdcGroup[i].bValid == true)
                            && (s_un32RCnt < EX_ADC_RBUF_SIZE -1)
#if defined(EX_ADC_TRGINFO_TYPE_SEQUENCE_NUMBER)
#else
                            && (un8TrgInfoMsked == (1UL << s_tTCAdcGroup[i].un8LastSeqNum))
#endif
                          )
                        {
                            for (int j = 0; j < EX_ADC_SEQ_CNT; j++)
                            {
                                if((s_tTCAdcGroup[i].un8TrgMap >> j) & 0x1UL)
                                {
                                    s_tResult[s_un32RCnt].bReadDDR = false;
                                    eErr = HAL_ADC_GetData(eId, j, &s_tResult[s_un32RCnt]);
                                    if(eErr != HAL_ERR_OK)
                                    {
                                        s_un32RCnt = 0;
                                        HAL_ADC_Stop(eId);
                                        break;
                                    }

                                    if(un32ISRLog == 1)
                                    {
                                        PRV_EX_ADC_Print((s_un32RCnt-1));
                                    }
#if defined(EX_ADC_TRGINFO_TYPE_SEQUENCE_NUMBER)
#else
                                    if(s_tResult[s_un32RCnt].un8TrgInfo != 0)
#endif
                                    {
                                        
                                        s_tTCAdcTrgInfo[s_un32RCnt].eTrgSrc = s_tTCAdcGroup[i].eTrgSrc;
                                        s_tTCAdcTrgInfo[s_un32RCnt].un8TrgNum = s_tTCAdcGroup[i].un8TrgNum;
                                        s_un32RCnt++;
                                    }
                                     
                                }
                            }
                        }
                    }
                }
                break;
            case ADC_MODE_SINGLE:
                if((un32Event & ADC_EVENT_SINGLE_CAPTURED)
                   && (bSkip == false))
                {
                    eErr = HAL_ADC_GetData(eId, 0, &s_tResult[s_un32RCnt++]);
                    if(eErr != HAL_ERR_OK)
                    {
                        s_un32RCnt = 0;
                        HAL_ADC_Stop(eId);
                    }

                    if(un32ISRLog == 1)
                    {
                        PRV_EX_ADC_Print((s_un32RCnt-1));
                    }
                }
                break;
            default:
                break;
        }
    }
    else
    {
        if(s_eMode == ADC_MODE_BURST)
        {
            if((un32Event & ADC_EVENT_BURST_CAPTURED)
               && (bSkip == false))
            {
                s_un32RCnt += s_un8SeqCnt;
            }
        }
        else
        {
            if(bSkip == false && un32Event & ADC_EVENT_SINGLE_CAPTURED)
            {
                s_un32RCnt++;
            }
        }
    }

    if(s_un32RCnt < EX_ADC_RBUF_SIZE - 1)
    {
        if(s_eMode != ADC_MODE_MULTIPLE)
        {
            HAL_ADC_Start(eId);
        }
    }
    else
    {
        HAL_ADC_Stop(eId);
        LOG("%s intr stop\n", EX_ADC_LOG_STR);
    }
}
#else
#error "There is no trigger type define."
#endif

static enum debug_cmd_status EX_ADC_Dump(int32_t n32Argc, char *pn8Argv[])
{
    for(int i = 0; i < EX_ADC_RBUF_SIZE; i++)
    {
#if defined (EX_ADC_TRG_TYPE_SINGLE)
        LOG("%s [%d\t:%d]\n", EX_ADC_LOG_STR, i, s_tResult[i].un16Result);
#elif defined (EX_ADC_TRG_TYPE_SHARE)
        if(s_eOps == ADC_OPS_INTR_DMA || s_eOps == ADC_OPS_NMI_DMA)
        {
            LOG("%s [%d\t:%d]\n", EX_ADC_LOG_STR, i, (s_aun16RBuf[i]));
        }
        else
        {
            LOG("%s [%d\t:%d]\n", EX_ADC_LOG_STR, i, s_tResult[i].un16Result);
        }
#elif defined (EX_ADC_TRG_TYPE_INDEPENDENT)
        if(s_eOps == ADC_OPS_INTR_DMA || s_eOps == ADC_OPS_NMI_DMA)
        {
            LOG("%s [%d\t:%d]\n", EX_ADC_LOG_STR, i,(s_aun16RBuf[i] >> 4));
        }
        else
        {
            LOG("%s [%s][%d\t:%d]\t[Ch=%d]\t[TrgInfo=%x]", EX_ADC_LOG_STR, s_tResult[i].bReadDDR == true ? "DDR" : "DRx",
                 i, s_tResult[i].un16Result, s_tResult[i].un8ChInfo, s_tResult[i].un8TrgInfo);
            if(s_eMode == ADC_MODE_MULTIPLE)
            {
                LOG("\t[TrgSrc=%d]\t[TrgSrcSubNum=%d]\n", s_tTCAdcTrgInfo[i].eTrgSrc, s_tTCAdcTrgInfo[i].un8TrgNum);
            }
            else
            {
                LOG("\n");
            }
        }
#else
#error "There is no trigger type define."
#endif

        SystemDelayMS(EX_ADC_DELAY);
    }

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_ADC_Init(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    ADC_ID_e eId = ADC_ID_0;

    eDbgStatus = EX_ADC_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_ADC_Init(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    memset(&s_tTCAdcGroup, 0x00, sizeof(EX_ADC_GROUP_t)*EX_ADC_SEQ_CNT);
    memset(&s_tTCAdcTrgInfo, 0x00, sizeof(EX_ADC_TRG_INFO_t)*EX_ADC_RBUF_SIZE);

#if defined (EX_ADC_TRG_TYPE_INDEPENDENT)
    s_bEnableCmp = false;
#endif

    LOG("%s (%d) %s\n", EX_ADC_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_INIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_ADC_Uninit(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    ADC_ID_e eId = ADC_ID_0;

    eDbgStatus = EX_ADC_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_ADC_Uninit(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_ADC_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_UNINIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_ADC_SetClkConfig(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;
    HAL_ERR_e eErr = HAL_ERR_OK;
    ADC_ID_e eId;
    ADC_CLK_CFG_t tClkCfg; 

    eDbgStatus = EX_ADC_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    memset(&tClkCfg, 0x00, sizeof(ADC_CLK_CFG_t));

    eDbgStatus = EX_ADC_GetClkConfig(n32Argc, pn8Argv, &tClkCfg);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_ADC_SetClkConfig(eId, &tClkCfg);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_ADC_ERR_STR, "ClkCfg", EX_COMM_STR_OPT_MAX);
        }
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_ADC_SetConfig(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus;
    ADC_ID_e eId = ADC_ID_0;
    ADC_CFG_t tCfg;
     
    eDbgStatus = EX_ADC_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    memset(&tCfg, 0x00, sizeof(ADC_CFG_t));

    eDbgStatus = EX_ADC_GetConfig(n32Argc, pn8Argv, &tCfg, &s_eOps);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_ADC_SetConfig(eId, &tCfg);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_ADC_ERR_STR, "Cfg", EX_COMM_STR_OPT_MAX);
        }
        goto err;
    }

    s_tADCContext[eId].eId = eId;

    eErr = HAL_ADC_SetIRQ(eId, s_eOps, EX_ADC_IRQHandler, &s_tADCContext[eId], EX_ADC_IRQ_PRIO);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_ADC_ERR_STR, "IRQ", EX_COMM_STR_OPT_MAX);
        }
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_ADC_SetSeqConfig(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus;
    ADC_ID_e eId = ADC_ID_0;
    ADC_SEQ_TRG_CFG_t tSeqTrgCfg;
     
    eDbgStatus = EX_ADC_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    memset(&tSeqTrgCfg, 0x00, sizeof(ADC_SEQ_TRG_CFG_t));

    eDbgStatus = EX_ADC_GetSeqConfig(n32Argc, pn8Argv, &tSeqTrgCfg);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_ADC_SetSeqConfig(eId, &tSeqTrgCfg);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_ADC_ERR_STR, "SeqConfig", EX_COMM_STR_OPT_MAX);
        }
        goto err;
    }

    s_un32RCnt = 0;

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

#if defined (EX_ADC_TRG_TYPE_INDEPENDENT)
static enum debug_cmd_status EX_ADC_SetCmpConfig(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus;
    ADC_ID_e eId = ADC_ID_0;
    ADC_CMP_CFG_t tCmpCfg;
     
    eDbgStatus = EX_ADC_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    memset(&tCmpCfg, 0x00, sizeof(ADC_CMP_CFG_t));

    eDbgStatus = EX_ADC_GetCmpConfig(n32Argc, pn8Argv, &tCmpCfg);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_ADC_SetCmpConfig(eId, &tCmpCfg);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_ADC_ERR_STR, "CmpCfg", EX_COMM_STR_OPT_MAX);
        }
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}
#endif

#if defined (EX_ADC_OVERSAMPLING)
static enum debug_cmd_status EX_ADC_GetOVSConfig(int32_t n32Argc, char *pn8Argv[], ADC_OVS_CFG_t *ptOvsCfg)
{
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;
    uint8_t un8Arg = 2;

    eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], &ptOvsCfg->bEnable);
    if (eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_ADC_ERR_STR, NULL, EX_COMM_STR_OPT_ENA);
        goto err;
    }

    if(ptOvsCfg->bEnable == true)
    {
        ptOvsCfg->un8Ratio = (uint8_t)atoi(pn8Argv[un8Arg++]);
        ptOvsCfg->un8DataShift = (uint8_t)atoi(pn8Argv[un8Arg++]);
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_ADC_SetOVSConfig(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus;
    ADC_ID_e eId = ADC_ID_0;
    ADC_OVS_CFG_t tOvsCfg;
     
    eDbgStatus = EX_ADC_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    memset(&tOvsCfg, 0x00, sizeof(ADC_OVS_CFG_t));

    eDbgStatus = EX_ADC_GetOVSConfig(n32Argc, pn8Argv, &tOvsCfg);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_ADC_SetOVSConfig(eId, &tOvsCfg);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_ADC_ERR_STR, "OvsCfg", EX_COMM_STR_OPT_MAX);
        }
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}
#endif

static enum debug_cmd_status EX_ADC_Start(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    ADC_ID_e eId = ADC_ID_0;
    ADC_SEQ_TRG_CFG_t tSeqTrgCfg;

    eDbgStatus = EX_ADC_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    memset(s_tResult, 0x00, sizeof(ADC_SEQ_DATA_t)*EX_ADC_RBUF_SIZE);

    s_un32RCnt = 0;

    LOG("%s (%d) %s\n", EX_ADC_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_START]);

    if(s_eOps == ADC_OPS_POLL)
    {
        tSeqTrgCfg.eType = s_eType;
        tSeqTrgCfg.eTrgSrc = ADC_TRG_SRC_ADST;
        tSeqTrgCfg.un8TrgNum = 0;
        eErr = HAL_ADC_SetSeqConfig(eId, &tSeqTrgCfg);
        if(eErr != HAL_ERR_OK)
        {
            goto err;
        }

        for(int i = 0; i < EX_ADC_LOOP; i++)
        {
            eErr = HAL_ADC_Start(eId);
            if(eErr != HAL_ERR_OK)
            {
                goto err;
            }

            eErr = HAL_ADC_SetWaitComplete(eId, 20000);
            if(eErr == HAL_ERR_OK)
            {
                eErr = HAL_ADC_GetData(eId, 0, &s_tResult[0]);
                if(eErr != HAL_ERR_OK)
                {
                    LOG("%s GetData\n", EX_ADC_ERR_STR);
                }
                else
                {
                    LOG("%s [%d:%d] [Ch=%d] [Trg=0x%x]\n", EX_ADC_LOG_STR, i, s_tResult[0].un16Result, s_tResult[0].un8ChInfo, s_tResult[0].un8TrgInfo);
                }
            }
            else
            {
                LOG("%s timeout\n", EX_ADC_LOG_STR);
            }

            eErr = HAL_ADC_Stop(eId);
            if(eErr != HAL_ERR_OK)
            {
                goto err;
            }

            if(i == 300)
            {
                if (s_eType == ADC_TRG_TYPE_SINGLE)
                    tSeqTrgCfg.utCfg.tSgl.un8ChNum = 1;
                else if (s_eType == ADC_TRG_TYPE_SHARE)
                    tSeqTrgCfg.utCfg.tShe.un32ChNum = BIT(1);
                else if (s_eType == ADC_TRG_TYPE_INDEPENDENT)
                    tSeqTrgCfg.utCfg.tInd.un8ChNum = 1;
                
                eErr = HAL_ADC_SetSeqConfig(eId, &tSeqTrgCfg);
                if(eErr != HAL_ERR_OK)
                {
                    goto err;
                }
                LOG("%s change chan port to 1\n", EX_ADC_LOG_STR);
            }

            if(i == 600)
            {
                if (s_eType == ADC_TRG_TYPE_SINGLE)
                    tSeqTrgCfg.utCfg.tSgl.un8ChNum = 2;
                else if (s_eType == ADC_TRG_TYPE_SHARE)
                    tSeqTrgCfg.utCfg.tShe.un32ChNum = BIT(2);
                else if (s_eType == ADC_TRG_TYPE_INDEPENDENT)
                    tSeqTrgCfg.utCfg.tInd.un8ChNum = 2;
                eErr = HAL_ADC_SetSeqConfig(eId, &tSeqTrgCfg);
                if(eErr != HAL_ERR_OK)
                {
                    goto err;
                }
                LOG("%s change chan port to 2\n", EX_ADC_LOG_STR);
            }

            SystemDelayMS(EX_ADC_DELAY);

        }
    }
    else
    {
        if(s_eOps == ADC_OPS_INTR_DMA || s_eOps == ADC_OPS_NMI_DMA)
        {
#if defined (EX_ADC_TRG_TYPE_SHARE)
            eErr = HAL_ADC_SetDMA(eId, (uint32_t)&s_aun16RBuf, s_un8ChCnt);
#else
            eErr = HAL_ADC_SetDMA(eId, (uint32_t)&s_aun16RBuf, (uint32_t)(sizeof(s_aun16RBuf)/sizeof(uint16_t)));
#endif
            if(eErr != HAL_ERR_OK)
            {
                goto err;
            }
        }
        eErr = HAL_ADC_Start(eId);
        if(eErr != HAL_ERR_OK)
        {
            goto err;
        }

    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_ADC_Stop(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    ADC_ID_e eId = ADC_ID_0;

    eDbgStatus = EX_ADC_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_ADC_Stop(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_ADC_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_STOP]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_ADC_SetLog(int32_t n32Argc, char *pn8Argv[])
{
    if((strncmp(pn8Argv[1],"on",2) == 0))
    {
        un32ISRLog = 1;
    }
    else
    {
        un32ISRLog = 0;
    }

    LOG("%s ISR Log %s\n", EX_ADC_LOG_STR, (un32ISRLog == 1 ? "on":"off"));

    return DEBUG_CMD_SUCCESS;
}

static const struct debug_cmd s_tEX_ADC_CMD[] =
{
    {"ADC", "h", EX_ADC_Help, "help"},
    {"ADC", "init", EX_ADC_Init, ""},
    {"ADC", "uninit", EX_ADC_Uninit, ""},
    {"ADC", "clk", EX_ADC_SetClkConfig, ""},
    {"ADC", "config", EX_ADC_SetConfig, ""},
    {"ADC", "seq", EX_ADC_SetSeqConfig, ""},
#if defined (EX_ADC_TRG_TYPE_INDEPENDENT)
    {"ADC", "cmp", EX_ADC_SetCmpConfig, ""},
#endif
#if defined (EX_ADC_OVERSAMPLING)
    {"ADC", "ovs", EX_ADC_SetOVSConfig, ""},
#endif
    {"ADC", "start", EX_ADC_Start, ""},
    {"ADC", "stop", EX_ADC_Stop, ""},
    {"ADC", "dump", EX_ADC_Dump, ""},
    {"ADC", "log", EX_ADC_SetLog, ""}
};

/**********************************************************************
 * @brief		EX_ADC
 * @param[in]	None
 * @return	None
 **********************************************************************/
void EX_ADC(void)
{
    /* Add TC commands */
    debug_cmd_init(s_tEX_ADC_CMD, DEBUG_CMD_LIST_COUNT(s_tEX_ADC_CMD));
}

#endif /* ADC_TC */

/* --------------------------------- End Of File ------------------------------ */
