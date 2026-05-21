/**
 *******************************************************************************
 * @file        ex_peripheral.h
 * @author      ABOV R&D Division
 * @brief       Example Function List
 *
 * Copyright 2022 ABOV Semiconductor Co.,Ltd. All rights reserved.
 *
 * This file is licensed under terms that are found in the LICENSE file
 * located at Document directory.
 * If this file is delivered or shared without applicable license terms,
 * the terms of the BSD-3-Clause license shall be applied.
 * Reference: https://opensource.org/licenses/BSD-3-Clause
 ******************************************************************************/

#ifndef _EXAMPLE_H_
#define _EXAMPLE_H_

#ifdef __cplusplus
extern "C"
{
#endif

/**
 *******************************************************************************
 * @brief       SCU command-line input example
 * @param[in]   void
 * @return      void
 ******************************************************************************/
void EX_SCU(void);

/**
 *******************************************************************************
 * @brief       SCU CLK command-line input example
 * @param[in]   void
 * @return      void
 ******************************************************************************/
void EX_SCU_CLK(void);

/**
 *******************************************************************************
 * @brief       SCU LVD command-line input example
 * @param[in]   void
 * @return      void
 ******************************************************************************/
void EX_SCU_LVD(void);

/**
 *******************************************************************************
 * @brief       SCU PWR command-line input example
 * @param[in]   void
 * @return      void
 ******************************************************************************/
void EX_SCU_PWR(void);

/**
 *******************************************************************************
 * @brief       SCU PCU command-line input example
 * @param[in]   void
 * @return      void
 ******************************************************************************/
void EX_PCU(void);

/**
 *******************************************************************************
 * @brief       Timer1 command-line input example
 * @param[in]   void
 * @return      void
 ******************************************************************************/
void EX_TIMER1(void);

/**
 *******************************************************************************
 * @brief       Timer2 command-line input example
 * @param[in]   void
 * @return      void
 ******************************************************************************/
void EX_TIMER2(void);

/**
 *******************************************************************************
 * @brief       Timer3 command-line input example
 * @param[in]   void
 * @return      void
 ******************************************************************************/
void EX_TIMER3(void);

/**
 *******************************************************************************
 * @brief       Timer4 command-line input example
 * @param[in]   void
 * @return      void
 ******************************************************************************/
void EX_TIMER4(void);

/**
 *******************************************************************************
 * @brief       Timer4e command-line input example
 * @param[in]   void
 * @return      void
 ******************************************************************************/
void EX_TIMER4E(void);

/**
 *******************************************************************************
 * @brief       Timer5 command-line input example
 * @param[in]   void
 * @return      void
 ******************************************************************************/
void EX_TIMER5(void);

/**
 *******************************************************************************
 * @brief       Timer6 command-line input example
 * @param[in]   void
 * @return      void
 ******************************************************************************/
void EX_TIMER6(void);

/**
 *******************************************************************************
 * @brief       ADC command-line input example
 * @param[in]   void
 * @return      void
 ******************************************************************************/
void EX_ADC(void);

/**
 *******************************************************************************
 * @brief       DAC command-line input example
 * @param[in]   void
 * @return      void
 ******************************************************************************/
void EX_DAC(void);

/**
 *******************************************************************************
 * @brief       CMP command-line input example
 * @param[in]   void
 * @return      void
 ******************************************************************************/
void EX_CMP(void);

/**
 *******************************************************************************
 * @brief       WDT command-line input example
 * @param[in]   void
 * @return      void
 ******************************************************************************/
void EX_WDT(void);

/**
 *******************************************************************************
 * @brief       WT command-line input example
 * @param[in]   void
 * @return      void
 ******************************************************************************/
void EX_WT(void);

/**
 *******************************************************************************
 * @brief       UART command-line input example
 * @param[in]   void
 * @return      void
 ******************************************************************************/
void EX_UART(void);

/**
 *******************************************************************************
 * @brief       LPUART command-line input example
 * @param[in]   void
 * @return      void
 ******************************************************************************/
void EX_LPUART(void);

/**
 *******************************************************************************
 * @brief       USART command-line input example
 * @param[in]   void
 * @return      void
 ******************************************************************************/
void EX_USART(void);

/**
 *******************************************************************************
 * @brief       SPI command-line input example
 * @param[in]   void
 * @return      void
 ******************************************************************************/
void EX_SPI(void);

/**
 *******************************************************************************
 * @brief       I2C command-line input example
 * @param[in]   void
 * @return      void
 ******************************************************************************/
void EX_I2C(void);

/**
 *******************************************************************************
 * @brief       EBI command-line input example
 * @param[in]   void
 * @return      void
 ******************************************************************************/
void EX_EBI(void);

/**
 *******************************************************************************
 * @brief       FLASH command-line input example
 * @param[in]   void
 * @return      void
 ******************************************************************************/
void EX_FLASH(void);

/**
 *******************************************************************************
 * @brief       LCD command-line input example
 * @param[in]   void
 * @return      void
 ******************************************************************************/
void EX_LCD(void);

/**
 *******************************************************************************
 * @brief       LED command-line input example
 * @param[in]   void
 * @return      void
 ******************************************************************************/
void EX_LED(void);

/**
 *******************************************************************************
 * @brief       RTC command-line input example
 * @param[in]   void
 * @return      void
 ******************************************************************************/
void EX_RTC(void);

/**
 *******************************************************************************
 * @brief       Temp Senser command-line input example
 * @param[in]   void
 * @return      void
 ******************************************************************************/
void EX_TEMPSENS(void);

/**
 *******************************************************************************
 * @brief       CRC command-line input example
 * @param[in]   void
 * @return      void
 ******************************************************************************/
void EX_CRC(void);

/**
 *******************************************************************************
 * @brief       FRT command-line input example
 * @param[in]   void
 * @return      void
 ******************************************************************************/
void EX_FRT(void);

/**
 *******************************************************************************
 * @brief       RNG command-line input example
 * @param[in]   void
 * @return      void
 ******************************************************************************/
void EX_RNG(void);

/**
 *******************************************************************************
 * @brief       TRNG command-line input example
 * @param[in]   void
 * @return      void
 ******************************************************************************/
void EX_TRNG(void);

/**
 *******************************************************************************
 * @brief       AES command-line input example
 * @param[in]   void
 * @return      void
 ******************************************************************************/
void EX_AES(void);

/**
 *******************************************************************************
 * @brief       MPWM command-line input example
 * @param[in]   void
 * @return      void
 ******************************************************************************/
void EX_MPWM(void);

/**
 *******************************************************************************
 * @brief       QEI command-line input example
 * @param[in]   void
 * @return      void
 ******************************************************************************/
void EX_QEI(void);

/**
 *******************************************************************************
 * @brief       PGA command-line input example
 * @param[in]   void
 * @return      void
 ******************************************************************************/
void EX_PGA(void);

/**
 *******************************************************************************
 * @brief       PWM command-line input example
 * @param[in]   void
 * @return      void
 ******************************************************************************/
void EX_PWM(void);

/**
 *******************************************************************************
 * @brief       AFE command-line input example
 * @param[in]   void
 * @return      void
 ******************************************************************************/
void EX_AFE(void);

/**
 *******************************************************************************
 * @brief       COA command-line input example
 * @param[in]   void
 * @return      void
 ******************************************************************************/
void EX_COA(void);

/**
 *******************************************************************************
 * @brief       VREFBUF command-line input example
 * @param[in]   void
 * @return      void
 ******************************************************************************/
void EX_VREFBUF(void);

#ifdef __cplusplus
}
#endif

#endif /* _EXAMPLE_H_ */
