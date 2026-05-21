/**
 *******************************************************************************
 * @file        rtc.c
 * @author      ABOV R&D Division
 * @brief       RTC Example Code
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

#if defined(RTC_TC)
#include "abov_config.h"
#include "ex_configs.h"
#include "ex_common.h"
#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "hal_scu.h"
#include "hal_scu_clk.h"
#include "hal_pcu.h"
#include "hal_rtc.h"

#if !defined(_RTC)
#error "This chipset did not support this example."
#endif

#define EX_RTC_STR "RTC"
#define EX_RTC_LOG_STR "RTC :"
#define EX_RTC_ERR_STR "[E]RTC :"
#define EX_RTC_IRQ_PRIO     3
#define EX_RTC_SAMPLE_COUNT 360
#define EX_RTC_MAX_NUM  (CONFIG_RTC_MAX_COUNT - 1)

#define TO_HEX(x) (x + (x / 10) * 6)
#define FROM_HEX(x) (x - (x / 16) * 6)

extern const char *pcCommStr[];
extern const char *pcCmdStr[];
extern const char *pcValStr[];
extern const char *pcOptStr[];

extern uint32_t SystemCoreClock;
static RTC_Context_t s_tRTCContext[CONFIG_RTC_MAX_COUNT];
static bool s_b24Hour = false;

static int8_t *s_an8TCWeek[] =
{
    "Sun",
    "Mon",
    "Tue",
    "Wed",
    "Thu",
    "Fri",
    "Sat"
};

static enum debug_cmd_status EX_RTC_Help(int32_t n32Argc, char *pn8Argv[])
{
    uint32_t un32ShowOpt = 0;

    EX_COMM_STR_OPT_e eOpt[2];

    EX_COMMON_SetShowModuleInfo(EX_RTC_STR, CONFIG_RTC_MAX_COUNT);

    un32ShowOpt = EX_COMMON_GetShowOpt(pn8Argv[1]);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_INIT, NULL, EX_RTC_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_UNINIT, NULL, EX_RTC_MAX_NUM, eOpt, 0, NULL);

    eOpt[0] = EX_COMM_STR_OPT_SRC; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CLK, NULL, EX_RTC_MAX_NUM, eOpt, 1, NULL);
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_SRC, EX_COMM_STR_VAL_MAX, "l(lsi40khz)/wdtrc/m(mccr)");
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MCCR, EX_COMM_STR_VAL_MAX, "[mccr] [div]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MCCR, EX_COMM_STR_VAL_CLKSRC, "/m(mclk)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_DIV, EX_COMM_STR_VAL_N_255, NULL);
    }
    eOpt[0] = EX_COMM_STR_OPT_OPS; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CONFIG, NULL, EX_RTC_MAX_NUM, eOpt, 1, "[-err] [-meas]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_OPS, EX_COMM_STR_VAL_OPS, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-err: [dir] [time] [da]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "dir: i(inc)/d(dec)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "time: 2(20-sec)/6(60-sec)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_DA, EX_COMM_STR_VAL_N_NUM, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-meas: measuring real freq at PC13 port");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "clock", EX_RTC_MAX_NUM, eOpt, 0, "[yy] [mm] [ww] [dd] [hh] [mm] [ss] [period] [-24]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "yy: 0~N(year)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "mm: 0~N(month)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "ww: 0~N(week)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "dd: 0~N(day)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "hh: 0~N(hour)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "mm: 0~N(min)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "ss: 0~N(sec)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "period: n(none)/f(half)/s(sec)/m(min)/h(hour)/d(day)/o(month)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-24h: enable 24h");
    }
    eOpt[0] = EX_COMM_STR_OPT_ENA; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "timestamp", EX_RTC_MAX_NUM, eOpt, 1, NULL);
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_ENA, EX_COMM_STR_VAL_ENDIS, NULL);
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_ENA, EX_COMM_STR_VAL_MAX, "[edge]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_EDGE, EX_COMM_STR_VAL_EDGE, "/b(both)");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "alarm", EX_RTC_MAX_NUM, eOpt, 1, NULL);
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_ENA, EX_COMM_STR_VAL_ENDIS, NULL);
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_ENA, EX_COMM_STR_VAL_MAX, "[ww] [hh] [mm]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "ww: a(all)/s(sun)/m(mon)/t(tue)/w(wed)/h(thu)/f(fri)/r(sat)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "(ex. a (all day), s:w:f (sun:wed:fri)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "hh: 0~N(hour)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "mm: 0~N(min)");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_START, NULL, EX_RTC_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_STOP, NULL, EX_RTC_MAX_NUM, eOpt, 0, NULL);

    eOpt[0] = EX_COMM_STR_OPT_ENA; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "wkup", EX_RTC_MAX_NUM, eOpt, 1, "[ext]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_ENA, EX_COMM_STR_VAL_ENDIS, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "ext: e(en)/d(dis) - Ext Pin wake-up evt");
    }
    LOG("\n");

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_RTC_GetId(int32_t n32Argc, char *pn8Argv[], RTC_ID_e *peId)
{
    uint8_t un8Data = 0;
    if(n32Argc == 1)
    {
        EX_COMMON_SetShowModuleLog(EX_RTC_ERR_STR, (char *)pcOptStr[EX_COMM_STR_OPT_ARG], EX_COMM_STR_OPT_MAX);
        return DEBUG_CMD_INVALID;
    }

    un8Data = atoi(pn8Argv[1]); 
    if(un8Data >= CONFIG_RTC_MAX_COUNT)
    {
        LOG("%s Max Chan %d\n", EX_RTC_ERR_STR, CONFIG_RTC_MAX_COUNT);
        return DEBUG_CMD_INVALID;
    }

    *peId = (RTC_ID_e)un8Data;

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_RTC_GetClkConfig(int32_t n32Argc, char *pn8Argv[], RTC_CLK_CFG_t *ptClkCfg)
{
    uint8_t un8Arg = 2;

    switch (pn8Argv[un8Arg++][0])
    {
        case 'l':
            ptClkCfg->eClk = RTC_CLK_LSI40KHZ;
            break;
        case 'm':
            ptClkCfg->eClk = RTC_CLK_MCCR;
            break;
        case 'w':
            ptClkCfg->eClk = RTC_CLK_WDTRC;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_RTC_ERR_STR, NULL, EX_COMM_STR_OPT_SRC);
            goto err;
    }

    if(ptClkCfg->eClk == RTC_CLK_MCCR)
    {
        if (pn8Argv[un8Arg][0] == 'l')
            ptClkCfg->eMccr = RTC_CLK_MCCR_LSI;
        else if (pn8Argv[un8Arg][0] == 's')
            ptClkCfg->eMccr = RTC_CLK_MCCR_LSE;
        else if (pn8Argv[un8Arg][0] == 'm')
            ptClkCfg->eMccr = RTC_CLK_MCCR_MCLK;
        else if (pn8Argv[un8Arg][0] == 'h')
            ptClkCfg->eMccr = RTC_CLK_MCCR_HSI;
        else if (pn8Argv[un8Arg][0] == 'e')
            ptClkCfg->eMccr = RTC_CLK_MCCR_HSE;
        else if (pn8Argv[un8Arg][0] == 'p')
            ptClkCfg->eMccr = RTC_CLK_MCCR_PLL;
        else
        {
            EX_COMMON_SetShowModuleLog(EX_RTC_ERR_STR, NULL, EX_COMM_STR_OPT_MCCR);
            goto err;
        }
        un8Arg++;
        ptClkCfg->un8MccrDiv = (uint8_t)atoi(pn8Argv[un8Arg++]);
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_RTC_GetConfig(int32_t n32Argc, char *pn8Argv[], RTC_CFG_t *ptCfg, RTC_OPS_e *peOps)
{
    uint8_t un8Arg = 2;

    switch (pn8Argv[un8Arg++][0])
    {
        case 'p':
            *peOps = RTC_OPS_POLL;
            break;
        case 'i':
            *peOps = RTC_OPS_INTR;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_RTC_ERR_STR, NULL, EX_COMM_STR_OPT_OPS);
            goto err;
    }
 
    if(strncmp(pn8Argv[un8Arg], "-err", 4) == 0)
    {
        un8Arg++;
        ptCfg->bErrCorEn = true;
        
        switch (pn8Argv[un8Arg++][0])
        {
            case 'i':
                ptCfg->eErrCorDir = RTC_ERRCOR_DIR_INC;
                break;
            case 'd':
                ptCfg->eErrCorDir = RTC_ERRCOR_DIR_DEC;
                break;
            default:
                EX_COMMON_SetShowModuleLog(EX_RTC_ERR_STR, "[dir]", EX_COMM_STR_OPT_OPS);
                goto err;
        }

        switch (pn8Argv[un8Arg++][0])
        {
            case '2':
                ptCfg->eErrCorTime = RTC_ERRCOR_TIME_20SEC;
                break;
            case '6':
                ptCfg->eErrCorTime = RTC_ERRCOR_TIME_60SEC;
                break;
            default:
                EX_COMMON_SetShowModuleLog(EX_RTC_ERR_STR, "err[time]", EX_COMM_STR_OPT_OPS);
                goto err;
        }
        
        ptCfg->un32ErrCorData = (uint32_t)atoi(pn8Argv[un8Arg++]);
    }

    if(strncmp(pn8Argv[un8Arg], "-meas", 5) == 0)
    {
        un8Arg++;
        ptCfg->bRTCClkOut = true;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_RTC_GetClockConfig(int32_t n32Argc, char *pn8Argv[], RTC_CLOCK_t *ptCfg, bool *pbPm)
{
    uint8_t un8Arg = 2, un8Data = 0, un8Hour = 0;

    un8Data = atoi(pn8Argv[un8Arg++]);
    if(un8Data < 100)
    {
        ptCfg->tDate.un8Year = TO_HEX(un8Data);
    }
    else
    {
        LOG("%s Year invalid\n", EX_RTC_ERR_STR);
        goto err;
    }

    un8Data = atoi(pn8Argv[un8Arg++]);
    if(un8Data != 0 || un8Data < 13)
    {
        ptCfg->tDate.un8Month = TO_HEX(un8Data);
    }
    else
    {
        LOG("%s Month invalid\n", EX_RTC_ERR_STR);
        goto err;
    }

    un8Data = atoi(pn8Argv[un8Arg++]);
    if(un8Data != 0 || un8Data < 8)
    {
        ptCfg->tDate.un8Week = TO_HEX(un8Data);
    }
    else
    {
        LOG("%s Week invalid\n", EX_RTC_ERR_STR);
        goto err;
    }

    un8Data = atoi(pn8Argv[un8Arg++]);
    if(un8Data != 0 || un8Data < 32)
    {
        ptCfg->tDate.un8Day = TO_HEX(un8Data);
    }
    else
    {
        LOG("%s Day invalid\n", EX_RTC_ERR_STR);
        goto err;
    }

    un8Hour = (uint8_t)atoi(pn8Argv[un8Arg++]);
    if(un8Data > 23)
    {
        LOG("%s Hour invalid\n", EX_RTC_ERR_STR);
        goto err;
    }

    un8Data = atoi(pn8Argv[un8Arg++]);
    if(un8Data < 60)
    {
        ptCfg->tTime.un8Min = TO_HEX(un8Data);
    }
    else
    {
        LOG("%s Minute invalid\n", EX_RTC_ERR_STR);
        goto err;
    }

    un8Data = atoi(pn8Argv[un8Arg++]);
    if(un8Data < 60)
    {
        ptCfg->tTime.un8Sec = TO_HEX(un8Data);
    }
    else
    {
        LOG("%s Second invalid\n", EX_RTC_ERR_STR);
        goto err;
    }

    if (pn8Argv[un8Arg][0] == 'n')
        ptCfg->ePeriod = RTC_CONST_PERIOD_NONE;
    else if (pn8Argv[un8Arg][0] == 'f')
        ptCfg->ePeriod = RTC_CONST_PERIOD_HALFSEC;
    else if (pn8Argv[un8Arg][0] == 's')
        ptCfg->ePeriod = RTC_CONST_PERIOD_ONESEC;
    else if (pn8Argv[un8Arg][0] == 'm')
        ptCfg->ePeriod = RTC_CONST_PERIOD_ONEMIN;
    else if (pn8Argv[un8Arg][0] == 'h')
        ptCfg->ePeriod = RTC_CONST_PERIOD_ONEHOUR;
    else if (pn8Argv[un8Arg][0] == 'd')
        ptCfg->ePeriod = RTC_CONST_PERIOD_ONEDAY;
    else if (pn8Argv[un8Arg][0] == 'o')
        ptCfg->ePeriod = RTC_CONST_PERIOD_ONEMONTH;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_RTC_ERR_STR, "[period]", EX_COMM_STR_OPT_OPS);
        goto err;
    }

    un8Arg++;
    if(strncmp(pn8Argv[un8Arg], "-24h", 4) == 0)
    {
        un8Arg++;
        ptCfg->b24Hour = true;
        ptCfg->tTime.un8Hour = (TO_HEX(un8Hour));
        s_b24Hour = ptCfg->b24Hour;
    }
    else
    {
        ptCfg->b24Hour = false;

        if (un8Hour == 0)
        {
            *pbPm = false;
            ptCfg->tTime.un8Hour = (TO_HEX(12));
        }
        else if(un8Hour < 12)
        {
            *pbPm = false;
            ptCfg->tTime.un8Hour = (TO_HEX(un8Hour));
        }
        else if(un8Hour == 12)
        {
            *pbPm = true;
            ptCfg->tTime.un8Hour = (TO_HEX(32));
        }
        else
        {
            *pbPm = true;
            ptCfg->tTime.un8Hour = (TO_HEX(un8Hour) + 0x20);
        }
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static void EX_RTC_IRQHandler(uint32_t un32Event, void *pContext)
{
    RTC_ID_e eId = RTC_ID_0;
    RTC_Context_t *ptContext = (RTC_Context_t *)pContext;
    RTC_TIME_t tTime;
    RTC_DATE_t tDate;
    bool b24Hour = false, bPm = false;

    eId = ptContext->eId;
    b24Hour = ptContext->b24Hour;

    if(un32Event & RTC_EVENT_PERIOD)
    {
        HAL_RTC_GetClock(eId, &tTime, &tDate);
        if(b24Hour == true)
        {
            LOG("RTC : PERIOD 20%02d-%02d-%02d (%s) %02d:%02d:%02d\n", FROM_HEX(tDate.un8Year), FROM_HEX(tDate.un8Month),
                 FROM_HEX(tDate.un8Day), s_an8TCWeek[FROM_HEX(tDate.un8Week)], FROM_HEX(tTime.un8Hour),
                 FROM_HEX(tTime.un8Min), FROM_HEX(tTime.un8Sec));
        }
        else
        {
            if(tTime.un8Hour > 0x12)
            {
                bPm = true;
            }

            LOG("RTC : PERIOD 20%02d-%02d-%02d (%s) %s. %02d:%02d:%02d\n", FROM_HEX(tDate.un8Year), FROM_HEX(tDate.un8Month),
                 FROM_HEX(tDate.un8Day), s_an8TCWeek[FROM_HEX(tDate.un8Week)], (bPm == true ? "PM" : "AM"),
                 (tTime.un8Hour == 0x32 || tTime.un8Hour == 0x12) ? 12 : (bPm == true ? FROM_HEX(tTime.un8Hour) - 20 : FROM_HEX(tTime.un8Hour)), 
                 FROM_HEX(tTime.un8Min), FROM_HEX(tTime.un8Sec));
        }

    }

    if(un32Event & RTC_EVENT_ALARM)
    {
        LOG("RTC : ALARM event is fired! (ID = %d)\n",eId);
    }

    if(un32Event & RTC_EVENT_TIMESTAMP)
    {
        HAL_RTC_GetTimestamp(eId, &tTime, &tDate);
        if(b24Hour == true)
        {
            LOG("RTC : TIMESTAMP 20%02d-%02d-%02d (%s) %02d:%02d:%02d\n", FROM_HEX(tDate.un8Year), FROM_HEX(tDate.un8Month),
                 FROM_HEX(tDate.un8Day), s_an8TCWeek[FROM_HEX(tDate.un8Week)], FROM_HEX(tTime.un8Hour),
                 FROM_HEX(tTime.un8Min), FROM_HEX(tTime.un8Sec));
        }
        else
        {
            if(tTime.un8Hour > 0x12)
            {
                bPm = true;
            }

            LOG("RTC : TIMESTAMP 20%02d-%02d-%02d (%s) %s. %02d:%02d:%02d\n", FROM_HEX(tDate.un8Year), FROM_HEX(tDate.un8Month),
                 FROM_HEX(tDate.un8Day), s_an8TCWeek[FROM_HEX(tDate.un8Week)], (bPm == true ? "PM" : "AM"),
                 (tTime.un8Hour == 0x32 || tTime.un8Hour == 0x12) ? 12 : (bPm == true ? FROM_HEX(tTime.un8Hour) - 20 : FROM_HEX(tTime.un8Hour)), 
                 FROM_HEX(tTime.un8Min), FROM_HEX(tTime.un8Sec));
        }
    }

    /* meaningless, it is only for removing compile warning */
    (void)bPm;
    (void)s_an8TCWeek;
}

