/**
 *******************************************************************************
 * @file        freertos.c
 * @author      ABOV R&D Division
 * @brief       Main Example
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
#include "freertos_example.h"

#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "hal_scu.h"
#include "hal_scu_clk.h"
#include "hal_pcu.h"

#if (CONFIG_DEBUG == 1)
static void GetChipSetInfo(void)
{
    LOG("\n************************************************\n\r");
    LOG("- MCU - %s \n",PRV_CHIPSET_GetInfo());
    LOG("- Core: ARM %s  \n",PRV_CHIPSET_GetCoreInfo());
    LOG("- Communicate via: %s%d - %dbps\n",CONFIG_DEBUG_MODULE_STR,DEBUG_UART_ID,APP_UART_BAUD);
    LOG("- ARM System Core Clock = %d\n",SystemCoreClock);
    LOG("************************************************\n\r");
}
#endif

/**********************************************************************
 * @brief		Main program
 * @param[in]	None
 * @return	None
 **********************************************************************/
int main(void)
{
    SCUCLK_MCLK_CFG_t tMClkCfg =
    {
        .eMClk = SCUCLK_SRC_HSI,
        .ePreMClkDiv = SCUCLK_DIV_NONE,
        .ePostMClkDiv = SCUCLK_DIV_NONE
    };
    uint32_t un32LedID;

    PRV_PORT_Init();

    (void)HAL_SCU_CLK_SetSrcEnable(SCUCLK_SRC_LSI, true);
    (void)HAL_SCU_CLK_SetSrcEnable(SCUCLK_SRC_HSE, true);
    (void)HAL_SCU_CLK_SetSrcEnable(SCUCLK_SRC_HSI, true);
    (void)HAL_SCU_CLK_SetMClk(&tMClkCfg);

    SystemDelayMS(120);

#if (CONFIG_DEBUG == 1)
    /* configure serial port */

    /* some of the chipsets must configure serial port like below */
    HAL_PCU_SetInOutMode((PCU_ID_e)DEBUG_PORT_ID,(PCU_PIN_ID_e)DEBUG_RX_PORT_ID,PCU_INOUT_INPUT); /* For Debug */
    HAL_PCU_SetPullUpDown((PCU_ID_e)DEBUG_PORT_ID,(PCU_PIN_ID_e)DEBUG_RX_PORT_ID,PCU_PUPD_UP); /* For Debug */
    HAL_PCU_SetInOutMode((PCU_ID_e)DEBUG_PORT_ID,(PCU_PIN_ID_e)DEBUG_TX_PORT_ID,PCU_INOUT_OUTPUT_PUSH_PULL); /* For Debug */

    /* set port as a serial tx/rx mode */
    HAL_PCU_SetAltMode((PCU_ID_e)DEBUG_PORT_ID,(PCU_PIN_ID_e)DEBUG_RX_PORT_ID,(PCU_ALT_e)DEBUG_RX_ALT_ID); /* For Debug */
    HAL_PCU_SetAltMode((PCU_ID_e)DEBUG_PORT_ID,(PCU_PIN_ID_e)DEBUG_TX_PORT_ID,(PCU_ALT_e)DEBUG_TX_ALT_ID); /* For Debug */
    Debug_Init();
#endif

    GetChipSetInfo();

    for (un32LedID = FIRST_LED_ID; un32LedID <= LAST_LED_ID; un32LedID++)
    {
        HAL_PCU_SetAltMode((PCU_ID_e)LED_PORT_ID, (PCU_PIN_ID_e)un32LedID, PCU_ALT_0);
        HAL_PCU_SetInOutMode((PCU_ID_e)LED_PORT_ID, (PCU_PIN_ID_e)un32LedID, PCU_INOUT_OUTPUT_PUSH_PULL);
        HAL_PCU_SetPullUpDown((PCU_ID_e)LED_PORT_ID, (PCU_PIN_ID_e)un32LedID, PCU_PUPD_DISABLED);
        HAL_PCU_SetOutputBit((PCU_ID_e)LED_PORT_ID, (PCU_PIN_ID_e)un32LedID, PCU_OUTPUT_BIT_CLEAR);
        HAL_PCU_SetOutputBit((PCU_ID_e)LED_PORT_ID, (PCU_PIN_ID_e)un32LedID, PCU_OUTPUT_BIT_SET);
    }

#if defined(FREERTOS_EXAMPE_API_NATIVE)
    freertos_native();
#elif defined(FREERTOS_EXAMPE_API_CMSIS_RTOS_V1)
    cmsis_rtos_v1();
#elif defined(FREERTOS_EXAMPE_API_CMSIS_RTOS_V2)
    cmsis_rtos_v2();
#endif

    while(1)
    {
        ;
    }

    return 0;
}

/* --------------------------------- End Of File ------------------------------ */
