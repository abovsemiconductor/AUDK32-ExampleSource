/**
 *******************************************************************************
 * @file        lpuart.c
 * @author      ABOV R&D Division
 * @brief       LPLPUART Example Code
 *
 * Copyright 2024 ABOV Semiconductor Co.,Ltd. All rights reserved.
 *
 * This file is licensed under terms that are found in the LICENSE file
 * located at Document directory.
 * If this file is delivered or shared without applicable license terms,
 * the terms of the BSD-3-Clause license shall be applied.
 * Reference: https://opensource.org/licenses/BSD-3-Clause
 ******************************************************************************/

#include "abov_example_config.h"

#if defined(LPUART_TC)
#include "abov_config.h"
#include "ex_configs.h"
#include "ex_common.h"
#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "hal_scu.h"
#include "hal_scu_clk.h"
#include "hal_pcu.h"
#include "hal_lpuart.h"

#if !defined(_LPUART)
#error "This chipset did not support this example."
#endif

#define EX_LPUART_STR "LPUART"
#define EX_LPUART_LOG_STR "LPUART :"
#define EX_LPUART_ERR_STR "[E]LPUART :"
#define EX_LPUART_TX_IRQ_PRIO 3
#define EX_LPUART_RX_IRQ_PRIO 0
#define EX_LPUART_DATA_LEN   16
#define EX_LPUART_MAX_NUM  (CONFIG_LPUART_MAX_COUNT - 1)

extern const char *pcCommStr[];
extern const char *pcCmdStr[];
extern const char *pcValStr[];
extern const char *pcOptStr[];

static LPUART_Context_t s_tLPUARTContext[CONFIG_LPUART_MAX_COUNT];
static uint8_t s_aun8EX_TxData[EX_LPUART_DATA_LEN];
static uint8_t s_aun8EX_RxData[EX_LPUART_DATA_LEN];
static LPUART_OPS_e s_eOps = LPUART_OPS_POLL;

static enum debug_cmd_status EX_LPUART_Help(int32_t n32Argc, char *pn8Argv[])
{
    uint32_t un32ShowOpt = 0;
    EX_COMM_STR_OPT_e eOpt[2];

    EX_COMMON_SetShowModuleInfo(EX_LPUART_STR, CONFIG_LPUART_MAX_COUNT);

    un32ShowOpt = EX_COMMON_GetShowOpt(pn8Argv[1]);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_INIT, NULL, EX_LPUART_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_UNINIT, NULL, EX_LPUART_MAX_NUM, eOpt, 0, NULL);

    eOpt[0] = EX_COMM_STR_OPT_SRC; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CLK, NULL, EX_LPUART_MAX_NUM, eOpt, 1, NULL);
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_SRC, EX_COMM_STR_VAL_CLKPATH, NULL);
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MCCR, EX_COMM_STR_VAL_MAX, "[mccr] [div]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MCCR, EX_COMM_STR_VAL_CLKSRC, "/m(mclk)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_DIV, EX_COMM_STR_VAL_N_255, NULL);
    }

    eOpt[0] = EX_COMM_STR_OPT_OPS; 
    eOpt[1] = EX_COMM_STR_OPT_DA; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CONFIG, NULL, EX_LPUART_MAX_NUM, eOpt, 2, "[parity] [stop] [baud] [-rto]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_OPS, EX_COMM_STR_VAL_OPS, "/d(dma)/n(nmi)/m(nmi dma)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_DA, EX_COMM_STR_VAL_MAX, "5/6/7/8");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "o(odd)/e(even)/n(none)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "stop: 1/2");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "baud: 0~N");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-rto: [en] [en(intr)] [cnt]");
    }
    eOpt[0] = EX_COMM_STR_OPT_CNT;
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "data", EX_LPUART_MAX_NUM, eOpt, 1, "[da ...]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_CNT, EX_COMM_STR_VAL_N_NUM, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_DA, EX_COMM_STR_VAL_MAX, "max 16 bytes (hexa and space (delimitor))");
    }

    eOpt[0] = EX_COMM_STR_OPT_CNT;
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "tx", EX_LPUART_MAX_NUM, eOpt, 1, "[-poll]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_CNT, EX_COMM_STR_VAL_MAX, "1~16 (tx data cnt)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-poll: en-force polling operation");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "rx", EX_LPUART_MAX_NUM, eOpt, 1, "[-poll]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_CNT, EX_COMM_STR_VAL_MAX, "1~16 (rx data cnt)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-poll: en-force polling operation");
    }

    return DEBUG_CMD_SUCCESS;
}

