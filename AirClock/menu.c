/**
 *******************************************************************************
 * @file        menu.c
 * @author      ABOV R&D Division
 * @brief       EXAMPLE Menu UI Implementation
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
#include "debug_log.h"
#include "oled.h"
#include "u8g2_graphic.h"
#include "virtual_key.h"
#include "button.h"
#include "ir_receiver.h"

#include "buzzer_bmx2705.h"
#include "hw_timer.h"
#include "time.h"
#include "menu.h"
#include "airclock_sensor.h"
#include "snor_w25q16jv.h"
#include "leddrv_al8700.h"
#include "math.h"
/************************* Clock, Air ***********************************/
static time_t s_tmUnixTime = 1714731010; /* 2024.05.03.10:10:10 (GMT+0) */
struct tm stCurTime = {0};
/************************************************************************/
static uint8_t CLOCK_AMPM[3] = {0};
static uint8_t CLOCK_MONTHDAY[6] = {0};
static uint8_t CLOCK_HMS[9] = {0};
static uint8_t s_TimeEditing;
/************************* Grahic ***************************************/
static u8g2_t u8g2;

/************************* sensor ***************************************/
typedef struct
{
    float temp;      
    float humi;       
    
    uint8_t read_interval;     
    uint8_t read_set_time;     
    uint8_t raed_set_focus;
    
    uint8_t alarm_temp_flag;
    uint8_t alarm_humi_flag;
    uint8_t alarm_temp_focus;
    uint8_t alarm_humi_focus;
    
    char str_temp[10];       
    char str_humi[10];         
    char alarm_temp[4];
    char alarm_humi[4];
} Sensor_A96H115;
Sensor_A96H115 h115 = {0};
/************************* Buzzer ***************************************/
extern uint8_t g_BuzzerVolumeMode;
extern uint8_t g_BuzzerLevel;
uint8_t s_preBuzzerLevel;
/************************* SNOR *****************************************/
#define SNOR_TIME_BASEADDR 0x1F0000

uint8_t s_snorSaveFlag;

typedef struct {
    uint16_t year;
    uint8_t mon;
    uint8_t mday;
    uint8_t hour;
    uint8_t min;
    
    uint8_t h115_set_time; 
    uint8_t h115_set_focus; 
    uint8_t h115_alarm_temp_focus;
    uint8_t h115_alarm_humi_focus;

    uint8_t led_bright;
} FlashSaveData;

FlashSaveData saveData;
/************************************************************************/
#define MENU_ITEMS_COUNT 9

typedef struct {
    const char *label;
    const char * const *submenu;
    uint8_t submenu_count;
} MenuItem;

static const char * const submenu_time[]  = {"1. 1s", "2. 10s", "3. 30s", "Back"};
static const char * const submenu_temp_alarm[]  = {"OFF", "100", "90", "80", "70", "60", "50", "40", "30", "20", "10", "0", "-10", "-20", "-30"};
static const char * const submenu_humi_alarm[]  = {"OFF", "0", "10", "20", "30", "40", "50", "60", "70", "80", "90", "100"};
static const char * const submenu_led_bright[]  = {"1. level 1", "2. level 2", "3. level 3", "4. level 4", "5. level 5", "6. level 6", "7. level 7", "8. level 8"};


static const MenuItem menu_lv1[MENU_ITEMS_COUNT] = {    
    { "1. Measure Peroid",  submenu_time,  (uint8_t)(sizeof(submenu_time)/sizeof(submenu_time[0])) },
    { "2. Temp Alarm", submenu_temp_alarm, (uint8_t)(sizeof(submenu_temp_alarm)/sizeof(submenu_temp_alarm[0])) },
    { "3. Humi Alarm", submenu_humi_alarm, (uint8_t)(sizeof(submenu_humi_alarm)/sizeof(submenu_humi_alarm[0])) },
    { "4. Date Setting", 0,  0 },
    { "5. Time Setting", 0,  0 },
    { "6. Volume Setting", 0,  0 },
    { "7. LED Bright", submenu_led_bright,  (uint8_t)(sizeof(submenu_led_bright)/sizeof(submenu_led_bright[0])) },
    { "8. Information", 0,  0 },
    { "9. Back", 0,  0 },
};

typedef enum { 
    DEPTH_LOADING_SCREEN = 0,
    DEPTH_HOME_SCREEN,
    DEPTH_LV1,
    DEPTH_LV2,
    DEPTH_ALARM
} Depth;

typedef struct {
    Depth depth;
    uint8_t focus_lv1;
    uint8_t focus_lv2;
    
    uint8_t focus_year;
    uint8_t focus_mon;
    uint8_t focus_mday;
    
    uint8_t focus_hour;
    uint8_t focus_min;
} MenuState;

static MenuState ui = { 
    .depth = DEPTH_LOADING_SCREEN,
    .focus_lv1 = 0,
    .focus_lv2 = 0
};
/************************************************************************/
static void draw_icon_thermometer(int x, int y ) 
{
  int outerR = 5;       
  int innerR = 3;       
  int stemW = 5;        
  int stemH = 10;       
  int baseY = y + stemH + outerR - 1;  

  u8g2_SetDrawColor(&u8g2, 1);
  u8g2_DrawCircle(&u8g2, x, baseY, outerR, U8G2_DRAW_ALL);
  u8g2_DrawBox(&u8g2, x - stemW/2, y, stemW, stemH);

  u8g2_SetDrawColor(&u8g2, 0);
  u8g2_DrawCircle(&u8g2, x, baseY, innerR, U8G2_DRAW_ALL);
  u8g2_DrawBox(&u8g2, x - (stemW/2 - 1), y + 1, stemW - 2, stemH + 1);

  u8g2_SetDrawColor(&u8g2, 1);
  u8g2_DrawDisc(&u8g2, x, baseY, innerR - 1, U8G2_DRAW_ALL);
  u8g2_DrawBox(&u8g2, x, baseY - outerR - 2, 1, 5);
}

