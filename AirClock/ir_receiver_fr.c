/**
 *******************************************************************************
 * @file        ir_receiver.c
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

#include "abov_config.h"
#include "debug_log.h"
#include "debug.h"

#include "hal_timer1.h"
#include "hal_frt.h"
#include "hal_pcu.h"
#include "gpio_shared_irq.h"
#include "ir_receiver.h"
#include "app_event.h"

#include "FreeRTOS.h"

typedef enum {
    DEBUG_CLEAR,
    DEBUG_ERROR,
    DEBUG_TRACE,
    DEBUG_ALL
} DEBUG_LEVEL_e;

#define DBG_LOG(level)      ((level) <= debug_level && (level) > DEBUG_CLEAR ? 1 : 0)

/* ===== Timing Value (us) & Offset(%) ===== */
#define NEC_TOL_PCT      20
#define NEC_T_LEAD_MARK  9000
#define NEC_T_LEAD_SPACE 4500
#define NEC_T_RPT_SPACE  2250
#define NEC_T_BIT_MARK    560
#define NEC_T_0_SPACE     560
#define NEC_T_1_SPACE    1690

/* Glitch Filter */
#define NEC_MIN_PULSE_US 200

#define LED_TIMER_ID TIMER1_ID_8

#define IR_LED_PORT LED_PORT
#define IR_LED_PIN_ID LED3_PIN_ID
#define IR_LED_BLINK_MS 300

static inline int in_range(uint32_t x, uint32_t target, uint32_t tol_pct)
{
    uint32_t lo = target - (target * tol_pct) / 100;
    uint32_t hi = target + (target * tol_pct) / 100;
    return (x >= lo) && (x <= hi);
}

typedef enum {
    NEC_IDLE = 0,
    NEC_LEAD_SPACE,   // Detected leader mark; expect 4.5ms space next
    NEC_BIT_MARK,     // Expect 560us mark for each bit
    NEC_BIT_SPACE,    // Bit value determined by length of the space
    NEC_REPEAT        // Repeat frame detected
} NEC_STATE_e;

typedef enum {
    NEC_RET_NONE,
    NEC_RET_FRAME_DONE,
    NEC_RET_REPEAT,
    NEC_RET_ERROR
} NEC_RET_e;

typedef struct {
    NEC_STATE_e st;
    uint32_t    bits;       // collecting 32 bits
    int         bit_idx;    // 0..31

    /* flags for output */
    uint8_t     frame_ready; // 1 = received full 32-bit frame
    uint8_t     repeat;      // 1 = repeat frame detected

    /* Debug/Support */
    uint32_t    last_mark_us; // timestamp of last low mark (microseconds)
    uint8_t     last_level;   // last pin level
} nec_decoder_t;

static PCU_Context_t s_tPCUContext[CONFIG_PCU_MAX_COUNT];
static uint8_t debug_level = DEBUG_CLEAR;
static volatile uint32_t last_ticks = 0;
static nec_decoder_t nec;
static pfnKeyCallback_t s_pfnKey = NULL;

/**
 *  FUNCTIONS
**/
static void nec_reset(nec_decoder_t *d)
{
    d->st          = NEC_IDLE;
    d->bits        = 0;
    d->bit_idx     = 0;
    d->frame_ready = 0;
    d->repeat      = 0;
    d->last_mark_us= 0;
}
static void nec_rearm(nec_decoder_t *d)
{
    d->st          = NEC_IDLE;
    d->bits        = 0;
    d->bit_idx     = 0;
    d->last_mark_us= 0;
}

/**
 *  - dt_us : time between last edge and this edge (microseconds)
 *  - level : current pin level (If IR receiver is active-low -> LOW = Mark, HIGH = Space)
 *  Return: 0 = in progress, 1 = valid frame, 2 = repeat frame
 **/
