/**
 *******************************************************************************
 * @file        qspi_flash.c
 * @author      ABOV R&D Division
 * @brief       Flash Memory Example Code
 *
 * Copyright 2022 ABOV Semiconductor Co.,Ltd. All rights reserved.
 *
 * This file is licensed under terms that are found in the LICENSE file
 * located at Document directory.
 * If this file is delivered or shared without applicable license terms,
 * the terms of the BSD-3-Clause license shall be applied.
 * Reference: https://opensource.org/licenses/BSD-3-Clause
 ******************************************************************************/

#include "abov_config.h"

#include "debug_log.h"
#include "debug.h"
#include "hal_scu.h"
#include "hal_scu_clk.h"
#include "hal_pcu.h"
#include "hal_qspi.h"

#include "flash_setup.h"

/* HCLK is generated from HSI by default */
#define TC_SYSTICK_1MS      1000

#define TC_TEST_OPS         QSPI_OPS_INTR

#define TC_QSPI_EXAMPLE_INTR
#define TC_QSPI_EXAMPLE_DMA

#define TC_QSPI_DESC_MAX_CNT 2

extern uint32_t SystemCoreClock;
static volatile uint32_t s_un32SysTimerVal=0;
static volatile uint32_t g_msTick = 0;

typedef struct
{
    volatile uint32_t un32Events;
    volatile bool     bDone;
} EX_QSPI_IRQ_CTX_t;

static EX_QSPI_IRQ_CTX_t s_tQspiIrqCtx;

#if (CONFIG_DEBUG == 1)
void GetChipSetInfo(void)
{
    int8_t *pn8ChipInfo = NULL;
    int8_t *pn8ChipCoreInfo = NULL;
    pn8ChipInfo = PRV_CHIPSET_GetInfo();
    pn8ChipCoreInfo = PRV_CHIPSET_GetCoreInfo();
    LOG("************************************************\n\r");
    LOG("- MCU - %s \n",pn8ChipInfo);
    LOG("- Core: ARM %s  \n",pn8ChipCoreInfo);
    LOG("- Communicate via: %s%d - %dbps\n",CONFIG_DEBUG_MODULE_STR,DEBUG_UART_ID,APP_UART_BAUD);
    LOG("- ARM System Core Clock = %d\n",SystemCoreClock);
    LOG("- QSPI FLASH Interrupt Example\n");
    LOG("************************************************\n\r");
}
#endif

/**********************************************************************
 * @brief		Swap Endian per 32-bit word
 * @param[in]	buf : Buffer pointer to swap
 * @param[in]	len : Buffer length in bytes (must be multiple of 4)
 * @return	    true : Success, false : Invalid parameter
 **********************************************************************/
bool SwapEndian32PerWord(uint8_t *buf, uint32_t len)
{
    if ((buf == NULL) || ((len & 0x3U) != 0U))
    {
        return false;
    }

    for (uint32_t i = 0; i < len; i += 4U)
    {
        uint8_t t0 = buf[i + 0];
        uint8_t t1 = buf[i + 1];

        buf[i + 0] = buf[i + 3];
        buf[i + 1] = buf[i + 2];
        buf[i + 2] = t1;
        buf[i + 3] = t0;
    }

    return true;
}

/**********************************************************************
 * @brief		ARM System Timer Interrupt Handler.
 * @param[in]	None
 * @return	    None
 **********************************************************************/
void SysTick_Handler (void)
{
    if (s_un32SysTimerVal)
    {
        s_un32SysTimerVal--;
    }
}

/**********************************************************************
 * @brief		Waiting by time(ms)
 * @param[in]	un32TimeMS : Milisecond time to wait.
 * @return	    None
 **********************************************************************/
static void SYSTICK_Wait (uint32_t un32TimeMS)
{
    s_un32SysTimerVal = un32TimeMS;
    while(s_un32SysTimerVal);
}

static uint32_t GetMsTick(void)
{
    return g_msTick;
}

static HAL_ERR_e PRV_QSPI_WaitIrqDone(EX_QSPI_IRQ_CTX_t *tCtx, uint32_t timeoutMs)
{
    if (TC_TEST_OPS == QSPI_OPS_INTR)
    {
        uint32_t t0 = GetMsTick();

        while (!tCtx->bDone)
        {
            if ((GetMsTick() - t0) >= timeoutMs)
                return HAL_ERR_TIMEOUT;
        }
        tCtx->bDone = false;

        if (tCtx->un32Events & (QSPI_EVENT_RX_ERROR | QSPI_EVENT_TX_ERROR))
            return HAL_ERR_UNKNOWN;
    }
    return HAL_ERR_OK;
}
/**********************************************************************
 * @brief		QSPI Transfer (Command + Address + Data)
 * @param[in]	eId : QSPI Instance ID
 * @param[in]	un16Cmd : Command Value
 * @param[in]	un32Addr : Address Value
 * @param[in]	un32Size : Data Size
 * @param[in]	*pData : Data Buffer Pointer
 * @param[in]	eDataMode : Data Phase Mode
 * @return	    HAL_ERR_e : HAL ERR code
 **********************************************************************/
