/**
 *******************************************************************************
 * @file        usart_uart_tx.c
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
 * - This example demonstrates how to set up USART in UART mode and handle transmission completion in the interrupt service routine.
 * - The transmission completion event is printed in the interrupt handler.
 * 
 * Connection:
 * - PB0(TXD10) - PB1(RXD10)
 * - PB1(RXD10) - PB0(TXD10)
*/

#include "abov_config.h"
#include "abov_simpleapp_config.h"

#include "hal_pcu.h"
#include "hal_usart.h"

#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"

#if (CONFIG_APP_USART == 1)

static uint8_t s_un8TxData[8] = {0xa5, 0x5a, 0xa5, 0x5a, 0x05, 0x07, 0x09, 0xa0};

static void USART_IRQHandler(uint32_t un32Event, void *pContext)
{
    if (un32Event & USART_EVENT_TX_DONE)
    {
        LOG("Tx Done\n");
    }
}

void USART_UART_Tx(void)
{
    HAL_ERR_e eErr = HAL_ERR_OK;

    USART_CFG_t tCfg =
    {
        .eMode = USART_MODE_UART,
        .un32BaudRate = 38400,
        .tCfg.tUart.eData = USART_DATA_8,
        .tCfg.tUart.eParity = USART_PARITY_NONE,
        .tCfg.tUart.eStop = USART_STOP_1,
        .tCfg.tUart.bDoubleSpeed = false
    };

    eErr = HAL_PCU_SetAltMode((PCU_ID_e)USART0_RX_PORT, (PCU_PIN_ID_e)USART0_RX_PORT_ID, (PCU_ALT_e)USART0_RX_MUX_ID);
    if (eErr != HAL_ERR_OK)
    {
        return;
    }

    eErr = HAL_PCU_SetAltMode((PCU_ID_e)USART0_TX_PORT, (PCU_PIN_ID_e)USART0_TX_PORT_ID, (PCU_ALT_e)USART0_TX_MUX_ID);
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

    eErr = HAL_USART_Transmit(USART_ID_0, s_un8TxData, sizeof(s_un8TxData), false);
    if (eErr != HAL_ERR_OK)
    {
        return;
    }

    LOG("Wait Tx...\n");
}
#endif
