/**
 *******************************************************************************
 * @file        uart_app.c
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

#include "abov_config.h"
#include "abov_simpleapp_config.h"

#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"

#if (CONFIG_APP_UART == 1)

extern void UART_Rx(void);
extern void UART_Tx(void);

void UART_App(void)
{
#if UART_APP == UART_APP_MODE_RX
    UART_Rx();
#elif UART_APP == UART_APP_MODE_TX
    UART_Tx();
#endif
}
#endif

