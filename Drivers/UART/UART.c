#include "stdio.h"
#include "main.c"
#define HAL_MAX_DELAY 1000

int __io_putchar(int ch) {
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

