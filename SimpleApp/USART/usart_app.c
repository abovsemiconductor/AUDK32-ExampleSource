/**
 *******************************************************************************
 * @file        usart_app.c
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

#include <stdbool.h>

#include "abov_config.h"
#include "abov_simpleapp_config.h"

#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"

#if (CONFIG_APP_USART == 1)

extern void USART_UART_Rx(void);
extern void USART_UART_Tx(void);
extern void USART_USRT_Rx(void);
extern void USART_USRT_Tx(void);
extern void USART_SPI_Rx(void);
extern void USART_SPI_Tx(void);

void USART_App(void)
{
#if USART_APP == USART_APP_MODE_UART_RX
    USART_UART_Rx();
#elif USART_APP == USART_APP_MODE_UART_TX
    USART_UART_Tx();
#elif USART_APP == USART_APP_MODE_USRT_RX
    USART_USRT_Rx();
#elif USART_APP == USART_APP_MODE_USRT_TX
    USART_USRT_Tx();
#elif USART_APP == USART_APP_MODE_SPI_RX
    USART_SPI_Rx();
#elif USART_APP == USART_APP_MODE_SPI_TX
    USART_SPI_Tx();
#endif
}
#endif

