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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdbool.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_tim.h"
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
TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */
#define EEPROM_ADDR 0x0800FC00
#define EEPROM_CAL_ADDR   0x0800FC02

#define DIN_PIN   GPIO_PIN_7
#define DIN_PORT  GPIOA
 
#define CLK_PIN   GPIO_PIN_5
#define CLK_PORT  GPIOA

#define CS_PIN    GPIO_PIN_6
#define CS_PORT   GPIOA

void MAX7219_Init(void);
void MAX7219_Send(uint8_t address, uint8_t data);
void MAX7219_Clear(void);
void MAX7219_DisplayDigit(uint8_t digit);
void MAX7219_WriteDigit(uint8_t position, uint8_t value);
void setRunScreen(bool autoMode);
uint16_t LoadCounter(void);
void SaveCounter(uint16_t value);
uint16_t LoadCaliValue(void);
void SaveCaliValue(uint16_t value);
void calibration_mode(void);
void showCalibrationValue(uint16_t value);
bool nowRun = false;

uint8_t setValue = 0;
uint8_t lsbVal = 0;
uint8_t msbVal = 0,wdt_Counter=0;
uint32_t actual_Milli_Value = 98123;
volatile uint32_t pulses = 0;
volatile uint16_t calibration_value = 260;
bool ready = true;
bool nowater=false;
bool manaul=false;
uint8_t MAX7219_DIGITS[10] =
{
    0x7E, // 0
    0x30, // 1
    0x6D, // 2
    0x79, // 3
    0x33, // 4
    0x5B, // 5
    0x5F, // 6
    0x70, // 7
    0x7F, // 8
    0x7B  // 9
};

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if(GPIO_Pin == GPIO_PIN_14)
    {
        pulses++;
        float actual_Milli_Value_F =(  (float)(pulses) / (float)calibration_value) *  (1000.00);
       actual_Milli_Value = actual_Milli_Value_F;
       // actual_Milli_Value = pulses;
       wdt_Counter=0;
       if (actual_Milli_Value > setValue * 1000.0 && manaul==false)
      {
        actual_Milli_Value = 0;
        pulses = 0;
        nowRun = false;
        ready = true;
       HAL_GPIO_WritePin(relayOut_GPIO_Port, relayOut_Pin, GPIO_PIN_SET); 
      }
    }
}

void MAX7219_WriteDigit(uint8_t position, uint8_t value)
{
    if(position > 7 || value > 9)
        return;

    MAX7219_Send(position + 1, value);
}

static void MAX7219_WriteByte(uint8_t data)
{
    for(int i=0;i<8;i++)
    {
        HAL_GPIO_WritePin(CLK_PORT, CLK_PIN, GPIO_PIN_RESET);

        if(data & 0x80)
            HAL_GPIO_WritePin(DIN_PORT, DIN_PIN, GPIO_PIN_SET);
        else
            HAL_GPIO_WritePin(DIN_PORT, DIN_PIN, GPIO_PIN_RESET);
//HAL_Delay(1);
        HAL_GPIO_WritePin(CLK_PORT, CLK_PIN, GPIO_PIN_SET);

        data <<= 1;
    }
}

void MAX7219_Send(uint8_t address, uint8_t data)
{
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);
    
    MAX7219_WriteByte(address);
  //  HAL_Delay(1);
    MAX7219_WriteByte(data);
   // HAL_Delay(1);
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);
}

void MAX7219_Init(void)
{
    MAX7219_Send(0x09, 0);   // BCD decode for all 8 digits
    MAX7219_Send(0x0A, 0x08);   // Brightness (0-15)
    MAX7219_Send(0x0B, 0x07);   // Scan limit = 8 digits
    MAX7219_Send(0x0C, 0x01);   // Normal operation
    MAX7219_Send(0x0F, 0x00);   // Display test off


    MAX7219_Clear();
}

void MAX7219_Clear(void)
{
    for(uint8_t i=1;i<=8;i++)
        MAX7219_Send(i,0x00);
}

 
void setRunScreen(bool autoMode)
{

  if (autoMode)
  {
     
    MAX7219_Send( 8, MAX7219_DIGITS[msbVal]);
    MAX7219_Send( 7, MAX7219_DIGITS[lsbVal]);
  }
  else
  {
    MAX7219_Clear();
  }

  MAX7219_Send( 6, 0x00);
  MAX7219_Send( 5, MAX7219_DIGITS[actual_Milli_Value / 10000]);
  MAX7219_Send( 4, MAX7219_DIGITS[(actual_Milli_Value / 1000) % 10] +0x80);
  MAX7219_Send( 3, MAX7219_DIGITS[(actual_Milli_Value / 100) % 10]);
  MAX7219_Send( 2, MAX7219_DIGITS[(actual_Milli_Value % 100) / 10]);
   MAX7219_Send( 1, 0x0e);
}

