/**
 *******************************************************************************
 * @file        prv_scu_clk.c
 * @author      ABOV R&D Division
 * @brief       Dummy User Clock Configuration
 *
 * Copyright 2024 ABOV Semiconductor Co.,Ltd. All rights reserved.
 *
 * This file is licensed under terms that are found in the LICENSE file
 * located at Document directory.
 * If this file is delivered or shared without applicable license terms,
 * the terms of the BSD-3-Clause license shall be applied.
 * Reference: https://opensource.org/licenses/BSD-3-Clause
 ******************************************************************************/

#include "abov_config.h"
#include "abov_module_config.h"

#include "hal_scu_clk.h"

#if (CONFIG_DEBUG == 1)
#include "debug_log.h"
#include "debug.h"
#endif

/**********************************************************************
 * @brief       Dummy User Clock Configuration Function
 * @param[in]   None
 * @return      None
 **********************************************************************/
void PRV_SCU_CLK_Init(void)
{
	return;
}
