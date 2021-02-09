#ifndef GIF_BUTTON_BSP_H_
#define GIF_BUTTON_BSP_H_

#include "stm32f4xx_hal.h"

#define LCD_DC_PIN				GPIO_PIN_3
#define LCD_DC_GPIO_PORT		GPIOA
#define LCD_RES_PIN				GPIO_PIN_2
#define LCD_RES_GPIO_PORT		GPIOA

#define LCD_DC_LOW()			HAL_GPIO_WritePin(LCD_DC_GPIO_PORT, LCD_DC_PIN, GPIO_PIN_RESET)
#define LCD_DC_HIGH()			HAL_GPIO_WritePin(LCD_DC_GPIO_PORT, LCD_DC_PIN, GPIO_PIN_SET)
#define LCD_RES_LOW()			HAL_GPIO_WritePin(LCD_RES_GPIO_PORT, LCD_RES_PIN, GPIO_PIN_RESET)
#define LCD_RES_HIGH()			HAL_GPIO_WritePin(LCD_RES_GPIO_PORT, LCD_RES_PIN, GPIO_PIN_SET)

void LCD_hw_reset(void);
void LCD_IO_WriteReg(uint8_t LCDReg);
void LCD_IO_WriteData(uint8_t Data);
void LCD_IO_WriteMultipleData(uint8_t *pData, uint32_t Size);

void lis2dh_init(void);

#endif /* GIF_BUTTON_BSP_H_ */
