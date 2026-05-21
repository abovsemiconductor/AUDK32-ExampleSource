/**
 *******************************************************************************
 * @file        code_flash.c
 * @author      ABOV R&D Division
 * @brief       CFMC Example Code
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

#if defined(FLASH_TC)
#include "abov_config.h"

#if defined(_CFMC) && !defined (_DFMC)
#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "hal_scu.h"
#include "hal_scu_clk.h"
#include "hal_pcu.h"

#include "hal_cfmc.h"

#if !defined(_CFMC)
#error "This chipset did not support this example."
#endif

#define EX_CFMC_1ST_ARG             1
#define EX_CFMC_2ND_ARG             2
#define EX_CFMC_3RD_ARG             3

#define EX_CFMC_MAX_LATENCY_VAL     3
#define EX_CFMC_DATA_BUFFER_SIZE    32

static uint8_t s_aun8Buffer [EX_CFMC_DATA_BUFFER_SIZE];
static CFMC_GEOMETRY_t s_tCfmcGeometry;

static enum debug_cmd_status EX_CFMC_Help(int32_t n32Argc, char *pn8Argv[])
{
    LOG("\n");
    LOG("Usage :\n");
    LOG("\t01.info      - print flash geometric info\n");
    LOG("\t02.latency   - latency [0 ~ 3] \n");
    LOG("\t03.cache     - cache [flag] \n");
    LOG("\t               flag    : 1(ON), 0(OFF)\n");
    LOG("\t04.wprot     - wprot [address] [size] [flag]\n");
    LOG("\t               address : should be aligned to write protection segment size\n");
    LOG("\t                         if 0xFFFFFFFF, whole area is selected\n");
    LOG("\t               size    : should be aligned to write protection segment size\n");
    LOG("\t               flag    : 1(ON), 0(OFF) \n");
    LOG("\t05.bbwprot   - bbwprot [flag]\n");
    LOG("\t               flag    : 1(ON), 0(OFF) \n");
    LOG("\t06.rprot     - rprot [level]\n");
    LOG("\t               level   : 0(level-0), 1(level-1), 2(level-2), 3(level-1+passwd), 4(level-2+passwd)\n");
    LOG("\t07.erase     - erase [sector address] [mode]\n");
    LOG("\t               sector address : should be aligned to mode\n");
    LOG("\t               mode           : 0(Chip Erase), 1(Page), 2(1KB), 4(2KB) or 8(4KB)\n");
    LOG("\t08.write     - write [address] [size]\n");
    LOG("\t               address : should be aligned to word\n");
    LOG("\t               size    : any word aligned size and less than max. 32B \n");
    LOG("\t                         if 0xFFFFFFFF, one write block is written by test buffer\n");
    LOG("\t09.selferase - selferase [address]\n");
    LOG("\t               address : Page aligend\n");
    LOG("\t10.selfwrite - selfwrite [address] [size]\n");
    LOG("\t               address : should be aligned to word\n");
    LOG("\t               size    : any word aligned size and less than max. 32B \n");
    LOG("\t                         if 0xFFFFFFFF, one write block is written by test buffer\n");

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_CFMC_PrintFlahsInfo(int32_t n32Argc, char *pn8Argv[])
{
    s_tCfmcGeometry = HAL_CFMC_GetGeometry();
    LOG("\tCode Flash Size    : 0x%x Bytes\n", s_tCfmcGeometry.un32CflashSize);
    LOG("\tSystem Flash Size  : 0x%x Bytes\n", s_tCfmcGeometry.un32SystemFlashSize);
    LOG("\tWrite Protect Size : 0x%x Bytes\n", s_tCfmcGeometry.un32CflashWProtectedSegSize);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_CFMC_SetLatency(int32_t n32Argc, char *pn8Argv[])
{
    CFMC_CONFIG_t tCfmcConfig;
    uint32_t un32Latency;

    if (n32Argc != 2)
    {
        EX_CFMC_Help(0,NULL);
        return DEBUG_CMD_INVALID;
    }

    un32Latency = atoi(pn8Argv[EX_CFMC_1ST_ARG]);
    if (un32Latency > EX_CFMC_MAX_LATENCY_VAL)
    {
        EX_CFMC_Help(0,NULL);
        return DEBUG_CMD_INVALID;
    }

    tCfmcConfig.bReqCrcEnable = false;
    tCfmcConfig.bReqCrcInit = false;
    tCfmcConfig.un8Latency = un32Latency;
    HAL_CFMC_SetConfig(&tCfmcConfig);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_CFMC_SetCache(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eRet;
    bool bRequestEnable;

    if (n32Argc != 2)
    {
        EX_CFMC_Help(0,NULL);
        return DEBUG_CMD_INVALID;
    }

    bRequestEnable = atoi(pn8Argv[EX_CFMC_1ST_ARG]);

    eRet = HAL_CFMC_SetCache(bRequestEnable);
    if (eRet)
    {
        return DEBUG_CMD_FAILED;
    }

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_CFMC_SetWriteProtect(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eRet;
    uint32_t un32FlashAddr;
    uint32_t un32Len;
    bool bRequestLock;

    if (n32Argc != 4)
    {
        EX_CFMC_Help(0,NULL);
        return DEBUG_CMD_INVALID;
    }

    un32FlashAddr = debug_cmd_string2int(pn8Argv[EX_CFMC_1ST_ARG]);
    un32Len = debug_cmd_string2int(pn8Argv[EX_CFMC_2ND_ARG]);
    bRequestLock = atoi(pn8Argv[EX_CFMC_3RD_ARG]);

    eRet = HAL_CFMC_SetWriteProtect(un32FlashAddr, un32Len, bRequestLock);
    if (eRet)
    {
        return DEBUG_CMD_FAILED;
    }

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_CFMC_SetWriteBootProtect(int32_t n32Argc, char *pn8Argv[])
{
    bool bRequestLock;

    if (n32Argc != 2)
    {
        EX_CFMC_Help(0,NULL);
        return DEBUG_CMD_INVALID;
    }
    bRequestLock = atoi(pn8Argv[EX_CFMC_1ST_ARG]);

    HAL_CFMC_SetWriteProtectBootBlk(bRequestLock);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_CFMC_SetReadProtect(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eRet = HAL_ERR_OK;
    CFMC_RD_PROTECT_CONFIG_t tRdProtectConfig;

    if (n32Argc != 2)
    {
        EX_CFMC_Help(0,NULL);
        return DEBUG_CMD_INVALID;
    }
    tRdProtectConfig.eRdProtectLevel = (CFMC_RD_PROTECT_LEVEL_e)atoi(pn8Argv[EX_CFMC_1ST_ARG]);

    eRet = HAL_CFMC_SetReadProtect(tRdProtectConfig);
    if (eRet)
    {
        return DEBUG_CMD_FAILED;
    }

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_CFMC_Erase(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eRet;
    uint32_t un32SectorAddr;
    CFMC_ERASE_MODE_e eEraseMode;

    if (n32Argc != 3)
    {
        EX_CFMC_Help(0,NULL);
        return DEBUG_CMD_INVALID;
    }

    un32SectorAddr = debug_cmd_string2int(pn8Argv[EX_CFMC_1ST_ARG]);
    eEraseMode = (CFMC_ERASE_MODE_e)atoi(pn8Argv[EX_CFMC_2ND_ARG]);

    eRet = HAL_CFMC_Erase(un32SectorAddr, eEraseMode);
    if (eRet)
    {
        return DEBUG_CMD_FAILED;
    }

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_CFMC_Write(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eRet;
    uint32_t un32StartAddr;
    uint32_t un32TargetAddr;
    uint32_t un32Len;
    uint32_t un32Cnt;

    if (n32Argc != 3)
    {
        EX_CFMC_Help(0,NULL);
        return DEBUG_CMD_INVALID;
    }

    un32StartAddr = debug_cmd_string2int(pn8Argv[EX_CFMC_1ST_ARG]);
    un32Len = debug_cmd_string2int(pn8Argv[EX_CFMC_2ND_ARG]);
    if ((un32Len & 0x3) || (un32Len > EX_CFMC_DATA_BUFFER_SIZE))
    {
        if (un32Len != 0xFFFFFFFF)
        {
            EX_CFMC_Help(0,NULL);
            return DEBUG_CMD_INVALID;
        }
    }

    if (un32Len == 0xFFFFFFFF)
    {
        for (un32Cnt=0; un32Cnt < s_tCfmcGeometry.un32CflashWProtectedSegSize/EX_CFMC_DATA_BUFFER_SIZE; un32Cnt++)
        {
            un32TargetAddr = un32StartAddr + (EX_CFMC_DATA_BUFFER_SIZE *un32Cnt);
            eRet = HAL_CFMC_Write(un32TargetAddr, s_aun8Buffer, EX_CFMC_DATA_BUFFER_SIZE);
            if (eRet)
            {
                break;
            }
        }
    }
    else
    {
        for (un32Cnt=0; un32Cnt < un32Len/4; un32Cnt++)
        {
            un32TargetAddr = un32StartAddr + (4 *un32Cnt);
            eRet = HAL_CFMC_Write(un32TargetAddr, &s_aun8Buffer[4 *un32Cnt], 4);
            if (eRet)
            {
                break;
            }
        }
    }

    if (eRet)
    {
        return DEBUG_CMD_FAILED;
    }

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_CFMC_SelfErase(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eRet;
    uint32_t un32SectorAddr;

    if (n32Argc != 2)
    {
        EX_CFMC_Help(0,NULL);
        return DEBUG_CMD_INVALID;
    }

    un32SectorAddr = debug_cmd_string2int(pn8Argv[EX_CFMC_1ST_ARG]);
    eRet = HAL_CFMC_SelfErase(un32SectorAddr);
    if (eRet)
    {
        return DEBUG_CMD_FAILED;
    }

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_CFMC_SelfWrite(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eRet;
    uint32_t un32StartAddr;
    uint32_t un32TargetAddr;
    uint32_t un32Len;
    uint32_t un32Cnt;

    if (n32Argc != 3)
    {
        EX_CFMC_Help(0,NULL);
        return DEBUG_CMD_INVALID;
    }

    un32StartAddr = debug_cmd_string2int(pn8Argv[EX_CFMC_1ST_ARG]);
    un32Len = debug_cmd_string2int(pn8Argv[EX_CFMC_2ND_ARG]);

    if ((un32Len & 0x3) || (un32Len > EX_CFMC_DATA_BUFFER_SIZE))
    {
        if (un32Len != 0xFFFFFFFF)
        {
            EX_CFMC_Help(0,NULL);
            return DEBUG_CMD_INVALID;
        }
    }

    if (un32Len == 0xFFFFFFFF)
    {
        for (un32Cnt=0; un32Cnt < s_tCfmcGeometry.un32CflashWProtectedSegSize/EX_CFMC_DATA_BUFFER_SIZE; un32Cnt++)
        {
            un32TargetAddr = un32StartAddr + (EX_CFMC_DATA_BUFFER_SIZE *un32Cnt);
            eRet = HAL_CFMC_SelfWrite(un32TargetAddr, s_aun8Buffer, EX_CFMC_DATA_BUFFER_SIZE);
            if (eRet)
            {
                break;
            }
        }
    }
    else
    {
        for (un32Cnt=0; un32Cnt < un32Len/4; un32Cnt++)
        {
            un32TargetAddr = un32StartAddr + (4 *un32Cnt);
            eRet = HAL_CFMC_SelfWrite(un32TargetAddr, &s_aun8Buffer[4 *un32Cnt], 4);
            if (eRet)
            {
                break;
            }
        }
    }

    if (eRet)
    {
        return DEBUG_CMD_FAILED;
    }

    return DEBUG_CMD_SUCCESS;
}

static const struct debug_cmd s_tEX_CFMC_CMD[] =
{
    {"CFMC", "h",EX_CFMC_Help,"Usage"},
    {"CFMC", "info",EX_CFMC_PrintFlahsInfo, "Print flash geometric info"},
    {"CFMC", "latency",EX_CFMC_SetLatency,"Set latency"},
    {"CFMC", "cache",EX_CFMC_SetCache,"Set I cache"},
    {"CFMC", "wprot",EX_CFMC_SetWriteProtect,"Set write protection"},
    {"CFMC", "bbwprot",EX_CFMC_SetWriteBootProtect,"Set boot-block(first 4KB) write protection"},
    {"CFMC", "rprot",EX_CFMC_SetReadProtect,"Set read protection in run-time"},
    {"CFMC", "erase",EX_CFMC_Erase,"Erase flash(Page, 1KB, 2KB or 4KB mode)"},
    {"CFMC", "write",EX_CFMC_Write,"Write flash with word aligned data size (max. 32B)"},
    {"CFMC", "selferase",EX_CFMC_SelfErase,"Self Erase flash with page aligned address"},
    {"CFMC", "selfwrite",EX_CFMC_SelfWrite,"Self Write flash with word aligned data size (max. 32B)"},
};

void EX_FLASH(void)
{
    uint32_t un32Cnt;

    /* Initialization */
    HAL_CFMC_Init();

    s_tCfmcGeometry = HAL_CFMC_GetGeometry();
    LOG("\tCode Flash Size    : 0x%x Bytes\n", s_tCfmcGeometry.un32CflashSize);
    LOG("\tSystem Flash Size  : 0x%x Bytes\n", s_tCfmcGeometry.un32SystemFlashSize);
    LOG("\tWrite Protect Size : 0x%x Bytes\n", s_tCfmcGeometry.un32CflashWProtectedSegSize);

    /* Update test buffer */
    for (un32Cnt = 0; un32Cnt < EX_CFMC_DATA_BUFFER_SIZE; un32Cnt++)
    {
        s_aun8Buffer[un32Cnt] = un32Cnt;
    }

    /* Add TC commands */
    debug_cmd_init(s_tEX_CFMC_CMD,DEBUG_CMD_LIST_COUNT(s_tEX_CFMC_CMD));
}

#endif /* !_DFMC */
#endif
/* --------------------------------- End Of File ------------------------------ */
