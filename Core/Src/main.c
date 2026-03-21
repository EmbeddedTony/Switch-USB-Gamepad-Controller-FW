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
#include "usart.h"
#include "usb.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "tusb.h"
#include "usb_descriptors.h"
#include "debug_uart.h"
#include <string.h>
#include <stdlib.h>
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

// ---- UART Command Protocol ----
#define CMD_START 0xAA

// UART RX state machine — runs entirely in ISR for zero-latency re-arm
typedef enum { RX_WAIT_START, RX_WAIT_CMD, RX_WAIT_LF } rx_state_t;
static volatile rx_state_t rx_state = RX_WAIT_START;
static uint8_t    rx_byte;

// Command received from ISR, processed in main loop
static volatile uint8_t pending_cmd = 0;  // 'R', 'P', or 0 (none)

// Reset macro state: buttons for 500ms, then idle for 1500ms, then resume A
static volatile bool     reset_active = false;
static volatile uint32_t reset_start_ms = 0;
#define RESET_ABXY_MS   500   // hold all buttons (title screen reset)
static volatile uint32_t reset_pause_ms = 2000;  // randomized 2000-4000ms each reset

// Lightweight xorshift32 PRNG for randomizing A-press intervals
static uint32_t rng_state = 1;
static uint32_t xorshift32(void) {
    uint32_t x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng_state = x;
    return x;
}
// Returns a random value in [min, max] inclusive
static uint32_t rand_range(uint32_t min, uint32_t max) {
    return min + (xorshift32() % (max - min + 1));
}

static void send_ack(void) {
    uint8_t ack[] = { CMD_START, 'K', '\n' };
    HAL_UART_Transmit(&huart1, ack, 3, 10);
}

// Called from UART RX complete ISR — parse + re-arm immediately
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance != USART1) return;

    switch (rx_state) {
    case RX_WAIT_START:
        if (rx_byte == CMD_START) {
            rx_state = RX_WAIT_CMD;
        }
        break;

    case RX_WAIT_CMD:
        if (rx_byte == 'R' || rx_byte == 'P') {
            pending_cmd = rx_byte;
        }
        rx_state = RX_WAIT_LF;
        break;

    case RX_WAIT_LF:
        rx_state = RX_WAIT_START;
        break;
    }

    // Re-arm immediately — no gap between bytes
    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
}

// Start listening for one byte (non-blocking, interrupt-driven)
static void uart_listen(void) {
    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
}

