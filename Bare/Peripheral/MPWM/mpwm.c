/**
 *******************************************************************************
 * @file        mpwm.c
 * @author      ABOV R&D Division
 * @brief       MPWM Example Code
 *
 * Copyright 2022 ABOV Semiconductor Co.,Ltd. All rights reserved.
 *
 * This file is licensed under terms that are found in the LICENSE file
 * located at Document directory.
 * If this file is delivered or shared without applicable license terms,
 * the terms of the BSD-3-Clause license shall be applied.
 * Reference: https://opensource.org/licenses/BSD-3-Clause
 ******************************************************************************/

#include "abov_example_config.h"

#if defined(MPWM_TC)
#include "abov_config.h"
#include "ex_common.h"
#include "debug_cmd.h"
#include "debug_log.h"
#include "debug.h"
#include "hal_scu.h"
#include "hal_scu_clk.h"
#include "hal_pcu.h"
#include "hal_mpwm.h"

#if !defined(_MPWM)
#error "This chipset did not support this example."
#endif

#define EX_MPWM_STR              "MPWM"
#define EX_MPWM_LOG_STR          "MPWM :"
#define EX_MPWM_ERR_STR          "[E]MPWM :"
#define EX_MPWM_IRQ_PRIO         3
#define EX_MPWM_PCU_IRQ_PRIO     3
#define EX_MPWM_MAX_ARG          7
#define EX_MPWM_PHASE_CNT        6
#define EX_MPWM_PHASE_OFFSET     2
#define EX_MPWM_SIG_CNT          3
#define EX_MPWM_MAX_NUM          (CONFIG_MPWM_MAX_COUNT - 1)

extern const char *pcCommStr[];
extern const char *pcCmdStr[];
extern const char *pcValStr[];
extern const char *pcOptStr[];

typedef struct {
    PCU_ID_e eId;
    PCU_PIN_ID_e ePinId;
} EX_MPWM_HALL_PORT_t;

typedef struct {
    MPWM_SIG_e eSig;
    MPWM_SD_CFG_t tSDCfg;
} EX_MPWM_SIG_t;

typedef enum {
    EX_MPWM_PHASE_1 = 5, /* CW : WU, CCW : UW */
    EX_MPWM_PHASE_2 = 4, /* CW : WV, CCW : VW */
    EX_MPWM_PHASE_3 = 6, /* CW : UV, CCW : VU */
    EX_MPWM_PHASE_4 = 2, /* CW : UW, CCW : WU */
    EX_MPWM_PHASE_5 = 3, /* CW : VW, CCW : WV */
    EX_MPWM_PHASE_6 = 1, /* CW : VU, CCW : UV */
    EX_MPWM_PHASE_MAX = 7,
} EX_MPWM_PHASE_e;

EX_MPWM_PHASE_e s_aePhase[EX_MPWM_PHASE_CNT] = {
    EX_MPWM_PHASE_1,
    EX_MPWM_PHASE_2,
    EX_MPWM_PHASE_3,
    EX_MPWM_PHASE_4,
    EX_MPWM_PHASE_5,
    EX_MPWM_PHASE_6
};

typedef enum {
    EX_MPWM_BLDC_MODE_FORCE,
    EX_MPWM_BLDC_MODE_HALL_SENSOR,
    EX_MPWM_BLDC_MODE_MAX,
} EX_MPWM_BLDC_MODE_e;

static uint8_t s_un8Phase = 0;
static bool s_bCW = false;
static EX_MPWM_HALL_PORT_t s_tHallPort[3];

static MPWM_PWM_CFG_t   s_tPWMCfg[CONFIG_MPWM_MAX_COUNT];
static MPWM_INDIV_CFG_t s_tINDIVCfg[CONFIG_MPWM_MAX_COUNT];
static MPWM_Context_t   s_tMPWMContext[CONFIG_MPWM_MAX_COUNT];
static MPWM_MODE_e s_eMode = MPWM_MODE_MAX;

static bool s_bIFX007TRefEnable = false;
static EX_MPWM_BLDC_MODE_e s_eBLDCMode = EX_MPWM_BLDC_MODE_MAX;

static uint32_t s_un32PhaseCount = 0;
static uint32_t s_un32PhaseDelay = 0;

static uint16_t s_un16UHDuty = 0;
static uint16_t s_un16ULDuty = 0;
static uint16_t s_un16VHDuty = 0;
static uint16_t s_un16VLDuty = 0;
static uint16_t s_un16WHDuty = 0;
static uint16_t s_un16WLDuty = 0;
static uint16_t s_un16IFX007TOnDuty = 0;
static uint16_t s_un16IFX007TOffDuty = 0;

static bool s_bISRLog = true;
static uint32_t s_un32LogDispCnt = 0;
static uint32_t s_un32LogCurCnt = 0;

static PCU_Context_t s_tMPWMPCUContext[CONFIG_PCU_MAX_COUNT];

static enum debug_cmd_status EX_MPWM_Help(int32_t n32Argc, char *pn8Argv[])
{
    uint32_t un32ShowOpt = 0;
    EX_COMM_STR_OPT_e eOpt[2];

    EX_COMMON_SetShowModuleInfo(EX_MPWM_STR, CONFIG_MPWM_MAX_COUNT);

