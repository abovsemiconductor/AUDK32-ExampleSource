/**
 *******************************************************************************
 * @file        airclock.c
 * @author      ABOV R&D Division
 * @brief       AirClock Application Main
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
#include "hal_scu.h"
#include "hal_scu_clk.h"
#include "hal_pcu.h"

#include "debug_log.h"
#include "debug.h"
#include "u8g2.h"

#include "airclock_sensor.h"
#include "adc_rs3011.h"
#include "gpio_shared_irq.h"
#include "ir_receiver.h"
#include "button.h"
#include "softtimer.h"
#include "hw_timer.h"
#include "buzzer_bmx2705.h"
#include "oled.h"
#include "snor_w25q16jv.h"
#include "leddrv_al8700.h"
#include "menu.h"
#include "app_event.h"

#include "FreeRTOS.h"
#include "task.h"


static volatile uint32_t s_un32SysTimerVal=0;

volatile uint8_t g_BuzzerVolumeMode = 0;
volatile uint8_t g_BuzzerLevel = 0;

#define AIRCLOCK_APP_TASK_STACK_WORDS     384
#define AIRCLOCK_APP_TASK_PRIORITY        (tskIDLE_PRIORITY + 2)

static StaticTask_t s_appTaskTcb;
static StackType_t s_appTaskStack[AIRCLOCK_APP_TASK_STACK_WORDS];
static StaticTask_t s_idleTaskTcb;
static StackType_t s_idleTaskStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize)
{
    *ppxIdleTaskTCBBuffer = &s_idleTaskTcb;
    *ppxIdleTaskStackBuffer = s_idleTaskStack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

static void PRV_GetChipSetInfo(void)
{
    LOG("\n************************************************\n\r");
    LOG("- MCU - %s \n", PRV_CHIPSET_GetInfo());
    LOG("- Core: ARM %s  \n", PRV_CHIPSET_GetCoreInfo());
    LOG("- Communicate via: %s%d - %dbps\n", CONFIG_DEBUG_MODULE_STR,DEBUG_UART_ID,APP_UART_BAUD);
    LOG("- ARM System Core Clock = %d\n", SystemCoreClock);
    LOG("- AirClock Application (FreeRTOS).\n");
    LOG("************************************************\n\r");
}

/**********************************************************************
 * @brief		ARM System Timer Interrupt Handler.
 * @param[in]	None
 * @return	    None
 **********************************************************************/
void SysTick_Handler (void)
{
    EX_SoftTimer_TickIsr();
	
    xPortSysTickHandler();
}

/**********************************************************************
 * @brief       Change Buzzer Volume from ADC Data
 * @param[in]   event  : zero.
 * @param[in]   *pData : adc data.
 * @return      None
 **********************************************************************/
void ADC_BuzzerVolume (uint32_t event, void *pData)
{
    uint8_t adcData = *(uint8_t *)pData;
    
    g_BuzzerLevel = adcData;
    
    if(g_BuzzerVolumeMode)
    {
        EX_BUZZER_SetVolume(adcData);        
    }
}

static void AirClock_ServiceManagers(void)
{
    AppEvent_Manager();
    EX_SoftTimer_Manager();
}

static void AirClock_ServiceUI(void)
{
    menu_run();
}

static void AirClock_AppTask(void *pvParameters)
{
    TickType_t lastWake = xTaskGetTickCount();
    uint32_t uiTick = 0;
    uint32_t ledTick = 0;

    (void)pvParameters;

    while(1)
    {
        AirClock_ServiceManagers();
        if (++uiTick >= 50)
        {
            uiTick = 0;
            AirClock_ServiceUI();
        }

        if (++ledTick >= 10)
        {
            ledTick = 0;
            EX_LEDDRV_Manager();
        }
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(1));
    }
}

static void AirClock_RunFallbackLoop(void)
{
    while(1)
    {
        AirClock_ServiceManagers();
        AirClock_ServiceUI();
        EX_LEDDRV_Manager();
    }
}
   
/**********************************************************************
 * @brief       Main program
 * @param[in]   None
 * @return      0 : No error, Non-Zero : Any error
 **********************************************************************/
int main(void)
{
    SCUCLK_MCLK_CFG_t tMClkCfg =
    {
        .eMClk = SCUCLK_SRC_HSI,
        .ePreMClkDiv = SCUCLK_DIV_NONE,
        .ePostMClkDiv = SCUCLK_DIV_NONE
    };

    PRV_PORT_Init();

    (void)HAL_SCU_CLK_SetSrcEnable(SCUCLK_SRC_LSI, true);
    (void)HAL_SCU_CLK_SetSrcEnable(SCUCLK_SRC_HSE, true);
    (void)HAL_SCU_CLK_SetSrcEnable(SCUCLK_SRC_HSI, true);
    (void)HAL_SCU_CLK_SetMClk(&tMClkCfg);

#if (CONFIG_DEBUG == 1)
    /* configure serial port */

    /* waiting for initializing external device serial port */
    SystemDelayMS(120);

    /* some of the chipsets must configure serial port like below */
    HAL_PCU_SetInOutMode((PCU_ID_e)DEBUG_PORT_ID,(PCU_PIN_ID_e)DEBUG_RX_PORT_ID,PCU_INOUT_INPUT); /* For Debug */
    HAL_PCU_SetPullUpDown((PCU_ID_e)DEBUG_PORT_ID,(PCU_PIN_ID_e)DEBUG_RX_PORT_ID,PCU_PUPD_UP); /* For Debug */
    HAL_PCU_SetInOutMode((PCU_ID_e)DEBUG_PORT_ID,(PCU_PIN_ID_e)DEBUG_TX_PORT_ID,PCU_INOUT_OUTPUT_PUSH_PULL); /* For Debug */

    /* set port as a serial tx/rx mode */
    HAL_PCU_SetAltMode((PCU_ID_e)DEBUG_PORT_ID,(PCU_PIN_ID_e)DEBUG_RX_PORT_ID,(PCU_ALT_e)DEBUG_RX_ALT_ID); /* For Debug */
    HAL_PCU_SetAltMode((PCU_ID_e)DEBUG_PORT_ID,(PCU_PIN_ID_e)DEBUG_TX_PORT_ID,(PCU_ALT_e)DEBUG_TX_ALT_ID); /* For Debug */

    Debug_Init();
#endif

    PRV_GetChipSetInfo();

    if (!AppEvent_Init())
    {
        AirClock_RunFallbackLoop();
    }

    EX_SharedIRQ_Init();
    EX_BUZZER_Init();
    EX_SoftTimer_Init();
    EX_ADC_Init(ADC_BuzzerVolume);
    EX_IR_Init();
    EX_SNOR_Init();
    EX_TCS_Init();
    EX_LEDDRV_Init();
    
    menu_init();

    
    if (xTaskCreateStatic(AirClock_AppTask,
                          "airclock_app",
                          AIRCLOCK_APP_TASK_STACK_WORDS,
                          NULL,
                          AIRCLOCK_APP_TASK_PRIORITY,
                          s_appTaskStack,
                          &s_appTaskTcb) == NULL)
    {
        LOG("xTaskCreateStatic(air_app) fail.\n");
        AirClock_RunFallbackLoop();
    }

    vTaskStartScheduler();

    LOG("vTaskStartScheduler() returned.\n");
    AirClock_RunFallbackLoop();
}
