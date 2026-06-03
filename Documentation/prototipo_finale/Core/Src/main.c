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
#include "stdio.h"

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

uint16_t prev_pot = 0;          // Memorizza l'ultima posizione del potenziometro
uint32_t manual_timestamp = 0;  // Salva il millisecondo in cui è avvenuto l'ultimo movimento
#define POT_THRESHOLD  300      // Soglia di variazione, per evitare il rumore dell'ADC

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

void RestoreClockAfterStopMode(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

char buffertx[100];                   // Buffer UART maggiorato per il debug.
uint32_t pwm_value = 0;               // MODIFICATO IN uint32_t: Il Timer 2 lavora a 32-bit!

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

  HAL_Delay(5); // dà tempo al DMA di fare la prima lettura
  prev_pot = adc_values[1]; //  allinea il potenziometro attuale

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

				  // ---------------------------------------------------------
				  // LOGICA DI RIPRISTINO MODALITÀ (FORZATURA AUTOMATICO)
				  // ---------------------------------------------------------
				  HAL_Delay(10);             // Dà il tempo al DMA di fare la prima lettura reale
				  mode_active = 0;          // Forza il ritorno alla modalità AUTOMATICA
				  flag_leggi_adc = 1;       // Forza una lettura immediata della fotoresistenza senza aspettare 15s
				  prev_pot = adc_values[1]; // Cattura la posizione attuale del potenziometro per evitare falsi override
				  // ---------------------------------------------------------

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
	  // 2. LOGICA MODALITÀ CON OVERRIDE MANUALE AUTOMATICO (TIMEOUT 30s)
	  // =================================================================
	  if (sistema_acceso == 1)
	  {
		  // Leggiamo costantemente il potenziometro dal buffer DMA per massima reattività
		  uint16_t current_pot = adc_values[1];

		  // Calcoliamo il valore assoluto della variazione rispetto al ciclo precedente
		  int32_t variazione = (int32_t)current_pot - (int32_t)prev_pot;
		  if (variazione < 0) variazione = -variazione;

		  // SE L'UTENTE MUOVE IL POTENZIOMETRO (Variazione apprezzabile)
		  if (variazione > POT_THRESHOLD)
		  {
			  mode_active = 1;                  // Attiva/Mantiene la modalità manuale
			  manual_timestamp = HAL_GetTick(); // Avvia (o resetta) il timer da questo preciso millisecondo
			  prev_pot = current_pot;           // Aggiorna il valore di riferimento

			  // Calcola e aggiorna immediatamente il PWM (feedback istantaneo sulla manopola)
			  uint32_t timer_arr = htim2.Instance->ARR;
			  if (timer_arr == 0) timer_arr = 255;
			  pwm_value = (uint32_t)(((uint64_t)current_pot * timer_arr) / 4095);
			  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, pwm_value);

			  // Stampa UART: viene eseguita SOLO quando muovi la manopola (non intasa la seriale)
			  int len = sprintf(buffertx, "OVERRIDE MANUALE | Pot: %u | PWM: %lu\r\n", current_pot, pwm_value);
			  HAL_UART_Transmit(&huart2, (uint8_t*)buffertx, len, 100);
		  }

		  // SE SIAMO IN MODALITÀ MANUALE, CONTROLLIAMO SE SONO PASSATI 30 SECONDI
		  if (mode_active == 1)
		  {
			  // 30000 millisecondi = 30 secondi dall'ultimo movimento rilevato
			  if (HAL_GetTick() - manual_timestamp >= 30000)
			  {
				  mode_active = 0;    // Forza il ritorno alla modalità automatica
				  flag_leggi_adc = 1; // Forza una lettura immediata del sensore luce (evita di attendere il timer dei 15s)

				  int len = sprintf(buffertx, "TIMEOUT 30s SCADUTO | Ritorno in modalità AUTOMATICA\r\n");
				  HAL_UART_Transmit(&huart2, (uint8_t*)buffertx, len, 100);

				  // Riallinea prev_pot al valore attuale per evitare che la modalità manuale si riattivi da sola
				  prev_pot = adc_values[1];
			  }
		  }

		  // LOGICA AUTOMATICA (Eseguita ogni 15 secondi, SOLO se mode_active è 0)
		  if (mode_active == 0)
		  {
			  if (flag_leggi_adc == 1)
			  {
				  flag_leggi_adc = 0; // Resetta il flag del timer dei 15s

				  uint16_t sensore = adc_values[0];
				  uint32_t timer_arr = htim2.Instance->ARR;
				  if (timer_arr == 0) timer_arr = 255;

				  pwm_value = (uint32_t)(((uint64_t)sensore * timer_arr) / 4095);
				  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, pwm_value);

				  // Mantieni prev_pot allineato per evitare falsi trigger quando si esce dall'automatico
				  prev_pot = adc_values[1];

				  int len = sprintf(buffertx, "MOD: AUTO | Sensore: %u | PWM: %lu\r\n", sensore, pwm_value);
				  HAL_UART_Transmit(&huart2, (uint8_t*)buffertx, len, 100);
			  }
		  }
	  }
  /* USER CODE END 3 */
}
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