    un32ShowOpt = EX_COMMON_GetShowOpt(pn8Argv[1]);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_INIT, NULL, EX_MPWM_MAX_NUM, eOpt, 0, NULL);
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_UNINIT, NULL, EX_MPWM_MAX_NUM, eOpt, 0, NULL);

    eOpt[0] = EX_COMM_STR_OPT_SRC; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CLK, NULL, EX_MPWM_MAX_NUM, eOpt, 1, NULL);
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_SRC, EX_COMM_STR_VAL_MAX, "m(mccr)");
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MCCR, EX_COMM_STR_VAL_MAX, "[mccr] [div]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MCCR, EX_COMM_STR_VAL_CLKSRC, "/m(mclk)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_DIV, EX_COMM_STR_VAL_N_255, NULL);
    }
    eOpt[0] = EX_COMM_STR_OPT_MODE; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_CONFIG, NULL, EX_MPWM_MAX_NUM, eOpt, 1, "[chn] [cnt] [pm] [bm]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MODE, EX_COMM_STR_VAL_MAX, "m(motor)/p(pwm)/i(individual pwm)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "chn: t(two symmetric)/s(one symmetric)/a(one asymmetric)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "cnt: u(up)/b(up-down)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "pm: e(en)/d(dis) - update every period match");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "bm: e(en)/d(di) - update every bottom match");
    }
    eOpt[0] = EX_COMM_STR_OPT_OPS; 
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "irq", EX_MPWM_MAX_NUM, eOpt, 1, "[hdlr] [prio]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_OPS, EX_COMM_STR_VAL_OPS, "/n(nmi)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "hdlr: p(protection)/o(overvoltage)/u(signal-u)/v(signal-v)/w(signal-w)/m (motor/pwm)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "prio: 0~255(decimal)");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "pwm", EX_MPWM_MAX_NUM, eOpt, 0, "[-ped] [-intr] [-pdp] [-dt] [-prot] [-ov] [-apply] - motor/normal pwm");
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "indiv", EX_MPWM_MAX_NUM, eOpt, 0, "[sig] [-ped] [-intr] [-pdp] [-dt] [-prot] [-ov] [-apply] -individual pmw");
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "duty", EX_MPWM_MAX_NUM, eOpt, 0, "[sig] [hl] [ld]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "sig: u(signal-u)/v(signal-v)/w(signal-w)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-ped: 0~N(decimal) - period");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-intr: hexa value (mask intr num ex.0x3f)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "     : 0x1(UL)/0x2(VL)/0x4(WL)/0x8(UH)/0x10(VH)/0x20(WH)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "     : 0x40(BU)/0x80(PU)/0x100(BV)/0x200(PV)/0x400(BW)/0x800(PW)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-sdp: [sig] [hd] [ld] [hp] [lp]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "sig: u(signal-u)/v(signal-v)/w(signal-w)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "hd: 0~N(decimal) - H-duty");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "ld: 0~N(decimal) - L-duty");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "hp: [out] [start] [force]");
        EX_COMMON_SetShowOptVal(3, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "out: l(low)/h(high)");
        EX_COMMON_SetShowOptVal(3, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "start: l(low)/h(high)");
        EX_COMMON_SetShowOptVal(3, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "force: l(low)/h(high)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "lp: [out] [start] [force]");
        EX_COMMON_SetShowOptVal(3, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "out: l(low)/h(high)");
        EX_COMMON_SetShowOptVal(3, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "start: l(low)/h(high)");
        EX_COMMON_SetShowOptVal(3, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "force: l(low)/h(high)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-dt: [en] [mode] [scp] [pre] [val]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_ENA, EX_COMM_STR_VAL_ENDIS, NULL);
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MODE, EX_COMM_STR_VAL_MAX, "l(lead l)/h(lead h) - leading edge");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "scp: e(en)/d(dis) - short-circuit protection");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "pre: 2/4/8/16 - prescale");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_VAL, EX_COMM_STR_VAL_N_NUM, "(decimal) - time value");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-prot/-ov: [en] [intr] [inpol] [deb] [hl] [ll] - protection input");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_ENA, EX_COMM_STR_VAL_ENDIS, NULL);
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "intr: e(en)/d(dis)");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "inpol: l(low)/h(high) - in-polarity");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "deb: 0~N(decimal) - debounce time");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "hl: e(en)/d(dis) - preset level output when occurred");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "ll: e(en)/d(dis) - preset level output when occurred");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-apply: apply all value to config");
    }

    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "bldc", EX_MPWM_MAX_NUM, eOpt, 0, "[-ref] [-mode] [-phdly] [-hall] [-dir] [-duty]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-ref: IFX007T half-bridge board");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_ENA, EX_COMM_STR_VAL_ENDIS, NULL);
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-mode: f(force)/h(hall sensor)/o(mode off)");
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "f:");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-phdly: 0~N(decimal) - next phase update delay");
        EX_COMMON_SetShowOptVal(1, true, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "h:");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-hall: [ha] [hb] [hc]");
        EX_COMMON_SetShowOptVal(3, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "ha/hb/hc: [port] [pin]");
        EX_COMMON_SetShowOptVal(4, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "port: 0~N (group port num (ex. group a = 0))");
        EX_COMMON_SetShowOptVal(4, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "pin: 0~N (pin num)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-dir: f(cw)/b(ccw)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-duty: [uhd] [uld] [vhd] [vld] [whd] [wld] [-ref]");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "hd: 0~N(decimal) - (UVW)H-duty");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "ld: 0~N(decimal) - (UVW)L-duty");
        EX_COMMON_SetShowOptVal(2, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "-ref: [on] [off]");
        EX_COMMON_SetShowOptVal(3, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "on: 0~N(decimal) - On-duty (vdd)");
        EX_COMMON_SetShowOptVal(3, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "off: 0~N(decimal) - On-duty (gnd)");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "adctrg", EX_MPWM_MAX_NUM, eOpt, 0, "[num] [sig] [mode] [intr] [upd] [val]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "num: 0~N(decimal)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "sig: u(signal-u)/v(signal-v)/w(signal-w)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "mode: n(dis)/u(up)/d(down)/b(both) - match up/down counter mode");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "intr: e(en)/d(dis)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "upd: e(en)/d(dis) - trigger update mode");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_VAL, EX_COMM_STR_VAL_N_NUM, "(decimal) - trigger generation counter value");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_MAX, "hall", EX_MPWM_MAX_NUM, eOpt, 0, "display current hall position");
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_START, NULL, EX_MPWM_MAX_NUM, eOpt, 0, "[stmode] [sig]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "stmode: c(recount)/r(resume)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "sig: hexa value (mask signal num ex.0xe)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "   : m(motor) or p(pwm) - 0x1(pwm)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "   : i(individual pwm - 0x2(PU)|0x04(PV)|0x08(PW)");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_STOP, NULL, EX_MPWM_MAX_NUM, eOpt, 0, "[spmode] [sig]");
    if(un32ShowOpt == 1)
    {
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "spmode: c(clear)/m(remain)/r(reset)/d(dis)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "sig: hexa value (mask signal num ex.0xe)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "   : m(motor) or p(pwm) - 0x1(pwm)");
        EX_COMMON_SetShowOptVal(1, false, EX_COMM_STR_OPT_MAX, EX_COMM_STR_VAL_MAX, "   : i(individual pwm - 0x2(PU)|0x04(PV)|0x08(PW)");
    }
    EX_COMMON_SetShowCmd(EX_COMM_STR_CMD_LOG, NULL, -1, eOpt, 0, "on/off");
    LOG("\n");

    return DEBUG_CMD_SUCCESS;
}

static void PRV_EX_MPWM_EventLog(MPWM_ID_e eId, uint32_t un32Event)
{
    if(un32Event & MPWM_EVENT_DUTY_UL_ATR1)
    {
        if(s_bISRLog == true && s_un32LogDispCnt == s_un32LogCurCnt)
        {
            LOG("%s (%d) DUTY U-L'side or Adc Trg 1 evt fired\n", EX_MPWM_LOG_STR, eId);
        }
    }

    if(un32Event & MPWM_EVENT_DUTY_UH_ATR4)
    {
        if(s_bISRLog == true && s_un32LogDispCnt == s_un32LogCurCnt)
        {
            LOG("%s (%d) DUTY U-H'side or Adc Trg 4 evt fired\n", EX_MPWM_LOG_STR, eId);
        }
    }

    if(un32Event & MPWM_EVENT_DUTY_VL_ATR2)
    {
        if(s_bISRLog == true && s_un32LogDispCnt == s_un32LogCurCnt)
        {
            LOG("%s (%d) DUTY V-L'side or Adc Trg 2 evt fired\n", EX_MPWM_LOG_STR, eId);
        }
    }

    if(un32Event & MPWM_EVENT_DUTY_VH_ATR5)
    {
        if(s_bISRLog == true && s_un32LogDispCnt == s_un32LogCurCnt)
        {
            LOG("%s (%d) DUTY V-H'side or Adc Trg 5 evt fired\n", EX_MPWM_LOG_STR, eId);
        }
    }

    if(un32Event & MPWM_EVENT_DUTY_WL_ATR3)
    {
        if(s_bISRLog == true && s_un32LogDispCnt == s_un32LogCurCnt)
        {
            LOG("%s (%d) DUTY W-L'side or Adc Trg 3 evt fired\n", EX_MPWM_LOG_STR, eId);
        }
    }

    if(un32Event & MPWM_EVENT_DUTY_WH_ATR6)
    {
        if(s_bISRLog == true && s_un32LogDispCnt == s_un32LogCurCnt)
        {
            LOG("%s (%d) DUTY W-H'side or Adc Trg 6 evt fired\n", EX_MPWM_LOG_STR, eId);
        }
    }

    if(un32Event & MPWM_EVENT_BOTTOM_U)
    {
        if(s_bISRLog == true && s_un32LogDispCnt == s_un32LogCurCnt)
        {
            LOG("%s (%d) Signal-U Botton evt fired\n", EX_MPWM_LOG_STR, eId);
        }
    }

    if(un32Event & MPWM_EVENT_PERIOD_U)
    {
        if(s_bISRLog == true && s_un32LogDispCnt == s_un32LogCurCnt)
        {
            LOG("%s (%d) Signal-U Period evt fired\n", EX_MPWM_LOG_STR, eId);
        }
    }

    if(un32Event & MPWM_EVENT_BOTTOM_V)
    {
        if(s_bISRLog == true && s_un32LogDispCnt == s_un32LogCurCnt)
        {
            LOG("%s (%d) Signal-V Botton evt fired\n", EX_MPWM_LOG_STR, eId);
        }
    }

    if(un32Event & MPWM_EVENT_PERIOD_V)
    {
        if(s_bISRLog == true && s_un32LogDispCnt == s_un32LogCurCnt)
        {
            LOG("%s (%d) Signal-V Period evt fired\n", EX_MPWM_LOG_STR, eId);
        }
    }

    if(un32Event & MPWM_EVENT_BOTTOM_W)
    {
        if(s_bISRLog == true && s_un32LogDispCnt == s_un32LogCurCnt)
        {
            LOG("%s (%d) Signal-W Botton evt fired\n", EX_MPWM_LOG_STR, eId);
        }
    }

    if(un32Event & MPWM_EVENT_PERIOD_W)
    {
        if(s_bISRLog == true && s_un32LogDispCnt == s_un32LogCurCnt)
        {
            LOG("%s (%d) Signal-W Period evt fired\n", EX_MPWM_LOG_STR, eId);
        }
    }
}

