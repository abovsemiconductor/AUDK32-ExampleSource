/**
 *******************************************************************************
 * @file        ex_config.h
 * @author      ABOV R&D Division
 * @brief       Example Config A31C11x
 *
 * Copyright 2025 ABOV Semiconductor Co.,Ltd. All rights reserved.
 *
 * This file is licensed under terms that are found in the LICENSE file
 * located at Document directory.
 * If this file is delivered or shared without applicable license terms,
 * the terms of the BSD-3-Clause license shall be applied.
 * Reference: https://opensource.org/licenses/BSD-3-Clause
 ******************************************************************************/

#ifndef _EX_CONFIG_H_
#define _EX_CONFIG_H_

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(EXTRN_SUBFAMILY_A31C11x)
#define EXAMPLE_FIXED_PERI_MAPPING
#define EX_ADC_TRG_TYPE_INDEPENDENT
#define EX_ADC_PRINT_RESULT_ONLY
#define EX_ADC_DISABLE_HELP_CMD
#define EXTRN_SINGLE_ADC

#define EX_ADC_LOG_OFF

#endif

#ifdef __cplusplus
}
#endif

#endif /* _EX_CONFIG_H_ */
