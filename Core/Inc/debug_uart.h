#ifndef DEBUG_UART_H_
#define DEBUG_UART_H_

#include "stm32h5xx_hal.h"

// Initialize after MX_USART1_UART_Init() has been called
void debug_init(UART_HandleTypeDef *huart);

// Print a string (blocking, for debug only)
void debug_print(const char *str);

// Print a hex byte
void debug_hex8(uint8_t val);

// Print a hex word
void debug_hex16(uint16_t val);

// Print a hex dword
void debug_hex32(uint32_t val);

// Print string + newline
void debug_println(const char *str);

// Print a buffer as hex dump (up to 64 bytes)
void debug_dump(const char *label, const uint8_t *data, uint8_t len);

#endif
