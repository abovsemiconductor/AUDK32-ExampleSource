/**
 *******************************************************************************
 * @file        afe.c
 * @author      ABOV R&D Division
 * @brief       AFE Example Code
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

#if defined(AFE_TC)
#include "abov_config.h"
#include "ex_common.h"
#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "hal_scu.h"
#include "hal_scu_clk.h"
#include "hal_pcu.h"
#include "hal_afe.h"

#if !defined(_AFE)
#error "This chipset did not support this example."
#endif

#define EX_AFE_STR "AFE"
#define EX_AFE_LOG_STR "AFE :"
#define EX_AFE_ERR_STR "[E]AFE :"
#define EX_AFE_IRQ_PRIO 3
#define EX_AFE_MAX_NUM  (CONFIG_AFE_MAX_COUNT - 1)

extern const char *pcCommStr[];
extern const char *pcCmdStr[];
extern const char *pcValStr[];
extern const char *pcOptStr[];

extern uint32_t SystemCoreClock;
static AFE_Context_t s_tAFEContext[CONFIG_AFE_MAX_COUNT];
static AFE_OPS_e s_eOps = AFE_OPS_POLL;
static bool s_bISRLog = false;
static uint32_t s_un32LogDispCnt = 0;
static uint32_t s_un32LogCurCnt = 0;

static enum debug_cmd_status EX_AFE_Help(int32_t n32Argc, char *pn8Argv[])
{
    uint32_t un32ShowOpt = 0;
    EX_COMM_STR_OPT_e eOpt[2];

    EX_COMMON_SetShowModuleInfo(EX_AFE_STR, CONFIG_AFE_MAX_COUNT);

    un32ShowOpt = EX_COMMON_GetShowOpt(pn8Argv[1]);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_INIT, NULL, EX_AFE_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_UNINIT, NULL, EX_AFE_MAX_NUM, eOpt, 0, NULL);

    eOpt[0] = EX_COMM_STR_OPT_OPS; 
    eOpt[1] = EX_COMM_STR_OPT_MODE; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CONFIG, NULL, EX_AFE_MAX_NUM, eOpt, 2, NULL);
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_OPS, EX_COMM_STR_VAL_OPS, "/n(nmi)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MODE, EX_COMM_STR_VAL_MAX, "c(cmp)/o(opamp)/g(unity gain)");
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "c: [trg] [inv] [debcnt]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "trg: d(disable)/l(level)/s(rise or fall)/b(both)");
        EX_COMMON_SetShowOptVal(2, true, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "l or s : [pol]"); 
        EX_COMMON_SetShowOptVal(3, false, EX_COMM_STR_OPT_POL, EX_COMM_STR_VAL_MAX, "h(high or rise)/l(low or fall)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "inv: n(not invert)/i(invert) - cmp output signal"); 
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "debcnt: 0~N(4bit) - 0 is disable");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_START, NULL, EX_AFE_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_STOP, NULL, EX_AFE_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_LOG, NULL, -1, eOpt, 0, "on [cnt] / off");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_CNT, EX_COMM_STR_VAL_N_NUM, NULL);
    }
    LOG("\n");

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_AFE_GetId(int32_t n32Argc, char *pn8Argv[], AFE_ID_e *peId)
{
    uint8_t un8Data = 0;
    if(n32Argc == 1)
    {
        EX_COMMON_SetShowModuleLog(EX_AFE_ERR_STR, (char *)pcOptStr[EX_COMM_STR_OPT_ARG], EX_COMM_STR_OPT_MAX);
        return DEBUG_CMD_INVALID;
    }

    un8Data = atoi(pn8Argv[1]); 
    if(un8Data >= CONFIG_AFE_MAX_COUNT)
    {
        LOG("%s Max Chan %d\n", EX_AFE_ERR_STR, CONFIG_AFE_MAX_COUNT);
        return DEBUG_CMD_INVALID;
    }

    *peId = (AFE_ID_e)un8Data;

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_AFE_GetConfig(int32_t n32Argc, char *pn8Argv[], AFE_CFG_t *ptCfg, AFE_OPS_e *peOps, bool *pbNonMask)
{
    uint8_t un8Arg = 2;

    if (pn8Argv[un8Arg][0] == 'p')
        *peOps = AFE_OPS_POLL;
    else if (pn8Argv[un8Arg][0] == 'i')
        *peOps = AFE_OPS_INTR;
    else if (pn8Argv[un8Arg][0] == 'n')
        *peOps = AFE_OPS_NMI;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_AFE_ERR_STR, NULL, EX_COMM_STR_OPT_OPS);
        goto err;
    }

    un8Arg++;
    if (pn8Argv[un8Arg][0] == 'c')
        ptCfg->eMode = AFE_MODE_CMP;
    else if (pn8Argv[un8Arg][0] == 'o')
        ptCfg->eMode = AFE_MODE_OPAMP;
    else if (pn8Argv[un8Arg][0] == 'g')
        ptCfg->eMode = AFE_MODE_UNITY_GAIN;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_AFE_ERR_STR, NULL, EX_COMM_STR_OPT_MODE);
        goto err;
    }

    if(ptCfg->eMode == AFE_MODE_CMP)
    {
        un8Arg++;
        if (pn8Argv[un8Arg][0] == 'd')
            ptCfg->tCmp.eIntrTrg = AFE_CMP_INTR_TRG_DISABLE;
        else if (pn8Argv[un8Arg][0] == 'l')
            ptCfg->tCmp.eIntrTrg = AFE_CMP_INTR_TRG_LEVEL;
        else if (pn8Argv[un8Arg][0] == 's')
            ptCfg->tCmp.eIntrTrg = AFE_CMP_INTR_TRG_SINGLE_EDGE;
        else if (pn8Argv[un8Arg][0] == 'b')
            ptCfg->tCmp.eIntrTrg = AFE_CMP_INTR_TRG_BOTH_EDGE;
        else
        {
            EX_COMMON_SetShowModuleLog(EX_AFE_ERR_STR, "cmp[trg]", EX_COMM_STR_OPT_MAX);
            goto err;
        }

        un8Arg++;
        if (ptCfg->tCmp.eIntrTrg != AFE_CMP_INTR_TRG_BOTH_EDGE)
        {
            switch (pn8Argv[un8Arg++][0])
            {
                case 'h':
                    ptCfg->tCmp.eIntrTrgPol = AFE_CMP_INTR_TRG_POL_HIGH_RISING;
                    break;
                case 'l':
                    ptCfg->tCmp.eIntrTrgPol = AFE_CMP_INTR_TRG_POL_LOW_FALLING;
                    break;
                default:
                    EX_COMMON_SetShowModuleLog(EX_AFE_ERR_STR, "cmp[pol]", EX_COMM_STR_OPT_MAX);
                    goto err;
            }
        }

        switch (pn8Argv[un8Arg++][0])
        {
            case 'n':
                ptCfg->tCmp.bTrgOutPol = false;
                break;
            case 'i':
                ptCfg->tCmp.bTrgOutPol = true;
                break;
            default:
                EX_COMMON_SetShowModuleLog(EX_AFE_ERR_STR, "cmp[inv]", EX_COMM_STR_OPT_MAX);
                goto err;
        }

        ptCfg->tCmp.un8DebCnt = (uint8_t)atoi(pn8Argv[un8Arg++]);
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static void EX_AFE_IRQHandler(uint32_t un32Event, void *pContext)
{

    if(un32Event & AFE_CMP_EVENT_LOW)
    {
        if(s_bISRLog == true && s_un32LogDispCnt == s_un32LogCurCnt)
        {
            LOG("%s (%d) L-Low evt fired\n", EX_AFE_LOG_STR, ((AFE_Context_t *)pContext)->eId);
            s_un32LogCurCnt = 0;
        }
    }

    if(un32Event & AFE_CMP_EVENT_HIGH)
    {
        if(s_bISRLog == true && s_un32LogDispCnt == s_un32LogCurCnt)
        {
            LOG("%s (%d) L-High evt fired\n", EX_AFE_LOG_STR, ((AFE_Context_t *)pContext)->eId);
            s_un32LogCurCnt = 0;
        }
    }

    if(un32Event & AFE_CMP_EVENT_RISING)
    {
        if(s_bISRLog == true && s_un32LogDispCnt == s_un32LogCurCnt)
        {
            LOG("%s (%d) R evt fired\n", EX_AFE_LOG_STR, ((AFE_Context_t *)pContext)->eId);
            s_un32LogCurCnt = 0;
        }
    }

    if(un32Event & AFE_CMP_EVENT_FALLING)
    {
        if(s_bISRLog == true && s_un32LogDispCnt == s_un32LogCurCnt)
        {
            LOG("%s (%d) F evt fired\n", EX_AFE_LOG_STR, ((AFE_Context_t *)pContext)->eId);
            s_un32LogCurCnt = 0;
        }
    }

    s_un32LogCurCnt++;
    if(s_un32LogCurCnt > s_un32LogDispCnt)
    {
        s_un32LogCurCnt = 0;
    }
}

static enum debug_cmd_status EX_AFE_Init(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    AFE_ID_e eId = AFE_ID_0;

    eDbgStatus = EX_AFE_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_AFE_Init(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_AFE_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_INIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_AFE_Uninit(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    AFE_ID_e eId = AFE_ID_0;

    eDbgStatus = EX_AFE_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_AFE_Uninit(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_AFE_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_UNINIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_AFE_SetConfig(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus;
    AFE_ID_e eId = AFE_ID_0;
    bool bNonMask = false;
    AFE_CFG_t tCfg;

    eDbgStatus = EX_AFE_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    memset(&tCfg, 0x00, sizeof(AFE_CFG_t));

    eDbgStatus = EX_AFE_GetConfig(n32Argc, pn8Argv, &tCfg, &s_eOps, &bNonMask);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_AFE_SetConfig(eId, &tCfg);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_AFE_ERR_STR, "Cfg", EX_COMM_STR_OPT_MAX);
        }
        goto err;
    }

    s_tAFEContext[eId].eId = eId;

    eErr = HAL_AFE_SetIRQ(eId, s_eOps, EX_AFE_IRQHandler, &s_tAFEContext[eId], EX_AFE_IRQ_PRIO, bNonMask);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_AFE_ERR_STR, "IRQ", EX_COMM_STR_OPT_MAX);
        }
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;

}

static enum debug_cmd_status EX_AFE_Start(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    AFE_ID_e eId = AFE_ID_0;

    eDbgStatus = EX_AFE_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    s_un32LogCurCnt = 0;

    LOG("%s (%d) %s\n", EX_AFE_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_START]);

    eErr = HAL_AFE_Start(eId);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    if(s_eOps == AFE_OPS_POLL)
    {
        eErr = HAL_AFE_SetWaitComplete(eId, 20000);
        if(eErr == HAL_ERR_OK)
        {
            LOG("%s trigger\n", EX_AFE_LOG_STR);
        }
        else
        {
            LOG("%s timeout\n", EX_AFE_LOG_STR);
        }
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_AFE_Stop(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    AFE_ID_e eId = AFE_ID_0;

    eDbgStatus = EX_AFE_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_AFE_Stop(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_AFE_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_STOP]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_AFE_SetLog(int32_t n32Argc, char *pn8Argv[])
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

    LOG("%s ISR Log %s.\n", EX_AFE_LOG_STR, (s_bISRLog == true ? "on":"off"));

    return DEBUG_CMD_SUCCESS;
}

static const struct debug_cmd s_tEX_AFE_CMD[] =
{
    {"AFE", "h", EX_AFE_Help,"help"},
    {"AFE", "init", EX_AFE_Init, ""},
    {"AFE", "uninit", EX_AFE_Uninit, ""},
    {"AFE", "config", EX_AFE_SetConfig, ""},
    {"AFE", "start", EX_AFE_Start, ""},
    {"AFE", "stop", EX_AFE_Stop, ""},
    {"LED", "log", EX_AFE_SetLog, ""}
};

void EX_AFE(void)
{
    /* Add TC commands */
    debug_cmd_init(s_tEX_AFE_CMD, DEBUG_CMD_LIST_COUNT(s_tEX_AFE_CMD));
}

#endif /* AFE_TC */

/* --------------------------------- End Of File ------------------------------ */
