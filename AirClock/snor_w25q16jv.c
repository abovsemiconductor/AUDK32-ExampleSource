/**
 *******************************************************************************
 * @file        snor_w25q16jv.c
 * @author      ABOV R&D Division
 * @brief       EXAMPLE W25Q16JV SPI NOR Driver Interface
 *
 * Copyright 2025 ABOV Semiconductor Co.,Ltd. All rights reserved.
 *
 * This file is licensed under terms that are found in the LICENSE file
 * located at Document directory.
 * If this file is delivered or shared without applicable license terms,
 * the terms of the BSD-3-Clause license shall be applied.
 * Reference: https://opensource.org/licenses/BSD-3-Clause
 ******************************************************************************/

#include "abov_config.h"

#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"

#include "snor_w25q16jv.h"

#include "hal_pcu.h"
#include "hal_usart.h"

#define W25Q16JV_MANUFACTURE_DEVICE_ID_CMD  0x90
#define W25Q16JV_WRITE_ENABLE_CMD           0x06
#define W25Q16JV_WRITE_DISABLE_CMD          0x04
#define W25Q16JV_READ_STATUS_CMD            0x05
#define W25Q16JV_READ_CMD                   0x03
#define W25Q16JV_PROGRAM_CMD                0x02
#define W25Q16JV_SECTOR_ERASE_4K_CMD        0x20
#define W25Q16JV_BLOCK_ERASE_32K_CMD        0x52
#define W25Q16JV_BLOCK_ERASE_64K_CMD        0xD8
#define W25Q16JV_CHIP_ERASE_CMD             0xC7

#define SNOR_DEFAULT_TIMEOUT                1000

#define SNOR_ADDRESS_24_MASK                0x00FF0000
#define SNOR_ADDRESS_16_MASK                0x0000FF00
#define SNOR_ADDRESS_8_MASK                 0x000000FF

static bool s_bSNorTxDone = false;
static bool s_bSNorRxDone = false;

static void PRV_EX_SNOR_Dump(const uint8_t *pun8Str, const uint8_t *pun8Buf, uint32_t un32Len)
{
    LOG("%s", pun8Str);
    for(int i = 0 ; i < un32Len ; i++)
    {
        if(i % 16 == 0)
            LOG("\n");
        LOG("%02x ", pun8Buf[i]);
    }
    LOG("\n");
}

static void PRV_EX_SNOR_IRQHandler(uint32_t un32Event, void *pContext)
{
    if (un32Event & USART_EVENT_TX_DONE)
    {
        s_bSNorTxDone = true;
    }

    if (un32Event & USART_EVENT_RX_DONE) 
    {
        s_bSNorRxDone = true;
    }
}

static void PRV_EX_SNOR_Write(uint8_t *pun8Dout, uint32_t un32Size, uint32_t un32Timeout)
{
    HAL_USART_Transmit(USART_ID_1, pun8Dout, un32Size, false);
    while(un32Timeout)
    {
        if(s_bSNorTxDone)
            break;
        un32Timeout--;
    }
    s_bSNorTxDone = false;
}

static void PRV_EX_SNOR_Read(uint8_t *pun8Din, uint32_t un32Size, uint32_t un32Timeout)
{
    HAL_USART_Receive(USART_ID_1, pun8Din, un32Size, false);
    while(un32Timeout)
    {
        if(s_bSNorRxDone)
            break;
        un32Timeout--;
    }
    s_bSNorRxDone = false;
}

static void PRV_EX_SNOR_GetBusy(uint32_t un32Timeout)
{
    uint8_t un8Cmd = W25Q16JV_READ_STATUS_CMD;
    uint8_t un8Sta;

    while(un32Timeout)
    {
        HAL_PCU_SetOutputBit((PCU_ID_e)FLASH_SS_PORT, (PCU_PIN_ID_e)FLASH_SS_PIN_ID, PCU_OUTPUT_BIT_CLEAR);
        PRV_EX_SNOR_Write(&un8Cmd, 1, SNOR_DEFAULT_TIMEOUT);
        PRV_EX_SNOR_Read(&un8Sta, 1, SNOR_DEFAULT_TIMEOUT);
        HAL_PCU_SetOutputBit((PCU_ID_e)FLASH_SS_PORT, (PCU_PIN_ID_e)FLASH_SS_PIN_ID, PCU_OUTPUT_BIT_SET);

        if(un8Sta & 0x01)  /* Check Busy bit */
        {
            SystemDelayMS(1);
        }
        else
        {
            break;
        }
        un32Timeout--;
    }
}

