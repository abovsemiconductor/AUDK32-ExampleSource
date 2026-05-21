/**
 *******************************************************************************
 * @file        uart.c
 * @author      ABOV R&D Division
 * @brief       UART Example Code
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

#if defined(UART_TC)
#include "abov_config.h"
#include "ex_common.h"
#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "hal_scu.h"
#include "hal_scu_clk.h"
#include "hal_pcu.h"
#include "hal_uart.h"

#if !defined(_UART)
#error "This chipset did not support this example."
#endif

#define EX_UART_STR "UART"
#define EX_UART_LOG_STR "UART :"
#define EX_UART_ERR_STR "[E]UART :"
#define EX_UART_TX_IRQ_PRIO 3
#define EX_UART_RX_IRQ_PRIO 0
#define EX_UART_DATA_LEN   16
#define EX_UART_MAX_NUM  (CONFIG_UART_MAX_COUNT - 1)

extern const char *pcCommStr[];
extern const char *pcCmdStr[];
extern const char *pcValStr[];
extern const char *pcOptStr[];

static UART_Context_t s_tUARTContext[CONFIG_UART_MAX_COUNT];
static uint8_t s_aun8EX_TxData[EX_UART_DATA_LEN];
static uint8_t s_aun8EX_RxData[EX_UART_DATA_LEN];
static UART_OPS_e s_eOps = UART_OPS_POLL;

static enum debug_cmd_status EX_UART_Help(int32_t n32Argc, char *pn8Argv[])
{
    uint32_t un32ShowOpt = 0;
    EX_COMM_STR_OPT_e eOpt[2];

    EX_COMMON_SetShowModuleInfo(EX_UART_STR, CONFIG_UART_MAX_COUNT);

    un32ShowOpt = EX_COMMON_GetShowOpt(pn8Argv[1]);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_INIT, NULL, EX_UART_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_UNINIT, NULL, EX_UART_MAX_NUM, eOpt, 0, NULL);

    eOpt[0] = EX_COMM_STR_OPT_SRC; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CLK, NULL, EX_UART_MAX_NUM, eOpt, 1, NULL);
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_SRC, EX_COMM_STR_VAL_CLKPATH, NULL);
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MCCR, EX_COMM_STR_VAL_MAX, "[mccr] [div]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MCCR, EX_COMM_STR_VAL_CLKSRC, "/m(mclk)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_DIV, EX_COMM_STR_VAL_N_255, NULL);
    }

    eOpt[0] = EX_COMM_STR_OPT_OPS; 
    eOpt[1] = EX_COMM_STR_OPT_DA; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CONFIG, NULL, EX_UART_MAX_NUM, eOpt, 2, "[parity] [stop] [baud]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_OPS, EX_COMM_STR_VAL_OPS, "/d(dma)/n(nmi)/m(nmi dma)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_DA, EX_COMM_STR_VAL_MAX, "5/6/7/8");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "o(odd)/e(even)/n(none)/0(force 0)/1(force 1)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "stop: 1/2");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "baud: 0~N");
    }
    eOpt[0] = EX_COMM_STR_OPT_CNT;
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "data", EX_UART_MAX_NUM, eOpt, 1, "[da ...]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_CNT, EX_COMM_STR_VAL_N_NUM, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_DA, EX_COMM_STR_VAL_MAX, "max 16 bytes (hexa and space (delimitor))");
    }

    eOpt[0] = EX_COMM_STR_OPT_CNT;
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "tx", EX_UART_MAX_NUM, eOpt, 1, "[-poll]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_CNT, EX_COMM_STR_VAL_MAX, "1~16 (tx data cnt)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-poll: en-force polling operation");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "rx", EX_UART_MAX_NUM, eOpt, 1, "[-poll]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_CNT, EX_COMM_STR_VAL_MAX, "1~16 (rx data cnt)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-poll: en-force polling operation");
    }

    return DEBUG_CMD_SUCCESS;
}

static void EX_UART_IRQHandler(uint32_t un32Event, void *pContext)
{
    uint32_t i = 0;

    if(un32Event & UART_EVENT_TX_DONE)
    {
#if defined(EXTRN_SUBFAMILY_A33G52x)
#else
        LOG("%s (%d) tx done\n", EX_UART_LOG_STR, ((UART_Context_t *)pContext)->eId);
#endif
    }

    if(un32Event & UART_EVENT_RX_DONE)
    {
        LOG("%s (%d) rx done\n", EX_UART_LOG_STR, ((UART_Context_t *)pContext)->eId);
        for(i = 0; i < EX_UART_DATA_LEN; i++)
        {
            LOG("%s rx Data=0x%x\n", EX_UART_LOG_STR, s_aun8EX_RxData[i]);
        }
    }
}

static enum debug_cmd_status EX_UART_GetId(int32_t n32Argc, char *pn8Argv[], UART_ID_e *peId)
{
    uint8_t un8Data = 0;
    if(n32Argc == 1)
    {
        EX_COMMON_SetShowModuleLog(EX_UART_ERR_STR, (char *)pcOptStr[EX_COMM_STR_OPT_ARG], EX_COMM_STR_OPT_MAX);
        return DEBUG_CMD_INVALID;
    }

    un8Data = atoi(pn8Argv[1]); 
    if(un8Data >= CONFIG_UART_MAX_COUNT)
    {
        LOG("%s Max Chan %d\n", EX_UART_ERR_STR, CONFIG_UART_MAX_COUNT);
        return DEBUG_CMD_INVALID;
    }

    *peId = (UART_ID_e)un8Data;

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_UART_GetClkConfig(int32_t n32Argc, char *pn8Argv[], UART_CLK_CFG_t *ptClkCfg)
{
    uint8_t un8Arg = 2;

    switch (pn8Argv[un8Arg++][0])
    {
        case 'p':
            ptClkCfg->eClk = UART_CLK_PCLK;
            break;
        case 'm':
            ptClkCfg->eClk = UART_CLK_MCCR;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_UART_ERR_STR, NULL, EX_COMM_STR_OPT_MCCR);
            goto err;
    }

    if(ptClkCfg->eClk == UART_CLK_MCCR)
    {
        if (pn8Argv[un8Arg][0] == 'l')
            ptClkCfg->eMccr = UART_CLK_MCCR_LSI;
        else if (pn8Argv[un8Arg][0] == 's')
            ptClkCfg->eMccr = UART_CLK_MCCR_LSE;
        else if (pn8Argv[un8Arg][0] == 'm')
            ptClkCfg->eMccr = UART_CLK_MCCR_MCLK;
        else if (pn8Argv[un8Arg][0] == 'h')
            ptClkCfg->eMccr = UART_CLK_MCCR_HSI;
        else if (pn8Argv[un8Arg][0] == 'e')
            ptClkCfg->eMccr = UART_CLK_MCCR_HSE;
        else if (pn8Argv[un8Arg][0] == 'p')
            ptClkCfg->eMccr = UART_CLK_MCCR_PLL;
        else
        {
            EX_COMMON_SetShowModuleLog(EX_UART_ERR_STR, NULL, EX_COMM_STR_OPT_MCCR);
            goto err;
        }

        un8Arg++;
        ptClkCfg->un8MccrDiv = atoi(pn8Argv[un8Arg++]);
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_UART_GetConfig(int32_t n32Argc, char *pn8Argv[], UART_CFG_t *ptCfg, UART_OPS_e *peOps)
{
    uint8_t un8Arg = 2;

    if (pn8Argv[un8Arg][0] == 'p')
        *peOps = UART_OPS_POLL;
    else if (pn8Argv[un8Arg][0] == 'i')
        *peOps = UART_OPS_INTR;
    else if (pn8Argv[un8Arg][0] == 'd')
        *peOps = UART_OPS_INTR_DMA;
    else if (pn8Argv[un8Arg][0] == 'n')
        *peOps = UART_OPS_NMI;
    else if (pn8Argv[un8Arg][0] == 'm')
        *peOps = UART_OPS_NMI_DMA;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_UART_ERR_STR, NULL, EX_COMM_STR_OPT_OPS);
        goto err;
    }

    un8Arg++;
    if (pn8Argv[un8Arg][0] == '5')
        ptCfg->eData = UART_DATA_5;
    else if (pn8Argv[un8Arg][0] == '6')
        ptCfg->eData = UART_DATA_6;
    else if (pn8Argv[un8Arg][0] == '7')
        ptCfg->eData = UART_DATA_7;
    else if (pn8Argv[un8Arg][0] == '8')
        ptCfg->eData = UART_DATA_8;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_UART_ERR_STR, "[data]", EX_COMM_STR_OPT_MAX);
        goto err;
    }

    un8Arg++;
    if (pn8Argv[un8Arg][0] == 'n')
        ptCfg->eParity = UART_PARITY_NONE;
    else if (pn8Argv[un8Arg][0] == 'o')
        ptCfg->eParity = UART_PARITY_ODD;
    else if (pn8Argv[un8Arg][0] == 'e')
        ptCfg->eParity = UART_PARITY_EVEN;
    else if (pn8Argv[un8Arg][0] == '0')
        ptCfg->eParity = UART_PARITY_SP_0;
    else if (pn8Argv[un8Arg][0] == '1')
        ptCfg->eParity = UART_PARITY_SP_1;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_UART_ERR_STR, "[parity]", EX_COMM_STR_OPT_MAX);
        goto err;
    }

    switch (pn8Argv[++un8Arg][0])
    {
        case '1':
            ptCfg->eStop = UART_STOP_1;
            break;
        case '2':
            ptCfg->eStop = UART_STOP_2;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_UART_ERR_STR, "[stop]", EX_COMM_STR_OPT_MAX);
            goto err;
    }

    ptCfg->un32BaudRate = (uint32_t)atoi(pn8Argv[++un8Arg]);

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_UART_Init(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    UART_ID_e eId = UART_ID_0;

    eDbgStatus = EX_UART_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_UART_Init(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_UART_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_INIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_UART_Uninit(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    UART_ID_e eId = UART_ID_0;

    eDbgStatus = EX_UART_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_UART_Uninit(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_UART_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_UNINIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_UART_SetClkConfig(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    UART_ID_e eId;
    UART_CLK_CFG_t tClkCfg; 

    eDbgStatus = EX_UART_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    memset(&tClkCfg, 0x00, sizeof(UART_CLK_CFG_t));

    eDbgStatus = EX_UART_GetClkConfig(n32Argc, pn8Argv, &tClkCfg);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_UART_SetClkConfig(eId, &tClkCfg);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_UART_ERR_STR, "ClkCfg", EX_COMM_STR_OPT_MAX);
        }

        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_UART_SetConfig(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus;
    UART_ID_e eId = UART_ID_0;
    UART_OPS_e eOps = UART_OPS_MAX;
    UART_CFG_t tCfg;
     
    eDbgStatus = EX_UART_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    memset(&tCfg, 0x00, sizeof(UART_CFG_t));

    eDbgStatus = EX_UART_GetConfig(n32Argc, pn8Argv, &tCfg, &eOps);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_UART_SetConfig(eId, &tCfg);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_UART_ERR_STR, "Cfg", EX_COMM_STR_OPT_MAX);
        }
        goto err;
    }

    s_tUARTContext[eId].eId = eId;

    s_eOps = eOps;

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;

}

static enum debug_cmd_status EX_UART_SetBuffer(int32_t n32Argc, char *pn8Argv[])
{
    uint8_t un8Arg = 2, un8BufLen = 0;
    uint32_t un32Data = 0, i = 0;

    un8BufLen = (uint8_t)atoi(pn8Argv[un8Arg++]);

    if(un8BufLen > EX_UART_DATA_LEN)
    {
        LOG(" %s len too many(max:%d)\n", EX_UART_ERR_STR, EX_UART_DATA_LEN);
        return DEBUG_CMD_INVALID;
    }
 
    for(i = 0; i < un8BufLen; i++)
    {
        sscanf(pn8Argv[un8Arg++], "%X", &un32Data);
        s_aun8EX_TxData[i] = un32Data;
    }

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_UART_Tx(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    UART_ID_e eId;
    uint8_t un32TxLen = 0, un8Arg = 2;
    bool bEnForcePoll = false;

    eDbgStatus = EX_UART_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    un32TxLen = (uint32_t)atoi(pn8Argv[un8Arg++]);

    if(un32TxLen > EX_UART_DATA_LEN || eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    if(strncmp(pn8Argv[un8Arg], "-poll", 5) == 0)
    {
        un8Arg++;
        bEnForcePoll = true;
    }

    s_tUARTContext[eId].eId = eId;

    LOG("%s (%d) tx %s\n", EX_UART_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_START]);

    eErr = HAL_UART_SetIRQ(eId, s_eOps, EX_UART_IRQHandler, &s_tUARTContext[eId], EX_UART_TX_IRQ_PRIO);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_UART_ERR_STR, "IRQ", EX_COMM_STR_OPT_MAX);
        }

        goto err;
    }

    eErr = HAL_UART_Transmit(eId, s_aun8EX_TxData, un32TxLen, bEnForcePoll);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    LOG("%s (%d) tx complete\n", EX_UART_LOG_STR, eId);

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_UART_Rx(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus;
    UART_ID_e eId;
    uint8_t un32RxLen = 0, un8Arg = 2;
    uint32_t i=0;
    bool bEnForcePoll = false;

    eDbgStatus = EX_UART_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    un32RxLen = (uint32_t)atoi(pn8Argv[un8Arg++]);

    if(un32RxLen > EX_UART_DATA_LEN || eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    if(strncmp(pn8Argv[un8Arg], "-poll", 5) == 0)
    {
        un8Arg++;
        bEnForcePoll = true;
    }

    s_tUARTContext[eId].eId = eId;

    LOG("%s (%d) rx %s\n", EX_UART_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_START]);

    eErr = HAL_UART_SetIRQ(eId, s_eOps, EX_UART_IRQHandler, &s_tUARTContext[eId], EX_UART_RX_IRQ_PRIO);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_UART_ERR_STR, "IRQ", EX_COMM_STR_OPT_MAX);
        }

        goto err;
    }

    eErr = HAL_UART_Receive(eId, s_aun8EX_RxData, un32RxLen, bEnForcePoll);
    if(eErr != HAL_ERR_OK)
    {
        LOG("%s (%d) rx error (%d)\n", EX_UART_LOG_STR, eId, eErr);
        goto err;
    }

    if(s_eOps == UART_OPS_POLL || bEnForcePoll == true)
    {
        for(i = 0; i < un32RxLen; i++)
        {
            LOG("%s rx data=0x%x\n", EX_UART_LOG_STR, s_aun8EX_RxData[i]);
        }
        LOG("%s (%d) rx complete\n", EX_UART_LOG_STR, eId);
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static const struct debug_cmd s_tEX_UART_CMD[] =
{
    {"UART", "h", EX_UART_Help, "help"},
    {"UART", "init", EX_UART_Init, ""},
    {"UART", "uninit", EX_UART_Uninit, ""},
    {"UART", "clk", EX_UART_SetClkConfig, ""},
    {"UART", "config", EX_UART_SetConfig, ""},
    {"UART", "data", EX_UART_SetBuffer, ""},
    {"UART", "tx", EX_UART_Tx, ""},
    {"UART", "rx", EX_UART_Rx, ""}
};

void EX_UART(void)
{
    /* Add TC commands */
    debug_cmd_init(s_tEX_UART_CMD,DEBUG_CMD_LIST_COUNT(s_tEX_UART_CMD));
}

#endif
/* --------------------------------- End Of File ------------------------------ */