static enum debug_cmd_status PRV_EX_MPWM_SetPhase(MPWM_ID_e eId, bool bCW, EX_MPWM_PHASE_e ePhase)
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;
    EX_MPWM_SIG_t tSigCfg[EX_MPWM_SIG_CNT];

    switch(ePhase)
    {
        case EX_MPWM_PHASE_3: /* CW : UV, CCW : VU */
        case EX_MPWM_PHASE_6: /* CW : VU, CCW : UV */
            tSigCfg[0].eSig = MPWM_SIG_W;
            if(s_bIFX007TRefEnable == false)
            {
                tSigCfg[0].tSDCfg.un16HDuty = s_un16WHDuty;
                tSigCfg[0].tSDCfg.un16LDuty = s_un16WLDuty;
            }
            if((bCW == true && ePhase == EX_MPWM_PHASE_3)
               || (bCW == false && ePhase == EX_MPWM_PHASE_6))
            {
               tSigCfg[1].eSig = MPWM_SIG_V;
               tSigCfg[2].eSig = MPWM_SIG_U;
               if(s_bIFX007TRefEnable == true)
               {
                   tSigCfg[2].tSDCfg.un16HDuty = s_un16UHDuty; 
               }
               else
               {
                   tSigCfg[1].tSDCfg.un16HDuty = s_un16VHDuty; 
                   tSigCfg[1].tSDCfg.un16LDuty = s_un16VLDuty; 
                   tSigCfg[2].tSDCfg.un16HDuty = s_un16UHDuty; 
                   tSigCfg[2].tSDCfg.un16LDuty = s_un16ULDuty; 
               }
            }
            else
            {
               tSigCfg[1].eSig = MPWM_SIG_U;
               tSigCfg[2].eSig = MPWM_SIG_V;
               if(s_bIFX007TRefEnable == true)
               {
                   tSigCfg[2].tSDCfg.un16HDuty = s_un16VHDuty;
               }
               else
               {
                   tSigCfg[1].tSDCfg.un16HDuty = s_un16UHDuty;
                   tSigCfg[1].tSDCfg.un16LDuty = s_un16ULDuty;
                   tSigCfg[2].tSDCfg.un16LDuty = s_un16VLDuty;
               }
            }
            break;

        case EX_MPWM_PHASE_1: /* CW : WU, CCW : UW */
        case EX_MPWM_PHASE_4: /* CW : UW, CCW : WU */
            tSigCfg[0].eSig = MPWM_SIG_V;
            if(s_bIFX007TRefEnable == false)
            {
                tSigCfg[0].tSDCfg.un16HDuty = s_un16VHDuty;
                tSigCfg[0].tSDCfg.un16LDuty = s_un16VLDuty;
            } 
            if((bCW == true && ePhase == EX_MPWM_PHASE_1)
               || (bCW == false && ePhase == EX_MPWM_PHASE_4))
            {
               tSigCfg[1].eSig = MPWM_SIG_U;
               tSigCfg[2].eSig = MPWM_SIG_W;

               if(s_bIFX007TRefEnable == true)
               {
                   tSigCfg[2].tSDCfg.un16HDuty = s_un16WHDuty;
               }
               else
               {
                   tSigCfg[1].tSDCfg.un16HDuty = s_un16UHDuty;
                   tSigCfg[1].tSDCfg.un16LDuty = s_un16ULDuty;
                   tSigCfg[2].tSDCfg.un16HDuty = s_un16WHDuty;
                   tSigCfg[2].tSDCfg.un16LDuty = s_un16WLDuty;
               }
            }
            else
            {
               tSigCfg[1].eSig = MPWM_SIG_W;
               tSigCfg[2].eSig = MPWM_SIG_U;
               if(s_bIFX007TRefEnable == true)
               {
                   tSigCfg[2].tSDCfg.un16HDuty = s_un16UHDuty;
               }
               else
               {
                   tSigCfg[1].tSDCfg.un16HDuty = s_un16WHDuty;
                   tSigCfg[1].tSDCfg.un16LDuty = s_un16WLDuty;
                   tSigCfg[2].tSDCfg.un16HDuty = s_un16UHDuty;
                   tSigCfg[2].tSDCfg.un16LDuty = s_un16ULDuty;
                }
            }
            break;

        case EX_MPWM_PHASE_2: /* CW : WV, CCW : VW */
        case EX_MPWM_PHASE_5: /* CW : VW, CCW : WV */
            tSigCfg[0].eSig = MPWM_SIG_U;
            if(s_bIFX007TRefEnable == false)
            {
                tSigCfg[0].tSDCfg.un16HDuty = s_un16UHDuty;
                tSigCfg[0].tSDCfg.un16LDuty = s_un16ULDuty;
            }
            if((bCW == true  && ePhase == EX_MPWM_PHASE_2)
               || (bCW == false && ePhase == EX_MPWM_PHASE_5))
            {
               tSigCfg[1].eSig = MPWM_SIG_V;
               tSigCfg[2].eSig = MPWM_SIG_W;
               if(s_bIFX007TRefEnable == true)
               {
                   tSigCfg[2].tSDCfg.un16HDuty = s_un16WHDuty;
               }
               else
               {
                   tSigCfg[1].tSDCfg.un16HDuty = s_un16VHDuty;
                   tSigCfg[1].tSDCfg.un16LDuty = s_un16VLDuty;
                   tSigCfg[2].tSDCfg.un16HDuty = s_un16WHDuty;
                   tSigCfg[2].tSDCfg.un16LDuty = s_un16WLDuty;
               }
            }
            else
            {
               tSigCfg[1].eSig = MPWM_SIG_W;
               tSigCfg[2].eSig = MPWM_SIG_V;
               if(s_bIFX007TRefEnable == true)
               {
                   tSigCfg[2].tSDCfg.un16HDuty = s_un16VHDuty;
               }
               else
               {
                   tSigCfg[1].tSDCfg.un16HDuty = s_un16WHDuty;
                   tSigCfg[1].tSDCfg.un16LDuty = s_un16WLDuty;
                   tSigCfg[2].tSDCfg.un16HDuty = s_un16VHDuty;
                   tSigCfg[2].tSDCfg.un16LDuty = s_un16VLDuty;
               }
            }
            break;
        default:
            eDbgStatus = DEBUG_CMD_INVALID;
            break;
    }

    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    if(s_bIFX007TRefEnable == true)
    {
        tSigCfg[0].tSDCfg.un16HDuty = s_un16IFX007TOffDuty;
        tSigCfg[0].tSDCfg.un16LDuty = s_un16IFX007TOffDuty;

        tSigCfg[1].tSDCfg.un16HDuty = s_un16IFX007TOffDuty;
        tSigCfg[1].tSDCfg.un16LDuty = s_un16IFX007TOnDuty;

        tSigCfg[2].tSDCfg.un16LDuty = s_un16IFX007TOnDuty;
    }

    for(int i = 0; i < EX_MPWM_SIG_CNT; i++)
    {
        eErr = HAL_MPWM_SetDuty(eId, tSigCfg[i].eSig, &tSigCfg[i].tSDCfg);
        if(eErr != HAL_ERR_OK)
        {
            return DEBUG_CMD_INVALID;
        }
    }

    return DEBUG_CMD_SUCCESS;
}

static void PRV_EX_MPWM_GetNextPhase(MPWM_ID_e eId, EX_MPWM_PHASE_e ePhase, uint8_t *pun8NextPhase)
{
    uint8_t un8Phase = 0;

    for (int i = 0; i < EX_MPWM_PHASE_CNT; i++)
    {
        if(ePhase == s_aePhase[i])
        { 
            un8Phase = i + EX_MPWM_PHASE_OFFSET;
            if(un8Phase >= EX_MPWM_PHASE_CNT)
            {
                un8Phase = un8Phase - EX_MPWM_PHASE_CNT; 
            }
            *pun8NextPhase = un8Phase;
            break;
        }
    }
}

static enum debug_cmd_status PRV_EX_MPWM_GetHallPosition(MPWM_ID_e eId, EX_MPWM_PHASE_e *pePhase)
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    PCU_PORT_e ePinValue;
    uint8_t un8HallValue = 0;

    for (int i = 0; i < EX_MPWM_SIG_CNT; i++)
    {
        eErr = HAL_PCU_GetInputValue(s_tHallPort[i].eId, s_tHallPort[i].ePinId, &ePinValue);
        if(eErr != HAL_ERR_OK)
        {
            return DEBUG_CMD_INVALID;
        }

        un8HallValue |= ((uint8_t)ePinValue << i);
    }

    if(un8HallValue >= EX_MPWM_PHASE_MAX)
    {
        return DEBUG_CMD_INVALID;
    }
    else
    {
        *pePhase = (EX_MPWM_PHASE_e)un8HallValue;
    }

    return DEBUG_CMD_SUCCESS;
   
}

