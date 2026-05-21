/**
 *******************************************************************************
 * @file        dac.c
 * @author      ABOV R&D Division
 * @brief       DAC Example Code
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

#if defined(DAC_TC)
#include "abov_config.h"
#include "ex_common.h"
#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "hal_scu.h"
#include "hal_scu_clk.h"
#include "hal_pcu.h"
#include "hal_dac.h"

#if !defined(_DAC)
#error "This chipset did not support this example."
#endif

#define EX_DAC_STR "DAC"
#define EX_DAC_LOG_STR "DAC :"
#define EX_DAC_ERR_STR "[E]DAC :"
#define EX_DAC_IRQ_PRIO    3
#define EX_DAC_LOOP        50
#define EX_DAC_DATA_LEN    32 
#define EX_DAC_MAX_NUM (CONFIG_DAC_MAX_COUNT - 1)

extern const char *pcCommStr[];
extern const char *pcCmdStr[];
extern const char *pcValStr[];
extern const char *pcOptStr[];

extern uint32_t SystemCoreClock;
static DAC_OPS_e s_eOps = DAC_OPS_POLL;
static bool bIgn = false;
static uint16_t s_aun16EX_Data[EX_DAC_DATA_LEN] =
{
    0x0000, 0x0800, 0x1000, 0x1800, 0x2000, 0x2800, 0x3000, 0x3800,
    0x4000, 0x4800, 0x5000, 0x5800, 0x6000, 0x6800, 0x7000, 0x7800,
    0x8000, 0x8800, 0x9000, 0x9800, 0xA000, 0xA800, 0xB000, 0xB800,
    0xC000, 0xC800, 0xD000, 0xD800, 0xE000, 0xE800, 0xF000, 0xFFF0
};

static enum debug_cmd_status EX_DAC_Help(int32_t n32Argc, char *pn8Argv[])
{
    uint32_t un32ShowOpt = 0;
    EX_COMM_STR_OPT_e eOpt[7];

    EX_COMMON_SetShowModuleInfo(EX_DAC_STR, CONFIG_DAC_MAX_COUNT);

    un32ShowOpt = EX_COMMON_GetShowOpt(pn8Argv[1]);

    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_INIT, NULL, EX_DAC_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_UNINIT, NULL, EX_DAC_MAX_NUM, eOpt, 0, NULL);

    eOpt[0] = EX_COMM_STR_OPT_OPS; 
    eOpt[1] = EX_COMM_STR_OPT_MODE; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CONFIG, NULL, EX_DAC_MAX_NUM, eOpt, 2, "[ref] [reload] [-port] [-pg] [-buf]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_OPS, EX_COMM_STR_VAL_OPS, "/d(intr dma)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MODE, EX_COMM_STR_VAL_MAX, "m(manual)/a(auto)");
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "m: [ign]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "ign: ignore local test procedure");
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "a: [dir]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "dir: i(inc)/d(dec)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "ref: i(int vdd)/e(ext pin)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "reload: c(const)/0(timer10)/1(timer11)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-port: [val]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "val: hexa (mask port num ex.0x03)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-pg: [sign] [db]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "sign: p(pos)/n(neg)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "db: 0,6,12,18,24,30");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-buf: [mode]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "mode: e(enable)/d(bypass)");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "ofs", EX_DAC_MAX_NUM, eOpt, 0, "[dir] [val]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "dir: n(none)/a(add)/s(sub)");
	EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "val: 0~N"); 
    }
    eOpt[0] = EX_COMM_STR_OPT_NUM; 
    eOpt[1] = EX_COMM_STR_OPT_CNT; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "buf", EX_DAC_MAX_NUM, eOpt, 2, "[da ...]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_NUM, EX_COMM_STR_VAL_N_NUM, "(buf start num)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_CNT, EX_COMM_STR_VAL_N_NUM, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_DA, EX_COMM_STR_VAL_MAX, "max 32 half-word (hexa and space (delimitor))");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "data", EX_DAC_MAX_NUM, eOpt, 0, "[val]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "val: set dac value(half-word and hexa) ");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_START, NULL, EX_DAC_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_STOP, NULL, EX_DAC_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "dump", -1, eOpt, 0, "display data");
    LOG("\n");

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_DAC_GetId(int32_t n32Argc, char *pn8Argv[], DAC_ID_e *peId)
{
    uint8_t un8Data = 0;
    if(n32Argc == 1)
    {
        EX_COMMON_SetShowModuleLog(EX_DAC_ERR_STR, (char *)pcOptStr[EX_COMM_STR_OPT_ARG], EX_COMM_STR_OPT_MAX);
        return DEBUG_CMD_INVALID;
    }

    un8Data = atoi(pn8Argv[1]); 
    if(un8Data >= CONFIG_DAC_MAX_COUNT)
    {
        LOG("%s Max Chan %d\n", EX_DAC_ERR_STR, CONFIG_DAC_MAX_COUNT);
        return DEBUG_CMD_INVALID;
    }

    *peId = (DAC_ID_e)un8Data;

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_DAC_GetConfig(int32_t n32Argc, char *pn8Argv[], DAC_CFG_t *ptCfg, DAC_OPS_e *peOps, bool *bIgn)
{
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;
    uint8_t un8Arg = 2;
    char c = 0;
    uint8_t un8Data = 0;

    if (pn8Argv[un8Arg][0] == 'p')
        *peOps = DAC_OPS_POLL;
    else if (pn8Argv[un8Arg][0] == 'i')
        *peOps = DAC_OPS_INTR;
    else if (pn8Argv[un8Arg][0] == 'd')
        *peOps = DAC_OPS_INTR_DMA;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_DAC_ERR_STR, NULL, EX_COMM_STR_OPT_OPS);
        goto err;
    }

    switch (pn8Argv[++un8Arg][0])
    {
        case 'm':
            ptCfg->eMode = DAC_MODE_MANUAL;
            break;
        case 'a':
            ptCfg->eMode = DAC_MODE_AUTO;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_DAC_ERR_STR, NULL, EX_COMM_STR_OPT_MODE);
            goto err;
    }

    if(ptCfg->eMode == DAC_MODE_AUTO)
    {
        switch (pn8Argv[++un8Arg][0])
        {
            case 'i':
                ptCfg->eAutoDir = DAC_AUTO_DIR_INC;
                break;
            case 'd':
                ptCfg->eAutoDir = DAC_AUTO_DIR_DEC;
                break;
            default:
                EX_COMMON_SetShowModuleLog(EX_DAC_ERR_STR, "mode[dir]", EX_COMM_STR_OPT_MAX);
                goto err;
        }
    }
    else
    {
        if(strncmp(pn8Argv[++un8Arg], "ign", 3) == 0)
        {
            *bIgn = true;
        }
        else
        {
            un8Arg--;
            *bIgn = false;
        }
        
    }

    switch (pn8Argv[++un8Arg][0])
    {
        case 'i':
            ptCfg->eRef = DAC_REF_INT;
            break;
        case 'e':
            ptCfg->eRef = DAC_REF_EXT;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_DAC_ERR_STR, "[ref]", EX_COMM_STR_OPT_MAX);
            goto err;
    }

    un8Arg++;
    if (pn8Argv[un8Arg][0] == 'c')
        ptCfg->eRLod = DAC_RLOD_CONST;
    else if (pn8Argv[un8Arg][0] == '0')
        ptCfg->eRLod = DAC_RLOD_TIMER10;
    else if (pn8Argv[un8Arg][0] == '1')
        ptCfg->eRLod = DAC_RLOD_TIMER11;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_DAC_ERR_STR, "[reload]", EX_COMM_STR_OPT_MAX);
        goto err;
    }

    un8Arg++;
    if(strncmp(pn8Argv[un8Arg], "-port", 5) == 0)
    {
        un8Arg++;
        uint32_t un32Data;
        sscanf(pn8Argv[un8Arg++], "%X", &un32Data);
        ptCfg->un8OutPort = (uint8_t)un32Data; 
    }

    if(strncmp(pn8Argv[un8Arg],"-pg",3) == 0)
    {
        pn8Argv++;
        c = pn8Argv[un8Arg++][0];
        un8Data = (uint8_t)atoi(pn8Argv[un8Arg++]);
        switch (un8Data)
        {
            case 0:
                ptCfg->ePg = DAC_PG_0DB;
                break;
            case 6:
                if(c == 'p')
                {
                    ptCfg->ePg = DAC_PG_P_6DB;
                }
                else if (c == 'n')
                {
                    ptCfg->ePg = DAC_PG_N_6DB;
                }
                else
                {
                    eDbgStatus = DEBUG_CMD_INVALID;
                }
                break;
            case 12:
                if(c == 'p')
                {
                    ptCfg->ePg = DAC_PG_P_12DB;
                }
                else if (c == 'n')
                {
                    ptCfg->ePg = DAC_PG_N_12DB;
                }
                else
                {
                    eDbgStatus = DEBUG_CMD_INVALID;
                }
                break;
            case 18:
                if(c == 'p')
                {
                    ptCfg->ePg = DAC_PG_P_18DB;
                }
                else if (c == 'n')
                {
                    ptCfg->ePg = DAC_PG_N_18DB;
                }
                else
                {
                    eDbgStatus = DEBUG_CMD_INVALID;
                }
                break;
            case 24:
                if(c == 'p')
                {
                    ptCfg->ePg = DAC_PG_P_24DB;
                }
                else if (c == 'n')
                {
                    ptCfg->ePg = DAC_PG_N_24DB;
                }
                else
                {
                    eDbgStatus = DEBUG_CMD_INVALID;
                }
                break;
            case 30:
                if(c == 'p')
                {
                    ptCfg->ePg = DAC_PG_P_30DB;
                }
                else if (c == 'n')
                {
                    ptCfg->ePg = DAC_PG_N_30DB;
                }
                else
                {
                    eDbgStatus = DEBUG_CMD_INVALID;
                }
                break;
            default:
                EX_COMMON_SetShowModuleLog(EX_DAC_ERR_STR, "pg[db]", EX_COMM_STR_OPT_MAX);
                goto err;
        }

        if (eDbgStatus == DEBUG_CMD_INVALID)
        {
            EX_COMMON_SetShowModuleLog(EX_DAC_ERR_STR, "pg[sig]", EX_COMM_STR_OPT_MAX);
            goto err;
        }

        if(strncmp(pn8Argv[un8Arg],"-buf",4) == 0)
        {
            pn8Argv++;
            c = pn8Argv[un8Arg++][0];
            if(c == 'e')
            {
                ptCfg->bOutBuffer= true;
            }
            else if(c == 'd')
            {
                ptCfg->bOutBuffer = false;
            }
            else
            {
                EX_COMMON_SetShowModuleLog(EX_DAC_ERR_STR, "buf[mode]", EX_COMM_STR_OPT_MAX);
                goto err;
            }
        }
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_DAC_GetOfsConfig(int32_t n32Argc, char *pn8Argv[], DAC_OFS_CFG_t *ptCfg)
{
    uint8_t un8Arg = 2;

    if (pn8Argv[un8Arg][0] == 'n')
    {
        ptCfg->bEnable = false;
    }
    else if (pn8Argv[un8Arg][0] == 'a')
    {
        ptCfg->bEnable = true;
        ptCfg->eOfs = DAC_OFS_ADD;
    }
    else if (pn8Argv[un8Arg][0] == 's')
    {
        ptCfg->bEnable = true;
        ptCfg->eOfs = DAC_OFS_SUB;
    }
    else
    {
        EX_COMMON_SetShowModuleLog(EX_DAC_ERR_STR, "[dir]", EX_COMM_STR_OPT_MAX);
        return DEBUG_CMD_INVALID;
    }

    ptCfg->un8Ofs = (uint8_t)atoi(pn8Argv[un8Arg++]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_DAC_Dump(int32_t n32Argc, char *pn8Argv[])
{
    for(int i = 0; i < EX_DAC_DATA_LEN; i++)
    {
        LOG("%s %d=0x%x\n", EX_DAC_LOG_STR, i, s_aun16EX_Data[i]);
    }

    return DEBUG_CMD_SUCCESS;
}

static void EX_DAC_IRQHandler(uint32_t un32Event, void *pContext)
{

    if(un32Event & DAC_EVENT_DATA_REACHED)
    {
        LOG("%s data reach evt fired\n", EX_DAC_LOG_STR);
    }

    if(un32Event & DAC_EVENT_RX_DONE)
    {
        LOG("%s rx done evt fired\n", EX_DAC_LOG_STR);
    }

    if(un32Event & DAC_EVENT_RX_UNDERRUN)
    {
        LOG("%s rx under evt fired\n", EX_DAC_LOG_STR);
    }

}

static enum debug_cmd_status EX_DAC_Init(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    DAC_ID_e eId = DAC_ID_0;

    eDbgStatus = EX_DAC_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_DAC_Init(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_DAC_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_INIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_DAC_Uninit(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    DAC_ID_e eId = DAC_ID_0;

    eDbgStatus = EX_DAC_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_DAC_Uninit(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_DAC_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_UNINIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_DAC_SetConfig(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus;
    DAC_ID_e eId = DAC_ID_0;
    DAC_CFG_t tCfg;

    eDbgStatus = EX_DAC_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    memset(&tCfg, 0x00, sizeof(DAC_CFG_t));

    eDbgStatus = EX_DAC_GetConfig(n32Argc, pn8Argv, &tCfg, &s_eOps, &bIgn);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_DAC_SetConfig(eId, &tCfg);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_DAC_ERR_STR, "Cfg", EX_COMM_STR_OPT_MAX);
        }

        goto err;
    }

    if(tCfg.eMode == DAC_MODE_AUTO)
    {
        if(tCfg.eAutoDir == DAC_AUTO_DIR_INC)
        {
            eErr = HAL_DAC_SetData(eId, 0x00);
        }
        else
        {
            eErr = HAL_DAC_SetData(eId, 0xFFF);
        }
    }

    eErr = HAL_DAC_SetIRQ(eId, s_eOps, EX_DAC_IRQHandler, NULL, EX_DAC_IRQ_PRIO);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_DAC_ERR_STR, "IRQ", EX_COMM_STR_OPT_MAX);
        }
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;

}

static enum debug_cmd_status EX_DAC_SetOfsConfig(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus;
    DAC_ID_e eId = DAC_ID_0;
    DAC_OFS_CFG_t tOfsCfg;

    eDbgStatus = EX_DAC_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    memset(&tOfsCfg, 0x00, sizeof(DAC_OFS_CFG_t));
 
    eDbgStatus = EX_DAC_GetOfsConfig(n32Argc, pn8Argv, &tOfsCfg);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_DAC_SetOfsConfig(eId, &tOfsCfg);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_DAC_ERR_STR, "OfsCfg", EX_COMM_STR_OPT_MAX);
        }
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_DAC_SetBuffer(int32_t n32Argc, char *pn8Argv[])
{
    uint8_t un8Arg = 2, un8BufLen = 0;
    uint32_t un32Data = 0, un8BufStart = 0;

    un8BufStart = (uint8_t)atoi(pn8Argv[un8Arg++]); 

    if(un8BufStart >= EX_DAC_DATA_LEN)
    {
        LOG("%s start number is out-of-scope (max:%d).\n", EX_DAC_ERR_STR, EX_DAC_DATA_LEN);
        goto err;
    }

    un8BufLen = (uint8_t)atoi(pn8Argv[un8Arg++]);

    if(un8BufLen > EX_DAC_DATA_LEN)
    {
        LOG(" %s len too many(max:%d)\n", EX_DAC_ERR_STR, EX_DAC_DATA_LEN);
        goto err;
    }
    
    if(un8BufStart + un8BufLen > EX_DAC_DATA_LEN)
    {
        LOG("%s start number is out-of-scope (max:%d).\n", EX_DAC_ERR_STR, EX_DAC_DATA_LEN);
        goto err;
    }
 
    for(int i = 0; i < un8BufLen; i++)
    {
        sscanf(pn8Argv[un8Arg++], "%X", &un32Data);
        s_aun16EX_Data[un8BufStart++] = (uint16_t)un32Data;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_DAC_SetData(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    uint8_t un8Arg = 2;
    uint32_t un32Data = 0;
    DAC_ID_e eId = DAC_ID_0;

    eDbgStatus = EX_DAC_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    sscanf(pn8Argv[un8Arg++], "%X", &un32Data);
    eErr = HAL_DAC_SetData(eId, (uint16_t)un32Data);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    return DEBUG_CMD_SUCCESS;
}


static enum debug_cmd_status EX_DAC_Start(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    DAC_ID_e eId = DAC_ID_0;
    uint32_t un32MaxRes = 0;

    eDbgStatus = EX_DAC_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    un32MaxRes = HAL_DAC_GetMaxResolution();

    eErr = HAL_DAC_Start(eId);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    LOG("%s (%d) %s\n", EX_DAC_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_START]);

    if(s_eOps == DAC_OPS_POLL && bIgn == false)
    {
        for(int j = 0; j < EX_DAC_LOOP; j++)
        {
            for(int i = 0; i <= un32MaxRes; i++)
            {
                HAL_DAC_SetData(eId, i);
                SystemDelayUS(10);
            }

            for(int i = un32MaxRes; i > 0; i--)
            {
                HAL_DAC_SetData(eId, i);
                SystemDelayUS(10);
            }
        }
    }

    if(s_eOps == DAC_OPS_INTR_DMA)
    {
        eErr = HAL_DAC_SetDMA(eId, (uint32_t)&s_aun16EX_Data, (uint32_t)(sizeof(s_aun16EX_Data)/sizeof(uint16_t)));
        if(eErr != HAL_ERR_OK)
        {
            goto err;
        }
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_DAC_Stop(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    DAC_ID_e eId = DAC_ID_0;

    eDbgStatus = EX_DAC_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_DAC_Stop(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_DAC_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_STOP]);

    return DEBUG_CMD_SUCCESS;
}

static const struct debug_cmd s_tEX_DAC_CMD[] =
{
    {"DAC", "h", EX_DAC_Help, "help"},
    {"DAC", "init", EX_DAC_Init, ""},
    {"DAC", "uninit", EX_DAC_Uninit, ""},
    {"DAC", "config", EX_DAC_SetConfig, ""},
    {"DAC", "ofs", EX_DAC_SetOfsConfig, ""},
    {"DAC", "buf", EX_DAC_SetBuffer, ""},
    {"DAC", "data", EX_DAC_SetData, ""},
    {"DAC", "start", EX_DAC_Start, ""},
    {"DAC", "stop", EX_DAC_Stop, ""},
    {"DAC", "dump", EX_DAC_Dump, ""}
};

/**********************************************************************
 * @brief		Main program
 * @param[in]	None
 * @return	None
 **********************************************************************/
void EX_DAC(void)
{
    /* Add TC commands */
    debug_cmd_init(s_tEX_DAC_CMD, DEBUG_CMD_LIST_COUNT(s_tEX_DAC_CMD));
}

#endif /* DAC_TC */

/* --------------------------------- End Of File ------------------------------ */
