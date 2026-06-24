/**
 *******************************************************************************
 * @file        adc_app.c
 * @author      ABOV R&D Division
 * @brief       Simple Application for ADC peripheral
 *
 * Copyright 2026 ABOV Semiconductor Co.,Ltd. All rights reserved.
 *
 * This file is licensed under terms that are found in the LICENSE file
 * located at Document directory.
 * If this file is delivered or shared without applicable license terms,
 * the terms of the BSD-3-Clause license shall be applied.
 * Reference: https://opensource.org/licenses/BSD-3-Clause
 ******************************************************************************/

#include "abov_config.h"
#include "abov_simpleapp_config.h"

#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"

#if (CONFIG_APP_ADC == 1)

extern void ADC_INIT_Single(void);
extern void ADC_INIT_Sequential(void);
extern void ADC_INIT_Multiple(void);
extern void ADC_INIT_Burst(void);
extern void ADC_INIT_Comparison(void);

void ADC_App(void)
{
#if ADC_APP == ADC_APP_MODE_SINGLE
    ADC_INIT_Single();
#elif ADC_APP == ADC_APP_MODE_SEQUENTIAL
    ADC_INIT_Sequential();
#elif ADC_APP == ADC_APP_MODE_MULTIPLE
    ADC_INIT_Multiple();
#elif ADC_APP == ADC_APP_MODE_BURST
    ADC_INIT_Burst();
#elif ADC_APP == ADC_APP_MODE_COMPARISON
    ADC_INIT_Comparison();
#endif
}
#endif

