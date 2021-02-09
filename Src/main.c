/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "fatfs.h"
#include "i2c.h"
#include "libjpeg.h"
#include "sdio.h"
#include "spi.h"
#include "tim.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdarg.h>
#include <stdbool.h>
#include "st7789h2.h"
#include "string.h"
#include "SEGGER_RTT.h"
#include "gif_button_bsp.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
bool go_to_sleep_flag = false;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void DisableGpios()
{
   GPIO_InitTypeDef GPIO_InitStructure = {0};
   GPIO_InitStructure.Pin = GPIO_PIN_All;
   GPIO_InitStructure.Mode = GPIO_MODE_ANALOG;
   GPIO_InitStructure.Pull = GPIO_NOPULL;

   // TODO check this since it is already probably in analog state
//   HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);
//   __HAL_RCC_GPIOC_CLK_DISABLE();

   // RCC HSE pins leak ~100uA
   HAL_GPIO_Init(GPIOH, &GPIO_InitStructure);
   __HAL_RCC_GPIOH_CLK_DISABLE();
}

void go_to_sleep(void)
{
	go_to_sleep_flag = false;

	// hold pins low due to leakage (~60uA)
	LCD_RES_LOW();
	LCD_DC_LOW();

	// set all pins that leak to analog mode
	HAL_SD_DeInit(&hsd);
	HAL_SPI_MspDeInit(&hspi1);
//	HAL_I2C_MspDeInit(&hi2c1);
	DisableGpios();

	USB_PowerDown();

	/* turn display, sd card power supply OFF */
	HAL_GPIO_WritePin(SW_3V3_CTL_GPIO_Port, SW_3V3_CTL_Pin, GPIO_PIN_RESET);

	HAL_GPIO_WritePin(D_BCKL_GPIO_Port, D_BCKL_Pin, GPIO_PIN_RESET);

	HAL_SuspendTick();

	CLEAR_BIT(ADC1->CR2, ADC_CR2_ADON);

	HAL_PWREx_EnableFlashPowerDown();
	HAL_PWREx_EnableLowRegulatorLowVoltage();

	// TODO? disable interrupt except the one you want to wake you up (button?) or set up an event
	HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_SLEEPENTRY_WFI);

	HAL_PWREx_DisableFlashPowerDown();
	HAL_PWREx_DisableLowRegulatorLowVoltage();

	SystemClock_Config();
	HAL_ResumeTick();

	/* turn display, sd_card supply ON */
	HAL_GPIO_WritePin(SW_3V3_CTL_GPIO_Port, SW_3V3_CTL_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(D_BCKL_GPIO_Port, D_BCKL_Pin, GPIO_PIN_SET);

	USB_PowerUp();
	HAL_SPI_MspInit(&hspi1);
	ST7789H2_Init();

	//	HAL_SD_MspInit(&hsd);
	BSP_SD_Init();
	fatfs_read_SD_contents();
	decode_and_display_jpg(jpg0);
}

/**
  * @brief  User button callback
  * @param  GPIO_Pin Specifies the pins connected EXTI line
  * @retval None
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if(GPIO_Pin == GPIO_PIN_9)
	{
		log_segger("LIS2 int 1\n");

//		__HAL_TIM_CLEAR_IT(&htim5, TIM_IT_UPDATE); // Clear update flag that triggers int as soon as the base is started
		HAL_TIM_Base_Start_IT(&htim5); 		// Start tim9 to count SLEEP_TIMEOUT useconds before going back to sleep.
		__HAL_TIM_SET_COUNTER(&htim5, 0); 	// restart the counter every time (in case a second interrupt occurs while already on run mode
	}
	// If the button is pressed/depressed, start the counter
	if(GPIO_Pin == GPIO_PIN_14)
	{
		// +Software debounce, 1ms
		__HAL_TIM_SET_COUNTER(&htim10, 0);
		HAL_TIM_Base_Start_IT(&htim10);
	}
	if(GPIO_Pin == SD_DETECT_PIN)
	{
		// debounce, reinit sd card, etc etc
		__HAL_TIM_SET_COUNTER(&htim11, 0);
		HAL_TIM_Base_Start_IT(&htim11);
	}
}

/**
  * @brief  Period elapsed callback in non-blocking mode for button debounce & backlight timeout
  * @param  htim TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == TIM5)
	{
//		log_segger("int tim5\n");
		HAL_TIM_Base_Stop_IT(&htim5);

		/* when timer elapses, go to sleep if usb is not plugged-in = PA9 is VBUS */
		if (!HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_9))
			go_to_sleep_flag = true;
	}
	if(htim->Instance == TIM10)
	{
		HAL_TIM_Base_Stop_IT(&htim10);

		if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14) == GPIO_PIN_RESET)
		{
			// Button pressed
//			log_segger("button debounced, pressed\n");
			HAL_GPIO_WritePin(UI_LED_GPIO_Port, UI_LED_Pin, GPIO_PIN_RESET);
			decode_and_display_jpg(jpg1);

			// Start timer 9 to count long presses for showing batt & temp OR restart
			HAL_TIM_Base_Start(&htim9);
		}
		else
		{
			// Button depressed
//			log_segger("button debounced, depressed\n");
			HAL_GPIO_WritePin(UI_LED_GPIO_Port, UI_LED_Pin, GPIO_PIN_SET);
			decode_and_display_jpg(jpg0);

			// In case the button is depressed, measure its duration and act accordingly
//			log_segger("tim9 duration: %4.2f seconds\n", (htim9.Instance->CNT)/2000.0);
			if(htim9.Instance->CNT > 20000) // 10 seconds to reset the system
				NVIC_SystemReset();
			if(htim9.Instance->CNT > 4000) 	// 2 seconds to display batt v & temp
				measure_and_display_voltage();
			__HAL_TIM_SET_COUNTER(&htim9, 0);
			HAL_TIM_Base_Stop(&htim9);
		}
	}
	if(htim->Instance == TIM11)
	{
		HAL_TIM_Base_Stop_IT(&htim11);

		if (BSP_PlatformIsDetected() == SD_PRESENT)
		{
			BSP_SD_Init();
			fatfs_read_SD_contents();
		}
		else
		{
			log_segger("sd removed \n");
			fatfs_unmount_SD();
		}
	}
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_SDIO_SD_Init();
  MX_ADC1_Init();
  MX_USB_DEVICE_Init();
  MX_LIBJPEG_Init();
  MX_FATFS_Init();
  MX_TIM9_Init();
  MX_TIM10_Init();
  MX_TIM11_Init();
  MX_TIM5_Init();
  /* USER CODE BEGIN 2 */
  ST7789H2_Init();
  lis2dh_init();

  log_segger("Init done\n");
  decode_and_display_jpg(jpg0);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
	  if (go_to_sleep_flag)
		  go_to_sleep();
    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 8;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
  /** Enables the Clock Security System
  */
  HAL_RCC_EnableCSS();
}

/* USER CODE BEGIN 4 */

void log_segger(const char *format, ...)
{
	static char segger_debug_string[1024];
	va_list args;
	va_start(args, format);
	vsprintf(segger_debug_string, format, args);
	va_end(args);

	SEGGER_RTT_WriteString(0, segger_debug_string);
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

	log_segger("error!!\n");

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	log_segger("Wrong parameters value: file %s on line %ld\r\n", file, line);
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
