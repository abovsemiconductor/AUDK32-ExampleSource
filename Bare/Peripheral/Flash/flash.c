/**
 *******************************************************************************
 * @file        flash.c
 * @author      ABOV R&D Division
 * @brief       FMC Example Code
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

#if defined(_CFMC) && defined(_DFMC)
#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "hal_scu.h"
#include "hal_scu_clk.h"
#include "hal_pcu.h"
#include "hal_fmc.h"

#if !defined(_CFMC) && !defined(_DFMC)
#error "This chipset did not support this example."
#endif

#define EX_FMC_1ST_ARG             1
#define EX_FMC_2ND_ARG             2
#define EX_FMC_3RD_ARG             3
#define EX_FMC_4TH_ARG             4

#define EX_FMC_MAX_LATENCY_VAL     7
#define EX_FMC_DATA_BUFFER_SIZE    32

#define EX_FMC_FIRST_WPROT_BLOCK    0

static uint8_t s_aun8Buffer [EX_FMC_DATA_BUFFER_SIZE];
static FMC_GEOMETRY_t s_tFmcGeometry;

static enum debug_cmd_status EX_FMC_Help(int32_t n32Argc, char *pn8Argv[])
{
    LOG("\n");
    LOG("Usage :\n");
    LOG("\t01.info       - print flash geometric info\n");
    LOG("\t02.latency    - latency [fmcid] [0 ~ 7] \n");
    LOG("\t                fmcid   : 0(CFMC), 1(DFMC)\n");
    LOG("\t03.cache      - cache [cacheid] [flag] \n");
    LOG("\t                cacheid : 0(I Cache), 1(D Cache), 2(I and D Cache)\n");
    LOG("\t                flag    : 1(ON), 0(OFF)\n");
    LOG("\t04.multibank  - multibank [flag] \n");
    LOG("\t                flag    : 0(Single-Bank), 1(Multi-Bank)\n");
    LOG("\t05.activebank - activebank [bankid] \n");
    LOG("\t                bankid  : 1(Bank A), 2(Bank B)\n");
    LOG("\t06.bbank      - bbank [bankid] \n");
    LOG("\t                bankid  : 0 (Single Bank), 1(Bank A), 2(Bank B)\n");
    LOG("\t07.wprot      - wprot [fmcid] [offset] [size] [flag]\n");
    LOG("\t                fmcid   : 0(CFMC), 1(DFMC)\n");
    LOG("\t                offset  : should be aligned to write protection segment size\n");
    LOG("\t                          if 0xFFFFFFFF, whole area is selected\n");
    LOG("\t                size    : should be aligned to write protection segment size\n");
    LOG("\t                flag    : 1(ON), 0(OFF) \n");
    LOG("\t08.bbwprot    - bbwprot [flag]\n");
    LOG("\t                flag : 1(ON), 0(OFF) \n");
    LOG("\t09.rprot      - rprot [fmcid] [level] [password]\n");
    LOG("\t                fmcid   : 0(CFMC), 1(DFMC)\n");
    LOG("\t                level   : 0(level-0), 1(level-1), 2(level-2), 3(level-1+passwd), 4(level-2+passwd)\n");
    LOG("\t                password: password (hex string format)\n");
    LOG("\t10.erase      - erase [fmcid] [offset] [mode]\n");
    LOG("\t                fmcid   : 0(CFMC), 1(DFMC)\n");
    LOG("\t                offset  : should be aligned to mode\n");
    LOG("\t                mode    : 0(Chip Erase), 1(Page), 2(1KB), 4(2KB) or 8(4KB)\n");
    LOG("\t11.write      - write [fmcid] [offset] [size]\n");
    LOG("\t                fmcid   : 0(CFMC), 1(DFMC)\n");
    LOG("\t                offset  : should be aligned to word\n");
    LOG("\t                size    : any word aligned size and less than max. 32B \n");
    LOG("\t                          if 0xFFFFFFFF, one write block is written by test buffer\n");
    LOG("\t12.bwrite     - bwrite [offset] [data]\n");
    LOG("\t                offset  : offset in Data flash\n");
    LOG("\t                data    : one byte data to write\n");
    LOG("\t13.selferase  - selferase [fmcid] [offset] [mode]\n");
    LOG("\t                fmcid   : 0(CFMC), 1(DFMC)\n");
    LOG("\t                offset  : Page aligend\n");
    LOG("\t                mode    : 0 (Page erase), 1(sector erase)\n");
    LOG("\t14.selfwrite  - selfwrite [fmcid] [offset] [size]\n");
    LOG("\t                fmcid   : 0(CFMC), 1(DFMC)\n");
    LOG("\t                offset  : should be aligned to word\n");
    LOG("\t                size    : any word aligned size and less than max. 32B \n");
    LOG("\t                          if 0xFFFFFFFF, one write block is written by test buffer\n");

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_FMC_PrintFlahsInfo(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eRet;
    CFMC_BANK_ID_e eBootBank;
    bool bBankSwitchDone;
    CFMC_BANK_ID_e eActiveBank;
    int8_t i;

    LOG("\tCode Flash Size    : 0x%x Bytes\n", s_tFmcGeometry.tCflash.un32Size);
    LOG("\tData Flash Size    : 0x%x Bytes\n", s_tFmcGeometry.tDflash.un32Size);
    LOG("\tSystem Flash Size  : 0x%x Bytes\n\n", s_tFmcGeometry.tSysflash.un32Size);

    LOG("\tWrite Protection Size (Code Flash)\n");
    for (i = 0; i < s_tFmcGeometry.tCflash.un8NumWProtectedBlocks; i++)
    {
        LOG("\t[%d]block : %d segments with 0x%x write protion\n" ,i, s_tFmcGeometry.tCflash.tWProtectedBlocks[i].un8NumWProtectedSegs,
            s_tFmcGeometry.tCflash.tWProtectedBlocks[i].un32WProtectedSegSize);
    }

    LOG("\n\tWrite Protection Size (Data Flash)\n");
    for (i = 0; i < s_tFmcGeometry.tDflash.un8NumWProtectedBlocks; i++)
    {
        LOG("\t[%d]block : %d segments with 0x%x write protion\n" ,i, s_tFmcGeometry.tDflash.tWProtectedBlocks[i].un8NumWProtectedSegs,
            s_tFmcGeometry.tDflash.tWProtectedBlocks[i].un32WProtectedSegSize);
    }

    /* Multi bank information */
    eRet = HAL_CFMC_GetBankSwitchStatus(&eBootBank, &bBankSwitchDone, &eActiveBank);
    if (eRet == HAL_ERR_OK)
    {
        LOG("\tBoot bank is (%s), Bank siwtching is (%s), Active bank is (%s)\n", (eBootBank == CFMC_BANK_A) ? "BANK-A" : "BANK-B", \
                              (bBankSwitchDone) ? "Done" : "NOT Done", (eActiveBank == CFMC_BANK_A) ? "BANK-A" : "BANK-B");
    }

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_FMC_SetLatency(int32_t n32Argc, char *pn8Argv[])
{
    FMC_CONFIG_t tFmcConfig;
    FMC_ID_e eFmcID;
    uint32_t un32Latency;

    if (n32Argc != 3)
    {
        EX_FMC_Help((int32_t)NULL,(char **)NULL);
        return DEBUG_CMD_INVALID;
    }

    eFmcID = (FMC_ID_e)atoi(pn8Argv[EX_FMC_1ST_ARG]);
    un32Latency = atoi(pn8Argv[EX_FMC_2ND_ARG]);

    if (un32Latency > EX_FMC_MAX_LATENCY_VAL)
    {
        EX_FMC_Help((int32_t)NULL,(char **)NULL);
        return DEBUG_CMD_INVALID;
    }

    tFmcConfig.bReqCrcEnable = false;
    tFmcConfig.bReqCrcInit = false;
    tFmcConfig.un8TimeOut = 0;
    tFmcConfig.un8Latency = un32Latency;
    HAL_FMC_SetConfig(eFmcID, &tFmcConfig);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_CFMC_SetCache(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eRet;
    CFMC_CACHE_ID_e eCacheID;
    bool bRequestEnable;

    if (n32Argc != 3)
    {
        EX_FMC_Help((int32_t)NULL,(char **)NULL);
        return DEBUG_CMD_INVALID;
    }

    eCacheID = (CFMC_CACHE_ID_e)atoi(pn8Argv[EX_FMC_1ST_ARG]);
    bRequestEnable = atoi(pn8Argv[EX_FMC_2ND_ARG]);

    eRet = HAL_CFMC_SetCache(eCacheID, bRequestEnable);
    if (eRet)
    {
        return DEBUG_CMD_FAILED;
    }

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_CFMC_SetMultiBank(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eRet;
    bool bRequestMultiBank;

    if (n32Argc != 2)
    {
        EX_FMC_Help((int32_t)NULL,(char **)NULL);
        return DEBUG_CMD_INVALID;
    }

    bRequestMultiBank = atoi(pn8Argv[EX_FMC_1ST_ARG]);

    eRet = HAL_CFMC_SetMultiBankConfig(bRequestMultiBank);
    if (eRet)
    {
        return DEBUG_CMD_FAILED;
    }

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_CFMC_SetActiveBank(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eRet;
    CFMC_BANK_ID_e eBankID;

    if (n32Argc != 2)
    {
        EX_FMC_Help((int32_t)NULL,(char **)NULL);
        return DEBUG_CMD_INVALID;
    }

    eBankID = (CFMC_BANK_ID_e)atoi(pn8Argv[EX_FMC_1ST_ARG]);

    eRet = HAL_CFMC_SetActiveBank(eBankID);
    if (eRet)
    {
        return DEBUG_CMD_FAILED;
    }

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_CFMC_SetBootBankOnUserInfo(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eRet;
    CFMC_BANK_ID_e eBankID;

    if (n32Argc != 2)
    {
        EX_FMC_Help((int32_t)NULL,(char **)NULL);
        return DEBUG_CMD_INVALID;
    }

    eBankID = (CFMC_BANK_ID_e)atoi(pn8Argv[EX_FMC_1ST_ARG]);

    eRet = HAL_CFMC_SetBootBankOnUserInfo(eBankID);
    if (eRet)
    {
        return DEBUG_CMD_FAILED;
    }

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_FMC_SetWriteProtect(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eRet;
    FMC_ID_e eFmcID;
    uint32_t un32FlashOffset;
    uint32_t un32Len;
    bool bRequestLock;

    if (n32Argc != 5)
    {
        EX_FMC_Help((int32_t)NULL,(char **)NULL);
        return DEBUG_CMD_INVALID;
    }

    eFmcID = (FMC_ID_e)atoi(pn8Argv[EX_FMC_1ST_ARG]);
    un32FlashOffset = debug_cmd_string2int(pn8Argv[EX_FMC_2ND_ARG]);
    un32Len = debug_cmd_string2int(pn8Argv[EX_FMC_3RD_ARG]);
    bRequestLock = atoi(pn8Argv[EX_FMC_4TH_ARG]);

    eRet = HAL_FMC_SetWriteProtect(eFmcID, un32FlashOffset, un32Len, bRequestLock);
    if (eRet)
    {
        return DEBUG_CMD_FAILED;
    }

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_CFMC_SetWriteBootProtect(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eRet;
    bool bRequestLock;

    if (n32Argc != 2)
    {
        EX_FMC_Help((int32_t)NULL,(char **)NULL);
        return DEBUG_CMD_INVALID;
    }

    bRequestLock = atoi(pn8Argv[EX_FMC_1ST_ARG]);

    eRet = HAL_CFMC_SetWriteProtectBootBlk(bRequestLock);
    if (eRet)
    {
        return DEBUG_CMD_FAILED;
    }

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_FMC_SetReadProtect(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eRet;
    FMC_ID_e eFmcID;
    FMC_RD_PROTECT_CONFIG_t tRdProtectConfig;

    if (n32Argc != 4)
    {
        EX_FMC_Help((int32_t)NULL,(char **)NULL);
        return DEBUG_CMD_INVALID;
    }

    eFmcID = (FMC_ID_e)atoi(pn8Argv[EX_FMC_1ST_ARG]);
    tRdProtectConfig.eRdProtectLevel = (FMC_RD_PROTECT_LEVEL_e)debug_cmd_string2int(pn8Argv[EX_FMC_2ND_ARG]);
    tRdProtectConfig.un32RegisteredPasswd = debug_cmd_string2int(pn8Argv[EX_FMC_3RD_ARG]);

    eRet = HAL_FMC_SetReadProtect(eFmcID, tRdProtectConfig);
    if (eRet)
    {
        return DEBUG_CMD_FAILED;
    }

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_FMC_Erase(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eRet;
    FMC_ID_e eFmcID;
    uint32_t un32SectorOffset;
    FMC_ERASE_MODE_e eEraseMode;

    if (n32Argc != 4)
    {
        EX_FMC_Help((int32_t)NULL,(char **)NULL);
        return DEBUG_CMD_INVALID;
    }

    eFmcID = (FMC_ID_e)atoi(pn8Argv[EX_FMC_1ST_ARG]);
    un32SectorOffset = debug_cmd_string2int(pn8Argv[EX_FMC_2ND_ARG]);
    eEraseMode = (FMC_ERASE_MODE_e)atoi(pn8Argv[EX_FMC_3RD_ARG]);

    eRet = HAL_FMC_Erase(eFmcID, un32SectorOffset, eEraseMode);
    if (eRet)
    {
        return DEBUG_CMD_FAILED;
    }

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_FMC_Write(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eRet;
    FMC_ID_e eFmcID;
    uint32_t un32StartAddr;
    uint32_t un32TargetAddr;
    uint32_t un32Len;
    uint32_t un32Cnt;
    uint32_t un32WProtSegSizeFirstBlock;

    if (n32Argc != 4)
    {
        EX_FMC_Help((int32_t)NULL,(char **)NULL);
        return DEBUG_CMD_INVALID;
    }

    eFmcID = (FMC_ID_e)atoi(pn8Argv[EX_FMC_1ST_ARG]);
    un32StartAddr = debug_cmd_string2int(pn8Argv[EX_FMC_2ND_ARG]);
    un32Len = debug_cmd_string2int(pn8Argv[EX_FMC_3RD_ARG]);

    if ((un32Len & 0x3) || (un32Len > EX_FMC_DATA_BUFFER_SIZE))
    {
        if (un32Len != 0xFFFFFFFF)
        {
            EX_FMC_Help((int32_t)NULL,(char **)NULL);
            return DEBUG_CMD_INVALID;
        }
    }

    if (un32Len == 0xFFFFFFFF)
    {
        if (eFmcID == FMC_ID_CFMC)
        {
            un32WProtSegSizeFirstBlock = s_tFmcGeometry.tCflash.tWProtectedBlocks[EX_FMC_FIRST_WPROT_BLOCK].un32WProtectedSegSize;
        }
        else
        {
            un32WProtSegSizeFirstBlock = s_tFmcGeometry.tDflash.tWProtectedBlocks[EX_FMC_FIRST_WPROT_BLOCK].un32WProtectedSegSize;
        }

        for (un32Cnt=0; un32Cnt < un32WProtSegSizeFirstBlock/EX_FMC_DATA_BUFFER_SIZE; un32Cnt++)
        {
            un32TargetAddr = un32StartAddr + (EX_FMC_DATA_BUFFER_SIZE *un32Cnt);
            eRet = HAL_FMC_Write(eFmcID, un32TargetAddr, s_aun8Buffer, EX_FMC_DATA_BUFFER_SIZE);
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
            eRet = HAL_FMC_Write(eFmcID, un32TargetAddr, &s_aun8Buffer[4 *un32Cnt], 4);
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

static enum debug_cmd_status EX_FMC_WriteByte(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eRet;
    uint32_t un32StartAddr;
    uint8_t un8Byte;

    if (n32Argc != 3)
    {
        EX_FMC_Help((int32_t)NULL,(char **)NULL);
        return DEBUG_CMD_INVALID;
    }

    un32StartAddr = debug_cmd_string2int(pn8Argv[EX_FMC_1ST_ARG]);
    un8Byte = debug_cmd_string2int(pn8Argv[EX_FMC_2ND_ARG]);

    eRet = HAL_DFMC_WriteByte(un32StartAddr, &un8Byte, 1);
    if (eRet)
    {
        return DEBUG_CMD_FAILED;
    }

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_FMC_SelfErase(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eRet;
    FMC_ID_e eFmcID;
    uint32_t un32StartAddr;

    if (n32Argc != 3)
    {
        EX_FMC_Help((int32_t)NULL,(char **)NULL);
        return DEBUG_CMD_INVALID;
    }

    eFmcID = (FMC_ID_e)atoi(pn8Argv[EX_FMC_1ST_ARG]);
    un32StartAddr = debug_cmd_string2int(pn8Argv[EX_FMC_2ND_ARG]);

    eRet = HAL_FMC_SelfErase(eFmcID, un32StartAddr);
    if (eRet)
    {
        return DEBUG_CMD_FAILED;
    }

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_FMC_SelfWrite(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eRet;
    FMC_ID_e eFmcID;
    uint32_t un32StartAddr;
    uint32_t un32TargetAddr;
    uint32_t un32Len;
    uint32_t un32Cnt;
    uint32_t un32WProtSegSizeFirstBlock;

    if (n32Argc != 4)
    {
        EX_FMC_Help((int32_t)NULL,(char **)NULL);
        return DEBUG_CMD_INVALID;
    }

    eFmcID = (FMC_ID_e)atoi(pn8Argv[EX_FMC_1ST_ARG]);
    un32StartAddr = debug_cmd_string2int(pn8Argv[EX_FMC_2ND_ARG]);
    un32Len = debug_cmd_string2int(pn8Argv[EX_FMC_3RD_ARG]);

    if ((un32Len & 0x3) || (un32Len > EX_FMC_DATA_BUFFER_SIZE))
    {
        if (un32Len != 0xFFFFFFFF)
        {
            EX_FMC_Help((int32_t)NULL,(char **)NULL);
            return DEBUG_CMD_INVALID;
        }
    }

    if (un32Len == 0xFFFFFFFF)
    {
        if (eFmcID == FMC_ID_CFMC)
        {
            un32WProtSegSizeFirstBlock = s_tFmcGeometry.tCflash.tWProtectedBlocks[EX_FMC_FIRST_WPROT_BLOCK].un32WProtectedSegSize;
        }
        else
        {
            un32WProtSegSizeFirstBlock = s_tFmcGeometry.tDflash.tWProtectedBlocks[EX_FMC_FIRST_WPROT_BLOCK].un32WProtectedSegSize;
        }

        for (un32Cnt=0; un32Cnt < un32WProtSegSizeFirstBlock/EX_FMC_DATA_BUFFER_SIZE; un32Cnt++)
        {
            un32TargetAddr = un32StartAddr + (EX_FMC_DATA_BUFFER_SIZE *un32Cnt);
            eRet = HAL_FMC_SelfWrite(eFmcID, un32TargetAddr, s_aun8Buffer, EX_FMC_DATA_BUFFER_SIZE);
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
            eRet = HAL_FMC_SelfWrite(eFmcID, un32TargetAddr, &s_aun8Buffer[4 *un32Cnt], 4);
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

static const struct debug_cmd s_tEX_FMC_CMD[] =
{
    {"FMC", "h",EX_FMC_Help,"Usage"},
    {"FMC", "info",EX_FMC_PrintFlahsInfo, "Print flash geometric info"},
    {"FMC", "latency",EX_FMC_SetLatency,"Set latency"},
    {"FMC", "cache",EX_CFMC_SetCache,"Set I or/and D cache"},
    {"FMC", "multibank",EX_CFMC_SetMultiBank,"Set multi-banks"},
    {"FMC", "activebank",EX_CFMC_SetActiveBank,"Set active banks (bank A or bank B)"},
    {"FMC", "bbank",EX_CFMC_SetBootBankOnUserInfo,"Write boot bank on UserInfo"},
    {"FMC", "wprot",EX_FMC_SetWriteProtect,"Set write protection"},
    {"FMC", "rprot",EX_FMC_SetReadProtect,"Set read protection"},
    {"FMC", "bbwprot",EX_CFMC_SetWriteBootProtect,"Set boot-block(first 4KB) write protection"},
    {"FMC", "erase",EX_FMC_Erase,"Erase flash(Page, 1KB, 2KB or 4KB mode)"},
    {"FMC", "write",EX_FMC_Write,"Write flash with word aligned data size (max. 32B)"},
    {"FMC", "bwrite",EX_FMC_WriteByte,"Write one byte to Data flash only"},

    {"FMC", "selferase",EX_FMC_SelfErase,"Self Erase flash with page or sector aligned address"},
    {"FMC", "selfwrite",EX_FMC_SelfWrite,"Self Write flash with word aligned data size (max. 32B)"},
};

void EX_FLASH(void)
{
    uint32_t un32Cnt;

    /* Initialization */
    HAL_FMC_Init();

    s_tFmcGeometry = HAL_FMC_GetGeometry();
    LOG("\tCode Flash Size    : 0x%x Bytes\n", s_tFmcGeometry.tCflash.un32Size);
    LOG("\tData Flash Size    : 0x%x Bytes\n", s_tFmcGeometry.tDflash.un32Size);
    LOG("\tSystem Flash Size  : 0x%x Bytes\n\n", s_tFmcGeometry.tSysflash.un32Size);

    LOG("\tWrite Protection Size (Code Flash)\n");
    for (un32Cnt = 0; un32Cnt < s_tFmcGeometry.tCflash.un8NumWProtectedBlocks; un32Cnt++)
    {
        LOG("\t[%d]block : %d segments with 0x%x write protion\n" ,un32Cnt, s_tFmcGeometry.tCflash.tWProtectedBlocks[un32Cnt].un8NumWProtectedSegs,
            s_tFmcGeometry.tCflash.tWProtectedBlocks[un32Cnt].un32WProtectedSegSize);
    }

    LOG("\n\tWrite Protection Size (Data Flash)\n");
    for (un32Cnt = 0; un32Cnt < s_tFmcGeometry.tDflash.un8NumWProtectedBlocks; un32Cnt++)
    {
        LOG("\t[%d]block : %d segments with 0x%x write protion" ,un32Cnt, s_tFmcGeometry.tDflash.tWProtectedBlocks[un32Cnt].un8NumWProtectedSegs,
            s_tFmcGeometry.tDflash.tWProtectedBlocks[un32Cnt].un32WProtectedSegSize);
    }

    /* Update test buffer */
    for (un32Cnt = 0; un32Cnt < EX_FMC_DATA_BUFFER_SIZE; un32Cnt++)
    {
        s_aun8Buffer[un32Cnt] = un32Cnt;
    }

    /* Add TC commands */
    debug_cmd_init(s_tEX_FMC_CMD,DEBUG_CMD_LIST_COUNT(s_tEX_FMC_CMD));
}
#endif /* _CFMC && _DFMC */
#endif /* FLASH_TC */

/* --------------------------------- End Of File ------------------------------ */
