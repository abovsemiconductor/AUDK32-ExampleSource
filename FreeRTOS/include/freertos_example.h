/**
 *******************************************************************************
 * @file        FREERTOS_EXAMPLE_APPL.h
 * @author      ABOV R&D Division
 * @brief       Top level configuration file
 *
 * Copyright 2022 ABOV Semiconductor Co.,Ltd. All rights reserved.
 *
 * This file is licensed under terms that are found in the LICENSE file
 * located at Document directory.
 * If this file is delivered or shared without applicable license terms,
 * the terms of the BSD-3-Clause license shall be applied.
 * Reference: https://opensource.org/licenses/BSD-3-Clause
 ******************************************************************************/

/** @addtogroup VENDOR ABOV Semiconductor Co., Ltd.
  * @{
*/


/* @addtogroup FreeRTOS Example
  * @{
*/

#ifndef ABOV_FREERTOS_EXAMPLE_H
#define ABOV_FREERTOS_EXAMPLE_H

/*
//-------- <<< Use Configuration Wizard in Context Menu >>> -------------------
*/

// <h> FreeRTOS Example
// <o> API Version
//  <i> Select API Version
//     <0=> NATIVE
//     <1=> CMSIS-RTOS V1
//     <2=> CMSIS-RTOS V2
//  <i> Default: NATIVE
#define FREERTOS_API_VERSION 0

// <o> Example Application
//  <i> Select Example Application
//     <0=> Led Blinking with Task Only
//     <1=> Led Blinking with Task + Message
//     <2=> Led Blinking with Task + Semaphore
//     <3=> Led Blinking with ISR + Task + Message
//  <i> Default: Led Blinking with ISR + Task + Message
#define FREERTOS_EXAMPLE_APPL 3
// </h> End of FreeRTOS Example


#if(FREERTOS_API_VERSION == 0)
#define FREERTOS_EXAMPE_API_NATIVE

#elif(FREERTOS_API_VERSION == 1)
#define FREERTOS_EXAMPE_API_CMSIS_RTOS_V1

#elif(FREERTOS_API_VERSION == 2)
#define FREERTOS_EXAMPE_API_CMSIS_RTOS_V2

#endif  /* FREERTOS_API_VERSION */


#if(FREERTOS_EXAMPLE_APPL == 0)
#define FREERTOS_EXAMPE_LED_BLIKING_TASK_ONLY

#elif(FREERTOS_EXAMPLE_APPL == 1)
#define FREERTOS_EXAMPE_LED_BLIKING_TASK_MESSAGE

#elif(FREERTOS_EXAMPLE_APPL == 2)
#define FREERTOS_EXAMPE_LED_BLIKING_TASK_SEMAPHORE

#elif(FREERTOS_EXAMPLE_APPL == 3)
#define FREERTOS_EXAMPE_LED_BLIKING_ISR_TASK_MESSAGE

#endif  /* FREERTOS_EXAMPLE_APPL */


void freertos_native(void);
void cmsis_rtos_v1(void);
void cmsis_rtos_v2(void);

#endif /* ABOV_FREERTOS_EXAMPLE_H */
/** @} */ /* End of group FreeRTOS Example */

/** @} */ /* End of group VENDOR ABOV Semiconductor Co., Ltd. */
