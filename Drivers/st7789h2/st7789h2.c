#include "st7789h2.h"
#include "stm32f4xx_hal.h"
#include "gif_button_bsp.h"


/**
  * @brief  Writes to the selected LCD register.
  * @param  Command: command value (or register address as named in st7789h2 doc).
  * @param  Parameters: pointer on parameters value (if command uses one or several parameters).
  * @param  NbParameters: number of command parameters (0 if no parameter)
  * @retval None
  */
void ST7789H2_WriteReg(uint8_t Command, uint8_t *Parameters, uint8_t NbParameters)
{
  uint8_t   i;

  /* Send command */
  LCD_IO_WriteReg(Command);

  /* Send command's parameters if any */
  for (i=0; i<NbParameters; i++)
  {
    LCD_IO_WriteData(Parameters[i]);
  }
}

void ST7789H2_Init(void)
{
  uint8_t   parameter[2];
  
  LCD_hw_reset();

//  /* SW Reset Command */
//  ST7789H2_WriteReg(0x01, (uint8_t*)NULL, 0);
//  /* Wait for 200ms */
//  HAL_Delay(150);

  /* Sleep Out Command */
  ST7789H2_WriteReg(ST7789H2_SLEEP_OUT, (uint8_t*)NULL, 0);
  /* Wait for 5ms before next command*/
  HAL_Delay(5);

  /* Color mode 16bits/pixel */
//  parameter[0] = 0x55;
  /* Color mode 18bits/pixel */
  parameter[0] = 0x66;
  ST7789H2_WriteReg(ST7789H2_COLOR_MODE, parameter, 1);

  /* Set the orientation */
  parameter[0] = 0xC0;
  ST7789H2_WriteReg(ST7789H2_MADCTL, parameter, 1);

  /* invert colors*/
  ST7789H2_WriteReg(ST7789H2_DISPLAY_INVERSION, (uint8_t*)NULL, 0);

  /* Normal ON (no-partial) command */
  ST7789H2_WriteReg(ST7789H2_NORMAL_DISPLAY_ON, (uint8_t*)NULL, 0);

  /* Display ON command */
  ST7789H2_WriteReg(ST7789H2_DISPLAY_ON, (uint8_t*)NULL, 0);

//  ST7789H2_FillScreenRGB666(WHITE);
}

/**
  * @brief  Set Cursor position.
  * @param  Xpos: specifies the X position.
  * @param  Ypos: specifies the Y position.
  * @retval None
  */
void ST7789H2_SetCursor(uint16_t Xpos, uint16_t Ypos)
{
  uint8_t   parameter[4];
  /* CASET: Comumn Addrses Set */
  parameter[0] = 0x00;
  parameter[1] = 0x00 + Xpos;
  parameter[2] = 0x00;
  parameter[3] = 0xEF + Xpos;
  ST7789H2_WriteReg(ST7789H2_CASET, parameter, 4);
  /* RASET: Row Addrses Set */
  // Added 0x50 offset to RASET since MADCTL was MY=1 to complete the 180deg rotation for gif_button
  parameter[0] = ((0x50 + Ypos) >> 8);
  parameter[1] = 0x50 + Ypos;
  parameter[2] = 0x01;
  parameter[3] = 0x3F + Ypos;
  ST7789H2_WriteReg(ST7789H2_RASET, parameter, 4);
}

/**
  * @brief  Write pixel.   
  * @param  Xpos: specifies the X position.
  * @param  Ypos: specifies the Y position.
  * @param  RGBCode: the RGB pixel color in RGB565 format
  * @retval None
  */
void ST7789H2_WritePixel(uint16_t Xpos, uint16_t Ypos, uint32_t RGBCode)
{
  /* Set Cursor */
  ST7789H2_SetCursor(Xpos, Ypos);

  /* Prepare to write to LCD RAM */
  ST7789H2_WriteReg(ST7789H2_WRITE_RAM, (uint8_t*)NULL, 0);   /* RAM write data command */

  /* Write RAM data */

  LCD_IO_WriteMultipleData((uint8_t*)&RGBCode, 3); // 3 for RGB666
}


