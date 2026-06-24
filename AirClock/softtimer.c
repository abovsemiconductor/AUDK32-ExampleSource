/**
 *******************************************************************************
 * @file        softtimmer.c
 * @author      ABOV R&D Division
 * @brief       EXAMPLE Software Timer Implementation
 *
 * Copyright 2025 ABOV Semiconductor Co.,Ltd. All rights reserved.
 *
 * This file is licensed under terms that are found in the LICENSE file
 * located at Document directory.
 * If this file is delivered or shared without applicable license terms,
 * the terms of the BSD-3-Clause license shall be applied.
 * Reference: https://opensource.org/licenses/BSD-3-Clause
 ******************************************************************************/
#include "abov_config.h"

#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"

#include "softtimer.h"

typedef struct {
    uint8_t  in_use;
    uint8_t  active;
    uint32_t deadline_ms;
    softtimer_cb_t cb;
    void*    user;
} softtimer_t;

static softtimer_t s_timers[SOFTTIMER_MAX];
static volatile uint32_t s_now_ms = 0;

void EX_SoftTimer_Init(void)
{
    memset(s_timers, 0x00, sizeof(softtimer_t) * SOFTTIMER_MAX);
    s_now_ms = 0;
}

void EX_SoftTimer_TickIsr(void)
{
    s_now_ms++;
}

static inline uint32_t _now(void)
{
    return s_now_ms; 
}

uint32_t softtimer_now_ms(void)
{ 
    return s_now_ms; 
}

bool EX_SoftTimer_StartOneshot(uint8_t id, uint32_t delay_ms, softtimer_cb_t cb, void* user)
{
    if (id >= SOFTTIMER_MAX || !cb)
    {
        return false;
    }
        
    s_timers[id].in_use = 1;
    s_timers[id].active = 1;
    s_timers[id].cb = cb;
    s_timers[id].user = user;
    s_timers[id].deadline_ms = _now() + delay_ms;
    return true;
}

void EX_SoftTimer_Cancel(uint8_t id)
{
    if (id >= SOFTTIMER_MAX)
    {
        return;
    }
    s_timers[id].active = 0;
}

void EX_SoftTimer_Manager(void)
{
    uint32_t now = _now();
    for (uint8_t i = 0; i < SOFTTIMER_MAX; i++)
    {
        softtimer_t* t = &s_timers[i];
        if (!t->in_use || !t->active) 
            continue;

        if ((int32_t)(now - t->deadline_ms) >= 0)
        {
            // fire!
            t->active = 0;
            softtimer_cb_t cb = t->cb;
            void* user = t->user;
            if (cb)
            {
                cb(user);
            }
        }
    }
}
