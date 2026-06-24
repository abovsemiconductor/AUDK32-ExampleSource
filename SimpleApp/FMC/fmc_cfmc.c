/**
 *******************************************************************************
 * @file        fmc_cfmc.c
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

/* 
 * Readme:
 * This example demonstrates how to use CFMC to erase and write code flash.
 * The memory content is printed in the debug log.
 * 
 * Connection:
 * - None
 */

#include "abov_config.h"
#include "abov_simpleapp_config.h"

#if defined(_DFMC)
#include "hal_fmc.h"
#else
#include "hal_cfmc.h"
#endif

#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"

#if (CONFIG_APP_FMC == 1)

#define CFMC_TEST_SIZE			2048
#define CFMC_DATA_BUF_SIZE		16


void FMC_CFlash(void)
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    CFMC_GEOMETRY_t tCfmcGeo;
	uint32_t un32TestBaseAddr, offset;
	uint8_t un8Data[CFMC_DATA_BUF_SIZE] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};

    eErr = HAL_CFMC_Init();
    if(eErr != HAL_ERR_OK)
    {
        LOG("HAL_CFMC_Init() error, (%d)\n", eErr);
        return;
    }

	tCfmcGeo = HAL_CFMC_GetGeometry();

	eErr = HAL_CFMC_SetWriteProtect(0, tCfmcGeo.un32CflashSize, false);
    if(eErr != HAL_ERR_OK)
    {
        return;
    }

	un32TestBaseAddr = tCfmcGeo.un32CflashSize - CFMC_TEST_SIZE;

	LOG("Erase 2KB at 0x%08X.\n", un32TestBaseAddr);
	eErr = HAL_CFMC_Erase(un32TestBaseAddr, CFMC_2KB_ERASE_MODE);
	if(eErr != HAL_ERR_OK)
	{
		return;
	}

	LOG("Dump memory at 0x%08X.\n", un32TestBaseAddr);
    for(offset = 0 ; offset < CFMC_TEST_SIZE; offset += 4)
    {
        LOG("\tA=0x%08X, D=0x%08X\n", un32TestBaseAddr + offset, *(uint32_t *)(un32TestBaseAddr + offset));
    }

	LOG("Write data at 0x%08X.\n", un32TestBaseAddr);
	for(offset = 0 ; offset < CFMC_TEST_SIZE ; offset += CFMC_DATA_BUF_SIZE)
	{
		eErr = HAL_CFMC_Write(un32TestBaseAddr + offset, un8Data, CFMC_DATA_BUF_SIZE);
		if(eErr != HAL_ERR_OK)
		{
			return;
		}
	}

	LOG("Dump memory at 0x%08X.\n", un32TestBaseAddr);
    for(offset = 0 ; offset < CFMC_TEST_SIZE; offset += 4)
    {
        LOG("\tA=0x%08X, D=0x%08X\n", un32TestBaseAddr + offset, *(uint32_t *)(un32TestBaseAddr + offset));
    }
}
#endif