static void PRV_EX_SNOR_SetWriteEnable(bool bEnable)
{
    uint8_t un8Cmd;

    if (bEnable == true)
       un8Cmd = W25Q16JV_WRITE_ENABLE_CMD;
    else
       un8Cmd = W25Q16JV_WRITE_DISABLE_CMD;

    HAL_PCU_SetOutputBit((PCU_ID_e)FLASH_SS_PORT, (PCU_PIN_ID_e)FLASH_SS_PIN_ID, PCU_OUTPUT_BIT_CLEAR);
    PRV_EX_SNOR_Write(&un8Cmd, 1, SNOR_DEFAULT_TIMEOUT);
    HAL_PCU_SetOutputBit((PCU_ID_e)FLASH_SS_PORT, (PCU_PIN_ID_e)FLASH_SS_PIN_ID, PCU_OUTPUT_BIT_SET);
}

void EX_SNOR_Init(void)
{
    USART_CFG_t tCfg = {
        .eMode = USART_MODE_SPI,
        .tCfg.tSpi.eMs = USART_MS_MASTER,
        .tCfg.tSpi.eBitOrder = USART_BIT_ORDER_MSB,
        .tCfg.tSpi.eClkPol = USART_CLKPOL_TXD_FALL_RXD_RISE,
        .tCfg.tSpi.eClkPha = USART_CLKPHA_SETUP,
        .tCfg.tSpi.bRxSCKGen = true,
        .tCfg.tSpi.bSSGenDisable = true,
        .un32BaudRate = 3 /* 32MHz / 2(3 + 1) = 4MHz */
    };

    HAL_USART_Init(USART_ID_1);
    HAL_USART_SetConfig(USART_ID_1, &tCfg);
    HAL_USART_SetIRQ(USART_ID_1, USART_OPS_INTR, PRV_EX_SNOR_IRQHandler, NULL, 0);

    HAL_PCU_SetAltMode((PCU_ID_e)FLASH_SS_PORT, (PCU_PIN_ID_e)FLASH_SS_PIN_ID, (PCU_ALT_e)PCU_ALT_0);
    HAL_PCU_SetAltMode((PCU_ID_e)FLASH_SCK_PORT, (PCU_PIN_ID_e)FLASH_SCK_PIN_ID, (PCU_ALT_e)FLASH_SCK_MUX_ID);
    HAL_PCU_SetAltMode((PCU_ID_e)FLASH_MOSI_PORT, (PCU_PIN_ID_e)FLASH_MOSI_PIN_ID, (PCU_ALT_e)FLASH_MOSI_MUX_ID);
    HAL_PCU_SetAltMode((PCU_ID_e)FLASH_MISO_PORT, (PCU_PIN_ID_e)FLASH_MISO_PIN_ID, (PCU_ALT_e)FLASH_MISO_MUX_ID);

    HAL_PCU_SetInOutMode((PCU_ID_e)FLASH_SS_PORT, (PCU_PIN_ID_e)FLASH_SS_PIN_ID, PCU_INOUT_OUTPUT_OPEN_DRAIN);
    HAL_PCU_SetPullUpDown((PCU_ID_e)FLASH_SS_PORT, (PCU_PIN_ID_e)FLASH_SS_PIN_ID, PCU_PUPD_UP);
    HAL_PCU_SetOutputBit((PCU_ID_e)FLASH_SS_PORT, (PCU_PIN_ID_e)FLASH_SS_PIN_ID, PCU_OUTPUT_BIT_SET);

    /* level shifter */
    HAL_PCU_SetInOutMode((PCU_ID_e)LEVEL_SHIFTER_PORT, (PCU_PIN_ID_e)LEVEL_SHIFTER_PIN_ID, PCU_INOUT_OUTPUT_PUSH_PULL);
    HAL_PCU_SetPullUpDown((PCU_ID_e)LEVEL_SHIFTER_PORT, (PCU_PIN_ID_e)LEVEL_SHIFTER_PIN_ID, PCU_PUPD_UP);
    HAL_PCU_SetOutputBit((PCU_ID_e)LEVEL_SHIFTER_PORT, (PCU_PIN_ID_e)LEVEL_SHIFTER_PIN_ID, PCU_OUTPUT_BIT_SET);
}

