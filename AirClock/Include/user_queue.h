/**
 *******************************************************************************
 * @file        user_queue.h
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

#pragma once
#ifdef __cplusplus
extern "C"
{
#endif

#include "hal_common.h"

typedef struct {
    volatile uint16_t head;
    volatile uint16_t tail;
    uint16_t capacity;
    uint16_t item_size;
    uint8_t* buf;
    uint16_t mask;
} queue_t;

bool queue_init(queue_t* q, void* buffer, uint16_t capacity, uint16_t item_size);
bool queue_push(queue_t* q, const void* item);
bool queue_pop(queue_t* q, void* out_item);
static inline bool queue_is_empty(const queue_t* q) { return q->head == q->tail; }
bool  queue_is_full(const queue_t* q);
uint16_t queue_count(const queue_t* q);


#ifdef __cplusplus
}
#endif

