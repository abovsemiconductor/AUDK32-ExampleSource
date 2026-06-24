/**
 *******************************************************************************
 * @file        pcu_gpio_isr.c
 * @author      ABOV R&D Division
 * @brief       Simple Application for PCU peripheral
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
 * - This example demonstrates how to set up GPIO interrupt using PCU and handle it in the interrupt service routine.
 * - The interrupt is triggered on both rising and falling edges of the input GPIO pin.
 * 
 * Connection:
 * - PA7 (GPIO_OUTPUT_PORT) - PE7 (GPIO_INPUT_PORT)
 */

#include "abov_config.h"
#include "abov_simpleapp_config.h"

#include "hal_pcu.h"

#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"

#if (CONFIG_APP_PCU == 1)

static void PRV_PCU_IRQHandler(uint32_t un32Event, void *pContext)
{
	uint32_t un32PortStatus = (un32Event >> (GPIO_INPUT_PORT_ID * 2)) & 0x3;
	
	if (un32PortStatus & 0x01)
	{
		LOG("Low Level or Falling Edge\n");
	}
	else if (un32PortStatus & 0x02)
	{
		LOG("High Level or Rising Edge\n");
	}
	else
	{
		LOG("Both Level or Edge\n");
	}
}

void PCU_GpioISR(void)
{
    HAL_ERR_e eErr = HAL_ERR_OK;
	PCU_IRQ_CFG_t tIrqCfg = {
		.eId = (PCU_ID_e)GPIO_INPUT_PORT,
		.eOps = PCU_OPS_INTR,
		.pfnHandler = &PRV_PCU_IRQHandler,
		.pContext = NULL,
		.un32IRQPrio = 3,
		.un8IntNum = GPIO_INPUT_PORT_INTR_NUM
	};

	eErr = HAL_PCU_SetInOutMode((PCU_ID_e)GPIO_OUTPUT_PORT, (PCU_PIN_ID_e)GPIO_OUTPUT_PORT_ID, PCU_INOUT_OUTPUT_PUSH_PULL);
	if(eErr != HAL_ERR_OK)
	{
		return;
	}

	eErr = HAL_PCU_SetPullUpDown((PCU_ID_e)GPIO_OUTPUT_PORT, (PCU_PIN_ID_e)GPIO_OUTPUT_PORT_ID, PCU_PUPD_UP);
	if(eErr != HAL_ERR_OK)
	{
		return;
	}

	eErr = HAL_PCU_SetInOutMode((PCU_ID_e)GPIO_INPUT_PORT, (PCU_PIN_ID_e)GPIO_INPUT_PORT_ID, PCU_INOUT_INPUT);
	if(eErr != HAL_ERR_OK)
	{
		return;
	}

	eErr = HAL_PCU_SetIntrPort((PCU_ID_e)GPIO_INPUT_PORT, (PCU_PIN_ID_e)GPIO_INPUT_PORT_ID, PCU_INTR_MODE_EDGE, PCU_INTR_TRG_BOTH_LEVEL_EDGE, GPIO_INPUT_PORT_INTR_NUM);
	if(eErr != HAL_ERR_OK)
	{
		return;
	}

	eErr = HAL_PCU_SetIRQ(&tIrqCfg);
	if(eErr != HAL_ERR_OK)
	{
		return;
	}

	LOG("Change port output value at 1sec intervals\n");
	for(uint32_t i = 0 ; i < 10 ; i++)
	{
	eErr = HAL_PCU_SetOutputValue((PCU_ID_e)GPIO_OUTPUT_PORT, (PCU_PIN_ID_e)GPIO_OUTPUT_PORT_ID, (PCU_PORT_e)(i % 2));
	if(eErr != HAL_ERR_OK)
	{
		return;
	}

		SystemDelayMS(1000);
	}
}
#endif