void EX_SNOR_GetID(uint8_t *pun8Id)
{
    uint8_t un8Cmd[4] = { W25Q16JV_MANUFACTURE_DEVICE_ID_CMD, 0x00, 0x00, 0x00 };
    // uint8_t un8ReadData[2];

    /* 1. CSpin (SSpin) Low */
    HAL_PCU_SetOutputBit((PCU_ID_e)FLASH_SS_PORT, (PCU_PIN_ID_e)FLASH_SS_PIN_ID, PCU_OUTPUT_BIT_CLEAR);
    /* 2. Send command */
    PRV_EX_SNOR_Write(un8Cmd, 4, SNOR_DEFAULT_TIMEOUT);
    /* 3. Read Data */
    PRV_EX_SNOR_Read(pun8Id, 2, SNOR_DEFAULT_TIMEOUT);
    /* 4. CSpin (SSpin) High */
    HAL_PCU_SetOutputBit((PCU_ID_e)FLASH_SS_PORT, (PCU_PIN_ID_e)FLASH_SS_PIN_ID, PCU_OUTPUT_BIT_SET);
}

void EX_SNOR_SetErase(uint32_t un32Addr, EX_SNOR_ERASE_e eErase)
{
    uint8_t un8Cmd;
    uint8_t un8Addr[3];

    un8Addr[0] = (uint8_t)((un32Addr & SNOR_ADDRESS_24_MASK) >> 16);
    un8Addr[1] = (uint8_t)((un32Addr & SNOR_ADDRESS_16_MASK) >> 8);
    un8Addr[2] = (uint8_t)(un32Addr & SNOR_ADDRESS_8_MASK);

    switch(eErase)
    {
        case EX_SNOR_ERASE_4K:
            un8Cmd = W25Q16JV_SECTOR_ERASE_4K_CMD;
            break;
        case EX_SNOR_ERASE_32K:
            un8Cmd = W25Q16JV_BLOCK_ERASE_32K_CMD;
            break;
        case EX_SNOR_ERASE_64K:
            un8Cmd = W25Q16JV_BLOCK_ERASE_64K_CMD;
            break;
        case EX_SNOR_ERASE_CHIP:
            un8Cmd = W25Q16JV_CHIP_ERASE_CMD;
            break;
        default: 
            un8Cmd = W25Q16JV_SECTOR_ERASE_4K_CMD;
            break;
    }

    /* 1. Write Enable */
    PRV_EX_SNOR_SetWriteEnable(true);

    /* 2. CSpin (SSpin) Low */
    HAL_PCU_SetOutputBit((PCU_ID_e)FLASH_SS_PORT, (PCU_PIN_ID_e)FLASH_SS_PIN_ID, PCU_OUTPUT_BIT_CLEAR);

    /* 3. Send command */
    PRV_EX_SNOR_Write(&un8Cmd, 1, SNOR_DEFAULT_TIMEOUT);

    if (eErase != EX_SNOR_ERASE_CHIP)
    {
        /* 4. Send address */
        PRV_EX_SNOR_Write(un8Addr, 3, SNOR_DEFAULT_TIMEOUT);
    }

    /* 6. CSpin (SSpin) High */
    HAL_PCU_SetOutputBit((PCU_ID_e)FLASH_SS_PORT, (PCU_PIN_ID_e)FLASH_SS_PIN_ID, PCU_OUTPUT_BIT_SET);

    /* 7. Check Busy */
    PRV_EX_SNOR_GetBusy(SNOR_DEFAULT_TIMEOUT);
}