static int nec_on_edge(nec_decoder_t *d, uint32_t dt_us, uint8_t level)
{
    /* Interpret the segment based on level transition:
     * - If previous level was LOW, the just-ended segment is a Mark
     * - If previous level was HIGH, the just-ended segment is a "Space"
     * Note: last_level may be uninitialized on the first call; initialize
     * d->last_level with the current pin level before first use.
     */
    uint8_t prev_level = d->last_level;
    d->last_level = level;

    if (dt_us < NEC_MIN_PULSE_US)
    {
        return NEC_RET_NONE; // Ignore glitch
    }

    if (prev_level == 0)
    {
        /* LOW -> this segment was a Mark */
        uint32_t mark = dt_us;
        if (d->st == NEC_IDLE)
        {
            /* Leader mark candidate */
            if (in_range(mark, NEC_T_LEAD_MARK, NEC_TOL_PCT))
            {
                d->st = NEC_LEAD_SPACE;
            }
        }
        else if (d->st == NEC_BIT_MARK || d->st == NEC_LEAD_SPACE)
        {
            if (!in_range(mark, NEC_T_BIT_MARK, NEC_TOL_PCT))
            {
                nec_reset(d);
            }
            else
            {
                d->st = NEC_BIT_SPACE;
            }
        }
        else if (d->st == NEC_REPEAT)
        {
            /* Final mark (560us) of the repeat sequence */
            if (in_range(mark, NEC_T_BIT_MARK, NEC_TOL_PCT))
            {
                d->repeat = 1;
                nec_rearm(d);
                return NEC_RET_REPEAT; /* repeat frame */
            }
            nec_reset(d);
        }
        else
        {
            /* Otherwise synchronization failed -> reset */
            nec_reset(d);
        }
    }
    else
    {
        /* HIGH -> this segment was a Space */
        uint32_t space = dt_us;
        if (d->st == NEC_LEAD_SPACE)
        {
            /* Leader space: 4.5ms or 2.25ms (repeat) */
            if (in_range(space, NEC_T_LEAD_SPACE, NEC_TOL_PCT))
            {
                d->st = NEC_BIT_MARK;
                d->bits  = 0;
                d->bit_idx = 0;
                return NEC_RET_NONE;
            }
            if (in_range(space, NEC_T_RPT_SPACE, NEC_TOL_PCT))
            {
                d->st = NEC_REPEAT;
                return NEC_RET_NONE;
            }
            nec_reset(d);
        }
        else if (d->st == NEC_BIT_SPACE)
        {
            /* Determine bit 0/1 based on space length */
            if (in_range(space, NEC_T_0_SPACE, NEC_TOL_PCT))
            {
                d->bits >>= 1; /* LSB-first reception example */
                /* If bit is 0, do not OR anything */
            }
            else if (in_range(space, NEC_T_1_SPACE, NEC_TOL_PCT))
            {
                d->bits >>= 1;
                d->bits |= 0x80000000u;  /* Set MSB to '1' and gather bits by shifting right */
            }
            else
            {
                nec_reset(d);
            }

            d->bit_idx++;
            if (d->bit_idx >= 32)
            {
                d->frame_ready = 1;
                return NEC_RET_FRAME_DONE;
            }
            else
            {
                d->st = NEC_BIT_MARK; /* Expect next bit's Mark (560us) */
            }
        }
        else
        {
            /* Otherwise ignore or reset */
        }
    }
    return NEC_RET_NONE;
}

static inline uint32_t ticks_to_us(uint32_t d_ticks)
{
    return d_ticks / (SystemCoreClock / 1000000u);
}

static void led_control(bool on)
{
    if (on == true)
    {
        HAL_PCU_SetOutputBit((PCU_ID_e)IR_LED_PORT, (PCU_PIN_ID_e)IR_LED_PIN_ID, PCU_OUTPUT_BIT_CLEAR);
    }
    else
    {
        HAL_PCU_SetOutputBit((PCU_ID_e)IR_LED_PORT, (PCU_PIN_ID_e)IR_LED_PIN_ID, PCU_OUTPUT_BIT_SET);
    }
}

