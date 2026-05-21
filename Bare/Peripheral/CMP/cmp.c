/**
 *******************************************************************************
 * @file        cmp.c
 * @author      ABOV R&D Division
 * @brief       CMP Example Code
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

#if defined(CMP_TC)
#include "abov_config.h"
#include "ex_configs.h"
#include "ex_common.h"
#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "hal_scu.h"
#include "hal_scu_clk.h"
#include "hal_pcu.h"
#include "hal_cmp.h"

#if !defined(_CMP)
#error "This chipset did not support this example."
#endif

#define EX_CMP_STR "CMP"
#define EX_CMP_LOG_STR "CMP :"
#define EX_CMP_ERR_STR "[E]CMP :"
#define EX_CMP_IRQ_PRIO 3
#define EX_CMP_MAX_NUM (CONFIG_CMP_MAX_COUNT - 1)

extern const char *pcCommStr[];
extern const char *pcCmdStr[];
extern const char *pcValStr[];
extern const char *pcOptStr[];

extern uint32_t SystemCoreClock;
static CMP_Context_t s_tCMPContext[CONFIG_CMP_MAX_COUNT];
static CMP_OPS_e s_eOps = CMP_OPS_POLL;
static bool s_bISRLog = false;
static uint32_t s_un32LogDispCnt = 0;
static uint32_t s_un32LogCurCnt = 0;

static enum debug_cmd_status EX_CMP_Help(int32_t n32Argc, char *pn8Argv[])
{
    uint32_t un32ShowOpt = 0;
    EX_COMM_STR_OPT_e eOpt[7];

    EX_COMMON_SetShowModuleInfo(EX_CMP_STR, CONFIG_CMP_MAX_COUNT);

    un32ShowOpt = EX_COMMON_GetShowOpt(pn8Argv[1]);

    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_INIT, NULL, EX_CMP_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_UNINIT, NULL, EX_CMP_MAX_NUM, eOpt, 0, NULL);

    eOpt[0] = EX_COMM_STR_OPT_OPS; 
    eOpt[1] = EX_COMM_STR_OPT_SRC; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CONFIG, NULL, EX_CMP_MAX_NUM, eOpt, 2, "[ref] [hyss] [trg] [-dbc] [-ext]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_OPS, EX_COMM_STR_VAL_OPS, "/n(nmi)");
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "i: [msk] (for A31G22x)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MASK, EX_COMM_STR_VAL_MASK, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_SRC, EX_COMM_STR_VAL_MAX, "0,1,2,3,4,5,6,7");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "ref: 0,1,2,3,4,5,6,7");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "hyss: o(off)/l(low)/m(medium)/h(high)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "trg: l(level)/r(rise)/f(fall)/b(both edge)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-dbc: [clkdiv] [shift] [lsi]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "clkdiv: 0~N");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "shift: 0~N");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "lsi: e(en)/d(dis)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-ext: [pwr] [win] [intref]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "pwr: u(ultra low)/l(low)/m(medium)/h(high)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "win: e(en)/d(dis)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "intref: e(en)/d(dis)");
#if defined (EX_CMP_INTERNAL_REFERENCE_LEVEL)
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "intlvl: 0~N");
#endif
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_START, NULL, EX_CMP_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_STOP, NULL, EX_CMP_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_LOG, NULL, -1, eOpt, 0, "on [cnt] / off");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_CNT, EX_COMM_STR_VAL_N_NUM, NULL);
    }
    LOG("\n");

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_CMP_GetSrc(uint8_t un8Argv, char *pn8Argv[], CMP_SRC_e *peSrc)
{
    if (pn8Argv[un8Argv][0] == '0')
        *peSrc = CMP_SRC_0;
    else if (pn8Argv[un8Argv][0] == '1')
        *peSrc = CMP_SRC_1;
    else if (pn8Argv[un8Argv][0] == '2')
        *peSrc = CMP_SRC_2;
    else if (pn8Argv[un8Argv][0] == '3')
        *peSrc = CMP_SRC_3;
    else if (pn8Argv[un8Argv][0] == '4')
        *peSrc = CMP_SRC_4;
    else if (pn8Argv[un8Argv][0] == '5')
        *peSrc = CMP_SRC_5;
    else if (pn8Argv[un8Argv][0] == '6')
        *peSrc = CMP_SRC_6;
    else if (pn8Argv[un8Argv][0] == '7')
        *peSrc = CMP_SRC_7;
    else
    {
        return DEBUG_CMD_INVALID;
    }

    return DEBUG_CMD_SUCCESS;
}
static enum debug_cmd_status EX_CMP_GetId(int32_t n32Argc, char *pn8Argv[], CMP_ID_e *peId)
{
    uint8_t un8Data = 0;
    if(n32Argc == 1)
    {
        EX_COMMON_SetShowModuleLog(EX_CMP_ERR_STR, (char *)pcOptStr[EX_COMM_STR_OPT_ARG], EX_COMM_STR_OPT_MAX);
        return DEBUG_CMD_INVALID;
    }

    un8Data = atoi(pn8Argv[1]); 
    if(un8Data >= CONFIG_CMP_MAX_COUNT)
    {
        LOG("%s Max Chan %d\n", EX_CMP_ERR_STR, CONFIG_CMP_MAX_COUNT);
        return DEBUG_CMD_INVALID;
    }

    *peId = (CMP_ID_e)un8Data;

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_CMP_GetConfig(int32_t n32Argc, char *pn8Argv[], CMP_CFG_t *ptCfg, CMP_OPS_e *peOps, bool *pbNonMask)
{
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;
    uint8_t un8Arg = 2;

    if (pn8Argv[un8Arg][0] == 'p')
        *peOps = CMP_OPS_POLL;
    else if (pn8Argv[un8Arg][0] == 'i')
        *peOps = CMP_OPS_INTR;
    else if (pn8Argv[un8Arg][0] == 'n')
        *peOps = CMP_OPS_NMI;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_CMP_ERR_STR, NULL, EX_COMM_STR_OPT_OPS);
        goto err;
    }

    un8Arg++;
    if(*peOps == CMP_OPS_INTR)
    {
        switch (pn8Argv[un8Arg++][0])
        {
            case 'n':
                *pbNonMask = true;
                break;
            case 'm':
                *pbNonMask = false;
                break;
            default:
                eDbgStatus = DEBUG_CMD_INVALID;
                EX_COMMON_SetShowModuleLog(EX_CMP_ERR_STR, "ops", EX_COMM_STR_OPT_MASK);
                break;
        }
    }

    eDbgStatus = EX_CMP_GetSrc(un8Arg++, pn8Argv, &ptCfg->eInSrc);
    if (eDbgStatus == DEBUG_CMD_INVALID)
    {
        EX_COMMON_SetShowModuleLog(EX_CMP_ERR_STR, NULL, EX_COMM_STR_OPT_SRC);
        goto err;
    }

    eDbgStatus = EX_CMP_GetSrc(un8Arg++, pn8Argv, &ptCfg->eRefSrc);
    if (eDbgStatus == DEBUG_CMD_INVALID)
    {
        EX_COMMON_SetShowModuleLog(EX_CMP_ERR_STR, "[ref]", EX_COMM_STR_OPT_MAX);
        goto err;
    }

    if (pn8Argv[un8Arg][0] == 'o')
        ptCfg->eHyss = CMP_HYSS_OFF;
    else if (pn8Argv[un8Arg][0] == 'l')
        ptCfg->eHyss = CMP_HYSS_LOW;
    else if (pn8Argv[un8Arg][0] == 'm')
        ptCfg->eHyss = CMP_HYSS_MEDIUM;
    else if (pn8Argv[un8Arg][0] == 'h')
        ptCfg->eHyss = CMP_HYSS_HIGH;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_CMP_ERR_STR, "[hyss]", EX_COMM_STR_OPT_MAX);
        goto err;
    }

    un8Arg++;
    if (pn8Argv[un8Arg][0] == 'l')
        ptCfg->eIntrTrg = CMP_INTR_TRG_LEVEL;
    else if (pn8Argv[un8Arg][0] == 'r')
        ptCfg->eIntrTrg = CMP_INTR_TRG_RISING;
    else if (pn8Argv[un8Arg][0] == 'f')
        ptCfg->eIntrTrg = CMP_INTR_TRG_FALLING;
    else if (pn8Argv[un8Arg][0] == 'b')
        ptCfg->eIntrTrg = CMP_INTR_TRG_BOTH_EDGE;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_CMP_ERR_STR, "[trg]", EX_COMM_STR_OPT_MAX);
        goto err;
    }

    un8Arg++;
    if(strncmp(pn8Argv[un8Arg], "-dbc", 4) == 0)
    {
        un8Arg++;
        ptCfg->tDbc.bEnable = true;
        ptCfg->tDbc.un16ClkDiv = (uint16_t)atoi(pn8Argv[un8Arg++]);
        ptCfg->tDbc.un8Shift = (uint8_t)atoi(pn8Argv[un8Arg++]);
        eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], &ptCfg->tDbc.bLSIClk);
        if (eDbgStatus != DEBUG_CMD_SUCCESS)
        {
            EX_COMMON_SetShowModuleLog(EX_CMP_ERR_STR, "-dbc[lsi]", EX_COMM_STR_OPT_ENA);
            goto err;
        }
    }
    else
    {
        ptCfg->tDbc.bEnable = false;
    }

    if(strncmp(pn8Argv[un8Arg], "-ext", 4) == 0)
    {
        un8Arg++;
        if (pn8Argv[un8Arg][0] == 'u')
            ptCfg->tExt.ePwr = CMP_PWR_ULTRALOW;
        else if (pn8Argv[un8Arg][0] == 'l')
            ptCfg->tExt.ePwr = CMP_PWR_LOW;
        else if (pn8Argv[un8Arg][0] == 'm')
            ptCfg->tExt.ePwr = CMP_PWR_MEDIUM;
        else if (pn8Argv[un8Arg][0] == 'h')
            ptCfg->tExt.ePwr = CMP_PWR_HIGH;
        else
        {
            EX_COMMON_SetShowModuleLog(EX_CMP_ERR_STR, "-ext[pwr]", EX_COMM_STR_OPT_ENA);
            goto err;
        }

        un8Arg++;
        eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg][0], &ptCfg->tExt.bWindowEn);
        if (eDbgStatus != DEBUG_CMD_SUCCESS)
        {
            EX_COMMON_SetShowModuleLog(EX_CMP_ERR_STR, "-ext[win]", EX_COMM_STR_OPT_ENA);
            goto err;
        }

        un8Arg++;
        eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg][0], &ptCfg->tExt.bIntRefEn);
        if (eDbgStatus != DEBUG_CMD_SUCCESS)
        {
            EX_COMMON_SetShowModuleLog(EX_CMP_ERR_STR, "-ext[intref]", EX_COMM_STR_OPT_ENA);
            goto err;
        }

#if defined (EX_CMP_INTERNAL_REFERENCE_LEVEL)
        un8Arg++;
        ptCfg->tExt.un8IntRefVLvl = (uint8_t)atoi(pn8Argv[un8Arg++]);
#endif

    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static void EX_CMP_IRQHandler(uint32_t un32Event, void *pContext)
{

    if(un32Event & CMP_EVENT_LEVEL)
    {
        if(s_bISRLog == true && s_un32LogDispCnt == s_un32LogCurCnt)
        {
            LOG("%s (%d) L evt fired\n", EX_CMP_LOG_STR, ((CMP_Context_t *)pContext)->eId);
            s_un32LogCurCnt = 0;
        }
    }

    if(un32Event & CMP_EVENT_RISING)
    {
        if(s_bISRLog == true && s_un32LogDispCnt == s_un32LogCurCnt)
        {
            LOG("%s (%d) R evt fired\n", EX_CMP_LOG_STR, ((CMP_Context_t *)pContext)->eId);
            s_un32LogCurCnt = 0;
        }
    }

    if(un32Event & CMP_EVENT_FALLING)
    {
        if(s_bISRLog == true && s_un32LogDispCnt == s_un32LogCurCnt)
        {
            LOG("%s (%d) F evt fired\n", EX_CMP_LOG_STR, ((CMP_Context_t *)pContext)->eId);
            s_un32LogCurCnt = 0;
        }
    }

    if(un32Event & CMP_EVENT_BOTH_EDGE)
    {
        if(s_bISRLog == true && s_un32LogDispCnt == s_un32LogCurCnt)
        {
            LOG("%s (%d) B evt fired\n", EX_CMP_LOG_STR, ((CMP_Context_t *)pContext)->eId);
            s_un32LogCurCnt = 0;
        }
    }

    s_un32LogCurCnt++;
    if(s_un32LogCurCnt > s_un32LogDispCnt)
    {
        s_un32LogCurCnt = 0;
    }
}

static enum debug_cmd_status EX_CMP_Init(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    CMP_ID_e eId = CMP_ID_0;

    eDbgStatus = EX_CMP_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_CMP_Init(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_CMP_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_INIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_CMP_Uninit(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    CMP_ID_e eId = CMP_ID_0;

    eDbgStatus = EX_CMP_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_CMP_Uninit(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_CMP_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_UNINIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_CMP_SetConfig(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus;
    CMP_ID_e eId = CMP_ID_0;
    bool bNonMask = false;
    CMP_CFG_t tCfg;

    eDbgStatus = EX_CMP_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    memset(&tCfg, 0x00, sizeof(CMP_CFG_t));

    eDbgStatus = EX_CMP_GetConfig(n32Argc, pn8Argv, &tCfg, &s_eOps, &bNonMask);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_CMP_SetConfig(eId, &tCfg);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_CMP_ERR_STR, "Cfg", EX_COMM_STR_OPT_MAX);
        }
        goto err;
    }

    s_tCMPContext[eId].eId = eId;

    eErr = HAL_CMP_SetIRQ(eId, s_eOps, EX_CMP_IRQHandler, &s_tCMPContext[eId], EX_CMP_IRQ_PRIO, bNonMask);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_CMP_ERR_STR, "IRQ", EX_COMM_STR_OPT_MAX);
        }
        goto err;

    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;

}

static enum debug_cmd_status EX_CMP_Start(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    CMP_ID_e eId = CMP_ID_0;

    eDbgStatus = EX_CMP_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    s_un32LogCurCnt = 0;

    LOG("%s (%d) %s\n", EX_CMP_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_START]);

    eErr = HAL_CMP_Start(eId);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    if(s_eOps == CMP_OPS_POLL)
    {
        eErr = HAL_CMP_SetWaitComplete(eId, 20000);
        if(eErr == HAL_ERR_OK)
        {
            LOG("%s (%d) trigger\n", EX_CMP_LOG_STR, eId);
        }
        else
        {
            LOG("%s (%d) timeout\n", EX_CMP_LOG_STR, eId);
        }
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_CMP_Stop(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    CMP_ID_e eId = CMP_ID_0;

    eDbgStatus = EX_CMP_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_CMP_Stop(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_CMP_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_STOP]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_CMP_SetLog(int32_t n32Argc, char *pn8Argv[])
{
    if((strncmp(pn8Argv[1],"on",2) == 0))
    {
        s_bISRLog = true;
        if(n32Argc == 3)
        {
            s_un32LogDispCnt = (uint32_t)atoi(pn8Argv[2]);
        }
    }
    else
    {
        s_bISRLog = false;
    }

    LOG("%s ISR Log %s.\n", EX_CMP_LOG_STR, (s_bISRLog == true ? "on":"off"));

    return DEBUG_CMD_SUCCESS;
}

static const struct debug_cmd s_tEX_CMP_CMD[] =
{
    {"CMP", "h", EX_CMP_Help,"help"},
    {"CMP", "init", EX_CMP_Init, ""},
    {"CMP", "uninit", EX_CMP_Uninit, ""},
    {"CMP", "config", EX_CMP_SetConfig, ""},
    {"CMP", "start", EX_CMP_Start, ""},
    {"CMP", "stop", EX_CMP_Stop, ""},
    {"LED", "log", EX_CMP_SetLog, ""}
};

void EX_CMP(void)
{
    /* Add TC commands */
    debug_cmd_init(s_tEX_CMP_CMD, DEBUG_CMD_LIST_COUNT(s_tEX_CMP_CMD));
}

#endif /* CMP_TC */

/* --------------------------------- End Of File ------------------------------ */