static HAL_ERR_e PRV_QSPI_Transfer(QSPI_ID_e eId, uint16_t un16Cmd, bool bAddr, uint32_t un32Addr, uint32_t un32DataSize, uint8_t *pData, QSPI_DIR_MODE_e un8DirMode)
{
    HAL_ERR_e ret = HAL_ERR_OK;
    QSPI_PHASE_CFG_t tCmdCfg =
    {
        .ePhaseType             = QSPI_PHASE_CMD,
        .eOpMode                = QSPI_OP_MODE_INDIRECT,
        .eSampleRate            = QSPI_SAMPLE_RATE_SINGLE,
        .tPhaCfg.tCmd.eWidth    = QSPI_PHASE_WIDTH_8BIT,
        .eBusMode               = QSPI_BUS_MODE_SINGLE,
        .bSkip = false,
    };
    QSPI_PHASE_CFG_t tAddrCfg =
    {
        .ePhaseType             = QSPI_PHASE_ADDR,
        .eOpMode                = QSPI_OP_MODE_INDIRECT,
        .eSampleRate            = QSPI_SAMPLE_RATE_SINGLE,
        .tPhaCfg.tAddr.eWidth   = QSPI_PHASE_WIDTH_24BIT,
        .bSkip = true
    };
    QSPI_PHASE_CFG_t tMBitCfg =
    {
        .ePhaseType             = QSPI_PHASE_MBIT,
        .eOpMode                = QSPI_OP_MODE_INDIRECT,
        .eSampleRate            = QSPI_SAMPLE_RATE_SINGLE,
        .tPhaCfg.tMBits.eWidth  = QSPI_PHASE_WIDTH_8BIT,
        .bSkip = true
    };
    QSPI_PHASE_CFG_t tDmyCfg =
    {
        .ePhaseType = QSPI_PHASE_DUMMY,
        .eOpMode = QSPI_OP_MODE_INDIRECT,
        .bSkip = true
    };
    QSPI_PHASE_CFG_t tDataCfg =
    {
        .ePhaseType = QSPI_PHASE_DATA,
        .eOpMode = QSPI_OP_MODE_INDIRECT,
        .bSkip = true
    };

    if((un16Cmd == MX25_CMD_QPP) || (un16Cmd == MX25_CMD_QIOREAD))
    {
        tAddrCfg.eBusMode = QSPI_BUS_MODE_QUAD;
        tDmyCfg.bSkip = false;
        tDmyCfg.tPhaCfg.tDmy.un8Cycles = 6; // 6 dummy cycles
        tDataCfg.eBusMode = QSPI_BUS_MODE_QUAD;
    }
    else if(un16Cmd == MX25_CMD_QREAD)
    {
        tAddrCfg.eBusMode = QSPI_BUS_MODE_SINGLE;
        tDmyCfg.bSkip = false;
        tDmyCfg.tPhaCfg.tDmy.un8Cycles = 8; // 8 dummy cycles
        tDataCfg.eBusMode = QSPI_BUS_MODE_QUAD;
    }
    else if(un16Cmd == MX25_CMD_DIOREAD)
    {
        tAddrCfg.eBusMode = QSPI_BUS_MODE_DUAL;
        tDataCfg.eBusMode = QSPI_BUS_MODE_DUAL;
    }
    else if(un16Cmd == MX25_CMD_DREAD)
    {
        tAddrCfg.eBusMode = QSPI_BUS_MODE_SINGLE;
        tDataCfg.eBusMode = QSPI_BUS_MODE_DUAL;
    }
    else
    {
        tAddrCfg.eBusMode = QSPI_BUS_MODE_SINGLE;
        tDataCfg.eBusMode = QSPI_BUS_MODE_SINGLE;
    }

    /* ---------------- Command Phase ---------------- */
    ret = HAL_QSPI_SetPhaseConfig(eId, &tCmdCfg);

    /* ---------------- Address Phase ---------------- */
    tAddrCfg.bSkip                  = !bAddr;
    tAddrCfg.tPhaCfg.tAddr.un32Addr = un32Addr;
    ret = HAL_QSPI_SetPhaseConfig(eId, &tAddrCfg);

    /* ---------------- Mode Bits Phase (skip) -------- */
    ret = HAL_QSPI_SetPhaseConfig(eId, &tMBitCfg);

    /* ---------------- Dummy Phase (skip) ------------ */
    ret = HAL_QSPI_SetPhaseConfig(eId, &tDmyCfg);

    /* ---------------- Data Phase -------------------- */
    tDataCfg.bSkip = un32DataSize > 0 ? false : true;
    tDataCfg.eSampleRate = QSPI_SAMPLE_RATE_SINGLE;
    tDataCfg.tPhaCfg.tData.eDirMode = un8DirMode;
    tDataCfg.tPhaCfg.tData.eFlipMode = QSPI_FLIP_DISABLE;
    tDataCfg.tPhaCfg.tData.un32Size = un32DataSize;
    ret = HAL_QSPI_SetPhaseConfig(eId, &tDataCfg);

    /* Execute command */
    ret = HAL_QSPI_Command(eId, un16Cmd, 10000);
    if (ret != HAL_ERR_OK)
    {
        LOG("QSPI Execute Error: %d\n", ret);
        return ret;
    }
    if (pData != NULL)
    {
        /* Data Transfer */
        if (un8DirMode == QSPI_DIR_INPUT)
        {
            ret = HAL_QSPI_Receive(eId, pData, un32DataSize, false);
        }
        else
        {
            ret = HAL_QSPI_Transmit(eId, pData, un32DataSize, false);
        }
        /* 5. Wait for Done */
        ret = PRV_QSPI_WaitIrqDone(&s_tQspiIrqCtx, 1000);
        if (ret != HAL_ERR_OK)
        {
            LOG("QSPI Wait IRQ Timeout/Error: %d evt=0x%08X\n", ret, s_tQspiIrqCtx.un32Events);
            return ret;
        }
    }
    return ret;
}

