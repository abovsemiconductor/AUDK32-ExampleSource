/**
 *******************************************************************************
 * @file        scu_app.c
 * @author      ABOV R&D Division
 * @brief       Simple Application for SCU peripheral
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

#if (CONFIG_APP_SCU == 1)

extern void SCU_CLK_ClockOutput(void);
extern void SCU_LVD_LowVoltageIndicator(void);
extern void SCU_LVD_LowVoltageReset(void);
extern void SCU_PWR_DeepSleep(void);

void SCU_App(void)
{
#if SCU_APP == SCU_APP_CLKOUTPUT
    SCU_CLK_ClockOutput();
#elif SCU_APP == SCU_APP_LVD_INDICATOR
    SCU_LVD_LowVoltageIndicator();
#elif SCU_APP == SCU_APP_LVD_RESET
    SCU_LVD_LowVoltageReset();
#elif SCU_APP == SCU_APP_PWR_DEEPSLEEP
    SCU_PWR_DeepSleep();
#endif
}
#endif

