/**
 *******************************************************************************
 * @file        gpio_shared_irq.c
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
#include "abov_config.h"
#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "hal_common.h"
#include "hal_scu.h"
#include "hal_scu_clk.h"
#include "hal_pcu.h"
#include "gpio_shared_irq.h"
#include "app_event.h"


/******************************** Context *******************************************/
typedef struct {
    pfnPCU_IRQ_Handler_t    cb;
    SharedGPIO_Type_t       type;
} GPIOSHAREDIRQ_Context_t;

static GPIOSHAREDIRQ_Context_t s_GSI_Context[PCU_ID_MAX][PCU_PIN_ID_MAX] = {0};


static void PRV_SharedIRQ_Handler(uint32_t un32Event, void *pContext)
{
    // find which pin?
    for (int nPin = 0; nPin < PCU_PIN_ID_MAX; nPin++)
    {
        if ((un32Event >> (nPin * 2) & 0x3) == 0)
        {
            // this pin is not in ISR.
            continue;
        }

        for (int nPort = 0; nPort < PCU_ID_MAX; nPort++)
        {
            if (s_GSI_Context[nPort][nPin].cb && s_GSI_Context[nPort][nPin].type == SHARED_GPIO_HIGH_ISR)
            {
                s_GSI_Context[nPort][nPin].cb(un32Event, pContext);
                //LOG("fire SHARED_GPIO_HIGH_ISR! pin%d_%d\n", nPort, nPin);
            }
            else if(s_GSI_Context[nPort][nPin].cb && s_GSI_Context[nPort][nPin].type == SHARED_GPIO_LOW_QUEUE)
            {
                (void)AppEvent_SendGPIOFromISR(s_GSI_Context[nPort][nPin].cb, un32Event, pContext);
                //LOG("queue SHARED_GPIO_LOW_QUEUE! pin%d_%d\n", nPort, nPin);
            }
        }
    }
}

static inline int is_pin_active(uint32_t PINS, uint32_t isPin)
{
    return (PINS >> isPin) & 1u;
}

bool EX_SharedIRQ_Register(PCU_IRQ_CFG_t *ptIRQCfg, uint32_t PINS, SharedGPIO_Type_t irqType)
{
    if (ptIRQCfg == NULL || ptIRQCfg->eId >= PCU_ID_MAX)
        return false;

    for (uint8_t pin = 0; pin < PCU_PIN_ID_MAX; pin++)
    {
        if (is_pin_active(PINS, pin))
        {
            GPIOSHAREDIRQ_Context_t newCtx = {
                ptIRQCfg->pfnHandler,
                irqType
            };

            // register request
            s_GSI_Context[ptIRQCfg->eId][pin] = newCtx;
            LOG("registered pin%d_%d for GPIO Shared IRQ as [%s]\n", ptIRQCfg->eId, pin, (irqType == SHARED_GPIO_HIGH_ISR ? "HIGH" : "LOW"));		
            }
    }	
    // call HAL IRQ
    ptIRQCfg->pfnHandler = PRV_SharedIRQ_Handler;
    HAL_PCU_SetIRQ(ptIRQCfg);

    return true;
}


void EX_SharedIRQ_Unregister(PCU_IRQ_CFG_t *ptIRQCfg)
{
    for (int i = 0; i < PCU_PIN_ID_MAX; i++)
    {
        if (s_GSI_Context[ptIRQCfg->eId][i].cb == ptIRQCfg->pfnHandler)
        {
            memset(&s_GSI_Context[ptIRQCfg->eId][i], 0x00, sizeof(GPIOSHAREDIRQ_Context_t));
            LOG("unregister pin%d_%d for GPIO Shared IRQ.\n", ptIRQCfg->eId, i);
        }
    }
}
void EX_SharedIRQ_Manager(void)
{
    return;
}
void EX_SharedIRQ_Init(void)
{
    memset(s_GSI_Context, 0x00, sizeof(s_GSI_Context));	
}
