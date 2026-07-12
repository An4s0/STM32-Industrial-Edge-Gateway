/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fatfs.h"
#include "lwip.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stm32f7xx_hal_def.h"
#include "stm32f7xx_hal_spi.h"
#include "string.h"
#include "stdio.h"
#include "time.h"
#include "lwip/apps/httpd.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct
{
  float temperature;
  float humidity;
  float pressure;
  float pm1;
  float pm25;
  float pm10;
  uint16_t aqi;

  uint32_t timestamp;
} AirData_t;

typedef struct
{
  float Clow;
  float Chigh;
  uint16_t Ilow;
  uint16_t Ihigh;
} AQITable;

typedef enum
{
  MODBUS_IDLE, // LED OFF
  MODBUS_BUSY, // Slow blink
  MODBUS_OK,   // Short flash
  MODBUS_ERROR // Solid ON
} ModbusStatus_t;

typedef enum
{
  SD_IDLE, // LED OFF
  SD_BUSY, // Slow blink
  SD_OK,   // Short flash
  SD_ERROR // Solid ON
} SDStatus_t;

typedef struct
{
  uint8_t rx[32];
  HAL_StatusTypeDef status;
} Modbus;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SLAVE_ID 0x01
#define MODBUS_REQ_SIZE 8
#define FUN04_RESPONSE_SIZE 19
#define FUN03_RESPONSE_SIZE 9
#define PERIOD_MS 10000
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart4;
UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */
AirData_t data = {0};
uint32_t lastSampleTick = 0;

uint8_t frameFun04[MODBUS_REQ_SIZE] = {
    SLAVE_ID,
    0x04,       // Function Code (3xxxx)
    0x00, 0x07, // Starting Address (30008)
    0x00, 0x07  // Number of Registers (30014)
};

uint8_t frameFun03[MODBUS_REQ_SIZE] = {
    SLAVE_ID,
    0x03,       // Function Code (4xxxx)
    0x00, 0x04, // Starting Address (40005)
    0x00, 0x02  // Number of Registers (40006)
};

static const AQITable PM25Table[] =
    {{0.0f, 9.0f, 0, 50},
     {9.1f, 35.4f, 51, 100},
     {35.5f, 55.4f, 101, 150},
     {55.5f, 125.4f, 151, 200},
     {125.5f, 225.4f, 201, 300},
     {225.5f, 325.4f, 301, 400},
     {325.5f, 500.4f, 401, 500}};

static const AQITable PM10Table[] =
    {{0, 54, 0, 50},
     {55, 154, 51, 100},
     {155, 254, 101, 150},
     {255, 354, 151, 200},
     {355, 424, 201, 300},
     {425, 504, 301, 400},
     {505, 604, 401, 500}};

FATFS fs;
FIL file;
FRESULT fr;
UINT bytesWritten;
UINT bytesRead;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
static void MX_GPIO_Init(void);
static void MX_UART4_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_SPI1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint16_t Modbus_CRC16(uint8_t *buf, uint16_t len)
{
  uint16_t crc = 0xFFFF;

  for (uint16_t i = 0; i < len; i++)
  {
    crc ^= buf[i];

    for (uint8_t j = 0; j < 8; j++)
    {
      if (crc & 1)
        crc = (crc >> 1) ^ 0xA001;
      else
        crc >>= 1;
    }
  }

  return crc;
}

Modbus Modbus_TransmitReceive(UART_HandleTypeDef *huart, uint8_t *frame, uint8_t rxLength)
{
  Modbus result = {0};

  uint16_t crc = Modbus_CRC16(frame, MODBUS_REQ_SIZE - 2);

  uint8_t tx[8];
  memcpy(tx, frame, MODBUS_REQ_SIZE);

  tx[6] = crc & 0xFF;
  tx[7] = (crc >> 8) & 0xFF;

  result.status = HAL_UART_Transmit(huart, tx, MODBUS_REQ_SIZE, 1000);

  if (result.status != HAL_OK)
    return result;

  result.status = HAL_UART_Receive(huart, result.rx, rxLength, 1000);

  if (result.status != HAL_OK)
    return result;

  uint16_t rxCRC = result.rx[rxLength - 2] | (result.rx[rxLength - 1] << 8);
  uint16_t calcCRC = Modbus_CRC16(result.rx, rxLength - 2);

  if (rxCRC != calcCRC)
    result.status = HAL_ERROR;

  return result;
}

