/**
 *******************************************************************************
 * @file        fmc_app.c
 * @author      ABOV R&D Division
 * @brief       Simple Application for FMC peripheral
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

#if (CONFIG_APP_FMC == 1)
extern void FMC_CFlash(void);
extern void FMC_DFlash(void);

void FMC_App(void)
{
#if FMC_APP == FMC_APP_CODE_FLASH
    FMC_CFlash();
#elif FMC_APP == FMC_APP_DATA_FLASH
    FMC_DFlash();
#endif
}
#endif