static void EVB_IR_IRQHandler(uint32_t un32Event, void *pContext)
{
    uint32_t now_ticks, d_ticks, dt_us;
    uint8_t level = !((un32Event >> (PCU_PIN_ID_4 * 2)) & 0x1);
    irr_msg_t irr_msg;
    int nec_ret = 0;

    HAL_FRT_ReadCount(FRT_ID_0, false, &now_ticks); // hardware timestamp register
    d_ticks = now_ticks - last_ticks;
    last_ticks = now_ticks;

    dt_us = ticks_to_us(d_ticks);
    nec_ret = nec_on_edge(&nec, dt_us, level);

    switch (nec_ret)
    {
    case NEC_RET_FRAME_DONE:
        if (nec.frame_ready)
        {
            irr_msg.data = nec.bits;
            irr_msg.repeated = 0;
            nec.frame_ready = 0;
        }
        (void)AppEvent_SendIRFromISR(&irr_msg);
        break;
    case NEC_RET_REPEAT:
        /* Repeat Frame: keep last button state */
        irr_msg.repeated = 1;
        (void)AppEvent_SendIRFromISR(&irr_msg);
        break;
    default:
        break;
    }
    // LOG("%d %dus\n", level, dt_us);
}

static void LED_TIMER_IRQHandler(uint32_t un32Event, void *pContext)
{
    led_control(false);
    HAL_TIMER1_Stop(LED_TIMER_ID);
}

static void frt_init(FRT_ID_e eId)
{
    FRT_CLK_CFG_t tClkCfg = {
        tClkCfg.eClk = FRT_CLK_MCCR,
        tClkCfg.eMccr = FRT_CLK_MCCR_MCLK,
        tClkCfg.un8MccrDiv = 1,
        tClkCfg.ePreDiv = FRT_CLK_PREDIV_1,
    };
    HAL_FRT_SetClkConfig(eId, &tClkCfg);

    FRT_CFG_t ptCfg = {
        ptCfg.eMode = FRT_MODE_FREERUN,
        ptCfg.eIntr = FRT_INTR_OVERFLOW,
    };
    HAL_FRT_Init(eId);
    HAL_FRT_SetConfig(eId, &ptCfg);
    HAL_FRT_Start(eId, true);
}

static void timer_init(TIMER1_ID_e eId, TIMER1_MODE_e mode, uint16_t mSec)
{
    TIMER1_CLK_CFG_t tTimeClkCfg = {
        .eClk = TIMER1_CLK_PCLK,
        .uSubClk.ePClkDiv = TIMER1_PCLK_DIV_64,
        .un16PreScale = 499
    };
    TIMER1_CFG_t tTimeCfg = {
        .eMode = mode,
        .bIntrEnable = true,
        .utData.tGRD.un16DataA = mSec,
        .utData.tGRD.un16DataB = mSec
    };

    HAL_TIMER1_Init(eId);
    HAL_TIMER1_SetClkConfig(eId, &tTimeClkCfg);
    HAL_TIMER1_SetConfig(eId, &tTimeCfg);
    HAL_TIMER1_SetIRQ(eId, TIMER1_OPS_INTR, LED_TIMER_IRQHandler, NULL, 3);
}

static void led_init(PCU_ID_e eId, PCU_PIN_ID_e un32LedID)
{
    HAL_PCU_SetAltMode(eId, un32LedID, PCU_ALT_0); /* Set GPIO Mode */
    HAL_PCU_SetInOutMode(eId, un32LedID, PCU_INOUT_OUTPUT_PUSH_PULL);
    HAL_PCU_SetPullUpDown(eId, un32LedID, PCU_PUPD_DISABLED);
    HAL_PCU_SetOutputBit(eId, un32LedID, PCU_OUTPUT_BIT_CLEAR);
    HAL_PCU_SetOutputBit(eId, un32LedID, PCU_OUTPUT_BIT_SET);
}

