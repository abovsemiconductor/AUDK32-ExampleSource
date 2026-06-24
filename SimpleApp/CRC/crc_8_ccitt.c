/**
 *******************************************************************************
 * @file        crc_8_ccitt.c
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

/* 
 * Readme:
 * This example demonstrates how to use CRC in CRC-8 CCITT mode.
 * The CRC result is printed in the debug log.
 * 
 * Connection:
 * - None
 */

#include "abov_config.h"
#include "abov_simpleapp_config.h"

#include "hal_crc.h"

#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"

#if (CONFIG_APP_CRC == 1)

#define CRC_RESULT       0x8f

static uint8_t s_un8Data[] = {0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36};

void CRC_8_CCITT(void)
{
    HAL_ERR_e eErr = HAL_ERR_OK;

    CRC_CFG_t tCfg =
    {
        .eMode = CRC_MODE_CRC,
        .eFirstIn = CRC_INP_MSB,
        .bIntrEnable = false,
        .tOutputCfg.eFirstOut = CRC_OUTP_LSB,
        .tOutputCfg.eInv = CRC_OUTP_INV_OFF,
        .ePoly = CRC_POLY_8
    };

    uint32_t un32Result;

    /* Initialize instance */
    eErr = HAL_CRC_Init(CRC_ID_0);
    if (eErr != HAL_ERR_OK)
    {
        LOG("HAL_CRC_Init() error, (%d)\n", eErr);
        return;
    }

    /* Set up operation parameters */
    eErr = HAL_CRC_SetConfig(CRC_ID_0, &tCfg);
    if (eErr != HAL_ERR_OK)
    {
        LOG("HAL_CRC_SetConfig() error, (%d)\n", eErr);
        return;
    }

    eErr = HAL_CRC_SetCompute(CRC_ID_0, 0, s_un8Data, sizeof(s_un8Data), &un32Result);
    if (eErr != HAL_ERR_OK)
    {
        LOG("HAL_CRC_SetCompute() error, (%d)\n", eErr);
        return;
    }

    if(un32Result != CRC_RESULT)
    {
        LOG("CRC-8 CCITT Result mismatch(0x%x, 0x%x)\n", un32Result, CRC_RESULT);
    }
    else
    {
        LOG("CRC-8 CCITT Result (0x%x)\n", un32Result);
    }
}
#endif
