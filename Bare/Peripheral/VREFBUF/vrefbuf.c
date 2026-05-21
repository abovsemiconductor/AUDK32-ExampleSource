/**
 *******************************************************************************
 * @file        crc.c
 * @author      ABOV R&D Division
 * @brief       VREFBUF Example Code
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

#if defined(VREFBUF_TC)
#include "abov_config.h"
#include "ex_common.h"
#include "ex_configs.h"
#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "hal_scu.h"
#include "hal_scu_clk.h"
#include "hal_pcu.h"
#include "hal_vrefbuf.h"

#if !defined(_VREFBUF)
#error "This chipset did not support this example."
#endif

#define EX_VREFBUF_STR "VREFBUF"
#define EX_VREFBUF_LOG_STR "VREFBUF :"
#define EX_VREFBUF_ERR_STR "[E]VREFBUF :"
#define EX_VREFBUF_IRQ_PRIO      3
#define EX_VREFBUF_IRQ_TIMEOUT   10000000
#define EX_VREFBUF_DELAY         50
#define EX_VREFBUF_DATA_LEN      16
#define EX_VREFBUF_MAX_NUM  (CONFIG_VREFBUF_MAX_COUNT - 1)

extern const char *pcCommStr[];
extern const char *pcCmdStr[];
extern const char *pcValStr[];
extern const char *pcOptStr[];

static VREFBUF_OPS_e s_eOps = VREFBUF_OPS_POLL;

static enum debug_cmd_status EX_VREFBUF_Help(int32_t n32Argc, char *pn8Argv[])
{
    uint32_t un32ShowOpt = 0;
    EX_COMM_STR_OPT_e eOpt[2];

    EX_COMMON_SetShowModuleInfo(EX_VREFBUF_STR, CONFIG_VREFBUF_MAX_COUNT);

    un32ShowOpt = EX_COMMON_GetShowOpt(pn8Argv[1]);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_INIT, NULL, EX_VREFBUF_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_UNINIT, NULL, EX_VREFBUF_MAX_NUM, eOpt, 0, NULL);

    eOpt[0] = EX_COMM_STR_OPT_OPS; 
    eOpt[1] = EX_COMM_STR_OPT_MODE; 
#if defined (EX_VREFBUF_INPUT_CONF_MODE)
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CONFIG, NULL, EX_VREFBUF_MAX_NUM, eOpt, 2, "[in] [-out] [size] [comple]");
#else
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CONFIG, NULL, EX_VREFBUF_MAX_NUM, eOpt, 2, "[in] [-out]");
#endif
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MODE, EX_COMM_STR_VAL_MAX, "e(external)/v(vddext)/i(internal)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_OPS, EX_COMM_STR_VAL_MAX, "p(polling)/i(interrupt)/n(nmi)");
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "internal: [volt]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "volt: 0(2.0V)/1(2.5V)");
    }
    LOG("\n");

    return DEBUG_CMD_SUCCESS;
}

static void EX_VREFBUF_IRQHandler(uint32_t un32Event, void *pContext)
{
    VREFBUF_ID_e eId = VREFBUF_ID_0;

    LOG("%s (%d) (ISR)\n", EX_VREFBUF_LOG_STR, eId);
}

static enum debug_cmd_status EX_VREFBUF_GetId(int32_t n32Argc, char *pn8Argv[], VREFBUF_ID_e *peId)
{
    uint8_t un8Data = 0;
    if(n32Argc == 1)
    {
        EX_COMMON_SetShowModuleLog(EX_VREFBUF_ERR_STR, (char *)pcOptStr[EX_COMM_STR_OPT_ARG], EX_COMM_STR_OPT_MAX);
        return DEBUG_CMD_INVALID;
    }

    un8Data = atoi(pn8Argv[1]); 
    if(un8Data >= CONFIG_VREFBUF_MAX_COUNT)
    {
        LOG("%s Max Chan %d\n", EX_VREFBUF_ERR_STR, CONFIG_VREFBUF_MAX_COUNT);
        return DEBUG_CMD_INVALID;
    }

    *peId = (VREFBUF_ID_e)un8Data;

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_VREFBUF_GetConfig(int32_t n32Argc, char *pn8Argv[], VREFBUF_CFG_t *ptCfg, VREFBUF_OPS_e *peOps)
{
    uint8_t un8Arg = 2;

    if (pn8Argv[un8Arg][0] == 'p')
    {
        *peOps = VREFBUF_OPS_POLL;
    }
    else if (pn8Argv[un8Arg][0] == 'i')
    {
        *peOps = VREFBUF_OPS_INTR;
    }
    else if (pn8Argv[un8Arg][0] == 'n')
    {
        *peOps = VREFBUF_OPS_NMI;
    }
    else
    {
        EX_COMMON_SetShowModuleLog(EX_VREFBUF_ERR_STR, NULL, EX_COMM_STR_OPT_OPS);
        goto err;
    }

    switch (pn8Argv[++un8Arg][0])
    {
        case 'e':
            ptCfg->eMode = VREFBUF_MODE_EXTERNAL;
            break;
        case 'v':
            ptCfg->eMode = VREFBUF_MODE_VDDEXT;
            break;
        case 'i':
            ptCfg->eMode = VREFBUF_MODE_INTERNAL;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_VREFBUF_ERR_STR, NULL, EX_COMM_STR_OPT_MODE);
            goto err;
    }

    if(ptCfg->eMode == VREFBUF_MODE_VDDEXT)
    {
        ptCfg->un32StlTime = atoi(pn8Argv[++un8Arg]);
    }
    else if(ptCfg->eMode == VREFBUF_MODE_INTERNAL)
    {
        ptCfg->un32StlTime = atoi(pn8Argv[++un8Arg]);
        switch (pn8Argv[++un8Arg][0])
        {
            case '0':
                ptCfg->eVoltage = VREFBUF_VOLTAGE_INT20V;
                break;
            case '1':
                ptCfg->eVoltage = VREFBUF_VOLTAGE_INT25V;
                break;
            default:
                EX_COMMON_SetShowModuleLog(EX_VREFBUF_ERR_STR, "out[out]", EX_COMM_STR_OPT_MAX);
                goto err;
        }
    }
    else
    {
        ;
    }
        
    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_VREFBUF_Init(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    VREFBUF_ID_e eId = VREFBUF_ID_0;

    eDbgStatus = EX_VREFBUF_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_VREFBUF_Init(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_VREFBUF_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_INIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_VREFBUF_Uninit(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    VREFBUF_ID_e eId = VREFBUF_ID_0;

    eDbgStatus = EX_VREFBUF_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_VREFBUF_Uninit(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_VREFBUF_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_UNINIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_VREFBUF_SetConfig(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus;
    VREFBUF_ID_e eId = VREFBUF_ID_0;
    VREFBUF_CFG_t tCfg;

    eDbgStatus = EX_VREFBUF_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    memset(&tCfg, 0x00, sizeof(VREFBUF_CFG_t));

    eDbgStatus = EX_VREFBUF_GetConfig(n32Argc, pn8Argv, &tCfg, &s_eOps);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_VREFBUF_SetConfig(eId, &tCfg);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_VREFBUF_ERR_STR, "Cfg", EX_COMM_STR_OPT_MAX);
        }

        goto err;
    }

    if(s_eOps == VREFBUF_OPS_INTR || s_eOps == VREFBUF_OPS_NMI)
    {
        eErr = HAL_VREFBUF_SetIRQ(eId, s_eOps, EX_VREFBUF_IRQHandler, NULL, EX_VREFBUF_IRQ_PRIO);
        if(eErr != HAL_ERR_OK)
        {
            goto err;
        }
    }
    else
    {
        eErr = HAL_VREFBUF_SetWaitStable(eId, 10000);
        if(eErr != HAL_ERR_OK)
        {
            goto err;
        }
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static const struct debug_cmd s_tEX_VREFBUF_CMD[] =
{
    {"VREFBUF", "h",EX_VREFBUF_Help, "help"},
    {"VREFBUF", "init",EX_VREFBUF_Init, ""},
    {"VREFBUF", "uninit",EX_VREFBUF_Uninit, ""},
    {"VREFBUF", "config",EX_VREFBUF_SetConfig, ""},
};

/**********************************************************************
 * @brief		Main program
 * @param[in]	None
 * @return	None
 **********************************************************************/
void EX_VREFBUF(void)
{
    /* Add TC commands */
    debug_cmd_init(s_tEX_VREFBUF_CMD, DEBUG_CMD_LIST_COUNT(s_tEX_VREFBUF_CMD));
}

#endif /* VREFBUF_TC */

/* --------------------------------- End Of File ------------------------------ */
