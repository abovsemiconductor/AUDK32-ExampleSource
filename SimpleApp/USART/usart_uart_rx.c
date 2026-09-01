/**
 *******************************************************************************
 * @file        usart_uart_rx.c
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
 * - This example demonstrates how to set up USART in UART mode and handle received data in the interrupt service routine.
 * - The received data is printed in the interrupt handler.
 * 
 * Connection:
 * - PB0(TXD10) - PB1(RXD10)
 * - PB1(RXD10) - PB0(TXD10)
*/

#include "abov_config.h"
#include "abov_simpleapp_config.h"

#include "hal_pcu.h"
#include "hal_usart.h"

#include "debug_serial.h"
#include "debug_log.h"
#include "debug.h"

#if (CONFIG_APP_USART == 1)

#if defined(USART1_TX_PORT) || defined(USART1_RX_PORT)
#define USART_ID                USART_ID_1
#define USART_TX_PORT           USART1_TX_PORT
#define USART_TX_PORT_ID        USART1_TX_PORT_ID
#define USART_TX_MUX_ID         USART1_TX_MUX_ID
#define USART_RX_PORT           USART1_RX_PORT
#define USART_RX_PORT_ID        USART1_RX_PORT_ID
#define USART_RX_MUX_ID         USART1_RX_MUX_ID
#else
#define USART_ID                USART_ID_0
#define USART_TX_PORT           USART0_TX_PORT
#define USART_TX_PORT_ID        USART0_TX_PORT_ID
#define USART_TX_MUX_ID         USART0_TX_MUX_ID
#define USART_RX_PORT           USART0_RX_PORT
#define USART_RX_PORT_ID        USART0_RX_PORT_ID
#define USART_RX_MUX_ID         USART0_RX_MUX_ID
#endif

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

void USART_UART_Rx(void)
{
    LOG("Receive USART with uart mode.\n");

    char ch;
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

    eErr = HAL_PCU_SetAltMode((PCU_ID_e)USART_RX_PORT, (PCU_PIN_ID_e)USART_RX_PORT_ID, (PCU_ALT_e)USART_RX_MUX_ID);
    if (eErr != HAL_ERR_OK)
    {
        return;
    }

    eErr = HAL_PCU_SetAltMode((PCU_ID_e)USART_TX_PORT, (PCU_PIN_ID_e)USART_TX_PORT_ID, (PCU_ALT_e)USART_TX_MUX_ID);
    if (eErr != HAL_ERR_OK)
    {
        return;
    }

    /* Initialize instance */
    eErr = HAL_USART_Init(USART_ID);
    if (eErr != HAL_ERR_OK)
    {
        return;
    }

    /* Set up operation parameters */
    eErr = HAL_USART_SetConfig(USART_ID, &tCfg);
    if (eErr != HAL_ERR_OK)
    {
        return;
    }

    /* Set IRQ priority and enable interrupt */
    eErr = HAL_USART_SetIRQ(USART_ID, USART_OPS_INTR, USART_IRQHandler, NULL, 3);
    if (eErr != HAL_ERR_OK)
    {
        return;
    }

    LOG("Press 'r' to receive data from the transmitter.\n");

    do {
      ch = serial_getc(NULL);
    } while(ch != 'r');

    eErr = HAL_USART_Receive(USART_ID, s_un8RxData, sizeof(s_un8RxData), false);
    if (eErr != HAL_ERR_OK)
    {
        return;
    }

    LOG("Wait Rx...\n");
}
#endif