// Process pending commands in main loop (debug prints + ACK)
static void uart_poll(void) {
    uint8_t cmd = pending_cmd;
    if (cmd == 0) return;
    pending_cmd = 0;

    switch (cmd) {
    case 'R':
        reset_active = true;
        reset_start_ms = HAL_GetTick();
        reset_pause_ms = rand_range(2000, 4000);
        send_ack();
        debug_println("[CMD] RESET");
        break;
    case 'P':
        send_ack();
        debug_println("[CMD] PING");
        break;
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
  MX_USB_PCD_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  // Debug UART (USART1 on PB14/PB15, initialized by CubeMX above)
  debug_init(&huart1);
  debug_println("=== Auto Switch Pro boot ===");

  // LED on at boot (PB2, active-low)
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);

  // Enable CRS to trim HSI48 using USB SOF for accurate 48 MHz
  __HAL_RCC_CRS_CLK_ENABLE();
  RCC_CRSInitTypeDef crs = {0};
  crs.Prescaler             = RCC_CRS_SYNC_DIV1;
  crs.Source                = RCC_CRS_SYNC_SOURCE_USB;
  crs.Polarity              = RCC_CRS_SYNC_POLARITY_RISING;
  crs.ReloadValue           = RCC_CRS_RELOADVALUE_DEFAULT;
  crs.ErrorLimitValue       = RCC_CRS_ERRORLIMIT_DEFAULT;
  crs.HSI48CalibrationValue = RCC_CRS_HSI48CALIBRATION_DEFAULT;
  HAL_RCCEx_CRSConfig(&crs);

  // Initialize TinyUSB (resets USB peripheral, opens EP0, enables D+ pull-up + NVIC)
  tusb_init();

  // Soft-reconnect: pull D+ low, wait, then re-assert.
  // The STM32H5 boots so fast that D+ goes high almost instantly after VBUS.
  // Strict USB hosts (like the Switch dock) need to see a clean transition.
  tud_disconnect();
  HAL_Delay(1000);
  tud_connect();

  debug_println("USB connected, waiting for host...");

  // Enable USART1 interrupt (CubeMX didn't enable it in NVIC)
  HAL_NVIC_SetPriority(USART1_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(USART1_IRQn);

  // Start non-blocking UART receive for command protocol
  uart_listen();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    // LED heartbeat: toggle every 500ms so we know firmware is alive
    {
      static uint32_t led_last = 0;
      uint32_t t = HAL_GetTick();
      if (t - led_last >= 500) {
        led_last = t;
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
      }
    }

    tud_task();

    // Service any queued protocol responses (handshake, subcommand replies)
    extern bool switch_pro_send_queued(void);
    switch_pro_send_queued();

    // Poll UART command protocol (non-blocking)
    uart_poll();

    // Physical user button (PC13, active-low) triggers reset macro
    {
      static uint32_t btn_debounce = 0;
      static bool btn_last = false;
      bool btn_now = (HAL_GPIO_ReadPin(user_button_GPIO_Port, user_button_Pin) == GPIO_PIN_RESET);
      if (btn_now && !btn_last && (HAL_GetTick() - btn_debounce > 200)) {
        btn_debounce = HAL_GetTick();
        reset_active = true;
        reset_start_ms = HAL_GetTick();
        reset_pause_ms = rand_range(2000, 4000);
        debug_println("[BTN] USR pressed -> RESET");
      }
      btn_last = btn_now;
    }

    // Only send 0x30 input reports after handshake is complete
    if (pro_state.reports_enabled) {
      static uint32_t last_report_ms = 0;
      uint32_t now = HAL_GetTick();
      // Send input reports every 8ms (~125 Hz)
      if (tud_hid_ready() && (now - last_report_ms >= 8)) {
        last_report_ms = now;

        switch_pro_report_t report;
        memset(&report, 0, sizeof(report));
        report.report_id = REPORT_ID_INPUT_30;
        report.timestamp = pro_state.report_counter++;

        // Battery good, USB connection
        report.input.battery_level = 0x08;
        report.input.connection_info = 0x00;
        report.input.charging = 1;

        // Left stick held up-center, right stick at center (12-bit: 0x7FF = 2047)
        switch_analog_set_xy(&report.input.left_stick,  0x7FF, 0xFFF);
        switch_analog_set_xy(&report.input.right_stick, 0x7FF, 0x7FF);

        // Vibrator report
        report.rumble_report = 0x00;

        // ---- Button automation ----
        static uint32_t phase_start = 0;
        if (phase_start == 0) phase_start = now;
        uint32_t elapsed = now - phase_start;

        if (elapsed < 3000) {
          // First 3 seconds: press L+R to register on Grip/Order screen
          report.input.btn_l = 1;
          report.input.btn_r = 1;
        } else if (reset_active) {
          // Reset macro: ABXY 500ms → idle 1500ms → resume A
          uint32_t reset_elapsed = now - reset_start_ms;
          static bool reset_logged = false;
          // Clear charging flag during reset to avoid any byte confusion
          report.input.charging = 0;
          if (reset_elapsed < RESET_ABXY_MS) {
            // Phase 1: hold A+B+X+Y+L+R+ZL+ZR to force game reset
            report.input.btn_a = 1;
            report.input.btn_b = 1;
            report.input.btn_x = 1;
            report.input.btn_y = 1;
            report.input.btn_l = 1;
            report.input.btn_r = 1;
            report.input.btn_zl = 1;
            report.input.btn_zr = 1;
            if (!reset_logged) {
              uint8_t *raw = (uint8_t *)&report;
              debug_print("[RST] ON  raw=");
              debug_hex8(raw[3]); debug_print(" ");
              debug_hex8(raw[4]); debug_print(" ");
              debug_hex8(raw[5]); debug_print("\r\n");
              reset_logged = true;
            }
          } else if (reset_elapsed < reset_pause_ms) {
            // Phase 2: release all buttons, let title screen load
            // (report already zeroed — no buttons pressed)
          } else {
            // Done — resume normal A presses
            reset_active = false;
            reset_logged = false;
            debug_println("[ABXY] DONE, resuming A");
          }
        } else {
          // Default: press A for 100ms, randomized interval 200-550ms
          static uint32_t a_press_start = 0;
          static uint32_t a_interval = 0;
          static bool     a_initialized = false;
          if (!a_initialized) {
            rng_state = HAL_GetTick() | 1;  // seed PRNG
            a_press_start = now;
            a_interval = rand_range(200, 550);
            a_initialized = true;
          }
          uint32_t a_elapsed = now - a_press_start;
          if (a_elapsed < 100) {
            report.input.btn_a = 1;
          } else if (a_elapsed >= a_interval) {
            // Start next press cycle with a new random interval
            a_press_start = now;
            a_interval = rand_range(200, 550);
            report.input.btn_a = 1;
          }
        }

        tud_hid_report(0, &report, sizeof(report));
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_CSI;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.CSIState = RCC_CSI_ON;
  RCC_OscInitStruct.CSICalibrationValue = RCC_CSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLL1_SOURCE_CSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 125;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1_VCIRANGE_2;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1_VCORANGE_WIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the programming delay
  */
  __HAL_FLASH_SET_PROGRAM_DELAY(FLASH_PROGRAMMING_DELAY_2);
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
