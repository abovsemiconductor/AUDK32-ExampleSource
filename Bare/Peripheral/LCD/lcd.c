/**
 *******************************************************************************
 * @file        lcd.c
 * @author      ABOV R&D Division
 * @brief       LCD Example Code
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

#if defined(LCD_TC)
#include "abov_config.h"
#include "ex_configs.h"
#include "ex_common.h"
#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "hal_scu.h"
#include "hal_scu_clk.h"
#include "hal_pcu.h"
#include "hal_lcd.h"

#if !defined(_LCD)
#error "This chipset did not support this example."
#endif

#if defined (EX_COMMON_ENABLE_CUSTOM_SSCANF)
#define sscanf(str, fmt, out) EX_COMMON_ParseByFormat((str), (fmt)[1], (uint32_t *)(out))
#endif

#define EX_LCD_STR "LCD"
#define EX_LCD_LOG_STR "LCD :"
#define EX_LCD_ERR_STR "[E]LCD :"
#define EX_LCD_MAX_NUM  (CONFIG_LCD_MAX_COUNT - 1)

extern const char *pcCommStr[];
extern const char *pcCmdStr[];
extern const char *pcValStr[];
extern const char *pcOptStr[];

static LCD_DATA_t tData[CONFIG_LCD_MAX_SEG_CH_NUM];

static enum debug_cmd_status EX_LCD_Help(int32_t n32Argc, char *pn8Argv[])
{
    uint32_t un32ShowOpt = 0;
    EX_COMM_STR_OPT_e eOpt[2];

    EX_COMMON_SetShowModuleInfo(EX_LCD_STR, CONFIG_LCD_MAX_COUNT);

    un32ShowOpt = EX_COMMON_GetShowOpt(pn8Argv[1]);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_INIT, NULL, EX_LCD_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_UNINIT, NULL, EX_LCD_MAX_NUM, eOpt, 0, NULL);

    eOpt[0] = EX_COMM_STR_OPT_SRC; 
    eOpt[1] = EX_COMM_STR_OPT_DIV;
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CLK, NULL, EX_LCD_MAX_NUM, eOpt, 2, NULL);
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_SRC, EX_COMM_STR_VAL_MAX, "s(lse)/w(wdtrc)/m(mccr)");
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MCCR, EX_COMM_STR_VAL_MAX, "[mccr] [div]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MCCR, EX_COMM_STR_VAL_CLKSRC, "/m(mclk)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_DIV, EX_COMM_STR_VAL_N_255, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_DIV, EX_COMM_STR_VAL_MAX, "256,128,64,32");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CONFIG, NULL, EX_LCD_MAX_NUM, eOpt, 0, "[bias] [duty] [-auto] [-cont]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "bias: i(int)/e(ext)");
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "int: [rlcd]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "rlcd: 1(10/10/10)/2(66/66/50)/3(105/105/80)/4(320/320/240)");
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "ext: [vlc]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "vlc: 0:1:2:3 (ex. enable vlc0 & vlc3 : 0:3");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "duty: 2(1/3duty and 1/2bias),3,4,5,6,8");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-auto: [clk]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_CLK, EX_COMM_STR_VAL_MAX, "1,2,3,4,5,6,7,8");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-cont: [step]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "step: 16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "disp", EX_LCD_MAX_NUM, eOpt, 0, "on/off");
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "data", EX_LCD_MAX_NUM, eOpt, 0, "[seg] [com] ...");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "seg: 0~N");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "com: 0x0~0xN(hexa and bit-order)");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "write", EX_LCD_MAX_NUM, eOpt, 0, "[segnum] [da]");
    if(un32ShowOpt == 1)
    {
        LOG("\t segnum: 0~%d\n", CONFIG_LCD_MAX_SEG_CH_NUM);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_DA, EX_COMM_STR_VAL_N_NUM, NULL);
    }
    LOG("\n");

    return DEBUG_CMD_SUCCESS;
}


static enum debug_cmd_status EX_LCD_GetId(int32_t n32Argc, char *pn8Argv[], LCD_ID_e *peId)
{
    uint8_t un8Data = 0;
    if(n32Argc == 1)
    {
        EX_COMMON_SetShowModuleLog(EX_LCD_ERR_STR, (char *)pcOptStr[EX_COMM_STR_OPT_ARG], EX_COMM_STR_OPT_MAX);
        return DEBUG_CMD_INVALID;
    }

    un8Data = atoi(pn8Argv[1]); 
    if(un8Data >= CONFIG_LCD_MAX_COUNT)
    {
        LOG("%s Max Chan %d\n", EX_LCD_ERR_STR, CONFIG_LCD_MAX_COUNT);
        return DEBUG_CMD_INVALID;
    }

    *peId = (LCD_ID_e)un8Data;

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_LCD_GetClkConfig(int32_t n32Argc, char *pn8Argv[], LCD_CLK_CFG_t *ptClkCfg)
{
    uint8_t un8Arg = 2;
    uint16_t un16Data = 0;

    if (pn8Argv[un8Arg][0] == 's')
        ptClkCfg->eClk = LCD_CLK_LSE;
    else if (pn8Argv[un8Arg][0] == 'w')
        ptClkCfg->eClk = LCD_CLK_WDTRC;
    else if (pn8Argv[un8Arg][0] == 'm')
        ptClkCfg->eClk = LCD_CLK_MCCR;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_LCD_ERR_STR, NULL, EX_COMM_STR_OPT_SRC);
        goto err;
    }
   
    un8Arg++;
    if(ptClkCfg->eClk == LCD_CLK_MCCR)
    {
        if (pn8Argv[un8Arg][0] == 'l')
            ptClkCfg->eMccr = LCD_CLK_MCCR_LSI;
        else if (pn8Argv[un8Arg][0] == 's')
            ptClkCfg->eMccr = LCD_CLK_MCCR_LSE;
        else if (pn8Argv[un8Arg][0] == 'm')
            ptClkCfg->eMccr = LCD_CLK_MCCR_MCLK;
        else if (pn8Argv[un8Arg][0] == 'h')
            ptClkCfg->eMccr = LCD_CLK_MCCR_HSI;
        else if (pn8Argv[un8Arg][0] == 'e')
            ptClkCfg->eMccr = LCD_CLK_MCCR_HSE;
        else if (pn8Argv[un8Arg][0] == 'p')
            ptClkCfg->eMccr = LCD_CLK_MCCR_PLL;
        else
        {
            EX_COMMON_SetShowModuleLog(EX_LCD_ERR_STR, NULL, EX_COMM_STR_OPT_MCCR);
            goto err;
        }
        un8Arg++;
        ptClkCfg->un8MccrDiv = (uint8_t)atoi(pn8Argv[un8Arg++]);
    }

    un16Data = (uint16_t)atoi(pn8Argv[un8Arg++]);
    if (un16Data == 256)
        ptClkCfg->ePreDiv = LCD_CLK_PREDIV_256;
    else if (un16Data == 128)
        ptClkCfg->ePreDiv = LCD_CLK_PREDIV_128;
    else if (un16Data == 64)
        ptClkCfg->ePreDiv = LCD_CLK_PREDIV_64;
    else if (un16Data == 32)
        ptClkCfg->ePreDiv = LCD_CLK_PREDIV_32;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_LCD_ERR_STR, NULL, EX_COMM_STR_OPT_DIV);
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_LCD_GetConfig(int32_t n32Argc, char *pn8Argv[], LCD_CFG_t *ptCfg)
{
    uint8_t un8Arg = 2, i = 0, un8Data = 0;

    switch (pn8Argv[un8Arg++][0])
    {
        case 'i':
            ptCfg->eDriveBias = LCD_BIAS_INTERNAL;
            break;
        case 'e':
            ptCfg->eDriveBias = LCD_BIAS_EXTERNAL;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_LCD_ERR_STR, "[bias]", EX_COMM_STR_OPT_MAX);
            goto err;
    }

    if(ptCfg->eDriveBias == LCD_BIAS_EXTERNAL)
    {
        while(i < 7)
        {
            switch(pn8Argv[un8Arg][i++])
            {
                case '0':
                    ptCfg->uType.tExt.bVLC0Enable = true;
                    break;
                case '1':
                    ptCfg->uType.tExt.bVLC1Enable = true;
                    break;
                case '2':
                    ptCfg->uType.tExt.bVLC2Enable = true;
                    break;
                case '3':
                    ptCfg->uType.tExt.bVLC3Enable = true;
                    break;
                default:
                    EX_COMMON_SetShowModuleLog(EX_LCD_ERR_STR, "[vlc]", EX_COMM_STR_OPT_MAX);
                    goto err;
            }

            if(pn8Argv[un8Arg][i++] != ':')
            { 
                break;
            }
        }

        un8Arg++;
    }
    else
    {
        if (pn8Argv[un8Arg][0] == '1')
            ptCfg->uType.tInt.eIntBiasResistor = LCD_RLCD1_10_10_10;
        else if (pn8Argv[un8Arg][0] == '2')
            ptCfg->uType.tInt.eIntBiasResistor = LCD_RLCD2_66_66_50;
        else if (pn8Argv[un8Arg][0] == '3')
            ptCfg->uType.tInt.eIntBiasResistor = LCD_RLCD3_105_105_80;
        else if (pn8Argv[un8Arg][0] == '4')
            ptCfg->uType.tInt.eIntBiasResistor = LCD_RLCD4_320_320_240;
        else
        {
            EX_COMMON_SetShowModuleLog(EX_LCD_ERR_STR, "[rlcd]", EX_COMM_STR_OPT_MAX);
            goto err;
        }
    }

    un8Arg++;
    if (pn8Argv[un8Arg][0] == '2')
        ptCfg->eDutyBias = LCD_DUTY_1_3_BIAS_1_2;
    else if (pn8Argv[un8Arg][0] == '3')
        ptCfg->eDutyBias = LCD_DUTY_1_3_BIAS_1_3;
    else if (pn8Argv[un8Arg][0] == '4')
        ptCfg->eDutyBias = LCD_DUTY_1_4_BIAS_1_3;
    else if (pn8Argv[un8Arg][0] == '5')
        ptCfg->eDutyBias = LCD_DUTY_1_5_BIAS_1_3;
    else if (pn8Argv[un8Arg][0] == '6')
        ptCfg->eDutyBias = LCD_DUTY_1_6_BIAS_1_4;
    else if (pn8Argv[un8Arg][0] == '8')
        ptCfg->eDutyBias = LCD_DUTY_1_8_BIAS_1_4;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_LCD_ERR_STR, "[duty]", EX_COMM_STR_OPT_MAX);
        goto err;
    }

    un8Arg++;
    if(strncmp(pn8Argv[un8Arg], "-auto", 5) == 0)
    {
        un8Arg++;
        ptCfg->tAutoBias.bEnable = true;
        if (pn8Argv[un8Arg][0] == '1')
            ptCfg->tAutoBias.eBiasModeA = LCD_BIAS_MODE_A_1CLK;
        else if (pn8Argv[un8Arg][0] == '2')
            ptCfg->tAutoBias.eBiasModeA = LCD_BIAS_MODE_A_2CLK;
        else if (pn8Argv[un8Arg][0] == '3')
            ptCfg->tAutoBias.eBiasModeA = LCD_BIAS_MODE_A_3CLK;
        else if (pn8Argv[un8Arg][0] == '4')
            ptCfg->tAutoBias.eBiasModeA = LCD_BIAS_MODE_A_4CLK;
        else if (pn8Argv[un8Arg][0] == '5')
            ptCfg->tAutoBias.eBiasModeA = LCD_BIAS_MODE_A_5CLK;
        else if (pn8Argv[un8Arg][0] == '6')
            ptCfg->tAutoBias.eBiasModeA = LCD_BIAS_MODE_A_6CLK;
        else if (pn8Argv[un8Arg][0] == '7')
            ptCfg->tAutoBias.eBiasModeA = LCD_BIAS_MODE_A_7CLK;
        else if (pn8Argv[un8Arg][0] == '8')
            ptCfg->tAutoBias.eBiasModeA = LCD_BIAS_MODE_A_8CLK;
        else
        {
            EX_COMMON_SetShowModuleLog(EX_LCD_ERR_STR, "-auto", EX_COMM_STR_OPT_CLK);
            goto err;
        }

        un8Arg++;
    }
    else
    {
        un8Arg++;
        ptCfg->tAutoBias.bEnable = false;
    }

    if(strncmp(pn8Argv[un8Arg], "-cont", 5) == 0)
    {
        un8Arg++;
        ptCfg->tContrast.bEnable = true;
        un8Data = (uint8_t)atoi(pn8Argv[un8Arg++]);
        if (un8Data == 16) 
            ptCfg->tContrast.eVlc0Volt = LCD_VLC0_VDD16_16_STEP;
        else if (un8Data == 17) 
            ptCfg->tContrast.eVlc0Volt = LCD_VLC0_VDD16_17_STEP;
        else if (un8Data == 18) 
            ptCfg->tContrast.eVlc0Volt = LCD_VLC0_VDD16_18_STEP;
        else if (un8Data == 19) 
            ptCfg->tContrast.eVlc0Volt = LCD_VLC0_VDD16_19_STEP;
        else if (un8Data == 20) 
            ptCfg->tContrast.eVlc0Volt = LCD_VLC0_VDD16_20_STEP;
        else if (un8Data == 21) 
            ptCfg->tContrast.eVlc0Volt = LCD_VLC0_VDD16_21_STEP;
        else if (un8Data == 22) 
            ptCfg->tContrast.eVlc0Volt = LCD_VLC0_VDD16_22_STEP;
        else if (un8Data == 23) 
            ptCfg->tContrast.eVlc0Volt = LCD_VLC0_VDD16_23_STEP;
        else if (un8Data == 24) 
            ptCfg->tContrast.eVlc0Volt = LCD_VLC0_VDD16_24_STEP;
        else if (un8Data == 25) 
            ptCfg->tContrast.eVlc0Volt = LCD_VLC0_VDD16_25_STEP;
        else if (un8Data == 26) 
            ptCfg->tContrast.eVlc0Volt = LCD_VLC0_VDD16_26_STEP;
        else if (un8Data == 27) 
            ptCfg->tContrast.eVlc0Volt = LCD_VLC0_VDD16_27_STEP;
        else if (un8Data == 28) 
            ptCfg->tContrast.eVlc0Volt = LCD_VLC0_VDD16_28_STEP;
        else if (un8Data == 29) 
            ptCfg->tContrast.eVlc0Volt = LCD_VLC0_VDD16_29_STEP;
        else if (un8Data == 30) 
            ptCfg->tContrast.eVlc0Volt = LCD_VLC0_VDD16_30_STEP;
        else if (un8Data == 31) 
            ptCfg->tContrast.eVlc0Volt = LCD_VLC0_VDD16_31_STEP;
        else
        {
            EX_COMMON_SetShowModuleLog(EX_LCD_ERR_STR, "-cont[step]", EX_COMM_STR_OPT_MAX);
            goto err;
        }
        un8Arg++;
    }
    else
    {
        un8Arg++;
        ptCfg->tContrast.bEnable = false;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_LCD_Init(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    LCD_ID_e eId = LCD_ID_0;

    eDbgStatus = EX_LCD_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_LCD_Init(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_LCD_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_INIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_LCD_Uninit(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    LCD_ID_e eId = LCD_ID_0;

    eDbgStatus = EX_LCD_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_LCD_Uninit(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_LCD_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_UNINIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_LCD_SetClkConfig(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus;
    LCD_ID_e eId = LCD_ID_0;
    LCD_CLK_CFG_t tClkCfg;

    eDbgStatus = EX_LCD_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eDbgStatus = EX_LCD_GetClkConfig(n32Argc, pn8Argv, &tClkCfg);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_LCD_SetClkConfig(eId, &tClkCfg);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_LCD_ERR_STR, "ClkCfg", EX_COMM_STR_OPT_MAX);
        }
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_LCD_SetConfig(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus;
    LCD_ID_e eId = LCD_ID_0;
    LCD_CFG_t tCfg;

    eDbgStatus = EX_LCD_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eDbgStatus = EX_LCD_GetConfig(n32Argc, pn8Argv, &tCfg);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_LCD_SetConfig(eId, &tCfg);
    if(eErr != HAL_ERR_OK)
    {
        if(eErr == HAL_ERR_NOT_SUPPORTED)
        {
            EX_COMMON_SetShowModuleLog(EX_LCD_ERR_STR, "Cfg", EX_COMM_STR_OPT_MAX);
        }

        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_LCD_SetDisplay(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    LCD_ID_e eId = LCD_ID_0;

    eDbgStatus = EX_LCD_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    if((strncmp(pn8Argv[2],"on",2) == 0))
    {
        eErr = HAL_LCD_Start(eId);
        if(eErr != HAL_ERR_OK)
        {
            goto err;
        }
    }

    if((strncmp(pn8Argv[2],"off",3) == 0))
    {
        eErr = HAL_LCD_Stop(eId);
        if(eErr != HAL_ERR_OK)
        {
            goto err;
        }
    }
    
    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_LCD_SetData(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr;
    enum debug_cmd_status eDbgStatus;
    LCD_ID_e eId = LCD_ID_0;
    uint8_t un8Idx = 1, un8InCnt = 0;
    uint32_t un32Data = 0;

    eDbgStatus = EX_LCD_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    if(n32Argc < 3 || (n32Argc % 2) != 0)
    {
        goto err;
    }

    memset(tData, 0x00, sizeof(LCD_DATA_t)*CONFIG_LCD_MAX_SEG_CH_NUM);
    un8InCnt = (n32Argc - 2) / 2;

    while(un8Idx <= un8InCnt)
    {
        tData[un8Idx - 1].un8SegNum = (uint8_t)atoi(pn8Argv[un8Idx*2]);
        sscanf(pn8Argv[(un8Idx*2)+1], "%X", &un32Data);
        tData[un8Idx - 1].un8Data = (uint8_t)un32Data;
        un8Idx++;
    }

    eErr = HAL_LCD_SetData(eId, tData, un8InCnt);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static const struct debug_cmd s_tEX_LCD_CMD[] =
{
    {"LCD", "h", EX_LCD_Help,"help"},
    {"LCD", "init", EX_LCD_Init, ""},
    {"LCD", "uninit", EX_LCD_Uninit, ""},
    {"LCD", "clk", EX_LCD_SetClkConfig, ""},
    {"LCD", "config", EX_LCD_SetConfig, ""},
    {"LCD", "disp", EX_LCD_SetDisplay, ""},
    {"LCD", "data", EX_LCD_SetData, ""},
};

void EX_LCD(void)
{
    /* Add TC commands */
    debug_cmd_init(s_tEX_LCD_CMD, DEBUG_CMD_LIST_COUNT(s_tEX_LCD_CMD));
}

#endif /* LCD */
/* --------------------------------- End Of File ------------------------------ */