/*
 * Fill screen with RGB666 data
 * RRRRRRXX GGGGGGXX BBBBBBXX = 3 bytes
 */
void ST7789H2_FillScreenRGB666(uint32_t RGBCode)
{
  /* Set Cursor */
  ST7789H2_SetCursor(0, 0);
  
  /* Prepare to write to LCD RAM */
  ST7789H2_WriteReg(ST7789H2_WRITE_RAM, (uint8_t*)NULL, 0);   /* RAM write data command */
  
  for(uint32_t idx = 0; idx < 240*240*3; idx++)
  {
	    LCD_IO_WriteMultipleData((uint8_t*)&RGBCode, 3);
  }
}

/*
 * Fill screen with RGB565 data
 * RRRRRGGG GGGBBBBB = 2 bytes
 */
void ST7789H2_FillScreenRGB565(uint16_t RGBCode)
{
  /* Set Cursor */
  ST7789H2_SetCursor(0, 0);

  /* Prepare to write to LCD RAM */
  ST7789H2_WriteReg(ST7789H2_WRITE_RAM, (uint8_t*)NULL, 0);   /* RAM write data command */

  for(uint32_t idx = 0; idx < 240*240*2; idx++)
  {
	    LCD_IO_WriteMultipleData((uint8_t*)&RGBCode, 2);
  }
}


void ST7789H2_DrawDMAFastFullScreen(uint8_t * data)
{
    ST7789H2_SetCursor(0, 0);
    ST7789H2_WriteReg(ST7789H2_WRITE_RAM, (uint8_t*)NULL, 0);   /* RAM write data command */
    LCD_IO_WriteMultipleData(data, 240*120*2); //rgb565 data (hence 2 bytes per pixel)

    // breaking the transfer to 2 parts since the spi driver takes up to 16bit size argument
//    ST7789H2_SetCursor(0, 120);
//    ST7789H2_WriteReg(ST7789H2_WRITE_RAM, (uint8_t*)NULL, 0);   /* RAM write data command */
    LCD_IO_WriteMultipleData(data+240*120*2, 240*120*2);
}

static void ST7789H2_DrawChar(uint16_t Xpos, uint16_t Ypos, const uint8_t *c, sFONT font, uint32_t textcolor, uint32_t bgcolor)
{
	uint32_t i = 0, j = 0;
	uint16_t height, width;
	uint8_t  offset;
	uint8_t  *pchar;
	uint32_t line;

	height = font.Height;
	width  = font.Width;

	offset =  8 *((width + 7)/8) -  width ;

	for(i = 0; i < height; i++)
	{
		pchar = ((uint8_t *)c + (width + 7)/8 * i);

		switch(((width + 7)/8))
		{

		case 1:
			line =  pchar[0];
			break;

		case 2:
			line =  (pchar[0]<< 8) | pchar[1];
			break;

		case 3:
		default:
			line =  (pchar[0]<< 16) | (pchar[1]<< 8) | pchar[2];
			break;
		}

		for (j = 0; j < width; j++)
		{
			if(line & (1 << (width- j + offset- 1)))
			{
				ST7789H2_WritePixel((Xpos + j), Ypos, textcolor);
			}
			else
			{
				ST7789H2_WritePixel((Xpos + j), Ypos, bgcolor);
			}
		}
		Ypos++;
	}
}


/**
 * @brief Write a string
 * @param  x&y -> cursor of the start point.
 * @param str -> string to write
 * @param font -> fontstyle of the string
 * @param color -> color of the string
 * @param bgcolor -> background color of the string
 * @return  none
 */
void ST7789H2_WriteString(uint16_t x, uint16_t y, uint8_t *str, sFONT font, uint32_t textcolor, uint32_t bgcolor)
{
	while (*str)
	{
		if (x + font.Width >= 240)
		{
			x = 0;
			y += font.Height;
			if (y + font.Height >= 240)
				break;
		}
		ST7789H2_DrawChar(x, y, &font.table[(*str-' ') * font.Height * ((font.Width + 7) / 8)], font, textcolor, bgcolor);
		x += font.Width;
		str++;
	}
}
