/**
 *******************************************************************************
 * @file        timer1_app.c
 * @author      ABOV R&D Division
 * @brief       Simple Application for TIMER1 peripheral
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

#if (CONFIG_APP_TIMER1 == 1)

extern void TIMER1_Periodic(void);
extern void TIMER1_Oneshot(void);
extern void TIMER1_Pwm(void);
extern void TIMER1_Capture(void);

void TIMER1_App(void)
{
#if TIMER1_APP == TIMER1_APP_MODE_PERIODIC
    TIMER1_Periodic();
#elif TIMER1_APP == TIMER1_APP_MODE_ONESHOT
    TIMER1_Oneshot();
#elif TIMER1_APP == TIMER1_APP_MODE_PWM
    TIMER1_Pwm();
#elif TIMER1_APP == TIMER1_APP_MODE_CAPTURE
    TIMER1_Capture();
#endif
}
#endif

