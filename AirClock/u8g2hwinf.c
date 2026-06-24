/**
 *******************************************************************************
 * @file        u8g2hwinf.c
 * @author      ABOV R&D Division
 * @brief       u8g2 Graphic Functions with Software SPI Interface
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
#include "hal_i2c.h"
#include "hal_usart.h"
#include "hal_pcu.h"
#include "u8g2.h"

#define TX_TIMEOUT		100
#define SSD1306_I2C_SLVADDR    0x3C

static volatile bool u8x8_update_flag = false;

static void u8x8_spi_irqhandler(uint32_t un32Event, void *pContext)
{
    if (un32Event & USART_EVENT_TX_DONE)
    {
        u8x8_update_flag = false;
    }
}

static void u8x8_spi_init(void) {

    /* USART2 MOSI */
    HAL_PCU_SetAltMode(PCU_ID_E, PCU_PIN_ID_8, PCU_ALT_4);
    HAL_PCU_SetPullUpDown(PCU_ID_E, PCU_PIN_ID_8, PCU_PUPD_UP);

    /* USART2 SCK */
    HAL_PCU_SetAltMode(PCU_ID_E, PCU_PIN_ID_10, PCU_ALT_4);

    /* Chip Selection */
    HAL_PCU_SetAltMode(PCU_ID_E, PCU_PIN_ID_11, PCU_ALT_0);
    HAL_PCU_SetInOutMode(PCU_ID_E, PCU_PIN_ID_11, PCU_INOUT_OUTPUT_PUSH_PULL);

    /* Data / Command */
    HAL_PCU_SetAltMode(PCU_ID_E, PCU_PIN_ID_0, PCU_ALT_0);
    HAL_PCU_SetInOutMode(PCU_ID_E, PCU_PIN_ID_0, PCU_INOUT_OUTPUT_PUSH_PULL);

    /* Reset */
    HAL_PCU_SetAltMode(PCU_ID_E, PCU_PIN_ID_1, PCU_ALT_0);
    HAL_PCU_SetInOutMode(PCU_ID_E, PCU_PIN_ID_1, PCU_INOUT_OUTPUT_PUSH_PULL);

    USART_CFG_t tCfg = {
        .eMode = USART_MODE_SPI,
        .tCfg.tSpi.eMs = USART_MS_MASTER,
        .tCfg.tSpi.eBitOrder = USART_BIT_ORDER_MSB,
        .tCfg.tSpi.eClkPol = USART_CLKPOL_TXD_RISE_RXD_FALL,
        .tCfg.tSpi.eClkPha = USART_CLKPHA_SAMPLE,
        .tCfg.tSpi.bRxSCKGen = false,
        .tCfg.tSpi.bSSGenDisable = true,
        .un32BaudRate = 15 /* 32MHz / 2*(2 + 1) = 6.3MHz */
    };

    HAL_USART_Init(USART_ID_2);
    HAL_USART_SetConfig(USART_ID_2, &tCfg);
    HAL_USART_SetIRQ(USART_ID_2, USART_OPS_INTR, u8x8_spi_irqhandler, NULL, 0);
}

void u8x8_tx_complete(void)
{
    while(u8x8_update_flag);
}

uint8_t u8x8_abov_gpio_and_delay(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    switch(msg)
    {
        case U8X8_MSG_GPIO_AND_DELAY_INIT:
            break;
        case U8X8_MSG_DELAY_MILLI:
            SystemDelayMS(arg_int);
            break;
    }
    return 1;
}

uint8_t u8x8_byte_abov_hw_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    static uint8_t buffer[32];
    static uint8_t buf_idx;
    uint8_t *data;

    switch(msg)
    {
        case U8X8_MSG_BYTE_SEND:
            data = (uint8_t *)arg_ptr;
            while( arg_int > 0 )
            {
                buffer[buf_idx++] = *data;
                data++;
                arg_int--;
            }
            break;
        case U8X8_MSG_BYTE_INIT:
            /* add your custom code to init i2c subsystem */
            break;
        case U8X8_MSG_BYTE_SET_DC:
            break;
        case U8X8_MSG_BYTE_START_TRANSFER:
            buf_idx = 0;
            break;
        case U8X8_MSG_BYTE_END_TRANSFER:
            u8x8_update_flag = true;
            HAL_I2C_Transmit(I2C_ID_1, SSD1306_I2C_SLVADDR, buffer, buf_idx, false);
            u8x8_tx_complete();
    	    break;
        default:
            return 0;
    }

    return 1;
}

uint8_t u8x8_byte_abov_hw_spi(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    switch(msg)
    {
        case U8X8_MSG_BYTE_SEND:
            HAL_PCU_SetOutputBit(PCU_ID_E, PCU_PIN_ID_11, PCU_OUTPUT_BIT_CLEAR);
            u8x8_update_flag = true;
            HAL_USART_Transmit(USART_ID_2, (uint8_t *)arg_ptr, arg_int, false);
            u8x8_tx_complete();
            HAL_PCU_SetOutputBit(PCU_ID_E, PCU_PIN_ID_11, PCU_OUTPUT_BIT_SET);
            break;
        case U8X8_MSG_BYTE_INIT:
            u8x8_spi_init();
            SystemDelayMS(10);
            HAL_PCU_SetOutputBit(PCU_ID_E, PCU_PIN_ID_1, PCU_OUTPUT_BIT_SET);
            SystemDelayMS(10);
            HAL_PCU_SetOutputBit(PCU_ID_E, PCU_PIN_ID_1, PCU_OUTPUT_BIT_CLEAR);
            SystemDelayMS(500);
            HAL_PCU_SetOutputBit(PCU_ID_E, PCU_PIN_ID_1, PCU_OUTPUT_BIT_SET);
            SystemDelayMS(500);
            break;
        case U8X8_MSG_BYTE_SET_DC:
            if (arg_int == 0)
                HAL_PCU_SetOutputBit(PCU_ID_E, PCU_PIN_ID_0, PCU_OUTPUT_BIT_CLEAR);
            else
                HAL_PCU_SetOutputBit(PCU_ID_E, PCU_PIN_ID_0, PCU_OUTPUT_BIT_SET);
            break;
        case U8X8_MSG_BYTE_START_TRANSFER:
            HAL_PCU_SetOutputBit(PCU_ID_E, PCU_PIN_ID_11, PCU_OUTPUT_BIT_CLEAR);
	    break;
        case U8X8_MSG_BYTE_END_TRANSFER:
            HAL_PCU_SetOutputBit(PCU_ID_E, PCU_PIN_ID_11, PCU_OUTPUT_BIT_SET);
	    break;
        default:
            return 0;
    }
    return 1;
}
