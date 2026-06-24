/**
 *******************************************************************************
 * @file        crc_app.c
 * @author      ABOV R&D Division
 * @brief       Simple Application for CRC peripheral
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

#if (CONFIG_APP_CRC == 1)

extern void CRC_32(void);
extern void CRC_16_USB(void);
extern void CRC_16_CCITT(void);
extern void CRC_8_CCITT(void);

void CRC_App(void)
{
#if CRC_APP == CRC_APP_POLY_32
    CRC_32();
#elif CRC_APP == CRC_APP_POLY_16_USB
    CRC_16_USB();
#elif CRC_APP == CRC_APP_POLY_16_CCITT
    CRC_16_CCITT();
#elif CRC_APP == CRC_APP_POLY_8_CCITT
    CRC_8_CCITT();
#endif
}
#endif