void EX_SNOR_Write(uint32_t un32Addr, uint8_t *pun8Data, uint32_t un32Len)
{
    uint8_t un8Cmd = W25Q16JV_PROGRAM_CMD;
    uint8_t un8Addr[3];

    if (un32Len > 256)
        return;

    un8Addr[0] = (uint8_t)((un32Addr & SNOR_ADDRESS_24_MASK) >> 16);
    un8Addr[1] = (uint8_t)((un32Addr & SNOR_ADDRESS_16_MASK) >> 8);
    un8Addr[2] = (uint8_t)(un32Addr & SNOR_ADDRESS_8_MASK);

    /* 1. Write Enable */
    PRV_EX_SNOR_SetWriteEnable(true);

    /* 2. CSpin (SSpin) Low */
    HAL_PCU_SetOutputBit((PCU_ID_e)FLASH_SS_PORT, (PCU_PIN_ID_e)FLASH_SS_PIN_ID, PCU_OUTPUT_BIT_CLEAR);

    /* 3. Send command */
    PRV_EX_SNOR_Write(&un8Cmd, 1, SNOR_DEFAULT_TIMEOUT);
    /* 4. Send address */
    PRV_EX_SNOR_Write(un8Addr, 3, SNOR_DEFAULT_TIMEOUT);
    /* 5. Send Data */
    PRV_EX_SNOR_Write(pun8Data, un32Len, SNOR_DEFAULT_TIMEOUT);

    /* 6. CSpin (SSpin) High */
    HAL_PCU_SetOutputBit((PCU_ID_e)FLASH_SS_PORT, (PCU_PIN_ID_e)FLASH_SS_PIN_ID, PCU_OUTPUT_BIT_SET);

    /* 7. Check Busy */
    PRV_EX_SNOR_GetBusy(SNOR_DEFAULT_TIMEOUT);
}

void EX_SNOR_Read(uint32_t un32Addr, uint8_t *pun8Data, uint32_t un32Len)
{
    uint8_t un8Cmd = W25Q16JV_READ_CMD;
    uint8_t un8Addr[3];

    if (un32Len > 256)
        return;

    un8Addr[0] = (uint8_t)((un32Addr & SNOR_ADDRESS_24_MASK) >> 16);
    un8Addr[1] = (uint8_t)((un32Addr & SNOR_ADDRESS_16_MASK) >> 8);
    un8Addr[2] = (uint8_t)(un32Addr & SNOR_ADDRESS_8_MASK);

    /* 2. CSpin (SSpin) Low */
    HAL_PCU_SetOutputBit((PCU_ID_e)FLASH_SS_PORT, (PCU_PIN_ID_e)FLASH_SS_PIN_ID, PCU_OUTPUT_BIT_CLEAR);

    /* 3. Send command */
    PRV_EX_SNOR_Write(&un8Cmd, 1, SNOR_DEFAULT_TIMEOUT);
    /* 4. Send address */
    PRV_EX_SNOR_Write(un8Addr, 3, SNOR_DEFAULT_TIMEOUT);
    /* 5. Read data  */
    PRV_EX_SNOR_Read(pun8Data, un32Len, SNOR_DEFAULT_TIMEOUT);

    /* 6. CSpin (SSpin) Low */
    HAL_PCU_SetOutputBit((PCU_ID_e)FLASH_SS_PORT, (PCU_PIN_ID_e)FLASH_SS_PIN_ID, PCU_OUTPUT_BIT_SET);

    /* 7. Check Busy */
    PRV_EX_SNOR_GetBusy(SNOR_DEFAULT_TIMEOUT);
}

void EX_SNOR_Test(void)
{
    uint8_t un8DeviceId[3] = {0,};
    uint8_t un8Data[64] = {0,};

    EX_SNOR_GetID(un8DeviceId);
    PRV_EX_SNOR_Dump("Device ID", un8DeviceId, 2);

    EX_SNOR_Read(0, un8Data, 64);
    PRV_EX_SNOR_Dump("R", un8Data, 64);

    EX_SNOR_SetErase(0, EX_SNOR_ERASE_4K);

    EX_SNOR_Read(0, un8Data, 64);
    PRV_EX_SNOR_Dump("R/E", un8Data, 64);

    for(int i = 0 ; i < 64 ; i++)
        un8Data[i] = i;
    EX_SNOR_Write(0, un8Data, 64);

    for(int i = 0 ; i < 64 ; i++)
        un8Data[i] = 0;

    EX_SNOR_Read(0, un8Data, 64);
    PRV_EX_SNOR_Dump("R/W", un8Data, 64);
}
