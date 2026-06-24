/**
 *******************************************************************************
 * @file        user_crc.c
 * @author      ABOV R&D Division
 * @brief       Template User Application Peripheral CRC V2x
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
#include "abov_simpleapp_config.h"

#include "hal_scu.h"
#include "hal_scu_clk.h"
#include "hal_pcu.h"

#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"

extern void PCU_App(void);
extern void FMC_App(void);
extern void CRC_App(void);
extern void TIMER1_App(void);
extern void I2C_App(void);
extern void UART_App(void);
extern void USART_App(void);
extern void ADC_App(void);
extern void WDT_App(void);
extern void SCU_App(void);

void GetChipSetInfo(void)
{
    LOG("\n************************************************\n\r");
    LOG("- MCU - %s \n",PRV_CHIPSET_GetInfo());
    LOG("- Core: ARM %s  \n",PRV_CHIPSET_GetCoreInfo());
    LOG("- Communicate via: %s%d - %dbps\n",CONFIG_DEBUG_MODULE_STR,DEBUG_UART_ID,APP_UART_BAUD);
    LOG("- ARM System Core Clock = %d\n",SystemCoreClock);
    LOG("************************************************\n\r");
}

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

    /* waiting for initializing external device serial port */
    SystemDelayMS(120);

#if (CONFIG_DEBUG == 1)
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

#if (CONFIG_APP_PCU == 1)
    PCU_App();
#endif
#if (CONFIG_APP_FMC == 1)
    FMC_App();
#endif
#if (CONFIG_APP_CRC == 1)
    CRC_App();
#endif
#if (CONFIG_APP_TIMER1 == 1)
    TIMER1_App();
#endif
#if (CONFIG_APP_I2C == 1)
    I2C_App();
#endif
#if (CONFIG_APP_UART == 1)
    UART_App();
#endif
#if (CONFIG_APP_USART == 1)
    USART_App();
#endif
#if (CONFIG_APP_ADC == 1)
    ADC_App();
#endif
#if (CONFIG_APP_WDT == 1)
    WDT_App();
#endif
#if (CONFIG_APP_SCU == 1)
    SCU_App();
#endif

    while(1)
    {
        ;
    }
}