uint16_t AQIFromTable(float concentration,
                      const AQITable *table,
                      uint8_t size)
{
  if (concentration < 0)
    return 0;

  for (int i = 0; i < size; i++)
  {
    if (concentration >= table[i].Clow &&
        concentration <= table[i].Chigh)
    {
      float aqi =
          ((table[i].Ihigh - table[i].Ilow) /
           (table[i].Chigh - table[i].Clow)) *
              (concentration - table[i].Clow) +
          table[i].Ilow;

      return (uint16_t)(aqi + 0.5f);
    }
  }

  return 500;
}

uint16_t CalculateAQI(float pm25, float pm10)
{
  uint16_t aqi25 =
      AQIFromTable(pm25,
                   PM25Table,
                   sizeof(PM25Table) / sizeof(AQITable));

  uint16_t aqi10 =
      AQIFromTable(pm10,
                   PM10Table,
                   sizeof(PM10Table) / sizeof(AQITable));

  return (aqi25 > aqi10) ? aqi25 : aqi10;
}

void ModbusStatusLED(ModbusStatus_t modbusStatus)
{
  static uint32_t lastToggle = 0;

  switch (modbusStatus)
  {
  case MODBUS_IDLE:
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);
    break;

  case MODBUS_BUSY:
    if (HAL_GetTick() - lastToggle >= 500)
    {
      lastToggle = HAL_GetTick();
      HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_3);
    }
    break;

  case MODBUS_OK:
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);
    break;

  case MODBUS_ERROR:
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);
    break;
  }
}

void SDStatusLED(SDStatus_t sdStatus)
{
  static uint32_t lastToggle = 0;

  switch (sdStatus)
  {
  case SD_IDLE:
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);
    break;

  case SD_BUSY:
    if (HAL_GetTick() - lastToggle >= 500)
    {
      lastToggle = HAL_GetTick();
      HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_0);
    }
    break;

  case SD_OK:
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);
    break;

  case SD_ERROR:
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET);
    break;
  }
}

