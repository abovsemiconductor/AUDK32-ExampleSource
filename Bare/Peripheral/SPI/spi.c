/**
 *******************************************************************************
 * @file        spi.c
 * @author      ABOV R&D Division
 * @brief       SPI Example Code
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

#if defined(SPI_TC)
#include "abov_config.h"
#include "ex_configs.h"
#include "ex_common.h"
#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "hal_scu.h"
#include "hal_scu_clk.h"
#include "hal_pcu.h"
#include "hal_spi.h"

#if !defined(_SPI)
#error "This chipset did not support this example."
#endif

#define EX_SPI_STR "SPI"
#define EX_SPI_LOG_STR "SPI :"
#define EX_SPI_ERR_STR "[E]SPI :"
#define EX_SPI_TX_IRQ_PRIO 3
#define EX_SPI_RX_IRQ_PRIO 0
#define EX_SPI_DATA_LEN    16
#define EX_SPI_MAX_NUM  (CONFIG_SPI_MAX_COUNT - 1)

extern const char *pcCommStr[];
extern const char *pcCmdStr[];
extern const char *pcValStr[];
extern const char *pcOptStr[];

static SPI_OPS_e s_eOps = SPI_OPS_MAX;
static SPI_Context_t s_tSPIContext[CONFIG_SPI_MAX_COUNT];
static uint8_t s_aun8EX_TxData[EX_SPI_DATA_LEN];
static uint8_t s_aun8EX_RxData[EX_SPI_DATA_LEN];

static enum debug_cmd_status EX_SPI_Help(int32_t n32Argc, char *pn8Argv[])
{
    uint32_t un32ShowOpt = 0;
    EX_COMM_STR_OPT_e eOpt[3];

    EX_COMMON_SetShowModuleInfo(EX_SPI_STR, CONFIG_SPI_MAX_COUNT);

    un32ShowOpt = EX_COMMON_GetShowOpt(pn8Argv[1]);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_INIT, NULL, EX_SPI_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_UNINIT, NULL, EX_SPI_MAX_NUM, eOpt, 0, NULL);

    eOpt[0] = EX_COMM_STR_OPT_OPS; 
    eOpt[1] = EX_COMM_STR_OPT_MODE; 
    eOpt[2] = EX_COMM_STR_OPT_DA; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CLK, NULL, EX_SPI_MAX_NUM, eOpt, 3, "[clkmode] [order] [sspol] [bdr] [-delay]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_OPS, EX_COMM_STR_VAL_OPS, "/d(dma)/n(nmi)/m(nmi dma)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MODE, EX_COMM_STR_VAL_MAX, "m(master)/s(slave)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_DA, EX_COMM_STR_VAL_MAX, "8/9/16/17");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "clkmode: 0(pol:0,pha:0)/1(pol:0,pha:1)/2(pol:1,pha:0)/3(pol:1,pha:1)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "order: m(msb)/l(lsb)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "sspol: l(low)/h(high");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "bdr: 0~N");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-delay: [start] [stop] [burst]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "start: 1~N");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "stop: 1~N");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "burst: 1~N");
    }
    eOpt[0] = EX_COMM_STR_OPT_CNT;
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "data", EX_SPI_MAX_NUM, eOpt, 1, "[da ...]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_CNT, EX_COMM_STR_VAL_N_NUM, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_DA, EX_COMM_STR_VAL_MAX, "max 16 bytes (hexa and space (delimitor))");
    }

    eOpt[0] = EX_COMM_STR_OPT_CNT;
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "tx", EX_SPI_MAX_NUM, eOpt, 1, "[-poll]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_CNT, EX_COMM_STR_VAL_MAX, "1~16 (tx data cnt)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-poll: en-force polling operation");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "rx", EX_SPI_MAX_NUM, eOpt, 1, "[-poll]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_CNT, EX_COMM_STR_VAL_MAX, "1~16 (rx data cnt)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-poll: en-force polling operation");
    }

    return DEBUG_CMD_SUCCESS;
}

static void EX_SPI_IRQHandler(uint32_t un32Event, void *pContext)
{
    uint32_t i = 0;

    if(un32Event & SPI_EVENT_UNDERRUN)
    {
        LOG("%s (%d) U evt fire\n", EX_SPI_LOG_STR, ((SPI_Context_t *)pContext)->eId);
    }

    if(un32Event & SPI_EVENT_OVERRUN)
    {
        LOG("%s (%d) O evt fire\n", EX_SPI_LOG_STR, ((SPI_Context_t *)pContext)->eId);
    }

    if(un32Event & SPI_EVENT_TX_DONE)
    {
        LOG("%s (%d) tx done\n", EX_SPI_LOG_STR, ((SPI_Context_t *)pContext)->eId);
    }

    if(un32Event & SPI_EVENT_RX_DONE)
    {
        LOG("\n");
        for(i = 0; i < EX_SPI_DATA_LEN;i++)
            LOG("%s rx Data=0x%x\n", EX_SPI_LOG_STR, s_aun8EX_RxData[i]);

        LOG("%s (%d) rx done\n", EX_SPI_LOG_STR, ((SPI_Context_t *)pContext)->eId);
    }
}

static enum debug_cmd_status EX_SPI_GetId(int32_t n32Argc, char *pn8Argv[], SPI_ID_e *peId)
{
    uint8_t un8Data = 0;
    if(n32Argc == 1)
    {
        EX_COMMON_SetShowModuleLog(EX_SPI_ERR_STR, (char *)pcOptStr[EX_COMM_STR_OPT_ARG], EX_COMM_STR_OPT_MAX);
        return DEBUG_CMD_INVALID;
    }

    un8Data = atoi(pn8Argv[1]); 
    if(un8Data >= CONFIG_SPI_MAX_COUNT)
    {
        LOG("%s Max Chan %d\n", EX_SPI_ERR_STR, CONFIG_SPI_MAX_COUNT);
        return DEBUG_CMD_INVALID;
    }

    *peId = (SPI_ID_e)un8Data;

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_SPI_GetConfig(int32_t n32Argc, char *pn8Argv[], SPI_CFG_t *ptCfg, SPI_OPS_e *peOps)
{
    uint8_t un8Arg = 2, un8Data = 0;

    if (pn8Argv[un8Arg][0] == 'p')
        *peOps = SPI_OPS_POLL;
    else if (pn8Argv[un8Arg][0] == 'i')
        *peOps = SPI_OPS_INTR;
    else if (pn8Argv[un8Arg][0] == 'd')
        *peOps = SPI_OPS_INTR_DMA;
    else if (pn8Argv[un8Arg][0] == 'n')
        *peOps = SPI_OPS_NMI;
    else if (pn8Argv[un8Arg][0] == 'm')
        *peOps = SPI_OPS_NMI_DMA;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_SPI_ERR_STR, NULL, EX_COMM_STR_OPT_OPS);
        goto err;
    }
  
    switch (pn8Argv[++un8Arg][0])
    {
        case 'm':
            ptCfg->eMode = SPI_MODE_MASTER;
            break;
        case 's':
            ptCfg->eMode = SPI_MODE_SLAVE;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_SPI_ERR_STR, NULL, EX_COMM_STR_OPT_MODE);
            goto err;
    }
   
    un8Data = ((uint8_t)atoi(pn8Argv[++un8Arg]));
    if (un8Data == 8)
        ptCfg->eData = SPI_DATA_8;
    else if (un8Data == 9)
        ptCfg->eData = SPI_DATA_9;
    else if (un8Data == 16)
        ptCfg->eData = SPI_DATA_16;
    else if (un8Data == 17)
        ptCfg->eData = SPI_DATA_17;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_SPI_ERR_STR, "[data]", EX_COMM_STR_OPT_MAX);
        goto err;
    }

    un8Arg++;
    if (pn8Argv[un8Arg][0] == '0')
        ptCfg->eClkMode = SPI_CLK_MODE_CPOL_0_CPHA_0;
    else if (pn8Argv[un8Arg][0] == '1')
        ptCfg->eClkMode = SPI_CLK_MODE_CPOL_0_CPHA_1;
    else if (pn8Argv[un8Arg][0] == '2')
        ptCfg->eClkMode = SPI_CLK_MODE_CPOL_1_CPHA_0;
    else if (pn8Argv[un8Arg][0] == '3')
        ptCfg->eClkMode = SPI_CLK_MODE_CPOL_1_CPHA_1;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_SPI_ERR_STR, "[clkmode]", EX_COMM_STR_OPT_MAX);
        goto err;
    }
    
    switch (pn8Argv[++un8Arg][0])
    {
        case 'm':
            ptCfg->eBitOrder = SPI_BIT_ORDER_MSB;
            break;
        case 'l':
            ptCfg->eBitOrder = SPI_BIT_ORDER_LSB;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_SPI_ERR_STR, "[order]", EX_COMM_STR_OPT_MAX);
            goto err;
    } 

    switch (pn8Argv[++un8Arg][0])
    {
        case 'l':
            ptCfg->eSSPol = SPI_SS_POL_LOW;
            break;
        case 'h':
            ptCfg->eSSPol = SPI_SS_POL_HIGH;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_SPI_ERR_STR, "[sspol]", EX_COMM_STR_OPT_MAX);
            goto err;
    } 

    ptCfg->un16BaudRate = (uint16_t)atoi(pn8Argv[++un8Arg]);

    if(strncmp(pn8Argv[++un8Arg], "-ss", 3) == 0)
    {
        switch (pn8Argv[++un8Arg][0])
        {
            case 'a':
                ptCfg->bSSManual = false;
                break;
            case 'm':
                ptCfg->bSSManual = true;
                break;
            default:
                EX_COMMON_SetShowModuleLog(EX_SPI_ERR_STR, "[manual]", EX_COMM_STR_OPT_MAX);
                goto err;
        }
    }

    if(strncmp(pn8Argv[++un8Arg], "-delay", 7) == 0)
    {
        un8Arg++;
        if((n32Argc - un8Arg) != 3)
        {
            EX_COMMON_SetShowModuleLog(EX_SPI_ERR_STR, "[-delay]", EX_COMM_STR_OPT_MAX);
        }
        else
        {
            ptCfg->un8DelayStart = (uint8_t)atoi(pn8Argv[un8Arg++]);
            ptCfg->un8DelayStop = (uint8_t)atoi(pn8Argv[un8Arg++]);
            ptCfg->un8DelayBurst = (uint8_t)atoi(pn8Argv[un8Arg++]);
        }
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_SPI_Init(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    SPI_ID_e eId = SPI_ID_0;

    eDbgStatus=EX_SPI_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_SPI_Init(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_SPI_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_INIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_SPI_Uninit(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    SPI_ID_e eId = SPI_ID_0;

    eDbgStatus=EX_SPI_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_SPI_Uninit(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_SPI_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_UNINIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_SPI_SetConfig(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus;
    SPI_ID_e eId = SPI_ID_0;
    SPI_CFG_t tCfg;
    SPI_OPS_e eOps = SPI_OPS_POLL;
     
    eDbgStatus = EX_SPI_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    memset(&tCfg, 0x00, sizeof(SPI_CFG_t));

    eDbgStatus=EX_SPI_GetConfig(n32Argc, pn8Argv, &tCfg, &eOps);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_SPI_SetConfig(eId, &tCfg);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_SPI_ERR_STR, "Cfg", EX_COMM_STR_OPT_MAX);
        }

        goto err;
    }

    s_tSPIContext[eId].eId = eId;
  
    s_eOps = eOps;


    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_SPI_SetBuffer(int32_t n32Argc, char *pn8Argv[])
{
    uint8_t un8Arg = 2, un8BufLen = 0;
    uint32_t un32Data = 0, i = 0;

    un8BufLen = (uint8_t)atoi(pn8Argv[un8Arg++]);

    if(un8BufLen > EX_SPI_DATA_LEN)
    {
        LOG(" %s len too many(max:%d)\n", EX_SPI_ERR_STR, EX_SPI_DATA_LEN);
        return DEBUG_CMD_INVALID;
    }
 
    for(i = 0; i < un8BufLen; i++)
    {
        sscanf(pn8Argv[un8Arg++], "%X", &un32Data);
        s_aun8EX_TxData[i] = un32Data;
    }

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_SPI_Tx(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    SPI_ID_e eId;
    uint8_t un8Arg = 2;
    uint32_t un32TxLen = 0;
    bool bEnForcePoll = false;

    eDbgStatus = EX_SPI_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    un32TxLen = (uint32_t)atoi(pn8Argv[un8Arg++]);

    if(un32TxLen > EX_SPI_DATA_LEN || eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    if(strncmp(pn8Argv[un8Arg], "-poll", 5) == 0)
    {
        un8Arg++;
        bEnForcePoll = true;
    }

    s_tSPIContext[eId].eId = eId;

    LOG("%s (%d) tx %s\n", EX_SPI_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_START]);

    eErr = HAL_SPI_SetIRQ(eId, s_eOps, EX_SPI_IRQHandler, &s_tSPIContext[eId], EX_SPI_TX_IRQ_PRIO);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_SPI_ERR_STR, "IRQ", EX_COMM_STR_OPT_MAX);
        }

        goto err;
    }

    eErr = HAL_SPI_Transmit(eId, s_aun8EX_TxData, un32TxLen, bEnForcePoll);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    if(s_eOps == SPI_OPS_POLL || bEnForcePoll == true)
    {
        LOG("%s (%d) tx complete\n", EX_SPI_LOG_STR, eId);
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_SPI_Rx(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    SPI_ID_e eId;
    uint8_t un8Arg = 2;
    uint32_t un32RxLen = 0, i = 0;
    bool bEnForcePoll = false;

    eDbgStatus = EX_SPI_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    un32RxLen = (uint32_t)atoi(pn8Argv[un8Arg++]);

    if(un32RxLen > EX_SPI_DATA_LEN || eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    if(strncmp(pn8Argv[un8Arg], "-poll", 5) == 0)
    {
        un8Arg++;
        bEnForcePoll = true;
    }

    s_tSPIContext[eId].eId = eId;

    LOG("%s (%d) rx %s\n", EX_SPI_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_START]);

    memset(s_aun8EX_RxData, 0x00, sizeof(s_aun8EX_RxData));

    eErr = HAL_SPI_SetIRQ(eId, s_eOps, EX_SPI_IRQHandler, &s_tSPIContext[eId], EX_SPI_RX_IRQ_PRIO);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_SPI_ERR_STR, "IRQ", EX_COMM_STR_OPT_MAX);
        }

        goto err;
    }

    eErr = HAL_SPI_Receive(eId, s_aun8EX_RxData, un32RxLen, bEnForcePoll);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    if(s_eOps == SPI_OPS_POLL || bEnForcePoll == true)
    {
        for(i = 0; i < EX_SPI_DATA_LEN; i++)
        {
            LOG("%s rx data=0x%x\n", EX_SPI_LOG_STR, s_aun8EX_RxData[i]);
        }
        LOG("%s (%d) rx complete\n", EX_SPI_LOG_STR, eId);
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

#if defined (EXTRN_HAL_SPI_TRANSCEIVE)
static enum debug_cmd_status EX_SPI_Trans(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    SPI_ID_e eId;
    uint8_t un8Arg = 2;
    uint32_t un32TransLen = 0;

    eDbgStatus = EX_SPI_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    un32TransLen = (uint32_t)atoi(pn8Argv[un8Arg++]);
    if(un32TransLen > EX_SPI_DATA_LEN || eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }
    
    switch (*pn8Argv[un8Arg++])
    {
        case 'r':
            eErr = HAL_SPI_Transceive(SPI_ID_0, s_aun8EX_RxData, s_aun8EX_TxData, un32TransLen, true);
            break;

        case 't':
            eErr = HAL_SPI_Transceive(SPI_ID_0, NULL, s_aun8EX_TxData, un32TransLen, true);
            break;
    
        default:
            EX_COMMON_SetShowModuleLog(EX_SPI_ERR_STR, "[trasn-mode]", EX_COMM_STR_OPT_MAX);
            goto err;
    }

    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}
#endif

static const struct debug_cmd s_tEX_SPI_CMD[] =
{
    {"SPI", "h", EX_SPI_Help, "help"},
    {"SPI", "init", EX_SPI_Init, ""},
    {"SPI", "uninit", EX_SPI_Uninit, ""},
    {"SPI", "config", EX_SPI_SetConfig, ""},
    {"SPI", "data", EX_SPI_SetBuffer, ""},
    {"SPI", "tx", EX_SPI_Tx, ""},
    {"SPI", "rx", EX_SPI_Rx, ""},
#if defined (EXTRN_HAL_SPI_TRANSCEIVE)
    {"SPI", "trans", EX_SPI_Trans, ""}
#endif
};

void EX_SPI(void)
{
    /* Add TC commands */
    debug_cmd_init(s_tEX_SPI_CMD,DEBUG_CMD_LIST_COUNT(s_tEX_SPI_CMD));
}

#endif
/* --------------------------------- End Of File ------------------------------ */