static void EX_LPUART_IRQHandler(uint32_t un32Event, void *pContext)
{
    uint32_t i = 0;

    if(un32Event & LPUART_EVENT_TX_DONE)
    {
        LOG("%s (%d) tx done\n", EX_LPUART_LOG_STR, ((LPUART_Context_t *)pContext)->eId);
    }

    if(un32Event & LPUART_EVENT_RX_DONE)
    {
        LOG("%s (%d) rx done\n", EX_LPUART_LOG_STR, ((LPUART_Context_t *)pContext)->eId);
        for(i = 0; i < EX_LPUART_DATA_LEN; i++)
        {
            LOG("%s rx Data=0x%x\n", EX_LPUART_LOG_STR, s_aun8EX_RxData[i]);
        }
    }

    if(un32Event & LPUART_EVENT_RTO)
    {
        LOG("%s (%d) rx timeout\n", EX_LPUART_LOG_STR, ((LPUART_Context_t *)pContext)->eId);
    }
}

static enum debug_cmd_status EX_LPUART_GetId(int32_t n32Argc, char *pn8Argv[], LPUART_ID_e *peId)
{
    uint8_t un8Data = 0;
    if(n32Argc == 1)
    {
        EX_COMMON_SetShowModuleLog(EX_LPUART_ERR_STR, (char *)pcOptStr[EX_COMM_STR_OPT_ARG], EX_COMM_STR_OPT_MAX);
        return DEBUG_CMD_INVALID;
    }

    un8Data = atoi(pn8Argv[1]); 
    if(un8Data >= CONFIG_LPUART_MAX_COUNT)
    {
        LOG("%s Max Chan %d\n", EX_LPUART_ERR_STR, CONFIG_LPUART_MAX_COUNT);
        return DEBUG_CMD_INVALID;
    }

    *peId = (LPUART_ID_e)un8Data;

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_LPUART_GetClkConfig(int32_t n32Argc, char *pn8Argv[], LPUART_CLK_CFG_t *ptClkCfg)
{
    uint8_t un8Arg = 2;

    switch (pn8Argv[un8Arg++][0])
    {
        case 'p':
            ptClkCfg->eClk = LPUART_CLK_PCLK;
            break;
        case 'm':
            ptClkCfg->eClk = LPUART_CLK_MCCR;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_LPUART_ERR_STR, NULL, EX_COMM_STR_OPT_MCCR);
            goto err;
    }

    if(ptClkCfg->eClk == LPUART_CLK_MCCR)
    {
        if (pn8Argv[un8Arg][0] == 'l')
            ptClkCfg->eMccr = LPUART_CLK_MCCR_LSI;
        else if (pn8Argv[un8Arg][0] == 's')
            ptClkCfg->eMccr = LPUART_CLK_MCCR_LSE;
        else if (pn8Argv[un8Arg][0] == 'm')
            ptClkCfg->eMccr = LPUART_CLK_MCCR_MCLK;
        else if (pn8Argv[un8Arg][0] == 'h')
            ptClkCfg->eMccr = LPUART_CLK_MCCR_HSI;
        else if (pn8Argv[un8Arg][0] == 'e')
            ptClkCfg->eMccr = LPUART_CLK_MCCR_HSE;
        else if (pn8Argv[un8Arg][0] == 'p')
            ptClkCfg->eMccr = LPUART_CLK_MCCR_PLL;
        else
        {
            EX_COMMON_SetShowModuleLog(EX_LPUART_ERR_STR, NULL, EX_COMM_STR_OPT_MCCR);
            goto err;
        }

        un8Arg++;
        ptClkCfg->un8MccrDiv = atoi(pn8Argv[un8Arg++]);
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_LPUART_GetConfig(int32_t n32Argc, char *pn8Argv[], LPUART_CFG_t *ptCfg, LPUART_OPS_e *peOps)
{
    uint8_t un8Arg = 2;
    enum debug_cmd_status eDbgStatus;

    if (pn8Argv[un8Arg][0] == 'p')
        *peOps = LPUART_OPS_POLL;
    else if (pn8Argv[un8Arg][0] == 'i')
        *peOps = LPUART_OPS_INTR;
    else if (pn8Argv[un8Arg][0] == 'd')
        *peOps = LPUART_OPS_INTR_DMA;
    else if (pn8Argv[un8Arg][0] == 'n')
        *peOps = LPUART_OPS_NMI;
    else if (pn8Argv[un8Arg][0] == 'm')
        *peOps = LPUART_OPS_NMI_DMA;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_LPUART_ERR_STR, NULL, EX_COMM_STR_OPT_OPS);
        goto err;
    }