static void PRV_EX_MPWM_SetForcePhase(MPWM_ID_e eId, bool bCW)
{
    if(s_un32PhaseCount >= s_un32PhaseDelay)
    {
        if(bCW == true)
        {
            s_un8Phase--;
            if(s_un8Phase == 0xFF)
            {
                s_un8Phase = EX_MPWM_PHASE_CNT - 1;
            }
        }
        else
        {
            s_un8Phase++;
            if(s_un8Phase >= EX_MPWM_PHASE_CNT)
            {
                s_un8Phase = 0;
            }
        }
        (void)PRV_EX_MPWM_SetPhase(eId, bCW, s_aePhase[s_un8Phase]);
        s_un32PhaseCount = 0;
    }
    else
    {
        s_un32PhaseCount++;
    }
}

static void EX_MPWM_PROTIRQHandler(uint32_t un32Event, void *pContext)
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    MPWM_ID_e eId = MPWM_ID_0;
    uint8_t un8Signal = 0;

    if(s_eMode == MPWM_MODE_INDIVIDUAL)
    {
        un8Signal = (MPWM_SIG_U | MPWM_SIG_V | MPWM_SIG_W); 
    }
    else
    {
        un8Signal = MPWM_SIG_PWM;
    }

    eErr = HAL_MPWM_Stop(eId, s_eMode, MPWM_STOP_MODE_CLEAR, un8Signal);
    if(eErr != HAL_ERR_OK)
    {
        return; 
    }

    LOG("%s (%d) Protection evt fired & stop\n", EX_MPWM_LOG_STR, eId);
}

static void EX_MPWM_OVIRQHandler(uint32_t un32Event, void *pContext)
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    MPWM_ID_e eId = MPWM_ID_0;
    uint8_t un8Signal = 0;

    if(s_eMode == MPWM_MODE_INDIVIDUAL)
    {
        un8Signal = (MPWM_SIG_U | MPWM_SIG_V | MPWM_SIG_W); 
    }
    else
    {
        un8Signal = MPWM_SIG_PWM;
    }

    eErr = HAL_MPWM_Stop(eId, s_eMode, MPWM_STOP_MODE_CLEAR, un8Signal);
    if(eErr != HAL_ERR_OK)
    {
        return; 
    }

    LOG("%s (%d) Over Voltage evt fired & stop\n", EX_MPWM_LOG_STR, eId);
}

static void EX_MPWM_PWMUIRQHandler(uint32_t un32Event, void *pContext)
{
    MPWM_ID_e eId = MPWM_ID_0;

    PRV_EX_MPWM_EventLog(eId, un32Event);
    if(s_eBLDCMode == EX_MPWM_BLDC_MODE_FORCE)
    {
        PRV_EX_MPWM_SetForcePhase(eId, s_bCW);
    }

    s_un32LogCurCnt++;
    if(s_un32LogCurCnt > s_un32LogDispCnt)
    {
        s_un32LogCurCnt = 0;
    }
}

static void EX_MPWM_PVIRQHandler(uint32_t un32Event, void *pContext)
{
    MPWM_ID_e eId = MPWM_ID_0;

    PRV_EX_MPWM_EventLog(eId, un32Event);

    s_un32LogCurCnt++;
    if(s_un32LogCurCnt > s_un32LogDispCnt)
    {
        s_un32LogCurCnt = 0;
    }
}

static void EX_MPWM_PWIRQHandler(uint32_t un32Event, void *pContext)
{
    MPWM_ID_e eId = MPWM_ID_0;

    PRV_EX_MPWM_EventLog(eId, un32Event);

    s_un32LogCurCnt++;
    if(s_un32LogCurCnt > s_un32LogDispCnt)
    {
        s_un32LogCurCnt = 0;
    }
}

static void EX_MPWM_PWMIRQHandler(uint32_t un32Event, void *pContext)
{

}

static void EX_MPWM_HallIRQHandler(uint32_t un32Event, void *pContext)
{
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;
    MPWM_ID_e eId = MPWM_ID_0;
    EX_MPWM_PHASE_e ePhase = EX_MPWM_PHASE_MAX;
    uint8_t un8NextPhase = 0;

    eDbgStatus = PRV_EX_MPWM_GetHallPosition(eId, &ePhase);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return;
    }

    PRV_EX_MPWM_GetNextPhase(eId, ePhase, &un8NextPhase);

    eDbgStatus = PRV_EX_MPWM_SetPhase(eId, s_bCW, s_aePhase[un8NextPhase]);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return;
    }
}

static enum debug_cmd_status PRV_EX_MPWM_GetEnableConfig(uint8_t n8Argc, char *pn8Argv[], bool *pbEnable)
{
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;
    uint8_t un8Arg = n8Argc;

    switch (pn8Argv[un8Arg++][0])
    {
        case 'e':
            *pbEnable = true;
            break;
        case 'd':
            *pbEnable = false;
            break;
        default:
            eDbgStatus = DEBUG_CMD_INVALID;
            LOG("(ERR) MPWM : en no proper argument.\n");
            break;
    }

    return eDbgStatus;
}

static enum debug_cmd_status PRV_EX_MPWM_GetSDConfig(uint8_t n8Argc, char *pn8Argv[], MPWM_SD_CFG_t *ptCfg)
{
    uint8_t un8Arg = n8Argc;

    ptCfg->un16HDuty = (uint16_t)atoi(pn8Argv[un8Arg++]);
    ptCfg->un16LDuty = (uint16_t)atoi(pn8Argv[un8Arg++]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status PRV_EX_MPWM_GetPortConfig(uint8_t n8Argc, char *pn8Argv[], MPWM_PORT_CFG_t *ptCfg)
{
    uint8_t un8Arg = n8Argc;

    switch (pn8Argv[un8Arg++][0])
    {
        case 'l':
            ptCfg->eOutLevel = MPWM_POL_LOW;
            break;
        case 'h':
            ptCfg->eOutLevel = MPWM_POL_HIGH;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_MPWM_ERR_STR, "SD[out]", EX_COMM_STR_OPT_MAX);
            goto err;
    }

    switch (pn8Argv[un8Arg++][0])
    {
        case 'l':
            ptCfg->eStartLevel = MPWM_POL_LOW;
            break;
        case 'h':
            ptCfg->eStartLevel = MPWM_POL_HIGH;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_MPWM_ERR_STR, "SD[start]", EX_COMM_STR_OPT_MAX);
            goto err;
    }

