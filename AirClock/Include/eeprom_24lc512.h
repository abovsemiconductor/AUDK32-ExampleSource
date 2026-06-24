/**
 *******************************************************************************
 * @file        eeprom_24lc512.h
 * @author      ABOV R&D Division
 * @brief       EXAMPLE 24LC512 EEPROM Driver Interface
 *
 * Copyright 2025 ABOV Semiconductor Co.,Ltd. All rights reserved.
 *
 * This file is licensed under terms that are found in the LICENSE file
 * located at Document directory.
 * If this file is delivered or shared without applicable license terms,
 * the terms of the BSD-3-Clause license shall be applied.
 * Reference: https://opensource.org/licenses/BSD-3-Clause
 ******************************************************************************/

#ifndef _24LC512_EEPROM_H_
#define _24LC512_EEPROM_H_

#ifdef __cplusplus
extern "C"
{
#endif

HAL_ERR_e EX_EEPROM_Init(void);
HAL_ERR_e EX_EEPROM_Read(uint32_t un32Addr, uint8_t *pun8Buf, uint32_t un32Len);
HAL_ERR_e EX_EEPROM_Write(uint32_t un32Addr, uint8_t *pun8Buf, uint32_t un32Len);

#ifdef __cplusplus
}
#endif

#endif /* _24LC512_EEPROM_H_ */
