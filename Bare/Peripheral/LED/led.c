/**
 *******************************************************************************
 * @file        led.c
 * @author      ABOV R&D Division
 * @brief       LED Example Code
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

#if defined(LED_TC)
#include "abov_config.h"
#include "ex_common.h"
#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "hal_scu.h"
#include "hal_scu_clk.h"
#include "hal_pcu.h"
#include "hal_led.h"

#if !defined(_LED)
#error "This chipset did not support this example."
#endif

#if defined(EXTRN_EX_LED_DEBUG_OTHER)
#warning The debug channel of this example should be use to UART1.
#define EX_LED_UART1_LOG "WARN : The debug channel of LED example should be use to UART1."
#endif

extern uint32_t SystemCoreClock;
static LED_OPS_e s_eOps = LED_OPS_POLL;
static bool s_bISRLog = false;
static LED_DATA_t tData[CONFIG_LED_MAX_ICOM_CH_NUM];
static uint8_t s_un8IcomCnt = 0, s_un8PortCnt = 0;
static bool s_bPortOps = false;

#define EX_LED_STR "LED"
#define EX_LED_LOG_STR "LED :"
#define EX_LED_ERR_STR "[E]LED :"
#define EX_LED_IRQ_PRIO 3
#define EX_LED_MAX_NUM  (CONFIG_LED_MAX_COUNT - 1)

extern const char *pcCommStr[];
extern const char *pcCmdStr[];
extern const char *pcValStr[];
extern const char *pcOptStr[];

static LED_Context_t s_tLEDContext[CONFIG_LED_MAX_COUNT];

typedef struct {
    uint8_t         un8IsegId;
    PCU_ID_e        ePortgroup;
    PCU_PIN_ID_e    ePortNum;
    char            *pPortName;
} LED_ISEG_PORT_t;

typedef struct {
    uint8_t         un8IcomId;
    PCU_ID_e        ePortgroup;
    PCU_PIN_ID_e    ePortNum;
    char           *pPortName;
} LED_ICOM_PORT_t;

LED_ISEG_PORT_t EX_LED_ISEG[CONFIG_LED_MAX_ISEG_CH_NUM] = {
#if (CONFIG_LED_MAX_ISEG_CH_NUM > 0)
    {0, (PCU_ID_e)LED_ISEG0_ID, (PCU_PIN_ID_e)LED_ISEG0_PORT_ID, "ISEG0"},
#endif
#if (CONFIG_LED_MAX_ISEG_CH_NUM > 1)
    {1, (PCU_ID_e)LED_ISEG1_ID, (PCU_PIN_ID_e)LED_ISEG1_PORT_ID, "ISEG1"},
#endif
#if (CONFIG_LED_MAX_ISEG_CH_NUM > 2)
    {2, (PCU_ID_e)LED_ISEG2_ID, (PCU_PIN_ID_e)LED_ISEG2_PORT_ID, "ISEG2"},
#endif
#if (CONFIG_LED_MAX_ISEG_CH_NUM > 3)
    {3, (PCU_ID_e)LED_ISEG3_ID, (PCU_PIN_ID_e)LED_ISEG3_PORT_ID, "ISEG3"},
#endif
#if (CONFIG_LED_MAX_ISEG_CH_NUM > 4)
    {4, (PCU_ID_e)LED_ISEG4_ID, (PCU_PIN_ID_e)LED_ISEG4_PORT_ID, "ISEG4"},
#endif
#if (CONFIG_LED_MAX_ISEG_CH_NUM > 5)
    {5, (PCU_ID_e)LED_ISEG5_ID, (PCU_PIN_ID_e)LED_ISEG5_PORT_ID, "ISEG5"},
#endif
#if (CONFIG_LED_MAX_ISEG_CH_NUM > 6)
    {6, (PCU_ID_e)LED_ISEG6_ID, (PCU_PIN_ID_e)LED_ISEG6_PORT_ID, "ISEG6"},
#endif
#if (CONFIG_LED_MAX_ISEG_CH_NUM > 7)
    {7, (PCU_ID_e)LED_ISEG7_ID, (PCU_PIN_ID_e)LED_ISEG7_PORT_ID, "ISEG7"},
#endif
#if (CONFIG_LED_MAX_ISEG_CH_NUM > 8)
    {8, (PCU_ID_e)LED_ISEG8_ID, (PCU_PIN_ID_e)LED_ISEG8_PORT_ID, "ISEG8"},
#endif
#if (CONFIG_LED_MAX_ISEG_CH_NUM > 9)
    {9, (PCU_ID_e)LED_ISEG9_ID, (PCU_PIN_ID_e)LED_ISEG9_PORT_ID, "ISEG9"},
#endif
#if (CONFIG_LED_MAX_ISEG_CH_NUM > 10)
    {10, (PCU_ID_e)LED_ISEG10_ID, (PCU_PIN_ID_e)LED_ISEG10_PORT_ID, "ISEG10"},
#endif
#if (CONFIG_LED_MAX_ISEG_CH_NUM > 11)
    {10, (PCU_ID_e)LED_ISEG11_ID, (PCU_PIN_ID_e)LED_ISEG11_PORT_ID, "ISEG11"},
#endif
#if (CONFIG_LED_MAX_ISEG_CH_NUM > 12)
    {10, (PCU_ID_e)LED_ISEG12_ID, (PCU_PIN_ID_e)LED_ISEG12_PORT_ID, "ISEG12"},
#endif
#if (CONFIG_LED_MAX_ISEG_CH_NUM > 13)
    {10, (PCU_ID_e)LED_ISEG13_ID, (PCU_PIN_ID_e)LED_ISEG13_PORT_ID, "ISEG13"},
#endif
#if (CONFIG_LED_MAX_ISEG_CH_NUM > 14)
    {10, (PCU_ID_e)LED_ISEG14_ID, (PCU_PIN_ID_e)LED_ISEG14_PORT_ID, "ISEG14"},
#endif
#if (CONFIG_LED_MAX_ISEG_CH_NUM > 15)
    {10, (PCU_ID_e)LED_ISEG15_ID, (PCU_PIN_ID_e)LED_ISEG15_PORT_ID, "ISEG15"},
#endif
#if (CONFIG_LED_MAX_ISEG_CH_NUM > 16)
    {10, (PCU_ID_e)LED_ISEG16_ID, (PCU_PIN_ID_e)LED_ISEG16_PORT_ID, "ISEG16"},
#endif
};

LED_ICOM_PORT_t EX_LED_ICOM[CONFIG_LED_MAX_ICOM_CH_NUM] = {
#if (CONFIG_LED_MAX_ICOM_CH_NUM > 0)
    {0, (PCU_ID_e)LED_ICOM0_ID, (PCU_PIN_ID_e)LED_ICOM0_PORT_ID, "ICOM0"},
#endif
#if (CONFIG_LED_MAX_ICOM_CH_NUM > 1)
    {1, (PCU_ID_e)LED_ICOM1_ID, (PCU_PIN_ID_e)LED_ICOM1_PORT_ID, "ICOM1"},
#endif
#if (CONFIG_LED_MAX_ICOM_CH_NUM > 2)
    {2, (PCU_ID_e)LED_ICOM2_ID, (PCU_PIN_ID_e)LED_ICOM2_PORT_ID, "ICOM2"},
#endif
#if (CONFIG_LED_MAX_ICOM_CH_NUM > 3)
    {3, (PCU_ID_e)LED_ICOM3_ID, (PCU_PIN_ID_e)LED_ICOM3_PORT_ID, "ICOM3"},
#endif
#if (CONFIG_LED_MAX_ICOM_CH_NUM > 4)
    {4, (PCU_ID_e)LED_ICOM4_ID, (PCU_PIN_ID_e)LED_ICOM4_PORT_ID, "ICOM4"},
#endif
#if (CONFIG_LED_MAX_ICOM_CH_NUM > 5)
    {5, (PCU_ID_e)LED_ICOM5_ID, (PCU_PIN_ID_e)LED_ICOM5_PORT_ID, "ICOM5"},
#endif
#if (CONFIG_LED_MAX_ICOM_CH_NUM > 6)
    {6, (PCU_ID_e)LED_ICOM6_ID, (PCU_PIN_ID_e)LED_ICOM6_PORT_ID, "ICOM6"},
#endif
#if (CONFIG_LED_MAX_ICOM_CH_NUM > 7)
    {7, (PCU_ID_e)LED_ICOM7_ID, (PCU_PIN_ID_e)LED_ICOM7_PORT_ID, "ICOM7"},
#endif
#if (CONFIG_LED_MAX_ICOM_CH_NUM > 8)
    {8, (PCU_ID_e)LED_ICOM8_ID, (PCU_PIN_ID_e)LED_ICOM8_PORT_ID, "ICOM8"},
#endif
#if (CONFIG_LED_MAX_ICOM_CH_NUM > 9)
    {9, (PCU_ID_e)LED_ICOM9_ID, (PCU_PIN_ID_e)LED_ICOM9_PORT_ID, "ICOM9"},
#endif
#if (CONFIG_LED_MAX_ICOM_CH_NUM > 10)
    {10, (PCU_ID_e)LED_ICOM10_ID, (PCU_PIN_ID_e)LED_ICOM10_PORT_ID, "ICOM10"},
#endif
#if (CONFIG_LED_MAX_ICOM_CH_NUM > 11)
    {11, (PCU_ID_e)LED_ICOM11_ID, (PCU_PIN_ID_e)LED_ICOM11_PORT_ID, "ICOM11"},
#endif
#if (CONFIG_LED_MAX_ICOM_CH_NUM > 12)
    {12, (PCU_ID_e)LED_ICOM12_ID, (PCU_PIN_ID_e)LED_ICOM12_PORT_ID, "ICOM12"},
#endif
#if (CONFIG_LED_MAX_ICOM_CH_NUM > 13)
    {13, (PCU_ID_e)LED_ICOM13_ID, (PCU_PIN_ID_e)LED_ICOM13_PORT_ID, "ICOM13"},
#endif
#if (CONFIG_LED_MAX_ICOM_CH_NUM > 14)
    {14, (PCU_ID_e)LED_ICOM14_ID, (PCU_PIN_ID_e)LED_ICOM14_PORT_ID, "ICOM14"},
#endif
#if (CONFIG_LED_MAX_ICOM_CH_NUM > 15)
    {15, (PCU_ID_e)LED_ICOM15_ID, (PCU_PIN_ID_e)LED_ICOM15_PORT_ID, "ICOM15"},
#endif
#if (CONFIG_LED_MAX_ICOM_CH_NUM > 16)
    {16, (PCU_ID_e)LED_ICOM16_ID, (PCU_PIN_ID_e)LED_ICOM16_PORT_ID, "ICOM16"},
#endif
#if (CONFIG_LED_MAX_ICOM_CH_NUM > 17)
    {17, (PCU_ID_e)LED_ICOM17_ID, (PCU_PIN_ID_e)LED_ICOM17_PORT_ID, "ICOM17"},
#endif
#if (CONFIG_LED_MAX_ICOM_CH_NUM > 18)
    {18, (PCU_ID_e)LED_ICOM18_ID, (PCU_PIN_ID_e)LED_ICOM18_PORT_ID, "ICOM18"},
#endif
#if (CONFIG_LED_MAX_ICOM_CH_NUM > 19)
    {19, (PCU_ID_e)LED_ICOM19_ID, (PCU_PIN_ID_e)LED_ICOM17_PORT_ID, "ICOM19"},
#endif
#if (CONFIG_LED_MAX_ICOM_CH_NUM > 20)
    {20, (PCU_ID_e)LED_ICOM20_ID, (PCU_PIN_ID_e)LED_ICOM18_PORT_ID, "ICOM20"},
#endif
#if (CONFIG_LED_MAX_ICOM_CH_NUM > 21)
    {21, (PCU_ID_e)LED_ICOM21_ID, (PCU_PIN_ID_e)LED_ICOM17_PORT_ID, "ICOM21"},
#endif
#if (CONFIG_LED_MAX_ICOM_CH_NUM > 22)
    {22, (PCU_ID_e)LED_ICOM22_ID, (PCU_PIN_ID_e)LED_ICOM18_PORT_ID, "ICOM22"},
#endif
#if (CONFIG_LED_MAX_ICOM_CH_NUM > 23)
    {23, (PCU_ID_e)LED_ICOM23_ID, (PCU_PIN_ID_e)LED_ICOM17_PORT_ID, "ICOM23"},
#endif
#if (CONFIG_LED_MAX_ICOM_CH_NUM > 24)
    {24, (PCU_ID_e)LED_ICOM24_ID, (PCU_PIN_ID_e)LED_ICOM24_PORT_ID, "ICOM24"},
#endif
#if (CONFIG_LED_MAX_ICOM_CH_NUM > 25)
    {25, (PCU_ID_e)LED_ICOM25_ID, (PCU_PIN_ID_e)LED_ICOM25_PORT_ID, "ICOM25"},
#endif
#if (CONFIG_LED_MAX_ICOM_CH_NUM > 26)
    {26, (PCU_ID_e)LED_ICOM26_ID, (PCU_PIN_ID_e)LED_ICOM26_PORT_ID, "ICOM26"},
#endif
};

static enum debug_cmd_status EX_LED_Help(int32_t n32Argc, char *pn8Argv[])
{
    uint32_t un32ShowOpt = 0;
    EX_COMM_STR_OPT_e eOpt[2];

    EX_COMMON_SetShowModuleInfo(EX_LED_STR, CONFIG_LED_MAX_COUNT);

    un32ShowOpt = EX_COMMON_GetShowOpt(pn8Argv[1]);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_INIT, NULL, EX_LED_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_UNINIT, NULL, EX_LED_MAX_NUM, eOpt, 0, NULL);

    eOpt[0] = EX_COMM_STR_OPT_SRC; 
    eOpt[1] = EX_COMM_STR_OPT_DIV;
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CLK, NULL, EX_LED_MAX_NUM, eOpt, 2, NULL);
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_SRC, EX_COMM_STR_VAL_CLKPATH, NULL);
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MCCR, EX_COMM_STR_VAL_MAX, "[mccr] [div]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MCCR, EX_COMM_STR_VAL_CLKSRC, "/m(mclk)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_DIV, EX_COMM_STR_VAL_N_255, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_DIV, EX_COMM_STR_VAL_N_NUM, NULL);
    }
    eOpt[0] = EX_COMM_STR_OPT_OPS; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CONFIG, NULL, EX_LED_MAX_NUM, eOpt, 1, "[way] [mode] [share] [width] [duration] [-overlap]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_OPS, EX_COMM_STR_VAL_OPS, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "way: r(display ram)/p(port)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "mode: a(alone)/h(hand-share)/c(stop count)/s(smart share)/t(auto)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "share: 1(iseq0-iseq1)/2(iseg0-icom26)/3(icom25-iseg1)/4(icom25-icom26)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "width: 0~N");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "duration: 0~N");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-overlap: [time]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "time: 8,16,32,64,128,256,512,1024");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "icom", EX_LED_MAX_NUM, eOpt, 0, "[num] ...");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_NUM, EX_COMM_STR_VAL_N_NUM, NULL);
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "iseg", EX_LED_MAX_NUM, eOpt, 0, "[num] ...");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_NUM, EX_COMM_STR_VAL_N_NUM, NULL);
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "dim", EX_LED_MAX_NUM, eOpt, 0, "[icom] [da] ...");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "icom: 0~N");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_DA, EX_COMM_STR_VAL_N_NUM, NULL);
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "data", EX_LED_MAX_NUM, eOpt, 0, "[icom] [iseg] ...");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "icom: 0~N");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "iseg: 0x0~0xN(hexa)");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_START, NULL, EX_LED_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_STOP, NULL, EX_LED_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_LOG, NULL, -1, eOpt, 0, "on [cnt] / off");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_CNT, EX_COMM_STR_VAL_N_NUM, NULL);
    }
    LOG("\n");

    return DEBUG_CMD_SUCCESS;
}

static void EX_LED_IRQHandler(uint32_t un32Event, void *pContext)
{
    if(un32Event & LED_EVENT_MATCH)
    {
        if(s_bISRLog == true)
        {
            LOG("%s (%d) M evt fired\n", EX_LED_LOG_STR, ((LED_Context_t *)pContext)->eId);
        }
    }
    else if (un32Event & LED_EVENT_END)
    {
        if(s_bISRLog == true)
        {
            LOG("%s (%d) E evt fired\n", EX_LED_LOG_STR, ((LED_Context_t *)pContext)->eId);
        }

    }
    else if (un32Event & LED_EVENT_INTR)
    {
        if(s_bISRLog == true)
        {
            LOG("%s (%d) intr evt fired\n", EX_LED_LOG_STR, ((LED_Context_t *)pContext)->eId);
        }
    }

    if(s_bPortOps == true)
    {
        (void)HAL_LED_SetIsegOutput(LED_ID_0, (tData[s_un8PortCnt].un32Data));
        (void)HAL_LED_SetIcomOutput(LED_ID_0, (0x1UL << tData[s_un8PortCnt++].un8Num));
        if(s_un8PortCnt > s_un8IcomCnt)
        {
            s_un8PortCnt = 0;
        }
    }
    (void)HAL_LED_Start(LED_ID_0);
}

static enum debug_cmd_status EX_LED_GetId(int32_t n32Argc, char *pn8Argv[], LED_ID_e *peId)
{
    uint8_t un8Data = 0;
    if(n32Argc == 1)
    {
        EX_COMMON_SetShowModuleLog(EX_LED_ERR_STR, (char *)pcOptStr[EX_COMM_STR_OPT_ARG], EX_COMM_STR_OPT_MAX);
        return DEBUG_CMD_INVALID;
    }

    un8Data = atoi(pn8Argv[1]); 
    if(un8Data >= CONFIG_LED_MAX_COUNT)
    {
        LOG("%s Max Chan %d\n", EX_LED_ERR_STR, CONFIG_LED_MAX_COUNT);
        return DEBUG_CMD_INVALID;
    }

    *peId = (LED_ID_e)un8Data;

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_LED_GetClkConfig(int32_t n32Argc, char *pn8Argv[], LED_CLK_CFG_t *ptClkCfg)
{
    uint8_t un8Arg = 2;

    switch (pn8Argv[un8Arg++][0])
    {
        case 'p':
            ptClkCfg->eClk = LED_CLK_PCLK;
            break;
        case 'm':
            ptClkCfg->eClk = LED_CLK_MCCR;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_LED_ERR_STR, NULL, EX_COMM_STR_OPT_SRC);
            goto err;
    }

    if(ptClkCfg->eClk == LED_CLK_MCCR)
    {
        if (pn8Argv[un8Arg][0] == 'l')
            ptClkCfg->eMccr = LED_CLK_MCCR_LSI;
        else if (pn8Argv[un8Arg][0] == 's')
            ptClkCfg->eMccr = LED_CLK_MCCR_LSE;
        else if (pn8Argv[un8Arg][0] == 'm')
            ptClkCfg->eMccr = LED_CLK_MCCR_MCLK;
        else if (pn8Argv[un8Arg][0] == 'h')
            ptClkCfg->eMccr = LED_CLK_MCCR_HSI;
        else if (pn8Argv[un8Arg][0] == 'e')
            ptClkCfg->eMccr = LED_CLK_MCCR_HSE;
        else if (pn8Argv[un8Arg][0] == 'p')
            ptClkCfg->eMccr = LED_CLK_MCCR_PLL;
        else
        {
            EX_COMMON_SetShowModuleLog(EX_LED_ERR_STR, NULL, EX_COMM_STR_OPT_MCCR);
            goto err;
        }

        un8Arg++;
        ptClkCfg->un8MccrDiv = (uint8_t)atoi(pn8Argv[un8Arg++]);

    }

    ptClkCfg->un16Prescale = (uint16_t)atoi(pn8Argv[un8Arg++]);

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_LED_GetConfig(int32_t n32Argc, char *pn8Argv[], LED_CFG_t *ptCfg, LED_OPS_e *peOps)
{
    uint8_t un8Arg = 2;
    uint16_t un16Data = 0;

    switch (pn8Argv[un8Arg++][0])
    {
        case 'p':
            *peOps = LED_OPS_POLL;
            break;
        case 'i':
            *peOps = LED_OPS_INTR;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_LED_ERR_STR, NULL, EX_COMM_STR_OPT_OPS);
            goto err;
 
    }

    switch (pn8Argv[un8Arg++][0])
    {
        case 'r':
            s_bPortOps = false;
            break;
        case 'p':
            s_bPortOps = true;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_LED_ERR_STR, "[way]", EX_COMM_STR_OPT_MAX);
            goto err;
    }

    if (pn8Argv[un8Arg][0] == 'a')
        ptCfg->eLedMode = LED_MODE_ALONE;
    else if (pn8Argv[un8Arg][0] == 'h')
        ptCfg->eLedMode = LED_MODE_HANDSHAKE;
    else if (pn8Argv[un8Arg][0] == 'c')
        ptCfg->eLedMode = LED_MODE_STOPCOUNT;
    else if (pn8Argv[un8Arg][0] == 's')
        ptCfg->eLedMode = LED_MODE_SMARTSHARE;
    else if (pn8Argv[un8Arg][0] == 't')
        ptCfg->eLedMode = LED_MODE_AUTO;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_LED_ERR_STR, NULL, EX_COMM_STR_OPT_MODE);
        goto err;
    }

    un8Arg++;
    if (pn8Argv[un8Arg][0] == '1')
        ptCfg->eSharePin = LED_SHARE_PIN_ISEG0_ISEG1;
    else if (pn8Argv[un8Arg][0] == '2')
        ptCfg->eSharePin = LED_SHARE_PIN_ISEG0_ICOM26;
    else if (pn8Argv[un8Arg][0] == '3')
        ptCfg->eSharePin = LED_SHARE_PIN_ICOM25_ISEG1;
    else if (pn8Argv[un8Arg][0] == '4')
        ptCfg->eSharePin = LED_SHARE_PIN_ICOM25_ICOM26;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_LED_ERR_STR, "[share]", EX_COMM_STR_OPT_MODE);
        goto err;
    }

    un8Arg++;
    ptCfg->un8IcomPulse = (uint8_t)atoi(pn8Argv[un8Arg++]);
    ptCfg->un32Duration = (uint32_t)atoi(pn8Argv[un8Arg++]);

    if(strncmp(pn8Argv[un8Arg], "-overlap", 8) == 0)
    {
        un8Arg++;
        ptCfg->tOverlap.bEnable = true;
        un16Data = (uint16_t)atoi(pn8Argv[un8Arg++]);
        if(un16Data == 8)
            ptCfg->tOverlap.eTime = LED_OVERLAP_TIME_DIV_8;
        else if(un16Data == 16)
            ptCfg->tOverlap.eTime = LED_OVERLAP_TIME_DIV_16;
        else if(un16Data == 32)
            ptCfg->tOverlap.eTime = LED_OVERLAP_TIME_DIV_32;
        else if(un16Data == 64)
            ptCfg->tOverlap.eTime = LED_OVERLAP_TIME_DIV_64;
        else if(un16Data == 128)
            ptCfg->tOverlap.eTime = LED_OVERLAP_TIME_DIV_128;
        else if(un16Data == 256)
            ptCfg->tOverlap.eTime = LED_OVERLAP_TIME_DIV_256;
        else if(un16Data == 512)
            ptCfg->tOverlap.eTime = LED_OVERLAP_TIME_DIV_512;
        else if(un16Data == 1024)
            ptCfg->tOverlap.eTime = LED_OVERLAP_TIME_DIV_1024;
        else
        {
            EX_COMMON_SetShowModuleLog(EX_LED_ERR_STR, "-overlap[time]", EX_COMM_STR_OPT_MODE);
            goto err;
        }
    }
    else
    {
        un8Arg++;
        ptCfg->tOverlap.bEnable = false;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_LED_Init(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    LED_ID_e eId = LED_ID_0;

    eDbgStatus = EX_LED_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_LED_Init(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_LED_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_INIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_LED_Uninit(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    LED_ID_e eId = LED_ID_0;

    eDbgStatus = EX_LED_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_LED_Uninit(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_LED_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_UNINIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_LED_SetClkConfig(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    LED_ID_e eId;
    LED_CLK_CFG_t tClkCfg; 

    eDbgStatus = EX_LED_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    memset(&tClkCfg, 0x00, sizeof(LED_CLK_CFG_t));

    eDbgStatus = EX_LED_GetClkConfig(n32Argc, pn8Argv, &tClkCfg);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_LED_SetClkConfig(eId, &tClkCfg);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_LED_ERR_STR, "ClkCfg", EX_COMM_STR_OPT_MAX);
        }
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_LED_SetConfig(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus;
    LED_ID_e eId = LED_ID_0;
    LED_CFG_t tCfg;

    eDbgStatus = EX_LED_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    memset(&tCfg, 0x00, sizeof(LED_CFG_t));

    eDbgStatus = EX_LED_GetConfig(n32Argc, pn8Argv, &tCfg, &s_eOps);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_LED_SetConfig(eId, &tCfg);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_LED_ERR_STR, "Cfg", EX_COMM_STR_OPT_MAX);
        }
        goto err;
    }

    s_tLEDContext[eId].eId = eId;

    eErr = HAL_LED_SetIRQ(eId, s_eOps, EX_LED_IRQHandler, &s_tLEDContext[eId], EX_LED_IRQ_PRIO);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_LED_ERR_STR, "IRQ", EX_COMM_STR_OPT_MAX);
        }
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_LED_Start(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    LED_ID_e eId = LED_ID_0;
    LED_DATA_t tPreDispRam;

    eDbgStatus = EX_LED_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    if(s_bPortOps == true)
    {
        for( int i = 0; i < CONFIG_LED_MAX_ICOM_CH_NUM; i++)
        {
            tPreDispRam.un8Num = i;
            tPreDispRam.un32Data = 0x7FFFFFFF;
            eErr = HAL_LED_SetDispData(eId, &tPreDispRam, 1);  /** Dimming init */
        }
    }

    s_un8PortCnt = 0;

    eErr = HAL_LED_Start(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_LED_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_START]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_LED_Stop(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    LED_ID_e eId = LED_ID_0;

    eDbgStatus = EX_LED_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_LED_Stop(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_LED_LOG_STR, eId, pcCmdStr[EX_COMM_STR_CMD_STOP]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_LED_SetIcom(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus;
    LED_ID_e eId = LED_ID_0;
    uint8_t un8Idx = 2;
    uint32_t un32Enable = 0;

    eDbgStatus = EX_LED_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    if(n32Argc < 3)
    {
        goto err;
    }

    while(un8Idx <= n32Argc)
    {
        un32Enable |= (0x1UL << (uint8_t)atoi(pn8Argv[un8Idx++]));
    }

    eErr = HAL_LED_SetIcomOutput(eId, un32Enable);  /** Dimming init */
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    eErr = HAL_LED_SetIcomEnable(eId, un32Enable);  /** Dimming init */
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_LED_SetIseg(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus;
    LED_ID_e eId = LED_ID_0;
    uint8_t un8Idx = 2;
    uint32_t un32Enable = 0;

    eDbgStatus = EX_LED_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    if(n32Argc < 3)
    {
        goto err;
    }

    while(un8Idx <= n32Argc)
    {
        un32Enable |= (0x1UL << (uint8_t)atoi(pn8Argv[un8Idx++]));
    }

    eErr = HAL_LED_SetIsegOutput(eId, un32Enable);  /** Dimming init */
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    goto err;
}

static enum debug_cmd_status EX_LED_SetDimming(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus;
    LED_ID_e eId = LED_ID_0;
    uint8_t un8Idx = 1, un8InCnt = 0;

    eDbgStatus = EX_LED_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    if(n32Argc < 3 || (n32Argc % 2) != 0)
    {
        goto err;
    }

    memset(tData, 0x00, sizeof(LED_DATA_t)*CONFIG_LED_MAX_ICOM_CH_NUM);
    un8InCnt = (n32Argc - 2) / 2;

    while(un8Idx <= un8InCnt)
    {
        tData[un8Idx - 1].un8Num = (uint8_t)atoi(pn8Argv[un8Idx*2]);
        tData[un8Idx - 1].un32Data = (uint32_t)atoi(pn8Argv[(un8Idx*2)+1]);
        un8Idx++;
    }

    eErr = HAL_LED_SetDimming(eId, tData, un8InCnt);  /** Dimming init */
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_LED_SetData(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus;
    LED_ID_e eId = LED_ID_0;
    uint8_t un8Idx = 1, un8InCnt = 0;
    uint32_t un32Data = 0;

    eDbgStatus = EX_LED_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    if(n32Argc < 3 || (n32Argc % 2) != 0)
    {
        goto err;
    }

    memset(tData, 0x00, sizeof(LED_DATA_t)*CONFIG_LED_MAX_ICOM_CH_NUM);
    un8InCnt = (n32Argc - 2) / 2;

    while(un8Idx <= un8InCnt)
    {
        tData[un8Idx - 1].un8Num = (uint8_t)atoi(pn8Argv[un8Idx*2]);
        sscanf(pn8Argv[(un8Idx*2)+1], "%X", &un32Data);
        tData[un8Idx - 1].un32Data = un32Data;
        un8Idx++;
    }

    s_un8IcomCnt = un8InCnt;

    if(s_bPortOps == false)
    {
        eErr = HAL_LED_SetDispData(eId, tData, un8InCnt);  /** Dimming init */
        if(eErr != HAL_ERR_OK)
        {
            goto err;
        }
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_LED_SetLog(int32_t n32Argc, char *pn8Argv[])
{
    if((strncmp(pn8Argv[1],"on",2) == 0))
    {
        s_bISRLog = true;
    }
    else
    {
        s_bISRLog = false;
    }

    LOG("%s ISR Log %s.\n", EX_LED_LOG_STR, (s_bISRLog == true ? "on":"off"));

    return DEBUG_CMD_SUCCESS;
}

static const struct debug_cmd s_tEX_LED_CMD[] =
{
    {"LED", "h", EX_LED_Help, "help"},
    {"LED", "init", EX_LED_Init, ""},
    {"LED", "uninit", EX_LED_Uninit, ""},
    {"LED", "config", EX_LED_SetConfig, ""},
    {"LED", "clk", EX_LED_SetClkConfig, ""},
    {"LED", "icom", EX_LED_SetIcom, ""},
    {"LED", "iseg", EX_LED_SetIseg, ""},
    {"LED", "dim", EX_LED_SetDimming, ""},
    {"LED", "data", EX_LED_SetData, ""},
    {"LED", "start", EX_LED_Start, ""},
    {"LED", "stop", EX_LED_Stop, ""},
    {"LED", "log", EX_LED_SetLog, ""}
};

/**********************************************************************
 * @brief		EX_LED
 * @param[in]	None
 * @return	None
 **********************************************************************/
void EX_LED(void)
{
#if defined(EXTRN_EX_LED_DEBUG_OTHER)
    LOG("%s\n", EX_LED_UART1_LOG);
#endif

    SystemDelayMS(3);

    /* Add TC commands */
    debug_cmd_init(s_tEX_LED_CMD, DEBUG_CMD_LIST_COUNT(s_tEX_LED_CMD));
}

#endif /* LED */
/* --------------------------------- End Of File ------------------------------ */
