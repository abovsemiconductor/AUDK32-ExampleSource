/**
 *******************************************************************************
 * @file        usart_usrt_tx.c
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
 * - This example demonstrates how to set up USART in USRT mode and handle transmission completion in the interrupt service routine.
 * - The transmission completion event is printed in the interrupt handler.
 * 
 * Connection:
 * - PB2(SCK10) - PB2(SCK10)
 * - PB1(TXD10) - PB0(RXD10)
 * - PB0(RXD10) - PB1(TXD10)
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
#define USART_SCK_PORT          USART1_SCK_PORT
#define USART_SCK_PORT_ID       USART1_SCK_PORT_ID
#define USART_SCK_MUX_ID        USART1_SCK_MUX_ID
#define USART_TX_PORT           USART1_TX_PORT
#define USART_TX_PORT_ID        USART1_TX_PORT_ID
#define USART_TX_MUX_ID         USART1_TX_MUX_ID
#define USART_RX_PORT           USART1_RX_PORT
#define USART_RX_PORT_ID        USART1_RX_PORT_ID
#define USART_RX_MUX_ID         USART1_RX_MUX_ID
#else
#define USART_ID                USART_ID_0
#define USART_SCK_PORT          USART0_SCK_PORT
#define USART_SCK_PORT_ID       USART0_SCK_PORT_ID
#define USART_SCK_MUX_ID        USART0_SCK_MUX_ID
#define USART_TX_PORT           USART0_TX_PORT
#define USART_TX_PORT_ID        USART0_TX_PORT_ID
#define USART_TX_MUX_ID         USART0_TX_MUX_ID
#define USART_RX_PORT           USART0_RX_PORT
#define USART_RX_PORT_ID        USART0_RX_PORT_ID
#define USART_RX_MUX_ID         USART0_RX_MUX_ID
#endif

static uint8_t s_un8TxData[8] = {0xa5, 0x5a, 0xa5, 0x5a, 0x05, 0x07, 0x09, 0xa0};

static void USART_IRQHandler(uint32_t un32Event, void *pContext)
{
    if (un32Event & USART_EVENT_TX_DONE)
    {
        LOG("Tx Done\n");
    }
}

void USART_USRT_Tx(bool bTxMode)
{
    LOG("Transmit USART with usrt mode.\n");

    char ch;
    HAL_ERR_e eErr = HAL_ERR_OK;

    USART_CFG_t tCfg =
    {
        .eMode = USART_MODE_USRT,
        .un32BaudRate = 38400,
        .tCfg.tUsrt.eMs = USART_MS_MASTER,
        .tCfg.tUsrt.eData = USART_DATA_8,
        .tCfg.tUsrt.eParity = USART_PARITY_NONE,
        .tCfg.tUsrt.eStop = USART_STOP_1,
        .tCfg.tUsrt.eClkPol = USART_CLKPOL_TXD_RISE_RXD_FALL
    };

    eErr = HAL_PCU_SetAltMode((PCU_ID_e)USART_SCK_PORT, (PCU_PIN_ID_e)USART_SCK_PORT_ID, (PCU_ALT_e)USART_SCK_MUX_ID);
    if (eErr != HAL_ERR_OK)
    {
        return;
    }

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

    LOG("Press 't' to transmit data to the receiver.\n");

    do {
      ch = serial_getc(NULL);
    } while(ch != 't');

    /* Set IRQ priority and enable interrupt */
    eErr = HAL_USART_SetIRQ(USART_ID, USART_OPS_INTR, USART_IRQHandler, NULL, 3);
    if (eErr != HAL_ERR_OK)
    {
        return;
    }

    eErr = HAL_USART_Transmit(USART_ID, s_un8TxData, sizeof(s_un8TxData), false);
    if (eErr != HAL_ERR_OK)
    {
        return;
    }

    LOG("Wait Tx...\n");
}
#endif
