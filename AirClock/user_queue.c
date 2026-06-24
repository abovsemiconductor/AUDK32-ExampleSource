/**
 *******************************************************************************
 * @file        user_queue.c
 * @author      ABOV R&D Division
 * @brief       EXAMPLE Queue Implementation
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
#include "hal_common.h"

#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "user_queue.h"

bool queue_init(queue_t* q, void* buffer, uint16_t capacity, uint16_t item_size)
{
    if (!q || !buffer || capacity < 2 || item_size == 0)
    {
        return false;
    }
    q->head = q->tail = 0;
    q->capacity = capacity;
    q->item_size = item_size;
    q->buf = (uint8_t*)buffer;
    return true;
}

static inline uint16_t _next(const queue_t* q, uint16_t i)
{
    i++;
    return (i == q->capacity) ? 0 : i;
}

bool queue_is_full(const queue_t* q)
{
    return _next(q, q->head) == q->tail;
}

uint16_t queue_count(const queue_t* q)
{
    uint16_t h = q->head, t = q->tail;
    if (h >= t)
    {
        return (uint16_t)(h - t);
    }
    return (uint16_t)(q->capacity - (t - h));
}

bool queue_push(queue_t* q, const void* item)
{
    uint16_t next = _next(q, q->head);
    if (next == q->tail)
    {
        // q is full
        return false;
    }
    memcpy(q->buf + (q->head * q->item_size), item, q->item_size);
    q->head = next;
    return true;
}

bool queue_pop(queue_t *q, void *out_item)
{
    if (q->head == q->tail)
    {
        // q is empty
        return false;
    }
    if (out_item)
    {
        memcpy(out_item, q->buf + (q->tail * q->item_size), q->item_size);
    }
    q->tail = _next(q, q->tail);
    return true;
}