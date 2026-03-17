#include "debug_uart.h"
#include <string.h>

static UART_HandleTypeDef *debug_huart = NULL;

void debug_init(UART_HandleTypeDef *huart) {
    debug_huart = huart;
    debug_println("\r\n--- Switch Pro Controller Debug ---");
}

void debug_print(const char *str) {
    if (!debug_huart) return;
    HAL_UART_Transmit(debug_huart, (const uint8_t *)str, (uint16_t)strlen(str), 10);
}

static const char hex_chars[] = "0123456789ABCDEF";

void debug_hex8(uint8_t val) {
    if (!debug_huart) return;
    char buf[3];
    buf[0] = hex_chars[(val >> 4) & 0x0F];
    buf[1] = hex_chars[val & 0x0F];
    buf[2] = '\0';
    debug_print(buf);
}

void debug_hex16(uint16_t val) {
    debug_hex8((uint8_t)(val >> 8));
    debug_hex8((uint8_t)(val & 0xFF));
}

void debug_hex32(uint32_t val) {
    debug_hex16((uint16_t)(val >> 16));
    debug_hex16((uint16_t)(val & 0xFFFF));
}

void debug_println(const char *str) {
    debug_print(str);
    debug_print("\r\n");
}

void debug_dump(const char *label, const uint8_t *data, uint8_t len) {
    if (!debug_huart) return;
    debug_print(label);
    debug_print(": ");
    uint8_t n = (len > 64) ? 64 : len;
    for (uint8_t i = 0; i < n; i++) {
        debug_hex8(data[i]);
        debug_print(" ");
    }
    debug_print("\r\n");
}
