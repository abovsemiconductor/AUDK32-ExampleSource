/**
 *******************************************************************************
 * @file        aes.c
 * @author      ABOV R&D Division
 * @brief       AES Example Code
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

#if defined(AES_TC)
#include "abov_config.h"
#include "ex_configs.h"
#include "ex_common.h"
#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "hal_scu.h"
#include "hal_pcu.h"
#include "hal_aes.h"

#if !defined(_AES)
#error "This chipset did not support this example."
#endif

#if defined (EX_COMMON_ENABLE_CUSTOM_SSCANF)
#define sscanf(str, fmt, out) EX_COMMON_ParseByFormat((str), (fmt)[1], (uint32_t *)(out))
#endif

#define EX_AES_STR "AES"
#define EX_AES_LOG_STR "AES :"
#define EX_AES_ERR_STR "[E]AES :"
#define EX_AES_IRQ_PRIO      3
#define EX_AES_DATA_LEN      32 
#define EX_AES_KEY_DATA_CNT  4
#define EX_AES_IV_KEY_DATA_CNT  4
#define EX_AES_MAX_NUM  (CONFIG_AES_MAX_COUNT - 1)

extern const char *pcCommStr[];
extern const char *pcCmdStr[];
extern const char *pcValStr[];
extern const char *pcOptStr[];

static AES_OPS_e s_eOps = AES_OPS_POLL;
static uint32_t s_aun32EX_InData[EX_AES_DATA_LEN];
static uint32_t s_aun32EX_OutData[EX_AES_DATA_LEN];

static enum debug_cmd_status EX_AES_Help(int32_t n32Argc, char *pn8Argv[])
{
    uint32_t un32ShowOpt = 0;
    EX_COMM_STR_OPT_e eOpt[2];

    EX_COMMON_SetShowModuleInfo(EX_AES_STR, CONFIG_AES_MAX_COUNT);

    un32ShowOpt = EX_COMMON_GetShowOpt(pn8Argv[1]);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_INIT, NULL, EX_AES_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_UNINIT, NULL, EX_AES_MAX_NUM, eOpt, 0, NULL);

    eOpt[0] = EX_COMM_STR_OPT_OPS; 
#if defined (EX_AES_MODE_SELECT)
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CONFIG, NULL, EX_AES_MAX_NUM, eOpt, 1, "[mode] [cip] [ina] [outa]");
#else
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CONFIG, NULL, EX_AES_MAX_NUM, eOpt, 1, "[cip] [ina] [outa]");
#endif
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_OPS, EX_COMM_STR_VAL_OPS, "/d(intr dma)/n(nmi)");
#if defined (EX_AES_MODE_SELECT)
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "mode: e(ecb)/c(cbc)/t(ctr)");
#else
#endif
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "cip: e(enc)/d(dec)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "ina: w(word)/s(swap word)/b(swap byte)/i(swap byte in word)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "outa: w(word)/s(swap word)/b(swap byte)/i(swap byte in word)");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "key", EX_AES_MAX_NUM, eOpt, 0, "[val ...]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "val: 0~N(32bit hexa)");
    }
#if defined (EX_AES_MODE_SELECT)
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "ivkey", EX_AES_MAX_NUM, eOpt, 0, "[val ...]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "val: 0~N(32bit hexa)");
    }
#endif
    eOpt[0] = EX_COMM_STR_OPT_NUM; 
    eOpt[1] = EX_COMM_STR_OPT_CNT; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "buf", EX_AES_MAX_NUM, eOpt, 2, "[da ...]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_NUM, EX_COMM_STR_VAL_N_NUM, "(buf start num)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_CNT, EX_COMM_STR_VAL_N_NUM, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_DA, EX_COMM_STR_VAL_MAX, "max 16 bytes (hexa and space (delimitor))");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "cmpt", EX_AES_MAX_NUM, eOpt, 0, "[len] [-cp]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "len: 0~15");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-cp: copy out buf to in buf");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "dump", -1, eOpt, 0, "display result data");
    LOG("\n");

    return DEBUG_CMD_SUCCESS;
}

static void EX_AES_IRQHandler(uint32_t un32Event, void *pContext)
{
    if(un32Event & AES_EVENT_ENC_DONE)
    {
        LOG("%s (%d) enc done\n", EX_AES_LOG_STR, AES_ID_0);
    }
 
    if(un32Event & AES_EVENT_DEC_DONE)
    {
        LOG("%s (%d) dec done\n", EX_AES_LOG_STR, AES_ID_0);
    }
}

static enum debug_cmd_status EX_AES_GetAlignMode(uint8_t un8Argv, char *pn8Argv[], AES_ALIGN_MODE_e *peMode)
{
    if (pn8Argv[un8Argv][0] == 'w')
        *peMode = AES_ALIGN_MODE_WORD;
    else if (pn8Argv[un8Argv][0] == 's')
        *peMode = AES_ALIGN_MODE_WORD_INV;
    else if (pn8Argv[un8Argv][0] == 'i')
        *peMode = AES_ALIGN_MODE_BYTE_INV_WORD;
    else if (pn8Argv[un8Argv][0] == 'b')
        *peMode = AES_ALIGN_MODE_BYTE_INV;
    else
    {
        return DEBUG_CMD_INVALID;
    }

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_AES_GetId(int32_t n32Argc, char *pn8Argv[], AES_ID_e *peId)
{
    uint8_t un8Data = 0;
    if(n32Argc == 1)
    {
        EX_COMMON_SetShowModuleLog(EX_AES_ERR_STR, (char *)pcOptStr[EX_COMM_STR_OPT_ARG], EX_COMM_STR_OPT_MAX);
        return DEBUG_CMD_INVALID;
    }

    un8Data = atoi(pn8Argv[1]); 
    if(un8Data >= CONFIG_AES_MAX_COUNT)
    {
        LOG("%s Max Chan %d\n", EX_AES_ERR_STR, CONFIG_AES_MAX_COUNT);
        return DEBUG_CMD_INVALID;
    }

    *peId = (AES_ID_e)un8Data;

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_AES_GetConfig(int32_t n32Argc, char *pn8Argv[], AES_CFG_t *ptCfg, AES_OPS_e *peOps)
{
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;
    uint8_t un8Arg = 2;

    if (pn8Argv[un8Arg][0] == 'p')
    {
        *peOps = AES_OPS_POLL;
        ptCfg->un8IntrEnable = 0;
    }
    else if (pn8Argv[un8Arg][0] == 'i')
        *peOps = AES_OPS_INTR;
    else if (pn8Argv[un8Arg][0] == 'd')
        *peOps = AES_OPS_INTR_DMA;
    else if (pn8Argv[un8Arg][0] == 'n')
        *peOps = AES_OPS_NMI;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_AES_ERR_STR, NULL, EX_COMM_STR_OPT_OPS);
        goto err;
    }
#if defined (EX_AES_MODE_SELECT)
    switch(pn8Argv[++un8Arg][0])
    {
        case 'e':
            ptCfg->eChainMode = AES_CHAIN_MODE_ECB;
            break;        
        case 'c':
            ptCfg->eChainMode = AES_CHAIN_MODE_CBC;
            break;        
        case 't':
            ptCfg->eChainMode = AES_CHAIN_MODE_CTR;
            break;        
        default:
            EX_COMMON_SetShowModuleLog(EX_AES_ERR_STR, "[mode]", EX_COMM_STR_OPT_OPS);
            goto err;
    }
#else
    ptCfg->eChainMode = AES_CHAIN_MODE_ECB;
#endif

    switch (pn8Argv[++un8Arg][0])
    {
        case 'e':
            ptCfg->eCipherMode = AES_CIPHER_MODE_ENC;
            if((*peOps == AES_OPS_INTR) || (*peOps == AES_OPS_INTR_DMA)
              || (*peOps == AES_OPS_NMI))
            {
                ptCfg->un8IntrEnable = AES_INTR_ENC_DONE;
            }
            break;
        case 'd':
            ptCfg->eCipherMode = AES_CIPHER_MODE_DEC;
            if((*peOps == AES_OPS_INTR) || (*peOps == AES_OPS_INTR_DMA)
              || (*peOps == AES_OPS_NMI))
            {
                ptCfg->un8IntrEnable = AES_INTR_DEC_DONE;
            }
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_AES_ERR_STR, "[cip]", EX_COMM_STR_OPT_OPS);
            goto err;
    }

    eDbgStatus = EX_AES_GetAlignMode(++un8Arg, pn8Argv, &ptCfg->eTextInAlign);
    if (eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_AES_ERR_STR, "[ina]", EX_COMM_STR_OPT_MAX);
        goto err;
    }

    eDbgStatus = EX_AES_GetAlignMode(++un8Arg, pn8Argv, &ptCfg->eTextOutAlign);
    if (eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_AES_ERR_STR, "[outa]", EX_COMM_STR_OPT_MAX);
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_AES_Dump(int32_t n32Argc, char *pn8Argv[])
{
    for(int i = 0; i < EX_AES_DATA_LEN; i++)
    {
        LOG("%s %3d = %08x\n", EX_AES_LOG_STR, i, s_aun32EX_OutData[i]);
    }

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_AES_Init(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    AES_ID_e eId = AES_ID_0;

    eDbgStatus = EX_AES_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_AES_Init(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_AES_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_INIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_AES_Uninit(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    AES_ID_e eId = AES_ID_0;

    eDbgStatus = EX_AES_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_AES_Uninit(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_AES_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_UNINIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_AES_SetConfig(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus;
    AES_ID_e eId = AES_ID_0;
    AES_CFG_t tCfg;
   
    eDbgStatus = EX_AES_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    memset(&tCfg, 0x00, sizeof(AES_CFG_t));

    eDbgStatus = EX_AES_GetConfig(n32Argc, pn8Argv, &tCfg, &s_eOps);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_AES_SetConfig(eId, &tCfg);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_AES_ERR_STR, "Cfg", EX_COMM_STR_OPT_MAX);
        }

        goto err;
    }

    eErr = HAL_AES_SetIRQ(eId, s_eOps, EX_AES_IRQHandler, NULL, EX_AES_IRQ_PRIO);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_AES_SetKey(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    AES_ID_e eId = AES_ID_0;
    uint8_t un8Idx = 1, un8InCnt = 0;
    uint32_t un32Data = 0;
    uint32_t un32Key[EX_AES_KEY_DATA_CNT];

    eDbgStatus = EX_AES_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    un8InCnt = (n32Argc - 2);

    if(un8InCnt > EX_AES_KEY_DATA_CNT)
    {
        goto err;
    }

    memset(un32Key, 0x00, sizeof(un32Key));

    while(un8Idx <= un8InCnt)
    {
        sscanf(pn8Argv[un8Idx + 1], "%X", &un32Data);
        /* swap key data */
        un32Key[EX_AES_KEY_DATA_CNT - un8Idx] = un32Data;
        un8Idx++;
    }

    eErr = HAL_AES_SetKey(eId, un32Key);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

