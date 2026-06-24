/**
 *******************************************************************************
 * @file        ebi.c
 * @author      ABOV R&D Division
 * @brief       EBI Example Code
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

#if defined(EBI_TC)
#include "abov_config.h"
#include "ex_configs.h"
#include "ex_common.h"
#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "hal_scu.h"
#include "hal_scu_clk.h"
#include "hal_pcu.h"
#include "hal_ebi.h"

#if !defined(_EBI)
#error "This chipset did not support this example."
#endif

#warning The debug channel of this example should be use to USART1.

#if defined (EX_COMMON_ENABLE_CUSTOM_SSCANF)
#define sscanf(str, fmt, out) EX_COMMON_ParseByFormat((str), (fmt)[1], (uint32_t *)(out))
#endif

#define EX_EBI_STR "EBI"
#define EX_EBI_LOG_STR "EBI :"
#define EX_EBI_ERR_STR "[E]EBI :"
#define EX_EBI_USART1_LOG "WARN : The debug channel of EBI example should be use to USART1."
#define EX_EBI_MAX_NUM (CONFIG_EBI_MAX_COUNT - 1)

extern const char *pcCommStr[];
extern const char *pcCmdStr[];
extern const char *pcValStr[];
extern const char *pcOptStr[];

static enum debug_cmd_status EX_EBI_Help(int32_t n32Argc, char *pn8Argv[])
{
    uint32_t un32ShowOpt = 0;
    EX_COMM_STR_OPT_e eOpt[7];

    EX_COMMON_SetShowModuleInfo(EX_EBI_STR, CONFIG_EBI_MAX_COUNT);

    un32ShowOpt = EX_COMMON_GetShowOpt(pn8Argv[1]);

    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_INIT, NULL, EX_EBI_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_UNINIT, NULL, EX_EBI_MAX_NUM, eOpt, 0, NULL);

    eOpt[0] = EX_COMM_STR_OPT_MODE;
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CONFIG, NULL, EX_EBI_MAX_NUM, eOpt, 1, "[width] [lane] [clk] [wait] [-nwait] [-cycle]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MODE, EX_COMM_STR_VAL_MAX, "s(separated)/m(muxed)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "width: 8(8bit)/16(16bit)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "lane: e(en)/d(dis)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "clk: 1,2,3,4(n-clk)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "wait: 0~N");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-nwait: [pol]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_POL, EX_COMM_STR_VAL_LOWHIGH, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-cycle: [fall] [idle]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "fall: n(none),1,2,3");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "idle: n(none),1,2,3");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "write", EX_EBI_MAX_NUM, eOpt, 0, "[addr] [data] [width]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "addr: 0x00000000~0xNNNNNNNN");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "data: 0x00000000~0xNNNNNNNN");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "width: b(byte)/h(half-word)/w(word)");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "read", EX_EBI_MAX_NUM, eOpt, 0, "[addr] [width]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "addr: 0x00000000~0xNNNNNNNN");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "width: b(byte)/h(half-word)/w(word)");
    }
    LOG("\n");

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_EBI_GetId(int32_t n32Argc, char *pn8Argv[], EBI_ID_e *peId)
{
    uint8_t un8Data = 0;
    if(n32Argc == 1)
    {
        EX_COMMON_SetShowModuleLog(EX_EBI_ERR_STR, (char *)pcOptStr[EX_COMM_STR_OPT_ARG], EX_COMM_STR_OPT_MAX);
        return DEBUG_CMD_INVALID;
    }

    un8Data = atoi(pn8Argv[1]);
    if(un8Data >= CONFIG_EBI_MAX_COUNT)
    {
        LOG("%s Max Chan %d\n", EX_EBI_ERR_STR, CONFIG_EBI_MAX_COUNT);
        return DEBUG_CMD_INVALID;
    }

    *peId = (EBI_ID_e)un8Data;

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_EBI_GetConfig(int32_t n32Argc, char *pn8Argv[], EBI_CFG_t *ptCfg)
{
    uint8_t un8Arg = 2;

    switch (pn8Argv[un8Arg++][0])
    {
        case 's':
            ptCfg->eAddressType = EBI_ADDRESS_SEPARATED;
            break;
        case 'm':
            ptCfg->eAddressType = EBI_ADDRESS_MUXED;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_EBI_ERR_STR, NULL, EX_COMM_STR_OPT_MODE);
            goto err;
    }

    switch ((uint8_t)atoi(pn8Argv[un8Arg++]))
    {
        case 8:
             ptCfg->eMemBusWidth = EBI_BUS_WIDTH_8BIT;
             break;
        case 16:
             ptCfg->eMemBusWidth = EBI_BUS_WIDTH_16BIT;
             break;
        default:
            EX_COMMON_SetShowModuleLog(EX_EBI_ERR_STR, "[width]", EX_COMM_STR_OPT_MAX);
            goto err;
    }

    switch (pn8Argv[un8Arg++][0])
    {
        case 'e':
            ptCfg->eByteLaneSel = EBI_BLEN_ENABLE;
            break;
        case 'd':
            ptCfg->eByteLaneSel = EBI_BLEN_DISABLE;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_EBI_ERR_STR, "[lane]", EX_COMM_STR_OPT_MAX);
            goto err;
    }

    if (pn8Argv[un8Arg][0] == '1')
        ptCfg->eKeepCycle = EBI_CLK_CYCLE_1;
    else if (pn8Argv[un8Arg][0] == '2')
        ptCfg->eKeepCycle = EBI_CLK_CYCLE_2;
    else if (pn8Argv[un8Arg][0] == '3')
        ptCfg->eKeepCycle = EBI_CLK_CYCLE_3;
    else if (pn8Argv[un8Arg][0] == '4')
        ptCfg->eKeepCycle = EBI_CLK_CYCLE_4;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_EBI_ERR_STR, NULL, EX_COMM_STR_OPT_CLK);
        goto err;
    }

    un8Arg++;
    ptCfg->un8NormWait = (uint8_t)atoi(pn8Argv[un8Arg++]);

    if(strncmp(pn8Argv[un8Arg], "-nwait", 6) == 0)
    {
        un8Arg++;
        ptCfg->tExtReq.bEnable = true;
        switch (pn8Argv[un8Arg++][0])
        {
            case 'h':
                ptCfg->tExtReq.eWaitPolarity = EBI_POLARITY_ACTIVE_HIGH;
                break;
            case 'l':
                ptCfg->tExtReq.eWaitPolarity = EBI_POLARITY_ACTIVE_LOW;
                break;
            default:
                EX_COMMON_SetShowModuleLog(EX_EBI_ERR_STR, "-nwait", EX_COMM_STR_OPT_POL);
                goto err;
        }
    }
    else
    {
        ptCfg->tExtReq.bEnable = false;
    }

    if(strncmp(pn8Argv[un8Arg], "-cycle", 6) == 0)
    {
        un8Arg++;
        if (pn8Argv[un8Arg][0] == 'n')
            ptCfg->tExtCycle.eFallCycle = EBI_EXTEND_CYCLE_NONE;
        else if (pn8Argv[un8Arg][0] == '1')
            ptCfg->tExtCycle.eFallCycle = EBI_EXTEND_CYCLE_1;
        else if (pn8Argv[un8Arg][0] == '2')
            ptCfg->tExtCycle.eFallCycle = EBI_EXTEND_CYCLE_2;
        else if (pn8Argv[un8Arg][0] == '3')
            ptCfg->tExtCycle.eFallCycle = EBI_EXTEND_CYCLE_3;
        else
        {
            EX_COMMON_SetShowModuleLog(EX_EBI_ERR_STR, "-cycle[fall]", EX_COMM_STR_OPT_MAX);
            goto err;
        }

        un8Arg++;
        if (pn8Argv[un8Arg][0] == 'n')
            ptCfg->tExtCycle.eIdleCycle = EBI_IDLE_CYCLE_NONE;
        else if (pn8Argv[un8Arg][0] == '1')
            ptCfg->tExtCycle.eIdleCycle = EBI_IDLE_CYCLE_1;
        else if (pn8Argv[un8Arg][0] == '2')
            ptCfg->tExtCycle.eIdleCycle = EBI_IDLE_CYCLE_2;
        else if (pn8Argv[un8Arg][0] == '3')
            ptCfg->tExtCycle.eIdleCycle = EBI_IDLE_CYCLE_3;
        else
        {
            EX_COMMON_SetShowModuleLog(EX_EBI_ERR_STR, "-cycle[idle]", EX_COMM_STR_OPT_MAX);
            goto err;
        }
    }
    else
    {
        ptCfg->tExtCycle.eFallCycle = EBI_EXTEND_CYCLE_NONE;
        ptCfg->tExtCycle.eIdleCycle = EBI_IDLE_CYCLE_NONE;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_EBI_Init(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    EBI_ID_e eId = EBI_ID_0;

    eDbgStatus=EX_EBI_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_EBI_Init(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_EBI_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_INIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_EBI_Uninit(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    EBI_ID_e eId = EBI_ID_0;

    eDbgStatus=EX_EBI_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_EBI_Uninit(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_EBI_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_UNINIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_EBI_SetConfig(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus;
    EBI_ID_e eId = EBI_ID_0;
    EBI_CFG_t tCfg;

    eDbgStatus = EX_EBI_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    memset(&tCfg, 0x00, sizeof(EBI_CFG_t));

    eDbgStatus = EX_EBI_GetConfig(n32Argc, pn8Argv, &tCfg);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_EBI_SetConfig(eId, &tCfg);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_EBI_ERR_STR, "Cfg", EX_COMM_STR_OPT_MAX);
        }
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_EBI_Write(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    EBI_ID_e eId;
    uint32_t un32Addr = 0, un32Data = 0;
    uint8_t un8Arg = 2;

    eDbgStatus = EX_EBI_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    if(n32Argc != 5)
    {
        goto err;
    }

    sscanf(pn8Argv[un8Arg++], "%x", &un32Addr);
    sscanf(pn8Argv[un8Arg++], "%x", &un32Data);

    switch (pn8Argv[un8Arg++][0])
    {
        case 'b':
            *(volatile uint8_t *)un32Addr = un32Data;
            break;
        case 'h':
            *(volatile uint16_t *)un32Addr = un32Data;
            break;
        case 'w':
            *(volatile uint32_t *)un32Addr = un32Data;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_EBI_ERR_STR, "[width]", EX_COMM_STR_OPT_MAX);
            goto err;
    }

    if(eDbgStatus == DEBUG_CMD_SUCCESS)
    {
        LOG("%s (%d) Write [A=0x%08x] [D=0x%08x]\n", EX_EBI_LOG_STR, eId, un32Addr, un32Data);
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_EBI_Read(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    EBI_ID_e eId;
    uint32_t un32Addr = 0, un32Data = 0;
    uint8_t un8Arg = 2;

    eDbgStatus = EX_EBI_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    if(n32Argc != 4)
    {
        goto err;
    }

    sscanf(pn8Argv[un8Arg++], "%X", &un32Addr);

    switch (pn8Argv[un8Arg++][0])
    {
        case 'b':
            un32Data = *(volatile uint8_t *)un32Addr;
            break;
        case 'h':
            un32Data = *(volatile uint16_t *)un32Addr;
            break;
        case 'w':
            un32Data = *(volatile uint32_t *)un32Addr;
            break;
        default:
            eDbgStatus = DEBUG_CMD_INVALID;
            EX_COMMON_SetShowModuleLog(EX_EBI_ERR_STR, "[width]", EX_COMM_STR_OPT_MAX);
            break;
    }

    if(eDbgStatus == DEBUG_CMD_SUCCESS)
    {
        LOG("%s (%d) Read [A=0x%08x] [D=0x%08x]\n", EX_EBI_LOG_STR, eId, un32Addr, un32Data);
        /* meaningless, it is only for removing compile warning */
        (void)un32Data;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static const struct debug_cmd s_tEX_EBI_CMD[] =
{
    {"EBI", "h", EX_EBI_Help, "help"},
    {"EBI", "init", EX_EBI_Init, ""},
    {"EBI", "uninit", EX_EBI_Uninit, ""},
    {"EBI", "config", EX_EBI_SetConfig, ""},
    {"EBI", "read", EX_EBI_Read, ""},
    {"EBI", "write", EX_EBI_Write, ""},
};

void EX_EBI(void)
{
    LOG("%s\n", EX_EBI_USART1_LOG);

    SystemDelayMS(3);

    /* Add TC commands */
    debug_cmd_init(s_tEX_EBI_CMD, DEBUG_CMD_LIST_COUNT(s_tEX_EBI_CMD));

}

#endif /* EBI */
/* --------------------------------- End Of File ------------------------------ */
