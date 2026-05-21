/**
 *******************************************************************************
 * @file        ex_config.h
 * @author      ABOV R&D Division
 * @brief       Example Config A31L12x
 *
 * Copyright 2024 ABOV Semiconductor Co.,Ltd. All rights reserved.
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

#if defined(EXTRN_SUBFAMILY_A31L12x)

/* EX_ADC */
#define EX_ADC_TRG_TYPE_SHARE
#define EX_ADC_OVERSAMPLING

/* EX_ADC */
#define EX_AES_MODE_SELECT

/* EX_CRC */
#define EX_CRC_INPUT_CONF_MODE

/* EX_COA */
#define EX_COA_LVR_WTIDKY
#define EX_COA_LVR_WTIDKY_VALUE    0x9D58

/* EX_CMP */
#define EX_CMP_INTERNAL_REFERENCE_LEVEL

/* EX_SCU_PWR */
#define EX_SCU_PWR_NO_IRQ

#endif

#ifdef __cplusplus
}
#endif

#endif /* _EX_CONFIG_H_ */