void showCalibrationValue(uint16_t value)
{
   
  MAX7219_Send( 8, 0x4e); //c
  MAX7219_Send( 7, 0x77); //a
  MAX7219_Send( 6, 0x0e); //l
   MAX7219_Send( 5, 0x00); //l
 
  MAX7219_Send( 4, MAX7219_DIGITS[value / 100]);
  MAX7219_Send( 3, MAX7219_DIGITS[(value / 10) % 10]);
  MAX7219_Send( 2, MAX7219_DIGITS[value % 10]);
 MAX7219_Send( 1, 0x00); // 
}
void disSet(uint8_t setVal)
{

  setVal = setVal % 100;

  MAX7219_Clear();
  MAX7219_Send( 8, 0x5b); //s
  MAX7219_Send( 7, 0x4f); //e
  MAX7219_Send( 6, 0x0f); //t
 
  MAX7219_Send( 4, MAX7219_DIGITS[setVal / 10]);
  MAX7219_Send( 3, MAX7219_DIGITS[setVal % 10]);
  MAX7219_Send( 1, 0x0e);
}


void SaveCaliValue(uint16_t value){

   HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef erase;
    uint32_t error;


    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.PageAddress = EEPROM_CAL_ADDR;
    erase.NbPages = 1;
    HAL_FLASHEx_Erase(&erase, &error);
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,
                      EEPROM_CAL_ADDR,
                      value);

    HAL_FLASH_Lock();

}
void SaveCounter(uint16_t value)
{
    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef erase;
    uint32_t error;

    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.PageAddress = EEPROM_ADDR;
    erase.NbPages = 1;
    HAL_FLASHEx_Erase(&erase, &error);

    HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,
                      EEPROM_ADDR,
                      value);

    HAL_FLASH_Lock();
}
uint16_t LoadCounter(void)
{
    return *(volatile uint16_t *)EEPROM_ADDR;
}

uint16_t LoadCaliValue(void)
{
    return *(volatile uint16_t *)EEPROM_CAL_ADDR;
}