void EX_IR_Init(void)
{
    /* 
        Test code for IR Receiver 
    */
    PCU_IRQ_CFG_t tIRQCfg = {
        tIRQCfg.eId = PCU_ID_C,
        tIRQCfg.eOps = PCU_OPS_INTR,
        tIRQCfg.pfnHandler = &EVB_IR_IRQHandler,
        tIRQCfg.pContext = &s_tPCUContext[PCU_ID_C],
        tIRQCfg.un32IRQPrio = configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY,
        tIRQCfg.un8IntNum = GPIOCD_IRQn,
    };
    /* IR Receiver OUT */
    HAL_PCU_SetAltMode(PCU_ID_C, PCU_PIN_ID_4, PCU_ALT_0);
    HAL_PCU_SetInOutMode(PCU_ID_C, PCU_PIN_ID_4, PCU_INOUT_INPUT);
    HAL_PCU_SetIntrPort(PCU_ID_C, PCU_PIN_ID_4, PCU_INTR_MODE_EDGE, PCU_INTR_TRG_BOTH_LEVEL_EDGE, GPIOCD_IRQn);
    
    uint32_t PINS = (1u << PCU_PIN_ID_4);
    EX_SharedIRQ_Register(&tIRQCfg, PINS, SHARED_GPIO_HIGH_ISR);

    frt_init(FRT_ID_0);
    // Init Timer 1 channel 5 for LED Timer 300ms
    timer_init(LED_TIMER_ID, TIMER1_MODE_PERIODIC, IR_LED_BLINK_MS);
    led_init((PCU_ID_e)IR_LED_PORT, (PCU_PIN_ID_e)IR_LED_PIN_ID);

    nec_reset(&nec);
    nec.last_level = 1;

    if(DBG_LOG(DEBUG_ALL)) LOG("()=%s()\n", __FUNCTION__);
}

void IR_Reinit(void)
{
    HAL_TIMER1_Stop(LED_TIMER_ID);
    HAL_FRT_Stop(FRT_ID_0);
    /* IR Receiver OUT */
    HAL_PCU_SetAltMode(PCU_ID_C, PCU_PIN_ID_4, PCU_ALT_0);
    HAL_PCU_SetInOutMode(PCU_ID_C, PCU_PIN_ID_4, PCU_INOUT_OUTPUT_PUSH_PULL);

    if(DBG_LOG(DEBUG_ALL)) LOG("()=%s()\n", __FUNCTION__);
}
void IR_RegisterKeyCallback(pfnKeyCallback_t handler)
{
    if (handler != NULL)
    {
        s_pfnKey = handler;
    }
}

void EX_IR_Receiver_Manager(void)
{
    return;
}

void EX_IR_ProcessEvent(const irr_msg_t *msg)
{
    nec_rev_t ir_recv;

    if (msg == NULL)
    {
        return;
    }

    {
        if (msg->repeated)
        {
            /* Repeat Frame: keep last button state */
            //LOG("NEC REPEAT\n");
            return;
        }
        else
        {
            /* 32 bits collected -> split into bytes and validate */
            ir_recv.addr = msg->data & 0xFF;
            ir_recv.naddr = (msg->data >> 8) & 0xFF;
            ir_recv.cmd = (msg->data >> 16) & 0xFF;
            ir_recv.ncmd = (msg->data >> 24) & 0xFF;

            if (((uint8_t)(ir_recv.addr ^ ir_recv.naddr) == 0xFF) &&
                ((uint8_t)(ir_recv.cmd ^ ir_recv.ncmd) == 0xFF))
            {
                // Start Led Timer
                led_control(true);
                HAL_TIMER1_Start(LED_TIMER_ID);
                // send key
                if (s_pfnKey != NULL)
                {
                    if (ir_recv.cmd == IR_REMOTE_CMD_LEFT)
                        s_pfnKey(KEY_LEFT);
                    else if (ir_recv.cmd == IR_REMOTE_CMD_RIGHT)
                        s_pfnKey(KEY_RIGHT);
                    else if (ir_recv.cmd == IR_REMOTE_CMD_UP)
                        s_pfnKey(KEY_UP);
                    else if (ir_recv.cmd == IR_REMOTE_CMD_DOWN)
                        s_pfnKey(KEY_DOWN);
                    else if (ir_recv.cmd == IR_REMOTE_CMD_OK)
                        s_pfnKey(KEY_OK);
                }
                LOG("ADDR=0x%02X CMD=0x%02X\n", ir_recv.addr, ir_recv.cmd);
            }
        }
    }
}
