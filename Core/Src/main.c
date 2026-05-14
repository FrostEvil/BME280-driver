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
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "bme280.h"
#include "parse_command.h"
#include <string.h>
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

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#define TX_RING_BUFFER_SIZE 8

volatile uint8_t timer_flag = 0;
volatile uint8_t forced_measurement_flag = 0;
volatile uint8_t forced_timer_flag = 0;

/* UART receive flags and command buffer control */
volatile uint8_t rx_flag = 0;
volatile uint8_t overflow_flag = 0;

volatile uint8_t tx_busy = 0;

/* Single received UART character */
uint8_t rx_data = 0;

/* Current write position inside RX buffer */
uint8_t rx_index = 0;

/* Buffer for incoming UART command */
char rx_data_buffer[32];

/* Buffer for UART response messages */
char uart_response[256];

char tx_ring_buffer[TX_RING_BUFFER_SIZE][256];
volatile uint8_t tx_ring_head = 0;
volatile uint8_t tx_ring_tail = 0;
volatile uint8_t tx_overflow_counter = 0;

Command parsed_command;

static void UART_Send_Message() {
	HAL_StatusTypeDef tx_status;

	if (tx_overflow_counter > 0
			&& (tx_ring_head + 1) % TX_RING_BUFFER_SIZE != tx_ring_tail) {

		snprintf(tx_ring_buffer[tx_ring_head],
				sizeof(tx_ring_buffer[tx_ring_head]),
				"[WARNING] TX overflow! Dropped %d messages\r\n",
				tx_overflow_counter);
		tx_overflow_counter = 0;
		tx_ring_head = (tx_ring_head + 1) % TX_RING_BUFFER_SIZE;

	}

	if (tx_ring_head == tx_ring_tail) {
		return;
	}

	if (tx_busy == 1) {
		return;
	}

	tx_busy = 1;
	tx_status = HAL_UART_Transmit_IT(&huart2,
			(uint8_t*) tx_ring_buffer[tx_ring_tail],
			strlen(tx_ring_buffer[tx_ring_tail]));

	if (tx_status == HAL_OK) {
		tx_ring_tail = (tx_ring_tail + 1) % TX_RING_BUFFER_SIZE;
	} else {
		tx_busy = 0;
	}

}

static void Add_Tx_Message(const char *msg) {
	if ((tx_ring_head + 1) % TX_RING_BUFFER_SIZE == tx_ring_tail) {
		tx_overflow_counter++;
		return;
	} else {
		snprintf(tx_ring_buffer[tx_ring_head],
				sizeof(tx_ring_buffer[tx_ring_head]), "%s", msg);
		tx_ring_head = (tx_ring_head + 1) % TX_RING_BUFFER_SIZE;
	}

	UART_Send_Message();
}

static void UART_ProcessRxData(BME280_HandleTypeDef *bme) {
	/* Detect RX buffer overflow */
	if (rx_index >= sizeof(rx_data_buffer) - 1) {
		overflow_flag = 1;
	}

	/* Wait for end of invalid command after overflow */
	if (overflow_flag == 1 && rx_data == '\n') {

		rx_index = 0;
		overflow_flag = 0;

		rx_data_buffer[rx_index] = '\0';

		Add_Tx_Message("Overflow! Try again.\r\n");

	} else if (overflow_flag == 0) {

		/* Store received character inside RX buffer */
		if (rx_data != '\n') {

			rx_data_buffer[rx_index++] = rx_data;

		} else {

			/* End of command detected */
			rx_data_buffer[rx_index] = '\0';

			rx_index = 0;

			parsed_command = ParseCommand(rx_data_buffer);
			ExecuteCommand(&parsed_command, bme, uart_response);
			Add_Tx_Message(uart_response);
		}
	}

	rx_flag = 0;

}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

	/* USER CODE BEGIN 1 */

	BME280_HandleTypeDef bme;

	bme.hi2c = &hi2c1;
	bme.address = 0x76 << 1;
	bme.t_fine = 0;

	bme.osrs_t = BME280_OSRS_X1;
	bme.osrs_p = BME280_OSRS_X1;
	bme.osrs_h = BME280_OSRS_X1;

	bme.mode = BME280_SLEEP_MODE;

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
	MX_USART2_UART_Init();
	MX_I2C1_Init();
	MX_TIM10_Init();
	/* USER CODE BEGIN 2 */

	/* Start timer interrupt */
	HAL_TIM_Base_Init(&htim10);
	HAL_TIM_Base_Start_IT(&htim10);

	/* Start UART reception in interrupt mode */
	HAL_UART_Receive_IT(&huart2, &rx_data, 1);

	/* Initialize BME280 sensor */
	HAL_StatusTypeDef BME280_init_status = BME280_Init(&bme);

	float temp;
	float pressure;
	float humidity;

	char measurements[3][32];

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */

		/* Handle received UART data */

		if (rx_flag == 1) {
			UART_ProcessRxData(&bme);
		}

		/* Trigger single forced measurement after button press */
		if (HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) == GPIO_PIN_RESET
				&& forced_timer_flag == 0) {

			if (BME280_TriggerForcedMeasurement(&bme) == HAL_OK) {

				forced_timer_flag = 1;
				timer_flag = 0;
			}

		} else if (HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) == GPIO_PIN_SET) {

			forced_timer_flag = 0;
		}

		/* Read and print measurement after forced conversion */
		if (forced_measurement_flag == 1) {

			if (BME280_ReadMeasurements(&bme, &temp, &pressure, &humidity)
					== HAL_OK && BME280_init_status == HAL_OK) {

				snprintf(measurements[0], sizeof(measurements[0]),
						"Temperature: %.2f C.\r\n", temp);
				snprintf(measurements[1], sizeof(measurements[1]),
						"Pressure: %.2f hPa.\r\n", pressure);
				snprintf(measurements[2], sizeof(measurements[2]),
						"Humidity: %.2f RH.\r\n", humidity);

				for (uint8_t i = 0; i < 3; i++) {
					Add_Tx_Message(measurements[i]);
				}

			} else {

				Add_Tx_Message("Read error\r\n");
			}

			forced_measurement_flag = 0;
		}
	}

	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

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
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
		Error_Handler();
	}
}

/* USER CODE BEGIN 4 */

/* Timer interrupt callback */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if (htim == &htim10) {

		timer_flag++;

		/* Wait required time before reading measurement result */
		if (timer_flag >= 2 && forced_timer_flag == 1) {

			forced_measurement_flag = 1;
			timer_flag = 0;
		}
	}
}

/* UART receive complete callback */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {

	if (huart == &huart2) {

		rx_flag = 1;
		HAL_UART_Receive_IT(&huart2, &rx_data, 1);
	}
}

/* UART transmit complete callback */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart == &huart2) {
		tx_busy = 0;
		UART_Send_Message();
	}
}

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */

	__disable_irq();

	while (1) {
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

  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
