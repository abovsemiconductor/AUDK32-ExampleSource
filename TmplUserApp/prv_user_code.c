/**
 *******************************************************************************
 * @file        prv_user_code.c
 * @author      ABOV R&D Division
 * @brief       Dummy User Application Main
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

#if (CONFIG_DEBUG == 1)
#include "debug_log.h"
#include "debug.h"
#endif

/* Include HAL header files for your target modules */


/**********************************************************************
 * @brief       User Code Here
 * @param[in]   None
 * @return      None
 **********************************************************************/
void PRV_USER_Code(void)
{
    /* put your code above this line. */
    while (1)
    {
        ;
    }
}