static void draw_icon_humidity(int x, int y) 
{
  int r = 4;        
  int cy = y + 9;   
  int neckY = cy - 3;
    
  for (int i = 0; i < r + 1; i++)
  {          
    int halfWidth = i / 2;                   
    int startX = x - halfWidth;
    int endX   = x + halfWidth;
    int lineY  = y + i;
    u8g2_DrawHLine(&u8g2, startX, lineY, endX - startX + 1);
  }
  
  u8g2_DrawDisc(&u8g2, x, cy, r, U8G2_DRAW_ALL);
  
  for (int i = -r/2; i <= r/2; i++) 
  {
    int vx = x + i;
    u8g2_DrawVLine(&u8g2, vx, neckY - 2, 3);  
  }
}

static void draw_icon_bell(u8g2_t *u8g2, int x, int y)
{
    int bellWidth = 15;
    int bellHeight = 15;

    u8g2_DrawCircle(u8g2, x + bellWidth / 2, y + 6, 6, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT);

    u8g2_DrawTriangle(u8g2, x + 4,  y + 6, x + bellWidth - 4, y + 6, x + bellWidth, y + bellHeight); 
    u8g2_DrawTriangle(u8g2, x + 4,  y + 6, x, y + bellHeight, x + bellWidth, y + bellHeight);

    u8g2_DrawHLine(u8g2, x, y + bellHeight, bellWidth);
    u8g2_DrawDisc(u8g2, x + bellWidth / 2, y + bellHeight + 3, 2, U8G2_DRAW_ALL);

    u8g2_DrawCircle(u8g2, x - 3, y + bellHeight / 2, 5, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_LOWER_LEFT);
    u8g2_DrawCircle(u8g2, x + bellWidth + 3, y + bellHeight / 2, 5, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_LOWER_RIGHT);
}
static void draw_SpeakerIcon(int x, int y)
{
    u8g2_DrawBox(&u8g2, x, y + 4, 6, 8);

    u8g2_DrawTriangle(&u8g2, x + 6, y + 4, x + 14, y, x + 6, y + 11); 
    u8g2_DrawTriangle(&u8g2, x + 6, y + 11, x + 14, y + 15, x + 14, y);

    if (g_BuzzerLevel > 1) 
    {
        u8g2_DrawArc(&u8g2, x + 15, y + 7, 4, 200, 60); 
    }    
    if (g_BuzzerLevel > 10) 
    {
        u8g2_DrawArc(&u8g2, x + 15, y + 7, 7, 200, 60);
    }    
    if (g_BuzzerLevel > 20) 
    {
        u8g2_DrawArc(&u8g2, x + 15, y + 7, 10, 200, 60);
    }
}
#if 0
static void draw_arrow_up(u8g2_t *u8g2, int x, int y, int size)
{
    u8g2_DrawTriangle(u8g2, x, y - size, x - size, y + size, x + size, y + size); 
}
#endif

static void draw_arrow_down(u8g2_t *u8g2, int x, int y, int size)
{
    u8g2_DrawTriangle(u8g2, x, y + size, x - size, y - size, x + size, y - size); 
}

static void draw_menu_item(uint8_t x, uint8_t y, const char *text, bool selected)
{
    int w = u8g2_GetStrWidth(&u8g2, text);
    int h = u8g2_GetMaxCharHeight(&u8g2);

    // set inversion if focused
    if (selected)
    {
        u8g2_SetDrawColor(&u8g2, 1);
        DrawBox(&u8g2, x, y, w, h);

        u8g2_SetFontMode(&u8g2, 0);
        u8g2_SetDrawColor(&u8g2, 0);
    }

    u8g2_DrawStr(&u8g2, x, y, text);

    // set normal again
    if (selected)
    {
        u8g2_SetFontMode(&u8g2, 1);
        u8g2_SetDrawColor(&u8g2, 1);
    }   
}

/************************************************************************/
void save_time_snor(void)
{
    saveData.year = (uint16_t)stCurTime.tm_year;
    saveData.mon  = (uint8_t)stCurTime.tm_mon;
    saveData.mday = (uint8_t)stCurTime.tm_mday;
    saveData.hour = (uint8_t)stCurTime.tm_hour;
    saveData.min  = (uint8_t)stCurTime.tm_min;  
    
    saveData.h115_set_time = h115.read_set_time;
    saveData.h115_set_focus = h115.raed_set_focus;
    saveData.h115_alarm_temp_focus = h115.alarm_temp_focus;
    saveData.h115_alarm_humi_focus = h115.alarm_humi_focus;
    
    EX_SNOR_SetErase(SNOR_TIME_BASEADDR, EX_SNOR_ERASE_4K); 

    EX_SNOR_Write(SNOR_TIME_BASEADDR, (uint8_t*)&saveData, sizeof(saveData));
}
void load_time_snor(void)
{
    EX_SNOR_Read(SNOR_TIME_BASEADDR, (uint8_t*)&saveData, sizeof(saveData));

    if(saveData.year != 0xFFFF) 
    {
        stCurTime.tm_year = saveData.year;
        stCurTime.tm_mon  = saveData.mon;
        stCurTime.tm_mday = saveData.mday;
        stCurTime.tm_hour = saveData.hour;
        stCurTime.tm_min  = saveData.min;
        
        h115.read_set_time = saveData.h115_set_time;
        h115.raed_set_focus = saveData.h115_set_focus;
        h115.alarm_temp_focus = saveData.h115_alarm_temp_focus;
        h115.alarm_humi_focus = saveData.h115_alarm_humi_focus;
        
        time_t new_sec = mktime(&stCurTime);
        if (new_sec != (time_t)-1) 
        {
            s_tmUnixTime = new_sec;
        }        
    }
    else
    {
        h115.read_set_time = 1;
        h115.raed_set_focus = 0;
        h115.alarm_temp_focus = 0;
        h115.alarm_humi_focus = 0;
    }
}
/************************************************************************/
static void render_frame(u8g2_t *u8g2, int x, int y, const char *str, int pad)
{
    int w = u8g2_GetStrWidth(u8g2, str);
    int ascent = u8g2_GetAscent(u8g2);
    int descent = u8g2_GetDescent(u8g2);
    int h = ascent - descent;  

    int frame_x = x - pad - 1;
    int frame_y = y - pad;                
    int frame_w = w + pad * 2 + 2;
    int frame_h = h + pad * 2;            

    u8g2_DrawFrame(u8g2, frame_x, frame_y, frame_w, frame_h);
}

