/**
 *******************************************************************************
 * @file        qei.c
 * @author      ABOV R&D Division
 * @brief       QEI Example Code
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

#if defined(QEI_TC)
#include "abov_config.h"
#include "ex_configs.h"
#include "ex_common.h"
#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "hal_scu.h"
#include "hal_scu_clk.h"
#include "hal_pcu.h"
#include "hal_qei.h"

#if !defined(_QEI)
#error "This chipset did not support this example."
#endif

#define EX_QEI_STR              "QEI"
#define EX_QEI_LOG_STR          "QEI :"
#define EX_QEI_ERR_STR          "[E]QEI :"
#define EX_QEI_IRQ_PRIO         10
#define EX_QEI_RPM_SEC          60
#define EX_QEI_PPR              2048
#define EX_QEI_RPM_ACC          10
#define EX_QEI_MAX_NUM          (CONFIG_QEI_MAX_COUNT - 1)

extern const char *pcCommStr[];
extern const char *pcCmdStr[];
extern const char *pcValStr[];
extern const char *pcOptStr[];

static uint16_t s_un16Timer = 0;
static uint8_t s_un8Edge = 0;

static bool s_bISRLog = true;
static bool s_bRPMLog = false;
static uint32_t s_un32LogDispCnt = 0;
static uint32_t s_un32LogCurCnt = 0;

static QEI_Context_t s_tQEIContext[CONFIG_QEI_MAX_COUNT];

static enum debug_cmd_status EX_QEI_Help(int32_t n32Argc, char *pn8Argv[])
{
    uint32_t un32ShowOpt = 0;
    EX_COMM_STR_OPT_e eOpt[2];

    EX_COMMON_SetShowModuleInfo(EX_QEI_STR, CONFIG_QEI_MAX_COUNT);

    un32ShowOpt = EX_COMMON_GetShowOpt(pn8Argv[1]);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_INIT, NULL, EX_QEI_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_UNINIT, NULL, EX_QEI_MAX_NUM, eOpt, 0, NULL);

    eOpt[0] = EX_COMM_STR_OPT_OPS; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "cfg", EX_QEI_MAX_NUM, eOpt, 1, "[intr] [sig] [cap] [rst] [gate] [max] [-swap] [-pdir] [-idir] [-inverti]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_OPS, EX_COMM_STR_VAL_OPS, "/n(nmi)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "intr: 0~N (16bit hexa)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "    : 0x01(index pulse rise edge)/0x02(dir change)/0x04(phase error)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "    : 0x08(rise/fall ph-a edge)/0x10(pos 0 cmp)/0x20(pos 1 cmp)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "    : 0x40(pos 2 cmp)/0x80(pos cnt max)/0x100(idx cmp)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "    : 0x200(velocity zero)/0x400(velocity under compare)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "sig: q(quadrature)/c(clock & direction)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "cap: a(pha edge)/b(pha & phb edge)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "rst: m(maximum reset)/i(index reset)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "gate: d(disable)/1(ph-a 1 & ph-b 0)/2(ph-A 1 & ph-b 1)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "      /3 (ph-a 0 & ph-b 1)/4(ph-a 0 & ph-b 0)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "max: 0~N(32bit hexa)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-swap: swap pha and phb output");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-pdir: pos count direction changed by direction status");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-idir: idx count direction changed by direction status");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-inverti: invert index pulse");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "cmpcfg", EX_QEI_MAX_NUM, eOpt, 0, "[cnter] [intr] [val]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_OPS, EX_COMM_STR_VAL_OPS, "cnter: p(position)/i(index)");
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "p:");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_NUM, EX_COMM_STR_VAL_N_NUM, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "intr: e(en)/d(dis)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_VAL, EX_COMM_STR_VAL_N_NUM, "(32bit hexa)");
    }
    eOpt[0] = EX_COMM_STR_OPT_ENA; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "velcfg", EX_QEI_MAX_NUM, eOpt, 1, "[intr] [rel] [tim] [cmp]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_ENA, EX_COMM_STR_VAL_ENDIS, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "intr: e(en)/d(dis)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "rel: 0~N(16bit hexa) - reload value");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "tim: 0~N(16bit hexa) - timer initial value");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "cmp: 0~N(16bit hexa) - compare value");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_START, NULL, EX_QEI_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_STOP, NULL, EX_QEI_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "info", EX_QEI_MAX_NUM, eOpt, 0, "- display some information");
    eOpt[0] = EX_COMM_STR_OPT_MODE; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_LOG, NULL, EX_QEI_MAX_NUM, eOpt, 0, "- display some information");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MODE, EX_COMM_STR_VAL_MAX, "on(log on)/rpm(only rpm log)/off");
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "on or rpm:");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_CNT, EX_COMM_STR_VAL_N_NUM, "(decimal) - log display delay count");
    }
    LOG("\n");

    return DEBUG_CMD_SUCCESS;
}

static void PRV_EX_QEI_EventLog(QEI_ID_e eId, uint32_t un32Event)
{
    if(un32Event & QEI_EVENT_IDX_PULSE)
    {
        LOG("%s (%d) idx pulse evt fired", EX_QEI_LOG_STR, eId);
    }

    if(un32Event & QEI_EVENT_PHASE_ERROR)
    {
        LOG("%s (%d) pha err evt fired", EX_QEI_LOG_STR, eId);
    }

    if(un32Event & QEI_EVENT_CLOCK_PULSE)
    {
        LOG("%s (%d) clk pulse gen evt fired", EX_QEI_LOG_STR, eId);
    }

    if(un32Event & QEI_EVENT_IDX_MATCH)
    {
        LOG("%s (%d) idx cnt match cmp value evt fired", EX_QEI_LOG_STR, eId);
    }

    if(un32Event & QEI_EVENT_COMPARE_MATCH_0)
    {
        LOG("%s (%d) cmp match 0 evt fired", EX_QEI_LOG_STR, eId);
    }

    if(un32Event & QEI_EVENT_COMPARE_MATCH_1)
    {
        LOG("%s (%d) cmp match 1 evt fired", EX_QEI_LOG_STR, eId);
    }

    if(un32Event & QEI_EVENT_COMPARE_MATCH_2)
    {
        LOG("%s (%d) cmp match 2 evt fired", EX_QEI_LOG_STR, eId);
    }

    if(un32Event & QEI_EVENT_MAXIMUM_MATCH)
    {
        LOG("%s (%d) pos cnt reach max evt fired", EX_QEI_LOG_STR, eId);
    }

    if(un32Event & QEI_EVENT_VELOCITY_ZERO)
    {
        LOG("%s (%d) vel timer reach zero evt fired", EX_QEI_LOG_STR, eId);
    }

    if(un32Event & QEI_EVENT_VELOCITY_UNDER_COMPARE)
    {
        LOG("%s (%d) vel cap val small cmp val evt fired", EX_QEI_LOG_STR, eId);
    }

}

static void EX_QEI_IRQHandler(uint32_t un32Event, void *pContext)
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    QEI_ID_e eId = QEI_ID_0;
    QEI_DIR_e eDir = QEI_DIR_MAX;
    QEI_Context_t *ptContext = (QEI_Context_t *)pContext;
    uint32_t un32VelCap = 0;
    uint32_t un32Rpm = 0;

    eId = ptContext->eId;

    if(un32Event & QEI_EVENT_VELOCITY_ZERO)
    {
        if(s_bRPMLog == true && s_un32LogDispCnt == s_un32LogCurCnt)
        {
            eErr = HAL_QEI_GetCount(eId, QEI_COUNTER_VELOCITY, &un32VelCap);
            if(eErr != HAL_ERR_OK)
            {
                LOG("%s (%d) GetCount\n", EX_QEI_ERR_STR, eId);
            }
            un32Rpm = (32000000) / (s_un16Timer * s_un8Edge);
            un32Rpm = (un32Rpm * un32VelCap * EX_QEI_RPM_SEC) / EX_QEI_PPR; 
            LOG("RPM (%d)\n",un32Rpm);
        }
    }

    if(s_bISRLog == true && s_un32LogCurCnt >= s_un32LogDispCnt)
    {
        PRV_EX_QEI_EventLog(eId, un32Event);
    }

    if(un32Event & QEI_EVENT_DIR_CHANGE)
    {
        eErr = HAL_QEI_GetDirection(eId, &eDir);
        if(eErr != HAL_ERR_OK)
        {
            LOG("%s (%d) GetDirection\n", EX_QEI_ERR_STR, eId);
        }
        if(s_bISRLog == true || s_bRPMLog == true)
        {
            LOG("%s (%d) dir change evt fired (%s) !!!\n", EX_QEI_LOG_STR, eId,
                   (eDir == QEI_DIR_REVERSE ? "reverse" : "forward"));
        }
    }

    s_un32LogCurCnt++;
    if(s_un32LogCurCnt > s_un32LogDispCnt)
    {
        s_un32LogCurCnt = 0;
    }
}

static enum debug_cmd_status PRV_EX_QEI_GetEnableConfig(uint8_t n8Argc, char *pn8Argv[], bool *pbEnable)
{
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;
    uint8_t un8Arg = n8Argc;

    switch (pn8Argv[un8Arg++][0])
    {
        case 'e':
            *pbEnable = true;
            break;
        case 'd':
            *pbEnable = false;
            break;
        default:
            eDbgStatus = DEBUG_CMD_INVALID;
            LOG("(ERR) QEI : en no proper argument.\n");
            break;
    }

    return eDbgStatus;
}

static enum debug_cmd_status PRV_EX_QEI_GetConfig(int32_t n32Argc, char *pn8Argv[], QEI_OPS_e *peOps, QEI_CFG_t *ptCfg)
{
    uint8_t un8Arg = 2;
    uint32_t un32Data = 0;

    if (pn8Argv[un8Arg][0] == 'p')
        *peOps = QEI_OPS_POLL;
    else if (pn8Argv[un8Arg][0] == 'i')
        *peOps = QEI_OPS_INTR;
    else if (pn8Argv[un8Arg][0] == 'n')
        *peOps = QEI_OPS_NMI;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_QEI_ERR_STR, NULL, EX_COMM_STR_OPT_OPS);
        goto err;
    }

    un8Arg++;
    sscanf(pn8Argv[un8Arg++], "%X", &un32Data);
    ptCfg->un16IntrEnable = (uint16_t)un32Data;

    switch (pn8Argv[un8Arg++][0])
    {
        case 'q':
            ptCfg->eSigMode = QEI_SIG_MODE_QUADRATURE;
            break;
        case 'c':
            ptCfg->eSigMode = QEI_SIG_MODE_CLOCK_DIRECTION;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_QEI_ERR_STR, "[sig]", EX_COMM_STR_OPT_MAX);
            goto err;
    }

    switch (pn8Argv[un8Arg++][0])
    {
        case 'a':
            ptCfg->eCapCnt = QEI_CAP_COUNT_PHA_EDGE;
            s_un8Edge = 2;
            break;
        case 'b':
            ptCfg->eCapCnt = QEI_CAP_COUNT_BOTH_EDGE;
            s_un8Edge = 4;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_QEI_ERR_STR, "[cap]", EX_COMM_STR_OPT_MAX);
            goto err;
    }

    switch (pn8Argv[un8Arg++][0])
    {
        case 'm':
            ptCfg->ePosRst = QEI_POS_CNTER_RST_MAXIMUM;
            break;
        case 'i':
            ptCfg->ePosRst = QEI_POS_CNTER_RST_INDEX;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_QEI_ERR_STR, "[rst]", EX_COMM_STR_OPT_MAX);
            goto err;
    }

    if (pn8Argv[un8Arg][0] == 'd')
        ptCfg->ePosRstIdx = QEI_POS_CNTER_RST_IDX_DISABLE;
    else if (pn8Argv[un8Arg][0] == '1')
        ptCfg->ePosRstIdx = QEI_POS_CNTER_RST_IDX_1_0;
    else if (pn8Argv[un8Arg][0] == '2')
        ptCfg->ePosRstIdx = QEI_POS_CNTER_RST_IDX_1_1;
    else if (pn8Argv[un8Arg][0] == '3')
        ptCfg->ePosRstIdx = QEI_POS_CNTER_RST_IDX_0_1;
    else if (pn8Argv[un8Arg][0] == '4')
        ptCfg->ePosRstIdx = QEI_POS_CNTER_RST_IDX_0_0;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_QEI_ERR_STR, "[gate]", EX_COMM_STR_OPT_MAX);
    }

    un8Arg++;
    sscanf(pn8Argv[un8Arg++], "%X", &un32Data);
    ptCfg->un32PosMaxValue = (uint32_t)un32Data;

    if(strncmp(pn8Argv[un8Arg], "-swap", 5) == 0)
    {
        un8Arg++;
        ptCfg->bSwap = true;
    }

    if(strncmp(pn8Argv[un8Arg], "-pdir", 5) == 0)
    {
        un8Arg++;
        ptCfg->bPosCntDir = true;
    }

    if(strncmp(pn8Argv[un8Arg], "-idir", 5) == 0)
    {
        un8Arg++;
        ptCfg->bIdxCntDir = true;
    }

    if(strncmp(pn8Argv[un8Arg], "-inverti", 8) == 0)
    {
        un8Arg++;
        ptCfg->bIdxPulseInverted = true;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status PRV_EX_QEI_GetVelConfig(int32_t n32Argc, char *pn8Argv[], QEI_VEL_CFG_t *ptCfg)
{
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;
    uint8_t un8Arg = 2;
    uint32_t un32Data = 0;

    eDbgStatus = PRV_EX_QEI_GetEnableConfig(un8Arg++, pn8Argv, &ptCfg->bEnable);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eDbgStatus = PRV_EX_QEI_GetEnableConfig(un8Arg++, pn8Argv, &ptCfg->bIntrEnable);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    sscanf(pn8Argv[un8Arg++], "%X", &un32Data);
    ptCfg->un16Reload = (uint16_t)un32Data;
    sscanf(pn8Argv[un8Arg++], "%X", &un32Data);
    ptCfg->un16Timer = (uint16_t)un32Data;
    sscanf(pn8Argv[un8Arg++], "%X", &un32Data);
    ptCfg->un16Compare = (uint16_t)un32Data;

    s_un16Timer = ptCfg->un16Timer;

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status PRV_EX_QEI_GetCmpConfig(int32_t n32Argc, char *pn8Argv[], QEI_COUNTER_e *peCounter ,QEI_CMP_CFG_t *ptCfg)
{
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;
    uint8_t un8Arg = 2;
    uint32_t un32Data = 0;

    switch (pn8Argv[un8Arg++][0])
    {
        case 'p':
            *peCounter = QEI_COUNTER_POSITION;
            ptCfg->un8Num = (uint8_t)atoi(pn8Argv[un8Arg++]);
            break;
        case 'i':
            *peCounter = QEI_COUNTER_INDEX;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_QEI_ERR_STR, "[cnter]", EX_COMM_STR_OPT_MAX);
            goto err;
    }

    eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], &ptCfg->bIntrEnable);
    if (eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_QEI_ERR_STR, "[intr]", EX_COMM_STR_OPT_MAX);
        goto err;
    }

    sscanf(pn8Argv[un8Arg++], "%X", &un32Data);
    ptCfg->un32Compare = (uint16_t)un32Data;

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status PRV_EX_QEI_GetId(int32_t n32Argc, char *pn8Argv[], QEI_ID_e *peId)
{
    uint8_t un8Data = 0;
    if(n32Argc == 1)
    {
        EX_COMMON_SetShowModuleLog(EX_QEI_ERR_STR, (char *)pcOptStr[EX_COMM_STR_OPT_ARG], EX_COMM_STR_OPT_MAX);
        return DEBUG_CMD_INVALID;
    }

    un8Data = atoi(pn8Argv[1]); 
    if(un8Data >= CONFIG_QEI_MAX_COUNT)
    {
        LOG("%s Max Chan %d\n", EX_QEI_ERR_STR, CONFIG_QEI_MAX_COUNT);
        return DEBUG_CMD_INVALID;
    }

    *peId = (QEI_ID_e)un8Data;

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_QEI_Init(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    QEI_ID_e eId;

    eDbgStatus = PRV_EX_QEI_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_QEI_Init(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_QEI_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_INIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_QEI_Uninit(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    QEI_ID_e eId;

    eDbgStatus = PRV_EX_QEI_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_QEI_Uninit(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_QEI_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_UNINIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_QEI_SetConfig(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    QEI_ID_e eId;
    QEI_CFG_t tQEICfg;
    QEI_OPS_e eOps;

    eDbgStatus = PRV_EX_QEI_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    memset(&tQEICfg, 0x00, sizeof(QEI_CFG_t));

    eDbgStatus = PRV_EX_QEI_GetConfig(n32Argc, pn8Argv, &eOps, &tQEICfg);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_QEI_SetConfig(eId, &tQEICfg);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    s_tQEIContext[eId].eId = eId;

    eErr = HAL_QEI_SetIRQ(eId, eOps, EX_QEI_IRQHandler, &s_tQEIContext[eId], EX_QEI_IRQ_PRIO);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;

}

static enum debug_cmd_status EX_QEI_SetVelConfig(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    QEI_ID_e eId;
    QEI_VEL_CFG_t tVelCfg;

    eDbgStatus = PRV_EX_QEI_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    memset(&tVelCfg, 0x00, sizeof(QEI_VEL_CFG_t));

    eDbgStatus = PRV_EX_QEI_GetVelConfig(n32Argc, pn8Argv, &tVelCfg);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_QEI_SetVelConfig(eId, &tVelCfg);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_QEI_SetCmpConfig(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    QEI_ID_e eId;
    QEI_CMP_CFG_t tCmpCfg;
    QEI_COUNTER_e eCounter;

    eDbgStatus = PRV_EX_QEI_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    memset(&tCmpCfg, 0x00, sizeof(QEI_CMP_CFG_t));

    eDbgStatus = PRV_EX_QEI_GetCmpConfig(n32Argc, pn8Argv, &eCounter, &tCmpCfg);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_QEI_SetCompare(eId, eCounter, &tCmpCfg);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_QEI_Start(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    QEI_ID_e eId;

    eDbgStatus = PRV_EX_QEI_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_QEI_Start(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_QEI_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_START]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_QEI_Stop(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    QEI_ID_e eId;

    eDbgStatus = PRV_EX_QEI_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_QEI_Stop(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_QEI_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_STOP]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_QEI_GetInfo(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    QEI_ID_e eId;
    QEI_DIR_e eDir;
    uint32_t un32PosCnt, un32IdxCnt;

    eDbgStatus = PRV_EX_QEI_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_QEI_GetDirection(eId, &eDir);
    if(eErr != HAL_ERR_OK)
    {
        LOG("%s (%d) GetDirection\n", EX_QEI_ERR_STR, eId);
    }

    eErr = HAL_QEI_GetCount(eId, QEI_COUNTER_POSITION, &un32PosCnt);
    if(eErr != HAL_ERR_OK)
    {
        LOG("%s (%d) GetCount\n", EX_QEI_ERR_STR, eId);
    }

    eErr = HAL_QEI_GetCount(eId, QEI_COUNTER_INDEX, &un32IdxCnt);
    if(eErr != HAL_ERR_OK)
    {
        LOG("%s (%d) GetCount\n", EX_QEI_ERR_STR, eId);
    }

    LOG("%s Info\n", EX_QEI_LOG_STR);
    LOG("%s Dir     : %s\n", EX_QEI_LOG_STR, (eDir == QEI_DIR_REVERSE ? "rev" : "for"));
    LOG("%s Pos cnt : %d\n", EX_QEI_LOG_STR, un32PosCnt);
    LOG("%s Idx cnt : %d\n", EX_QEI_LOG_STR, un32IdxCnt);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_QEI_SetLog(int32_t n32Argc, char *pn8Argv[])
{
    if((strncmp(pn8Argv[1],"on",2) == 0))
    {
        s_bISRLog = true;
        s_bRPMLog = false;
    }
    else if((strncmp(pn8Argv[1], "rpm", 3) == 0))
    {
        s_bISRLog = false;
        s_bRPMLog = true;
    }
    else
    {
        s_bISRLog = false;
        s_bRPMLog = false;
    }

    s_un32LogCurCnt = 0;

    if(s_bISRLog == true || s_bRPMLog == true)
    {
        if(n32Argc == 3)
        {
            s_un32LogDispCnt = (uint32_t)atoi(pn8Argv[2]);
        }
    }

    LOG("%s ISR Log %s.\n", EX_QEI_LOG_STR, (s_bISRLog == true ? "on": s_bRPMLog == true ? "rpm" : "off"));

    return DEBUG_CMD_SUCCESS;
}

static const struct debug_cmd s_tEX_QEI_CMD[] =
{
    {"QEI", "h", EX_QEI_Help, "help"},
    {"QEI", "init", EX_QEI_Init, ""},
    {"QEI", "uninit", EX_QEI_Uninit, ""},
    {"QEI", "cfg", EX_QEI_SetConfig, ""},
    {"QEI", "velcfg", EX_QEI_SetVelConfig, ""},
    {"QEI", "cmpcfg", EX_QEI_SetCmpConfig, ""},
    {"QEI", "start", EX_QEI_Start, ""},
    {"QEI", "stop", EX_QEI_Stop, ""},
    {"QEI", "info", EX_QEI_GetInfo, ""},
    {"QEI", "log", EX_QEI_SetLog, ""}
};

/**********************************************************************
 * @brief		Main program
 * @param[in]	None
 * @return	None
 **********************************************************************/
void EX_QEI(void)
{
    /* Add EX commands */
    debug_cmd_init(s_tEX_QEI_CMD,DEBUG_CMD_LIST_COUNT(s_tEX_QEI_CMD));
}

#endif
/* --------------------------------- End Of File ------------------------------ */