#if defined (EX_AES_MODE_SELECT)
static enum debug_cmd_status EX_AES_SetIVKey(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    AES_ID_e eId = AES_ID_0;
    uint8_t un8Idx = 1, un8InCnt = 0;
    uint32_t un32Data = 0;
    uint32_t un32Key[EX_AES_IV_KEY_DATA_CNT];

    eDbgStatus = EX_AES_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    un8InCnt = (n32Argc - 2);

    if(un8InCnt > EX_AES_IV_KEY_DATA_CNT)
    {
        goto err;
    }

    memset(un32Key, 0x00, sizeof(un32Key));

    while(un8Idx <= un8InCnt)
    {
        sscanf(pn8Argv[un8Idx + 1], "%X", &un32Data);
        /* swap key data */
        un32Key[EX_AES_IV_KEY_DATA_CNT - un8Idx] = un32Data;
        un8Idx++;
    }

    eErr = HAL_AES_SetIVKey(eId, un32Key);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}
#endif

static enum debug_cmd_status EX_AES_SetBuffer(int32_t n32Argc, char *pn8Argv[])
{
    uint8_t un8Arg = 2, un8BufLen = 0;
    uint32_t un32Data = 0, un8BufStart = 0;

    un8BufStart = (uint8_t)atoi(pn8Argv[un8Arg++]); 