#if defined (TC_QSPI_EXAMPLE_DMA)
static HAL_ERR_e PRV_QSPI_TransferDma(QSPI_ID_e eId, uint16_t un16Cmd, bool bAddr, uint32_t un32Addr, uint32_t un32DataSize, uint8_t *pun8Data, QSPI_DIR_MODE_e un8DirMode)
{
    HAL_ERR_e ret = HAL_ERR_OK;
    QSPI_PHASE_CFG_t tCmdCfg =
    {
        .ePhaseType             = QSPI_PHASE_CMD,
        .eOpMode                = QSPI_OP_MODE_INDIRECT,
        .eSampleRate            = QSPI_SAMPLE_RATE_SINGLE,
        .tPhaCfg.tCmd.eWidth    = QSPI_PHASE_WIDTH_8BIT,
        .eBusMode               = QSPI_BUS_MODE_SINGLE,
        .bSkip = false,
    };
    QSPI_PHASE_CFG_t tAddrCfg =
    {
        .ePhaseType             = QSPI_PHASE_ADDR,
        .eOpMode                = QSPI_OP_MODE_INDIRECT,
        .eSampleRate            = QSPI_SAMPLE_RATE_SINGLE,
        .tPhaCfg.tAddr.eWidth   = QSPI_PHASE_WIDTH_24BIT,
        .bSkip = true
    };
    QSPI_PHASE_CFG_t tMBitCfg =
    {
        .ePhaseType             = QSPI_PHASE_MBIT,
        .eOpMode                = QSPI_OP_MODE_INDIRECT,
        .eSampleRate            = QSPI_SAMPLE_RATE_SINGLE,
        .tPhaCfg.tMBits.eWidth  = QSPI_PHASE_WIDTH_8BIT,
        .bSkip = true
    };
    QSPI_PHASE_CFG_t tDmyCfg =
    {
        .ePhaseType = QSPI_PHASE_DUMMY,
        .eOpMode = QSPI_OP_MODE_INDIRECT,
        .bSkip = true
    };
    QSPI_PHASE_CFG_t tDataCfg =
    {
        .ePhaseType = QSPI_PHASE_DATA,
        .eOpMode = QSPI_OP_MODE_INDIRECT,
        .bSkip = true
    };

    if((un16Cmd == MX25_CMD_QPP) || (un16Cmd == MX25_CMD_QIOREAD))
    {
        tAddrCfg.eBusMode = QSPI_BUS_MODE_QUAD;
        tDmyCfg.bSkip = false;
        tDmyCfg.tPhaCfg.tDmy.un8Cycles = 6; // 6 dummy cycles
        tDataCfg.eBusMode = QSPI_BUS_MODE_QUAD;
    }
    else if(un16Cmd == MX25_CMD_QREAD)
    {
        tAddrCfg.eBusMode = QSPI_BUS_MODE_SINGLE;
        tDmyCfg.bSkip = false;
        tDmyCfg.tPhaCfg.tDmy.un8Cycles = 8; // 8 dummy cycles
        tDataCfg.eBusMode = QSPI_BUS_MODE_QUAD;
    }
    else if(un16Cmd == MX25_CMD_DIOREAD)
    {
        tAddrCfg.eBusMode = QSPI_BUS_MODE_DUAL;
        tDataCfg.eBusMode = QSPI_BUS_MODE_DUAL;
    }
    else if(un16Cmd == MX25_CMD_DREAD)
    {
        tAddrCfg.eBusMode = QSPI_BUS_MODE_SINGLE;
        tDataCfg.eBusMode = QSPI_BUS_MODE_DUAL;
    }
    else
    {
        tAddrCfg.eBusMode = QSPI_BUS_MODE_SINGLE;
        tDataCfg.eBusMode = QSPI_BUS_MODE_SINGLE;
    }

    /* ---------------- Command Phase ---------------- */
    ret = HAL_QSPI_SetPhaseConfig(eId, &tCmdCfg);

    /* ---------------- Address Phase ---------------- */
    tAddrCfg.bSkip                  = !bAddr;
    tAddrCfg.tPhaCfg.tAddr.un32Addr = un32Addr;
    ret = HAL_QSPI_SetPhaseConfig(eId, &tAddrCfg);

    /* ---------------- Mode Bits Phase (skip) -------- */
    ret = HAL_QSPI_SetPhaseConfig(eId, &tMBitCfg);

    /* ---------------- Dummy Phase (skip) ------------ */
    ret = HAL_QSPI_SetPhaseConfig(eId, &tDmyCfg);

    /* ---------------- Data Phase -------------------- */
    tDataCfg.bSkip = un32DataSize > 0 ? false : true;
    tDataCfg.eSampleRate = QSPI_SAMPLE_RATE_SINGLE;
    tDataCfg.tPhaCfg.tData.eDirMode = un8DirMode;
    tDataCfg.tPhaCfg.tData.eFlipMode = QSPI_FLIP_DISABLE;
    tDataCfg.tPhaCfg.tData.un32Size = un32DataSize;
    ret = HAL_QSPI_SetPhaseConfig(eId, &tDataCfg);

    QSPI_DMA_DESC_t tDmaDesc[TC_QSPI_DESC_MAX_CNT];
    uint32_t un32DescCnt = TC_QSPI_DESC_MAX_CNT;
    uint32_t un32DescBytes = un32DataSize / un32DescCnt;
    uint16_t un16DescWords = 1;

    if(un32DescBytes < 4)
    {
        un32DescCnt = 1;
        un32DescBytes = un32DataSize;
    }
    un16DescWords = un32DescBytes / 4;

    for (int i = 0; i < un32DescCnt; i++)
    {
        tDmaDesc[i] = (QSPI_DMA_DESC_t)
        {
            .tCtrl.b.bValid = true,
            .tCtrl.b.bEnd = (i == (un32DescCnt - 1)),
            .tCtrl.b.bIrqEn = (i == (un32DescCnt - 1)),
            .tCtrl.b.eDescType = QSPI_DMA_DESC_TYPE_DATA,
            .tCtrl.b.un16Size = un16DescWords,
            .un32Addr = (uint32_t)(pun8Data + (un32DescBytes * i)),
        };
    }

    QSPI_DMA_CFG_t tDmaCfg =
    {
        .un32DescAddr = (uint32_t)&tDmaDesc,
        .eFixedBurstLen = QSPI_BURST_UNDERMINED
    };

    ret = HAL_QSPI_SetDmaConfig(eId, &tDmaCfg);

    if (pun8Data != NULL)
    {
        /* Data Transfer */
        if (un8DirMode == QSPI_DIR_INPUT)
        {
            ret = HAL_QSPI_Receive(eId, NULL, un32DataSize, false);
        }
        else
        {
            ret = HAL_QSPI_Transmit(eId, NULL, un32DataSize, false);
        }
    }
    /* Execute command */
    ret = HAL_QSPI_Command(eId, un16Cmd, 10000);
    if (ret != HAL_ERR_OK)
    {
        LOG("QSPI Execute Error: %d\n", ret);
        return ret;
    }
    /* Wait for Done */
    ret = PRV_QSPI_WaitIrqDone(&s_tQspiIrqCtx, 1000);
    if (ret != HAL_ERR_OK)
    {
        LOG("QSPI Wait IRQ Timeout/Error: %d evt=0x%08X\n", ret, s_tQspiIrqCtx.un32Events);
        return ret;
    }
    return ret;
}
#endif
static uint8_t PRV_QSPI_ReadStatus(QSPI_ID_e eId)
{
    HAL_ERR_e ret = HAL_ERR_OK;
    uint8_t un8Status = 0;

    ret = PRV_QSPI_Transfer(eId, MX25_CMD_RDSR, false, 0, 1, &un8Status, QSPI_DIR_INPUT);
    if (ret != HAL_ERR_OK)
    {
        LOG("QSPI MX25_CMD_RDSR Error: %d\n", ret);
    }
    return un8Status;
}

