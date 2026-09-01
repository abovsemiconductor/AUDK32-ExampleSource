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

#if (CONFIG_APP_FRT == 1)

extern void FRT_Match(void);
extern void FRT_Overflow(void);

void FRT_App(void)
{
    LOG("Free Run Timer Application\n");

#if FRT_APP == FRT_APP_MODE_MATCH
    FRT_Match();
#elif FRT_APP == FRT_APP_MODE_OVERFLOW
    FRT_Overflow();
#else
    #error "Select one of FRT application"
#endif
}
#endif

