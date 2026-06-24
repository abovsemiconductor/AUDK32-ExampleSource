/**
 *******************************************************************************
 * @file        usart.c
 * @author      ABOV R&D Division
 * @brief       USART Example Code
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

#if defined(USART_TC)
#include "abov_config.h"
#include "ex_configs.h"
#include "ex_common.h"
#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "hal_scu.h"
#include "hal_scu_clk.h"
#include "hal_pcu.h"
#include "hal_usart.h"

#if !defined(_USART)
#error "This chipset did not support this example."
#endif

#if defined (EX_COMMON_ENABLE_CUSTOM_SSCANF)
#define sscanf(str, fmt, out) EX_COMMON_ParseByFormat((str), (fmt)[1], (uint32_t *)(out))
#endif

#define EX_USART_STR "USART"
#define EX_USART_LOG_STR "USART :"
#define EX_USART_ERR_STR "[E]USART :"
#define EX_USART_IRQ_PRIO    3
#define EX_USART_TX_IRQ_PRIO 3
#define EX_USART_RX_IRQ_PRIO 0
#define EX_USART_DATA_LEN    16
#define EX_USART_MAX_NUM  (CONFIG_USART_MAX_COUNT - 1)

extern const char *pcCommStr[];
extern const char *pcCmdStr[];
extern const char *pcValStr[];
extern const char *pcOptStr[];

static USART_OPS_e s_eOps = USART_OPS_MAX;
static USART_Context_t s_tUSARTContext[CONFIG_USART_MAX_COUNT];
static uint8_t s_aun8EX_TxData[EX_USART_DATA_LEN];
static uint8_t s_aun8EX_RxData[EX_USART_DATA_LEN];

static enum debug_cmd_status EX_USART_Help(int32_t n32Argc, char *pn8Argv[])
{
    uint32_t un32ShowOpt = 0;
    EX_COMM_STR_OPT_e eOpt[2];

    EX_COMMON_SetShowModuleInfo(EX_USART_STR, CONFIG_USART_MAX_COUNT);

    un32ShowOpt = EX_COMMON_GetShowOpt(pn8Argv[1]);

    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_INIT, NULL, EX_USART_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_UNINIT, NULL, EX_USART_MAX_NUM, eOpt, 0, NULL);

    eOpt[0] = EX_COMM_STR_OPT_OPS; 
    eOpt[1] = EX_COMM_STR_OPT_MODE; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CONFIG, NULL, EX_USART_MAX_NUM, eOpt, 2, "[baud] [-rto]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_OPS, EX_COMM_STR_VAL_OPS, "/d(dma)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MODE, EX_COMM_STR_VAL_OPS, "u(uart)/a(usrt)/s(spi)");
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "uart: [da] [parity] [stop] [ds]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_DA, EX_COMM_STR_VAL_MAX, "5/6/7/8/9");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "parity: n(none)/o(odd)/e(even)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "stop: 1/2");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "ds: n(normal)/d(double)");
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "usrt: [ms] [da] [parity] [stop] [clkpol]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "ms: m(master)/s(slave)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_DA, EX_COMM_STR_VAL_MAX, "5/6/7/8/9");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "parity: n(none)/o(odd)/e(even)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "stop: 1/2");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "clkpol: 0(Tx Rise/Rx Fall)/1(Tx Fall/Rx Rise)");
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "spi: [ms] [order] [pol] [pha] [-swap]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "ms: m(master)/s(slave)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "order: m(msb)/l(lsb)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_POL, EX_COMM_STR_VAL_LOWHIGH, NULL);
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "pha: o(odd edge)/e(even edge)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-swap: swap mosi and miso port");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "baud: 0~N");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-rto: [en] [cnt]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_ENA, EX_COMM_STR_VAL_ENDIS, NULL);
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_CNT, EX_COMM_STR_VAL_N_NUM, NULL);
    }

    eOpt[0] = EX_COMM_STR_OPT_CNT;
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "data", EX_USART_MAX_NUM, eOpt, 1, "[da ...]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_CNT, EX_COMM_STR_VAL_N_NUM, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_DA, EX_COMM_STR_VAL_MAX, "max 16 bytes (hexa and space (delimitor))");
    }

    eOpt[0] = EX_COMM_STR_OPT_CNT;
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "tx", EX_USART_MAX_NUM, eOpt, 1, "[-poll]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_CNT, EX_COMM_STR_VAL_MAX, "1~16 (tx data cnt)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-poll: en-force polling operation");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "rx", EX_USART_MAX_NUM, eOpt, 1, "[-poll]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_CNT, EX_COMM_STR_VAL_MAX, "1~16 (rx data cnt)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-poll: en-force polling operation");
    }

    return DEBUG_CMD_SUCCESS;
}

static void EX_USART_IRQHandler(uint32_t un32Event, void *pContext)
{
    uint32_t i = 0;

    if(un32Event & USART_EVENT_TX_DONE)
    {
        LOG("%s (%d) tx done\n", EX_USART_LOG_STR, ((USART_Context_t *)pContext)->eId);
    }

    if(un32Event & USART_EVENT_RX_DONE)
    {
        LOG("%s (%d) rx done\n", EX_USART_LOG_STR, ((USART_Context_t *)pContext)->eId);
        for(i = 0; i < EX_USART_DATA_LEN; i++)
        {
            LOG("%s rx Data=0x%x\n", EX_USART_LOG_STR, s_aun8EX_RxData[i]);
        }
    }

    if(un32Event & USART_EVENT_RX_TIMEOUT)
    {
        LOG("%s (%d) rx timeout\n", EX_USART_LOG_STR, ((USART_Context_t *)pContext)->eId);
    }
}

static enum debug_cmd_status EX_USART_GetDataBit(uint8_t un8Argv, char *pn8Argv[], USART_DATA_e *peDataBit)
{
    if (pn8Argv[un8Argv][0] == '5')
        *peDataBit = USART_DATA_5;
    else if (pn8Argv[un8Argv][0] == '6')
        *peDataBit = USART_DATA_6;
    else if (pn8Argv[un8Argv][0] == '7')
        *peDataBit = USART_DATA_7;
    else if (pn8Argv[un8Argv][0] == '8')
        *peDataBit = USART_DATA_8;
    else if (pn8Argv[un8Argv][0] == '9')
        *peDataBit = USART_DATA_9;
    else
    {
        return DEBUG_CMD_INVALID;
    }

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_USART_GetParity(uint8_t un8Argv, char *pn8Argv[], USART_PARITY_e *peParity)
{
    if (pn8Argv[un8Argv][0] == 'n')
        *peParity = USART_PARITY_NONE;
    else if (pn8Argv[un8Argv][0] == 'e')
        *peParity = USART_PARITY_EVEN;
    else if (pn8Argv[un8Argv][0] == 'o')
        *peParity = USART_PARITY_ODD;
    else
    {
        return DEBUG_CMD_INVALID;
    }

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_USART_GetStopBit(uint8_t un8Argv, char *pn8Argv[], USART_STOP_e *peStopBit)
{
    if (pn8Argv[un8Argv][0] == '1')
        *peStopBit = USART_STOP_1;
    else if (pn8Argv[un8Argv][0] == '2')
        *peStopBit = USART_STOP_2;
    else
    {
        return DEBUG_CMD_INVALID;
    }

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_USART_GetMsMode(uint8_t un8Argv, char *pn8Argv[], USART_MS_e *peMsMode)
{
    if (pn8Argv[un8Argv][0] == 'm')
        *peMsMode = USART_MS_MASTER;
    else if (pn8Argv[un8Argv][0] == 's')
        *peMsMode = USART_MS_SLAVE;
    else
    {
        return DEBUG_CMD_INVALID;
    }

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_USART_GetId(int32_t n32Argc, char *pn8Argv[], USART_ID_e *peId)
{
    uint8_t un8Data = 0;
    if(n32Argc == 1)
    {
        EX_COMMON_SetShowModuleLog(EX_USART_ERR_STR, (char *)pcOptStr[EX_COMM_STR_OPT_ARG], EX_COMM_STR_OPT_MAX);
        return DEBUG_CMD_INVALID;
    }

    un8Data = atoi(pn8Argv[1]); 
    if(un8Data >= CONFIG_USART_MAX_COUNT)
    {
        LOG("%s Max Chan %d\n", EX_USART_ERR_STR, CONFIG_USART_MAX_COUNT);
        return DEBUG_CMD_INVALID;
    }

    *peId = (USART_ID_e)un8Data;

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_USART_GetConfig(int32_t n32Argc, char *pn8Argv[], USART_CFG_t *ptCfg, USART_OPS_e *peOps)
{
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;
    uint8_t un8Arg = 2;

    if (pn8Argv[un8Arg][0] == 'p')
        *peOps = USART_OPS_POLL;
    else if (pn8Argv[un8Arg][0] == 'i')
        *peOps = USART_OPS_INTR;
    else if (pn8Argv[un8Arg][0] == 'd')
        *peOps = USART_OPS_INTR_DMA;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_USART_ERR_STR, NULL, EX_COMM_STR_OPT_OPS);
        goto err;
    }

    un8Arg++;
    if (pn8Argv[un8Arg][0] == 'u')
        ptCfg->eMode = USART_MODE_UART;
    else if (pn8Argv[un8Arg][0] == 'a')
        ptCfg->eMode = USART_MODE_USRT;
    else if (pn8Argv[un8Arg][0] == 's')
        ptCfg->eMode = USART_MODE_SPI;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_USART_ERR_STR, NULL, EX_COMM_STR_OPT_MODE);
        goto err;
    }

    if(ptCfg->eMode == USART_MODE_UART)
    {
        eDbgStatus = EX_USART_GetDataBit(++un8Arg, pn8Argv, &ptCfg->tCfg.tUart.eData);
        if (eDbgStatus != DEBUG_CMD_SUCCESS)
        {
            EX_COMMON_SetShowModuleLog(EX_USART_ERR_STR, "uart[data]", EX_COMM_STR_OPT_MAX);
            goto err;
        }

        eDbgStatus = EX_USART_GetParity(++un8Arg, pn8Argv, &ptCfg->tCfg.tUart.eParity);
        if (eDbgStatus != DEBUG_CMD_SUCCESS)
        {
            EX_COMMON_SetShowModuleLog(EX_USART_ERR_STR, "uart[parity]", EX_COMM_STR_OPT_MAX);
            goto err;
        }

        eDbgStatus = EX_USART_GetStopBit(++un8Arg, pn8Argv, &ptCfg->tCfg.tUart.eStop);
        if (eDbgStatus != DEBUG_CMD_SUCCESS)
        {
            EX_COMMON_SetShowModuleLog(EX_USART_ERR_STR, "uart[stop]", EX_COMM_STR_OPT_MAX);
            goto err;
        }

        switch (pn8Argv[++un8Arg][0])
        {
            case 'n':
                ptCfg->tCfg.tUart.bDoubleSpeed = false;
                break;
            case 'd':
                ptCfg->tCfg.tUart.bDoubleSpeed = true;
                break;
            default:
                EX_COMMON_SetShowModuleLog(EX_USART_ERR_STR, "uart[ds]", EX_COMM_STR_OPT_MAX);
                goto err;
        }

    }
    else if(ptCfg->eMode == USART_MODE_USRT)
    {
        eDbgStatus = EX_USART_GetMsMode(++un8Arg, pn8Argv, &ptCfg->tCfg.tUsrt.eMs);
        if (eDbgStatus != DEBUG_CMD_SUCCESS)
        {
            EX_COMMON_SetShowModuleLog(EX_USART_ERR_STR, "usrt[ms]", EX_COMM_STR_OPT_MAX);
            goto err;
        }

        eDbgStatus = EX_USART_GetDataBit(++un8Arg, pn8Argv, &ptCfg->tCfg.tUsrt.eData);
        if (eDbgStatus != DEBUG_CMD_SUCCESS)
        {
            EX_COMMON_SetShowModuleLog(EX_USART_ERR_STR, "usrt[data]", EX_COMM_STR_OPT_MAX);
            goto err;
        }

        eDbgStatus = EX_USART_GetParity(++un8Arg, pn8Argv, &ptCfg->tCfg.tUsrt.eParity);
        if (eDbgStatus != DEBUG_CMD_SUCCESS)
        {
            EX_COMMON_SetShowModuleLog(EX_USART_ERR_STR, "usrt[parity]", EX_COMM_STR_OPT_MAX);
            goto err;
        }

        eDbgStatus = EX_USART_GetStopBit(++un8Arg, pn8Argv, &ptCfg->tCfg.tUsrt.eStop);
        if (eDbgStatus != DEBUG_CMD_SUCCESS)
        {
            EX_COMMON_SetShowModuleLog(EX_USART_ERR_STR, "uart[stop]", EX_COMM_STR_OPT_MAX);
            goto err;
        }

        switch (pn8Argv[++un8Arg][0])
        {
            case '0':
                ptCfg->tCfg.tUsrt.eClkPol = USART_CLKPOL_TXD_RISE_RXD_FALL;
                break;
            case '1':
                ptCfg->tCfg.tUsrt.eClkPol = USART_CLKPOL_TXD_FALL_RXD_RISE;
                break;
            default:
                EX_COMMON_SetShowModuleLog(EX_USART_ERR_STR, "usrt[clkpol]", EX_COMM_STR_OPT_MAX);
                goto err;
        }

    }
    else if(ptCfg->eMode == USART_MODE_SPI)
    {
        eDbgStatus = EX_USART_GetMsMode(++un8Arg, pn8Argv, &ptCfg->tCfg.tUsrt.eMs);
        if (eDbgStatus != DEBUG_CMD_SUCCESS)
        {
            EX_COMMON_SetShowModuleLog(EX_USART_ERR_STR, "spi[ms]", EX_COMM_STR_OPT_MAX);
            goto err;
        }

        switch (pn8Argv[++un8Arg][0])
        {
            case 'm':
                ptCfg->tCfg.tSpi.eBitOrder = USART_BIT_ORDER_MSB;
                break;
            case 'l':
                ptCfg->tCfg.tSpi.eBitOrder = USART_BIT_ORDER_LSB;
                break;
            default:
                EX_COMMON_SetShowModuleLog(EX_USART_ERR_STR, "spi[order]", EX_COMM_STR_OPT_MAX);
                goto err;
        }

        switch (pn8Argv[++un8Arg][0])
        {
            case 'l':
                ptCfg->tCfg.tSpi.eClkPol = USART_CLKPOL_TXD_RISE_RXD_FALL;
                break;
            case 'h':
                ptCfg->tCfg.tSpi.eClkPol = USART_CLKPOL_TXD_RISE_RXD_FALL;
                break;
            default:
                EX_COMMON_SetShowModuleLog(EX_USART_ERR_STR, "spi", EX_COMM_STR_OPT_POL);
                goto err;
        }

        switch (pn8Argv[++un8Arg][0])
        {
            case 'o':
                ptCfg->tCfg.tSpi.eClkPha = USART_CLKPHA_SAMPLE;
                break;
            case 'e':
                ptCfg->tCfg.tSpi.eClkPha = USART_CLKPHA_SETUP;
                break;
            default:
                EX_COMMON_SetShowModuleLog(EX_USART_ERR_STR, "spi[pha]", EX_COMM_STR_OPT_MAX);
                goto err;
        }

        if(strncmp(pn8Argv[++un8Arg], "-swap", 5) == 0)
        {
            ptCfg->tCfg.tSpi.bSwapMSPort = true;
        }
        else
        {
            un8Arg--;
        }

    }

    ptCfg->un32BaudRate = (uint32_t)atoi(pn8Argv[++un8Arg]);

    if(strncmp(pn8Argv[++un8Arg], "-rto", 4) == 0)
    {
        eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[++un8Arg][0], &ptCfg->tRTO.bEnable);
        if (eDbgStatus != DEBUG_CMD_SUCCESS)
        {
            EX_COMMON_SetShowModuleLog(EX_USART_ERR_STR, "-rto[en]", EX_COMM_STR_OPT_MAX);
            goto err;
        }

        if (ptCfg->tRTO.bEnable == true)
        {
            ptCfg->tRTO.un32RTOCount = (uint32_t)atoi(pn8Argv[++un8Arg]);
        }

    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_USART_Init(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    USART_ID_e eId = USART_ID_0;

    eDbgStatus = EX_USART_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_USART_Init(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_USART_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_INIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_USART_Uninit(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    USART_ID_e eId = USART_ID_0;

    eDbgStatus = EX_USART_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_USART_Uninit(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_USART_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_UNINIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_USART_SetConfig(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus;
    USART_ID_e eId = USART_ID_0;
    USART_CFG_t tCfg;
    USART_OPS_e eOps = USART_OPS_POLL;
     
    eDbgStatus = EX_USART_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    memset(&tCfg, 0x00, sizeof(USART_CFG_t));

    eDbgStatus = EX_USART_GetConfig(n32Argc, pn8Argv, &tCfg, &eOps);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_USART_SetConfig(eId, &tCfg);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_USART_ERR_STR, "Cfg", EX_COMM_STR_OPT_MAX);
        }

        goto err;
    }

    s_tUSARTContext[eId].eId = eId;

    s_eOps = eOps;

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_USART_SetBuffer(int32_t n32Argc, char *pn8Argv[])
{
    uint8_t un8Arg = 2, un8BufLen = 0;
    uint32_t un32Data = 0, i = 0;

    un8BufLen = (uint8_t)atoi(pn8Argv[un8Arg++]);

    if(un8BufLen > EX_USART_DATA_LEN)
    {
        LOG(" %s len too many(max:%d)\n", EX_USART_ERR_STR, EX_USART_DATA_LEN);
        return DEBUG_CMD_INVALID;
    }
 
    for(i = 0; i < un8BufLen; i++)
    {
        sscanf(pn8Argv[un8Arg++], "%X", &un32Data);
        s_aun8EX_TxData[i] = un32Data;
    }

    return DEBUG_CMD_SUCCESS;
}
static enum debug_cmd_status EX_USART_Tx(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    USART_ID_e eId;
    uint8_t un8Arg = 2;
    uint32_t un32TxLen = 0;
    bool bEnForcePoll = false;

    eDbgStatus = EX_USART_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    un32TxLen = (uint32_t)atoi(pn8Argv[un8Arg++]);

    s_tUSARTContext[eId].eId = eId;

    if(un32TxLen > EX_USART_DATA_LEN || eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    if(strncmp(pn8Argv[un8Arg], "-poll", 5) == 0)
    {
        un8Arg++;
        bEnForcePoll = true;
    }

    LOG("%s (%d) tx %s\n", EX_USART_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_START]);

    eErr = HAL_USART_SetIRQ(eId, s_eOps, EX_USART_IRQHandler, &s_tUSARTContext[eId], EX_USART_TX_IRQ_PRIO);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_USART_ERR_STR, "IRQ", EX_COMM_STR_OPT_MAX);
        }

        goto err;
    }

    eErr = HAL_USART_Transmit(eId, s_aun8EX_TxData, un32TxLen, bEnForcePoll);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    LOG("%s (%d) tx complete\n", EX_USART_LOG_STR, eId);

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_USART_Rx(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    USART_ID_e eId;
    uint8_t un8Arg = 2;
    uint32_t un32RxLen = 0, i = 0;
    bool bEnForcePoll = false;

    eDbgStatus = EX_USART_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    un32RxLen = (uint32_t)atoi(pn8Argv[un8Arg++]);

    if(un32RxLen > EX_USART_DATA_LEN || eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    if(strncmp(pn8Argv[un8Arg], "-poll", 5) == 0)
    {
        un8Arg++;
        bEnForcePoll = true;
    }

    s_tUSARTContext[eId].eId = eId;
    
    memset(s_aun8EX_RxData, 0x00, sizeof(s_aun8EX_RxData));

    LOG("%s (%d) rx %s\n", EX_USART_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_START]);

    eErr = HAL_USART_SetIRQ(eId, s_eOps, EX_USART_IRQHandler, &s_tUSARTContext[eId], EX_USART_RX_IRQ_PRIO);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_USART_ERR_STR, "IRQ", EX_COMM_STR_OPT_MAX);
        }

        goto err;
    }

    eErr = HAL_USART_Receive(eId, s_aun8EX_RxData, un32RxLen, bEnForcePoll);
    if(eErr != HAL_ERR_OK)
    {
        LOG("%s (%d) rx error (%d)\n", EX_USART_LOG_STR, eId, eErr);
        goto err;
    }

    if(s_eOps == USART_OPS_POLL || bEnForcePoll == true)
    {
        for(i = 0; i < EX_USART_DATA_LEN; i++)
        {
            LOG("%s rx data=0x%x\n", EX_USART_LOG_STR, s_aun8EX_RxData[i]);
        }
        LOG("%s (%d) rx complete\n", EX_USART_LOG_STR, eId);
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_USART_Abort(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    USART_ID_e eId;
    uint8_t un8Arg = 2;

    eDbgStatus = EX_USART_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    switch (pn8Argv[un8Arg++][0])
    {
        case 't':
            (void)HAL_USART_Abort(eId);
            LOG("%s (%d) tx abort\n", EX_USART_ERR_STR, eId);
            break;
        case 'r':
            (void)HAL_USART_AbortRx(eId);
            LOG("%s (%d) rx abort\n", EX_USART_ERR_STR, eId);
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_USART_ERR_STR, "[com]", EX_COMM_STR_OPT_MAX);
            goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static const struct debug_cmd s_tEX_USART_CMD[] =
{
    {"USART", "h", EX_USART_Help, "help"},
    {"USART", "init", EX_USART_Init, ""},
    {"USART", "uninit", EX_USART_Uninit, ""},
    {"USART", "config", EX_USART_SetConfig, ""},
    {"USART", "data", EX_USART_SetBuffer, ""},
    {"USART", "tx", EX_USART_Tx, ""},
    {"USART", "rx", EX_USART_Rx, ""},
    {"USART", "abort", EX_USART_Abort, ""},
};

/**********************************************************************
 * @brief		Main program
 * @param[in]	None
 * @return	None
 **********************************************************************/
void EX_USART(void)
{
    /* Add TC commands */
    debug_cmd_init(s_tEX_USART_CMD,DEBUG_CMD_LIST_COUNT(s_tEX_USART_CMD));
}

#endif
/* --------------------------------- End Of File ------------------------------ */