    un8Arg++;
    if (pn8Argv[un8Arg][0] == '5')
        ptCfg->eData = LPUART_DATA_5;
    else if (pn8Argv[un8Arg][0] == '6')
        ptCfg->eData = LPUART_DATA_6;
    else if (pn8Argv[un8Arg][0] == '7')
        ptCfg->eData = LPUART_DATA_7;
    else if (pn8Argv[un8Arg][0] == '8')
        ptCfg->eData = LPUART_DATA_8;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_LPUART_ERR_STR, "[data]", EX_COMM_STR_OPT_MAX);
        goto err;
    }

    un8Arg++;
    if (pn8Argv[un8Arg][0] == 'n')
        ptCfg->eParity = LPUART_PARITY_NONE;
    else if (pn8Argv[un8Arg][0] == 'o')
        ptCfg->eParity = LPUART_PARITY_ODD;
    else if (pn8Argv[un8Arg][0] == 'e')
        ptCfg->eParity = LPUART_PARITY_EVEN;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_LPUART_ERR_STR, "[parity]", EX_COMM_STR_OPT_MAX);
        goto err;
    }

    switch (pn8Argv[++un8Arg][0])
    {
        case '1':
            ptCfg->eStop = LPUART_STOP_1;
            break;
        case '2':
            ptCfg->eStop = LPUART_STOP_2;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_LPUART_ERR_STR, "[stop]", EX_COMM_STR_OPT_MAX);
            goto err;
    }

    ptCfg->un32BaudRate = (uint32_t)atoi(pn8Argv[++un8Arg]);

    if(strncmp(pn8Argv[++un8Arg], "-rto", 4) == 0)
    {
        eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[++un8Arg][0], &ptCfg->tRtoCfg.bEnable);
        if (eDbgStatus != DEBUG_CMD_SUCCESS)
        {
            EX_COMMON_SetShowModuleLog(EX_LPUART_ERR_STR, "-rto[en]", EX_COMM_STR_OPT_MAX);
            goto err;
        }

        eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[++un8Arg][0], &ptCfg->tRtoCfg.bIntrEnable);
        if (eDbgStatus != DEBUG_CMD_SUCCESS)
        {
            EX_COMMON_SetShowModuleLog(EX_LPUART_ERR_STR, "-rto[en(intr)]", EX_COMM_STR_OPT_MAX);
            goto err;
        }

        ptCfg->tRtoCfg.un32TimeOut = (uint32_t)atoi(pn8Argv[++un8Arg]);
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_LPUART_Init(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    LPUART_ID_e eId = LPUART_ID_0;

    eDbgStatus = EX_LPUART_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_LPUART_Init(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_LPUART_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_INIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_LPUART_Uninit(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    LPUART_ID_e eId = LPUART_ID_0;

    eDbgStatus = EX_LPUART_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_LPUART_Uninit(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_LPUART_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_UNINIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_LPUART_SetClkConfig(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    LPUART_ID_e eId;
    LPUART_CLK_CFG_t tClkCfg; 

    eDbgStatus = EX_LPUART_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    memset(&tClkCfg, 0x00, sizeof(LPUART_CLK_CFG_t));

    eDbgStatus = EX_LPUART_GetClkConfig(n32Argc, pn8Argv, &tClkCfg);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_LPUART_SetClkConfig(eId, &tClkCfg);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_LPUART_ERR_STR, "ClkCfg", EX_COMM_STR_OPT_MAX);
        }

        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_LPUART_SetConfig(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus;
    LPUART_ID_e eId = LPUART_ID_0;
    LPUART_OPS_e eOps = LPUART_OPS_MAX;
    LPUART_CFG_t tCfg;
     
    eDbgStatus = EX_LPUART_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    memset(&tCfg, 0x00, sizeof(LPUART_CFG_t));

    eDbgStatus = EX_LPUART_GetConfig(n32Argc, pn8Argv, &tCfg, &eOps);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_LPUART_SetConfig(eId, &tCfg);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_LPUART_ERR_STR, "Cfg", EX_COMM_STR_OPT_MAX);
        }
        goto err;
    }

    s_tLPUARTContext[eId].eId = eId;

    s_eOps = eOps;

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;

}

static enum debug_cmd_status EX_LPUART_SetBuffer(int32_t n32Argc, char *pn8Argv[])
{
    uint8_t un8Arg = 2, un8BufLen = 0;
    uint32_t un32Data = 0, i = 0;

    un8BufLen = (uint8_t)atoi(pn8Argv[un8Arg++]);