static enum debug_cmd_status EX_RTC_Init(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    RTC_ID_e eId = RTC_ID_0;

    eDbgStatus = EX_RTC_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_RTC_Init(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_RTC_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_INIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_RTC_Uninit(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    RTC_ID_e eId = RTC_ID_0;

    eDbgStatus = EX_RTC_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_RTC_Uninit(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_RTC_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_UNINIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_RTC_SetClkConfig(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    RTC_ID_e eId;
    RTC_CLK_CFG_t tClkCfg; 

    eDbgStatus = EX_RTC_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    memset(&tClkCfg, 0x00, sizeof(RTC_CLK_CFG_t));

    eDbgStatus = EX_RTC_GetClkConfig(n32Argc, pn8Argv, &tClkCfg);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_RTC_SetClkConfig(eId, &tClkCfg);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_RTC_ERR_STR, "ClkCfg", EX_COMM_STR_OPT_MAX);
        }
        goto err;

    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_RTC_SetConfig(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus;
    RTC_ID_e eId = RTC_ID_0;
    RTC_OPS_e eOps = RTC_OPS_POLL;
    RTC_CFG_t tCfg;

    eDbgStatus = EX_RTC_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    memset(&tCfg, 0x00, sizeof(RTC_CFG_t));

    eDbgStatus = EX_RTC_GetConfig(n32Argc, pn8Argv, &tCfg, &eOps);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_RTC_SetConfig(eId, &tCfg);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    s_tRTCContext[eId].eId = eId;

    eErr = HAL_RTC_SetIRQ(eId, eOps, EX_RTC_IRQHandler, &s_tRTCContext[eId], EX_RTC_IRQ_PRIO);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_RTC_ERR_STR, "IRQ", EX_COMM_STR_OPT_MAX);
        }
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_RTC_SetClock(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    RTC_ID_e eId = RTC_ID_0;
    RTC_CLOCK_t tClockCfg;
    bool bPm = false;

    eDbgStatus = EX_RTC_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    memset(&tClockCfg, 0x00, sizeof(RTC_CLOCK_t));

    eDbgStatus = EX_RTC_GetClockConfig(n32Argc, pn8Argv, &tClockCfg, &bPm);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    s_tRTCContext[eId].b24Hour = tClockCfg.b24Hour;

    eErr = HAL_RTC_SetClock(eId, &tClockCfg);
    if(eErr != HAL_ERR_OK)
    {
        LOG("%s SetClock\n", EX_RTC_ERR_STR);
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_RTC_SetAlarm(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    RTC_ID_e eId = RTC_ID_0;
    RTC_ALARM_t tAlarmCfg;
    uint8_t un8Arg = 2, un8Data = 0, i = 0;

    eDbgStatus = EX_RTC_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    memset(&tAlarmCfg, 0x00, sizeof(RTC_ALARM_t));

    eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], &tAlarmCfg.bAlarmEn);
    if (eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_RTC_ERR_STR, NULL, EX_COMM_STR_OPT_ENA);
        goto err;
    }

    if(tAlarmCfg.bAlarmEn == true)
    {
        while(i < 14)
        {
            switch(pn8Argv[un8Arg][i++])
            {
                case 'a':
                    tAlarmCfg.un8Week = 0x7F;
                    break;
                case 's':
                    tAlarmCfg.un8Week |= RTC_ALARM_WEEK_SUN;
                    break;
                case 'm':
                    tAlarmCfg.un8Week |= RTC_ALARM_WEEK_MON;
                    break;
                case 't':
                    tAlarmCfg.un8Week |= RTC_ALARM_WEEK_TUE;
                    break;
                case 'w':
                    tAlarmCfg.un8Week |= RTC_ALARM_WEEK_WED;
                    break;
                case 'h':
                    tAlarmCfg.un8Week |= RTC_ALARM_WEEK_THU;
                    break;
                case 'f':
                    tAlarmCfg.un8Week |= RTC_ALARM_WEEK_FRI;
                    break;
                case 'r':
                    tAlarmCfg.un8Week |= RTC_ALARM_WEEK_SAT;
                    break;
                default:
                    EX_COMMON_SetShowModuleLog(EX_RTC_ERR_STR, "[ww]", EX_COMM_STR_OPT_MAX);
                    goto err;
            }

            if(pn8Argv[un8Arg][i++] != ':' || tAlarmCfg.un8Week == 0x7F)
            {
                break;
            }
            
        }

        un8Arg++;
        un8Data = (uint8_t)atoi(pn8Argv[un8Arg++]);
        if(un8Data < 24)
        {
            if(s_b24Hour == true)
            {
                tAlarmCfg.un8Hour = TO_HEX(un8Data);
            }
            else
            {
                if (un8Data == 0)
                {
                    tAlarmCfg.un8Hour = (TO_HEX(12));
                }
                else if(un8Data < 12)
                {
                    tAlarmCfg.un8Hour = (TO_HEX(un8Data));
                }
                else if(un8Data == 12)
                {
                    tAlarmCfg.un8Hour = (TO_HEX(32));
                }
                else
                {
                    tAlarmCfg.un8Hour = (TO_HEX(un8Data) + 0x20);
                }
            }
        }
        else
        {
            EX_COMMON_SetShowModuleLog(EX_RTC_ERR_STR, "[hh]", EX_COMM_STR_OPT_MAX);
            goto err;
        }

        un8Data = (uint8_t)atoi(pn8Argv[un8Arg++]);
        if(un8Data < 60)
        {
            tAlarmCfg.un8Min = TO_HEX(un8Data);
        }
        else
        {
            EX_COMMON_SetShowModuleLog(EX_RTC_ERR_STR, "[mm]", EX_COMM_STR_OPT_MAX);
            goto err;
        }
    }
     
    eErr = HAL_RTC_SetAlarm(eId, &tAlarmCfg);
    if(eErr != HAL_ERR_OK)
    {
        LOG("%s SetAlarm\n", EX_RTC_ERR_STR);
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_RTC_SetTimestamp(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    RTC_ID_e eId = RTC_ID_0;
    uint8_t un8Arg = 2;
    bool bEnable = false;
    RTC_TS_EVT_e eTsEvt = RTC_TS_EVT_MAX;

    eDbgStatus = EX_RTC_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], &bEnable);
    if (eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_RTC_ERR_STR, NULL, EX_COMM_STR_OPT_ENA);
        goto err;
    }

    if(bEnable == true)
    {
        switch (pn8Argv[un8Arg++][0])
        {
            case 'f':
                eTsEvt = RTC_TS_EVT_FALLING;
                break;
            case 'r':
                eTsEvt = RTC_TS_EVT_RISING;
                break;
            case 'b':
                eTsEvt = RTC_TS_EVT_BOTH;
                break;
            default:
                EX_COMMON_SetShowModuleLog(EX_RTC_ERR_STR, NULL, EX_COMM_STR_OPT_EDGE);
                goto err;
        }
    }

    eErr = HAL_RTC_SetTimestamp(eId, bEnable, eTsEvt);
    if(eErr != HAL_ERR_OK)
    {
        LOG("%s SetTimestamp\n", EX_RTC_ERR_STR);
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_RTC_Start(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    RTC_ID_e eId = RTC_ID_0;

    eDbgStatus = EX_RTC_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_RTC_Start(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_RTC_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_START]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_RTC_Stop(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    RTC_ID_e eId = RTC_ID_0;

    eDbgStatus = EX_RTC_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_RTC_Stop(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_RTC_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_STOP]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_RTC_SetWakeup(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    RTC_ID_e eId = RTC_ID_0;
    uint8_t un8Arg = 2;
    bool bEnable = false, bExtPinEnable = false;

    eDbgStatus = EX_RTC_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], &bEnable);
    if (eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_RTC_ERR_STR, NULL, EX_COMM_STR_OPT_ENA);
        goto err;
    }

    eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], &bExtPinEnable);
    if (eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_RTC_ERR_STR, "[ext]", EX_COMM_STR_OPT_MAX);
        goto err;
    }

    eErr = HAL_RTC_SetWakeupSrc(eId, bEnable, bExtPinEnable);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static const struct debug_cmd s_tEX_RTC_CMD[] =
{
    {"RTC", "h", EX_RTC_Help,"help"},
    {"RTC", "init", EX_RTC_Init, ""},
    {"RTC", "uninit", EX_RTC_Uninit, ""},
    {"RTC", "clk", EX_RTC_SetClkConfig, ""},
    {"RTC", "config", EX_RTC_SetConfig, ""},
    {"RTC", "clock", EX_RTC_SetClock, ""},
    {"RTC", "alarm", EX_RTC_SetAlarm, ""},
    {"RTC", "timestamp", EX_RTC_SetTimestamp, ""},
    {"RTC", "start", EX_RTC_Start, ""},
    {"RTC", "stop", EX_RTC_Stop, ""},
    {"RTC", "wkup", EX_RTC_SetWakeup, ""}

};

void EX_RTC(void)
{
    /* Add TC commands */
    debug_cmd_init(s_tEX_RTC_CMD, DEBUG_CMD_LIST_COUNT(s_tEX_RTC_CMD));
}

#endif /* RTC_TC */

/* --------------------------------- End Of File ------------------------------ */
