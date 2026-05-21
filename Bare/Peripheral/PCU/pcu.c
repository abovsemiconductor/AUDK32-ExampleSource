/**
 *******************************************************************************
 * @file        pcu.c
 * @author      ABOV R&D Division
 * @brief       PCU Example Code
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

#if defined(PCU_TC)
#include "abov_config.h"
#include "ex_common.h"
#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "hal_scu.h"
#include "hal_scu_clk.h"
#include "hal_pcu.h"
#include "hal_ebi.h"

#define EX_PCU_STR "PCU"
#define EX_PCU_LOG_STR "PCU :"
#define EX_PCU_ERR_STR "[E]PCU :"
#define EX_PCU_IRQ_PRIO   3
#define EX_PCU_MAX_NUM (CONFIG_PCU_MAX_COUNT - 1)

extern const char *pcCommStr[];
extern const char *pcCmdStr[];
extern const char *pcValStr[];
extern const char *pcOptStr[];

static PCU_Context_t s_tPCUContext[CONFIG_PCU_MAX_COUNT];

typedef enum {
    EX_PCU_MODE_INPUT,
    EX_PCU_MODE_OUTPUT,
    EX_PCU_MODE_ALT,
    EX_PCU_MODE_MAX
} EX_PCU_MODE_e;

static enum debug_cmd_status EX_PCU_Help(int32_t n32Argc, char *pn8Argv[])
{
    uint32_t un32ShowOpt = 0;
    EX_COMM_STR_OPT_e eOpt[10];

    EX_COMMON_SetShowModuleInfo(EX_PCU_STR, CONFIG_PCU_MAX_COUNT);

    un32ShowOpt = EX_COMMON_GetShowOpt(pn8Argv[1]);

    eOpt[0] = EX_COMM_STR_OPT_PIN; 
    eOpt[1] = EX_COMM_STR_OPT_MODE; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "port", EX_PCU_MAX_NUM, eOpt, 2, "[-pupd]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_PIN, EX_COMM_STR_VAL_N_NUM, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MODE, EX_COMM_STR_VAL_MAX, "i(in)/o(out)/a(alt)");
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "i: [in] [-intr] [-flvl]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "in: i(in)/a(ang in)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-intr: [ops] [mode] [trg] [-num]");
        EX_COMMON_SetShowOptVal(3, false, EX_COMM_STR_OPT_OPS, EX_COMM_STR_VAL_OPS, "/n(nmi)");
        EX_COMMON_SetShowOptVal(3, false, EX_COMM_STR_OPT_MODE, EX_COMM_STR_VAL_MAX, "d(dis)/n(level no-pend)/p(level pend)/e(edge)");
        EX_COMMON_SetShowOptVal(3, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "trg: d(dis)/f(low|fall)/r(high|rise)/b(both)");
        EX_COMMON_SetShowOptVal(4, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-num: 0~N (intr num)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-flvl: [lvl]");
        EX_COMMON_SetShowOptVal(3, false, EX_COMM_STR_OPT_LEVEL, EX_COMM_STR_VAL_MAX, "v(VDD)/l(1.8V)");
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "o: [out] [pol]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "out: p(push-pull)/o(open-drain)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_POL, EX_COMM_STR_VAL_LOWHIGH, NULL);
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "a: [alt]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "alt: 0~N");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-pupd: [pull]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "pull: d(dis)/p(up)/n(down)");
    }
    eOpt[0] = EX_COMM_STR_OPT_PIN; 
    eOpt[1] = EX_COMM_STR_OPT_ENA; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "db", EX_PCU_MAX_NUM, eOpt, 2, "[-clk]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_PIN, EX_COMM_STR_VAL_N_NUM, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_ENA, EX_COMM_STR_VAL_ENDIS, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-clk: [mccr] [div]");    
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MCCR, EX_COMM_STR_VAL_CLKSRC, "/m(mclk)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_DIV, EX_COMM_STR_VAL_N_255, NULL);
    } 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "str", EX_PCU_MAX_NUM, eOpt, 2, NULL);
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_PIN, EX_COMM_STR_VAL_N_NUM, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_ENA, EX_COMM_STR_VAL_ENDIS, NULL);
    }
    eOpt[1] = EX_COMM_STR_OPT_LEVEL; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "output", EX_PCU_MAX_NUM, eOpt, 2, "[-sustain]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_PIN, EX_COMM_STR_VAL_N_NUM, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_LEVEL, EX_COMM_STR_VAL_LOWHIGH, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-sustain: [en]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_ENA, EX_COMM_STR_VAL_ENDIS, NULL);
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, (char *)pcOptStr[EX_COMM_STR_OPT_PIN], EX_PCU_MAX_NUM, eOpt, 1, NULL);
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_PIN, EX_COMM_STR_VAL_N_NUM, NULL);
    }
    LOG("\n");

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_PCU_GetId(int32_t n32Argc, char *pn8Argv[], PCU_ID_e *peId)
{
    uint8_t un8Data = 0;
    if(n32Argc == 1)
    {
        EX_COMMON_SetShowModuleLog(EX_PCU_ERR_STR, (char *)pcOptStr[EX_COMM_STR_OPT_ARG], EX_COMM_STR_OPT_MAX);
        return DEBUG_CMD_INVALID;
    }

    un8Data = atoi(pn8Argv[1]); 
    *peId = (PCU_ID_e)un8Data;

    return DEBUG_CMD_SUCCESS;
}

static void EX_PCU_IRQHandler(uint32_t un32Event, void *pContext)
{
    LOG("%s Port-%d %s(0x%x)\n", EX_PCU_LOG_STR, ((PCU_Context_t *)pContext)->eId, pcCommStr[EX_COMM_STR_EVT_FIRE], un32Event);
}

static enum debug_cmd_status EX_PCU_SetPort(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;
    uint8_t un8Arg = 2, un8IntNum = 0;
    EX_PCU_MODE_e eMode = EX_PCU_MODE_MAX;
    PCU_ID_e eId = PCU_ID_A;
    PCU_PIN_ID_e ePinId = PCU_PIN_ID_MAX;
    PCU_INOUT_e eInOut = PCU_INOUT_INPUT;
    PCU_INTR_MODE_e eIntrMode = PCU_INTR_MODE_DISABLE;
    PCU_INTR_TRG_e eIntrTrg = PCU_INTR_TRG_DISABLE;
    PCU_ALT_e eAlt = PCU_ALT_MAX;
    PCU_PUPD_e ePupd = PCU_PUPD_MAX;
    PCU_PORT_e eOutput = PCU_PORT_MAX;
    PCU_OPS_e eOps = PCU_OPS_MAX;
    PCU_IRQ_CFG_t tIRQCfg;
    bool bSetIntr = false, bSetPortFLvl = false, bPortFLvl = false;
     
    eDbgStatus = EX_PCU_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    ePinId = (PCU_PIN_ID_e)atoi(pn8Argv[un8Arg++]); 

    if (pn8Argv[un8Arg][0] == 'i')
        eMode = EX_PCU_MODE_INPUT;
    else if (pn8Argv[un8Arg][0] == 'o')
        eMode = EX_PCU_MODE_OUTPUT;
    else if (pn8Argv[un8Arg][0] == 'a')
        eMode = EX_PCU_MODE_ALT;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_PCU_ERR_STR, NULL, EX_COMM_STR_OPT_MODE);
        goto err;
    }

    if(eMode == EX_PCU_MODE_INPUT)
    {
        un8Arg++;
        switch (pn8Argv[un8Arg++][0])
        {
            case 'i':
                eInOut = PCU_INOUT_INPUT;
                break;
            case 'a':
                eInOut = PCU_INOUT_ANG_INPUT;
                break;
            default:
	        EX_COMMON_SetShowModuleLog(EX_PCU_ERR_STR, "input[in]", EX_COMM_STR_OPT_MAX);
                goto err;
        }

        if(strncmp(pn8Argv[un8Arg], "-intr", 5) == 0)
        {
            un8Arg++;

            if (pn8Argv[un8Arg][0] == 'p')
                eOps = PCU_OPS_POLL;
            else if (pn8Argv[un8Arg][0] == 'i')
            {
                eOps = PCU_OPS_INTR;
                bSetIntr = true;
            }
            else if (pn8Argv[un8Arg][0] == 'n')
            {
                eOps = PCU_OPS_NMI;
                bSetIntr = true;
            }
            else
            {
	        EX_COMMON_SetShowModuleLog(EX_PCU_ERR_STR, "input", EX_COMM_STR_OPT_OPS);
                goto err;
            }

            un8Arg++;
            if (pn8Argv[un8Arg][0] == 'd')
                eIntrMode = PCU_INTR_MODE_DISABLE;
            else if (pn8Argv[un8Arg][0] == 'n')
                eIntrMode = PCU_INTR_MODE_LEVEL_NONPEND;
            else if (pn8Argv[un8Arg][0] == 'p')
                eIntrMode = PCU_INTR_MODE_LEVEL_PEND;
            else if (pn8Argv[un8Arg][0] == 'e')
                eIntrMode = PCU_INTR_MODE_EDGE;
            else
            {
	        EX_COMMON_SetShowModuleLog(EX_PCU_ERR_STR, "input", EX_COMM_STR_OPT_MODE);
                goto err;
            }

            un8Arg++;
            if (pn8Argv[un8Arg][0] == 'd')
                eIntrTrg = PCU_INTR_TRG_DISABLE;
            else if (pn8Argv[un8Arg][0] == 'f')
                eIntrTrg = PCU_INTR_TRG_LOW_FALLING;
            else if (pn8Argv[un8Arg][0] == 'r')
                eIntrTrg = PCU_INTR_TRG_HIGH_RISING;
            else if (pn8Argv[un8Arg][0] == 'b')
                eIntrTrg = PCU_INTR_TRG_BOTH_LEVEL_EDGE;
            else
            {
	        EX_COMMON_SetShowModuleLog(EX_PCU_ERR_STR, "input[trg]", EX_COMM_STR_OPT_MAX);
                goto err;
            }

            if(strncmp(pn8Argv[un8Arg], "-num", 4) == 0)
            {
                un8Arg++;
                un8IntNum = (uint8_t)atoi(pn8Argv[un8Arg++]);
            }
        }

        if(strncmp(pn8Argv[un8Arg], "-flvl", 5) == 0)
        {
            if(eId != PCU_ID_F || (ePinId != PCU_PIN_ID_5 
               && ePinId != PCU_PIN_ID_6 && ePinId != PCU_PIN_ID_7))
            {
                return DEBUG_CMD_INVALID;
            }

            un8Arg++;
            bSetPortFLvl = true;
            switch (pn8Argv[un8Arg++][0])
            {
                case 'v':
                    bPortFLvl = false;
                    break;
                case 'l':
                    bPortFLvl = true;
                    break;
                default:
	            EX_COMMON_SetShowModuleLog(EX_PCU_ERR_STR, "-flvl", EX_COMM_STR_OPT_LEVEL);
                    goto err;
            }
        }
    }
    else if(eMode == EX_PCU_MODE_OUTPUT)
    {
        un8Arg++;
        switch(pn8Argv[un8Arg++][0])
        {
            case 'p':
                eInOut = PCU_INOUT_OUTPUT_PUSH_PULL;
                break;
            case 'o':
                eInOut = PCU_INOUT_OUTPUT_OPEN_DRAIN;
                break;
            default:
	        EX_COMMON_SetShowModuleLog(EX_PCU_ERR_STR, "output[out]", EX_COMM_STR_OPT_MAX);
                goto err;
        }

        switch(pn8Argv[un8Arg++][0])
        {
            case 'l':
                eOutput = PCU_PORT_LOW;
                break;
            case 'h':
                eOutput = PCU_PORT_HIGH;
                break;
            default:
	        EX_COMMON_SetShowModuleLog(EX_PCU_ERR_STR, "output", EX_COMM_STR_OPT_POL);
                goto err;
        }
    }
    else if(eMode == EX_PCU_MODE_ALT)
    {
        un8Arg++;
        eAlt = (PCU_ALT_e)atoi(pn8Argv[un8Arg++]); 
    }
    else
    {
        goto err;
    }

    if(eMode == EX_PCU_MODE_ALT)
    {
        eErr = HAL_PCU_SetAltMode(eId, ePinId, eAlt);
        if(eErr != HAL_ERR_OK)
        {
            goto err;
        }
    }
    else
    {
        eErr = HAL_PCU_SetInOutMode(eId, ePinId, eInOut);
        if(eErr != HAL_ERR_OK)
        {
            goto err;
        }

        if(eMode == EX_PCU_MODE_INPUT)
        {
            if(bSetIntr == true)
            {
                eErr = HAL_PCU_SetIntrPort(eId, ePinId, eIntrMode, eIntrTrg, un8IntNum);
                if(eErr != HAL_ERR_OK)
                {
                    goto err;
                }

                s_tPCUContext[eId].eId = eId;

                memset(&tIRQCfg, 0x00, sizeof(PCU_IRQ_CFG_t));

                tIRQCfg.eId = eId;
                tIRQCfg.eOps = eOps;
                tIRQCfg.pfnHandler = &EX_PCU_IRQHandler;
                tIRQCfg.pContext = &s_tPCUContext[eId];
                tIRQCfg.un32IRQPrio = EX_PCU_IRQ_PRIO;
                tIRQCfg.un8IntNum = un8IntNum;

                eErr = HAL_PCU_SetIRQ(&tIRQCfg);
                if(eErr != HAL_ERR_OK)
                {
                    goto err;
                }
            }
            else
            {
                eErr = HAL_PCU_SetIntrPort(eId, ePinId, PCU_INTR_MODE_DISABLE, PCU_INTR_TRG_DISABLE, un8IntNum);
                if(eErr != HAL_ERR_OK)
                {
                    return DEBUG_CMD_INVALID;
                }

                memset(&tIRQCfg, 0x00, sizeof(PCU_IRQ_CFG_t));

                tIRQCfg.eId = eId;
                tIRQCfg.eOps = eOps;
                tIRQCfg.pfnHandler = NULL;
                tIRQCfg.pContext = NULL;
                tIRQCfg.un32IRQPrio = EX_PCU_IRQ_PRIO;

                eErr = HAL_PCU_SetIRQ(&tIRQCfg);
                if(eErr != HAL_ERR_OK)
                {
                    goto err;
                }
            }

            if(bSetPortFLvl == true)
            {
                eErr = HAL_PCU_SetPortFInputLevel(eId, ePinId, bPortFLvl);
                if(eErr != HAL_ERR_OK)
                {
                    goto err;
                }
            }
        }

        if(eMode == EX_PCU_MODE_OUTPUT)
        {
            eErr = HAL_PCU_SetOutputValue(eId, ePinId, eOutput);
            if(eErr != HAL_ERR_OK)
            {
                goto err;
            }
        }
    }

    if(strncmp(pn8Argv[un8Arg], "-pupd", 5) == 0)
    {
        un8Arg++;

        if (pn8Argv[un8Arg][0] == 'd')
            ePupd = PCU_PUPD_DISABLED;
        else if (pn8Argv[un8Arg][0] == 'p')
            ePupd = PCU_PUPD_UP;
        else if (pn8Argv[un8Arg][0] == 'n')
            ePupd = PCU_PUPD_DOWN;
        else
        {
	    EX_COMMON_SetShowModuleLog(EX_PCU_ERR_STR, "-pupd[pull]", EX_COMM_STR_OPT_MAX);
            goto err;
        }

        eErr = HAL_PCU_SetPullUpDown(eId, ePinId, ePupd);
        if(eErr != HAL_ERR_OK)
        {
            goto err;
        }
    }

    return eDbgStatus;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_PCU_GetInput(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    uint8_t un8Arg = 2;
    PCU_ID_e eId = PCU_ID_A;
    PCU_PIN_ID_e ePinId = PCU_PIN_ID_MAX;
    PCU_PORT_e ePinValue = PCU_PORT_MAX;
     
    eDbgStatus = EX_PCU_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    ePinId = (PCU_PIN_ID_e)atoi(pn8Argv[un8Arg++]); 

    eErr = HAL_PCU_GetInputValue(eId, ePinId, &ePinValue);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s in = %d:%d:%s\n",EX_PCU_LOG_STR, (uint8_t)eId, (uint8_t)ePinId, (ePinValue == PCU_PORT_LOW ? "l" : "h"));

    return eDbgStatus;
}

static enum debug_cmd_status EX_PCU_SetOutput(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    uint8_t un8Arg = 2;
    PCU_ID_e eId = PCU_ID_A;
    PCU_PIN_ID_e ePinId = PCU_PIN_ID_MAX;
    PCU_OUTPUT_BIT_e eBit = PCU_OUTPUT_BIT_MAX;
    bool bEnable = false;
     
    eDbgStatus = EX_PCU_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    ePinId = (PCU_PIN_ID_e)atoi(pn8Argv[un8Arg++]); 

    switch (pn8Argv[un8Arg++][0])
    {
        case 'h':
            eBit = PCU_OUTPUT_BIT_SET;
            break;
        case 'l':
            eBit = PCU_OUTPUT_BIT_CLEAR;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_PCU_ERR_STR, "[bit]", EX_COMM_STR_OPT_MAX);
            goto err;
    }

    eErr = HAL_PCU_SetOutputBit(eId, ePinId, eBit);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    if(strncmp(pn8Argv[un8Arg], "-sustain", 8) == 0)
    {
        un8Arg++;
        eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], &bEnable);
        if (eDbgStatus != DEBUG_CMD_SUCCESS)
        {
            EX_COMMON_SetShowModuleLog(EX_PCU_ERR_STR, "-sustain", EX_COMM_STR_OPT_ENA);
            goto err;
        }

        eErr = HAL_PCU_SetOutputSustain(eId, ePinId, bEnable);
        if(eErr != HAL_ERR_OK)
        {
            goto err;
        }
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_PCU_SetDebounce(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    uint8_t un8Arg = 2;
    PCU_ID_e eId = PCU_ID_A;
    PCU_PIN_ID_e ePinId = PCU_PIN_ID_MAX;
    bool bEnable = false;
    PCU_DEBOUNCE_CLK_CFG_t tClkCfg;
     
    eDbgStatus = EX_PCU_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    ePinId = (PCU_PIN_ID_e)atoi(pn8Argv[un8Arg++]); 

    eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], &bEnable);
    if (eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_PCU_ERR_STR, NULL, EX_COMM_STR_OPT_ENA);
        goto err;
    }

    eErr = HAL_PCU_SetPortDebounce(eId, ePinId, bEnable);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    if(strncmp(pn8Argv[un8Arg], "-clk", 4) == 0)
    {
        un8Arg++;
        memset(&tClkCfg, 0x00, sizeof(PCU_DEBOUNCE_CLK_CFG_t));

        if (pn8Argv[un8Arg][0] == 'l')
            tClkCfg.eMccr = PCU_CLK_MCCR_LSI;
        else if (pn8Argv[un8Arg][0] == 's')
            tClkCfg.eMccr = PCU_CLK_MCCR_LSE;
        else if (pn8Argv[un8Arg][0] == 'm')
            tClkCfg.eMccr = PCU_CLK_MCCR_MCLK;
        else if (pn8Argv[un8Arg][0] == 'h')
            tClkCfg.eMccr = PCU_CLK_MCCR_HSI;
        else if (pn8Argv[un8Arg][0] == 'e')
            tClkCfg.eMccr = PCU_CLK_MCCR_HSE;
        else if (pn8Argv[un8Arg][0] == 'p')
            tClkCfg.eMccr = PCU_CLK_MCCR_PLL;
        else
        {
            EX_COMMON_SetShowModuleLog(EX_PCU_ERR_STR, "-clk", EX_COMM_STR_OPT_MCCR);
            goto err;
        }

        tClkCfg.un8MccrDiv = atoi(pn8Argv[un8Arg++]);

        if(eDbgStatus == DEBUG_CMD_SUCCESS)
        {
            eErr = HAL_PCU_SetClkDebounce(eId, &tClkCfg);
            if(eErr != HAL_ERR_OK)
            {
                goto err;
            }
        }
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_PCU_SetStrength(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    uint8_t un8Arg = 2;
    PCU_ID_e eId = PCU_ID_A;
    PCU_PIN_ID_e ePinId = PCU_PIN_ID_MAX;
    bool bEnable = false;
     
    eDbgStatus = EX_PCU_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    ePinId = (PCU_PIN_ID_e)atoi(pn8Argv[un8Arg++]); 

    eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], &bEnable);
    if (eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_PCU_ERR_STR, NULL, EX_COMM_STR_OPT_ENA);
        goto err;
    }

    eErr = HAL_PCU_SetPortStrength(eId, ePinId, bEnable);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static struct debug_cmd s_tEX_PCU_CMD[] =
{
    {EX_PCU_STR, "h", EX_PCU_Help, "help"},
    {EX_PCU_STR, "port", EX_PCU_SetPort,""},
    {EX_PCU_STR, "db", EX_PCU_SetDebounce,""},
    {EX_PCU_STR, "str", EX_PCU_SetStrength,""},
    {EX_PCU_STR, "output", EX_PCU_SetOutput,""},
    {EX_PCU_STR, "input", EX_PCU_GetInput,""}
};

void EX_PCU(void)
{
    /* Add TC commands */
    debug_cmd_init(s_tEX_PCU_CMD, DEBUG_CMD_LIST_COUNT(s_tEX_PCU_CMD));
}

#endif /* PCU */
/* --------------------------------- End Of File ------------------------------ */