void render_time(void)
{
    if (ui.depth != DEPTH_HOME_SCREEN)
        return;
    
    char str_clock_hm[6];
    
    // hour, miniute
    strncpy(str_clock_hm, (char *)CLOCK_HMS, 5);
    str_clock_hm[5] = '\0';
    
    EX_OLED_SetFont(&u8g2, FONT_MONOCHROME_LARGE);

    // clear & update time area
    u8g2_SetDrawColor(&u8g2, 0);
    DrawBox(&u8g2, 0, 35, 128, u8g2_GetMaxCharHeight(&u8g2)-2);
    u8g2_SetDrawColor(&u8g2, 1);
    u8g2_DrawStr(&u8g2, 42, 35, (char *)str_clock_hm);
    
    // AM, PM
    EX_OLED_SetFont(&u8g2, FONT_MONOCHROME_NORMAL);     
    u8g2_DrawStr(&u8g2, 24, 35, (char *)CLOCK_AMPM);
    
    u8g2_SendBuffer(&u8g2);
}

void render_temp_humi(void)
{
    if (ui.depth != DEPTH_HOME_SCREEN)
        return;
     
    // temp & humidity icon        
    draw_icon_thermometer(5, 1);    
    draw_icon_humidity(80, 2);
           
    // temp & humidity  
    EX_OLED_SetFont(&u8g2, FONT_MONOCHROME_MEDIUM);
    u8g2_DrawStr(&u8g2, 13, 6, (char *)"TEMP");    
    u8g2_DrawStr(&u8g2, 87, 6, (char *)"HUMI");    
    
    // clear & update area
    u8g2_SetDrawColor(&u8g2, 0);
    DrawBox(&u8g2, 0, 21, 128, u8g2_GetMaxCharHeight(&u8g2));
    u8g2_SetDrawColor(&u8g2, 1);
    
    // temp & humidity value 
    u8g2_DrawStr(&u8g2, 13, 21, (char *)h115.str_temp);     
    u8g2_DrawStr(&u8g2, 87, 21, (char *)h115.str_humi); 
    
    int dotX = 13 + u8g2_GetStrWidth(&u8g2, h115.str_temp) + 1; 
    int dotY = 22 - 1;

    u8g2_DrawPixel(&u8g2, dotX, dotY);       // top
    u8g2_DrawPixel(&u8g2, dotX - 1, dotY + 1);
    u8g2_DrawPixel(&u8g2, dotX + 1, dotY + 1);
    u8g2_DrawPixel(&u8g2, dotX, dotY + 2);   // bottom   
      
    u8g2_DrawStr(&u8g2, dotX+1, 22, (char *)"C"); 
    
    u8g2_SendBuffer(&u8g2);
}
#if 0
void render_home_top(void)
{
    int len = 0;
    EX_OLED_SetFont(&u8g2, FONT_MONOCHROME_MEDIUM);

    // Month, Day
    u8g2_DrawStr(&u8g2, 0, 0, (char *)CLOCK_MONTHDAY);
    len = u8g2_GetStrWidth(&u8g2, (char *)CLOCK_MONTHDAY) + 10;

    // AM, PM
    u8g2_DrawStr(&u8g2, len, 0, (char *)CLOCK_AMPM);
}
#endif
void render_home_bottom(void)
{
    char bottom_menu[] = "MENU";
    char bottom_rst[] = "RESET";
    char bottom_ok[] = "OK";
        
    EX_OLED_SetFont(&u8g2, FONT_MONOCHROME_NORMAL);
    
    u8g2_DrawStr(&u8g2, 4, 54, bottom_rst);
    render_frame(&u8g2, 4, 54, bottom_rst, 1); 
    
    if (ui.depth == DEPTH_HOME_SCREEN)
    {
        u8g2_DrawStr(&u8g2, 100, 54, bottom_menu);   
        render_frame(&u8g2, 100, 54, bottom_menu, 1);
    }
    else // DEPTH_LV1, DEPTH_LV2
    {
        draw_arrow_down(&u8g2, 65, 59, 3);
        render_frame(&u8g2, 60, 54, bottom_ok, 1);
        
        u8g2_DrawStr(&u8g2, 110, 54, bottom_ok);   
        render_frame(&u8g2, 110, 54, bottom_ok, 1);
    }
}
void render_home(void)
{
    if (ui.depth != DEPTH_HOME_SCREEN)
        return;

    u8g2_ClearBuffer(&u8g2);
    
    render_temp_humi();
    render_home_bottom();
    render_time();
    
    u8g2_SendBuffer(&u8g2);
}

