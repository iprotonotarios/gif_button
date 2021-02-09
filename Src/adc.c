/**
  ******************************************************************************
  * File Name          : ADC.c
  * Description        : This file provides code for the configuration
  *                      of the ADC instances.
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

/* Includes ------------------------------------------------------------------*/
#include "adc.h"

/* USER CODE BEGIN 0 */
#include "st7789h2.h"
/* USER CODE END 0 */

ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

/* ADC1 init function */
void MX_ADC1_Init(void)
{
  ADC_ChannelConfTypeDef sConfig = {0};

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = ENABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 2;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SEQ_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_9;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
  sConfig.Rank = 2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

}

void HAL_ADC_MspInit(ADC_HandleTypeDef* adcHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(adcHandle->Instance==ADC1)
  {
  /* USER CODE BEGIN ADC1_MspInit 0 */

  /* USER CODE END ADC1_MspInit 0 */
    /* ADC1 clock enable */
    __HAL_RCC_ADC1_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**ADC1 GPIO Configuration
    PB1     ------> ADC1_IN9
    */
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* ADC1 DMA Init */
    /* ADC1 Init */
    hdma_adc1.Instance = DMA2_Stream0;
    hdma_adc1.Init.Channel = DMA_CHANNEL_0;
    hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_adc1.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_adc1.Init.MemInc = DMA_MINC_ENABLE;
    hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_adc1.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    hdma_adc1.Init.Mode = DMA_CIRCULAR;
    hdma_adc1.Init.Priority = DMA_PRIORITY_LOW;
    hdma_adc1.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_adc1) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(adcHandle,DMA_Handle,hdma_adc1);

    /* ADC1 interrupt Init */
    HAL_NVIC_SetPriority(ADC_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(ADC_IRQn);
  /* USER CODE BEGIN ADC1_MspInit 1 */

  /* USER CODE END ADC1_MspInit 1 */
  }
}

void HAL_ADC_MspDeInit(ADC_HandleTypeDef* adcHandle)
{

  if(adcHandle->Instance==ADC1)
  {
  /* USER CODE BEGIN ADC1_MspDeInit 0 */

  /* USER CODE END ADC1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_ADC1_CLK_DISABLE();

    /**ADC1 GPIO Configuration
    PB1     ------> ADC1_IN9
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_1);

    /* ADC1 DMA DeInit */
    HAL_DMA_DeInit(adcHandle->DMA_Handle);

    /* ADC1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(ADC_IRQn);
  /* USER CODE BEGIN ADC1_MspDeInit 1 */

  /* USER CODE END ADC1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

#define BATT_MEAS_LOW_BATT_LIMIT_MV      3000 // Cutoff voltage [mV].
#define BATT_MEAS_FULL_BATT_LIMIT_MV     4200 // Full charge definition [mV].
#define BATT_MEAS_VOLTAGE_TO_SOC_DELTA_MV  11 // mV between each element in the SoC vector.
#define BATT_MEAS_VOLTAGE_TO_SOC_ELEMENTS 111 // Number of elements in the state of charge vector.

/** Converts voltage to state of charge (SoC) [%]. The first element corresponds to the voltage
BATT_MEAS_LOW_BATT_LIMIT_MV and each element is BATT_MEAS_VOLTAGE_TO_SOC_DELTA_MV higher than the previous.
Numbers are obtained via model fed with experimental data. */
static const uint8_t BATT_MEAS_VOLTAGE_TO_SOC[] =
{
 0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,
 2,  2,  2,  2,  2,  2,  2,  2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,
 4,  5,  5,  5,  6,  6,  7,  7,  8,  8,  9,  9, 10, 11, 12, 13, 13, 14, 15, 16,
18, 19, 22, 25, 28, 32, 36, 40, 44, 47, 51, 53, 56, 58, 60, 62, 64, 66, 67, 69,
71, 72, 74, 76, 77, 79, 81, 82, 84, 85, 85, 86, 86, 86, 87, 88, 88, 89, 90, 91,
91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 100
};

uint32_t adc_buffer[2];
uint8_t string[100];
uint16_t battery_voltage_in_milli_volts;
uint8_t voltage_vector_element;
float ambient_temperature;

void measure_and_display_voltage(void)
{
	HAL_ADC_Start_DMA(&hadc1, adc_buffer, 2);
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	battery_voltage_in_milli_volts = adc_buffer[0]*ADC_REF_VOLTAGE/ADC_BITS*2; // *2 is due to 10M/10M voltage divider
	ambient_temperature = ((adc_buffer[1]*ADC_REF_VOLTAGE/ADC_BITS/1000.0)-0.76)/0.0025 + 25; // V25 = 0.76, Vsense = 2.5mV/C (RM0383 p226, DocID026289 p119)
	log_segger("Battery voltage: %d \nAmbient Temperature: %f \n", battery_voltage_in_milli_volts, ambient_temperature);

	HAL_ADC_Stop_DMA(&hadc1);

	voltage_vector_element = (battery_voltage_in_milli_volts - BATT_MEAS_LOW_BATT_LIMIT_MV)/BATT_MEAS_VOLTAGE_TO_SOC_DELTA_MV;

	// Ensure that only valid vector entries are used.
    if (voltage_vector_element < 0) {
    	voltage_vector_element = 0;
    } else if (voltage_vector_element > (BATT_MEAS_VOLTAGE_TO_SOC_ELEMENTS- 1) ) {
    	voltage_vector_element = (BATT_MEAS_VOLTAGE_TO_SOC_ELEMENTS- 1);
    }

	sprintf((char *)string, "Battery: %3d%%", BATT_MEAS_VOLTAGE_TO_SOC[voltage_vector_element]);
	ST7789H2_WriteString(0, 96, string, Font24, BLACK, WHITE);
	sprintf((char *)string, "Temp.: %3.1fC", ambient_temperature);
	ST7789H2_WriteString(0, 120, string, Font24, BLACK, WHITE);
}

void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc)
{
	log_segger("adc dma error: %d \n", hadc->ErrorCode);
}
/* USER CODE END 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