void calibration_mode(){
  
  if(HAL_GPIO_ReadPin(ManualPin_GPIO_Port, ManualPin_Pin)== GPIO_PIN_RESET ){
    wdt_Counter=0;
    for(uint8_t i=0;i<5;i++){
      HAL_GPIO_WritePin(led_GPIO_Port, led_Pin, GPIO_PIN_RESET);
      HAL_Delay(100);
      HAL_GPIO_WritePin(led_GPIO_Port, led_Pin, GPIO_PIN_SET); 
      HAL_Delay(100);
    }
    bool done=false;
    showCalibrationValue(calibration_value);
    while(done==false){
      if(HAL_GPIO_ReadPin(msbPin_GPIO_Port, msbPin_Pin)== GPIO_PIN_RESET){
         if(calibration_value>0){
        calibration_value--;
         }
        showCalibrationValue(calibration_value);
        HAL_Delay(100);
        wdt_Counter=0;
      }
      else if (HAL_GPIO_ReadPin(lsbPin_GPIO_Port, lsbPin_Pin)== GPIO_PIN_RESET) {
       
       if(calibration_value<600){
calibration_value++;
       }
       
        showCalibrationValue(calibration_value);
        HAL_Delay(100);
        wdt_Counter=0;
      }

      if(HAL_GPIO_ReadPin(startPin_GPIO_Port  , startPin_Pin)== GPIO_PIN_RESET){
         SaveCounter((calibration_value*100)+setValue);
        done=true;
      }
      else if (wdt_Counter>20){
         done=true;
      }
    }
  }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
        HAL_GPIO_TogglePin(led_GPIO_Port, led_Pin);
        if(wdt_Counter<255){
          wdt_Counter++;
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
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  MAX7219_Init();
HAL_TIM_Base_Start_IT(&htim2);

 setValue = (LoadCounter()%100);
 calibration_value= (LoadCounter()/100);

  calibration_mode();

 
   showCalibrationValue(calibration_value);
   HAL_Delay(1000);

     
    msbVal = setValue / 10;
    lsbVal =setValue % 10;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

   //  HAL_GPIO_WritePin(led_GPIO_Port, led_Pin, GPIO_PIN_RESET);
    // HAL_Delay(20);

     //HAL_GPIO_WritePin(led_GPIO_Port, led_Pin, GPIO_PIN_SET); 
     //HAL_Delay(20);

    //  MAX7219_WriteDigit(4, (pulses /1000) );   // Next digit = 3
   //   MAX7219_WriteDigit(3, (pulses % 1000) % 10);   // Next digit = 3
    //  MAX7219_WriteDigit(2, (pulses % 100) % 10);   // Next digit = 3
    //  MAX7219_WriteDigit(1, (pulses % 100) / 10);   // Next digit = 3

    if(HAL_GPIO_ReadPin(stopPin_GPIO_Port, stopPin_Pin)== GPIO_PIN_RESET || nowater==true){
      
        nowRun = false;
        ready = true;
        pulses = 0;
        actual_Milli_Value = 0;
        HAL_GPIO_WritePin(relayOut_GPIO_Port, relayOut_Pin, GPIO_PIN_SET);
         
        HAL_Delay(300);
       // Serial.println("STOP");
      }

      //---------------------------
      if (nowRun)
  {
    if(wdt_Counter>20){
      nowater=true;
    }
    setRunScreen(true);
  }
  else if (ready)
  {
   
           MAX7219_Send( 6, 0x00);
            MAX7219_Send( 5, 0x05); //r
             MAX7219_Send(4, 0x4f); //e
            MAX7219_Send( 3, 0x77); //a
            MAX7219_Send( 2, 0x3d); //d
            MAX7219_Send( 1, 0x3b); //y
         
          MAX7219_Send( 8, MAX7219_DIGITS[msbVal]);
          MAX7219_Send( 7, MAX7219_DIGITS[lsbVal]);
     
  }


  //---------------------------
    
        


      if(HAL_GPIO_ReadPin(ManualPin_GPIO_Port, ManualPin_Pin)== GPIO_PIN_RESET ){
        
      nowater=false;
      HAL_GPIO_WritePin(relayOut_GPIO_Port, relayOut_Pin, GPIO_PIN_RESET);
        HAL_Delay (15);
       
      manaul=true;
      
      pulses = 0;
        while (HAL_GPIO_ReadPin(ManualPin_GPIO_Port, ManualPin_Pin) == GPIO_PIN_RESET)
        {
          
        //  MAX7219_Init();
          HAL_Delay(50);
          //pulses=0;
          setRunScreen(false);
          
           
        }
        HAL_GPIO_WritePin(relayOut_GPIO_Port, relayOut_Pin, GPIO_PIN_SET);
        ready=0;
manaul=false;
        HAL_Delay(100);
        MAX7219_Init();
        setRunScreen(false);
        
      }

      else if (HAL_GPIO_ReadPin(startPin_GPIO_Port, startPin_Pin) == GPIO_PIN_RESET)
      {
        wdt_Counter=0;
        nowater=false;
        pulses = 0;
        nowRun = true;
        actual_Milli_Value = 0;
        ready = false;
        HAL_GPIO_WritePin(relayOut_GPIO_Port, relayOut_Pin, GPIO_PIN_RESET);
        HAL_Delay(50);

        MAX7219_Init();
      }

    if (HAL_GPIO_ReadPin(lsbPin_GPIO_Port , lsbPin_Pin) == GPIO_PIN_RESET) {
        ready = 0;
          wdt_Counter=0;

        lsbVal++;
        if (lsbVal > 9)
        {
          lsbVal = 0;
        }
        setValue = msbVal * 10 + lsbVal;
        disSet(setValue);
        nowRun = false;
        HAL_Delay(300);
        pulses = 0;
        actual_Milli_Value = 0;
        SaveCounter((calibration_value*100)+setValue);
        //EEPROM.update(0, setValue);
      }


      else if (HAL_GPIO_ReadPin(msbPin_GPIO_Port, msbPin_Pin) == GPIO_PIN_RESET) {
          wdt_Counter=0;
          ready = 0;
          msbVal++;
          if (msbVal > 9)
          {
            msbVal = 0;
          }
          setValue = msbVal * 10 + lsbVal;
          disSet(setValue);
          nowRun = false;
          HAL_Delay(300);
          pulses = 0;
        SaveCounter((calibration_value*100)+setValue);

          actual_Milli_Value = 0;
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 64000-1;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 1000;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(led_GPIO_Port, led_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(relayOut_GPIO_Port, relayOut_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : led_Pin */
  GPIO_InitStruct.Pin = led_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(led_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : msbPin_Pin startPin_Pin ManualPin_Pin stopPin_Pin
                           lsbPin_Pin */
  GPIO_InitStruct.Pin = msbPin_Pin|startPin_Pin|ManualPin_Pin|stopPin_Pin
                          |lsbPin_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PA5 PA6 PA7 */
  GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : relayOut_Pin */
  GPIO_InitStruct.Pin = relayOut_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(relayOut_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : isrpin_Pin */
  GPIO_InitStruct.Pin = isrpin_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(isrpin_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

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