static void render_lv1(void)
{
    u8g2_ClearDisplay(&u8g2);
    EX_OLED_SetFont(&u8g2, FONT_MONOCHROME_NORMAL);
    
    uint8_t x = 2;
    uint8_t y = 0;
    uint8_t start_cnt, end_cnt = 0;
    
    uint8_t line_h = (uint8_t)u8g2_GetMaxCharHeight(&u8g2) + 3; // line gap
    
    DrawCenteredString(&u8g2, y, "MENU");
    
    if (ui.focus_lv1 < 3)
    {
        start_cnt = 0;
        end_cnt = 3;
    }
    else if (ui.focus_lv1 < 6)
    {
        start_cnt = 3;
        end_cnt = 6;
    }
    else if (ui.focus_lv1 < MENU_ITEMS_COUNT)
    {
        start_cnt = 6;
        end_cnt = MENU_ITEMS_COUNT;
    }
    
    for (uint8_t i = start_cnt; i < end_cnt; i++)
    {
        bool focused = (i == ui.focus_lv1);
        y = ((i%3)+1)*line_h;
        draw_menu_item(x, y, menu_lv1[i].label, focused);
    }
        
    render_home_bottom();
    
    u8g2_SendBuffer(&u8g2);
}

static void render_lv2(void)
{
    u8g2_ClearDisplay(&u8g2);
    EX_OLED_SetFont(&u8g2, FONT_MONOCHROME_NORMAL);

    uint8_t x = 2;
    uint8_t y = 0;
    
    uint8_t line_h = (uint8_t)u8g2_GetMaxCharHeight(&u8g2) + 3; // line gap
    const MenuItem *m = &menu_lv1[ui.focus_lv1];

    // show current path
    uint8_t path[32] = {0};
    snprintf((char *)path, 32, "%s%s%s", "[", m->label, "]");
    DrawCenteredString(&u8g2, y, (char *)path);
    
    if(strcmp(m->label, "1. Measure Peroid") == 0)
    {
        if (ui.focus_lv2 < 3)
        {
            for (uint8_t i = 0; i < (m->submenu_count-1); i++)
            {
                bool focused = (i == ui.focus_lv2);
                y = ((i%3)+1)*line_h;
                draw_menu_item(x, y, m->submenu[i], focused);
            }
        }
        else if (ui.focus_lv2 < 4)
        {
             y = ((3%3)+1)*line_h;
             draw_menu_item(x, y, m->submenu[ui.focus_lv2], 3);
        }  
    }
    else if(strcmp(m->label, "2. Temp Alarm") == 0)
    {
        draw_icon_bell(&u8g2, 54, 12);
        draw_icon_thermometer(40, 32); 
        
        EX_OLED_SetFont(&u8g2, FONT_MONOCHROME_LARGE);         
        u8g2_DrawStr(&u8g2, 50, 35, m->submenu[ui.focus_lv2]);
        
        int dotX = 83; 
        int dotY = 35 - 1;
        
        u8g2_DrawPixel(&u8g2, dotX, dotY);       // top
        u8g2_DrawPixel(&u8g2, dotX - 1, dotY + 1);
        u8g2_DrawPixel(&u8g2, dotX + 1, dotY + 1);
        u8g2_DrawPixel(&u8g2, dotX, dotY + 2);   // bottom   
          
        u8g2_DrawStr(&u8g2, dotX+1, 35, (char *)"C"); 
        
    }
    else if(strcmp(m->label, "3. Humi Alarm") == 0)
    {
        draw_icon_bell(&u8g2, 54, 12);
        draw_icon_humidity(40, 35); 
        
        EX_OLED_SetFont(&u8g2, FONT_MONOCHROME_LARGE);         
        u8g2_DrawStr(&u8g2, 50, 35, m->submenu[ui.focus_lv2]);
        u8g2_DrawStr(&u8g2, 83, 35, (char *)"%");
    }
    else if(strcmp(m->label, "4. Date Setting") == 0)
    {
         char year_str[10], mon_str[3], mday_str[3];
        uint8_t line_x;

        EX_OLED_SetFont(&u8g2, FONT_MONOCHROME_LARGE);   
        
        sprintf(year_str, "%d", stCurTime.tm_year + 1900);
        sprintf(mon_str, "%d", stCurTime.tm_mon + 1);
        sprintf(mday_str, "%d", stCurTime.tm_mday);
        
        u8g2_DrawStr(&u8g2, 10, 25, year_str);
        u8g2_DrawStr(&u8g2, 50, 25, (char *)"/");
        u8g2_DrawStr(&u8g2, 60, 25, mon_str);
        line_x = u8g2_GetStrWidth(&u8g2, mon_str); 
        u8g2_DrawStr(&u8g2, line_x+60, 25, (char *)"/");
        u8g2_DrawStr(&u8g2, line_x+70, 25, mday_str);
        
        if(ui.focus_year)
        {
            draw_menu_item(10, 25, year_str, 1);
        }
        else if(ui.focus_mon)
        {
            draw_menu_item(60, 25, mon_str, 1);
        }
        else if(ui.focus_mday)
        {
            draw_menu_item(line_x+70, 25, mday_str, 1);
        }
    }
    else if (strcmp(m->label, "5. Time Setting") == 0)
    {
        char hour_str[3], min_str[3];
        
        EX_OLED_SetFont(&u8g2, FONT_MONOCHROME_LARGE);  
        
        sprintf(hour_str, "%d", stCurTime.tm_hour);
        sprintf(min_str, "%d", stCurTime.tm_min);
        
        u8g2_DrawStr(&u8g2, 40, 25, hour_str);
        u8g2_DrawStr(&u8g2, 60, 23, (char *)":");
        u8g2_DrawStr(&u8g2, 70, 25, min_str);
        
        if(ui.focus_hour)
        {
            draw_menu_item(40, 25, hour_str, 1);
        }
        else if(ui.focus_min)
        {
            draw_menu_item(70, 25, min_str, 1);
        }
    }
    else if (strcmp(m->label, "6. Volume Setting") == 0)
    {        
        uint16_t smoothWidth = (g_BuzzerLevel * 102) / 40;
        
        draw_SpeakerIcon(50, 15);
        
        u8g2_DrawFrame(&u8g2, 10, 35, 106, 12); 
  
        if (g_BuzzerLevel > 0) 
        {
            u8g2_DrawBox(&u8g2, 12, 37, smoothWidth, 8);
        }
        
        u8g2_SetDrawColor(&u8g2, 0); 
        for (int i = 1; i < 10; i++) 
        {
            u8g2_DrawVLine(&u8g2, 12 + (i * 10), 37, 8);
        }
        u8g2_SetDrawColor(&u8g2, 1); 
    }
    if(strcmp(m->label, "7. LED Bright") == 0)
    {
        uint8_t start_cnt, end_cnt = 0;
        if (ui.focus_lv2 < 3)
        {
            start_cnt = 0;
            end_cnt = 3;
        }
        else if (ui.focus_lv2 < 6)
        {
            start_cnt = 3;
            end_cnt = 6;
        }
        else if (ui.focus_lv2 < m->submenu_count)
        {
            start_cnt = 6;
            end_cnt = m->submenu_count;
        }

        for (uint8_t i = start_cnt; i < end_cnt; i++)
        {
            bool focused = (i == ui.focus_lv2);
            y = ((i%3)+1)*line_h;
            draw_menu_item(x, y, m->submenu[i], focused);
        }
    }
    else if(strcmp(m->label, "8. Information") == 0)
    {
        y = ((0%3)+1)*line_h;        
        u8g2_DrawStr(&u8g2, 4, y, "MCU    : A31C144");
        y = ((1%3)+1)*line_h;    
        u8g2_DrawStr(&u8g2, 4, y, "Sensor : A96H115");
        y = ((2%3)+1)*line_h;
        u8g2_DrawStr(&u8g2, 4, y, "F/W Ver: v1.0.0");        
    }
    
    render_home_bottom();
    
    u8g2_SendBuffer(&u8g2);
}


