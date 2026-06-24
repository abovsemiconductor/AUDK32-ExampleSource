/**
 *******************************************************************************
 * @file        airclock_sensor.h
 * @author      ABOV R&D Division
 * @brief       EXAMPLE AirClock Sensor Driver Interface
 *
 * Copyright 2025 ABOV Semiconductor Co.,Ltd. All rights reserved.
 *
 * This file is licensed under terms that are found in the LICENSE file
 * located at Document directory.
 * If this file is delivered or shared without applicable license terms,
 * the terms of the BSD-3-Clause license shall be applied.
 * Reference: https://opensource.org/licenses/BSD-3-Clause
 ******************************************************************************/

#ifndef _AIRCLOCK_SENSOR_H_
#define _AIRCLOCK_SENSOR_H_

#ifdef __cplusplus
extern "C"
{
#endif

/**
 *******************************************************************************
 * @brief       Initialize TCS(Temperature and Capacitive Sensing Microcontroller).
 * @return      void : none
 ******************************************************************************/
void EX_TCS_Init(void);

/**
 *******************************************************************************
 * @brief       Assertion.
 * @return      int : a non-zero on error, otherwise 0
 ******************************************************************************/
int EX_TCS_I2C_Single_Measure(uint16_t *pun16Hc, uint16_t *pun16Tc);

#ifdef __cplusplus
}
#endif

#endif /* _AIRCLOCK_SENSOR_H_ */
