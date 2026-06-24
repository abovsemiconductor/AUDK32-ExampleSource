/**
 *******************************************************************************
 * @file        i2c_rx.c
 * @author      ABOV R&D Division
 * @brief       Simple Application for I2C peripheral
 *
 * Copyright 2026 ABOV Semiconductor Co.,Ltd. All rights reserved.
 *
 * This file is licensed under terms that are found in the LICENSE file
 * located at Document directory.
 * If this file is delivered or shared without applicable license terms,
 * the terms of the BSD-3-Clause license shall be applied.
 * Reference: https://opensource.org/licenses/BSD-3-Clause
 ******************************************************************************/

/**
 * Readme:
 * This example demonstrates how to use I2C in master receive mode.
 * The received data is printed in the debug log.
 * 
 * Connection:
 * - PD0(SCL0) - PD0(SCL0) to I2C slave device
 * - PD1(SDA0) - PD1(SDA0) to I2C slave device
 */

#include "abov_config.h"
#include "abov_simpleapp_config.h"

#include "hal_pcu.h"
#include "hal_i2c.h"

#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"

#if (CONFIG_APP_I2C == 1)

#define I2C0_SLAVE_ADDR       0x49

static uint8_t s_un8RxData[8];

static void I2C_IRQHandler(uint32_t un32Event, void *pContext)
{
    if(un32Event & I2C_EVENT_RX_DONE)
    {
        for(uint32_t i = 0; i < sizeof(s_un8RxData); i++)
        {
            LOG("Rx Data=0x%x\n", s_un8RxData[i]);
        }

        LOG("Rx Done\n");
    }
}

void I2C_Rx(void)
{
    HAL_ERR_e eErr = HAL_ERR_OK;

    I2C_CFG_t tCfg =
    {
        .eMode = I2C_MODE_MASTER,
        .uPeriod.tFreq.un32Freq = 100000,
        .un8OwnSlvAddr = I2C0_SLAVE_ADDR,
        .tSdht.bEnable = false
    };

    eErr = HAL_PCU_SetAltMode((PCU_ID_e)I2C0_SCL_PORT, (PCU_PIN_ID_e)I2C0_SCL_PORT_ID, (PCU_ALT_e)I2C0_SCL_MUX_ID);
    if (eErr != HAL_ERR_OK)
    {
        LOG("HAL_PCU_SetAltMode() error, (%d)\n", eErr);
        return;
    }

    eErr = HAL_PCU_SetPullUpDown((PCU_ID_e)I2C0_SCL_PORT, (PCU_PIN_ID_e)I2C0_SCL_PORT_ID, PCU_PUPD_UP);
    if (eErr != HAL_ERR_OK)
    {
        LOG("HAL_PCU_SetPullUpDown() error, (%d)\n", eErr);
        return;
    }

    eErr = HAL_PCU_SetAltMode((PCU_ID_e)I2C0_SDA_PORT, (PCU_PIN_ID_e)I2C0_SDA_PORT_ID, (PCU_ALT_e)I2C0_SDA_MUX_ID);
    if (eErr != HAL_ERR_OK)
    {
        LOG("HAL_PCU_SetAltMode() error, (%d)\n", eErr);
        return;
    }

    eErr = HAL_PCU_SetPullUpDown((PCU_ID_e)I2C0_SDA_PORT, (PCU_PIN_ID_e)I2C0_SDA_PORT_ID, PCU_PUPD_UP);
    if (eErr != HAL_ERR_OK)
    {
        LOG("HAL_PCU_SetPullUpDown() error, (%d)\n", eErr);
        return;
    }

    /* Initialize instance */
    eErr = HAL_I2C_Init(I2C_ID_0);
    if (eErr != HAL_ERR_OK)
    {
        LOG("HAL_I2C_Init() error, (%d)\n", eErr);
        return;
    }

    /* Set up operation parameters */
    eErr = HAL_I2C_SetConfig(I2C_ID_0, &tCfg);
    if (eErr != HAL_ERR_OK)
    {
        LOG("HAL_I2C_SetConfig() error, (%d)\n", eErr);
        return;
    }

    /* Set IRQ priority and enable interrupt */
    eErr = HAL_I2C_SetIRQ(I2C_ID_0, I2C_OPS_INTR, I2C_IRQHandler, NULL, 3);
    if (eErr != HAL_ERR_OK)
    {
        LOG("HAL_I2C_SetIRQ() error, (%d)\n", eErr);
        return;
    }

    eErr = HAL_I2C_Receive(I2C_ID_0, I2C0_SLAVE_ADDR, s_un8RxData, sizeof(s_un8RxData), false);
    if (eErr != HAL_ERR_OK)
    {
        LOG("HAL_I2C_Receive() error, (%d)\n", eErr);
        return;
    }

    LOG("Wait Rx...\n");
}
#endif