u16_t SSI_Handler(int iIndex, char *pcInsert, int iInsertLen)
{
  extern uint32_t lastSampleTick;

  switch (iIndex)
  {
  case 0:
    return snprintf(pcInsert, iInsertLen, "%.1f", data.pm25);

  case 1:
    return snprintf(pcInsert, iInsertLen, "%.1f", data.pm10);

  case 2:
    return snprintf(pcInsert, iInsertLen, "%.1f", data.pm1);

  case 3:
    return snprintf(pcInsert, iInsertLen, "%.2f", data.temperature);

  case 4:
    return snprintf(pcInsert, iInsertLen, "%.2f", data.humidity);

  case 5:
    return snprintf(pcInsert, iInsertLen, "%.1f", data.pressure);

  case 6:
    return snprintf(pcInsert, iInsertLen, "%u", data.aqi);

  case 7:
    return snprintf(pcInsert, iInsertLen, "%lu", (HAL_GetTick() - lastSampleTick) / 1000);

  default:
    pcInsert[0] = '\0';
    return 0;
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
  ModbusStatus_t modbusStatus = MODBUS_IDLE;

  SDStatus_t sdStatus = SD_IDLE;
  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

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
  MX_UART4_Init();
  MX_USART3_UART_Init();
  MX_SPI1_Init();
  MX_FATFS_Init();
  MX_LWIP_Init();
  /* USER CODE BEGIN 2 */
  fr = f_mount(&fs, "", 1);
  if (fr != FR_OK)
  {
    sdStatus = SD_ERROR;
    SDStatusLED(sdStatus);
  }

  httpd_init();

  const char *tags[] = {
      "pm25", "pm10", "pm1",
      "temp", "hum", "press",
      "aqi", "updated"};

  http_set_ssi_handler(SSI_Handler, tags, LWIP_ARRAYSIZE(tags));
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  uint32_t lastSample = 0;

  while (1)
  {
    MX_LWIP_Process();

    if (HAL_GetTick() - lastSample >= PERIOD_MS)
    {
      lastSample = HAL_GetTick();
      modbusStatus = MODBUS_BUSY;
      ModbusStatusLED(modbusStatus);
      Modbus modbusFun04 = Modbus_TransmitReceive(&huart4, frameFun04, FUN04_RESPONSE_SIZE);
      Modbus modbusFun03 = Modbus_TransmitReceive(&huart4, frameFun03, FUN03_RESPONSE_SIZE);

      if (modbusFun04.status != HAL_OK || modbusFun03.status != HAL_OK)
      {
        modbusStatus = MODBUS_ERROR;
        ModbusStatusLED(modbusStatus);
        continue;
      }

      uint8_t *rxFun04 = modbusFun04.rx;
      uint8_t *rxFun03 = modbusFun03.rx;

      if ((rxFun04[0] != SLAVE_ID || rxFun04[1] != 0x04 || rxFun04[2] != 14) ||
          (rxFun03[0] != SLAVE_ID || rxFun03[1] != 0x03 || rxFun03[2] != 4))
      {
        modbusStatus = MODBUS_ERROR;
        ModbusStatusLED(modbusStatus);
        HAL_Delay(PERIOD_MS);
        continue;
      }

      data.temperature = (((uint16_t)rxFun04[3] << 8) | rxFun04[4]) * 0.01f;
      data.humidity = (((uint16_t)rxFun04[5] << 8) | rxFun04[6]) * 0.01f;
      data.pressure = (((uint16_t)rxFun04[9] << 8) | rxFun04[10]) * 0.1f;
      data.pm1 = (((uint16_t)rxFun04[11] << 8) | rxFun04[12]) * 0.1f;
      data.pm25 = (((uint16_t)rxFun04[13] << 8) | rxFun04[14]) * 0.1f;
      data.pm10 = (((uint16_t)rxFun04[15] << 8) | rxFun04[16]) * 0.1f;
      data.aqi = CalculateAQI(data.pm25, data.pm10);

      data.timestamp =
          ((uint32_t)(((uint16_t)rxFun03[3] << 8) | rxFun03[4]) << 16) |
          (((uint16_t)rxFun03[5] << 8) | rxFun03[6]);

      lastSampleTick = HAL_GetTick();

      time_t t = (time_t)(data.timestamp + 3 * 3600); // UTC+3
      struct tm *tm_info = gmtime(&t);

      char timeStr[32];

      strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", tm_info);

      modbusStatus = MODBUS_OK;
      ModbusStatusLED(modbusStatus);

      sdStatus = SD_BUSY;
      SDStatusLED(sdStatus);

      FILINFO fno;
      fr = f_stat("data.csv", &fno);

      if (fr == FR_NO_FILE)
      {
        fr = f_open(&file, "data.csv", FA_CREATE_NEW | FA_WRITE);

        if (fr == FR_OK)
        {
          char header[] = "Timestamp,Temperature,Humidity,Pressure,PM1,PM2.5,PM10,AQI\r\n";

          f_write(&file, header, strlen(header), &bytesWritten);
          f_close(&file);
        }
      }

      fr = f_open(&file, "data.csv", FA_OPEN_EXISTING | FA_WRITE);

      if (fr != FR_OK)
      {
        sdStatus = SD_ERROR;
        SDStatusLED(sdStatus);
        continue;
      }

      char fdata[128];
      sprintf(fdata,
              "%s,%.2f,%.2f,%.1f,%.1f,%.1f,%.1f,%u\r\n",
              timeStr,
              data.temperature,
              data.humidity,
              data.pressure,
              data.pm1,
              data.pm25,
              data.pm10,
              data.aqi);

      fr = f_write(&file, fdata, strlen(fdata), &bytesWritten);

      if (fr != FR_OK || bytesWritten != strlen(fdata))
      {
        sdStatus = SD_ERROR;
        SDStatusLED(sdStatus);
        continue;
      }

      f_sync(&file);
      f_close(&file);
      sdStatus = SD_OK;
      SDStatusLED(sdStatus);
    }
    /* USER CODE END WHILE */

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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 128;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
 * @brief SPI1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */
}

/**
 * @brief UART4 Initialization Function
 * @param None
 * @retval None
 */
static void MX_UART4_Init(void)
{

  /* USER CODE BEGIN UART4_Init 0 */

  /* USER CODE END UART4_Init 0 */

  /* USER CODE BEGIN UART4_Init 1 */

  /* USER CODE END UART4_Init 1 */
  huart4.Instance = UART4;
  huart4.Init.BaudRate = 19200;
  huart4.Init.WordLength = UART_WORDLENGTH_8B;
  huart4.Init.StopBits = UART_STOPBITS_1;
  huart4.Init.Parity = UART_PARITY_NONE;
  huart4.Init.Mode = UART_MODE_TX_RX;
  huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart4.Init.OverSampling = UART_OVERSAMPLING_16;
  huart4.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart4.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN UART4_Init 2 */

  /* USER CODE END UART4_Init 2 */
}

/**
 * @brief USART3 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOF, GPIO_PIN_5, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);

  /*Configure GPIO pin : PF5 */
  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pin : PF10 */
  GPIO_InitStruct.Pin = GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pin : PC0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PA3 */
  GPIO_InitStruct.Pin = GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
   */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
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
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