    if(un8BufLen > EX_LPUART_DATA_LEN)
    {
        LOG(" %s len too many(max:%d)\n", EX_LPUART_ERR_STR, EX_LPUART_DATA_LEN);
        return DEBUG_CMD_INVALID;
    }
 
    for(i = 0; i < un8BufLen; i++)
    {
        sscanf(pn8Argv[un8Arg++], "%X", &un32Data);
        s_aun8EX_TxData[i] = un32Data;
    }

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_LPUART_Tx(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    LPUART_ID_e eId;
    uint8_t un32TxLen = 0, un8Arg = 2;
    bool bEnForcePoll = false;

    eDbgStatus = EX_LPUART_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    un32TxLen = (uint32_t)atoi(pn8Argv[un8Arg++]);

    if(un32TxLen > EX_LPUART_DATA_LEN || eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    if(strncmp(pn8Argv[un8Arg], "-poll", 5) == 0)
    {
        un8Arg++;
        bEnForcePoll = true;
    }

    s_tLPUARTContext[eId].eId = eId;

    LOG("%s (%d) tx %s\n", EX_LPUART_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_START]);

    eErr = HAL_LPUART_SetIRQ(eId, s_eOps, EX_LPUART_IRQHandler, &s_tLPUARTContext[eId], EX_LPUART_TX_IRQ_PRIO);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_LPUART_ERR_STR, "IRQ", EX_COMM_STR_OPT_MAX);
        }

        goto err;
    }

    eErr = HAL_LPUART_Transmit(eId, s_aun8EX_TxData, un32TxLen, bEnForcePoll);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    LOG("%s (%d) tx complete\n", EX_LPUART_LOG_STR, eId);

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_LPUART_Rx(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus;
    LPUART_ID_e eId;
    uint8_t un32RxLen = 0, un8Arg = 2;
    uint32_t i=0;
    bool bEnForcePoll = false;

    eDbgStatus = EX_LPUART_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    un32RxLen = (uint32_t)atoi(pn8Argv[un8Arg++]);

    if(un32RxLen > EX_LPUART_DATA_LEN || eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    if(strncmp(pn8Argv[un8Arg], "-poll", 5) == 0)
    {
        un8Arg++;
        bEnForcePoll = true;
    }

    s_tLPUARTContext[eId].eId = eId;

    LOG("%s (%d) rx %s\n", EX_LPUART_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_START]);

    eErr = HAL_LPUART_SetIRQ(eId, s_eOps, EX_LPUART_IRQHandler, &s_tLPUARTContext[eId], EX_LPUART_RX_IRQ_PRIO);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_LPUART_ERR_STR, "IRQ", EX_COMM_STR_OPT_MAX);
        }

        goto err;
    }

    eErr = HAL_LPUART_Receive(eId, s_aun8EX_RxData, un32RxLen, bEnForcePoll);
    if (eErr != HAL_ERR_OK)
    {
        LOG("%s (%d) rx error (%d)\n", EX_LPUART_LOG_STR, eId, eErr);
        goto err;
    }

    if(s_eOps == LPUART_OPS_POLL || bEnForcePoll == true)
    {
        for(i = 0; i < un32RxLen; i++)
        {
            LOG("%s rx data=0x%x\n", EX_LPUART_LOG_STR, s_aun8EX_RxData[i]);
        }
        LOG("%s (%d) rx complete\n", EX_LPUART_LOG_STR, eId);
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_LPUART_Abort(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    LPUART_ID_e eId;
    uint8_t un8Arg = 2;

    eDbgStatus = EX_LPUART_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    (void)HAL_LPUART_Abort(eId);
    LOG("%s (%d) abort\n", EX_LPUART_LOG_STR, eId);

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static const struct debug_cmd s_tEX_LPUART_CMD[] =
{
    {"LPUART", "h", EX_LPUART_Help, "help"},
    {"LPUART", "init", EX_LPUART_Init, ""},
    {"LPUART", "uninit", EX_LPUART_Uninit, ""},
    {"LPUART", "clk", EX_LPUART_SetClkConfig, ""},
    {"LPUART", "config", EX_LPUART_SetConfig, ""},
    {"LPUART", "data", EX_LPUART_SetBuffer, ""},
    {"LPUART", "tx", EX_LPUART_Tx, ""},
    {"LPUART", "rx", EX_LPUART_Rx, ""},
    {"LPUART", "abort", EX_LPUART_Abort, ""},
};

void EX_LPUART(void)
{
    /* Add TC commands */
    debug_cmd_init(s_tEX_LPUART_CMD,DEBUG_CMD_LIST_COUNT(s_tEX_LPUART_CMD));
}

#endif
/* --------------------------------- End Of File ------------------------------ */
