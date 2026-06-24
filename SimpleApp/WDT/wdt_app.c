/**
 *******************************************************************************
 * @file        wdt_app.c
 * @author      ABOV R&D Division
 * @brief       Simple Application for WDT peripheral
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

#if (CONFIG_APP_WDT == 1)

extern void WDT_Underflow(void);
extern void WDT_Reset(void);

void WDT_App(void)
{
#if WDT_APP == WDT_APP_MODE_UNDERFLOW
    WDT_Underflow();
#elif WDT_APP == WDT_APP_MODE_RESET
    WDT_Reset();
#endif
}
#endif