    switch (pn8Argv[un8Arg++][0])
    {
        case 'l':
            ptCfg->eForceLevel = MPWM_POL_LOW;
            break;
        case 'h':
            ptCfg->eForceLevel = MPWM_POL_HIGH;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_MPWM_ERR_STR, "SD[force]", EX_COMM_STR_OPT_MAX);
            goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status PRV_EX_MPWM_GetDTConfig(uint8_t n8Argc, char *pn8Argv[], MPWM_DT_CFG_t *ptCfg)
{
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;
    uint8_t un8Arg = n8Argc, un8Data = 0;
    bool bEnable = false;

    eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], &ptCfg->bEnable);
    if (eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_MPWM_ERR_STR, NULL, EX_COMM_STR_OPT_ENA);
        goto err;
    }
    
    switch (pn8Argv[un8Arg++][0])
    {
        case 'l':
            ptCfg->eMode = MPWM_DT_MODE_LEAD_L;
            break;
        case 'h':
            ptCfg->eMode = MPWM_DT_MODE_LEAD_H;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_MPWM_ERR_STR, "DT[mode]", EX_COMM_STR_OPT_MAX);
            goto err;
    }

    eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], &bEnable);
    if (eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_MPWM_ERR_STR, "DT[scp]", EX_COMM_STR_OPT_MAX);
        goto err;
    }

    ptCfg->bSCPDisable = (bEnable == true) ? false : true;

    un8Data = (uint8_t)(atoi(pn8Argv[un8Arg++]));
    if (un8Data == 2)
        ptCfg->ePreScale = MPWM_DT_PRESCALE_2;
    else if (un8Data == 4)
        ptCfg->ePreScale = MPWM_DT_PRESCALE_4;
    else if (un8Data == 8)
        ptCfg->ePreScale = MPWM_DT_PRESCALE_8;
    else if (un8Data == 16)
        ptCfg->ePreScale = MPWM_DT_PRESCALE_16;
    else
    {
        EX_COMMON_SetShowModuleLog(EX_MPWM_ERR_STR, "DT[pre]", EX_COMM_STR_OPT_MAX);
        goto err;
    }
    
    ptCfg->un8EdgeDTValue = (uint8_t)atoi(pn8Argv[un8Arg++]);

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_MPWM_GetClkConfig(int32_t n32Argc, char *pn8Argv[], MPWM_CLK_CFG_t *ptClkCfg)
{
    uint8_t un8Arg = 2;

    switch (pn8Argv[un8Arg++][0])
    {
        case 'm':
            ptClkCfg->eClk = MPWM_CLK_MCCR;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_MPWM_ERR_STR, NULL, EX_COMM_STR_OPT_SRC);
            goto err;
    }

    if(ptClkCfg->eClk == MPWM_CLK_MCCR)
    {
        if (pn8Argv[un8Arg][0] == 'l')
            ptClkCfg->eMccr = MPWM_CLK_MCCR_LSI;
        else if (pn8Argv[un8Arg][0] == 's')
            ptClkCfg->eMccr = MPWM_CLK_MCCR_LSE;
        else if (pn8Argv[un8Arg][0] == 'm')
            ptClkCfg->eMccr = MPWM_CLK_MCCR_MCLK;
        else if (pn8Argv[un8Arg][0] == 'h')
            ptClkCfg->eMccr = MPWM_CLK_MCCR_HSI;
        else if (pn8Argv[un8Arg][0] == 'e')
            ptClkCfg->eMccr = MPWM_CLK_MCCR_HSE;
        else if (pn8Argv[un8Arg][0] == 'p')
            ptClkCfg->eMccr = MPWM_CLK_MCCR_PLL;
        else
        {
            EX_COMMON_SetShowModuleLog(EX_MPWM_ERR_STR, NULL, EX_COMM_STR_OPT_MCCR);
            goto err;
        }

        un8Arg++;
        ptClkCfg->un8MccrDiv = atoi(pn8Argv[un8Arg++]);
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status PRV_EX_MPWM_GetAlertConfig(uint8_t n8Argc, char *pn8Argv[], MPWM_ALERT_CFG_t *ptCfg)
{
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;
    uint8_t un8Arg = n8Argc;

    eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], &ptCfg->bEnable);
    if (eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_MPWM_ERR_STR, NULL, EX_COMM_STR_OPT_ENA);
        goto err;
    }

    eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], &ptCfg->bIntrEnable);
    if (eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_MPWM_ERR_STR, "[intr]", EX_COMM_STR_OPT_MAX);
        goto err;
    }

    switch (pn8Argv[un8Arg++][0])
    {
        case 'l':
            ptCfg->eInPolarity = MPWM_POL_LOW;
            break;
        case 'h':
            ptCfg->eInPolarity = MPWM_POL_HIGH;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_MPWM_ERR_STR, "[inpol]", EX_COMM_STR_OPT_MAX);
            goto err;
    }

    ptCfg->un8Debounce = (uint8_t)(atoi(pn8Argv[un8Arg++]));

    eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], &ptCfg->bHLevelOutEnable);
    if (eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_MPWM_ERR_STR, "[hl]", EX_COMM_STR_OPT_MAX);
        goto err;
    }

    eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], &ptCfg->bLLevelOutEnable);
    if (eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_MPWM_ERR_STR, "[ll]", EX_COMM_STR_OPT_MAX);
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status PRV_EX_MPWM_GetModeConfig(uint8_t un8Argc, char *pn8Argv[], MPWM_MODE_e *peMode)
{
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;
    uint8_t un8Arg = un8Argc;

    switch (pn8Argv[un8Arg++][0])
    {
        case 'm':
            *peMode = MPWM_MODE_MOTOR;
            break;
        case 'p':
            *peMode = MPWM_MODE_NORMAL;
            break;
        case 'i':
            *peMode = MPWM_MODE_INDIVIDUAL;
            break;
        default:
            eDbgStatus = DEBUG_CMD_INVALID;
            EX_COMMON_SetShowModuleLog(EX_MPWM_ERR_STR, NULL, EX_COMM_STR_OPT_MODE);
            break;
    }

    return eDbgStatus;
}

static enum debug_cmd_status PRV_EX_MPWM_GetAdcTrgConfig(int32_t n32Argc, char *pn8Argv[], MPWM_ADCTRG_CFG_t *ptCfg)
{
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;
    uint8_t un8Arg = 2;

    ptCfg->un8Idx = (uint8_t)atoi(pn8Argv[un8Arg++]);

