/**
 *******************************************************************************
 * @file        gpio_led_blinking.c
 * @author      ABOV R&D Division
 * @brief       Blinking GPIO LED Example Code
 *
 * Copyright 2022 ABOV Semiconductor Co.,Ltd. All rights reserved.
 *
 * This file is licensed under terms that are found in the LICENSE file
 * located at Document directory.
 * If this file is delivered or shared without applicable license terms,
 * the terms of the BSD-3-Clause license shall be applied.
 * Reference: https://opensource.org/licenses/BSD-3-Clause
 ******************************************************************************/

#include "abov_config.h"
#include "abov_example_config.h"

#if defined(GPIO_LED_BLINKING)
#include "debug_log.h"
#include "debug.h"
#include "hal_scu.h"
#include "hal_scu_clk.h"
#include "hal_pcu.h"

/* HCLK is generated from HSI by default */
#define TC_SYSTICK_1MS      1000

extern uint32_t SystemCoreClock;
static volatile uint32_t s_un32SysTimerVal=0;

#if (CONFIG_DEBUG == 1)
void GetChipSetInfo(void)
{
    int8_t *pn8ChipInfo = NULL;
    int8_t *pn8ChipCoreInfo = NULL;
    pn8ChipInfo = PRV_CHIPSET_GetInfo();
    pn8ChipCoreInfo = PRV_CHIPSET_GetCoreInfo();
    LOG("************************************************\n\r");
    LOG("- MCU - %s \n",pn8ChipInfo);
    LOG("- Core: ARM %s  \n",pn8ChipCoreInfo);
    LOG("- Communicate via: %s%d - %dbps\n",CONFIG_DEBUG_MODULE_STR,DEBUG_UART_ID,APP_UART_BAUD);
    LOG("- ARM System Core Clock = %d\n",SystemCoreClock);
    LOG("- LED GPIO Blinking Example\n");
    LOG("************************************************\n\r");
}
#endif

/**********************************************************************
 * @brief		ARM System Timer Interrupt Handler.
 * @param[in]	None
 * @return	    None
 **********************************************************************/
void SysTick_Handler (void)
{
    if (s_un32SysTimerVal)
    {
        s_un32SysTimerVal--;
    }
}

/**********************************************************************
 * @brief		Waiting by time(ms)
 * @param[in]	un32TimeMS : Milisecond time to wait.
 * @return	    None
 **********************************************************************/
static void SYSTICK_Wait (uint32_t un32TimeMS)
{
    s_un32SysTimerVal = un32TimeMS;
    while(s_un32SysTimerVal);
}

/**********************************************************************
 * @brief		Start blinking LED on Starter Kit.
 * @param[in]	None
 * @return	    None
 **********************************************************************/
static void TC_LED_StartBlinking(void)
{
    uint32_t un32LedID;

#if defined(STK_LED_PORT_NON_SEQUENTIAL)
    /* Turn on */
    for (un32LedID = FIRST_LED_ID; un32LedID <= LAST_LED_ID; un32LedID++)
    {
        HAL_PCU_SetOutputBit((PCU_ID_e)STK_LED_GetPortID(un32LedID), (PCU_PIN_ID_e)STK_LED_GetPinID(un32LedID), PCU_OUTPUT_BIT_SET);
        SYSTICK_Wait(50);
    }

    /* Turn off */
    for (un32LedID = FIRST_LED_ID; un32LedID <= LAST_LED_ID; un32LedID++)
    {
        HAL_PCU_SetOutputBit((PCU_ID_e)STK_LED_GetPortID(un32LedID), (PCU_PIN_ID_e)STK_LED_GetPinID(un32LedID), PCU_OUTPUT_BIT_CLEAR);
        SYSTICK_Wait(50);
    }
#else
    /* Turn on */
    for (un32LedID = FIRST_LED_ID; un32LedID <= LAST_LED_ID; un32LedID++)
    {
        HAL_PCU_SetOutputBit((PCU_ID_e)LED_PORT_ID, (PCU_PIN_ID_e)un32LedID, PCU_OUTPUT_BIT_SET);
        SYSTICK_Wait(50);
    }

    /* Turn off */
    for (un32LedID = FIRST_LED_ID; un32LedID <= LAST_LED_ID; un32LedID++)
    {
        HAL_PCU_SetOutputBit((PCU_ID_e)LED_PORT_ID, (PCU_PIN_ID_e)un32LedID, PCU_OUTPUT_BIT_CLEAR);
        SYSTICK_Wait(50);
    }
#endif
}