/**********************************************************************
 * @brief		QSPI Port Setting Test Case
 * @param[in]	None
 * @return	    None
 **********************************************************************/
static void EX_QSPI_Port(void)
{
    /* QSPI SS Port*/
    HAL_PCU_SetAltMode(PCU_ID_B, PCU_PIN_ID_15, PCU_ALT_1);
    HAL_PCU_SetInOutMode(PCU_ID_B, PCU_PIN_ID_15, PCU_INOUT_OUTPUT_PUSH_PULL);
    HAL_PCU_SetPullUpDown(PCU_ID_B, PCU_PIN_ID_15, PCU_PUPD_UP);
    HAL_PCU_SetPortStrength(PCU_ID_B, PCU_PIN_ID_15, true);
    /* QSPI SCK Port*/
    HAL_PCU_SetAltMode(PCU_ID_B, PCU_PIN_ID_14, PCU_ALT_1);
    HAL_PCU_SetInOutMode(PCU_ID_B, PCU_PIN_ID_14, PCU_INOUT_INPUT);
    HAL_PCU_SetPullUpDown(PCU_ID_B, PCU_PIN_ID_14, PCU_PUPD_UP);
    HAL_PCU_SetPortStrength(PCU_ID_B, PCU_PIN_ID_14, true);
    /* QSPI IO0 Port*/
    HAL_PCU_SetAltMode(PCU_ID_B, PCU_PIN_ID_13, PCU_ALT_1);
    HAL_PCU_SetInOutMode(PCU_ID_B, PCU_PIN_ID_13, PCU_INOUT_INPUT);
    HAL_PCU_SetPullUpDown(PCU_ID_B, PCU_PIN_ID_13, PCU_PUPD_UP);
    HAL_PCU_SetPortStrength(PCU_ID_B, PCU_PIN_ID_13, true);
    /* QSPI IO1 Port*/
    HAL_PCU_SetAltMode(PCU_ID_B, PCU_PIN_ID_12, PCU_ALT_1);
    HAL_PCU_SetInOutMode(PCU_ID_B, PCU_PIN_ID_12, PCU_INOUT_INPUT);
    HAL_PCU_SetPullUpDown(PCU_ID_B, PCU_PIN_ID_12, PCU_PUPD_UP);
    HAL_PCU_SetPortStrength(PCU_ID_B, PCU_PIN_ID_12, true);
    /* QSPI IO2 Port*/
    HAL_PCU_SetAltMode(PCU_ID_B, PCU_PIN_ID_11, PCU_ALT_1);
    HAL_PCU_SetInOutMode(PCU_ID_B, PCU_PIN_ID_11, PCU_INOUT_INPUT);
    HAL_PCU_SetPullUpDown(PCU_ID_B, PCU_PIN_ID_11, PCU_PUPD_UP);
    HAL_PCU_SetPortStrength(PCU_ID_B, PCU_PIN_ID_11, true);
    /* QSPI IO3 Port*/
    HAL_PCU_SetAltMode(PCU_ID_B, PCU_PIN_ID_10, PCU_ALT_1);
    HAL_PCU_SetInOutMode(PCU_ID_B, PCU_PIN_ID_10, PCU_INOUT_INPUT);
    HAL_PCU_SetPullUpDown(PCU_ID_B, PCU_PIN_ID_10, PCU_PUPD_UP);
    HAL_PCU_SetPortStrength(PCU_ID_B, PCU_PIN_ID_10, true);
}

