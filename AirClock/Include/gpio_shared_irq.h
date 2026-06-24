/**
 *******************************************************************************
 * @file        gpio_shared_irq.h
 * @author      ABOV R&D Division
 * @brief       gpio_shared_irq
 *
 * Copyright 2025 ABOV Semiconductor Co.,Ltd. All rights reserved.
 *
 * This file is licensed under terms that are found in the LICENSE file
 * located at Document directory.
 * If this file is delivered or shared without applicable license terms,
 * the terms of the BSD-3-Clause license shall be applied.
 * Reference: https://opensource.org/licenses/BSD-3-Clause
 ******************************************************************************/

#pragma once
#ifdef __cplusplus
extern "C"
{
#endif

typedef enum { 
    SHARED_GPIO_HIGH_ISR = 0, 
    SHARED_GPIO_LOW_QUEUE = 1
} SharedGPIO_Type_t;

bool EX_SharedIRQ_Register(PCU_IRQ_CFG_t *ptIRQCfg, uint32_t PINS, SharedGPIO_Type_t irqType);
void EX_SharedIRQ_Unregister(PCU_IRQ_CFG_t *ptIRQCfg);
void EX_SharedIRQ_Init(void);
void EX_SharedIRQ_Manager(void);

#ifdef __cplusplus
}
#endif

