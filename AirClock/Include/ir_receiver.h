/**
 *******************************************************************************
 * @file        ir_receiver.h
 * @author      ABOV R&D Division
 * @brief       EXAMPLE IR Receiver Driver Interface
 *
 * Copyright 2025 ABOV Semiconductor Co.,Ltd. All rights reserved.
 *
 * This file is licensed under terms that are found in the LICENSE file
 * located at Document directory.
 * If this file is delivered or shared without applicable license terms,
 * the terms of the BSD-3-Clause license shall be applied.
 * Reference: https://opensource.org/licenses/BSD-3-Clause
 ******************************************************************************/

#ifndef _IR_RECEIVER_H_
#define _IR_RECEIVER_H_

#ifdef __cplusplus
extern "C"
{
#endif
#include "virtual_key.h"

#define IR_REMOTE_CMD_UP       0x46
#define IR_REMOTE_CMD_DOWN     0x15
#define IR_REMOTE_CMD_LEFT     0x44
#define IR_REMOTE_CMD_RIGHT    0x43
#define IR_REMOTE_CMD_OK       0x40
#define IR_REMOTE_CMD_0        0x52
#define IR_REMOTE_CMD_1        0x16
#define IR_REMOTE_CMD_2        0x19
#define IR_REMOTE_CMD_3        0x0D
#define IR_REMOTE_CMD_4        0x0C
#define IR_REMOTE_CMD_5        0x18
#define IR_REMOTE_CMD_6        0x5E
#define IR_REMOTE_CMD_7        0x08
#define IR_REMOTE_CMD_8        0x1C
#define IR_REMOTE_CMD_9        0x5A
#define IR_REMOTE_CMD_STAR     0x42
#define IR_REMOTE_CMD_HASH     0x4A

typedef struct {
    uint32_t data;
    uint8_t repeated;
} irr_msg_t;

typedef struct {
    uint8_t addr;
    uint8_t naddr;
    uint8_t cmd;
    uint8_t ncmd;
} nec_rev_t;

/**
 *******************************************************************************
 * @brief       Initialize TCS(Temperature and Capacitive Sensing Microcontroller).
 * @return      void : none
 ******************************************************************************/
void  EX_IR_Init(void);

/**
 *******************************************************************************
 * @brief       Re-initialize TCS(Temperature and Capacitive Sensing Microcontroller).
 * @return      void : none
 ******************************************************************************/
void IR_Reinit(void);
void IR_RegisterKeyCallback(pfnKeyCallback_t handler);

// todo, FreeRTOS
void EX_IR_ProcessEvent(const irr_msg_t *msg);

/**
 *******************************************************************************
 * @brief       IR Recevier manager by Queue.
 * @return      void : none
 ******************************************************************************/
void EX_IR_Receiver_Manager(void);

#ifdef __cplusplus
}
#endif

#endif /* _IR_RECEIVER_H_ */
