/**
 *******************************************************************************
 * @file        ex_peripheral.c
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
#include "abov_example_config.h"
#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "hal_scu.h"
#include "hal_scu_clk.h"
#include "hal_pcu.h"
#include "ex_peripheral.h"

#if defined(CONFIG_MODULE_COA)
#if defined(_CFMC) && defined(_DFMC)
#include "hal_fmc.h"
#elif defined(_CFMC)
#include "hal_cfmc.h"
#endif
#endif

#if (CONFIG_DEBUG == 1)
void GetChipSetInfo(void)
{
    int8_t *pn8ChipInfo = NULL;
    int8_t *pn8ChipCoreInfo = NULL;
    pn8ChipInfo = PRV_CHIPSET_GetInfo();
    pn8ChipCoreInfo = PRV_CHIPSET_GetCoreInfo();
    LOG("\n************************************************\n\r");
    LOG("- MCU - %s \n",pn8ChipInfo);
    LOG("- Core: ARM %s  \n",pn8ChipCoreInfo);
    LOG("- Communicate via: %s%d - %dbps\n",CONFIG_DEBUG_MODULE_STR,DEBUG_UART_ID,APP_UART_BAUD);
    LOG("- ARM System Core Clock = %d\n",SystemCoreClock);
    LOG("- CLI : 'ls' show command list \n");
    LOG("************************************************\n\r");
}
#endif

#if (CONFIG_DEBUG == 1) && defined(CONFIG_MODULE_COA)
static void EX_SetCOADefault(void)
{
    uint32_t un32Page1;
    bool bNeedUpdate = false;

    un32Page1 = *(uint32_t *)CONFIG_COA_PAGE1_ADDR;

    if((un32Page1 == 0xFFFFFFFF) || (un32Page1 & 0x03) != 0x03)
    {
#if defined(_CFMC) && defined(_DFMC)
        (void)HAL_FMC_Init();
        FMC_RD_PROTECT_CONFIG_t tRdProtectConfig;
        tRdProtectConfig.eRdProtectLevel = FMC_RD_PROTECT_LEVEL0;
        (void)HAL_FMC_SetReadProtect(FMC_ID_CFMC, tRdProtectConfig);
        bNeedUpdate = true;
        LOG("\n Set Read Protection Level 0 at Debug!\n");
#elif defined(_CFMC)
        (void)HAL_CFMC_Init();
        CFMC_RD_PROTECT_CONFIG_t tRdProtectConfig;
        tRdProtectConfig.eRdProtectLevel = CFMC_RD_PROTECT_LEVEL0;
        (void)HAL_CFMC_SetReadProtect(tRdProtectConfig);
        bNeedUpdate = true;
        LOG("\n Set Read Protection Level 0 at Debug!\n");
#endif
    }

    un32Page1 = *(uint32_t *)CONFIG_COA_WDT_ADDR;
    if(un32Page1 == 0xFFFFFFFF)
    {
#if defined(_CFMC) && defined(_DFMC)
        uint32_t un32WdtDefault = CONFIG_WDT_DEFAULT;
        (void)HAL_FMC_Init();
        (void)HAL_FMC_Write(FMC_ID_CFMC, CONFIG_COA_WDT_ADDR, (uint8_t *)&un32WdtDefault, sizeof(uint32_t));
        bNeedUpdate = true;
        LOG("\n Set WDT default configuration at Debug!\n");
#elif defined(_CFMC)
        uint32_t un32WdtDefault = CONFIG_WDT_DEFAULT;
        (void)HAL_CFMC_Init();
        (void)HAL_CFMC_Write(CONFIG_COA_WDT_ADDR, (uint8_t *)&un32WdtDefault, sizeof(uint32_t));
        bNeedUpdate = true;
        LOG("\n Set WDT default configuration at Debug!\n");
#endif
    }
   
    if(bNeedUpdate == true)
    {
        LOG("\n Auto reset!!!\n");
        SystemDelayMS(50);
        HAL_SCU_SetSWReset();
    }
}
#endif

static void EX_SetModule(void)
{

    EX_SCU();

#if defined(SCU_CLK_TC)
    EX_SCU_CLK();
#endif

#if defined(SCU_LVD_TC)
    EX_SCU_LVD();
#endif

#if defined(SCU_PWR_TC)
    EX_SCU_PWR();
#endif

#if defined(PCU_TC)
    EX_PCU();
#endif

#if defined(TIMER1_TC)
    EX_TIMER1();
#endif

#if defined(TIMER2_TC)
    EX_TIMER2();
#endif

#if defined(TIMER3_TC)
    EX_TIMER3();
#endif

#if defined(TIMER4_TC)
    EX_TIMER4();
#endif

#if defined(TIMER4E_TC)
    EX_TIMER4E();
#endif

#if defined(TIMER5_TC)
    EX_TIMER5();
#endif

#if defined(TIMER6_TC)
    EX_TIMER6();
#endif

#if defined(ADC_TC)
    EX_ADC();
#endif

#if defined(DAC_TC)
    EX_DAC();
#endif

#if defined(CMP_TC)
    EX_CMP();
#endif

#if defined(WDT_TC)
    EX_WDT();
#endif

#if defined(WT_TC)
    EX_WT();
#endif

#if defined(UART_TC)
    EX_UART();
#endif

#if defined(LPUART_TC)
    EX_LPUART();
#endif

#if defined(USART_TC)
    EX_USART();
#endif

#if defined(SPI_TC)
    EX_SPI();
#endif

#if defined(I2C_TC)
    EX_I2C();
#endif

#if defined(EBI_TC)
    EX_EBI();
#endif

#if defined(FLASH_TC)
    EX_FLASH();
#endif

#if defined(LCD_TC)
    EX_LCD();
#endif

#if defined(LED_TC)
    EX_LED();
#endif

#if defined(RTC_TC)
    EX_RTC();
#endif

#if defined(TEMPSENS_TC)
    EX_TEMPSENS();
#endif

#if defined(CRC_TC)
    EX_CRC();
#endif

#if defined(FRT_TC)
    EX_FRT();
#endif

#if defined(RNG_TC)
    EX_RNG();
#endif

#if defined(TRNG_TC)
    EX_TRNG();
#endif

#if defined(AES_TC)
    EX_AES();
#endif

#if defined(MPWM_TC)
    EX_MPWM();
#endif

#if defined(QEI_TC)
    EX_QEI();
#endif

#if defined(PGA_TC)
    EX_PGA();
#endif

#if defined(PWM_TC)
    EX_PWM();
#endif

#if defined(AFE_TC)
    EX_AFE();
#endif

#if defined(COA_TC)
    EX_COA();
#endif

#if defined(VREFBUF_TC)
    EX_VREFBUF();
#endif

}

static void EX_Loop(void)
{
    debug_cmd_execute("lm");

    while(1)
    {
        debug_cmd_process();
    }
}

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

    PRV_PORT_Init();

    (void)HAL_SCU_CLK_SetSrcEnable(SCUCLK_SRC_LSI, true);
    (void)HAL_SCU_CLK_SetSrcEnable(SCUCLK_SRC_HSE, true);
    (void)HAL_SCU_CLK_SetSrcEnable(SCUCLK_SRC_HSI, true);
    (void)HAL_SCU_CLK_SetMClk(&tMClkCfg);


#if (CONFIG_DEBUG == 1)
    /* configure serial port */