/**********************************************************************
 * @brief		QSPI Read ID Test Case
 * @param[in]	eId : QSPI Instance ID
 * @return	    HAL_ERR_e : HAL ERR code
 **********************************************************************/
static void EX_QSPI_ReadID(QSPI_ID_e eId)
{
    uint8_t un8FlashId[3] = {0,};

    while (PRV_QSPI_ReadStatus(eId) & MX25_SR_WIP);

    PRV_QSPI_Transfer(eId, MX25_CMD_RDID, false, 0, 3, un8FlashId, QSPI_DIR_INPUT);

    LOG("FLASH ID : 0x%02X, 0x%02X, 0x%02X\r\n", un8FlashId[0], un8FlashId[1],un8FlashId[2]);
}

/**********************************************************************
 * @brief		QSPI Read Configuration Status Test Case
 * @param[in]	eId : QSPI Instance ID
 * @return	    HAL_ERR_e : HAL ERR code
 **********************************************************************/
static void EX_QSPI_ReadConfigueStatus(QSPI_ID_e eId)
{
    uint8_t result[2] = {0x1, 0x2};

    PRV_QSPI_Transfer(eId, MX25_CMD_RDCR, false, 0, 2, result, QSPI_DIR_INPUT);

    LOG("Configue Status : 0x%02X, 0x%02X\r\n", result[0], result[1]);
}

