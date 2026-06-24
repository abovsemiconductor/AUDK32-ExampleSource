/**
 *******************************************************************************
 * @file        usart_spi_rx.c
 * @author      ABOV R&D Division
 * @brief       Simple Application for USART peripheral
 *
 * Copyright 2026 ABOV Semiconductor Co.,Ltd. All rights reserved.
 *
 * This file is licensed under terms that are found in the LICENSE file
 * located at Document directory.
 * If this file is delivered or shared without applicable license terms,
 * the terms of the BSD-3-Clause license shall be applied.
 * Reference: https://opensource.org/licenses/BSD-3-Clause
 ******************************************************************************/

/*
 * Readme:
 * - This example demonstrates how to set up USART in SPI mode and handle received data in the interrupt service routine.
 * - The received data is printed in the interrupt handler.
 * 
 * Connection:
 * - PB3(SS10) - PB3(SS10)
 * - PB2(SCK10) - PB2(SCK10)
 * - PB1(MISO10) - PB1(MISO10
 * - PB0(MOSI10) - PB0(MOSI10)
 * Connection: Swap MISO and MOSI for master and slave communication
 * - PB3(SS10) - PB3(SS10)
 * - PB2(SCK10) - PB2(SCK10)
 * - PB1(MISO10) - PB0(MOSI10)
 * - PB0(MOSI10) - PB1(MISO10)
 * 
 * Note:
 * - In USART test,
 * - The debug port must be configured as UART.
 * - UART RX pin: PF1
 * - UART TX pin: PF0
*/

#include "abov_config.h"
#include "abov_simpleapp_config.h"

#include "hal_pcu.h"
#include "hal_usart.h"

#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"

#if (CONFIG_APP_USART == 1)

static uint8_t s_un8RxData[8];

static void USART_IRQHandler(uint32_t un32Event, void *pContext)
{
    if (un32Event & USART_EVENT_RX_DONE)
    {
        for(uint32_t i = 0; i < sizeof(s_un8RxData); i++)
        {
            LOG("Rx Data=0x%x\n", s_un8RxData[i]);
        }

        LOG("Rx Done\n");
    }
}

void USART_SPI_Rx(void)
{
    HAL_ERR_e eErr = HAL_ERR_OK;

    USART_CFG_t tCfg =
    {
        .eMode = USART_MODE_SPI,
        .un32BaudRate = 12,
        .tCfg.tSpi.eMs = USART_MS_SLAVE,
        .tCfg.tSpi.eBitOrder = USART_BIT_ORDER_MSB,
        .tCfg.tSpi.eClkPol = USART_CLKPOL_TXD_RISE_RXD_FALL,
        .tCfg.tSpi.eClkPha = USART_CLKPHA_SAMPLE,
        .tCfg.tSpi.bSwapMSPort = false,
        .tCfg.tSpi.bRxSCKGen = false,
        .tCfg.tSpi.bSlvRecvDisable = false
    };

    eErr = HAL_PCU_SetAltMode((PCU_ID_e)USART0_SS_PORT, (PCU_PIN_ID_e)USART0_SS_PORT_ID, (PCU_ALT_e)USART0_SS_MUX_ID);
    if (eErr != HAL_ERR_OK)
    {
        return;
    }

    eErr = HAL_PCU_SetAltMode((PCU_ID_e)USART0_SCK_PORT, (PCU_PIN_ID_e)USART0_SCK_PORT_ID, (PCU_ALT_e)USART0_SCK_MUX_ID);
    if (eErr != HAL_ERR_OK)
    {
        return;
    }

    eErr = HAL_PCU_SetAltMode((PCU_ID_e)USART0_MOSI_PORT, (PCU_PIN_ID_e)USART0_MOSI_PORT_ID, (PCU_ALT_e)USART0_MOSI_MUX_ID);
    if (eErr != HAL_ERR_OK)
    {
        return;
    }

    eErr = HAL_PCU_SetAltMode((PCU_ID_e)USART0_MISO_PORT, (PCU_PIN_ID_e)USART0_MISO_PORT_ID, (PCU_ALT_e)USART0_MISO_MUX_ID);
    if (eErr != HAL_ERR_OK)
    {
        return;
    }

    /* Initialize instance */
    eErr = HAL_USART_Init(USART_ID_0);
    if (eErr != HAL_ERR_OK)
    {
        return;
    }

    /* Set up operation parameters */
    eErr = HAL_USART_SetConfig(USART_ID_0, &tCfg);
    if (eErr != HAL_ERR_OK)
    {
        return;
    }

    /* Set IRQ priority and enable interrupt */
    eErr = HAL_USART_SetIRQ(USART_ID_0, USART_OPS_INTR, USART_IRQHandler, NULL, 3);
    if (eErr != HAL_ERR_OK)
    {
        return;
    }

    eErr = HAL_USART_Receive(USART_ID_0, s_un8RxData, sizeof(s_un8RxData), false);
    if (eErr != HAL_ERR_OK)
    {
        return;
    }

    LOG("Wait Rx...\n");
}
#endif
