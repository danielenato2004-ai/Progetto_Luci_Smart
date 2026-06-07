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
#include "adc.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include <stdio.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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

uint16_t adc_values[3];                 // Buffer per l'ADC


volatile uint8_t sistema_acceso = 1;    // Sostituisce la vecchia definizione
volatile uint8_t flag_leggi_adc = 0;       // Gestito dal TIM3 (15 secondi)
volatile uint8_t flag_bottone_premuto = 0; // Gestito dall'EXTI del bottone

// RE-INSERITA: Serve al Timer Callback a fondo pagina per non far fallire la compilazione!
volatile uint8_t mode_active = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

void RestoreClockAfterStopMode(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

uint16_t adc_values[3];               // Buffer ADC a 16-bit
char buffertx[100];                   // Buffer UART maggiorato per il debug
uint16_t pwm_value = 0;

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
  MX_ADC1_Init();
  MX_TIM2_Init();
  MX_USART2_UART_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */

  // 1. Avviamo le periferiche una volta sola all'avvio!
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
  HAL_TIM_Base_Start_IT(&htim3); // Avvia il TIM3 con interrupt attivo
  HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_values, 3); // Spostato qui risolve il crash!

  // --- STRATEGIA ANTI-FLOODING ---
  // Diciamo al DMA di lavorare in silenzio. Aggiornerà la RAM in background
  extern DMA_HandleTypeDef hdma_adc1;
  __HAL_DMA_DISABLE_IT(&hdma_adc1, DMA_IT_TC | DMA_IT_HT);


  // 2. ASSICURIAMO L'AVVIO: Pulizia da falsi interrupt elettrici all'accensione
  HAL_Delay(100);
  flag_bottone_premuto = 0;
  sistema_acceso = 1;
  mode_active = 0;       // <--- FORZA MODALITÀ AUTOMATICA ALL'AVVIO
  flag_leggi_adc = 1;    // <--- Richiedi subito la stampa del sensore automatico
  __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_13);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	  // =================================================================
	  // 1. GESTIONE BOTTONE (CON SOSPENSIONE SYSTICK CORRETTA)
	  // =================================================================
	  if (flag_bottone_premuto == 1)
	  {
		  HAL_Delay(50); // Debounce iniziale
		  if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET)
		  {
			  if (sistema_acceso == 1)
			  {
				  // Aspetta che l'utente rilasci il tasto prima di addormentarsi
				  while(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET);
				  HAL_Delay(50);

				  sistema_acceso = 0;

				  // Arresta i moduli hardware
				  HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
				  HAL_TIM_Base_Stop_IT(&htim3);
				  HAL_ADC_Stop_DMA(&hadc1);

				  // [FONDAMENTALE]: Sospende il clock di sistema da 1ms, altrimenti ci sveglia subito!
				  HAL_SuspendTick();

				  // Entra in STOP Mode
				  HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

				  // =========================================================
				  // RISVEGLIO (Il codice riparte da qui alla prossima pressione)
				  // =========================================================
				  RestoreClockAfterStopMode();
				  HAL_ResumeTick(); // Riattiva il clock di sistema di 1ms

				  // Aspetta il rilascio del tasto usato per svegliarlo
				  while(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET);
				  HAL_Delay(50);

				  // Ripristina l'hardware
				  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
				  HAL_TIM_Base_Start_IT(&htim3);
				  HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_values, 3);

				  // Silenzia il DMA per evitare l'allagamento della CPU
				  extern DMA_HandleTypeDef hdma_adc1;
				  __HAL_DMA_DISABLE_IT(&hdma_adc1, DMA_IT_TC | DMA_IT_HT);

				  flag_leggi_adc = 1; // Forza una lettura immediata al risveglio
				  sistema_acceso = 1;
			  }
		  }

		  flag_bottone_premuto = 0;
	  }

	  // =================================================================
	  // 2. LOGICA AUTOMATICA DEL SENSORE (POTENZIOMETRO ESCLUSO)
	  // =================================================================
	  if (sistema_acceso == 1)
	  {
		  if (flag_leggi_adc == 1)
		  {
			  flag_leggi_adc = 0;

			  // Leggiamo il sensore (assumiamo sia sul canale 0 del buffer)
			  uint16_t sensore = adc_values[0];

			  // Recuperiamo il valore di ARR impostato su CubeMX per il Timer del LED
			  uint32_t timer_arr = htim2.Instance->ARR;
			  if (timer_arr == 0) timer_arr = 255; // Valore di riserva se il registro è vuoto

			  // Formula Regolazione Automatica:
			  // Più c'è buio (valore sensore basso), più il LED deve illuminarsi (PWM alto)
			  pwm_value = timer_arr - ((sensore * timer_arr) / 4095);

			  // Invia il valore al registro del PWM
			  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, pwm_value);

			  // STAMPA DI TELEMETRIA (Ogni 15 secondi insieme al timer 3)
			  // Ci mostrerà i numeri esatti per capire se l'errore del LED è matematico o hardware
			  int len = sprintf(buffertx, "Sensore: %u | ARR Timer: %lu | Calcolo PWM: %u\r\n", sensore, timer_arr, pwm_value);
			  HAL_UART_Transmit(&huart2, (uint8_t*)buffertx, len, 100);
		  }
	  }
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
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

/* USER CODE BEGIN 4 */

void RestoreClockAfterStopMode(void)
{
    // 1. Riabilita il Power Clock e la configurazione del regolatore
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    // 2. Riaccendi il PLL (che si era spento in STOP mode)
    __HAL_RCC_PLL_ENABLE();

    // 3. Attendi che il PLL sia stabile e pronto
    while(__HAL_RCC_GET_FLAG(RCC_FLAG_PLLRDY) == RESET);

    // 4. Seleziona il PLL come sorgente del clock di sistema
    __HAL_RCC_SYSCLK_CONFIG(RCC_SYSCLKSOURCE_PLLCLK);

    // 5. Attendi che lo switch del clock sia completato
    while(__HAL_RCC_GET_SYSCLK_SOURCE() != RCC_SYSCLKSOURCE_STATUS_PLLCLK);
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
