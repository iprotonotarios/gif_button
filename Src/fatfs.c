/**
  ******************************************************************************
  * @file   fatfs.c
  * @brief  Code for fatfs applications
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

#include "fatfs.h"

uint8_t retSD;    /* Return value for SD */
char SDPath[4];   /* SD logical drive path */
FATFS SDFatFS;    /* File system object for SD logical drive */
FIL SDFile;       /* File object for SD */

/* USER CODE BEGIN Variables */
#include "bsp_driver_sd.h"
#include "jpeglib.h"
#include "st7789h2.h"

FRESULT ff_result;
FILINFO fno;
DIR dir;
FIL jpg0, jpg1;
/* USER CODE END Variables */

void MX_FATFS_Init(void)
{
  /*## FatFS: Link the SD driver ###########################*/
  retSD = FATFS_LinkDriver(&SD_Driver, SDPath);

  /* USER CODE BEGIN Init */
  /* additional user code for init */
  BSP_SD_ITConfig();
  fatfs_read_SD_contents();
  /* USER CODE END Init */
}

/**
  * @brief  Gets Time from RTC
  * @param  None
  * @retval Time in DWORD
  */
DWORD get_fattime(void)
{
  /* USER CODE BEGIN get_fattime */
  return 0;
  /* USER CODE END get_fattime */
}

/* USER CODE BEGIN Application */

// Handle sd_card removal
void fatfs_unmount_SD(void)
{
	ff_result = f_mount(0, (const TCHAR*)SDPath, 0);
	if (ff_result)
	{
		log_segger("Un-mounting SD failed with: %x!\n", ff_result);
	}
}

// Initialize sd card, mount storage and load image files
// This is called during init, and every time usb is unplugged in case the files were updated
void fatfs_read_SD_contents(void)
{
	if (BSP_PlatformIsDetected() == SD_NOT_PRESENT)
	{
		log_segger("SD_NOT_PRESENT!\n");
		ST7789H2_WriteString(10, 108, (uint8_t *)"Please insert a uSD card!", Font24, BLACK, WHITE);
		return;
	}

	ff_result = f_mount(&SDFatFS, (const TCHAR*)SDPath, 1);
	if (ff_result)
	{
		log_segger("Mounting SD failed!\n");
	}

	log_segger("\r\n Listing directory: \n");
	ff_result = f_opendir(&dir, (const TCHAR*)"/");
	if (ff_result)
	{
		log_segger("Directory listing failed!");
	}

	do
	{
		ff_result = f_readdir(&dir, &fno);
		if (ff_result != FR_OK)
		{
			log_segger("Directory read failed: %d", ff_result);
			break;
		}

		if (fno.fname[0])
		{
			if (fno.fattrib & AM_DIR)
			{
				log_segger("   <DIR>   %s\n", fno.fname);
			}
			else
			{
				log_segger("%9lu  %s\n", fno.fsize, fno.fname);
			}
		}
	}
	while (fno.fname[0]);

	// open files
	ff_result = f_open(&jpg0, (const TCHAR*)"/0.jpg", FA_READ);
	if (ff_result)
	{
		log_segger("Error opening \"0.jpg\": %d!\n", ff_result);
		ST7789H2_WriteString(10, 108, (uint8_t *)"File '0.jpg' not found!", Font24, BLACK, WHITE);
	}
	ff_result = f_open(&jpg1, (const TCHAR*)"/1.jpg", FA_READ);
	if (ff_result)
	{
		log_segger("Error opening \"1.jpg\": %d!\n", ff_result);
		ST7789H2_WriteString(10, 108, (uint8_t *)"File '1.jpg' not found!", Font24, BLACK, WHITE);
	}
}

/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
