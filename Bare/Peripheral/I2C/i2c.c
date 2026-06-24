/**
 *******************************************************************************
 * @file        i2c.c
 * @author      ABOV R&D Division
 * @brief       I2C Example Code
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

#if defined(I2C_TC)
#include "abov_config.h"
#include "ex_configs.h"
#include "ex_common.h"
#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "hal_scu.h"
#include "hal_scu_clk.h"
#include "hal_pcu.h"
#include "hal_i2c.h"

#if !defined(_I2C)
#error "This chipset did not support this example."
#endif

#if defined (EX_COMMON_ENABLE_CUSTOM_SSCANF)
#define sscanf(str, fmt, out) EX_COMMON_ParseByFormat((str), (fmt)[1], (uint32_t *)(out))
#endif

#define EX_I2C_STR "I2C"
#define EX_I2C_LOG_STR "I2C :"
#define EX_I2C_ERR_STR "[E]I2C :"
#define EX_I2C_IRQ_PRIO      3
#define EX_I2C_DATA_LEN      16
#define EX_I2C_MAX_NUM  (CONFIG_I2C_MAX_COUNT - 1)

extern const char *pcCommStr[];
extern const char *pcCmdStr[];
extern const char *pcValStr[];
extern const char *pcOptStr[];

static I2C_OPS_e s_eOps = I2C_OPS_MAX;
static I2C_Context_t s_tI2CContext[CONFIG_I2C_MAX_COUNT];
static uint8_t s_aun8EX_TxData[EX_I2C_DATA_LEN];
static uint8_t s_aun8EX_RxData[EX_I2C_DATA_LEN];

static enum debug_cmd_status EX_I2C_Help(int32_t n32Argc, char *pn8Argv[])
{
    uint32_t un32ShowOpt = 0;

    EX_COMM_STR_OPT_e eOpt[3];

    EX_COMMON_SetShowModuleInfo(EX_I2C_STR, CONFIG_I2C_MAX_COUNT);

    un32ShowOpt = EX_COMMON_GetShowOpt(pn8Argv[1]);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_INIT, NULL, EX_I2C_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_UNINIT, NULL, EX_I2C_MAX_NUM, eOpt, 0, NULL);

    eOpt[0] = EX_COMM_STR_OPT_OPS; 
    eOpt[1] = EX_COMM_STR_OPT_MODE; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CLK, NULL, EX_I2C_MAX_NUM, eOpt, 2, "[freq] [-saddr -gc] [-saddr2 -gc2]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_OPS, EX_COMM_STR_VAL_OPS, "d(dma)/n(nmi)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MODE, EX_COMM_STR_VAL_MAX, "m(master)/s(slave)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "freq: 0~N");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-saddr: 0x0~0xN(hexa)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-gc: enable global call");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-saddr2: 0x0~0xN(hexa)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-gc2: enable global call");
    }
    eOpt[0] = EX_COMM_STR_OPT_OPS; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "sltp", EX_I2C_MAX_NUM, eOpt, 1, "[timeout]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_ENA, EX_COMM_STR_VAL_ENDIS, NULL);
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_ENA, EX_COMM_STR_VAL_MAX, "[-intr]");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "timeout: 0~N");
    }
    eOpt[0] = EX_COMM_STR_OPT_CNT;
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "data", EX_I2C_MAX_NUM, eOpt, 1, "[da ...]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_CNT, EX_COMM_STR_VAL_N_NUM, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_DA, EX_COMM_STR_VAL_MAX, "max 16 bytes (hexa and space (delimitor))");
    }
    eOpt[0] = EX_COMM_STR_OPT_CNT;
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "tx", EX_I2C_MAX_NUM, eOpt, 1, "[-saddr] [-poll]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_CNT, EX_COMM_STR_VAL_MAX, "1~16 (tx data cnt)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-saddr: slave addr (hexa) if master");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-poll: en-force polling operation");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "rx", EX_I2C_MAX_NUM, eOpt, 1, "[-saddr] [-poll]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_CNT, EX_COMM_STR_VAL_MAX, "1~16 (tx data cnt)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-saddr: slave addr (hexa) if master");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-poll: en-force polling operation");
    }

    return DEBUG_CMD_SUCCESS;
}

static void EX_I2C_IRQHandler(uint32_t un32Event, void *pContext)
{
    uint32_t i = 0;

    if(un32Event & I2C_EVENT_TX_DONE)
    {
        LOG("%s (%d) tx done\n", EX_I2C_LOG_STR, ((I2C_Context_t *)pContext)->eId);
    }

    if(un32Event & I2C_EVENT_RX_DONE)
    {
        LOG("\n");
        for(i = 0; i < EX_I2C_DATA_LEN; i++)
        {
            LOG("%s rx Data=0x%x\n", EX_I2C_LOG_STR, s_aun8EX_RxData[i]);
        }

        LOG("%s (%d) rx done\n", EX_I2C_LOG_STR, ((I2C_Context_t *)pContext)->eId);
    }
}

static enum debug_cmd_status EX_I2C_GetId(int32_t n32Argc, char *pn8Argv[], I2C_ID_e *peId)
{
    uint8_t un8Data = 0;
    if(n32Argc == 1)
    {
        EX_COMMON_SetShowModuleLog(EX_I2C_ERR_STR, (char *)pcOptStr[EX_COMM_STR_OPT_ARG], EX_COMM_STR_OPT_MAX);
        return DEBUG_CMD_INVALID;
    }

    un8Data = atoi(pn8Argv[1]); 
    if(un8Data >= CONFIG_I2C_MAX_COUNT)
    {
        LOG("%s Max Chan %d\n", EX_I2C_ERR_STR, CONFIG_I2C_MAX_COUNT);
        return DEBUG_CMD_INVALID;
    }

    *peId = (I2C_ID_e)un8Data;

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_I2C_GetConfig(int32_t n32Argc, char *pn8Argv[], I2C_CFG_t *ptCfg, I2C_OPS_e *peOps)
{
    uint8_t un8Arg = 2;
    uint32_t un32SlvAddr = 0;

    if (pn8Argv[un8Arg][0] == 'p')
        *peOps = I2C_OPS_POLL;
    else if (pn8Argv[un8Arg][0] == 'i')
        *peOps = I2C_OPS_INTR;
    else if (pn8Argv[un8Arg][0] == 'd')
        *peOps = I2C_OPS_INTR_DMA;
    else if (pn8Argv[un8Arg][0] == 'n')
        *peOps = I2C_OPS_NMI;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_I2C_ERR_STR, NULL, EX_COMM_STR_OPT_OPS);
        goto err;
    }
  
    switch (pn8Argv[++un8Arg][0])
    {
        case 'm':
            ptCfg->eMode = I2C_MODE_MASTER;
            break;
        case 's':
            ptCfg->eMode = I2C_MODE_SLAVE;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_I2C_ERR_STR, NULL, EX_COMM_STR_OPT_MODE);
            goto err;
    }

    ptCfg->uPeriod.tFreq.un32Freq = (uint32_t)atoi(pn8Argv[++un8Arg]);
    
    un8Arg++;
    if(strncmp(pn8Argv[un8Arg++], "-saddr", 6) == 0)
    {
        sscanf(pn8Argv[un8Arg++], "%X", &un32SlvAddr);
        ptCfg->un8OwnSlvAddr = (uint8_t)un32SlvAddr;
        if(strncmp(pn8Argv[un8Arg], "-gc", 3) == 0)
        {
            un8Arg++;
            ptCfg->bSaGcEnable = true;
        }

    }

    if(strncmp(pn8Argv[un8Arg++], "-saddr2", 7) == 0)
    {
        sscanf(pn8Argv[un8Arg++], "%X", &un32SlvAddr);
        ptCfg->un8OwnSlvAddr2 = (uint8_t)un32SlvAddr;
        if(strncmp(pn8Argv[un8Arg], "-gc2", 4) == 0)
        {
            un8Arg++;
            ptCfg->bSa2GcEnable = true;
        }
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_I2C_Init(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    I2C_ID_e eId = I2C_ID_0;

    eDbgStatus = EX_I2C_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_I2C_Init(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_I2C_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_INIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_I2C_Uninit(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    I2C_ID_e eId = I2C_ID_0;

    eDbgStatus = EX_I2C_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_I2C_Uninit(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_I2C_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_UNINIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_I2C_SetConfig(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus;
    I2C_ID_e eId = I2C_ID_0;
    I2C_CFG_t tCfg;
    I2C_OPS_e eOps = I2C_OPS_POLL;
     
    eDbgStatus = EX_I2C_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    memset(&tCfg, 0x00, sizeof(I2C_CFG_t));

    eDbgStatus = EX_I2C_GetConfig(n32Argc, pn8Argv, &tCfg, &eOps);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_I2C_SetConfig(eId, &tCfg);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_I2C_ERR_STR, "Cfg", EX_COMM_STR_OPT_MAX);
        }

        goto err;
    }

    s_tI2CContext[eId].eId = eId;
 
    s_eOps = eOps;

    eErr = HAL_I2C_SetIRQ(eId, s_eOps, EX_I2C_IRQHandler, &s_tI2CContext[eId], EX_I2C_IRQ_PRIO);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_I2C_ERR_STR, "IRQ", EX_COMM_STR_OPT_MAX);
        }

        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_I2C_SetBuffer(int32_t n32Argc, char *pn8Argv[])
{
    uint8_t un8Arg = 2, un8BufLen = 0;
    uint32_t un32Data = 0, i = 0;

    un8BufLen = (uint8_t)atoi(pn8Argv[un8Arg++]);

    if(un8BufLen > EX_I2C_DATA_LEN)
    {
        LOG(" %s len too many(max:%d)\n", EX_I2C_ERR_STR, EX_I2C_DATA_LEN);
        return DEBUG_CMD_INVALID;
    }
 
    for(i = 0; i < un8BufLen; i++)
    {
        sscanf(pn8Argv[un8Arg++], "%X", &un32Data);
        s_aun8EX_TxData[i] = un32Data;
    }

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_I2C_Tx(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    I2C_ID_e eId;
    uint8_t un8Arg = 2;
    uint32_t un32TxLen = 0, un32SlvAddr = 0;
    bool bEnForcePoll = false;

    eDbgStatus = EX_I2C_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    un32TxLen = (uint32_t)atoi(pn8Argv[un8Arg++]);

    if(un32TxLen > EX_I2C_DATA_LEN || eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    if(strncmp(pn8Argv[un8Arg], "-saddr", 6) == 0)
    {
        un8Arg++;
        sscanf(pn8Argv[un8Arg++], "%X", &un32SlvAddr);
    }

    if(strncmp(pn8Argv[un8Arg], "-poll", 5) == 0)
    {
        un8Arg++;
        bEnForcePoll = true;
    }

    s_tI2CContext[eId].eId = eId;

    LOG("%s (%d) tx %s\n", EX_I2C_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_START]);

    eErr = HAL_I2C_Transmit(eId, (uint8_t)un32SlvAddr, s_aun8EX_TxData, un32TxLen, bEnForcePoll);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    if(s_eOps == I2C_OPS_POLL || bEnForcePoll == true)
    {
        LOG("%s (%d) tx complete\n", EX_I2C_LOG_STR, eId);
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_I2C_Rx(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    I2C_ID_e eId;
    uint8_t un8Arg = 2;
    uint32_t un32RxLen = 0, un32SlvAddr = 0, i = 0;
    bool bEnForcePoll = false;

    eDbgStatus = EX_I2C_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    un32RxLen = (uint32_t)atoi(pn8Argv[un8Arg++]);

    if(un32RxLen > EX_I2C_DATA_LEN || eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    if(strncmp(pn8Argv[un8Arg], "-saddr", 6) == 0)
    {
        un8Arg++;
        sscanf(pn8Argv[un8Arg++], "%X", &un32SlvAddr);
    }

    if(strncmp(pn8Argv[un8Arg], "-poll", 5) == 0)
    {
        un8Arg++;
        bEnForcePoll = true;
    }

    s_tI2CContext[eId].eId = eId;
    memset(s_aun8EX_RxData, 0x00, sizeof(s_aun8EX_RxData));

    LOG("%s (%d) rx %s\n", EX_I2C_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_START]);

    eErr = HAL_I2C_Receive(eId, (uint8_t)un32SlvAddr, s_aun8EX_RxData, un32RxLen, bEnForcePoll);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    if(s_eOps == I2C_OPS_POLL || bEnForcePoll == true)
    {
        for(i = 0; i < EX_I2C_DATA_LEN; i++)
        { 
            LOG("%s rx data=0x%x\n", EX_I2C_LOG_STR, s_aun8EX_RxData[i]);
        }
        LOG("%s (%d) rx complete\n", EX_I2C_LOG_STR, eId);
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_I2C_SetSltpConfig(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus;
    I2C_ID_e eId = I2C_ID_0;
    I2C_SLTP_CFG_t tCfg;
    uint8_t un8Arg = 2;
     
    eDbgStatus = EX_I2C_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    memset(&tCfg, 0x00, sizeof(I2C_SLTP_CFG_t));

    eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], &tCfg.bEnable);
    if (eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_I2C_ERR_STR, NULL, EX_COMM_STR_OPT_ENA);
        goto err;
    }

    if(tCfg.bEnable == true)
    {
        if(strncmp(pn8Argv[un8Arg], "-intr", 5) == 0)
        {
            un8Arg++;
            tCfg.bIntrEnable = true;
        }

        tCfg.un32Timeout = (uint32_t)atoi(pn8Argv[un8Arg++]);
    }

    eErr = HAL_I2C_SetSltpConfig(eId, &tCfg);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_I2C_ERR_STR, "SltpCfg", EX_COMM_STR_OPT_MAX);
        }
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static const struct debug_cmd s_tEX_I2C_CMD[] =
{
    {"I2C", "h", EX_I2C_Help, "help"},
    {"I2C", "init", EX_I2C_Init, ""},
    {"I2C", "uninit", EX_I2C_Uninit, ""},
    {"I2C", "config", EX_I2C_SetConfig, ""},
    {"I2C", "sltp", EX_I2C_SetSltpConfig, ""},
    {"I2C", "data", EX_I2C_SetBuffer, ""},
    {"I2C", "tx", EX_I2C_Tx, ""},
    {"I2C", "rx", EX_I2C_Rx, ""}
};

void EX_I2C(void)
{
    /* Add TC commands */
    debug_cmd_init(s_tEX_I2C_CMD,DEBUG_CMD_LIST_COUNT(s_tEX_I2C_CMD));

#if defined (EXAMPLE_FIXED_PERI_MAPPING)
    /* Initialize I2C SCL port */
    HAL_PCU_SetPullUpDown((PCU_ID_e)I2C0_SCL_PORT,(PCU_PIN_ID_e)I2C0_SCL_PORT_ID,PCU_PUPD_UP);
    HAL_PCU_SetAltMode((PCU_ID_e)I2C0_SCL_PORT,(PCU_PIN_ID_e)I2C0_SCL_PORT_ID,(PCU_ALT_e)I2C0_SCL_MUX_ID);
    /* Initialize I2C SDA port */
    HAL_PCU_SetPullUpDown((PCU_ID_e)I2C0_SDA_PORT,(PCU_PIN_ID_e)I2C0_SDA_PORT_ID,PCU_PUPD_UP);
    HAL_PCU_SetAltMode((PCU_ID_e)I2C0_SDA_PORT,(PCU_PIN_ID_e)I2C0_SDA_PORT_ID,(PCU_ALT_e)I2C0_SDA_MUX_ID);
#endif
}

#endif
/* --------------------------------- End Of File ------------------------------ */
