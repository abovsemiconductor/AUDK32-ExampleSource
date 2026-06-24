/**
 *******************************************************************************
 * @file        menu.h
 * @author      ABOV R&D Division
 * @brief       key event
 *
 * Copyright 2025 ABOV Semiconductor Co.,Ltd. All rights reserved.
 *
 * This file is licensed under terms that are found in the LICENSE file
 * located at Document directory.
 * If this file is delivered or shared without applicable license terms,
 * the terms of the BSD-3-Clause license shall be applied.
 * Reference: https://opensource.org/licenses/BSD-3-Clause
 ******************************************************************************/

#pragma once
#ifdef __cplusplus
extern "C"
{
#endif

typedef enum { 
    KEY_NONE = 0,
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_OK,
    KEY_MENU
} VKey;

typedef void (*pfnKeyCallback_t)(VKey key);

#ifdef __cplusplus
}
#endif