static void render_alarm(void)
{          
    EX_OLED_SetFont(&u8g2, FONT_MONOCHROME_NORMAL);
    
    char bottom_ok[] = "OK";

    DrawCenteredString(&u8g2, 0, "Alarm");
    
    u8g2_DrawStr(&u8g2, 110, 54, bottom_ok);   
    render_frame(&u8g2, 110, 54, bottom_ok, 1);
    
    if(h115.alarm_temp_flag)
    {        
        draw_icon_bell(&u8g2, 10, 12);
        u8g2_DrawStr(&u8g2, 40, 18, "Temperature");  
        draw_icon_thermometer(40, 38);
                
        // temp value 
        EX_OLED_SetFont(&u8g2, FONT_MONOCHROME_LARGE);
        u8g2_DrawStr(&u8g2, 50, 40, h115.alarm_temp);     
        
        int dotX = 50 + u8g2_GetStrWidth(&u8g2, h115.alarm_temp) + 1; 
        int dotY = 41 - 1;
        
        u8g2_DrawPixel(&u8g2, dotX, dotY);       // top
        u8g2_DrawPixel(&u8g2, dotX - 1, dotY + 1);
        u8g2_DrawPixel(&u8g2, dotX + 1, dotY + 1);
        u8g2_DrawPixel(&u8g2, dotX, dotY + 2);   // bottom   
          
        u8g2_DrawStr(&u8g2, dotX+1, 40, (char *)"C"); 
    }
    else if(h115.alarm_humi_flag)
    {             
        draw_icon_bell(&u8g2, 25, 12);
        u8g2_DrawStr(&u8g2, 52, 18, "Humidity");         
        draw_icon_humidity(40, 40);
                
        // humidity value 
        EX_OLED_SetFont(&u8g2, FONT_MONOCHROME_LARGE);
        u8g2_DrawStr(&u8g2, 50, 40, h115.alarm_humi);  
        u8g2_DrawStr(&u8g2, 80, 40, (char *)"%");        
    }
    
    u8g2_SendBuffer(&u8g2);
}


void render_loading(void)
{
    int y = 1;
    u8g2_ClearDisplay(&u8g2);

    EX_OLED_SetFont(&u8g2, FONT_MONOCHROME_LARGE);
    DrawCenteredString(&u8g2, y, "ABOV");
    y += u8g2_GetMaxCharHeight(&u8g2) + 2;

    EX_OLED_SetFont(&u8g2, FONT_MONOCHROME_NORMAL);
    DrawCenteredString(&u8g2, y, "Semiconductor");
    y += u8g2_GetMaxCharHeight(&u8g2) + 2;

    EX_OLED_SetFont(&u8g2, FONT_MONOCHROME_MEDIUM);
    DrawCenteredString(&u8g2, y, "A31C144 E/V BD");
    y += u8g2_GetMaxCharHeight(&u8g2) + 2;

    for (int i = 0; i <= 100; i += 10)
    {
        DrawProgressBar(&u8g2, 0, y, SCREEN_WIDTH - 2, 10, i);
        SystemDelayMS(90);
        u8g2_SendBuffer(&u8g2);
    }
    ui.depth = DEPTH_HOME_SCREEN;
    u8g2_ClearDisplay(&u8g2);
}

void render(void)
{
    if (ui.depth == DEPTH_LOADING_SCREEN)
        return;

    if (ui.depth == DEPTH_HOME_SCREEN)
    {
        render_home();
    }
    else if (ui.depth == DEPTH_LV1)
    {
        render_lv1();
    }
    else if (ui.depth == DEPTH_LV2)
    {
        render_lv2();
    }
    else if (ui.depth == DEPTH_ALARM)
    {
        render_alarm();
    }
}

