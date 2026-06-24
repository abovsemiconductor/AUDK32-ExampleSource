/**
 *******************************************************************************
 * @file        eeprom_24lc512.c
 * @author      ABOV R&D Division
 * @brief       EXAMPLE 24LC512 EEPROM Driver Interface
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

#include "hal_pcu.h"
#include "hal_i2c.h"

#include "eeprom_24lc512.h"

#include "debug_log.h"
#include "debug.h"

#define EEPROM_24LC512_SLAVE_ADDR   0x50
#define EEPROM_24LC512_PAGE_SIZE    128

static volatile bool s_bE2pTxRxComplete = false;

static void PRV_EEPROM_IRQHandler(uint32_t un32Event, void *pContext)
{
    if(un32Event & I2C_EVENT_TX_DONE)
    {
        s_bE2pTxRxComplete = true;
    }

    if(un32Event & I2C_EVENT_RX_DONE)
    {
        s_bE2pTxRxComplete = true;
    }
}

HAL_ERR_e EX_EEPROM_Init(void)
{
    I2C_CFG_t tI2cCfg =
    {
        .eMode= I2C_MODE_MASTER,
        .un8OwnSlvAddr = 0,
        .un8OwnSlvAddr2 = 0,
        .bSaGcEnable = false,
        .bSa2GcEnable = false,
        .uPeriod.tFreq.un32Freq = 400000,
        .tSdht.bEnable = false,
    };
    HAL_ERR_e eErr;

    HAL_PCU_SetAltMode((PCU_ID_e)E2PROM_SCL_PORT, (PCU_PIN_ID_e)E2PROM_SCL_PIN_ID, (PCU_ALT_e)E2PROM_SCL_MUX_ID);
    HAL_PCU_SetInOutMode((PCU_ID_e)E2PROM_SCL_PORT, (PCU_PIN_ID_e)E2PROM_SCL_PIN_ID, PCU_INOUT_OUTPUT_PUSH_PULL);

    HAL_PCU_SetAltMode((PCU_ID_e)E2PROM_SDA_PORT, (PCU_PIN_ID_e)E2PROM_SDA_PIN_ID, (PCU_ALT_e)E2PROM_SDA_MUX_ID);
    HAL_PCU_SetInOutMode((PCU_ID_e)E2PROM_SDA_PORT, (PCU_PIN_ID_e)E2PROM_SDA_PIN_ID, PCU_INOUT_OUTPUT_PUSH_PULL);

    eErr = HAL_I2C_Init(I2C_ID_2);
    if(eErr != HAL_ERR_OK)
    {
        return eErr;
    }

    eErr = HAL_I2C_SetConfig(I2C_ID_2, &tI2cCfg);
    if(eErr != HAL_ERR_OK)
    {
        return eErr;
    }

    eErr = HAL_I2C_SetIRQ(I2C_ID_2, I2C_OPS_INTR, PRV_EEPROM_IRQHandler, NULL, 0);
    if(eErr != HAL_ERR_OK)
    {
        return eErr;
    }

    return HAL_ERR_OK;
}

HAL_ERR_e EX_EEPROM_Read(uint32_t un32Addr, uint8_t *pun8Buf, uint32_t un32Len)
{
    HAL_ERR_e eErr;
    uint8_t un8Addr[2];

    s_bE2pTxRxComplete = false;

    un8Addr[0] = un32Addr >> 8;
    un8Addr[1] = un32Addr & 0xff;

    eErr = HAL_I2C_Transmit(I2C_ID_2, EEPROM_24LC512_SLAVE_ADDR, un8Addr, 2, false);
    if(eErr != HAL_ERR_OK)
    {
        LOG("HAL_I2C_Transmit(), (%d)\n", eErr);
        return eErr;
    }

    while(s_bE2pTxRxComplete == false);

    s_bE2pTxRxComplete = false;

    eErr = HAL_I2C_Receive(I2C_ID_2, EEPROM_24LC512_SLAVE_ADDR, pun8Buf, un32Len, false);
    if(eErr != HAL_ERR_OK)
    {
        LOG("HAL_I2C_Receive(), (%d)\n", eErr);
        return eErr;
    }

    while(s_bE2pTxRxComplete == false);

    return HAL_ERR_OK;
}

HAL_ERR_e EX_EEPROM_Write(uint32_t un32Addr, uint8_t *pun8Buf, uint32_t un32Len)
{
    HAL_ERR_e eErr;
    uint8_t un8Write[2 + EEPROM_24LC512_PAGE_SIZE];

    if(un32Len > EEPROM_24LC512_PAGE_SIZE)
    {
        return HAL_ERR_PARAMETER;
    }

    s_bE2pTxRxComplete = false;

    un8Write[0] = un32Addr >> 8;
    un8Write[1] = un32Addr & 0xff;

    for(int i = 0 ; i < un32Len ; i++)
        un8Write[2 + i] = pun8Buf[i];

    eErr = HAL_I2C_Transmit(I2C_ID_2, EEPROM_24LC512_SLAVE_ADDR, un8Write, 2 + un32Len, false);
    if(eErr != HAL_ERR_OK)
    {
        LOG("HAL_I2C_Transmit(), (%d)\n", eErr);
        return eErr;
    }

    while(s_bE2pTxRxComplete == false);
    return HAL_ERR_OK;
}

/**********************************************************************
 * @brief       Test API
 * @param[in]   None
 * @return      0 : No error, Non-Zero : Any error
 **********************************************************************/
void EX_EEPROM_Test(void)
{
    uint8_t un8Buffer[64];

    for(int i = 0 ; i < 64 ; i++)
        un8Buffer[i] = i;
    EX_EEPROM_Write(0x100, un8Buffer, 64);

    for(int i = 0 ; i < 64 ; i++)
        un8Buffer[i] = 0x00;

    EX_EEPROM_Read(0x100, un8Buffer, 64);

    LOG("R/W\n");
    for(int i = 0 ; i < 64 ; i++) {
        if( i && (i % 16 == 0))
            LOG("\n");
        LOG("%02x ", un8Buffer[i]);
    }
    LOG("\n");
}
