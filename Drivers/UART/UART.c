#include "stdio.h"
#include "stm32l4xx_hal.h"

#ifndef UART_DEF
#define UART_DEF		0
#define HAL_MAX_DELAY 1000

static UART_HandleTypeDef *g_uart_handle = NULL;

void UART_SetHandle(UART_HandleTypeDef *handle) {
    g_uart_handle = handle;
}

int __io_putchar(int ch) {
    if (g_uart_handle != NULL) {
        HAL_UART_Transmit(g_uart_handle, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    }
    return ch;
}

#endif
