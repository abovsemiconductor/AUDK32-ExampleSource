/**
 *******************************************************************************
 * @file        airclock_sensor.c
 * @author      ABOV R&D Division
 * @brief       AirClock Application Main
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
#include "hal_i2c.h"
#include "hal_pcu.h"

#include "airclock_sensor.h"

typedef enum {
    DEBUG_CLEAR,
    DEBUG_ERROR,
    DEBUG_TRACE,
    DEBUG_ALL
} TCS_DEBUG_LEVEL_e;

#define TCS_I2C_ID   I2C_ID_1
#define TCS_I2C_DATA_LEN    16
#define TCS_I2C_IRQ_PRIO    2
#define H115_I2C_SLVADDR    0x7F

#define REGISTER_CTL_RESET      0x00
#define REGISTER_CTL_MAN        0x01
#define REGISTER_CTL_CYCLE      0x02
#define REGISTER_CTL_ERROR      0x03
#define REGISTRE_HC_L           0x04
#define REGISTRE_HC_H           0x05
#define REGISTRE_TC_L           0x06
#define REGISTRE_TC_H           0x07

#define MAN_STANDBY         (0 << 0)
#define MAN_DETECTION       (1 << 0)
#define CMODE_SINGLE        (0 << 1)
#define CMODE_CONTINUOUS    (1 << 1)

#define DBG_LOG(level)      ((level) <= s_un8I2cDebugLevel && (level) > DEBUG_CLEAR ? 1 : 0)

static I2C_Context_t s_tI2CContext;
static volatile bool s_bI2cUpdataFlag = false;

static uint8_t s_un8I2cDebugLevel = DEBUG_CLEAR;

/**
 *  FUNCTIONS
**/
static void PRV_I2C_IRQHandler(uint32_t un32Event, void *pContext)
{
    if(un32Event & I2C_EVENT_TX_DONE)
    {
        // LOG("Sensor tx done\n");
    }

    if(un32Event & I2C_EVENT_RX_DONE)
    {
        // LOG("Sensor rx done\n");
    }
    s_bI2cUpdataFlag = false;
}

static void PRV_I2C_CompleteCmd(void)
{
    uint32_t un32Timeout = 1000000;
    while(s_bI2cUpdataFlag && un32Timeout)
    {
        un32Timeout--;
    }
    s_bI2cUpdataFlag = true;
}

static HAL_ERR_e PRV_TCS_I2C_Write(uint8_t un8Addr, uint8_t un8Len, uint8_t *pun8Data)
{
    HAL_ERR_e eErr;
    uint8_t un8Write[TCS_I2C_DATA_LEN] = {0, };

    un8Write[0] = un8Addr;
    memcpy(&un8Write[1], pun8Data, un8Len);

    un8Len += 1;
    eErr = HAL_I2C_Transmit(TCS_I2C_ID, H115_I2C_SLVADDR, un8Write, un8Len, false);
    PRV_I2C_CompleteCmd();

    if(DBG_LOG(DEBUG_ALL)) LOG("(%d)=%s(%x, 0x%X)\n", eErr, __FUNCTION__, un8Addr, pun8Data[0]);
    return eErr;
}

static HAL_ERR_e PRV_TCS_I2C_Read(uint8_t un8Addr, uint8_t un8Len, uint8_t *pun8Data)
{
    HAL_ERR_e eErr;
    uint8_t un8DataAddr;
    un8DataAddr = un8Addr;

    eErr = HAL_I2C_Transmit(TCS_I2C_ID, H115_I2C_SLVADDR, &un8DataAddr, 1, false);
    PRV_I2C_CompleteCmd();

    eErr = HAL_I2C_Receive(TCS_I2C_ID, H115_I2C_SLVADDR, pun8Data, un8Len, false);
    PRV_I2C_CompleteCmd();

    if(DBG_LOG(DEBUG_ALL)) LOG("(%d)=%s(%x, 0x%X)\n", eErr, __FUNCTION__, un8Addr, pun8Data[0]);
    return eErr;
}