static void on_key(VKey k)
{
    if (k == KEY_OK)
    {
        EX_BUZZER_Start(BUZZER_BEEP_TWICE);
    }
    else
    {
        EX_BUZZER_Start(BUZZER_BEEP_ONCE);
    }
    
    if (ui.depth == DEPTH_HOME_SCREEN)
    {
        switch (k) 
        {
            case KEY_OK:
                ui.depth = DEPTH_LV1;
                break;

            default:
                return;
        }
    }
    else if (ui.depth == DEPTH_LV1)
    {
        switch (k) 
        {
            case KEY_UP:
                if (ui.focus_lv1 > 0)
                    ui.focus_lv1--;
                else
                    ui.focus_lv1 = MENU_ITEMS_COUNT - 1;
                break;

            case KEY_DOWN:
                if (ui.focus_lv1 < MENU_ITEMS_COUNT - 1)
                    ui.focus_lv1++;
                else
                    ui.focus_lv1 = 0;
                break;

            case KEY_LEFT:
                ui.depth = DEPTH_HOME_SCREEN;
                break;

            case KEY_RIGHT:
            case KEY_OK:
                ui.depth = DEPTH_LV2;
                ui.focus_lv2 = 0;
            
                if(ui.focus_lv1 == 0)  // "1. Measure Period"
                {
                     ui.focus_lv2 = h115.raed_set_focus;
                }
                else if(ui.focus_lv1 == 1) // "2. Temp Alarm"
                {
                    ui.focus_lv2 = h115.alarm_temp_focus;
                }
                else if(ui.focus_lv1 == 2) // "3. Humi Alarm"
                {
                    ui.focus_lv2 = h115.alarm_humi_focus;
                }
                else if((ui.focus_lv1 == 3) || (ui.focus_lv1 == 4))  // "4. Date Setting", "5. Time Setting"
                {      
                    if(s_TimeEditing == 0)
                    {
                        s_TimeEditing = 1;
                    }
                }
                else if(ui.focus_lv1 == 5)  // "6. Volume Setting"
                {   
                    g_BuzzerVolumeMode = 1;
                }
                else if(ui.focus_lv1 == 8) // "9. Back"
                {
                    ui.depth = DEPTH_HOME_SCREEN;
                    ui.focus_lv1 = 0;
                }
                break;

            default:
                break;
        }
    } 
    else if (ui.depth == DEPTH_LV2)
    {
        const MenuItem *m = &menu_lv1[ui.focus_lv1];

        switch (k)
        {
            case KEY_LEFT:
                ui.depth = DEPTH_LV1;
                break;
            
            case KEY_UP:                
                if (strcmp(m->label, "4. Date Setting") == 0)
                {               
                    if (ui.focus_year)
                    {
                        stCurTime.tm_year++;
                    }
                    else if (ui.focus_mon)
                    {    
                        stCurTime.tm_mon++;
                        if(stCurTime.tm_mon > 11)
                        {
                            stCurTime.tm_mon = 0;
                        }
                    }
                    else if (ui.focus_mday)
                    {    
                        stCurTime.tm_mday++;
                        if(stCurTime.tm_mday > 31)
                        {
                            stCurTime.tm_mday = 1;
                        }
                    }  
                }
                else if (strcmp(m->label, "5. Time Setting") == 0)
                {      
                    if (ui.focus_hour)
                    {
                        stCurTime.tm_hour++;
                        if(stCurTime.tm_hour > 23)
                        {
                            stCurTime.tm_hour = 0;
                        }
                    }
                    else if (ui.focus_min)
                    {    
                        stCurTime.tm_min++;
                        if(stCurTime.tm_min > 59)
                        {
                            stCurTime.tm_min = 0;
                        }
                    }                    
                }       
                else
                {
                    if (ui.focus_lv2 > 0)
                    {
                        ui.focus_lv2--;
                    }
                    else
                    {    
                        ui.focus_lv2 = m->submenu_count - 1;
                    }
                }                    
                break;

            case KEY_DOWN:
                if (strcmp(m->label, "4. Date Setting") == 0)
                {
                    if (ui.focus_year)
                    {
                        stCurTime.tm_year--;
                    }
                    else if (ui.focus_mon)
                    {    
                        stCurTime.tm_mon--;
                        if(stCurTime.tm_mon < 0)
                        {
                            stCurTime.tm_mon = 11;
                        }
                    }
                    else if (ui.focus_mday)
                    {    
                        stCurTime.tm_mday--;
                        if(stCurTime.tm_mday < 1)
                        {
                            stCurTime.tm_mday = 31;
                        }
                    }
                }
                else if (strcmp(m->label, "5. Time Setting") == 0)
                {                    
                    if (ui.focus_hour)
                    {
                        stCurTime.tm_hour--;
                        if(stCurTime.tm_hour < 0)
                        {
                            stCurTime.tm_hour = 23;
                        }
                    }
                    else if (ui.focus_min)
                    {    
                        stCurTime.tm_min--;
                        if(stCurTime.tm_min < 0)
                        {
                            stCurTime.tm_min = 59;
                        }
                    }
                }       
                else
                {
                    if (ui.focus_lv2 < m->submenu_count - 1)
                    {
                        ui.focus_lv2++;
                    }
                    else
                    {
                        ui.focus_lv2 = 0;
                    }     
                }                    
                break;

            case KEY_RIGHT:
            case KEY_OK:
            {
                const char *sel = m->submenu[ui.focus_lv2];
                // exit if back
                if (strcmp(sel, "Back") == 0)
                {
                    ui.depth = DEPTH_LV1;
                    ui.focus_lv2 = 0;
                }
                else
                {
                    // action !!!
                    if (strcmp(m->label, "1. Measure Peroid") == 0)
                    {
                        h115.raed_set_focus = 0;
                        
                        if(strcmp(sel, "1. 1s") == 0)
                        {
                            h115.read_set_time = 1;
                            h115.raed_set_focus = ui.focus_lv2;
                        }
                        else if(strcmp(sel, "2. 10s") == 0)
                        {
                            h115.read_set_time = 10;
                            h115.raed_set_focus = ui.focus_lv2;
                        }
                        else if(strcmp(sel, "3. 30s") == 0)
                        {
                            h115.read_set_time = 30;
                            h115.raed_set_focus = ui.focus_lv2;
                        }
                        s_snorSaveFlag = 1;
                        
                        ui.depth = DEPTH_LV1;                         
                    }
                    else if (strcmp(m->label, "2. Temp Alarm") == 0)
                    {
                        // set temp alaram
                        snprintf(h115.alarm_temp, sizeof(h115.alarm_temp), "%s", sel);
                        h115.alarm_temp_focus = ui.focus_lv2;
                        
                        s_snorSaveFlag = 1;
                        
                        ui.depth = DEPTH_LV1; 
                    }
                    else if (strcmp(m->label, "3. Humi Alarm") == 0)
                    {
                        // set humi alaram
                        snprintf(h115.alarm_humi, sizeof(h115.alarm_humi), "%s", sel);
                        h115.alarm_humi_focus = ui.focus_lv2;
                        
                        s_snorSaveFlag = 1;
                        
                        ui.depth = DEPTH_LV1; 
                    }
                    else if (strcmp(m->label, "4. Date Setting") == 0)
                    {
                        if (ui.focus_year)
                        {
                            ui.focus_year = 0;
                            ui.focus_mon = 1;
                        }
                        else if (ui.focus_mon)
                        {    
                            ui.focus_mon = 0;
                            ui.focus_mday = 1;
                        }
                        else if (ui.focus_mday)
                        {    
                            ui.focus_year = 1;
                            ui.focus_mday = 0;
                            
                            ui.depth = DEPTH_LV1;
                            
                            time_t new_sec = mktime(&stCurTime);
                            if (new_sec != (time_t)-1) 
                            {
                                s_tmUnixTime = new_sec;
                            }
                            
                            s_snorSaveFlag = 1;                            
                            s_TimeEditing = 0;
                        }                        
                    }  
                    else if (strcmp(m->label, "5. Time Setting") == 0)
                    {               
                        if (ui.focus_hour)
                        {
                            ui.focus_hour = 0;
                            ui.focus_min = 1;
                        }
                        else if (ui.focus_min)
                        {  
                            ui.focus_hour = 1;                             
                            ui.focus_min = 0;
                            
                            ui.depth = DEPTH_LV1;
                            
                            time_t new_sec = mktime(&stCurTime);
                            if (new_sec != (time_t)-1) 
                            {
                                s_tmUnixTime = new_sec;
                            }
                            
                            s_snorSaveFlag = 1; 
                            s_TimeEditing = 0;
                        }
                    }
                    else if (strcmp(m->label, "6. Volume Setting") == 0)
                    {
                        g_BuzzerVolumeMode = 0;
                        ui.depth = DEPTH_LV1;
                    }
                    else if(strcmp(m->label, "7. LED Bright") == 0)
                    {
                        LEDDRV_DIMMING_e dimming = (LEDDRV_DIMMING_e)ui.focus_lv2;
                        EX_LEDDRV_SetDimming(dimming);
                        saveData.led_bright = ui.focus_lv2;
                        ui.depth = DEPTH_LV1;
                        s_snorSaveFlag = 1;
                    }
                    else 
                    {
                        ui.depth = DEPTH_LV1;
                    }                        
                }
                break;
            }
            default:
                break;
        }
    }
    else if (ui.depth == DEPTH_ALARM)
    {
        switch (k)
        {
            case KEY_RIGHT:
            case KEY_OK:
            {
                 if(h115.alarm_temp_flag)
                {            
                    h115.alarm_temp_flag = 0;
                    h115.alarm_temp_focus = 0;
                    snprintf(h115.alarm_temp, sizeof(h115.alarm_temp), "OFF");                
                }
                if(h115.alarm_humi_flag)
                {
                    h115.alarm_humi_flag = 0;
                    h115.alarm_humi_focus = 0;
                    snprintf(h115.alarm_humi, sizeof(h115.alarm_humi), "OFF");
                    
                } 
                s_snorSaveFlag = 1; 
                
                ui.depth = DEPTH_HOME_SCREEN;                       
                break;
            }
            default:
                break;
        }
    }
    
    render();
}

