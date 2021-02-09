#include "gif_button_bsp.h"
#include "string.h"
#include "stdio.h"
#include "main.h"
#include "lis2dh12_reg.h"
#include "fatfs_platform.h"

/*
 * ST7789 interface
 */

extern SPI_HandleTypeDef hspi1;
/**
  * @brief  Writes command to select the LCD register.
  * @param  LCDReg: Address of the selected register.
  */
void LCD_IO_WriteReg(uint8_t LCDReg)
{
  /* Set LCD data/command line DC to Low */
  LCD_DC_LOW();

  /* Send Command */
  HAL_SPI_Transmit_DMA(&hspi1, (uint8_t*) &LCDReg, 1);

  LCD_DC_HIGH();
}

/**
  * @brief  Writes data to select the LCD register.
  * @param  Data: data to write to the selected register.
  */
void LCD_IO_WriteData(uint8_t Data)
{
  /* Send Data */
  HAL_SPI_Transmit_DMA(&hspi1, (uint8_t*) &Data, 1);
}

/**
  * @brief  Writes register value.
  * @param  pData: Pointer on the register value
  * @param  Size: Size of byte to transmit to the register
  */
void LCD_IO_WriteMultipleData(uint8_t *pData, uint32_t Size)
{
	HAL_StatusTypeDef status = HAL_OK;

	status = HAL_SPI_Transmit_DMA(&hspi1, pData, Size);
	//	  status = HAL_SPI_Transmit(&hspi1, pData, Size, 1000);

	/* Check the communication status */
	if(status != HAL_OK)
	{
		log_segger("Error: %x!\n", status);
	}

	// wait until DMA transfer is finished (still way faster than normal spi transactions)
	while(((hspi1.Instance->SR) & SPI_FLAG_BSY) == SPI_FLAG_BSY);
}


void LCD_hw_reset(void)
{
	LCD_RES_HIGH();
	HAL_Delay(1);
	LCD_RES_LOW();
	HAL_Delay(1);
	LCD_RES_HIGH();
	HAL_Delay(140);
}

/*
 * LIS2DH interface
 */

extern I2C_HandleTypeDef hi2c1;

/*
 * @brief  Write generic device register (platform dependent)
 *
 * @param  handle    spi handle
 * @param  reg       register to write
 * @param  bufp      pointer to data to write in register reg
 * @param  len       number of consecutive register to write
 *
 */
static int32_t platform_write(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len)
{
	if (handle == &hi2c1)
	{
	    /* Write multiple command */
		if (len > 1)
			reg |= 0x80;
		HAL_I2C_Mem_Write(handle, LIS2DH_I2C_ADD, reg, I2C_MEMADD_SIZE_8BIT, bufp, len, 1000);
	}
	return 0;
}

/*
 * @brief  Read generic device register (platform dependent)
 *
 * @param  handle    spi handle
 * @param  reg       register to read
 * @param  bufp      pointer to buffer that store the data read
 * @param  len       number of consecutive register to read
 *
 */
static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len)
{
	if (handle == &hi2c1)
	{
	    /* Read multiple command */
		if (len > 1)
			reg |= 0x80;
	    HAL_I2C_Mem_Read(handle, LIS2DH_I2C_ADD, reg, I2C_MEMADD_SIZE_8BIT, bufp, len, 1000);
	}
	return 0;
}

void lis2dh_init(void) //TODO CTL5 bit 7 reboot memory
{
	stmdev_ctx_t dev_ctx;
	uint8_t param;

	/* Initialize mems driver interface */
	dev_ctx.write_reg = platform_write;
	dev_ctx.read_reg = platform_read;
	dev_ctx.handle = &hi2c1;


	lis2dh12_read_reg(&dev_ctx, LIS2DH12_WHO_AM_I, &param, 1);
	log_segger("whoami: %x\n", param);

	param = 0x80; // reboot memory content
	lis2dh12_write_reg(&dev_ctx, LIS2DH12_CTRL_REG5, &param, 1);

	HAL_Delay(10);

	param = 0x4F; // 50Hz, low power, all axis
	lis2dh12_write_reg(&dev_ctx, LIS2DH12_CTRL_REG1, &param, 1);
	param = 0x01; // HP filter enable
	lis2dh12_write_reg(&dev_ctx, LIS2DH12_CTRL_REG2, &param, 1);
	param = 0x40; // And/or interrupt enable on INT1 (AOI1)
	lis2dh12_write_reg(&dev_ctx, LIS2DH12_CTRL_REG3, &param, 1);

	param = 0x2A; // conf int1 for high events on xyz
	lis2dh12_write_reg(&dev_ctx, LIS2DH12_INT1_CFG, &param, 1);
	param = 0x05; // threshold for movement detection
	lis2dh12_write_reg(&dev_ctx, LIS2DH12_INT1_THS, &param, 1);
	param = 0x00; // duration for movement detection
	lis2dh12_write_reg(&dev_ctx, LIS2DH12_INT1_DURATION, &param, 1);
}

/*
 * SD card interface
 */

/**
  * @brief  Configures Interrupt mode for SD detection pin.
  * @retval Returns 0
  */
uint8_t BSP_SD_ITConfig(void)
{
  GPIO_InitTypeDef gpio_init_structure;

  /* Configure Interrupt mode for SD detection pin */
  gpio_init_structure.Pin = SD_DETECT_PIN;
  gpio_init_structure.Pull = GPIO_NOPULL;
  gpio_init_structure.Speed = GPIO_SPEED_FAST;
  gpio_init_structure.Mode = GPIO_MODE_IT_RISING_FALLING;
  HAL_GPIO_Init(SD_DETECT_GPIO_PORT, &gpio_init_structure);

  // Pin 10 is already enabled due EXTI15_10_IRQHandler

  return 0;
}