static void EX_QSPI_Program(QSPI_ID_e eId, uint16_t un16Command, uint32_t un32Address, uint32_t un32Len, uint8_t *pData)
{
    HAL_ERR_e ret = HAL_ERR_OK;

    while (PRV_QSPI_ReadStatus(eId) & MX25_SR_WIP);

    do
    {
        ret = PRV_QSPI_Transfer(QSPI_ID_0, MX25_CMD_WREN, false, 0, 0, NULL, QSPI_DIR_OUTPUT);
        if (ret != HAL_ERR_OK)
        {
            LOG("QSPI MX25_CMD_WREN Error: %d\n", ret);
            break;
        }
    }
    while (!(PRV_QSPI_ReadStatus(eId) & MX25_SR_WEL));

    ret = PRV_QSPI_Transfer(QSPI_ID_0, un16Command, true, un32Address, un32Len, pData, QSPI_DIR_OUTPUT);
    if (ret != HAL_ERR_OK)
    {
        LOG("QSPI CMD[%02X] Error: %d\n", un16Command, ret);
    }

    while (PRV_QSPI_ReadStatus(eId) & MX25_SR_WIP);

    ret = PRV_QSPI_Transfer(QSPI_ID_0, MX25_CMD_WRDI, false, 0, 0, NULL, QSPI_DIR_OUTPUT);
    if (ret != HAL_ERR_OK)
    {
        LOG("QSPI MX25_CMD_WRDI Error: %d\n", ret);
    }

    while (PRV_QSPI_ReadStatus(eId) & MX25_SR_WIP);
}
#if defined (TC_QSPI_EXAMPLE_DMA)
static void EX_QSPI_ProgramDma(QSPI_ID_e eId, uint16_t un16Command, uint32_t un32Address, uint32_t un32Len, uint8_t *pun8Data)
{
    HAL_ERR_e ret = HAL_ERR_OK;

    while (PRV_QSPI_ReadStatus(eId) & MX25_SR_WIP);

    do
    {
        ret = PRV_QSPI_Transfer(QSPI_ID_0, MX25_CMD_WREN, false, 0, 0, NULL, QSPI_DIR_OUTPUT);
        if (ret != HAL_ERR_OK)
        {
            LOG("QSPI MX25_CMD_WREN Error: %d\n", ret);
            break;
        }
    }
    while (!(PRV_QSPI_ReadStatus(eId) & MX25_SR_WEL));

    ret = PRV_QSPI_TransferDma(QSPI_ID_0, un16Command, true, un32Address, un32Len, pun8Data, QSPI_DIR_OUTPUT);
    if (ret != HAL_ERR_OK)
    {
        LOG("QSPI CMD[%02X] Error: %d\n", un16Command, ret);
    }

    while (PRV_QSPI_ReadStatus(eId) & MX25_SR_WIP);

    ret = PRV_QSPI_Transfer(QSPI_ID_0, MX25_CMD_WRDI, false, 0, 0, NULL, QSPI_DIR_OUTPUT);
    if (ret != HAL_ERR_OK)
    {
        LOG("QSPI MX25_CMD_WRDI Error: %d\n", ret);
    }

    while (PRV_QSPI_ReadStatus(eId) & MX25_SR_WIP);
}
#endif
static void EX_QSPI_QuadEnable(QSPI_ID_e eId,bool bEnable)
{
    uint8_t sr = 0;
    uint8_t cr = 0;
    uint8_t un8data[4] = {0,};

    do
    {
        PRV_QSPI_Transfer(eId, MX25_CMD_WREN, false, 0, 0, NULL, QSPI_DIR_OUTPUT);
        sr = PRV_QSPI_ReadStatus(eId);
        PRV_QSPI_Transfer(eId, MX25_CMD_RDCR, false, 0, 1, &cr, QSPI_DIR_INPUT);
    } while (!(sr & MX25_SR_WEL));

    un8data[0] = sr;
    if (bEnable)
    {
        un8data[0] = sr |= MX25_SR_QE_SR;
    }
    else
    {
        un8data[0] = sr &= !MX25_SR_QE_SR;
    }
    un8data[1] = cr;
    PRV_QSPI_Transfer(eId, MX25_CMD_WRSR, false, 0, 2, un8data, QSPI_DIR_OUTPUT);
    while (PRV_QSPI_ReadStatus(eId) & MX25_SR_WIP);

    sr = PRV_QSPI_ReadStatus(eId);
    
    PRV_QSPI_Transfer(eId, MX25_CMD_WRDI, false, 0, 0, NULL, QSPI_DIR_OUTPUT);

    while (PRV_QSPI_ReadStatus(eId) & MX25_SR_WIP);
}