void Init_LED_Brightness()
{
    if ((LEDDRV_DIMMING_e)saveData.led_bright < LEDDRV_DIMMING_MAX)
    {
        EX_LEDDRV_SetDimming((LEDDRV_DIMMING_e)saveData.led_bright);
    }
		else
    {
        EX_LEDDRV_SetDimming(LEDDRV_DIMMING_0);
    }
}

void Init_time()
{
    //struct tm stCurTime = {0};
    stCurTime = *localtime(&s_tmUnixTime);
    snprintf((char *)CLOCK_HMS, sizeof(CLOCK_HMS), "%02d:%02d:%02d", stCurTime.tm_hour, stCurTime.tm_min, stCurTime.tm_sec);
    snprintf((char *)CLOCK_AMPM, sizeof(CLOCK_AMPM), "%s", (stCurTime.tm_hour >= 12) ? "PM" : "AM");
    snprintf((char *)CLOCK_MONTHDAY, sizeof(CLOCK_MONTHDAY), "%2d/%02d", stCurTime.tm_mon + 1, stCurTime.tm_mday);
}

static void HWTIMER_SEC_IRQHandler(uint32_t un32Event, void *pContext)
{
    //struct tm stCurTime = {0};

    s_tmUnixTime++;
    if(h115.read_interval > 0) h115.read_interval--; 
    
    if(!s_TimeEditing)
    {
        stCurTime = *localtime(&s_tmUnixTime);
        snprintf((char *)CLOCK_HMS, sizeof(CLOCK_HMS), "%02d:%02d:%02d", stCurTime.tm_hour, stCurTime.tm_min, stCurTime.tm_sec);
        snprintf((char *)CLOCK_AMPM, sizeof(CLOCK_AMPM), "%s", (stCurTime.tm_hour >= 12) ? "PM" : "AM");
        snprintf((char *)CLOCK_MONTHDAY, sizeof(CLOCK_MONTHDAY), "%2d/%02d", stCurTime.tm_mon + 1, stCurTime.tm_mday);
    }
    
    //LOG("%s\n", CLOCK_HMS);
}

