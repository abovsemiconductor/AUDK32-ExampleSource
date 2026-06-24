/**
 *******************************************************************************
 * @file        snor_w25q16jv.h
 * @author      ABOV R&D Division
 * @brief       EXAMPLE W25Q16JV SPI NOR Driver Interface
 *
 * Copyright 2025 ABOV Semiconductor Co.,Ltd. All rights reserved.
 *
 * This file is licensed under terms that are found in the LICENSE file
 * located at Document directory.
 * If this file is delivered or shared without applicable license terms,
 * the terms of the BSD-3-Clause license shall be applied.
 * Reference: https://opensource.org/licenses/BSD-3-Clause
 ******************************************************************************/

#ifndef _W25Q16JV_SNOR_H_
#define _W25Q16JV_SNOR_H_

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief SNOR Erase size 
 */
typedef enum {
    EX_SNOR_ERASE_4K,
    EX_SNOR_ERASE_32K,
    EX_SNOR_ERASE_64K,
    EX_SNOR_ERASE_CHIP,
    EX_SNOR_ERASE_MAX
} EX_SNOR_ERASE_e;

/**
 *******************************************************************************
 * @brief       Initialize SNOR.
 * @return      void : None
 ******************************************************************************/
void EX_SNOR_Init(void);

/**
 *******************************************************************************
 * @brief       Get SNOR ID.
 * @return      void : None
 ******************************************************************************/
void EX_SNOR_GetID(uint8_t *pun8Id);

/**
 *******************************************************************************
 * @brief       Set Erase by address and size::EX_SNOR_ERASE_e.
 * @param[in]   un32Addr : SNOR address.
 * @param[in]   eErase: Erase size::EX_SNOR_ERASE_e.
 * @return      void : None
 ******************************************************************************/
void EX_SNOR_SetErase(uint32_t un32Addr, EX_SNOR_ERASE_e eErase);

/**
 *******************************************************************************
 * @brief       Write data to SNOR
 * @param[in]   un32Addr : SNOR address.
 * @param[in]   *pun8Data : write data buffer pointer.
 * @param[in]   un32Len : write data size.
 * @return      void : None
 ******************************************************************************/
void EX_SNOR_Write(uint32_t un32Addr, uint8_t *pun8Data, uint32_t un32Len);

/**
 *******************************************************************************
 * @brief       Read data from SNOR
 * @param[in]   un32Addr : SNOR address.
 * @param[in]   *pun8Data : read data buffer pointer.
 * @param[in]   un32Len : read data size.
 * @return      void : None
 ******************************************************************************/
void EX_SNOR_Read(uint32_t un32Addr, uint8_t *pun8Data, uint32_t un32Len);

#ifdef __cplusplus
}
#endif

#endif /* _W25Q16JV_SNOR_H_ */
