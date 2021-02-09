/* USER CODE BEGIN Header */
/**
 ******************************************************************************
  * File Name          : libjpeg.c
  * Description        : This file provides code for the configuration
  *                      of the libjpeg instances.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "libjpeg.h"

/* USER CODE BEGIN 0 */
#include "main.h"
#include "fatfs.h"
#include "st7789h2.h"
#include "gif_button_bsp.h"
/* USER CODE END 0 */

/* USER CODE BEGIN 1 */
/* USER CODE END 1 */

/* Global variables ---------------------------------------------------------*/

/* USER CODE BEGIN 2 */
/* USER CODE END 2 */

/* LIBJPEG init function */
void MX_LIBJPEG_Init(void)
{
/***************************************/
   /**
  */

  /* USER CODE BEGIN 3 */
  /* USER CODE END 3 */

}

/* USER CODE BEGIN 4 */
#define MAX_LINES	(100) // attempt to decode/send max_lines to lcd at once for speedup
uint8_t buff[240*3*MAX_LINES];

void decode_and_display_jpg(FIL image)
{
//	log_segger("image: %x \n", image.flag); // TODO why was this? for checking if sd is present but no filenames? usb con? recheck
	// check if sd is present
	if (BSP_SD_GetCardState() != SD_TRANSFER_OK)
	{
		ST7789H2_WriteString(10, 108, (uint8_t *)"Please insert a uSD card!", Font24, BLACK, WHITE);
		return;
	}

	// Prepare lcd
	ST7789H2_SetCursor(0, 0);
	ST7789H2_WriteReg(ST7789H2_WRITE_RAM, (uint8_t*)NULL, 0);   /* RAM write data command */

	uint8_t width = 240;
	JSAMPROW buffer[2] = {0}; /* Output row buffer */
	uint32_t row_stride = 0; /* physical row width in image buffer */

	buffer[0] = buff;

	// Decode jpegs - or decode every time? memory space vs time
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, &image);
	jpeg_read_header(&cinfo, TRUE);
	// TODO check if dimensions are ok? scale to 240*240?
	//cinfo.scale_num=1;
	//cinfo.scale_denom=2;
	cinfo.dct_method = JDCT_FLOAT;
	jpeg_start_decompress(&cinfo);

	row_stride = width * 3;
	while (cinfo.output_scanline < cinfo.output_height)
	{
		JDIMENSION scaned_lines = jpeg_read_scanlines(&cinfo, buffer, MAX_LINES);
		// send to lcd
		LCD_IO_WriteMultipleData(buffer[0], row_stride*scaned_lines);
	}

	/* Step 6: Finish decompression */
	jpeg_finish_decompress(&cinfo);

	/* Step 7: Release JPEG decompression object */
	jpeg_destroy_decompress(&cinfo);
}
/* USER CODE END 4 */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