    switch (pn8Argv[un8Arg++][0])
    {
        case 'u':
            ptCfg->eSrc = MPWM_ADCTRG_SRC_U;
            break;
        case 'v':
            ptCfg->eSrc = MPWM_ADCTRG_SRC_V;
            break;
        case 'w':
            ptCfg->eSrc = MPWM_ADCTRG_SRC_W;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_MPWM_ERR_STR, NULL, EX_COMM_STR_OPT_SRC);
            goto err;
    }

    switch (pn8Argv[un8Arg++][0])
    {
        case 'n':
            ptCfg->eMode = MPWM_ADCTRG_MODE_NONE;
            break;
        case 'u':
            ptCfg->eMode = MPWM_ADCTRG_MODE_MATCH_UP;
            break;
        case 'd':
            ptCfg->eMode = MPWM_ADCTRG_MODE_MATCH_DOWN;
            break;
        case 'b':
            ptCfg->eMode = MPWM_ADCTRG_MODE_MATCH_UP_DOWN;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_MPWM_ERR_STR, NULL, EX_COMM_STR_OPT_MODE);
            goto err;
    }

    eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], &ptCfg->bIntrEnable);
    if (eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_MPWM_ERR_STR, "[intr]", EX_COMM_STR_OPT_MAX);
        goto err;
    }

    eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], &ptCfg->bTrgUpdate);
    if (eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_MPWM_ERR_STR, "[upd]", EX_COMM_STR_OPT_MAX);
        goto err;
    }

    ptCfg->un16GenData = (uint16_t)atoi(pn8Argv[un8Arg]);

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status PRV_EX_MPWM_GetConfig(int32_t n32Argc, char *pn8Argv[], MPWM_CFG_t *ptCfg)
{
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;
    uint8_t un8Arg = 2;

    eDbgStatus = PRV_EX_MPWM_GetModeConfig(un8Arg++, pn8Argv, &ptCfg->eMode);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    switch (pn8Argv[un8Arg++][0])
    {
        case 't':
            ptCfg->eChannelMode = MPWM_CHANNEL_MODE_TWO_SYM;
            break;
        case 's':
            ptCfg->eChannelMode = MPWM_CHANNEL_MODE_ONE_SYM;
            break;
        case 'a':
            ptCfg->eChannelMode = MPWM_CHANNEL_MODE_ONE_ASYM;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_MPWM_ERR_STR, "[chn]", EX_COMM_STR_OPT_MAX);
            goto err;
    }

    switch (pn8Argv[un8Arg++][0])
    {
        case 'u':
            ptCfg->eCounterMode = MPWM_COUNTER_MODE_UP;
            break;
        case 'b':
            ptCfg->eCounterMode = MPWM_COUNTER_MODE_UP_DOWN;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_MPWM_ERR_STR, NULL, EX_COMM_STR_OPT_CNT);
            goto err;
    }

    eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], &ptCfg->bPeriodMatch);
    if (eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_MPWM_ERR_STR, "[pm]", EX_COMM_STR_OPT_MAX);
        goto err;
    }

    eDbgStatus = EX_COMMON_GetEnable(&pn8Argv[un8Arg++][0], &ptCfg->bBottomMatch);
    if (eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        EX_COMMON_SetShowModuleLog(EX_MPWM_ERR_STR, "[bm]", EX_COMM_STR_OPT_MAX);
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status PRV_EX_MPWM_GetId(int32_t n32Argc, char *pn8Argv[], MPWM_ID_e *peId)
{
    uint8_t un8Data = 0;
    if(n32Argc == 1)
    {
        EX_COMMON_SetShowModuleLog(EX_MPWM_ERR_STR, (char *)pcOptStr[EX_COMM_STR_OPT_ARG], EX_COMM_STR_OPT_MAX);
        return DEBUG_CMD_INVALID;
    }

    un8Data = atoi(pn8Argv[1]); 
    if(un8Data >= CONFIG_MPWM_MAX_COUNT)
    {
        LOG("%s Max Chan %d\n", EX_MPWM_ERR_STR, CONFIG_MPWM_MAX_COUNT);
        return DEBUG_CMD_INVALID;
    }

    *peId = (MPWM_ID_e)un8Data;

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_MPWM_Init(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    MPWM_ID_e eId;

    eDbgStatus = PRV_EX_MPWM_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_MPWM_Init(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_MPWM_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_INIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_MPWM_Uninit(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    MPWM_ID_e eId;

    eDbgStatus = PRV_EX_MPWM_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eErr = HAL_MPWM_Uninit(eId);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_MPWM_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_UNINIT]);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_MPWM_SetClk(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    MPWM_ID_e eId;

    MPWM_CLK_CFG_t tClkCfg =
    {
        .eClk  = MPWM_CLK_MCCR,
        .eMccr = MPWM_CLK_MCCR_HSE,
        .un8MccrDiv = DEFAULT_HSE_1MHZ_DIV,
    };

    eDbgStatus = PRV_EX_MPWM_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eDbgStatus = EX_MPWM_GetClkConfig(n32Argc, pn8Argv, &tClkCfg);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_MPWM_SetClkConfig(eId, &tClkCfg);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }


    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_MPWM_SetConfig(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    MPWM_ID_e eId;
    MPWM_CFG_t tMPWMCfg;

    eDbgStatus = PRV_EX_MPWM_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    memset(&tMPWMCfg, 0x00, sizeof(MPWM_CFG_t));

    eDbgStatus = PRV_EX_MPWM_GetConfig(n32Argc, pn8Argv, &tMPWMCfg);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    s_eMode = tMPWMCfg.eMode;

    eErr = HAL_MPWM_SetConfig(eId, &tMPWMCfg);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_MPWM_SetPWMConfig(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    MPWM_ID_e eId;
    MPWM_PWM_CFG_t *ptPwmCfg;
    MPWM_SDP_CFG_t *ptSdpCfg;
    uint8_t un8Arg = 2;
    uint32_t un32Data = 0;

    eDbgStatus = PRV_EX_MPWM_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }
    
    ptPwmCfg = &s_tPWMCfg[(uint32_t)eId];

    if(strncmp(pn8Argv[un8Arg], "-ped", 4) == 0)
    {
        un8Arg++;
        ptPwmCfg->un16Period = (uint16_t)atoi(pn8Argv[un8Arg++]); 
    }

    if(strncmp(pn8Argv[un8Arg], "-intr", 4) == 0)
    {
        un8Arg++;
        sscanf(pn8Argv[un8Arg++], "%X", &un32Data);
        ptPwmCfg->un32IntrEnable = un32Data;
    }

    if(strncmp(pn8Argv[un8Arg], "-sdp", 4) == 0)
    {
        un8Arg++;
        switch (pn8Argv[un8Arg++][0])
        {
            case 'u':
                ptSdpCfg = &ptPwmCfg->tUSDPCfg;
                break;
            case 'v':
                ptSdpCfg = &ptPwmCfg->tVSDPCfg;
                break;
            case 'w':
                ptSdpCfg = &ptPwmCfg->tWSDPCfg;
                break;
            default:
                EX_COMMON_SetShowModuleLog(EX_MPWM_ERR_STR, "[sig]", EX_COMM_STR_OPT_MAX);
                goto err;
        }
        
        eDbgStatus = PRV_EX_MPWM_GetSDConfig(un8Arg, pn8Argv, &ptSdpCfg->tSDCfg);
        if(eDbgStatus != DEBUG_CMD_SUCCESS)
        {
            goto err;
        }

        un8Arg += 2;
        eDbgStatus = PRV_EX_MPWM_GetPortConfig(un8Arg, pn8Argv, &ptSdpCfg->tSPCfg.tHPort);
        if(eDbgStatus != DEBUG_CMD_SUCCESS)
        {
            goto err;
        }

        un8Arg += 3;
        eDbgStatus = PRV_EX_MPWM_GetPortConfig(un8Arg, pn8Argv, &ptSdpCfg->tSPCfg.tLPort);
        if(eDbgStatus != DEBUG_CMD_SUCCESS)
        {
            goto err;
        }
        un8Arg += 3;
    }

    if(strncmp(pn8Argv[un8Arg], "-dt", 3) == 0)
    {
        un8Arg++;
        eDbgStatus = PRV_EX_MPWM_GetDTConfig(un8Arg, pn8Argv, &ptPwmCfg->tDTCfg);
        if(eDbgStatus != DEBUG_CMD_SUCCESS)
        {
            goto err;
        }
        un8Arg += 5;
    }

    if(strncmp(pn8Argv[un8Arg], "-prot", 5) == 0)
    {
        un8Arg++;
        eDbgStatus = PRV_EX_MPWM_GetAlertConfig(un8Arg, pn8Argv, &ptPwmCfg->tProtCfg);
        if(eDbgStatus != DEBUG_CMD_SUCCESS)
        {
            goto err;
        }
        un8Arg += 6;
    }

    if(strncmp(pn8Argv[un8Arg], "-ov", 3) == 0)
    {
        un8Arg++;
        eDbgStatus = PRV_EX_MPWM_GetAlertConfig(un8Arg, pn8Argv, &ptPwmCfg->tOVCfg);
        if(eDbgStatus != DEBUG_CMD_SUCCESS)
        {
            goto err;
        }

        un8Arg += 6;
    }

    if(strncmp(pn8Argv[un8Arg], "-apply", 6) == 0)
    {
        eErr = HAL_MPWM_SetModeConfig(eId, s_eMode, (void *)ptPwmCfg);
        if(eErr != HAL_ERR_OK)
        {
            goto err;
        }
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_MPWM_SetIndivConfig(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    MPWM_ID_e eId;
    MPWM_INDIV_CFG_t *ptIndivCfg;
    MPWM_INDIV_SIG_CFG_t *ptIndivSigCfg;

    uint8_t un8Arg = 2;
    uint32_t un32Data = 0;

    eDbgStatus = PRV_EX_MPWM_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }
    
    ptIndivCfg = &s_tINDIVCfg[(uint32_t)eId];
    switch (pn8Argv[un8Arg++][0])
    {
        case 'u':
            ptIndivSigCfg = &ptIndivCfg->tUCfg;
            break;
        case 'v':
            ptIndivSigCfg = &ptIndivCfg->tVCfg;
            break;
        case 'w':
            ptIndivSigCfg = &ptIndivCfg->tWCfg;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_MPWM_ERR_STR, "[sig]", EX_COMM_STR_OPT_MAX);
            goto err;
    }

    if(strncmp(pn8Argv[un8Arg], "-ped", 4) == 0)
    {
        un8Arg++;
        ptIndivSigCfg->un16Period = (uint16_t)atoi(pn8Argv[un8Arg++]); 
    }

    if(strncmp(pn8Argv[un8Arg], "-intr", 5) == 0)
    {
        un8Arg++;
        sscanf(pn8Argv[un8Arg++], "%X", &un32Data);
        ptIndivSigCfg->un32IntrEnable = un32Data;
    }

    if(strncmp(pn8Argv[un8Arg], "-sdp", 4) == 0)
    {
        un8Arg++;
        eDbgStatus = PRV_EX_MPWM_GetSDConfig(un8Arg, pn8Argv, &ptIndivSigCfg->tSDPCfg.tSDCfg);
        if(eDbgStatus != DEBUG_CMD_SUCCESS)
        {
            goto err;
        }

        un8Arg += 2;
        eDbgStatus = PRV_EX_MPWM_GetPortConfig(un8Arg, pn8Argv, &ptIndivSigCfg->tSDPCfg.tSPCfg.tHPort);
        if(eDbgStatus != DEBUG_CMD_SUCCESS)
        {
            goto err;
        }

        un8Arg += 3;
        eDbgStatus = PRV_EX_MPWM_GetPortConfig(un8Arg, pn8Argv, &ptIndivSigCfg->tSDPCfg.tSPCfg.tLPort);
        if(eDbgStatus != DEBUG_CMD_SUCCESS)
        {
            goto err;
        }
        un8Arg += 3;
    }

    if(strncmp(pn8Argv[un8Arg], "-dt", 3) == 0)
    {
        un8Arg++;
        eDbgStatus = PRV_EX_MPWM_GetDTConfig(un8Arg, pn8Argv, &ptIndivSigCfg->tDTCfg);
        if(eDbgStatus != DEBUG_CMD_SUCCESS)
        {
            goto err;
        }
    }

    if(strncmp(pn8Argv[un8Arg], "-prot", 5) == 0)
    {
        un8Arg++;
        eDbgStatus = PRV_EX_MPWM_GetAlertConfig(un8Arg, pn8Argv, &ptIndivSigCfg->tProtCfg);
        if(eDbgStatus != DEBUG_CMD_SUCCESS)
        {
            goto err;
        }
    }

    if(strncmp(pn8Argv[un8Arg], "-ov", 3) == 0)
    {
        un8Arg++;
        eDbgStatus = PRV_EX_MPWM_GetAlertConfig(un8Arg, pn8Argv, &ptIndivSigCfg->tOVCfg);
        if(eDbgStatus != DEBUG_CMD_SUCCESS)
        {
            goto err;
        }
    }

    if(strncmp(pn8Argv[un8Arg], "-apply", 6) == 0)
    {
        eErr = HAL_MPWM_SetModeConfig(eId, s_eMode, (void *)ptIndivCfg);
        if(eErr != HAL_ERR_OK)
        {
        }
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_MPWM_SetIRQ(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    MPWM_ID_e eId;
    MPWM_OPS_e eOps;
    MPWM_INTR_HDLR_e eHdlr;
    pfnMPWM_IRQ_Handler_t pIRQHandler = NULL;
    uint8_t un8Arg = 2;
    uint32_t un32IRQPrio = 0;

    eDbgStatus = PRV_EX_MPWM_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    switch (pn8Argv[un8Arg++][0])
    {
        case 'p':
            eOps = MPWM_OPS_POLL;
            break;
        case 'i':
            eOps = MPWM_OPS_INTR;
            break;
        case 'n':
            eOps = MPWM_OPS_NMI;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_MPWM_ERR_STR, NULL, EX_COMM_STR_OPT_OPS);
            goto err;
    }
    
    switch (pn8Argv[un8Arg++][0])
    {
        case 'p':
            eHdlr = MPWM_INTR_HDLR_PROTECTION;
            pIRQHandler = EX_MPWM_PROTIRQHandler;
            break;
        case 'o':
            eHdlr = MPWM_INTR_HDLR_OVERVOLTAGE;
            pIRQHandler = EX_MPWM_OVIRQHandler;
            break;
        case 'u':
            eHdlr = MPWM_INTR_HDLR_SIG_U;
            pIRQHandler = EX_MPWM_PWMUIRQHandler;
            break;
        case 'v':
            eHdlr = MPWM_INTR_HDLR_SIG_V;
            pIRQHandler = EX_MPWM_PVIRQHandler;
            break;
        case 'w':
            eHdlr = MPWM_INTR_HDLR_SIG_W;
            pIRQHandler = EX_MPWM_PWIRQHandler;
            break;
        case 'm':
            eHdlr = MPWM_INTR_HDLR_PWM;
            pIRQHandler = EX_MPWM_PWMIRQHandler;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_MPWM_ERR_STR, "[hdlr]", EX_COMM_STR_OPT_MAX);
            goto err;
    }

    un32IRQPrio = (uint32_t)atoi(pn8Argv[un8Arg++]);

    s_tMPWMContext[eId].eId = eId;

    eErr = HAL_MPWM_SetIRQ(eId, eHdlr, eOps, pIRQHandler, (void *)&s_tMPWMContext[(uint32_t)eId], un32IRQPrio);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_MPWM_SetBLDCConfig(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;
    HAL_ERR_e eErr = HAL_ERR_OK;
    MPWM_ID_e eId;
    uint8_t un8Arg = 2, un8IntNum = 0;
    PCU_IRQ_CFG_t tIRQCfg;

    eDbgStatus = PRV_EX_MPWM_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }
    
    if(strncmp(pn8Argv[un8Arg], "-ref", 4) == 0)
    {
        un8Arg++;
        eDbgStatus = PRV_EX_MPWM_GetEnableConfig(un8Arg++, pn8Argv, &s_bIFX007TRefEnable);
    }

    if(strncmp(pn8Argv[un8Arg], "-mode", 5) == 0)
    {
        un8Arg++;
        switch (pn8Argv[un8Arg++][0])
        {
            case 'o':
                s_eBLDCMode = EX_MPWM_BLDC_MODE_MAX;
                break;
            case 'f':
                s_eBLDCMode = EX_MPWM_BLDC_MODE_FORCE;
                break;
            case 'h':
                s_eBLDCMode = EX_MPWM_BLDC_MODE_HALL_SENSOR;
                break;
            default:
                EX_COMMON_SetShowModuleLog(EX_MPWM_ERR_STR, "[-mode]", EX_COMM_STR_OPT_MAX);
                goto err;
        }
    }

    if(strncmp(pn8Argv[un8Arg], "-phdly", 6) == 0)
    {
        un8Arg++;
        s_un32PhaseDelay = (uint32_t)atoi(pn8Argv[un8Arg++]);
    }

    if(strncmp(pn8Argv[un8Arg], "-hall", 5) == 0)
    {
        un8Arg++;
        for (int i = 0; i < EX_MPWM_SIG_CNT; i++)
        {
            s_tHallPort[i].eId = (PCU_ID_e)atoi(pn8Argv[un8Arg++]);
            s_tHallPort[i].ePinId = (PCU_PIN_ID_e)atoi(pn8Argv[un8Arg++]);

            eErr = HAL_PCU_SetInOutMode(s_tHallPort[i].eId, s_tHallPort[i].ePinId, PCU_INOUT_INPUT);
            if(eErr != HAL_ERR_OK)
            {
                goto err;
            }

            eErr = HAL_PCU_SetPullUpDown(s_tHallPort[i].eId, s_tHallPort[i].ePinId, PCU_PUPD_UP);
            if(eErr != HAL_ERR_OK)
            {
                goto err;
            }


            eErr = HAL_PCU_SetIntrPort(s_tHallPort[i].eId, s_tHallPort[i].ePinId, PCU_INTR_MODE_EDGE, PCU_INTR_TRG_BOTH_LEVEL_EDGE, un8IntNum);
            if(eErr != HAL_ERR_OK)
            {
                goto err;
            }

            s_tMPWMPCUContext[s_tHallPort[i].eId].eId = s_tHallPort[i].eId;

            memset(&tIRQCfg, 0x00, sizeof(PCU_IRQ_CFG_t));

            tIRQCfg.eId = s_tHallPort[i].eId;
            tIRQCfg.eOps = PCU_OPS_INTR;
            tIRQCfg.pfnHandler = &EX_MPWM_HallIRQHandler;
            tIRQCfg.pContext = &s_tMPWMPCUContext[s_tHallPort[i].eId];
            tIRQCfg.un32IRQPrio = EX_MPWM_PCU_IRQ_PRIO;
            tIRQCfg.un8IntNum = un8IntNum;

            eErr = HAL_PCU_SetIRQ(&tIRQCfg);
            if(eErr != HAL_ERR_OK)
            {
                goto err;
            }
        }
    }

    if(strncmp(pn8Argv[un8Arg], "-dir", 4) == 0)
    {
        un8Arg++;
        switch (pn8Argv[un8Arg++][0])
        {
            case 'f':
                s_bCW = true;
                break;
            case 'b':
                s_bCW = false;
                break;
            default:
                EX_COMMON_SetShowModuleLog(EX_MPWM_ERR_STR, "[dir]", EX_COMM_STR_OPT_MAX);
                goto err;
        }
    }

    if(strncmp(pn8Argv[un8Arg], "-duty", 5) == 0)
    {
        un8Arg++;
        s_un16UHDuty = (uint16_t)atoi(pn8Argv[un8Arg++]);
        s_un16ULDuty = (uint16_t)atoi(pn8Argv[un8Arg++]);
        s_un16VHDuty = (uint16_t)atoi(pn8Argv[un8Arg++]);
        s_un16VLDuty = (uint16_t)atoi(pn8Argv[un8Arg++]);
        s_un16WHDuty = (uint16_t)atoi(pn8Argv[un8Arg++]);
        s_un16WLDuty = (uint16_t)atoi(pn8Argv[un8Arg++]);
        if(s_bIFX007TRefEnable == true)
        {
            s_un16IFX007TOnDuty = (uint16_t)atoi(pn8Argv[un8Arg++]);
            s_un16IFX007TOffDuty = (uint16_t)atoi(pn8Argv[un8Arg++]);
        }
    }

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_MPWM_SetAdcTrgConfig(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;
    HAL_ERR_e eErr = HAL_ERR_OK;
    MPWM_ID_e eId;
    MPWM_ADCTRG_CFG_t tCfg;

    eDbgStatus = PRV_EX_MPWM_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eDbgStatus = PRV_EX_MPWM_GetAdcTrgConfig(n32Argc, pn8Argv, &tCfg);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    eErr = HAL_MPWM_SetAdcTrgConfig(eId, &tCfg);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }
    
    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_MPWM_SetDuty(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;
    HAL_ERR_e eErr = HAL_ERR_OK;
    MPWM_ID_e eId;
    MPWM_SIG_e eSig, eCurSig;
    MPWM_SD_CFG_t tSDCfg;
    uint8_t un8Arg = 2, un8Cnt = 1;

    eDbgStatus = PRV_EX_MPWM_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }
    
    switch (pn8Argv[un8Arg++][0])
    {
        case 'u':
            eSig = MPWM_SIG_U;
            break;
        case 'v':
            eSig = MPWM_SIG_V;
            break;
        case 'w':
            eSig = MPWM_SIG_W;
            break;
        case 'm':
            eSig = MPWM_SIG_PWM;
            un8Cnt = 3;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_MPWM_ERR_STR, "[pha]", EX_COMM_STR_OPT_MAX);
            goto err;
    }

    for (int i = 0; i < un8Cnt; i++)
    {
        tSDCfg.un16HDuty = (uint16_t)atoi(pn8Argv[un8Arg++]);
        tSDCfg.un16LDuty = (uint16_t)atoi(pn8Argv[un8Arg++]);
        
        if(eSig == MPWM_SIG_PWM)
        {
            eCurSig = (MPWM_SIG_e)BIT(i + 1);
        }
        else
        {
            eCurSig = eSig;
        }
        
        eErr = HAL_MPWM_SetDuty(eId, eCurSig, &tSDCfg);
        if(eErr != HAL_ERR_OK)
        {
            goto err;
        }
    }
    
    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_MPWM_GetHallPosition(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus = DEBUG_CMD_SUCCESS;
    MPWM_ID_e eId;
    EX_MPWM_PHASE_e ePhase = EX_MPWM_PHASE_MAX;

    eDbgStatus = PRV_EX_MPWM_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    eDbgStatus = PRV_EX_MPWM_GetHallPosition(eId, &ePhase);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    LOG("%s Hall Sensor=0x%x\n", EX_MPWM_LOG_STR, (uint32_t)ePhase);

    return DEBUG_CMD_SUCCESS;
}

static enum debug_cmd_status EX_MPWM_Start(int32_t n32Argc, char *pn8Argv[])
{
    enum debug_cmd_status eDbgStatus;
    HAL_ERR_e eErr = HAL_ERR_OK;
    MPWM_ID_e eId;
    MPWM_START_MODE_e eStMode;
    EX_MPWM_PHASE_e ePhase = EX_MPWM_PHASE_MAX;
    uint8_t un8NextPhase = 0;
    uint8_t un8Phase = 0;
    uint8_t un8Arg = 2;
    uint32_t un32Data = 0;

    eDbgStatus = PRV_EX_MPWM_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        return eDbgStatus;
    }

    switch (pn8Argv[un8Arg++][0])
    {
        case 'c':
            eStMode = MPWM_START_MODE_RECOUNT;
            break;
        case 'r':
            eStMode = MPWM_START_MODE_RESUME;
            break;
        default:
            EX_COMMON_SetShowModuleLog(EX_MPWM_ERR_STR, "[stmode]", EX_COMM_STR_OPT_MAX);
            goto err;
    }

    sscanf(pn8Argv[un8Arg++], "%X", &un32Data);
    un8Phase = (uint8_t)un32Data;

    if(s_eBLDCMode == EX_MPWM_BLDC_MODE_HALL_SENSOR)
    {
        eDbgStatus = PRV_EX_MPWM_GetHallPosition(eId, &ePhase);
        if(eDbgStatus != DEBUG_CMD_SUCCESS)
        {
            goto err;
        }

        PRV_EX_MPWM_GetNextPhase(eId, ePhase, &un8NextPhase);

        (void)PRV_EX_MPWM_SetPhase(eId, s_bCW, s_aePhase[un8NextPhase]);
    }
    else if(s_eBLDCMode == EX_MPWM_BLDC_MODE_FORCE)
    {
        (void)PRV_EX_MPWM_SetPhase(eId, s_bCW, s_aePhase[un8NextPhase]);
    }

    eErr = HAL_MPWM_Start(eId, s_eMode, eStMode, un8Phase);
    if(eErr != HAL_ERR_OK)
    {
        goto err;
    }

    LOG("%s (%d) %s\n", EX_MPWM_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_START]);

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_MPWM_Stop(int32_t n32Argc, char *pn8Argv[])
{
    HAL_ERR_e eErr = HAL_ERR_OK;
    enum debug_cmd_status eDbgStatus;
    MPWM_ID_e eId;
    MPWM_STOP_MODE_e eSpMode;
    uint8_t un8Phase = 0;
    uint8_t un8Arg = 2;
    uint32_t un32Data = 0;

    eDbgStatus = PRV_EX_MPWM_GetId(n32Argc, pn8Argv, &eId);
    if(eDbgStatus != DEBUG_CMD_SUCCESS)
    {
        goto err;
    }

    switch (pn8Argv[un8Arg++][0])
    {
        case 'c':
            eSpMode = MPWM_STOP_MODE_CLEAR;
            break;
        case 'm':
            eSpMode = MPWM_STOP_MODE_REMAIN;
            break;
        case 'r':
            eSpMode = MPWM_STOP_MODE_RESET;
            break;
        case 'd':
            eSpMode = MPWM_STOP_MODE_DISABLE;
            break;
        default:
            eDbgStatus = DEBUG_CMD_INVALID;
            EX_COMMON_SetShowModuleLog(EX_MPWM_ERR_STR, "[spmode]", EX_COMM_STR_OPT_MAX);
            break;
    }

    sscanf(pn8Argv[un8Arg++], "%X", &un32Data);
    un8Phase = (uint8_t)un32Data;

    eErr = HAL_MPWM_Stop(eId, s_eMode, eSpMode, un8Phase);
    if(eErr != HAL_ERR_OK)
    {
        return DEBUG_CMD_INVALID;
    }

    LOG("%s (%d) %s\n", EX_MPWM_LOG_STR, (uint32_t)eId, pcCmdStr[EX_COMM_STR_CMD_STOP]);

    return DEBUG_CMD_SUCCESS;

err:
    return DEBUG_CMD_INVALID;
}

static enum debug_cmd_status EX_MPWM_SetLog(int32_t n32Argc, char *pn8Argv[])
{
    if((strncmp(pn8Argv[1],"on",2) == 0))
    {
        s_bISRLog = true;
        if(n32Argc == 3)
        {
            s_un32LogDispCnt = (uint32_t)atoi(pn8Argv[2]);
        }
    }
    else
    {
        s_bISRLog = false;
    }

    LOG("%s ISR Log %s.\n", EX_MPWM_LOG_STR, (s_bISRLog == true ? "on":"off"));

    return DEBUG_CMD_SUCCESS;
}

static const struct debug_cmd s_tEX_MPWM_CMD[] =
{
    {"MPWM", "h", EX_MPWM_Help, "help"},
    {"MPWM", "init", EX_MPWM_Init, ""},
    {"MPWM", "uninit", EX_MPWM_Uninit, ""},
    {"MPWM", "clk", EX_MPWM_SetClk, ""},
    {"MPWM", "config", EX_MPWM_SetConfig, ""},
    {"MPWM", "irq", EX_MPWM_SetIRQ, ""},
    {"MPWM", "pwm", EX_MPWM_SetPWMConfig, ""},
    {"MPWM", "indiv", EX_MPWM_SetIndivConfig, ""},
    {"MPWM", "bldc", EX_MPWM_SetBLDCConfig, ""},
    {"MPWM", "adctrg", EX_MPWM_SetAdcTrgConfig, ""},
    {"MPWM", "duty", EX_MPWM_SetDuty, ""},
    {"MPWM", "hall", EX_MPWM_GetHallPosition, ""},
    {"MPWM", "start", EX_MPWM_Start, ""},
    {"MPWM", "stop", EX_MPWM_Stop, ""},
    {"MPWM", "log", EX_MPWM_SetLog, ""}
};

/**********************************************************************
 * @brief		Main program
 * @param[in]	None
 * @return	None
 **********************************************************************/
void EX_MPWM(void)
{
    /* Add EX commands */
    debug_cmd_init(s_tEX_MPWM_CMD,DEBUG_CMD_LIST_COUNT(s_tEX_MPWM_CMD));
}

#endif
/* --------------------------------- End Of File ------------------------------ */