    if(un8BufStart >= EX_AES_DATA_LEN)
    {
        LOG("%s start number is out-of-scope (max:%d).\n", EX_AES_ERR_STR, EX_AES_DATA_LEN);
        goto err;
    }

    un8BufLen = (uint8_t)atoi(pn8Argv[un8Arg++]);

    if(un8BufLen > EX_AES_DATA_LEN)
    {
        LOG(" %s len too many(max:%d)\n", EX_AES_ERR_STR, EX_AES_DATA_LEN);
        goto err;
    }
    
    if(un8BufStart + un8BufLen > EX_AES_DATA_LEN)
    {
        LOG("%s start number is out-of-scope (max:%d).\n", EX_AES_ERR_STR, EX_AES_DATA_LEN);
        goto err;
    }
 
    for(int i = 0; i < un8BufLen; i++)
    {
        sscanf(pn8Argv[un8Arg++], "%X", &un32Data);
        s_aun32EX_InData[un8BufStart++] = un32Data;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_AES_SetCompute(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    AES_ID_e eId = AES_ID_0;
    uint8_t un8Arg = 2;
    uint32_t un32Len = 0;

    eDbgStatus = EX_AES_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    un32Len = (uint32_t)atoi(pn8Argv[un8Arg++]);

    if(strncmp(pn8Argv[un8Arg], "-cp", 3) == 0)
    {
        memcpy(s_aun32EX_InData, s_aun32EX_OutData, EX_AES_DATA_LEN*4);
    }

    eErr = HAL_AES_SetCompute(eId, s_aun32EX_InData, un32Len, s_aun32EX_OutData);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    if(s_eOps == AES_OPS_POLL)
    {
        LOG("%s (%d) operation done\n", EX_AES_LOG_STR, eId);
    }

    return eDbgStatus;
}

static const struct debug_cmd s_tEX_AES_CMD[] =
{
    {"AES", "h",EX_AES_Help, "help"},
    {"AES", "init",EX_AES_Init, ""},
    {"AES", "uninit",EX_AES_Uninit, ""},
    {"AES", "config",EX_AES_SetConfig, ""},
    {"AES", "key",EX_AES_SetKey, ""},
#if defined (EX_AES_MODE_SELECT)
    {"AES", "ivkey",EX_AES_SetIVKey, ""},
#endif
    {"AES", "buf",EX_AES_SetBuffer, ""},
    {"AES", "cmpt",EX_AES_SetCompute, ""},
    {"AES", "dump",EX_AES_Dump, ""}
};

/**********************************************************************
 * @brief		Main program
 * @param[in]	None
 * @return	None
 **********************************************************************/
void EX_AES(void)
{
    /* Add TC commands */
    debug_cmd_init(s_tEX_AES_CMD, DEBUG_CMD_LIST_COUNT(s_tEX_AES_CMD));
}

#endif /* AES_TC */

/* --------------------------------- End Of File ------------------------------ */
