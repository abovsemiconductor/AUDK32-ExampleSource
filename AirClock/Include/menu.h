/**
 *******************************************************************************
 * @file        menu.h
 * @author      ABOV R&D Division
 * @brief       menu
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

void menu_init(void);
void menu_run(void);
void render_time(void);
void renter_temp_humi(void);

#ifdef __cplusplus
}
#endif