#if (EX_MULTI_FLASH == 1)
    /* waiting for initializing external device serial port */
    SystemDelayMS(120);
#endif

    /* some of the chipsets must configure serial port like below */
    HAL_PCU_SetInOutMode((PCU_ID_e)DEBUG_PORT_ID,(PCU_PIN_ID_e)DEBUG_RX_PORT_ID,PCU_INOUT_INPUT); /* For Debug */
    HAL_PCU_SetPullUpDown((PCU_ID_e)DEBUG_PORT_ID,(PCU_PIN_ID_e)DEBUG_RX_PORT_ID,PCU_PUPD_UP); /* For Debug */
    HAL_PCU_SetInOutMode((PCU_ID_e)DEBUG_PORT_ID,(PCU_PIN_ID_e)DEBUG_TX_PORT_ID,PCU_INOUT_OUTPUT_PUSH_PULL); /* For Debug */

    /* set port as a serial tx/rx mode */
    HAL_PCU_SetAltMode((PCU_ID_e)DEBUG_PORT_ID,(PCU_PIN_ID_e)DEBUG_RX_PORT_ID,(PCU_ALT_e)DEBUG_RX_ALT_ID); /* For Debug */
    HAL_PCU_SetAltMode((PCU_ID_e)DEBUG_PORT_ID,(PCU_PIN_ID_e)DEBUG_TX_PORT_ID,(PCU_ALT_e)DEBUG_TX_ALT_ID); /* For Debug */
    Debug_Init();

#if defined(CONFIG_MODULE_COA)
    /* Set default configuration to COA */
    EX_SetCOADefault();
#endif

    GetChipSetInfo();
#endif

    EX_SetModule();

    EX_Loop();

    return 0;
}

/* --------------------------------- End Of File ------------------------------ */
