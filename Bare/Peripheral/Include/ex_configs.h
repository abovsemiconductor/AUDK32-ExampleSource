/**
 *******************************************************************************
 * @file        ex_configs.h
 * @author      ABOV R&D Division
 * @brief       Example Config
 *
 * Copyright 2024 ABOV Semiconductor Co.,Ltd. All rights reserved.
 *
 * This file is licensed under terms that are found in the LICENSE file
 * located at Document directory.
 * If this file is delivered or shared without applicable license terms,
 * the terms of the BSD-3-Clause license shall be applied.
 * Reference: https://opensource.org/licenses/BSD-3-Clause
 ******************************************************************************/

#ifndef _EXAMPLE_CONFIGS_H_
#define _EXAMPLE_CONFIGS_H_

#ifdef __cplusplus
extern "C"
{
#endif

/* A31xxxx */
#if defined(EXTRN_SUBFAMILY_A31G11x)
#include "Config/A31xxxx/A31G11x/ex_config.h"
#elif defined(EXTRN_SUBFAMILY_A31G12x)
#include "Config/A31xxxx/A31G12x/ex_config.h"
#elif defined(EXTRN_SUBFAMILY_A31G21x)
#include "Config/A31xxxx/A31G21x/ex_config.h"
#elif defined(EXTRN_SUBFAMILY_A31G22x)
#include "Config/A31xxxx/A31G22x/ex_config.h"
#elif defined(EXTRN_SUBFAMILY_A31G31x)
#include "Config/A31xxxx/A31G31x/ex_config.h"
#elif defined(EXTRN_SUBFAMILY_A31G32x)
#include "Config/A31xxxx/A31G32x/ex_config.h"
#elif defined(EXTRN_SUBFAMILY_A31T21x)
#include "Config/A31xxxx/A31T21x/ex_config.h"
#elif defined(EXTRN_SUBFAMILY_A31L21x)
#include "Config/A31xxxx/A31L21x/ex_config.h"
#elif defined(EXTRN_SUBFAMILY_A31L12x)
#include "Config/A31xxxx/A31L12x/ex_config.h"
#elif defined(EXTRN_SUBFAMILY_A31L22x)
#include "Config/A31xxxx/A31L22x/ex_config.h"
#elif defined(EXTRN_SUBFAMILY_A31C14x)
#include "Config/A31xxxx/A31C14x/ex_config.h"
#elif defined(EXTRN_SUBFAMILY_A31C12x)
#include "Config/A31xxxx/A31C12x/ex_config.h"
#elif defined(EXTRN_SUBFAMILY_A31S13x)
#include "Config/A31xxxx/A31S13x/ex_config.h"
#elif defined(EXTRN_SUBFAMILY_A31G33x)
#include "Config/A31xxxx/A31G33x/ex_config.h"
#elif defined(EXTRN_SUBFAMILY_A31G34x)
#include "Config/A31xxxx/A31G34x/ex_config.h"
#elif defined(EXTRN_SUBFAMILY_A31T41x)
#include "Config/A31xxxx/A31T41x/ex_config.h"

/* A33xxxx */
#elif defined(EXTRN_SUBFAMILY_A33G52x)
#include "Config/A33xxxx/A33G52x/ex_config.h"
#elif defined(EXTRN_SUBFAMILY_A33G53x)
#include "Config/A33xxxx/A33G53x/ex_config.h"
#elif defined(EXTRN_SUBFAMILY_A33M11x)
#include "Config/A33xxxx/A33M11x/ex_config.h"

/* A34xxxx */
#elif defined(EXTRN_SUBFAMILY_A34M41x)
#include "Config/A34xxxx/A34M41x/ex_config.h"
#elif defined(EXTRN_SUBFAMILY_A34M42x)
#include "Config/A34xxxx/A34M42x/ex_config.h"
#elif defined(EXTRN_SUBFAMILY_A34L71x)
#include "Config/A34xxxx/A34L71x/ex_config.h"
#elif defined(EXTRN_SUBFAMILY_A34G43x)
#include "Config/A34xxxx/A34G43x/ex_config.h"
#elif defined(EXTRN_SUBFAMILY_A34M71x)
#include "Config/A34xxxx/A34M71x/ex_config.h"

#else
#error "There is no ex_config.h for Subfamily."
#endif

#ifdef __cplusplus
}
#endif

#endif /* _EXAMPLE_COMMONS_H_ */
