/**
 *******************************************************************************
 * @file        uart_tx.c
 * @author      ABOV R&D Division
 * @brief       Simple Application for UART peripheral
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
 * - This example demonstrates how to set up UART in transmit mode and handle transmission completion in the interrupt service routine.
 * - The transmission completion event is printed in the interrupt handler.
 *
 * Connection:
 * - PF0(TX0) - PF1(RX1)
 * - PF1(RX1) - PF0(TX0)
 */

#include "abov_config.h"
#include "abov_simpleapp_config.h"

#include "hal_pcu.h"
#include "hal_uart.h"

#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"

#if (CONFIG_APP_UART == 1)

static uint8_t s_un8TxData[8] = {0xa5, 0x5a, 0xa5, 0x5a, 0x05, 0x07, 0x09, 0xa0};

static void UART_IRQHandler(uint32_t un32Event, void *pContext)
{
    if(un32Event & UART_EVENT_TX_DONE)
    {
        LOG("Tx Done\n");
    }
}

void UART_Tx(void)
{
    HAL_ERR_e eErr = HAL_ERR_OK;

    UART_CLK_CFG_t tClkCfg =
    {
        .eClk = UART_CLK_MCCR,
        .eMccr = UART_CLK_MCCR_MCLK,
        .un8MccrDiv = 2,
    };

    UART_CFG_t tCfg =
    {
        .un32BaudRate = 38400,
        .eData = UART_DATA_8,
        .eParity = UART_PARITY_NONE,
        .eStop = UART_STOP_1,
        .bIntrLSEnable = true
    };

    eErr = HAL_PCU_SetAltMode((PCU_ID_e)UART0_RX_PORT, (PCU_PIN_ID_e)UART0_RX_PORT_ID, (PCU_ALT_e)UART0_RX_MUX_ID);
    if (eErr != HAL_ERR_OK)
    {
        return;
    }

    eErr = HAL_PCU_SetAltMode((PCU_ID_e)UART0_TX_PORT, (PCU_PIN_ID_e)UART0_TX_PORT_ID, (PCU_ALT_e)UART0_TX_MUX_ID);
    if (eErr != HAL_ERR_OK)
    {
        return;
    }

    /* Initialize instance */
    eErr = HAL_UART_Init(UART_ID_0);
    if (eErr != HAL_ERR_OK)
    {
        return;
    }

    /* Set up the clock source */
    eErr = HAL_UART_SetClkConfig(UART_ID_0, &tClkCfg);
    if (eErr != HAL_ERR_OK)
    {
        return;
    }

    /* Set up operation parameters */
    eErr = HAL_UART_SetConfig(UART_ID_0, &tCfg);
    if (eErr != HAL_ERR_OK)
    {
        return;
    }

    /* Set IRQ priority and enable interrupt */
    eErr = HAL_UART_SetIRQ(UART_ID_0, UART_OPS_INTR, UART_IRQHandler, NULL, 3);
    if (eErr != HAL_ERR_OK)
    {
        return;
    }

    eErr = HAL_UART_Transmit(UART_ID_0, s_un8TxData, sizeof(s_un8TxData), false);
    if (eErr != HAL_ERR_OK)
    {
        return;
    }

    LOG("Tx Data : 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", s_un8TxData[0], s_un8TxData[1], s_un8TxData[2], s_un8TxData[3],
        s_un8TxData[4], s_un8TxData[5], s_un8TxData[6], s_un8TxData[7]);
    LOG("Wait Tx...\n");
}
#endif