void EX_TCS_Init(void)
{
    I2C_CFG_t tCfg = {
        .eMode= I2C_MODE_MASTER,
        .un8OwnSlvAddr = 0,
        .un8OwnSlvAddr2 = 0,
        .bSaGcEnable = 0,
        .bSa2GcEnable = 0,
        .uPeriod.tFreq.un32Freq = 400000,
        .tSdht.bEnable = 0
    };

    /* I2C SCL */
    HAL_PCU_SetAltMode(PCU_ID_A, PCU_PIN_ID_1, PCU_ALT_1);
    HAL_PCU_SetInOutMode(PCU_ID_A, PCU_PIN_ID_1, PCU_INOUT_OUTPUT_PUSH_PULL);
    /* I2C SDA */
    HAL_PCU_SetAltMode(PCU_ID_A, PCU_PIN_ID_0, PCU_ALT_1);
    HAL_PCU_SetInOutMode(PCU_ID_A, PCU_PIN_ID_0, PCU_INOUT_OUTPUT_PUSH_PULL);

    /* Chip A96H115 Enable */
    HAL_PCU_SetAltMode(PCU_ID_A, PCU_PIN_ID_3, PCU_ALT_0);
    HAL_PCU_SetInOutMode(PCU_ID_A, PCU_PIN_ID_3, PCU_INOUT_OUTPUT_PUSH_PULL);
    HAL_PCU_SetOutputBit(PCU_ID_A, PCU_PIN_ID_3, PCU_OUTPUT_BIT_SET);
    SystemDelayMS(30);

    HAL_I2C_Init(TCS_I2C_ID);
    HAL_I2C_SetConfig(TCS_I2C_ID, &tCfg);
    
    s_tI2CContext.eId = TCS_I2C_ID;
    HAL_I2C_SetIRQ(TCS_I2C_ID, I2C_OPS_INTR, PRV_I2C_IRQHandler, &s_tI2CContext, TCS_I2C_IRQ_PRIO);

    s_bI2cUpdataFlag = true;

    if(DBG_LOG(DEBUG_ALL)) LOG("()=%s()\n", __FUNCTION__);
}

int EX_TCS_I2C_Single_Measure(uint16_t *pun16Hc, uint16_t *pun16Tc)
{
    HAL_ERR_e eErr;
    int32_t n32Timeout = 1000;
    uint8_t un8Control = 0;
    uint8_t un8Detected = 0;
    uint8_t un8Lenght = 1;

    un8Control = (MAN_DETECTION | CMODE_SINGLE);
    eErr = PRV_TCS_I2C_Write(REGISTER_CTL_MAN, un8Lenght, &un8Control);
    SystemDelayUS(200);

    n32Timeout = 1000;
    do
    {
        PRV_TCS_I2C_Read(REGISTER_CTL_MAN, un8Lenght, &un8Detected);
        if(!un8Detected)
        {
            break;
        }
        SystemDelayUS(2000);
    } while (--n32Timeout);
    if (0 == n32Timeout)
    {
        eErr = HAL_ERR_TIMEOUT;
        if(DBG_LOG(DEBUG_ERROR)) LOG("TIME OUT\n");
    }

    un8Lenght = 2;
    PRV_TCS_I2C_Read(REGISTRE_HC_L, un8Lenght, (uint8_t*)pun16Hc);
    PRV_TCS_I2C_Read(REGISTRE_TC_L, un8Lenght, (uint8_t*)pun16Tc);

    if(DBG_LOG(DEBUG_TRACE)) LOG("Humidity: 0x%X(%d)\n", *pun16Hc, *pun16Hc);
    if(DBG_LOG(DEBUG_TRACE)) LOG("Temperature: 0x%X(%d)\n", *pun16Tc, *pun16Tc);

    if(DBG_LOG(DEBUG_ALL)) LOG("(%d)=%s(%d)\n", eErr, __FUNCTION__, n32Timeout);
    return eErr;
}