static void EX_QSPI_IRQHandler(uint32_t un32Event, void *pContext)
{
    EX_QSPI_IRQ_CTX_t *tCtx = (EX_QSPI_IRQ_CTX_t *)pContext;

    tCtx->un32Events = un32Event;

    if (un32Event & QSPI_EVENT_RX_DONE)
    {
        tCtx->bDone = true;
    }
    if (un32Event &  QSPI_EVENT_TX_DONE)
    {
        tCtx->bDone = true;
    }
    if (un32Event & QSPI_EVENT_RX_ERROR)
    {
        tCtx->bDone = true;
        LOG("QSPI RX ERROR evt=0x%02X\r\n", un32Event);
    }
    if (un32Event & QSPI_EVENT_TX_ERROR)
    {
        tCtx->bDone = true;
        LOG("QSPI TX ERROR evt=0x%02X\r\n", un32Event);
    }
}

/**********************************************************************
 * @brief		Main program
 * @param[in]	None
 * @return	    0 : No error, Non-Zero : Any error
 **********************************************************************/
int main(void)
{
    SCUCLK_MCLK_CFG_t tMClkCfg = 
    {
        .eMClk = SCUCLK_SRC_HSI,
        .ePreMClkDiv = SCUCLK_DIV_NONE,
        .ePostMClkDiv = SCUCLK_DIV_NONE
    };

    PRV_PORT_Init();

    (void)HAL_SCU_CLK_SetSrcEnable(SCUCLK_SRC_LSI, true);
    (void)HAL_SCU_CLK_SetSrcEnable(SCUCLK_SRC_HSE, true);
    (void)HAL_SCU_CLK_SetSrcEnable(SCUCLK_SRC_HSI, true);
    (void)HAL_SCU_CLK_SetMClk(&tMClkCfg);

#if (CONFIG_DEBUG == 1)
    /* serial init */
    HAL_PCU_SetAltMode((PCU_ID_e)DEBUG_PORT_ID,(PCU_PIN_ID_e)DEBUG_TX_PORT_ID,(PCU_ALT_e)DEBUG_TX_ALT_ID); /* For Debug */
    HAL_PCU_SetAltMode((PCU_ID_e)DEBUG_PORT_ID,(PCU_PIN_ID_e)DEBUG_RX_PORT_ID,(PCU_ALT_e)DEBUG_RX_ALT_ID); /* For Debug */
    Debug_Init();
    GetChipSetInfo();
#endif

    /* Setup ARM System Timer to 1ms */
    SysTick_Config(SystemCoreClock/TC_SYSTICK_1MS);

    QSPI_CFG_t tCfg =
    {
        .eClkMode      = QSPI_CLK_MODE_TRANS_0_CPOL_0,
        .eTransEdge    = QSPI_TRANS_EDGE_NEGATIVE,
        .bCSLine       = true,
        .un8PRDivider  = 8,
        .un8SamplePoint= 9,
        .un8MinCsLow   = 8,
        .un8MinCsHigh  = 11,
        .un8HoldTime   = 7
    };

    EX_QSPI_Port();

    HAL_QSPI_Init(QSPI_ID_0);
    HAL_QSPI_SetConfig(QSPI_ID_0, &tCfg);
    HAL_QSPI_SetIRQ(QSPI_ID_0, TC_TEST_OPS, EX_QSPI_IRQHandler, (void*)&s_tQspiIrqCtx, 3);

    EX_QSPI_ReadID(QSPI_ID_0);
    EX_QSPI_ReadConfigueStatus(QSPI_ID_0);
    EX_QSPI_QuadEnable(QSPI_ID_0, false);

    uint8_t un8data[16] = {0,};

#if defined(TC_QSPI_EXAMPLE_INTR)
    LOG("\n --------- Start QSPI Interrupt example -------\n");
    LOG(" << Single Sector Erase/Read >>\r\n");
    EX_QSPI_Program(QSPI_ID_0, MX25_CMD_SE4K, 0, 0, NULL);
    PRV_QSPI_Transfer(QSPI_ID_0, MX25_CMD_READ, true, 0, 16, un8data, QSPI_DIR_INPUT);
    LOG_HEX(un8data, 16);

    LOG(" << Single Page Program >>\r\n");
    for (int i = 0; i < 16; i++)
    {
        un8data[i] = i;
    }
    EX_QSPI_QuadEnable(QSPI_ID_0, false);
    EX_QSPI_Program(QSPI_ID_0, MX25_CMD_PP, 0, 16, un8data);
    LOG_HEX(un8data, 16);

    LOG(" << Single Page Read >>\r\n");
    memset(un8data, 0, 16);
    PRV_QSPI_Transfer(QSPI_ID_0, MX25_CMD_READ, true, 0, 16, un8data, QSPI_DIR_INPUT);
    LOG_HEX(un8data, 16);

    LOG(" << Single Sector Erase/Read >>\r\n");
    EX_QSPI_Program(QSPI_ID_0, MX25_CMD_SE4K, 0, 0, NULL);
    PRV_QSPI_Transfer(QSPI_ID_0, MX25_CMD_READ, true, 0, 16, un8data, QSPI_DIR_INPUT);
    LOG_HEX(un8data, 16);

    LOG(" << Quad Page Program >>\r\n");
    for (int i = 0; i < 16; i++)
    {
        un8data[i] = (i << 4) | i;
    }
    EX_QSPI_QuadEnable(QSPI_ID_0, true);
    EX_QSPI_Program(QSPI_ID_0, MX25_CMD_QPP, 0, 16, un8data);
    LOG_HEX(un8data, 16);

    LOG(" << Quad Page Read >>\r\n");
    memset(un8data, 0, 16);
    PRV_QSPI_Transfer(QSPI_ID_0, MX25_CMD_QREAD, true, 0, 16, un8data, QSPI_DIR_INPUT);
    LOG_HEX(un8data, 16);

    LOG(" << Single Page Read >>\r\n");
    memset(un8data, 0, 16);
    EX_QSPI_QuadEnable(QSPI_ID_0, false);
    PRV_QSPI_Transfer(QSPI_ID_0, MX25_CMD_READ, true, 0, 16, un8data, QSPI_DIR_INPUT);
    LOG_HEX(un8data, 16);
#endif

#if defined(TC_QSPI_EXAMPLE_DMA)
    LOG("\n ------ Start QSPI DMA Interrupt example ------\n");
    LOG(" << Single Sector Erase/Read >>\r\n");
    EX_QSPI_Program(QSPI_ID_0, MX25_CMD_SE4K, 0, 0, NULL);
    PRV_QSPI_TransferDma(QSPI_ID_0, MX25_CMD_READ, true, 0, 16, un8data, QSPI_DIR_INPUT);
    LOG_HEX(un8data, 16);

    LOG(" << Single Page Program >>\r\n");
    for (uint32_t i = 0; i < 16; i++)
    {
        un8data[i] = (uint8_t)i;
    }
    LOG_HEX(un8data, 16);
    EX_QSPI_QuadEnable(QSPI_ID_0, false);
    SwapEndian32PerWord(un8data, 16);
    EX_QSPI_ProgramDma(QSPI_ID_0, MX25_CMD_PP, 0, 16, un8data);

    LOG(" << Single Page Read >>\r\n");
    memset(un8data, 0, 16);
    PRV_QSPI_TransferDma(QSPI_ID_0, MX25_CMD_READ, true, 0, 16, un8data, QSPI_DIR_INPUT);
    SwapEndian32PerWord(un8data, 16);
    LOG_HEX(un8data, 16);

    LOG(" << Single Sector Erase/Read >>\r\n");
    EX_QSPI_Program(QSPI_ID_0, MX25_CMD_SE4K, 0, 0, NULL);
    PRV_QSPI_TransferDma(QSPI_ID_0, MX25_CMD_READ, true, 0, 16, un8data, QSPI_DIR_INPUT);
    LOG_HEX(un8data, 16);

    LOG(" << Quad Page Program >>\r\n");
    for (int i = 0; i < 16; i++)
    {
        un8data[i] = (i << 4) | i;
    }
    LOG_HEX(un8data, 16);
    EX_QSPI_QuadEnable(QSPI_ID_0, true);
    SwapEndian32PerWord(un8data, 16);
    EX_QSPI_ProgramDma(QSPI_ID_0, MX25_CMD_QPP, 0, 16, un8data);

    LOG(" << Quad Page Read >>\r\n");
    memset(un8data, 0, 16);
    PRV_QSPI_TransferDma(QSPI_ID_0, MX25_CMD_QREAD, true, 0, 16, un8data, QSPI_DIR_INPUT);
    SwapEndian32PerWord(un8data, 16);
    LOG_HEX(un8data, 16);

    LOG(" << Single Page Read >>\r\n");
    memset(un8data, 0, 16);
    EX_QSPI_QuadEnable(QSPI_ID_0, false);
    PRV_QSPI_TransferDma(QSPI_ID_0, MX25_CMD_READ, true, 0, 16, un8data, QSPI_DIR_INPUT);
    SwapEndian32PerWord(un8data, 16);
    LOG_HEX(un8data, 16);
#endif

    /* main loop */
    while(1)
    {
        SYSTICK_Wait(1000);
    };

}
