/**
 *******************************************************************************
 * @file        flash_setup.h
 * @author      ABOV R&D Division
 * @brief       FLASH Setup header file.
 *
 * Copyright 2026 ABOV Semiconductor Co.,Ltd. All rights reserved.
 *
 * This file is licensed under terms that are found in the LICENSE file
 * located at Document directory.
 * If this file is delivered or shared without applicable license terms,
 * the terms of the BSD-3-Clause license shall be applied.
 * Reference: https://opensource.org/licenses/BSD-3-Clause
 ******************************************************************************/

#ifndef _FLASH_SETUP_H_
#define _FLASH_SETUP_H_

#ifdef __cplusplus
extern "C"
{
#endif
#include <stdint.h>

/* MX25L25645G Flash Memory */

/* Read/Write Array Commands */
typedef enum
{
    MX25_CMD_RDID       = 0x9F, /* READ IDENTIFICATION (JEDEC ID) */
    MX25_CMD_QPIID      = 0xAF, /* QUAD I/O READ IDENTIFICATION */
    MX25_CMD_RDSFDP     = 0x5A, /* READ SFDP */
    MX25_CMD_RDUID      = 0x4B, /* READ UNIQUE ID */
} MX25_IDCMD_e;

typedef enum
{
    MX25_CMD_RDSR       = 0x05, /* READ STATUS REGISTER */
    MX25_CMD_WRSR       = 0x01, /* WRITE STATUS + CONFIG REGISTER */
    MX25_CMD_RDCR       = 0x15, /* READ CONFIGURATION REGISTER */
} MX25_STSCMD_e;

typedef enum
{
    MX25_CMD_READ       = 0x03, /* NORMAL READ */
    MX25_CMD_FREAD      = 0x0B, /* FAST READ (1-1-1) */
    MX25_CMD_DREAD      = 0x3B, /* DUAL OUTPUT READ (1-1-2) */
    MX25_CMD_QREAD      = 0x6B, /* QUAD OUTPUT READ (1-1-4) */
    MX25_CMD_DIOREAD    = 0xBB, /* DUAL I/O READ (1-2-2) */
    MX25_CMD_QIOREAD    = 0xEB, /* QUAD I/O READ (1-4-4) */
} MX25_READCMD_e;

typedef enum
{
    MX25_CMD_WREN       = 0x06, /* WRITE ENABLE */
    MX25_CMD_WRDI       = 0x04, /* WRITE DISABLE */
    MX25_CMD_PP         = 0x02, /* PAGE PROGRAM */
    MX25_CMD_QPP        = 0x38, /* QUAD PAGE PROGRAM */
} MX25_PGMCMD_e;

typedef enum
{
    MX25_CMD_SE4K       = 0x20, /* SECTOR ERASE (4KB) */
    MX25_CMD_BE32K      = 0x52, /* BLOCK ERASE (32KB) */
    MX25_CMD_BE64K      = 0xD8, /* BLOCK ERASE (64KB) */
    MX25_CMD_CE         = 0xC7, /* CHIP ERASE */
} MX25_ERASECMD_e;

typedef enum
{
    MX25_CMD_DP         = 0xB9, /* DEEP POWER DOWN */
    MX25_CMD_RDP        = 0xAB, /* RELEASE FROM POWER DOWN */
    MX25_CMD_RSTEN      = 0x66, /* RESET ENABLE */
    MX25_CMD_RST        = 0x99, /* RESET MEMORY */
} MX25_PWRRCMD_e;

typedef enum
{
    MX25_SR_WIP     = (1u << 0), /* WRITE IN PROGRESS */
    MX25_SR_WEL     = (1u << 1), /* WRITE ENABLE LATCH */
    MX25_SR_BP0     = (1u << 2), /* BLOCK PROTECT BIT 0 */
    MX25_SR_BP1     = (1u << 3), /* BLOCK PROTECT BIT 1 */
    MX25_SR_BP2     = (1u << 4), /* BLOCK PROTECT BIT 2 */
    MX25_SR_BP3     = (1u << 5), /* BLOCK PROTECT BIT 3 */
    MX25_SR_QE_SR   = (1u << 6), /* QUAD ENABLE (legacy / not used) */
    MX25_SR_SRWD    = (1u << 7), /* STATUS REGISTER WRITE PROTECT */
} MX25_SR_e;

typedef enum
{
    MX25_CR_ODS0    = (1u << 0), /* OUTPUT DRIVER STRENGTH BIT 0 */
    MX25_CR_ODS1    = (1u << 1), /* OUTPUT DRIVER STRENGTH BIT 1 */
    MX25_CR_TB      = (1u << 3), /* TOP / BOTTOM PROTECT */
} MX25_CR_e;

#ifdef __cplusplus
}
#endif
#endif /* _FLASH_SETUP_H_ */

