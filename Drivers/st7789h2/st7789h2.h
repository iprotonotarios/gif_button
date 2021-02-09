#ifndef __ST7789H2_H
#define __ST7789H2_H

#include <stdio.h>
#include "fonts.h"

#define  ST7789H2_ID    0x85
   
#define ST7789H2_LCD_ID             0x04   
#define ST7789H2_SLEEP_IN           0x10
#define ST7789H2_SLEEP_OUT          0x11
#define ST7789H2_PARTIAL_DISPLAY    0x12
#define ST7789H2_NORMAL_DISPLAY_ON	0x13
#define ST7789H2_DISPLAY_INVERSION  0x21
#define ST7789H2_DISPLAY_ON         0x29
#define ST7789H2_WRITE_RAM          0x2C
#define ST7789H2_READ_RAM           0x2E
#define ST7789H2_CASET              0x2A
#define ST7789H2_RASET              0x2B
#define ST7789H2_VSCRDEF            0x33 /* Vertical Scroll Definition */
#define ST7789H2_VSCSAD             0x37 /* Vertical Scroll Start Address of RAM */   
#define ST7789H2_TEARING_EFFECT     0x35
#define ST7789H2_MADCTL     		0x36
#define ST7789H2_IDLE_MODE_OFF      0x38
#define ST7789H2_IDLE_MODE_ON       0x39
#define ST7789H2_COLOR_MODE         0x3A
#define ST7789H2_PORCH_CTRL         0xB2
#define ST7789H2_GATE_CTRL          0xB7
#define ST7789H2_VCOM_SET           0xBB
#define ST7789H2_DISPLAY_OFF        0xBD    
#define ST7789H2_LCM_CTRL           0xC0
#define ST7789H2_VDV_VRH_EN         0xC2
#define ST7789H2_VDV_SET            0xC4    
#define ST7789H2_VCOMH_OFFSET_SET   0xC5
#define ST7789H2_FR_CTRL            0xC6
#define ST7789H2_POWER_CTRL         0xD0
#define ST7789H2_PV_GAMMA_CTRL      0xE0
#define ST7789H2_NV_GAMMA_CTRL      0xE1 

#define BLACK	0x00000000
#define WHITE	0x00FFFFFF
  
void ST7789H2_Init(void);
void ST7789H2_SetCursor(uint16_t Xpos, uint16_t Ypos);
void ST7789H2_WriteReg(uint8_t Command, uint8_t *Parameters, uint8_t NbParameters);
void ST7789H2_FillScreenRGB666(uint32_t RGBCode);
void ST7789H2_DrawDMAFastFullScreen(uint8_t * data);

void ST7789H2_WriteString(uint16_t x, uint16_t y, uint8_t *str, sFONT font, uint32_t textcolor, uint32_t bgcolor);

#endif /* __ST7789H2_H */