void Init_register(void)
{
   // SNOR Read 
    load_time_snor();

    if(h115.alarm_temp_focus)
    {
        const char *sel = menu_lv1[1].submenu[h115.alarm_temp_focus];
        snprintf(h115.alarm_temp, sizeof(h115.alarm_temp), "%s", sel);
    }
    else
    {
        snprintf(h115.alarm_temp, sizeof(h115.alarm_temp), "OFF");
    }
    
    if(h115.alarm_humi_focus)
    {
        const char *sel = menu_lv1[2].submenu[h115.alarm_humi_focus];
        snprintf(h115.alarm_humi, sizeof(h115.alarm_humi), "%s", sel);
    }
    else
    {
        snprintf(h115.alarm_humi, sizeof(h115.alarm_humi), "OFF");
    }
    
    h115.alarm_temp_flag = 0;
    h115.alarm_humi_flag = 0;
    
    ui.focus_year = 1;
    ui.focus_hour = 1;
    
    s_TimeEditing = 0;
}

static void read_h115_sensor(void)
{    
    uint16_t hc, tc;    
    
    // read a h115
    EX_TCS_I2C_Single_Measure(&hc, &tc);
    
    // humidity, temperature value calculation 
    h115.humi = (float)hc * 100 / 1024;     
    h115.temp = ((float)tc - 774) * (float)0.1; 

    if(h115.humi > 100)
    {
        h115.humi = 100;
    }
    else if(h115.humi < 0)
    {
        h115.humi = 0;
    }
    
    if(h115.temp > 100)
    {
        h115.temp = 100;
    }
    else if(h115.temp < -30)
    {
        h115.temp = -30;
    }
    
    snprintf(h115.str_humi, sizeof(h115.str_humi), "%.1f%%", h115.humi);
    snprintf(h115.str_temp, sizeof(h115.str_temp), "%.1f", h115.temp);
    
    if ((strcmp(h115.alarm_temp, "OFF") != 0) && (!h115.alarm_temp_flag) && (!h115.alarm_humi_flag))
    {   
        int temp_compare = atoi(h115.alarm_temp);
        float temp_tolerance = 0.5f;  // 0.5'C
        
        if(fabs(h115.temp - temp_compare) <= temp_tolerance)
        {
            h115.alarm_temp_flag = 1;              
        }
    }
    if ((strcmp(h115.alarm_humi, "OFF") != 0) && (!h115.alarm_humi_flag) && (!h115.alarm_temp_flag))
    {   
        int humi_compare = atoi(h115.alarm_humi);
        float humi_tolerance = 1.0f;  // 1%
        
        if(fabs(h115.humi - humi_compare) <= humi_tolerance)
        {
            h115.alarm_humi_flag = 1;             
        }
    }   
}

void menu_run(void)
{
    // A96H115 I2C Read 
    if((h115.read_interval == 0) && (!h115.alarm_temp_flag) && (!h115.alarm_humi_flag))
    {
        read_h115_sensor();    
        render_temp_humi();        
        h115.read_interval = h115.read_set_time; 

        if(h115.alarm_temp_flag || h115.alarm_humi_flag)
        {
            u8g2_ClearDisplay(&u8g2);       
        }            
    }   
    
    // Temp & Humidity Alarm 
    if(h115.alarm_temp_flag)
    {
        ui.depth = DEPTH_ALARM;  
        render(); 
    }
    else if(h115.alarm_humi_flag)
    {
        ui.depth = DEPTH_ALARM;  
        render(); 
    }
    
    // Change Buzzer volume 
    if(g_BuzzerVolumeMode)
    {
        if(s_preBuzzerLevel != g_BuzzerLevel)
        {
            render(); 
            EX_BUZZER_Start(BUZZER_BEEP_ONCE);
            s_preBuzzerLevel = g_BuzzerLevel;
        }
    }
    
    if(s_snorSaveFlag)
    {
        save_time_snor();
        s_snorSaveFlag = 0;
    }
    
    render_time();
    
}
void menu_init(void)
{
    EX_OLED_Init(&u8g2);
    Init_time();
    Init_register();
    Init_LED_Brightness();
    EX_BUZZER_SetVolume(g_BuzzerLevel);
    EX_HW_TIMER_6_Start(HWTIMER_SEC_IRQHandler, TIMER1_MODE_PERIODIC, 1000);
    // todo, need to change irq handler management.
    EX_BUTTON_Init(on_key);
    IR_RegisterKeyCallback(on_key);

    render_loading();
    read_h115_sensor();
    render();
}
