/**
 *******************************************************************************
 * @file        crc.c
 * @author      ABOV R&D Division
 * @brief       CRC Example Code
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

#if defined(CRC_TC)
#include "abov_config.h"
#include "ex_configs.h"
#include "ex_common.h"
#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "hal_scu.h"
#include "hal_scu_clk.h"
#include "hal_pcu.h"
#include "hal_crc.h"

#if !defined(_CRC)
#error "This chipset did not support this example."
#endif

#define EX_CRC_STR "CRC"
#define EX_CRC_LOG_STR "CRC :"
#define EX_CRC_ERR_STR "[E]CRC :"
#define EX_CRC_IRQ_PRIO      3
#define EX_CRC_IRQ_TIMEOUT   10000000
#define EX_CRC_DELAY         50
#define EX_CRC_DATA_LEN      16
#define EX_CRC_MAX_NUM  (CONFIG_CRC_MAX_COUNT - 1)

extern const char *pcCommStr[];
extern const char *pcCmdStr[];
extern const char *pcValStr[];
extern const char *pcOptStr[];

static uint8_t s_aun8EX_Data[EX_CRC_DATA_LEN];
static CRC_OPS_e s_eOps = CRC_OPS_POLL;

static enum debug_cmd_status EX_CRC_Help(int32_t n32Argc, char *pn8Argv[])
{
    uint32_t un32ShowOpt = 0;
    EX_COMM_STR_OPT_e eOpt[2];

    EX_COMMON_SetShowModuleInfo(EX_CRC_STR, CONFIG_CRC_MAX_COUNT);

    un32ShowOpt = EX_COMMON_GetShowOpt(pn8Argv[1]);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_INIT, NULL, EX_CRC_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_UNINIT, NULL, EX_CRC_MAX_NUM, eOpt, 0, NULL);

    eOpt[0] = EX_COMM_STR_OPT_OPS; 
    eOpt[1] = EX_COMM_STR_OPT_MODE; 
#if defined (EX_CRC_INPUT_CONF_MODE)
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CONFIG, NULL, EX_CRC_MAX_NUM, eOpt, 2, "[in] [-out] [size] [comple]");
#else
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CONFIG, NULL, EX_CRC_MAX_NUM, eOpt, 2, "[in] [-out]");
#endif
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_OPS, EX_COMM_STR_VAL_MAX, "p(polling)/d(dma)/n(nmi dma)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MODE, EX_COMM_STR_VAL_MAX, "c(crc)/s(checksum)");
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "crc: [poly]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "poly: s(7)/e(8)/t(16)/i(16-CCITT)/w(32)"); 
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "in: l(lsb)/m(msb) first-in");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-out: [out] [inv]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "out: l(lsb)/m(msb) first-out");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "inv: o(on)/f(off)");
#if defined (EX_CRC_INPUT_CONF_MODE)
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "size: e(8)/s(16)/t(32) bit");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "comple: e(en)/d(dis)");
#endif
    }
    eOpt[0] = EX_COMM_STR_OPT_CNT;
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "data", EX_CRC_MAX_NUM, eOpt, 1, "[da ...]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_CNT, EX_COMM_STR_VAL_N_NUM, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_DA, EX_COMM_STR_VAL_MAX, "max 16 bytes (hexa and space (delimitor))");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "cmpt", EX_CRC_MAX_NUM, eOpt, 1, NULL);
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_CNT, EX_COMM_STR_VAL_MAX, "1~16(data cnt)");
    }
    LOG("\n");

    return DEBUG_CMD_SUCCESS;
}

static void EX_CRC_IRQHandler(uint32_t un32Event, void *pContext)
{
    CRC_ID_e eId = CRC_ID_0;
    uint32_t un32Result;

    HAL_CRC_GetResult(eId, &un32Result);
    LOG("%s (%d) result 0x%x (ISR)\n", EX_CRC_LOG_STR, eId, un32Result);
}

static enum debug_cmd_status EX_CRC_GetId(int32_t n32Argc, char *pn8Argv[], CRC_ID_e *peId)
{
    uint8_t un8Data = 0;
    if(n32Argc == 1)
    {
        EX_COMMON_SetShowModuleLog(EX_CRC_ERR_STR, (char *)pcOptStr[EX_COMM_STR_OPT_ARG], EX_COMM_STR_OPT_MAX);
        return DEBUG_CMD_INVALID;
    }

    un8Data = atoi(pn8Argv[1]); 
    if(un8Data >= CONFIG_CRC_MAX_COUNT)
    {
        LOG("%s Max Chan %d\n", EX_CRC_ERR_STR, CONFIG_CRC_MAX_COUNT);
        return DEBUG_CMD_INVALID;
    }

    *peId = (CRC_ID_e)un8Data;

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_CRC_GetConfig(int32_t n32Argc, char *pn8Argv[], CRC_CFG_t *ptCfg, CRC_OPS_e *peOps)
{
#if defined (EX_CRC_INPUT_CONF_MODE)
    enum debug_cmd_status eDbgStatus;
#endif
    uint8_t un8Arg = 2;

    if (pn8Argv[un8Arg][0] == 'p')
    {
        *peOps = CRC_OPS_POLL;
        ptCfg->bIntrEnable = false;
    }
    else if (pn8Argv[un8Arg][0] == 'd')
    {
        *peOps = CRC_OPS_INTR_DMA;
        ptCfg->bIntrEnable = true;
    }
    else if (pn8Argv[un8Arg][0] == 'n')
    {
        *peOps = CRC_OPS_NMI_DMA;
        ptCfg->bIntrEnable = true;
    }
    else
    {
        EX_COMMON_SetShowModuleLog(EX_CRC_ERR_STR, NULL, EX_COMM_STR_OPT_OPS);
        goto err;
    }

    switch (pn8Argv[++un8Arg][0])
    {
        case 'c':
            ptCfg->eMode = CRC_MODE_CRC;
            break;
        case 's':
            ptCfg->eMode = CRC_MODE_CHKSUM;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_CRC_ERR_STR, NULL, EX_COMM_STR_OPT_MODE);
            goto err;
    }

    if(ptCfg->eMode == CRC_MODE_CRC)
    {
        un8Arg++;
        if (pn8Argv[un8Arg][0] == 's')
            ptCfg->ePoly = CRC_POLY_7;
        else if (pn8Argv[un8Arg][0] == 'e')
            ptCfg->ePoly = CRC_POLY_8;
        else if (pn8Argv[un8Arg][0] == 't')
            ptCfg->ePoly = CRC_POLY_16;
        else if (pn8Argv[un8Arg][0] == 'i')
            ptCfg->ePoly = CRC_POLY_16_CCITT;
        else if (pn8Argv[un8Arg][0] == 'w')
            ptCfg->ePoly = CRC_POLY_32;
        else
        {
            EX_COMMON_SetShowModuleLog(EX_CRC_ERR_STR, "crc[poly]", EX_COMM_STR_OPT_MAX);
        }
    }

    switch (pn8Argv[++un8Arg][0])
    {
        case 'l':
            ptCfg->eFirstIn = CRC_INP_LSB;
            break;
        case 'm':
            ptCfg->eFirstIn = CRC_INP_MSB;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_CRC_ERR_STR, "[in]", EX_COMM_STR_OPT_MAX);
            goto err;
    }

    if(strncmp(pn8Argv[++un8Arg], "-out", 4) == 0)
    {
        un8Arg++;
        switch (pn8Argv[un8Arg++][0])
        {
            case 'l':
                ptCfg->tOutputCfg.eFirstOut = CRC_OUTP_LSB;
                break;
            case 'm':
                ptCfg->tOutputCfg.eFirstOut = CRC_OUTP_MSB;
                break;
            default:
                EX_COMMON_SetShowModuleLog(EX_CRC_ERR_STR, "out[out]", EX_COMM_STR_OPT_MAX);
                goto err;
        }

        switch (pn8Argv[un8Arg++][0])
        {
            case 'o':
                ptCfg->tOutputCfg.eInv = CRC_OUTP_INV_ON;
                break;
            case 'f':
                ptCfg->tOutputCfg.eInv = CRC_OUTP_INV_OFF;
                break;
            default:
                EX_COMMON_SetShowModuleLog(EX_CRC_ERR_STR, "out[inv]", EX_COMM_STR_OPT_MAX);
                goto err;
        }
    }

#if defined (EX_CRC_INPUT_CONF_MODE)
    switch (pn8Argv[un8Arg++][0])
    {
        case 'e':
            ptCfg->tInputCfg.eInDataSize = CRC_INP_DATA_8;
            break;
        case 's':
            ptCfg->tInputCfg.eInDataSize = CRC_INP_DATA_16;
            break;
        case 't':
            ptCfg->tInputCfg.eInDataSize = CRC_INP_DATA_32;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_CRC_ERR_STR, "[size]", EX_COMM_STR_OPT_MAX);
            goto err;
    }

    eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], &ptCfg->tInputCfg.bComplement);
    if (eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_CRC_ERR_STR, "[comple]", EX_COMM_STR_OPT_MAX);
        goto err;
    }
#endif
        
    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_CRC_Init(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    CRC_ID_e eId = CRC_ID_0;

    eDbgStatus = EX_CRC_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_CRC_Init(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_CRC_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_INIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_CRC_Uninit(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    CRC_ID_e eId = CRC_ID_0;

    eDbgStatus = EX_CRC_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_CRC_Uninit(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_CRC_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_UNINIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_CRC_SetConfig(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus;
    CRC_ID_e eId = CRC_ID_0;
    CRC_CFG_t tCfg;
   
    eDbgStatus = EX_CRC_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    memset(&tCfg, 0x00, sizeof(CRC_CFG_t));

    eDbgStatus = EX_CRC_GetConfig(n32Argc, pn8Argv, &tCfg, &s_eOps);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_CRC_SetConfig(eId, &tCfg);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_CRC_ERR_STR, "Cfg", EX_COMM_STR_OPT_MAX);
        }

        goto err;
    }

    if(s_eOps == CRC_OPS_INTR_DMA || s_eOps == CRC_OPS_NMI_DMA)
    {
        eErr = HAL_CRC_SetIRQ(eId, s_eOps, EX_CRC_IRQHandler, NULL, EX_CRC_IRQ_PRIO);
        if(eErr != HAL_ERR_OK)
        {
            goto err;
        }
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_CRC_SetBuffer(int32_t n32Argc, char *pn8Argv[])
{
    uint8_t un8Arg = 2, un8BufLen = 0;
    uint32_t un32Data = 0, i = 0;

    un8BufLen = (uint8_t)atoi(pn8Argv[un8Arg++]);

    if(un8BufLen > EX_CRC_DATA_LEN)
    {
        LOG(" %s len too many(max:%d)\n", EX_CRC_ERR_STR, EX_CRC_DATA_LEN);
        return DEBUG_CMD_INVALID;
    }

    if((n32Argc - 3) != un8BufLen)
    {
        LOG(" %s not proper cnt\n", EX_CRC_ERR_STR, EX_CRC_DATA_LEN);
        return DEBUG_CMD_INVALID;
    }

    for(i = 0; i < un8BufLen; i++)
    {
        sscanf(pn8Argv[un8Arg++], "%X", &un32Data);
        s_aun8EX_Data[i] = un32Data;
        LOG("%s (%d:0x%x)\n", EX_CRC_LOG_STR, i, s_aun8EX_Data[i]);
    }

    return DEBUG_CMD_SUCCESS;
}


static enum debug_cmd_status EX_CRC_Compute(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    CRC_ID_e eId = CRC_ID_0;
    uint8_t un8Arg = 2, un8BufLen = 0;
    uint32_t un32Init = 0;
    uint32_t un32Result = 0;

    eDbgStatus = EX_CRC_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    un8BufLen = (uint8_t)atoi(pn8Argv[un8Arg++]);

    if(un8BufLen > EX_CRC_DATA_LEN)
    {
        LOG(" %s len too many(max:%d)\n", EX_CRC_ERR_STR, EX_CRC_DATA_LEN);
        return DEBUG_CMD_INVALID;
    }

    eErr = HAL_CRC_SetCompute(eId, un32Init, s_aun8EX_Data, un8BufLen, &un32Result);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    if(s_eOps == CRC_OPS_POLL)
    {
        LOG("%s (%d) result 0x%x.\n", EX_CRC_LOG_STR, eId, un32Result);
    }

    return DEBUG_CMD_SUCCESS;
}

static const struct debug_cmd s_tEX_CRC_CMD[] =
{
    {"CRC", "h",EX_CRC_Help, "help"},
    {"CRC", "init",EX_CRC_Init, ""},
    {"CRC", "uninit",EX_CRC_Uninit, ""},
    {"CRC", "config",EX_CRC_SetConfig, ""},
    {"CRC", "data", EX_CRC_SetBuffer, ""},
    {"CRC", "cmpt",EX_CRC_Compute, ""}
};

/**********************************************************************
 * @brief		Main program
 * @param[in]	None
 * @return	None
 **********************************************************************/
void EX_CRC(void)
{
    /* Add TC commands */
    debug_cmd_init(s_tEX_CRC_CMD, DEBUG_CMD_LIST_COUNT(s_tEX_CRC_CMD));
}

#endif /* CRC_TC */

/* --------------------------------- End Of File ------------------------------ */