/**********************************************************************
 * @brief		Main program
 * @param[in]	None
 * @return	    0 : No error, Non-Zero : Any error
 **********************************************************************/
int main(void)
{
    uint32_t un32LedID=0;
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
    /* serial init */
    HAL_PCU_SetAltMode((PCU_ID_e)DEBUG_PORT_ID,(PCU_PIN_ID_e)DEBUG_TX_PORT_ID,(PCU_ALT_e)DEBUG_TX_ALT_ID); /* For Debug */
    HAL_PCU_SetAltMode((PCU_ID_e)DEBUG_PORT_ID,(PCU_PIN_ID_e)DEBUG_RX_PORT_ID,(PCU_ALT_e)DEBUG_RX_ALT_ID); /* For Debug */
    Debug_Init();
    GetChipSetInfo();
#endif

#if defined(STK_LED_PORT_NON_SEQUENTIAL)
    for (un32LedID = FIRST_LED_ID; un32LedID <= LAST_LED_ID; un32LedID++)
    {
        HAL_PCU_SetAltMode((PCU_ID_e)STK_LED_GetPortID(un32LedID), (PCU_PIN_ID_e)STK_LED_GetPinID(un32LedID), PCU_ALT_0); /* Set GPIO Mode */
        HAL_PCU_SetInOutMode((PCU_ID_e)STK_LED_GetPortID(un32LedID), (PCU_PIN_ID_e)STK_LED_GetPinID(un32LedID), PCU_INOUT_OUTPUT_PUSH_PULL);
        HAL_PCU_SetPullUpDown((PCU_ID_e)STK_LED_GetPortID(un32LedID), (PCU_PIN_ID_e)STK_LED_GetPinID(un32LedID), PCU_PUPD_DISABLED);
        HAL_PCU_SetOutputBit((PCU_ID_e)STK_LED_GetPortID(un32LedID), (PCU_PIN_ID_e)STK_LED_GetPinID(un32LedID), PCU_OUTPUT_BIT_CLEAR);
        HAL_PCU_SetOutputBit((PCU_ID_e)STK_LED_GetPortID(un32LedID), (PCU_PIN_ID_e)STK_LED_GetPinID(un32LedID), PCU_OUTPUT_BIT_SET);
    }
#else
    for (un32LedID = FIRST_LED_ID; un32LedID <= LAST_LED_ID; un32LedID++)
    {
        HAL_PCU_SetAltMode((PCU_ID_e)LED_PORT_ID, (PCU_PIN_ID_e)un32LedID, PCU_ALT_0); /* Set GPIO Mode */
        HAL_PCU_SetInOutMode((PCU_ID_e)LED_PORT_ID, (PCU_PIN_ID_e)un32LedID, PCU_INOUT_OUTPUT_PUSH_PULL);
        HAL_PCU_SetPullUpDown((PCU_ID_e)LED_PORT_ID, (PCU_PIN_ID_e)un32LedID, PCU_PUPD_DISABLED);
        HAL_PCU_SetOutputBit((PCU_ID_e)LED_PORT_ID, (PCU_PIN_ID_e)un32LedID, PCU_OUTPUT_BIT_CLEAR);
        HAL_PCU_SetOutputBit((PCU_ID_e)LED_PORT_ID, (PCU_PIN_ID_e)un32LedID, PCU_OUTPUT_BIT_SET);
    }
#endif

    /* Setup ARM System Timer to 1ms */
    SysTick_Config(SystemCoreClock/TC_SYSTICK_1MS);

    /* main loop */
    while(1)
    {
        TC_LED_StartBlinking();
    };

}

#else
#error Except "BARE", select one of target board at "ABOV Target Board Selection" of abov_config.h
#endif
